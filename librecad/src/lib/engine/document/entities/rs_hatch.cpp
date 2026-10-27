/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_hatch.h"

#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "lc_containertraverser.h"
#include "lc_loopclassifier.h"
#include "lc_looputils.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_pen.h"

namespace {

// Removes zero-length entities from the vector in place.
void avoidZeroLength(std::vector<RS_Entity*>& container) {
    container.erase(std::remove_if(container.begin(), container.end(),
                                   [](RS_Entity* e) {
                                       return e != nullptr && RS_Math::equal(e->getLength(), 0.);
                                   }),
                    container.end());
}

/** True if entity is a closed atomic curve with no valid vertex endpoints. */
bool isClosedAtomic(const RS_Entity& e) {
    switch (e.rtti()) {
    case RS2::EntityCircle:
        return true;
    case RS2::EntityEllipse: {
        const auto& ell = static_cast<const RS_Ellipse&>(e);
        return !ell.isArc() || std::abs(ell.getAngleLength() - 2.0 * M_PI) < RS_TOLERANCE_ANGLE;
    }
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(e);
        return std::abs(arc.getAngleLength() - 2.0 * M_PI) < RS_TOLERANCE_ANGLE;
    }
    case RS2::EntitySplinePoints: {
        const auto& sp = static_cast<const LC_SplinePoints&>(e);
        return sp.isClosed();
    }
    default:
        return false;
    }
}

void collectSingle(std::vector<std::shared_ptr<RS_EntityContainer>>& collected,
                   const LC_LoopUtils::LC_Loops& lcLoops)
{
    collected.push_back(lcLoops.getOuterLoop());
    for(const LC_LoopUtils::LC_Loops& child: lcLoops.getChildren()) {
        collectSingle(collected, child);
    }
}

std::vector<std::shared_ptr<RS_EntityContainer>> collectLoops(const std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>>& lcLoopsVPtr)
{
    if (lcLoopsVPtr==nullptr) {
        return {};
    }
    std::vector<std::shared_ptr<RS_EntityContainer>> collected;
    for(const LC_LoopUtils::LC_Loops& loop: *lcLoopsVPtr) {
        collectSingle(collected, loop);
    }
    return collected;
}
}  // anonymous namespace

std::ostream& operator<<(std::ostream& os, const RS_HatchData& td) {
    os << "(" << td.pattern.toLatin1().data() << ")";
    return os;
}

// RS_Hatch implementation
RS_Hatch::RS_Hatch(RS_EntityContainer* parent, const RS_HatchData& d)
    : RS_EntityContainer(parent)
    , m_data(d)
    , m_area(RS_MAXDOUBLE)
    , m_updateError(HATCH_UNDEFINED)
    , m_updateRunning(false)
    , m_needOptimization(true)
    , m_updated(false)
    , m_orderedLoops{std::make_shared<std::vector<LC_LoopUtils::LC_Loops>>()}
    , m_boundaryContainers()
{
    setOwner(true);
}

RS_Hatch::~RS_Hatch() = default;

RS_Entity* RS_Hatch::clone() const {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone()");
    auto* cloneHatch = new RS_Hatch(*this);

    // Force re-optimization and update to deep-copy caches and subcontainers
    cloneHatch->m_needOptimization = true;
    cloneHatch->m_area = RS_MAXDOUBLE;
    cloneHatch->m_firstMoment = LC_FirstMoment{};
    cloneHatch->m_secondMomentValid = false;
    cloneHatch->m_updated = false;
    cloneHatch->m_boundaryContainers.clear();  // Will be regenerated
    cloneHatch->update();

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::clone(): OK");
    return cloneHatch;
}

