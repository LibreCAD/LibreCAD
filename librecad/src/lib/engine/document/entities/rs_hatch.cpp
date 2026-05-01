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

#include <algorithm>
#include <iostream>
#include <set>
#include <memory>
#include <vector>

#include <QPainterPath>

#include "lc_containertraverser.h"
#include "lc_looputils.h"
#include "rs_debug.h"
#include "rs_hatch.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"
#include "rs_pen.h"

namespace {
// Removes zero-length entities from the container
void avoidZeroLength(std::set<RS_Entity*>& container) {
    std::set<RS_Entity*> toCleanUp;
    for (RS_Entity* e : container) {
        if (e != nullptr && RS_Math::equal(e->getLength(), 0.)) {
            toCleanUp.insert(e);
        }
    }
    for (RS_Entity* e : toCleanUp) {
        container.erase(e);
    }
}

void collectSingle(std::vector<std::shared_ptr<RS_EntityContainer>>& collected,
                   const LC_LoopUtils::LC_Loops& lcLoops)
{
    collected.push_back(lcLoops.getOuterLoop());
    for(const LC_LoopUtils::LC_Loops& child: lcLoops.getChildren())
        collectSingle(collected, child);
}

std::vector<std::shared_ptr<RS_EntityContainer>> collectLoops(std::shared_ptr<std::vector<LC_LoopUtils::LC_Loops>> lcLoopsVPtr)
{
    if (lcLoopsVPtr==nullptr)
        return {};
    std::vector<std::shared_ptr<RS_EntityContainer>> collected;
    for(const LC_LoopUtils::LC_Loops& loop: *lcLoopsVPtr)
        collectSingle(collected, loop);
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
    , data(d)
    , m_area(RS_MAXDOUBLE)
    , updateError(HATCH_UNDEFINED)
    , updateRunning(false)
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
    cloneHatch->setOwner(isOwner());
    cloneHatch->detach();

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
        // Collect atomic entities from loop subcontainers into a deduplicated set
        std::set<RS_Entity*> contourEdges;
        std::vector<RS_Entity*> loops;
        for(RS_Entity* en: std::as_const(*this)) {
            if (en == nullptr || ! en->isContainer())
                continue;

            lc::LC_ContainerTraverser traverser{*static_cast<RS_EntityContainer*>(en), RS2::ResolveAll};
            for (RS_Entity* entity : traverser.entities()) {
                if (entity != nullptr && entity->isAtomic()) {
                    contourEdges.insert(entity);
                }
            }
            loops.push_back(en);
        }

        // Now safely clear the original container (temporarily disable ownership)
        setOwner(false);
        clear();
        setOwner(true);
        std::copy(loops.begin(), loops.end(), std::back_inserter(*this));

        // Clean up zero-length entities in the flat container
        avoidZeroLength(contourEdges);

        // Build a working copy of the contour edges (owned clones) so that any
        // coordinate transforms applied for pattern alignment do not mutate the
        // original boundary entities stored in *this.
        RS_EntityContainer edgeContainer{nullptr, true};
        for (RS_Entity* e : contourEdges)
            edgeContainer.addEntity(e->clone());
        edgeContainer.forcedCalculateBorders();

        // For pattern hatches: rotate the working copy by -angle so that tiling
        // can be done in an axis-aligned frame; updatePatternHatch rotates the
        // trimmed lines back by +angle.  Solid hatches keep WCS coordinates so
        // that solid-fill paths and moments are computed correctly.
        if (!isSolid()) {
            const RS_Vector center = (edgeContainer.getMin() + edgeContainer.getMax()) * 0.5;
            edgeContainer.rotate(center, -data.angle);
        }

        LC_LoopUtils::LoopOptimizer optimizer{edgeContainer};
        auto results = optimizer.GetResults();

        if (!results || results->empty())
            throw std::runtime_error("Loop optimization failed: no loops generated");

        m_orderedLoops = results;
        m_needOptimization = false;

        // Collect all loops (outer + holes) into flat boundary container list
        m_boundaryContainers = collectLoops(results);
    } catch (const std::exception& e) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "Hatch validation error: %s", e.what());
        updateError = HATCH_INVALID_CONTOUR;
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

/**
 * @return Subcontainer for the boundary loop at index (0-based).
 */
