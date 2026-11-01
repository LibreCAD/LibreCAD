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
#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

#include "lc_containertraverser.h"
#include "lc_hatchdensity.h"
#include "lc_loopclassifier.h"
#include "lc_looputils.h"
#include "lc_patternlinededup.h"
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

// Removes entities whose length is at or below the per-hatch tolerance.
void avoidZeroLength(std::vector<RS_Entity*>& container, double eps) {
    container.erase(std::remove_if(container.begin(), container.end(),
                                   [eps](RS_Entity* e) {
                                       return e == nullptr || !(e->getLength() > eps);
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
    cloneHatch->m_centroid = RS_Vector(false);
    cloneHatch->m_centralMoment = {};
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

        // Pre-eps: a quick scale-relative tolerance computed from the raw
        // bbox and endpoint extrema BEFORE rotation. Rotation is rigid, so
        // the diagonal is identical post-rotation and M differs by at most
        // sqrt(2); the resulting pre-eps shares the magnitude of the final
        // tolerance and only drives avoidZeroLength (the length filter is
        // rotation-invariant). The final tolerance is recomputed after
        // rotation and is what the extractor and sorter see.
        double preMaxAbs = 0.0;
        for (RS_Entity *en : loops) {
          lc::LC_ContainerTraverser t{
              *static_cast<RS_EntityContainer *>(en), RS2::ResolveAll};
          for (RS_Entity *entity : t.entities()) {
            if (!entity || !entity->isAtomic()) continue;
            if (isClosedAtomic(*entity)) {
              const RS_Vector mn = entity->getMin();
              const RS_Vector mx = entity->getMax();
              if (mn.valid && mx.valid) {
                preMaxAbs = std::max({preMaxAbs,
                                        std::abs(mn.x), std::abs(mn.y),
                                        std::abs(mx.x), std::abs(mx.y)});
              }
            } else {
              const RS_Vector s = entity->getStartpoint();
              const RS_Vector tp = entity->getEndpoint();
              if (s.valid)
                preMaxAbs = std::max({preMaxAbs, std::abs(s.x), std::abs(s.y)});
              if (tp.valid)
                preMaxAbs = std::max({preMaxAbs, std::abs(tp.x), std::abs(tp.y)});
            }
          }
        }
        const double preDiag = (preMax - preMin).magnitude();
        const double preEps = LC_LoopUtils::hatchTolerance(preDiag, preMaxAbs);

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
          avoidZeroLength(edges, preEps);
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
          LC_LoopUtils::LoopExtractor extractor{*cont, tolerance};
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
        // -m_data.angle about rotCenter. The lattice anchor is mapped
        // through this stored pair; picking, the centroid and the moments use it too.
        m_pivot = rotCenter;
        m_loopAngle = isSolid() ? 0.0 : m_data.angle;
        m_tolerance = tolerance;
        m_duplicateLoops = LC_LoopUtils::countDuplicateLoops(*results);
        if (m_duplicateLoops > 0) {
            RS_DEBUG->print(RS_Debug::D_WARNING,
                "RS_Hatch::validate(): %d duplicate boundary loop(s); duplicated areas draw empty",
                m_duplicateLoops);
        }
        m_needOptimization = false;

        // Collect all loops (outer + holes) into flat boundary container list
        m_boundaryContainers = collectLoops(results);
    } catch (const std::exception& e) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "Hatch validation error: %s", e.what());
        m_updateError = HATCH_INVALID_CONTOUR;
        m_duplicateLoops = 0;
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

RS_HatchPatternFrame RS_Hatch::patternFrame() const {
    RS_HatchPatternFrame frame;
    frame.origin = latticeOrigin();
    frame.scale = m_data.scale;
    frame.angle = m_data.angle;
    frame.tileMin = m_tileMin;
    frame.tileSize = m_tileSize;
    const bool sizeFinite = m_tileSize.valid &&
                             std::isfinite(m_tileSize.x) &&
                             std::isfinite(m_tileSize.y) &&
                             m_tileSize.x > 1e-12 && m_tileSize.y > 1e-12;
    const bool minFinite = m_tileMin.valid &&
                            std::isfinite(m_tileMin.x) &&
                            std::isfinite(m_tileMin.y);
    frame.tileValid = !isSolid() && sizeFinite && minFinite;
    return frame;
}

namespace {

// Clones every atomic edge of `node.getOuterLoop()` into a fresh WCS
// container and recurses depth-first.
void appendExportLoops(const LC_LoopUtils::LC_Loops& node,
                       int depth,
                       const RS_Vector& pivot,
                       double loopAngle,
                       std::vector<RS_Hatch::ExportLoop>& out) {
    auto edges = std::make_unique<RS_EntityContainer>(nullptr, true);
    auto source = node.getOuterLoop();
    if (source) {
        for (RS_Entity* e : *source) {
            if (e == nullptr) continue;
            RS_Entity* copy = e->clone();
            copy->setParent(nullptr);
            copy->setLayer(nullptr);
            edges->addEntity(copy);
        }
    }
    // The loops live in the frame rotated by -loopAngle about pivot (see
    // validate()). Rotating by +loopAngle maps them back into WCS. The
    // identity case stays bit-identical, so a solid hatch (loopAngle = 0)
    // reaches the writer with the same coordinates as before.
    if (loopAngle != 0.0)
        edges->rotate(pivot, RS_Vector{loopAngle});
    out.push_back({std::move(edges), depth});
    for (const auto& child : node.getChildren())
        appendExportLoops(child, depth + 1, pivot, loopAngle, out);
}

// True when the given extracted container holds exactly one RS_Spline
// anywhere in it. validate() flattens source entities with ResolveAll, so
// an RS_Spline reaches the sorter as its chord lines; the writer wants to
// write the SPLINE edge back for the single-spline draw-hatch input.
bool holdsSingleRsSpline(const RS_EntityContainer& container) {
    bool found = false;
    lc::LC_ContainerTraverser traverser{container, RS2::ResolveNone};
    for (const RS_Entity* entity : traverser.entities()) {
        if (entity == nullptr) continue;
        if (entity->rtti() == RS2::EntitySpline) {
            if (found) return false;
            found = true;
            continue;
        }
        // A nested container is explored by the traverser already; any
        // non-spline atomic entity rules the single-spline case out.
        if (entity->isAtomic())
            return false;
    }
    return found;
}

std::unique_ptr<RS_EntityContainer> cloneSourceWithChildren(
    const RS_EntityContainer& source) {
    auto owned = std::make_unique<RS_EntityContainer>(nullptr, true);
    for (RS_Entity* e : source) {
        if (e == nullptr) continue;
        RS_Entity* copy = e->clone();
        copy->setParent(owned.get());
        copy->setLayer(nullptr);
        owned->addEntity(copy);
    }
    return owned;
}

}  // namespace

std::vector<RS_Hatch::ExportLoop> RS_Hatch::exportLoops() const {
    std::vector<ExportLoop> out;
    if (!m_orderedLoops || m_orderedLoops->empty())
        return out;
    for (const auto& root : *m_orderedLoops)
        appendExportLoops(root, 0, m_pivot, m_loopAngle, out);

    // One narrow exception: validate() flattens source entities with
    // ResolveAll, so an RS_Spline reaches the tree as its chord lines.
    // When the hatch's own source has exactly one container and it holds
    // one RS_Spline (any level), hand the writer the source container
    // instead so the SPLINE edge survives the round trip.
    if (out.size() == 1u) {
        const RS_EntityContainer* onlySource = nullptr;
        int containerCount = 0;
        for (RS_Entity* e : *this) {
            if (e != nullptr && e->isContainer() && !e->getFlag(RS2::FlagTemp)) {
                ++containerCount;
                onlySource = static_cast<const RS_EntityContainer*>(e);
            }
        }
        if (containerCount == 1 && onlySource != nullptr &&
            holdsSingleRsSpline(*onlySource)) {
            out[0].edges = cloneSourceWithChildren(*onlySource);
        }
    }
    return out;
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
    m_densityReport = LC_HatchDensityReport{};

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
    // The tile bbox is written by updatePatternHatch only; reset it so a
    // solid hatch or a missing pattern leaves patternFrame().tileValid false.
    m_tileMin = RS_Vector(false);
    m_tileSize = RS_Vector(false);

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

    // Compute area and the central tensor from the moment integrator in one
    // traversal about the stored pivot. The loops live in the frame rotated by
    // -m_loopAngle about m_pivot; the integrator returns the moments in that
    // frame relative to the pivot, and we map the centroid and the central
    // tensor back to WCS once the frame-local values are known.
    LC_AreaMoments total;
    for (const auto& loop : *m_orderedLoops) {
        total += loop.getTotalMoments(m_pivot);
    }
    m_area = total.area;
    m_centroid = RS_Vector(false);
    m_centralMoment = {};
    if (m_area >= RS_TOLERANCE) {
        // In the loop frame, moments are about m_pivot; d is the centroid
        // offset from the pivot in that frame.
        const RS_Vector d{total.first.mx / m_area, total.first.my / m_area};
        LC_SecondMoment central =
            total.second.getCentral(m_area, d.x, d.y);
        RS_Vector centroid = m_pivot + d.rotated(m_loopAngle);
        if (m_loopAngle != 0.0) {
            central = central.regionRotated(m_loopAngle);
        }
        m_centroid = centroid;
        m_centralMoment = central;
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

    // A NaN scale propagates through pattern->scale() without touching the
    // existing size checks (NaN comparisons never trip 1e-6 / RS_MAXDOUBLE);
    // treat it as a non-finite density outcome so the hatch stays empty of
    // children but keeps its loops, area and picking.
    if (!std::isfinite(m_data.scale)) {
        m_densityReport.reason = LC_HatchDensityReport::Reason::NonFinite;
        m_densityReport.finite = false;
        m_updateError = HATCH_TOO_DENSE;
        return;
    }
    // RS_EntityContainer::scale is a no-op when |factor| <= RS_TOLERANCE, so a
    // sub-tolerance scale would silently leave the pattern at its loaded
    // dimensions. Reject it up front as HATCH_TOO_SMALL.
    if (std::abs(m_data.scale) <= RS_TOLERANCE) {
        m_updateError = HATCH_TOO_SMALL;
        return;
    }

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

    // Capture the native (unscaled, unrotated) tile bbox before applying the
    // hatch scale. patternFrame() returns it to the exporter and to Quick
    // Info; it is the quantity DXF/DWG writers need to size the placeholder
    // definition line.
    pattern->calculateBorders();
    m_tileMin = pattern->getMin();
    m_tileSize = pattern->getSize();

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
    // Density guard: estimate, work bound, buffered piece budget. HATCH_TOO_DENSE
    // keeps the loops, area, moments and picking intact; the trimmed pieces are
    // buffered over all roots and only committed after every root finishes, so
    // an abort leaves no FlagHatchChild children behind.
    const LC_HatchDensityPolicy& dpolicy = LC_HatchDensity::policy();
    LC_HatchDensityReport report = LC_HatchDensity::estimate(
        *pattern, *m_orderedLoops, /*patternTurn*/ 0.0);
    const double softCap = dpolicy.forced ? dpolicy.hardMax : dpolicy.maxPieces;
    const double hardCap = dpolicy.hardMax;
    double effectiveCap = std::min(softCap, hardCap);

    // Insert expansion installs a shared budget that every hatch clone of
    // the expansion draws from. Shrink our local cap by the remaining room
    // so the first clones do not exhaust the shared pool; the "SharedBudget"
    // trip reason flags clones the pool refused.
    LC_HatchPieceBudget* shared = LC_HatchDensity::sharedBudget();
    std::size_t sharedRemaining = std::numeric_limits<std::size_t>::max();
    if (shared != nullptr) {
        sharedRemaining = shared->limit > shared->taken
            ? shared->limit - shared->taken
            : 0U;
        const double rem = static_cast<double>(sharedRemaining);
        if (rem < effectiveCap) effectiveCap = rem;
    }
    report.limit = effectiveCap;

    auto tripTooDense = [&](LC_HatchDensityReport::Reason why) {
        report.reason = why;
        m_densityReport = report;
        m_updateError = HATCH_TOO_DENSE;
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Hatch::updatePatternHatch: HATCH_TOO_DENSE "
                        "(estimate=%.3f, work=%.3f, limit=%.3f)",
                        report.estimatedPieces, report.work, report.limit);
    };

    if (!report.finite || !(report.estimatedPieces <= hardCap)) {
        tripTooDense(report.finite ? LC_HatchDensityReport::Reason::Estimate
                                    : LC_HatchDensityReport::Reason::NonFinite);
        return;
    }
    if (!dpolicy.forced && report.estimatedPieces > softCap) {
        tripTooDense(LC_HatchDensityReport::Reason::Estimate);
        return;
    }
    // The shared pool is a hard cap even in forced mode: insert expansion must
    // never produce more hatch pieces than the budget it was called with.
    if (shared != nullptr && report.estimatedPieces > static_cast<double>(sharedRemaining)) {
        tripTooDense(LC_HatchDensityReport::Reason::SharedBudget);
        return;
    }

    // The contour was optimized in a frame rotated by -m_loopAngle about
    // m_pivot (see validate()). The lattice anchor is one WCS point
    // (latticeOrigin() = (0, 0)) mapped into that frame; every root uses it,
    // so two roots of one hatch share the WCS phase and a non-lattice move of
    // the boundary does not drag the pattern. The trimmed pieces are rotated
    // back to WCS by +m_loopAngle about the same m_pivot.
    RS_Vector anchor = latticeOrigin();
    anchor.rotate(m_pivot, RS_Vector{-m_loopAngle});

    // gridWork is the (grid cells) * (boundary edges) overlap-test cost of
    // createTiles. Charge it before any tile is built so a thin diagonal
    // strip with a small tile does not run its O(B) grid walk inside the
    // density guard.
    const double gridWorkTotal = LC_LoopUtils::gridWork(*m_orderedLoops, *pattern, anchor);
    if (gridWorkTotal > 10.0 * dpolicy.workCap
        || (!dpolicy.forced && gridWorkTotal > dpolicy.workCap)) {
        report.work = gridWorkTotal;
        tripTooDense(LC_HatchDensityReport::Reason::Work);
        return;
    }

    // One tile set over the union of the roots' bounding boxes, de-duplicated
    // by (i, k); measure per-tile pair work with an early stop.
    const std::vector<LC_LoopUtils::LC_TileSlot> tiles =
        LC_LoopUtils::createTiles(*m_orderedLoops, *pattern, anchor);
    const double pairStop = 10.0 * dpolicy.workCap;
    const double pairWorkTotal =
        LC_LoopUtils::tileWork(*m_orderedLoops, *pattern, tiles, pairStop);
    report.work = gridWorkTotal + pairWorkTotal;
    if (report.work > 10.0 * dpolicy.workCap
        || (!dpolicy.forced && report.work > dpolicy.workCap)) {
        tripTooDense(LC_HatchDensityReport::Reason::Work);
        return;
    }

    // Buffer every produced piece under one shared budget. A trip discards
    // the whole buffer, keeping the hatch empty of FlagHatchChild children.
    const std::size_t cap = static_cast<std::size_t>(effectiveCap);
    LC_HatchPieceBudget budget{cap};
    std::unique_ptr<RS_EntityContainer> trimmed =
        LC_LoopUtils::trimPatternEntities(*m_orderedLoops, *pattern, tiles, budget);
    // Charge the shared pool for every piece this clone tried, tripped or not.
    // The next hatch clone in the same expansion sees a smaller remaining
    // budget; its sharedBudget comparison and softCap adjustment above keep
    // the pool bounded even when every clone would independently fit.
    if (shared != nullptr) {
        const std::size_t charge = budget.tripped ? budget.taken + 1 : budget.taken;
        if (shared->limit - shared->taken >= charge) {
            shared->taken += charge;
        } else {
            shared->taken = shared->limit;
            shared->tripped = true;
        }
    }
    if (budget.tripped) {
        report.attempted = budget.taken + 1;  // the piece that tripped
        trimmed.reset();
        tripTooDense(LC_HatchDensityReport::Reason::Budget);
        return;
    }
    report.attempted = budget.taken;

    // Collect every produced piece from the single trimmer run while they
    // are still in the loop frame. The LINE pieces feed one tile-relative
    // de-dup (tolerance 1e-6 x the scaled tile size), which merges touching
    // tile borders so no ink is drawn twice. Non-LINE pieces (arcs, circles,
    // ellipses, ...) pass through unchanged.
    std::vector<RS_LineData> lineData;
    std::vector<std::unique_ptr<RS_Entity>> otherPieces;
    if (trimmed) {
        for (auto& owned : trimmed->takeEntities()) {
            if (owned && owned->rtti() == RS2::EntityLine) {
                const auto* line = static_cast<const RS_Line*>(owned.get());
                lineData.push_back(line->getData());
                owned.reset();
            } else if (owned) {
                otherPieces.push_back(std::move(owned));
            }
        }
    }
    const double tolOffset = 1e-6 * std::max(patternSize.x, patternSize.y);
    std::vector<RS_LineData> mergedLines =
        LC_LoopUtils::deduplicatePatternLines(std::move(lineData), tolOffset);

    const RS_Vector rotationVector{m_loopAngle};
    int addedCount = 0;
    auto emitEntity = [&](RS_Entity* entity) {
        entity->setPen(pen);
        entity->setLayer(layer);
        entity->reparent(this);
        entity->setFlag(RS2::FlagHatchChild);
        entity->rotate(m_pivot, rotationVector);
        addEntity(entity);
        ++addedCount;
    };
    for (const RS_LineData& ld : mergedLines) {
        auto* line = new RS_Line(nullptr, ld);
        line->setVisible(true);
        emitEntity(line);
    }
    for (auto& owned : otherPieces) {
        RS_Entity* entity = owned.release();
        emitEntity(entity);
    }
    m_densityReport = report;

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
    else if (m_updateError == HATCH_TOO_DENSE) {
        drawDensityTint(painter);
    }
    else {
        drawPatternLines(painter);
    }
}

/**
 * Helper: Draws solid fill using cached QPainterPaths.
 */
void RS_Hatch::drawSolidFill(RS_Painter* painter) const {
    drawFill(painter, 1.0);
}

/**
 * Helper: Fills the hatch region with the resolved pen colour at the given
 * alpha channel. One QPainterPath carries every loop in the hierarchy as a
 * subpath and is drawn with Qt::OddEvenFill, so holes and islands fall out
 * of the odd-even rule naturally and crossing boundaries draw as XOR. Shared
 * by the solid fill (alpha = 1) and the HATCH_TOO_DENSE tint, so print,
 * print preview, and image export paths produce the same ink as the screen
 * for both outputs.
 */
void RS_Hatch::drawFill(RS_Painter* painter, double alpha) const {
    if (m_boundaryContainers.empty()) {
        return;
    }
    if (!std::isfinite(alpha) || alpha <= 0.0) {
        return;
    }
    if (alpha > 1.0) alpha = 1.0;

    painter->save();
    QBrush fillBrush = painter->brush();
    QColor fillColor = painter->getPen().getColor();
    // Fold the requested alpha into the pen's existing alpha so a pre-tinted
    // pen still contributes.
    const int penA = fillColor.alpha();
    const int a = static_cast<int>(std::lround(static_cast<double>(penA) * alpha));
    fillColor.setAlpha(std::clamp(a, 0, 255));
    fillBrush.setColor(fillColor);
    fillBrush.setStyle(Qt::SolidPattern);
    painter->setBrush(fillBrush);

    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    for (const auto& cont : m_boundaryContainers) {
        if (cont) {
            path.addPath(LC_LoopUtils::buildLoopPath(painter, *cont, m_tolerance));
        }
    }
    painter->drawPath(path);

    painter->restore();
}

/**
 * Helper: Draws the HATCH_TOO_DENSE tint. The alpha comes from
 * LC_HatchDensity::tintAlpha, which turns the saved line/piece densities
 * into a coverage estimate at the current zoom/stroke width.
 */
void RS_Hatch::drawDensityTint(RS_Painter* painter) const {
    // Physical pixels per UCS unit (one unit mapped through the view, then
    // through the device pixel ratio).
    double dpr = 1.0;
    if (painter != nullptr && painter->device() != nullptr) {
        dpr = painter->device()->devicePixelRatioF();
    }
    if (!std::isfinite(dpr) || dpr <= 0.0) dpr = 1.0;
    const double kPhys = painter != nullptr
        ? painter->toGuiDX(1.0) * dpr
        : dpr;
    // Pen width in physical pixels: a non-positive stored width means a
    // cosmetic pen (1 logical pixel).
    double wPhys = 1.0 * dpr;
    if (painter != nullptr) {
        const double sw = painter->getPen().getScreenWidth();
        if (std::isfinite(sw) && sw > 0.0) wPhys = sw * dpr;
    }
    const double alpha = LC_HatchDensity::tintAlpha(m_densityReport, kPhys, wPhys);
    drawFill(painter, alpha);
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
    return m_centroid;
}

LC_SecondMoment RS_Hatch::getMomentOfInertia() const {
    if (!m_secondMomentValid || m_area < RS_TOLERANCE) {
        return {};
    }
    return m_centralMoment;
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
    // Global even-odd parity: a point is inside the hatch region iff
    // classifyAll says so, matching one-path odd-even fill and the trimmer.
    if (LC_LoopUtils::classifyAll(query, *m_orderedLoops) != LC_LoopUtils::PointClass::Outside) {
        if (entity != nullptr) {
            *entity = const_cast<RS_Hatch*>(this);
        }
        return 0.0;
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