bool RS_Hatch::validate() {
    bool success = true;

    if (!m_needOptimization || count() == 0) {
        return success;
    }

    try {
      // Collect child loop containers and rebuild the container list.
      std::vector<RS_Entity *> loops;
      for (RS_Entity *en : std::as_const(*this)) {
        if (en != nullptr && en->isContainer())
          loops.push_back(en);
      }
        setOwner(false);
        clear();
        setOwner(true);
        std::copy(loops.begin(), loops.end(), std::back_inserter(*this));

        // Pre-pass: pre-rotation bbox for the pattern rotation centre, which
        // must be the same for every container so that all loops rotate into
        // the same axis-aligned frame.
        RS_Vector preMin(RS_MAXDOUBLE, RS_MAXDOUBLE);
        RS_Vector preMax(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
        for (RS_Entity *en : loops) {
          auto *cont = static_cast<RS_EntityContainer *>(en);
          cont->forcedCalculateBorders();
          preMin.x = std::min(preMin.x, cont->getMin().x);
          preMin.y = std::min(preMin.y, cont->getMin().y);
          preMax.x = std::max(preMax.x, cont->getMax().x);
          preMax.y = std::max(preMax.y, cont->getMax().y);
        }
        const RS_Vector rotCenter = (preMin + preMax) * 0.5;

        // Pass 1: per-container clone+rotate, accumulate the post-rotation
        // bbox D and the endpoint extremum M for the per-hatch tolerance.
        // Closed atomic curves (circle, full ellipse, closed spline) have
        // no valid vertex endpoints; include their finite bbox extrema in M.
        std::vector<std::unique_ptr<RS_EntityContainer>> perContainerClones;
        RS_Vector bbMin(RS_MAXDOUBLE, RS_MAXDOUBLE);
        RS_Vector bbMax(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
        double maxAbsCoord = 0.0;
        for (RS_Entity *en : loops) {
          std::vector<RS_Entity *> edges;
          std::unordered_set<const RS_Entity *> seen;
          lc::LC_ContainerTraverser traverser{
              *static_cast<RS_EntityContainer *>(en), RS2::ResolveAll};
          for (RS_Entity *entity : traverser.entities()) {
            if (entity != nullptr && entity->isAtomic() && seen.insert(entity).second) {
              edges.push_back(entity);
            }
          }
          avoidZeroLength(edges);
          if (edges.empty())
            continue;

          // A clone keeps its source's parent and layer, and so does every clone
          // made of it, down to the cached loops. Those outlive both: drop them.
          auto perLoopCont = std::make_unique<RS_EntityContainer>(nullptr, true);
          for (RS_Entity *e : edges) {
            RS_Entity *edge = e->clone();
            edge->setParent(nullptr);
            edge->setLayer(nullptr);
            perLoopCont->addEntity(edge);
          }

          // For pattern hatches apply the rotation before extraction so the
          // extracted loop coordinates are in the axis-aligned tiling frame.
          if (!isSolid())
            perLoopCont->rotate(rotCenter, -m_data.angle);
          perLoopCont->forcedCalculateBorders();

          bbMin.x = std::min(bbMin.x, perLoopCont->getMin().x);
          bbMin.y = std::min(bbMin.y, perLoopCont->getMin().y);
          bbMax.x = std::max(bbMax.x, perLoopCont->getMax().x);
          bbMax.y = std::max(bbMax.y, perLoopCont->getMax().y);

          for (RS_Entity *e : *perLoopCont) {
            if (e == nullptr) continue;
            if (isClosedAtomic(*e)) {
              const RS_Vector mn = e->getMin();
              const RS_Vector mx = e->getMax();
              maxAbsCoord = std::max({maxAbsCoord,
                                      std::abs(mn.x), std::abs(mn.y),
                                      std::abs(mx.x), std::abs(mx.y)});
            } else {
              const RS_Vector s = e->getStartpoint();
              const RS_Vector t = e->getEndpoint();
              maxAbsCoord = std::max({maxAbsCoord,
                                      std::abs(s.x), std::abs(s.y),
                                      std::abs(t.x), std::abs(t.y)});
            }
          }

          perContainerClones.push_back(std::move(perLoopCont));
        }

        const double diagonal = (perContainerClones.empty())
                                   ? 0.0
                                   : (bbMax - bbMin).magnitude();
        const double tolerance = LC_LoopUtils::hatchTolerance(diagonal, maxAbsCoord);

        // Pass 2: extract each container's loops with their vertex tables,
        // and run them through the geometry-aware LoopSorter.
        std::vector<LC_LoopUtils::ExtractedLoop> allExtracted;
        for (auto &cont : perContainerClones) {
          LC_LoopUtils::LoopExtractor extractor{*cont};
          auto extracted = extractor.extractLoops();
          for (auto &el : extracted)
            allExtracted.push_back(std::move(el));
        }

        if (allExtracted.empty())
          throw std::runtime_error(
              "Loop optimization failed: no loops generated");

        LC_LoopUtils::LoopSorter sorter{std::move(allExtracted), tolerance};
        auto results = sorter.getResults();

        if (!results || results->empty()) {
            throw std::runtime_error("Loop optimization failed: no loops generated");
        }

        m_orderedLoops = results;
        // Remember the frame of m_orderedLoops: validate() rotated by
        // -m_data.angle about rotCenter. H02's lattice anchor is mapped
        // through this stored pair, and H04/H07/H08 read them too.
        m_pivot = rotCenter;
        m_loopAngle = isSolid() ? 0.0 : m_data.angle;
        m_tolerance = tolerance;
        m_needOptimization = false;

        // Collect all loops (outer + holes) into flat boundary container list
        m_boundaryContainers = collectLoops(results);
    } catch (const std::exception& e) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "Hatch validation error: %s", e.what());
        m_updateError = HATCH_INVALID_CONTOUR;
        success = false;
    }

    return success;
}