RS_EntityContainer* RS_Hatch::getBoundaryContainer(int loopIndex) const {
    if (loopIndex >= 0 && loopIndex < static_cast<int>(m_boundaryContainers.size())) {
        return m_boundaryContainers[loopIndex].get();
    }
    return nullptr;
}

/**
 * Calculates the bounding box, temporarily activating contours for accurate computation.
 */
void RS_Hatch::calculateBorders() {
    RS_DEBUG->print("RS_Hatch::calculateBorders");
    activateContour(true);
    RS_EntityContainer::calculateBorders();
    RS_DEBUG->print("RS_Hatch::calculateBorders: size: %f,%f", getSize().x, getSize().y);
    activateContour(false);
}

/**
 * Updates the hatch by validating boundaries, generating fill paths or trimmed patterns directly in container,
 * and caching results. Skips if already updating or disabled.
 * Sets updateError on failure.
 */
void RS_Hatch::update() {
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update");

    // Early exits for invalid states
    if (updateRunning) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Already updating, skipping");
        return;
    }
    if (!updateEnabled) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Updates disabled, skipping");
        return;
    }
    if (isUndone()) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_Hatch::update: Undone hatch, skipping");
        return;
    }
    activateContour(true);  // active for loop validation

    updateRunning = true;
    updateError = HATCH_OK;

    // Reset caches
    m_area = RS_MAXDOUBLE;
    m_secondMomentValid = false;

    // Validate and optimize loops (moves boundaries to subcontainers)
    if (!validate()) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::update: Validation failed");
        activateContour(false);
        updateRunning = false;
        return;
    }

    // Store original attributes for child entities
    RS_Layer* layer = getLayer();
    RS_Pen pen = getPen();

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
    updateRunning = false;
    m_updated = true;

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_Hatch::update: Completed successfully");
}

/**
 * Helper: Generates and caches QPainterPaths for solid fill from optimized loops.
 */