/**
 * Counts the number of optimized boundary loops (subcontainers).
 * For solid hatches, this represents the fill areas.
 * For patterns, it represents regions to trim into.
 *
 * @return Number of loops.
 */
int RS_Hatch::countLoops() const {
    return static_cast<int>(m_orderedLoops->size());
}

int RS_Hatch::countAllLoops() const {
  return static_cast<int>(m_boundaryContainers.size());
}

/**
 * @return Subcontainer for the boundary loop at index (0-based).
 */
RS_EntityContainer* RS_Hatch::getBoundaryContainer(const int loopIndex) const {
    if (loopIndex >= 0 && loopIndex < static_cast<int>(m_boundaryContainers.size())) {
        return m_boundaryContainers[loopIndex].get();
    }
    return nullptr;
}

/**
 * Calculates the bounding box as update() does. The contour bounds the hatch
 * even while the layer its edges were picked from is hidden.
 */
void RS_Hatch::calculateBorders() {
    forcedCalculateBorders();
}

/**
 * Updates the hatch by validating boundaries, generating fill paths or trimmed patterns directly in container,
 * and caching results. Skips if already updating or disabled.
 * Sets updateError on failure.
 */
void RS_Hatch::update() {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update");

    // Early exits for invalid states
    if (m_updateRunning) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Already updating, skipping");
        return;
    }
    if (!m_updateEnabled) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Updates disabled, skipping");
        return;
    }
    if (isDeleted()) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Undone hatch, skipping");
        return;
    }
    activateContour(true);  // active for loop validation

    m_updateRunning = true;
    m_updateError = HATCH_OK;

    // Delete the pattern lines of an earlier update: validate() drops them only
    // when the loops need optimizing.
    std::vector<RS_Entity*> loops;
    std::vector<RS_Entity*> patternLines;
    for (RS_Entity* en : std::as_const(*this)) {
        (en->isContainer() ? loops : patternLines).push_back(en);
    }
    setOwner(false);
    clear();
    setOwner(true);
    std::copy(loops.begin(), loops.end(), std::back_inserter(*this));
    for (RS_Entity* en : patternLines) {
        delete en;
    }

    // Reset caches
    m_area = RS_MAXDOUBLE;
    m_secondMomentValid = false;

    // Validate and optimize loops (moves boundaries to subcontainers)
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: Validation failed");
        // Surviving contours must remain bounded when a copy cannot revalidate.
        forcedCalculateBorders();
        activateContour(false);
        m_updateRunning = false;
        return;
    }

    // Store original attributes for child entities
    RS_Layer* layer = getLayer();
    const RS_Pen pen = getPen();

    if (isSolid()) {
        updateSolidHatch(layer, pen);
    } else {
        updatePatternHatch(layer, pen);
    }

    // Compute 0th, 1st, and 2nd moments from loops
    m_area = 0.0;
    m_firstMoment = LC_FirstMoment{};
    m_secondMoment = LC_SecondMoment{};
    for (const auto& loop : *m_orderedLoops) {
        m_area         += loop.getTotalArea();
        m_firstMoment  += loop.getTotalFirstMoment();
        m_secondMoment += loop.getTotalSecondMoment();
    }
    m_secondMomentValid = true;

    forcedCalculateBorders();
    activateContour(false);  // Hide boundaries by default
    m_updateRunning = false;
    m_updated = true;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: Completed successfully");
}

/**
 * Helper: Generates and caches QPainterPaths for solid fill from optimized loops.
 */
void RS_Hatch::updateSolidHatch([[maybe_unused]] RS_Layer* layer, [[maybe_unused]] const RS_Pen& pen) const{
    // Solid-fill paths are viewport-dependent (world→screen transform varies per
    // draw call), so they are built lazily in drawSolidFill() rather than cached here.
}

/**
 * Helper: Loads, scales, rotates, and trims pattern into direct children of RS_Hatch.
 * Performs sanity checks on sizes to prevent overflows or invalid operations.
 */
void RS_Hatch::updatePatternHatch(RS_Layer* layer, const RS_Pen& pen) {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::updatePatternHatch");

    // Request and clone pattern
    std::unique_ptr<RS_Pattern> pattern = RS_PATTERNLIST->requestPattern(m_data.pattern);
    if (!pattern) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::updatePatternHatch: Pattern '%s' not found",
                        m_data.pattern.toUtf8().constData());
        m_updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    }

    // Clone to avoid modifying the shared pattern
    pattern.reset(dynamic_cast<RS_Pattern*>(pattern->clone()));
    if (!pattern) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::updatePatternHatch: Failed to clone pattern");
        m_updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    }
    pattern->setOwner(true);

    pattern->scale((pattern->getMin() + pattern->getMax()) * 0.5, RS_Vector{m_data.scale, m_data.scale});
    pattern->calculateBorders();
    forcedCalculateBorders();

    // Sanity checks: Ensure sizes are reasonable to avoid computation issues
    const RS_Vector patternSize = pattern->getSize();
    const RS_Vector contourSize = getSize();
    if (contourSize.x < 1e-6 || contourSize.y < 1e-6 ||
        patternSize.x < 1e-6 || patternSize.y < 1e-6 ||
        contourSize.x > RS_MAXDOUBLE - 1 || contourSize.y > RS_MAXDOUBLE - 1 ||
        patternSize.x > RS_MAXDOUBLE - 1 || patternSize.y > RS_MAXDOUBLE - 1) {
        LC_ERR<<"RS_Hatch::"<<__func__<<"(): contour or pattern size too small, "
               "contour=["<<contourSize.x<<'x'<<contourSize.y
               <<"], patternSize=["<<patternSize.x<<'x'<<patternSize.y<<']';
        m_updateError = HATCH_TOO_SMALL;
        return;
    }
    const double areaRatio = (contourSize.x * contourSize.y) / (patternSize.x * patternSize.y);
    if (areaRatio > 1e4) {
        LC_ERR<<"RS_Hatch::"<<__func__<<"(): contour to pattern ratio too large, "
                                              "contour=["<<contourSize.x<<'x'<<contourSize.y
               <<"], patternSize=["<<patternSize.x<<'x'<<patternSize.y<<']';
        m_updateError = HATCH_AREA_TOO_BIG;
        return;
    }

    // The contour was optimized in a frame rotated by -m_loopAngle about
    // m_pivot (see validate()). The lattice anchor is one WCS point (A9/D30:
    // latticeOrigin() = (0, 0)) mapped into that frame; every root uses it,
    // so two roots of one hatch share the WCS phase and a non-lattice move of
    // the boundary does not drag the pattern. The trimmed pieces are rotated
    // back to WCS by +m_loopAngle about the same m_pivot.
    RS_Vector anchor = latticeOrigin();
    anchor.rotate(m_pivot, RS_Vector{-m_loopAngle});
    const RS_Vector rotationVector{m_loopAngle};
    int addedCount = 0;
    for (size_t i = 0; i < m_orderedLoops->size(); ++i) {
        const auto& loop = (*m_orderedLoops)[i];
        for (auto& owned : loop.trimPatternEntities(*pattern, anchor)->takeEntities()) {
            RS_Entity* entity = owned.release();
            entity->setPen(pen);
            entity->setLayer(layer);
            entity->reparent(this);
            entity->setFlag(RS2::FlagHatchChild);
            entity->rotate(m_pivot, rotationVector);
            addEntity(entity);
            ++addedCount;
        }
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::updatePatternHatch: Added %d direct entities",
                    addedCount);
}

/**
 * Toggles visibility of boundary contour entities in subcontainers.
 * Used during border calculation or for debugging.
 *
 * @param visible true to show boundaries, false to hide.
 */
void RS_Hatch::activateContour(const bool visible) const {
    RS_DEBUG->print("RS_Hatch::activateContour: %d", static_cast<int>(visible));
    for (const auto& sub : m_boundaryContainers) {
        if (sub) {
            for (RS_Entity* entity : *sub) {
                if (!entity->isDeleted() && !entity->getFlag(RS2::FlagTemp)) {
                    RS_DEBUG->Log()
                        << "RS_Hatch::activateContour: Setting visibility for entity "
                        << entity->getId();
                    entity->setVisible(visible);
                }
            }
        }
    }
    RS_DEBUG->print("RS_Hatch::activateContour: OK");
}

/**
 * Draws the hatch: solid fill via cached paths or pattern lines via direct children.
 * Boundaries (in subcontainers) are not drawn by default (use activateContour for that).
 *
 * @param painter The painter to draw with.
 */
void RS_Hatch::draw(RS_Painter* painter) {
    if (isSolid()) {
        drawSolidFill(painter);
    }
    else {
        drawPatternLines(painter);
    }
}