void RS_Hatch::updateSolidHatch([[maybe_unused]] RS_Layer* layer, [[maybe_unused]] const RS_Pen& pen) {
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
    std::unique_ptr<RS_Pattern> pattern = RS_PATTERNLIST->requestPattern(data.pattern);
    if (!pattern) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::updatePatternHatch: Pattern '%s' not found",
                        data.pattern.toUtf8().constData());
        updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    }

    // Clone to avoid modifying the shared pattern
    pattern.reset(dynamic_cast<RS_Pattern*>(pattern->clone()));
    if (!pattern) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Hatch::updatePatternHatch: Failed to clone pattern");
        updateError = HATCH_PATTERN_NOT_FOUND;
        return;
    }
    pattern->setOwner(true);

    pattern->scale((pattern->getMin() + pattern->getMax()) * 0.5, RS_Vector{data.scale, data.scale});
    pattern->calculateBorders();
    forcedCalculateBorders();

    // Sanity checks: Ensure sizes are reasonable to avoid computation issues
    RS_Vector patternSize = pattern->getSize();
    RS_Vector contourSize = getSize();
    if (contourSize.x < 1e-6 || contourSize.y < 1e-6 ||
        patternSize.x < 1e-6 || patternSize.y < 1e-6 ||
        contourSize.x > RS_MAXDOUBLE - 1 || contourSize.y > RS_MAXDOUBLE - 1 ||
        patternSize.x > RS_MAXDOUBLE - 1 || patternSize.y > RS_MAXDOUBLE - 1) {
        LC_ERR<<"RS_Hatch::"<<__func__<<"(): contour or pattern size too small, "
               "contour=["<<contourSize.x<<'x'<<contourSize.y
               <<"], patternSize=["<<patternSize.x<<'x'<<patternSize.y<<']';
        updateError = HATCH_TOO_SMALL;
        return;
    }
    double areaRatio = (contourSize.x * contourSize.y) / (patternSize.x * patternSize.y);
    if (areaRatio > 1e4) {
        LC_ERR<<"RS_Hatch::"<<__func__<<"(): contour to pattern ratio too large, "
                                              "contour=["<<contourSize.x<<'x'<<contourSize.y
               <<"], patternSize=["<<patternSize.x<<'x'<<patternSize.y<<']';
        updateError = HATCH_AREA_TOO_BIG;
        return;
    }

    // The contour was optimized in a frame rotated by -data.angle (see validate()).
    // Rotate the trimmed pattern lines back to WCS by +data.angle.
    const RS_Vector center = (getMin() + getMax()) * 0.5;
    const RS_Vector rotationVector{data.angle};
    int addedCount = 0;
    for (int i = 0; i < static_cast<int>(m_orderedLoops->size()); ++i) {
        const auto& loop = (*m_orderedLoops)[i];
        auto trimmedEntities = loop.trimPatternEntities(*pattern);
        for (RS_Entity* entity : *trimmedEntities) {
            if (entity) {
                entity->setPen(pen);
                entity->setLayer(layer);
                entity->reparent(this);  // Reparent to RS_Hatch
                entity->setFlag(RS2::FlagHatchChild);
                entity->rotate(center, rotationVector);
                addEntity(entity);  // Transfers ownership; direct child
                ++addedCount;
            }
        }
        trimmedEntities->setOwner(false);  // Release after transfer
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
void RS_Hatch::activateContour(bool visible) {
    RS_DEBUG->print("RS_Hatch::activateContour: %d", static_cast<int>(visible));
    for (const auto& sub : m_boundaryContainers) {
        if (sub) {
            for (RS_Entity* entity : *sub) {
                if (!entity->isUndone() && !entity->getFlag(RS2::FlagTemp)) {
                    RS_DEBUG->print("RS_Hatch::activateContour: Setting visibility for entity %d",
                                    entity->getId());
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
    if (isSolid())
        drawSolidFill(painter);
    else
        drawPatternLines(painter);
}

/**
 * Helper: Draws solid fill using cached QPainterPaths.
 */
void RS_Hatch::drawSolidFill(RS_Painter* painter) {
    if (m_orderedLoops->empty())
        return;

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
        if (subEntity && !subEntity->isContainer() && subEntity->getFlag(RS2::FlagHatchChild)) {
            subEntity->setSelected(selected);
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
    if (!m_secondMomentValid || m_area < RS_TOLERANCE)
        return RS_Vector(false);
    return RS_Vector(m_firstMoment.mx / m_area, m_firstMoment.my / m_area);
}

LC_SecondMoment RS_Hatch::getMomentOfInertia() const {
    if (!m_secondMomentValid || m_area < RS_TOLERANCE)
        return {};
    const double cx = m_firstMoment.mx / m_area;
    const double cy = m_firstMoment.my / m_area;
    return m_secondMoment.getCentral(m_area, cx, cy);
}


double RS_Hatch::getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity,
                                    [[maybe_unused]] RS2::ResolveLevel level, [[maybe_unused]] double solidDist) const
{
    // Check if point is on the solid fill/hatch
    for (const auto& loop : *m_orderedLoops) {
        if (loop.isInside(coord)) {
            if (entity) {
                *entity = const_cast<RS_Hatch*>(this);
            }
            return 0.0;
        }
    }
    return RS_MAXDOUBLE;
}

void RS_Hatch::move(const RS_Vector& offset) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this))
        if(en->isContainer())
            en->move(offset);
    update();
}

void RS_Hatch::rotate(const RS_Vector& center, double angle) {
    rotate(center, RS_Vector{angle});
}

void RS_Hatch::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this))
        if(en->isContainer())
            en->rotate(center, angleVector);
    data.angle = RS_Math::correctAngle(data.angle + angleVector.angle());
    update();
}

void RS_Hatch::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this))
        if(en->isContainer())
            en->scale(center, factor);
    data.scale *= factor.x;
    update();
}

void RS_Hatch::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this))
        if(en->isContainer())
            en->mirror(axisPoint1, axisPoint2);
    // Reflecting a direction α across an axis at angle θ maps it to 2θ − α.
    double mirrorAngle = axisPoint1.angleTo(axisPoint2);
    data.angle = RS_Math::correctAngle(2.0 * mirrorAngle - data.angle);
    update();
}

void RS_Hatch::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner,
                       const RS_Vector& offset) {
    m_needOptimization = true;
    for(RS_Entity* en: std::as_const(*this))
        if(en->isContainer())
            en->stretch(firstCorner, secondCorner, offset);
    update();
}

std::ostream& operator<<(std::ostream& os, const RS_Hatch& hatchEntity) {
    os << "Hatch: " << hatchEntity.getData() << "\n";
    return os;
}