/**
 * Helper: Draws solid fill using cached QPainterPaths.
 */
void RS_Hatch::drawSolidFill(RS_Painter* painter) const {
    if (m_orderedLoops->empty()){
        return;
    }

    painter->save();
    QBrush fillBrush = painter->brush();
    fillBrush.setColor(painter->getPen().getColor());
    fillBrush.setStyle(Qt::SolidPattern);
    painter->setBrush(fillBrush);

    for (const LC_LoopUtils::LC_Loops& loop : *m_orderedLoops) {
        QPainterPath path = loop.getPainterPath(painter);
        painter->drawPath(path);
    }

    painter->restore();
}

/**
 * Helper: Draws pattern lines from direct atomic children (trimmed entities).
 * Skips subcontainers (boundaries).
 */
void RS_Hatch::drawPatternLines(RS_Painter* painter) const {
    const bool selected = isSelected();
    for (RS_Entity* subEntity : *this) {
        // Draw only direct atomic children with FlagHatchChild (patterns); skip subcontainers
        if ((subEntity != nullptr) && !subEntity->isContainer() && subEntity->getFlag(RS2::FlagHatchChild)) {
            subEntity->setSelectionFlag(selected); // fixme - selection - just utility entity used on draw?
            painter->drawEntity(subEntity);
        }
    }
}

/**
 * @return Total enclosed area, or RS_MAXDOUBLE if update() has not run.
 */
double RS_Hatch::getTotalArea() const {
    return m_area;
}

RS_Vector RS_Hatch::getCentroid() const {
    if (!m_secondMomentValid || m_area < RS_TOLERANCE) {
        return RS_Vector(false);
    }
    return RS_Vector(m_firstMoment.mx / m_area, m_firstMoment.my / m_area);
}

LC_SecondMoment RS_Hatch::getMomentOfInertia() const {
    if (!m_secondMomentValid || m_area < RS_TOLERANCE) {
        return {};
    }
    const double cx = m_firstMoment.mx / m_area;
    const double cy = m_firstMoment.my / m_area;
    return m_secondMoment.getCentral(m_area, cx, cy);
}


double RS_Hatch::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity,
                                    [[maybe_unused]] RS2::ResolveLevel level, [[maybe_unused]] double solidDist) const{
    // validate() cached the loops turned by -m_loopAngle about m_pivot: test
    // the point in that frame so picking agrees with what is drawn at every
    // angle. For solid hatches m_loopAngle is 0 and this is the identity.
    RS_Vector query = coord;
    if (m_loopAngle != 0.0) {
        query.rotate(m_pivot, -m_loopAngle);
    }
    // Check if point is on the solid fill/hatch
    for (const auto& loop : *m_orderedLoops) {
        if (loop.isInside(query)) {
            if (entity != nullptr) {
                *entity = const_cast<RS_Hatch*>(this);
            }
            return 0.0;
        }
    }
    return RS_MAXDOUBLE;
}

void RS_Hatch::setAngle(double angle) {
    m_data.angle = RS_Math::correctAngle(angle);
    m_needOptimization = true;
}

void RS_Hatch::move(const RS_Vector& offset) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this)) {
        if(en->isContainer()) {
            en->move(offset);
        }
    }
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, const double angle) {
    rotate(center, RS_Vector{angle});
}

void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_needOptimization = true;
    for (RS_Entity* en : std::as_const(*this)) {
        if (en->isContainer()) {
            en->rotate(center, angleVector);
        }
    }
    m_data.angle = RS_Math::correctAngle(m_data.angle + angleVector.angle());
    update();
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this)) {
        if(en->isContainer()) {
            en->scale(center, factor);
        }
    }
    m_data.scale *= factor.x;
    update();
}

void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this)) {
        if(en->isContainer()) {
            en->mirror(axisPoint1, axisPoint2);
        }
    }
    // Reflecting a direction α across an axis at angle θ maps it to 2θ − α.
    double mirrorAngle = axisPoint1.angleTo(axisPoint2);
    m_data.angle = RS_Math::correctAngle(2.0 * mirrorAngle - m_data.angle);
    update();
}

void RS_Hatch::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner,
                       const RS_Vector& offset) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this)) {
        if(en->isContainer()) {
            en->stretch(firstCorner, secondCorner, offset);
        }
    }
    update();
}

std::ostream& operator<<(std::ostream& os, const RS_Hatch& hatchEntity) {
    os << "Hatch: " << hatchEntity.getData() << "\n";
    return os;
}
