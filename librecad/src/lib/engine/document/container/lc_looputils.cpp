/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2023 librecad (www.librecad.org)
** Copyright (C) 2026 dxli (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/
// File: lc_looputils.cpp

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>

#include <QPen>
#include <QPainterPath>

#include "lc_looputils.h"
#include "lc_loopclassifier.h"
#include "lc_pathbuilder.h"
#include "lc_parabola.h"
#include "lc_rect.h"
#include "lc_splinepoints.h"
#include "rs.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pattern.h"
#include "rs_vector.h"

namespace {
    using VertexMap = std::unordered_map<const RS_EntityContainer*, std::vector<RS_Vector>>;

    // Definition for buildLC_Loops (moved to namespace level for accessibility)
    /**
     * @brief Recursively builds an LC_Loops hierarchy from a container and all loops.
     * Clones atomic entities and traverses children via parent pointers. If
     * vertexMap carries this container's vertex table, the clone's geometry
     * is built from the clone (so generic pieces, which point at entities,
     * outlive the sorter's containers) and attached with eps.
     */
    LC_LoopUtils::LC_Loops buildLoops(const RS_EntityContainer* cont,
                                      const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops,
                                      const VertexMap* vertexMap,
                                      double eps) {
        const auto loopCopy = std::make_shared<RS_EntityContainer>(nullptr, true);
        for (const RS_Entity* e : *cont) {
            if (e && !e->isContainer()) {
                RS_Entity* copy = e->clone(); // Clone atomics for independent ownership
                // Clear the clone's parent and layer: both would otherwise
                // keep pointing at the source container and its drawing, which
                // can outlive the sorter result or disappear while the
                // boundary cache is still read. RS_Hatch::validate wipes
                // these on its own input, but every other caller of the
                // LoopSorter has to be covered here.
                copy->setParent(nullptr);
                copy->setLayer(nullptr);
                loopCopy->addEntity(copy);
            }
        }
        LC_LoopUtils::LC_Loops lc(loopCopy, true);
        if (vertexMap != nullptr && eps > 0.0) {
            auto it = vertexMap->find(cont);
            std::vector<RS_Vector> vertices = (it != vertexMap->end()) ? it->second : std::vector<RS_Vector>{};
            auto geom = LC_LoopUtils::LoopGeometry::fromChain(*loopCopy, std::move(vertices));
            lc.setGeometry(std::move(geom), eps);
        }
        for (const auto& p : allLoops) {
            if (p.get()->getParent() == cont) {
                lc.addChild(buildLoops(p.get(), allLoops, vertexMap, eps));
            }
        }
        return lc;
    }

    /** Overload with no vertex map and no eps (legacy LoopSorter ctor). */
    LC_LoopUtils::LC_Loops buildLoops(const RS_EntityContainer* cont,
                                      const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops) {
        return buildLoops(cont, allLoops, nullptr, 0.0);
    }

// Returns true if the entity forms a closed loop by itself (circle, full ellipse,
// full-circle arc, closed spline, or zero-length line).
bool isClosed(const RS_Entity& en) {
  switch (en.rtti()) {
  case RS2::EntityCircle:
    return true;
  case RS2::EntityEllipse: {
    const auto& ell = static_cast<const RS_Ellipse&>(en);
    return !ell.isArc() || std::abs(ell.getAngleLength() - 2.0 * M_PI) < RS_TOLERANCE_ANGLE;
  }
  case RS2::EntityArc: {
    const auto& arc = static_cast<const RS_Arc&>(en);
    return std::abs(arc.getAngleLength() - 2.0 * M_PI) < RS_TOLERANCE_ANGLE;
  }
  case RS2::EntitySplinePoints: {
    // Only LC_SplinePoints is atomic and reaches here. RS_Spline (rtti
    // EntitySpline) is a container and is rejected by LoopExtractor's
    // isAtomic() filter before it gets to isClosed().
    const auto& spline = static_cast<const LC_SplinePoints&>(en);
    return spline.isClosed();
  }
  case RS2::EntityLine:
    return en.getStartpoint() == en.getEndpoint();
  case RS2::EntityParabola:
  default:
    return false;
  }
}

void debugPath(const QPainterPath &path) {
  LC_ERR << "Path Element Count:" << path.elementCount();
  for (int i = 0; i < path.elementCount(); ++i) {
    const QPainterPath::Element &e = path.elementAt(i);
    switch (e.type) {
    case QPainterPath::MoveToElement:      LC_ERR << i << ": MoveTo  " << e.x << "," << e.y; break;
    case QPainterPath::LineToElement:      LC_ERR << i << ": LineTo  " << e.x << "," << e.y; break;
    case QPainterPath::CurveToElement:     LC_ERR << i << ": CurveTo " << e.x << "," << e.y; break;
    case QPainterPath::CurveToDataElement: LC_ERR << i << ":   Data  " << e.x << "," << e.y; break;
    }
  }
}

// Tolerance-aware integer key for RS_Vector endpoints, rounded to 1e-8 precision.
struct VectorKey {
  long long x, y;
  bool operator==(const VectorKey& other) const { return x == other.x && y == other.y; }
};

struct VectorKeyHash {
  std::size_t operator()(const VectorKey& k) const {
    std::hash<long long> h;
    std::size_t seed = h(k.x);
    seed ^= h(k.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

VectorKey makeVectorKey(const RS_Vector& v) {
  constexpr double SCALE = 1e8;
  return {static_cast<long long>(std::round(v.x * SCALE)),
          static_cast<long long>(std::round(v.y * SCALE))};
}

}  // anonymous namespace

namespace LC_LoopUtils {

// Private implementation for LoopExtractor
struct LoopExtractor::LoopData {
  std::vector<RS_Entity*> unprocessed;
  std::map<RS_Entity*, bool> processed;
  RS_Entity* current = nullptr;
  RS_Vector endPoint;
  RS_Vector targetPoint;
  std::unordered_map<VectorKey, std::vector<RS_Entity*>, VectorKeyHash> endpointToEdges;
  // Stable container-order index, filled in the ctor. Used by findFirst() and
  // findOutermost() for a deterministic tie-break between candidate edges.
  std::unordered_map<const RS_Entity*, std::size_t> order;
};

/**
 * @brief Constructs LoopExtractor and initializes unprocessed edges.
 * Filters to atomic entities with length > ENDPOINT_TOLERANCE.
 * Builds endpoint adjacency map upfront for O(1) graph traversal.
 * Closed entities (circles, full ellipses) are excluded from the map since
 * their start/end points are invalid.
 */
LoopExtractor::LoopExtractor(const RS_EntityContainer& edges) :
                                                                m_data(std::make_unique<LoopData>())
{
  std::size_t idx = 0;
  for (RS_Entity* e : edges) {
    if (e->isAtomic() && e->getLength() > ENDPOINT_TOLERANCE) {  // Skip degenerate zero-length edges
      m_data->unprocessed.push_back(e);
      m_data->processed[e] = false;
      m_data->order[e] = idx;

             // Closed entities (circles, full ellipses) have no valid endpoints — skip adjacency map
      if (!isClosed(*e)) {
        VectorKey k1 = makeVectorKey(e->getStartpoint());
        m_data->endpointToEdges[k1].push_back(e);

        VectorKey k2 = makeVectorKey(e->getEndpoint());
        m_data->endpointToEdges[k2].push_back(e);
      }
    }
    ++idx;
  }
}

LoopExtractor::~LoopExtractor() = default;

/**
 * @brief Extracts closed loops iteratively until no unprocessed edges remain.
 * Builds each loop by chaining connected edges, clones and orients for
 * positive area, and records the per-edge start-point vertex table used by
 * the half-open monotone classifier.
 */
std::vector<ExtractedLoop> LoopExtractor::extractLoops() const {
  std::vector<ExtractedLoop> results;
  while (!m_data->unprocessed.empty()) {
    m_loop = std::make_unique<RS_EntityContainer>();
    const FirstEdge choice = findFirst();
    RS_Entity* first = choice.edge;
    if (first) {
      RS_Entity* cloned_first = first->clone();
      // Walk the first edge in the direction we chose. For a closed atomic
      // entity (circle, full ellipse, closed spline) revertDirection() is a
      // no-op; for a line/arc/ellipse arc it swaps the stored traversal.
      if (choice.reverse) {
        cloned_first->revertDirection();
      }
      m_loop->addEntity(cloned_first);
      m_data->processed[first] = true;
      m_data->current = cloned_first;
      const RS_Vector start = cloned_first->getStartpoint();
      const RS_Vector end = cloned_first->getEndpoint();
      m_data->targetPoint = start;
      m_data->endPoint = end;
      m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), first), m_data->unprocessed.end());
      size_t iteration = 0;
      const size_t maxIterations = m_data->unprocessed.size() + 2;  // cap based on remaining edges at loop start
      // Closed entities (circles, full ellipses) are already complete loops — no chaining needed
      while (!isClosed(*first) && m_data->endPoint.distanceTo(m_data->targetPoint) > ENDPOINT_TOLERANCE) {  // Continue until closure within tolerance
        if (++iteration > maxIterations) {
          RS_DEBUG->print(RS_Debug::D_WARNING, "LoopExtractor: possible degenerate loop detected");
          break;
        }
        if (findNext()) {
          if (m_data->endPoint.distanceTo(m_data->targetPoint) <= ENDPOINT_TOLERANCE) {
              break;
          }
        } else {
          break;
        }
      }
      if (validate()) {
        const double area = m_loop->areaLineIntegral();
        if (area < 0.0) {
          // Reverse direction for positive area (counter-clockwise)
          auto new_loop = std::make_unique<RS_EntityContainer>();
          for (int i = static_cast<int>(m_loop->count()) - 1; i >= 0; --i) {
            RS_Entity* e = m_loop->entityAt(static_cast<unsigned>(i))->clone();
            e->revertDirection();
            new_loop->addEntity(e);
          }
          m_loop = std::move(new_loop);
        }
        ExtractedLoop el;
        // Build the start-of-edge vertex table. A single closed atomic
        // entity (circle, full ellipse, closed spline) has no valid vertex
        // table; the classifier uses its own endpoints in that case.
        const unsigned n = m_loop->count();
        if (n > 1 || (n == 1 && !isClosed(*m_loop->entityAt(0)))) {
          el.vertices.reserve(n);
          for (unsigned k = 0; k < n; ++k) {
            el.vertices.push_back(m_loop->entityAt(k)->getStartpoint());
          }
        }
        el.edges = std::move(m_loop);
        results.push_back(std::move(el));
      } else {
        RS_DEBUG->print("LoopExtractor: Invalid loop discarded");  // Log invalid loops
      }
    }
  }
  return results;
}

/**
 * @brief Legacy wrapper that returns only the containers. New callers use
 * extractLoops() so each loop carries its vertex table alongside the edges.
 */
std::vector<std::unique_ptr<RS_EntityContainer>> LoopExtractor::extract() const {
  auto loops = extractLoops();
  std::vector<std::unique_ptr<RS_EntityContainer>> results;
  results.reserve(loops.size());
  for (auto& el : loops) {
    results.push_back(std::move(el.edges));
  }
  return results;
}

/**
 * @brief Validates loop closure and connectivity.
 * For multi-entity loops: checks each adjacent pair connects within ENDPOINT_TOLERANCE,
 * and that the last entity's end reaches the first entity's start.
 * Single closed entities (circles, full ellipses, closed splines) pass unconditionally.
 */
bool LoopExtractor::validate() const {
  const size_t n = m_loop->count();
  if (n == 0) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "LoopExtractor::validate: Empty loop");
    return false;
  }

         // Special case: single closed entity (no chain to check)
  if (n == 1) {
    RS_Entity* e = m_loop->entityAt(0);
    const RS2::EntityType type = e->rtti();
    if (type == RS2::EntityCircle) {
        return true;
    }
    if (type == RS2::EntityEllipse) {
        const auto* ell = static_cast<RS_Ellipse*>(e);
        return ell->getAngleLength() >= 2 * M_PI - RS_TOLERANCE;
    }
    if (type ==
        RS2::EntitySplinePoints) { // Closed spline (LC_SplinePoints only)
      auto spl = static_cast<LC_SplinePoints*>(e);
      return spl->isClosed();
    }
    if (type == RS2::EntityParabola) {  // Valid primitive
      auto para = static_cast<LC_Parabola*>(e);
      return para->getData().m_valid;
    }
    // Fallback: check self-closure
    return e->getStartpoint().distanceTo(e->getEndpoint()) <= ENDPOINT_TOLERANCE;
  }

         // Full multi-entity chain validation (ensures consistent forward directions)
  const RS_Entity* prev = m_loop->entityAt(0);
  for (size_t i = 1; i < n; ++i) {
    RS_Entity* curr = m_loop->entityAt(i);
    const double dist = prev->getEndpoint().distanceTo(curr->getStartpoint());
    if (dist > ENDPOINT_TOLERANCE) {
      RS_DEBUG->print(RS_Debug::D_WARNING,
                      "LoopExtractor::validate: Broken connection at entity %zu (dist=%.2e)", i, dist);
      return false;
    }
    prev = curr;
  }

         // Final closure: last end → first start
  const RS_Entity* first = m_loop->entityAt(0);
  const RS_Entity* last = m_loop->last();
  const double closeDist = last->getEndpoint().distanceTo(first->getStartpoint());
  if (closeDist > ENDPOINT_TOLERANCE) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LoopExtractor::validate: Loop does not close (dist=%.2e)", closeDist);
    return false;
  }

  // Reject geometrically degenerate loops. Threshold is (ENDPOINT_TOLERANCE)^2 — any
  // loop whose entities are all distinguishable by endpoint keys will exceed this.
  const double area = m_loop->areaLineIntegral();
  constexpr double MIN_AREA = ENDPOINT_TOLERANCE * ENDPOINT_TOLERANCE;
  if (std::abs(area) < MIN_AREA) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LoopExtractor::validate: Degenerate zero-area loop "
                    "(n=%zu, area=%.4e, MIN=%.4e)",
                    n, area, MIN_AREA);
    for (size_t i = 0; i < n; ++i) {
      RS_Entity *e = m_loop->entityAt(i);
      if (!e)
        continue;
      RS_Vector s = e->getStartpoint();
      RS_Vector ep = e->getEndpoint();
      RS_DEBUG->print(
          RS_Debug::D_WARNING,
          "  entity[%zu] type=%d start=(%.8f,%.8f) end=(%.8f,%.8f) len=%.4e", i,
          static_cast<int>(e->rtti()), s.x, s.y, ep.x, ep.y, e->getLength());
    }
    return false;
  }

  return true;
}

/**
 * @brief Finds the first edge to start a new loop.
 *
 * Deterministic rule: picks the lexicographically smallest (x, y, container
 * index) among the leftmostPoint() of every unprocessed edge.
 *
 *   - When that point is interior to its chosen edge, the reverse flag is set
 *     if the stored traversal walks away in +y at that point (we need to
 *     start in -y so the loop turns counter-clockwise around what lies to the
 *     right).
 *   - When the point is a vertex (an edge's own start or end), all edges
 *     attached at that vertex are compared by the CCW angle from -y of their
 *     travel direction leaving the vertex; ties within 1e-12 break by smaller
 *     signed curvature leaving the vertex, then by lower container index.
 */
LoopExtractor::FirstEdge LoopExtractor::findFirst() const
{
  FirstEdge result;
  if (m_data->unprocessed.empty()) return result;

  struct Candidate {
    RS_Entity* edge = nullptr;
    RS_Vector point{};
    bool interior = false;
    std::size_t idx = 0;
  };

  // Find the lexicographically smallest leftmostPoint among every edge.
  std::vector<Candidate> all;
  all.reserve(m_data->unprocessed.size());
  for (RS_Entity* e : m_data->unprocessed) {
    bool interior = false;
    const RS_Vector p = LC_LoopUtils::leftmostPoint(*e, &interior);
    if (!p.valid) continue;
    const auto it = m_data->order.find(e);
    const std::size_t idx = (it != m_data->order.end()) ? it->second
                                                        : static_cast<std::size_t>(-1);
    all.push_back({e, p, interior, idx});
  }
  if (all.empty()) {
    // No valid leftmost points: keep the first unprocessed edge as a fallback.
    result.edge = m_data->unprocessed.front();
    return result;
  }
  auto betterLex = [](const Candidate& a, const Candidate& b) {
    if (a.point.x != b.point.x) return a.point.x < b.point.x;
    if (a.point.y != b.point.y) return a.point.y < b.point.y;
    return a.idx < b.idx;
  };
  const Candidate& best = *std::min_element(all.begin(), all.end(), betterLex);

  if (best.interior) {
    // Pick the stored traversal if it walks in -y at the extremum, otherwise
    // reverse. The travel direction is the tangent; at a leftmost extremum of
    // a smooth curve the tangent is vertical, so sign(dy/du) picks the branch.
    const RS_Vector tangent = LC_LoopUtils::travelDirection(*best.edge, true);
    result.edge = best.edge;
    result.reverse = (tangent.y > 0.0);
    return result;
  }

  // The leftmost point is a vertex: collect every edge attached there (within
  // ENDPOINT_TOLERANCE) and pick the one whose travel direction leaving the
  // vertex has the smallest CCW angle measured from -y.
  struct Attach {
    RS_Entity* edge = nullptr;
    bool atStart = false;
    double ccw = 0.0;
    double curv = 0.0;
    std::size_t idx = 0;
  };
  std::vector<Attach> attached;
  attached.reserve(all.size());
  for (const Candidate& c : all) {
    const RS_Vector s = c.edge->getStartpoint();
    const RS_Vector e = c.edge->getEndpoint();
    const bool atStart = (s.valid && s.distanceTo(best.point) <= ENDPOINT_TOLERANCE);
    const bool atEnd = (e.valid && e.distanceTo(best.point) <= ENDPOINT_TOLERANCE);
    if (!atStart && !atEnd) continue;
    // Prefer start attachment if the vertex matches both ends (zero-length
    // after tolerance): we still want a deterministic choice.
    const bool pickStart = atStart;
    RS_Vector h = LC_LoopUtils::travelDirection(*c.edge, pickStart);
    if (!(h.valid && (std::abs(h.x) + std::abs(h.y) > 0.0))) continue;
    if (!pickStart) h = h * (-1.0);  // heading at an end becomes -travelDir
    const double a = std::atan2(h.y, h.x);
    // CCW angle from -y: the heading -y has atan2 = -pi/2 and we want 0 for it.
    double ccw = a + M_PI_2;
    while (ccw < 0.0) ccw += 2.0 * M_PI;
    while (ccw >= 2.0 * M_PI) ccw -= 2.0 * M_PI;
    double curv = LC_LoopUtils::signedCurvature(*c.edge, pickStart);
    if (!pickStart) curv = -curv;  // leaving an end flips the sign
    attached.push_back({c.edge, pickStart, ccw, curv, c.idx});
  }
  if (attached.empty()) {
    result.edge = best.edge;
    result.reverse = false;
    return result;
  }
  auto betterAttach = [](const Attach& a, const Attach& b) {
    if (std::abs(a.ccw - b.ccw) > 1e-12) return a.ccw < b.ccw;
    if (a.curv != b.curv) return a.curv < b.curv;
    return a.idx < b.idx;
  };
  const Attach& pick = *std::min_element(attached.begin(), attached.end(), betterAttach);
  result.edge = pick.edge;
  result.reverse = !pick.atStart;  // if the vertex is e's end, walk in reverse
  return result;
}

/**
 * @brief Selects the next edge making the strongest left turn (maximum CCW turn).
 * Purely tangent-based, so it handles curves and splines correctly.
 */
RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const
{
  if (edges.empty()) {
      return nullptr;
  }
  if (edges.size() == 1) {
      return edges[0];
  }

  // Incoming direction (how we arrived at the junction)
  const double incoming = m_data->current->getDirection2();

  std::vector<std::pair<double, RS_Entity*>> candidates;
  candidates.reserve(edges.size());

  for (RS_Entity* e : edges) {
    // Determine whether we will need to reverse this edge
    const bool attachAtStart =
        e->getStartpoint().distanceTo(m_data->endPoint) <= ENDPOINT_TOLERANCE;

    double outgoing;
    if (attachAtStart) {
      outgoing = e->getDirection1();                    // forward as-is
    } else {
      // Will be reversed → outgoing tangent = original direction2 + π
      outgoing = RS_Math::correctAngle(e->getDirection2() + M_PI);
    }

           // Signed turn angle in [-π, π]. Positive = left turn (CCW)
    double turn = RS_Math::correctAngle(outgoing - incoming);
    if (turn > M_PI) {
        turn -= 2.0 * M_PI;
    }

    candidates.emplace_back(turn, e);
  }

         // Prefer maximum left turn → this is what "outermost" actually means for CCW boundary following
  std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });

  return candidates.front().second;
}

/**
 * @brief Returns unprocessed entities whose endpoint matches the current endpoint.
 * O(degree) via the precomputed adjacency map.
 */
std::vector<RS_Entity*> LoopExtractor::getConnected() const {
  std::vector<RS_Entity*> ret;
  const VectorKey k = makeVectorKey(m_data->endPoint);

  const auto it = m_data->endpointToEdges.find(k);
  if (it != m_data->endpointToEdges.end()) {
    for (RS_Entity* e : it->second) {
      auto pit = m_data->processed.find(e);
      if (pit != m_data->processed.end() && !pit->second) {
        ret.push_back(e);
      }
    }
  }
  return ret;
}

/**
 * @brief Finds and appends the next connected edge to the current loop.
 * @return true if an edge was found.
 */
bool LoopExtractor::findNext() const {
  const std::vector<RS_Entity*> connected = getConnected();
  if (connected.empty()) {
      return false;
  }

  RS_Entity* next = (connected.size() == 1) ? connected[0] : findOutermost(connected);

  if (next) {
    RS_Entity* cloned = next->clone();
    m_loop->addEntity(cloned);
    m_data->processed[next] = true;
    m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), next), m_data->unprocessed.end());
    if (cloned->getStartpoint().distanceTo(m_data->endPoint) > ENDPOINT_TOLERANCE) {
      cloned->revertDirection();
    }
    m_data->endPoint = cloned->getEndpoint();
    m_data->current = cloned;
    return true;
  }
  return false;
}

// Private implementation for LoopSorter
struct LoopSorter::Data {
  std::vector<std::unique_ptr<RS_EntityContainer>> loops;  ///< Input loops
  std::shared_ptr<std::vector<LC_Loops>> results;          ///< Output hierarchy

  // Per-loop vertex table and per-loop geometry, parallel to loops. The
  // vertex map is keyed by the container pointer so buildLoops can look it
  // up while recursing the forest. eps is the per-hatch ON band.
  std::vector<std::vector<RS_Vector>> vertices;
  std::unordered_map<const RS_EntityContainer*, std::vector<RS_Vector>> vertexMap;
  std::unordered_map<const RS_EntityContainer*, std::shared_ptr<const LoopGeometry>> geometryMap;
  double eps = 0.0;

  // Per-loop metrics parallel to loops. absArea[i] is the loop's |area|,
  // perimeter[i] is the sum of edge lengths and box[i] is the loop's bbox,
  // all built once in sortAndBuild. order[] holds loop indices sorted
  // ascending by (absArea, extraction order) for the full-list parent walk.
  std::vector<double> absArea;
  std::vector<double> perimeter;
  std::vector<LC_Rect> box;
  std::vector<std::size_t> order;
};

    /**
     * @brief Predicate for sorting loops: ascending absolute area (small to large for parent assignment), tie-break by bounding box diagonal.
     * Ensures robust ordering for containment detection.
     */
    struct LoopSorter::AreaPredicate {
        bool operator()(const RS_EntityContainer* a, const RS_EntityContainer* b) const {
            const double areaA = std::abs(a->areaLineIntegral());
            const double areaB = std::abs(b->areaLineIntegral());
            if (std::abs(areaA - areaB) < RS_TOLERANCE) {
                const double diagA = (a->getMax() - a->getMin()).magnitude();
                const double diagB = (b->getMax() - b->getMin()).magnitude();
                return diagA < diagB;
            }
            return areaA < areaB; // Ascending abs area (small to large for parent assignment)
        }
    };

    /**
     * @brief Legacy ctor. Builds a per-loop start-of-edge vertex table and
     * a per-hatch ON band tolerance from the union bbox and the edges' end
     * points.
     */
    LoopSorter::LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops) : m_data(new Data) {
        m_data->loops = std::move(loops);

        // Build per-loop vertex tables from each container's edge starts.
        m_data->vertices.reserve(m_data->loops.size());
        RS_Vector bbMin(RS_MAXDOUBLE, RS_MAXDOUBLE);
        RS_Vector bbMax(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
        double maxAbs = 0.0;
        for (auto& p : m_data->loops) {
            p->forcedCalculateBorders();
            bbMin.x = std::min(bbMin.x, p->getMin().x);
            bbMin.y = std::min(bbMin.y, p->getMin().y);
            bbMax.x = std::max(bbMax.x, p->getMax().x);
            bbMax.y = std::max(bbMax.y, p->getMax().y);
            std::vector<RS_Vector> vs;
            const unsigned n = p->count();
            if (n > 1 || (n == 1 && !isClosed(*p->entityAt(0)))) {
                vs.reserve(n);
                for (unsigned k = 0; k < n; ++k) {
                    const RS_Vector s = p->entityAt(k)->getStartpoint();
                    vs.push_back(s);
                    maxAbs = std::max({maxAbs, std::abs(s.x), std::abs(s.y)});
                }
            }
            m_data->vertexMap[p.get()] = vs;
            m_data->vertices.push_back(std::move(vs));
        }
        const double diag = (bbMax - bbMin).magnitude();
        m_data->eps = hatchTolerance(diag, maxAbs);
        sortAndBuild();
    }

    /**
     * @brief Takes extracted loops that already carry their start-of-edge
     * vertex table and the per-hatch ON tolerance.
     */
    LoopSorter::LoopSorter(std::vector<ExtractedLoop> loops, double eps) : m_data(new Data) {
        m_data->eps = eps;
        m_data->loops.reserve(loops.size());
        m_data->vertices.reserve(loops.size());
        for (auto& el : loops) {
            RS_EntityContainer* ptr = el.edges.get();
            m_data->vertexMap[ptr] = el.vertices;
            m_data->vertices.push_back(std::move(el.vertices));
            m_data->loops.push_back(std::move(el.edges));
        }
        sortAndBuild();
    }

    LoopSorter::~LoopSorter() = default;

    /**
     * @brief Main sorting and hierarchy building: filters zero-area loops,
     * builds the per-loop geometry once, then assigns the smallest-area
     * parent for every loop against the full sorted list. Preserves the
     * input's extraction order for duplicate-area tie-breaks.
     */
    void LoopSorter::sortAndBuild() {
        const std::size_t n = m_data->loops.size();
        m_data->absArea.assign(n, 0.0);
        m_data->perimeter.assign(n, 0.0);
        m_data->box.assign(n, LC_Rect{RS_Vector(0.0, 0.0), RS_Vector(0.0, 0.0)});

        for (std::size_t i = 0; i < n; ++i) {
            RS_EntityContainer* p = m_data->loops[i].get();
            p->setParent(nullptr); // Reset parents
            p->forcedCalculateBorders();
            m_data->absArea[i] = std::abs(p->areaLineIntegral());
            double per = 0.0;
            for (const RS_Entity* e : *p) {
                if (e) per += e->getLength();
            }
            m_data->perimeter[i] = per;
            m_data->box[i] = LC_Rect{p->getMin(), p->getMax()};
        }

        // Build each loop's geometry once; findParent reads them.
        for (auto& p : m_data->loops) {
            auto it = m_data->vertexMap.find(p.get());
            std::vector<RS_Vector> vs = (it != m_data->vertexMap.end()) ? it->second : std::vector<RS_Vector>{};
            m_data->geometryMap[p.get()] = LoopGeometry::fromChain(*p, std::move(vs));
        }

        // Keep only non-degenerate loops in the parent-search order; the walk
        // visits every candidate in ascending (|area|, extraction index).
        m_data->order.clear();
        m_data->order.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            if (m_data->absArea[i] < RS_TOLERANCE) {
                RS_DEBUG->print("LoopSorter: Skipping degenerate zero-area loop");
                continue;
            }
            m_data->order.push_back(i);
        }
        std::stable_sort(m_data->order.begin(), m_data->order.end(),
                         [this](std::size_t a, std::size_t b) {
                             if (m_data->absArea[a] != m_data->absArea[b]) {
                                 return m_data->absArea[a] < m_data->absArea[b];
                             }
                             return a < b;
                         });

        std::vector<RS_EntityContainer*> forest;
        for (std::size_t idx : m_data->order) {
            findParent(idx);
            RS_EntityContainer* child = m_data->loops[idx].get();
            if (child->getParent() == nullptr) {
                forest.push_back(child);
            }
        }
        m_data->results = forestToLoops(forest);
    }

    /**
     * @brief Accessor for results.
     */
    std::shared_ptr<std::vector<LC_Loops>> LoopSorter::getResults() const {
        return m_data->results;
    }

    /**
     * @brief Converts forest roots to recursive LC_Loops trees using buildLoops.
     */
    std::shared_ptr<std::vector<LC_Loops>> LoopSorter::forestToLoops(std::vector<RS_EntityContainer*> forest) const {
        auto loops = std::make_shared<std::vector<LC_Loops>>();
        const VertexMap* vmap = m_data->vertexMap.empty() ? nullptr : &m_data->vertexMap;
        for (const RS_EntityContainer* container : forest) {
            loops->push_back(buildLoops(container, m_data->loops, vmap, m_data->eps));
        }
        return loops;
    }

/**
 * @brief Assigns the smallest enclosing parent for the loop at index child
 * among the full sorted list. Chooses an interior point of child (not a
 * bbox centre), walks every candidate in ascending (|area|, extraction
 * index) order, and accepts the first whose bbox contains child's with a
 * per-hatch epsilon slack and whose geometry has odd parity at that point.
 */
void LoopSorter::findParent(std::size_t child) {
    if (child >= m_data->loops.size()) return;
    RS_EntityContainer* childLoop = m_data->loops[child].get();
    if (!childLoop) return;
    const double childArea = m_data->absArea[child];
    const double childPerim = m_data->perimeter[child];
    const LC_Rect& childBox = m_data->box[child];
    const double eps = m_data->eps;

    auto geomIt = m_data->geometryMap.find(childLoop);
    if (geomIt == m_data->geometryMap.end()) return;
    const LoopGeometry* childGeom = geomIt->second.get();
    if (!childGeom) return;

    const RS_Vector p = interiorPoint(*childGeom);
    if (!p.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "LoopSorter: no interior point for loop with area %f",
                        childArea);
        return;
    }

    for (std::size_t q : m_data->order) {
        if (q == child) continue;
        const double parentArea = m_data->absArea[q];
        const double parentPerim = m_data->perimeter[q];
        const double epsA = eps * std::max(parentPerim, childPerim);
        if (parentArea < childArea - epsA) continue;
        if (parentArea <= childArea + epsA && q > child) {
            // Duplicate areas: the lower-index loop parents the higher-index twin.
            continue;
        }
        RS_EntityContainer* qLoop = m_data->loops[q].get();
        if (!qLoop) continue;
        const LC_Rect parentBoxSlack = m_data->box[q].increaseBy(eps);
        if (!childBox.inArea(parentBoxSlack)) continue;
        auto parentGeomIt = m_data->geometryMap.find(qLoop);
        if (parentGeomIt == m_data->geometryMap.end()) continue;
        const LoopGeometry* parentGeom = parentGeomIt->second.get();
        if (!parentGeom) continue;
        if (oddParity(p, {parentGeom})) {
            childLoop->setParent(qLoop);
            RS_DEBUG->print("LoopSorter: Assigned parent for loop with area %f",
                            childArea);
            return;
        }
    }
    RS_DEBUG->print("LoopSorter: No parent found for loop with area %f",
                    childArea);
}

    // Private implementation for LoopOptimizer
    struct LoopOptimizer::Data {
        std::shared_ptr<std::vector<LC_Loops>> results; ///< Processed results
    };

    /**
     * @brief Constructs LoopOptimizer and processes the input contour.
     */
    LoopOptimizer::LoopOptimizer(const RS_EntityContainer& contour) : m_data(new Data) {
        addContainer(contour);
    }

    LoopOptimizer::~LoopOptimizer() = default;

    /**
     * @brief Accessor for results.
     */
    std::shared_ptr<std::vector<LC_Loops>> LoopOptimizer::getResults() const {
        return m_data->results;
    }

    /**
     * @brief Processes a contour: Extract loops, sort, and build hierarchy.
     */
    void LoopOptimizer::addContainer(const RS_EntityContainer& contour) const {
        const LoopExtractor extractor(contour);
        auto loops = extractor.extract();
        const LoopSorter sorter(std::move(loops));
        m_data->results = sorter.getResults();
    }

    // LC_Loops implementation

    LC_Loops::LC_Loops(std::shared_ptr<RS_EntityContainer> loop, bool) : m_loop(loop) {
        // Ownership managed via shared_ptr; autoDelete assumed true
    }

    LC_Loops::~LC_Loops() = default;

    /**
     * @brief Moves a child into the children vector.
     */
    void LC_Loops::addChild(LC_Loops child) {
        m_children.push_back(std::move(child));
    }

    /**
     * @brief Adds an entity to the outer loop container.
     */
    void LC_Loops::addEntity(const RS_Entity* entity) const {
        m_loop->addEntity(entity);
        m_geometry.reset();  // Invalidate cached geometry; built lazily on next read.
    }

    void LC_Loops::setGeometry(std::shared_ptr<const LoopGeometry> geometry, double eps) {
        m_geometry = std::move(geometry);
        m_eps = eps;
    }

    const LoopGeometry& LC_Loops::geometry() const {
        if (!m_geometry && m_loop) {
            // Lazy build from the outer loop with a vertex table from edge
            // starts and a per-hatch tolerance derived from the loop's own
            // bbox and endpoint extrema.
            std::vector<RS_Vector> vs;
            const unsigned n = m_loop->count();
            double maxAbs = 0.0;
            if (n > 1 || (n == 1 && !isClosed(*m_loop->entityAt(0)))) {
                vs.reserve(n);
                for (unsigned k = 0; k < n; ++k) {
                    const RS_Vector s = m_loop->entityAt(k)->getStartpoint();
                    vs.push_back(s);
                    maxAbs = std::max({maxAbs, std::abs(s.x), std::abs(s.y)});
                }
            }
            if (m_eps <= 0.0) {
                m_loop->forcedCalculateBorders();
                const double diag = (m_loop->getMax() - m_loop->getMin()).magnitude();
                m_eps = hatchTolerance(diag, maxAbs);
            }
            m_geometry = LoopGeometry::fromChain(*m_loop, std::move(vs));
        }
        return *m_geometry;
    }

    /**
     * @brief Non-recursive check for the outer loop. Exact oddParity on this
     * node's own geometry: no ON band here. getContainingDepth() calls this
     * on every node in the tree so the ON band is applied once in classify().
     */
    bool LC_Loops::isInsideOuter(const RS_Vector& point) const {
        return geometry().crossingsRight(point) % 2 == 1;
    }

    /**
     * @brief Full classifier: ON if the point is within tolerance() of any
     * edge in this tree (root and descendants), else the depth parity.
     */
    PointClass LC_Loops::classify(const RS_Vector& point) const {
        if (m_eps > 0.0) {
            std::vector<const RS_EntityContainer*> all;
            getAllLoops(all);
            if (isNearAnyEdge(point, all, m_eps)) {
                return PointClass::On;
            }
        }
        return (getContainingDepth(point) % 2 == 1)
                   ? PointClass::Inside
                   : PointClass::Outside;
    }

    /**
     * @brief Recursive inside check using the full classifier.
     */
    bool LC_Loops::isInside(const RS_Vector& point) const {
        return classify(point) != PointClass::Outside;
    }

    /**
     * @brief Computes recursive depth: 1 for outer + sum of children.
     * Uses outer-only check to avoid cycles.
     */
    int LC_Loops::getContainingDepth(const RS_Vector& point) const {
        int depth = 0;
        if (isInsideOuter(point)) {
            // Check outer only
            depth = 1;
            for (const auto& child : m_children) {
                depth += child.getContainingDepth(point);
            }
        }
        return depth;
    }

    /**
     * @brief Builds hierarchical QPainterPath: Outer path (reversed if odd level), add children recursively.
     * Applies OddEvenFill for holes.
     */
    QPainterPath LC_Loops::getPainterPath(RS_Painter* painter, int level) const {
        QPainterPath path = buildPathFromLoop(painter, *m_loop);
        for (const auto& child : m_children) {
            const QPainterPath childPath = child.getPainterPath(painter, level + 1);
            path -= childPath;
        }
        path.setFillRule(Qt::OddEvenFill);
        return path;
    }

    /**
     * @brief Computes net area: Outer area minus children's total areas (subtracts holes, adds islands).
     */
    double LC_Loops::getTotalArea() const {
        return std::accumulate(m_children.begin(), m_children.end(), m_loop->areaLineIntegral(), [](double area, const LC_Loops& loop) {
            return area - loop.getTotalArea(); // Recursive subtraction
        });
    }

    LC_FirstMoment LC_Loops::getTotalFirstMoment() const {
  // Mirror the recursive hole-subtraction of getTotalArea():
  //   net = outer - sum(children)
  return std::accumulate(m_children.begin(), m_children.end(),
                         m_loop->firstMomentLineIntegral(),
                         [](LC_FirstMoment acc, const LC_Loops& child) {
                             return acc - child.getTotalFirstMoment();
                         });
}

LC_SecondMoment LC_Loops::getTotalSecondMoment() const {
  // Mirror the recursive hole-subtraction of getTotalArea():
  //   net = outer - sum(children)
  return std::accumulate(m_children.begin(), m_children.end(),
                         m_loop->secondMomentLineIntegral(),
                         [](LC_SecondMoment acc, const LC_Loops& child) {
                             return acc - child.getTotalSecondMoment();
                         });
}/**
     * @brief Parametric equation for point on ellipse.
     */
    RS_Vector LC_Loops::e_point(const RS_Vector& center, double major, double minor, double rot, double t) const {
        RS_Vector local(major * std::cos(t), minor * std::sin(t));
        local.rotate(rot);
        return center + local;
    }

    /**
     * @brief First derivative (tangent vector) for ellipse.
     */
    RS_Vector LC_Loops::e_prime(double major, double minor, double rot, double t) const {
        RS_Vector local_prime(-major * std::sin(t), minor * std::cos(t));
        local_prime.rotate(rot);
        return local_prime;
    }

    /**
     * @brief Appends an elliptic arc to path using cubic Bezier segments.
     * Segments based on sweep angle and aspect ratio for smoothness.
     * Optimized to reuse endpoint and tangent calculations across segments.
     */
    void LC_Loops::addEllipticArc(QPainterPath& path, const RS_Vector& center, double major, double minor, double rot, double a1,
                                  double a2) const {
        const double aspect = std::max(major, minor) / std::min(major, minor);
        const int extra_segments = static_cast<int>(std::ceil(aspect - 1.0)); // More segments for high aspect
        const double sweep = a2 - a1;
        const int n = static_cast<int>(std::ceil(std::abs(sweep) * 24. / M_PI)) + extra_segments; // Quadrants + extra
        if (n <= 0) {
            return; // Degenerate case
        }
        const double dt = sweep / n;
        const double lambda = (4.0 / 3.0) * std::tan(dt / 4.0); // Bezier control factor (constant)
        double current_t = a1;

        // Initial endpoint and tangent for first segment
        RS_Vector p0 = e_point(center, major, minor, rot, current_t);
        RS_Vector prime0 = e_prime(major, minor, rot, current_t) * lambda;

        if ((path.currentPosition() - QPointF{p0.x, p0.y}).manhattanLength() >= RS_TOLERANCE * 100.) {
            path.moveTo(p0.x, p0.y);
        }
        else {
            path.lineTo(p0.x, p0.y);
        }

        for (int i = 0; i < n; ++i) {
            const double t1 = current_t + dt;
            RS_Vector p3 = e_point(center, major, minor, rot, t1);
            RS_Vector prime1 = e_prime(major, minor, rot, t1) * lambda;
            const RS_Vector p1 = p0 + prime0;
            const RS_Vector p2 = p3 - prime1;
            path.cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);

            // Reuse for next segment
            p0 = p3;
            prime0 = prime1;
            current_t = t1;
        }
    }

    /**
     * @brief Converts container entities to QPainterPath, handling lines, arcs, circles, ellipses, splines, and parabolas.
     * Closes the subpath; skips unsupported types.
     */
    QPainterPath LC_Loops::buildPathFromLoop(RS_Painter* painter, const RS_EntityContainer& cont) const {
        PathBuilder builder{painter};
        QPainterPath& path = builder.getPath();
        if (cont.empty()) {
            return path;
        }

        // single closed entities may not have defined start/end points
        RS_Entity* first = cont.first();
        if (isClosed(*first)) {
            builder.append(first);
            builder.closeSubpath();
            return path;
        }

         //builder.moveTo(cont.first()->getStartpoint());
  for (RS_Entity* e : cont) {
    if (e->isAtomic()) {
      RS_Vector start = e->getStartpoint();
      if (!path.isEmpty() && RS_DEBUG->getLevel() >= RS_Debug::D_INFORMATIONAL) {
        if ((path.currentPosition() - builder.toGuiPoint({start.x, start.y})).manhattanLength() >= 3.) {
            LC_ERR << __func__ << "(): gap before (" << start.x << ", " << start.y << ")";
        }
      }
      builder.append(e);
    }
  }
  builder.closeSubpath();
  if (RS_DEBUG->getLevel() > RS_Debug::D_WARNING) {
      debugPath(path);
  }
  return path;
}

    /**
     * @brief Recursively collects all descendant loop containers.
     */
    void LC_Loops::getAllLoops(std::vector<const RS_EntityContainer*>& loops) const {
        if (m_loop) {
            loops.push_back(m_loop.get());
        }
        for (const auto& child : m_children) {
            child.getAllLoops(loops);
        }
    }

    /**
     * @brief Gets the bounding box from the outer loop.
     */
    LC_Rect LC_Loops::getBoundingBox() const {
        if (m_loop == nullptr) {
            return {};
        }
        m_loop->calculateBorders();
        return {m_loop->getMin(), m_loop->getMax()};
    }

    /**
     * @brief Alias for isInside (full classifier).
     */
    bool LC_Loops::isPointInside(const RS_Vector& p) const {
        return classify(p) != PointClass::Outside;
    }

    /**
     * @brief Flattens all atomic boundary entities from the hierarchy.
     */
    std::vector<RS_Entity*> LC_Loops::getAllBoundaries() const {
        std::vector<const RS_EntityContainer*> loops;
        getAllLoops(loops);
        std::vector<RS_Entity*> bounds;
        for (auto* l : loops) {
            for (RS_Entity* e : *l) {
                if (e->isAtomic()) {
                    bounds.push_back(e);
                }
            }
        }
        return bounds;
    }

bool LC_Loops::isEntityClosed(const RS_Entity* e) const {
  return isClosed(*e);
}

    /**
     * @brief Generates tile offsets for pattern repetition over this tree's
     * bounding box, anchored at latticeAnchor.
     *
     * Tile indices are half-open [floor((bbMin - a)/P), ceil((bbMax - a)/P))
     * on each axis; the pattern's minimum corner sits on the lattice points
     * latticeAnchor + (i*W, k*H), as in 2.2.1. Each tile is kept when its
     * rect intersects bbox *and* either overlaps the loop or contains the
     * pattern's centre.
     */
    std::vector<RS_Vector> LC_Loops::createTiles(const RS_Pattern& pattern,
                                                 const RS_Vector& latticeAnchor) const {
        const LC_Rect bBox = getBoundingBox();
        const LC_Rect pBox{pattern.getMin(), pattern.getMax()};
        const double pWidth = pBox.width();
        const double pHeight = pBox.height();
        if (pWidth < 1e-6 || pHeight < 1e-6) // Skip degenerate patterns
        {
            return {};
        }
        // Half-open [floor, ceil) range of tile indices; indices stay in
        // double/long long because |bbMin - anchor|/W can exceed 2^31 for a
        // tiny tile far from the origin, and the cast would be UB in int.
        // Never name these j0/j1 (glibc Bessel functions).
        const double iFirst = std::floor((bBox.minP().x - latticeAnchor.x) / pWidth);
        const double iEnd = std::ceil((bBox.maxP().x - latticeAnchor.x) / pWidth);
        const double kFirst = std::floor((bBox.minP().y - latticeAnchor.y) / pHeight);
        const double kEnd = std::ceil((bBox.maxP().y - latticeAnchor.y) / pHeight);
        // Use pattern center for inside check
        const RS_Vector pCenter = (pBox.lowerLeftCorner() + pBox.upperRightCorner()) / 2.0;
        const long long nIx = static_cast<long long>(iEnd - iFirst);
        const long long nIy = static_cast<long long>(kEnd - kFirst);
        std::vector<RS_Vector> tiles;
        for (long long ci = 0; ci < nIx; ++ci) {
            for (long long ck = 0; ck < nIy; ++ck) {
                const RS_Vector tile = latticeAnchor
                    + RS_Vector{(iFirst + static_cast<double>(ci)) * pWidth,
                                (kFirst + static_cast<double>(ck)) * pHeight}
                    - pBox.lowerLeftCorner();
                LC_Rect tileRect{pBox.lowerLeftCorner() + tile, pBox.upperRightCorner() + tile};
                // Quick bbox intersection
                if (!tileRect.intersects(bBox)) {
                    continue;
                }
                // Include if overlaps or center inside
                if (overlap(tileRect) || isPointInside(tile + pCenter)) {
                    tiles.push_back(tile);
                }
            }
        }
        return tiles;
    }

    /**
     * @brief Clips every tile copy of every atomic pattern entity to this loop
     * hierarchy: intersect with all boundary edges, sort, split, and keep the
     * sub-pieces whose midpoint is inside. Copies are never extended.
     */
    std::unique_ptr<RS_EntityContainer> LC_Loops::trimPatternEntities(const RS_Pattern& pattern,
                                                                      const RS_Vector& latticeAnchor) const {
        auto trimmed = std::make_unique<RS_EntityContainer>();
        std::vector<RS_Vector> tiles = createTiles(pattern, latticeAnchor);
        auto boundaries = getAllBoundaries();
        PointClassifier classifier = [this](const RS_Vector& q) {
            return this->classify(q);
        };
        const double epsOn = (m_eps > 0.0) ? m_eps : RS_TOLERANCE;
        for (const RS_Vector& tile : tiles) {
            for (RS_Entity* e : pattern) {
                if (!e->isAtomic()) {
                    continue;
                }
                auto cloned = std::unique_ptr<RS_Entity>(e->clone());
                cloned->move(tile);

      std::vector<RS_Vector> all_inters;
      for (auto* b : boundaries) {
        RS_VectorSolutions sol = RS_Information::getIntersection(cloned.get(), b, true);
        for (RS_Vector v : sol) {
          all_inters.push_back(v);
        }
      }
      // Remove duplicates
      std::sort(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
        return a.x < b.x || (RS_Math::equal(a.x, b.x) && a.y < b.y);
      });
      auto last = std::unique(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
        return a.distanceTo(b) < RS_TOLERANCE;
      });
      all_inters.erase(last, all_inters.end());
      auto sorted_inters = sortPointsAlongEntity(cloned.get(), all_inters);
      bool closed = isEntityClosed(cloned.get());
      if (sorted_inters.empty()) {
        if (keepPiece(*cloned, classifier, epsOn)) {
          cloned->setVisible(true);
          trimmed->addEntity(cloned.release());
        }
        continue;
      }
      if (closed && sorted_inters.size() % 2 != 0) {
        RS_DEBUG->print("LC_Loops::trimPatternEntities: Odd intersections for closed entity, skipping");
        continue;
      }
      std::vector<RS_Vector> points;
      if (!closed) {
        points.push_back(cloned->getStartpoint());
        points.insert(points.end(), sorted_inters.begin(), sorted_inters.end());
        points.push_back(cloned->getEndpoint());
      } else {
        points = sorted_inters;
      }
      for (size_t i = 0; i < points.size() - 1; ++i) {
        RS_Vector p1 = points[i];
        RS_Vector p2 = points[i + 1];
        if (p1.distanceTo(p2) < RS_TOLERANCE) {
            continue;
        }
        auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), p1, p2));
        // Filter short subs and add if the collinear-safe keepPiece says yes.
        if (sub && sub->getLength() > RS_TOLERANCE && keepPiece(*sub, classifier, epsOn)) {
          sub->setVisible(true);
          trimmed->addEntity(sub.release());
        }
      }
      if (closed && sorted_inters.size() > 0) {
        RS_Vector pw1 = points.back();
        RS_Vector pw2 = points[0];
        if (pw1.distanceTo(pw2) >= RS_TOLERANCE) {
          auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), pw1, pw2));
          if (sub && sub->getLength() > RS_TOLERANCE && keepPiece(*sub, classifier, epsOn)) {
            sub->setVisible(true);
            trimmed->addEntity(sub.release());
          }
        }
      }
    }
  }
  return trimmed;
}

/**
 * @brief Creates a trimmed sub-entity (line/arc/circle/ellipse/spline/parabola) between two points.
 * Handles type-specific parameterization.
 */
RS_Entity* LC_Loops::createSubEntity(RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) const {
  RS2::EntityType type = e->rtti();
  if (type == RS2::EntityLine) {
    return new RS_Line(nullptr, RS_LineData(p1, p2));
  } else if (type == RS2::EntityArc) {
    auto arc = static_cast<RS_Arc*>(e);
    RS_Vector center = arc->getCenter();
    double ang1 = (p1 - center).angle();
    double ang2 = (p2 - center).angle();
    return new RS_Arc(nullptr, RS_ArcData(center, arc->getRadius(), ang1, ang2, arc->isReversed()));
  } else if (type == RS2::EntityCircle) {
    auto circle = static_cast<RS_Circle*>(e);
    RS_Vector center = circle->getCenter();
    double ang1 = (p1 - center).angle();
    double ang2 = (p2 - center).angle();
    return new RS_Arc(nullptr, RS_ArcData(center, circle->getRadius(), ang1, ang2, false));
  } else if (type == RS2::EntityEllipse) {
    auto ell = static_cast<RS_Ellipse*>(e);
    RS_Vector center = ell->getCenter();
    double rot = ell->getAngle();
    RS_Vector lp1 = (p1 - center).rotate(-rot);
    double lang1 = std::atan2(lp1.y / ell->getMinorRadius(), lp1.x / ell->getMajorRadius());
    RS_Vector lp2 = (p2 - center).rotate(-rot);
    double lang2 = std::atan2(lp2.y / ell->getMinorRadius(), lp2.x / ell->getMajorRadius());
    return new RS_Ellipse(nullptr, {center, ell->getMajorP(), ell->getRatio(), lang1, lang2, ell->isReversed()});
  } else if (type == RS2::EntitySplinePoints) {
    auto spl = static_cast<LC_SplinePoints*>(e);
    double total_len = spl->getLength();
    if (total_len < RS_TOLERANCE) {
        return nullptr;
    }
    double t1 = spl->getDistanceToPoint(p1) / total_len;
    double t2 = spl->getDistanceToPoint(p2) / total_len;
    if (t1 > t2) {
        std::swap(t1, t2); // Ensure order
    }
    LC_SplinePoints* seg1 = spl->cut(p1);  // Trims original to start-p1
    if (seg1) {
      LC_SplinePoints* sub = seg1->cut(p2);  // Further trim p1-p2
      if (sub && sub->getLength() > RS_TOLERANCE) {
          return sub;
      }
      delete seg1;  // Cleanup
    }
    // Fallback: Resample splinePoints between nearest indices
    auto points = spl->getPoints();
    size_t i1 = 0, i2 = points.size() - 1;
    double min_d1 = RS_MAXDOUBLE, min_d2 = RS_MAXDOUBLE;
    for (size_t i = 0; i < points.size(); ++i) {
      double d = points[i].distanceTo(p1);
      if (d < min_d1) { min_d1 = d; i1 = i; }
      d = points[i].distanceTo(p2);
      if (d < min_d2) { min_d2 = d; i2 = i; }
    }
    if (i1 > i2) {
        std::swap(i1, i2);
    }
    LC_SplinePointsData sub_data;
    sub_data.splinePoints.assign(points.begin() + i1, points.begin() + i2 + 1);
    return new LC_SplinePoints(nullptr, sub_data);
  } else if (type == RS2::EntityParabola) {
    auto para = static_cast<LC_Parabola*>(e);
    RS_Vector tan1 = para->getTangentDirection(p1).normalize();
    RS_Vector tan2 = para->getTangentDirection(p2).normalize();
    std::array<RS_Vector, 2> ends = {p1, p2};
    std::array<RS_Vector, 2> tans = {tan1, tan2};
    LC_ParabolaData sub_data = LC_ParabolaData::FromEndPointsTangents(ends, tans);
    if (sub_data.m_valid) {
      return new LC_Parabola(nullptr, sub_data);
    }
    // Fallback: Interpolate controls
    auto cps = para->getData().m_controlPoints;
    std::sort(cps.begin(), cps.end(), [](const RS_Vector& a, const RS_Vector& b){ return a.x < b.x; });
    double x1 = p1.x, x2 = p2.x;
    if (x1 > x2) {
        std::swap(x1, x2);
    }
    std::array<RS_Vector, 3> sub_cps;
    double range = x2 - x1;
    if (range < RS_TOLERANCE) {
        return nullptr;
    }
    for (int i = 0; i < 3; ++i) {
      double ratio = (cps[i].x - x1) / range;
      if (ratio < 0) {
          ratio = 0;
      }
      else if (ratio > 1) {
          ratio = 1;
      }
      sub_cps[i] = RS_Vector{x1 + ratio * range, cps[i].y};  // Preserve y for parabolic shape
    }
    return new LC_Parabola(nullptr, LC_ParabolaData{sub_cps});
  }
  return nullptr;
}

/**
 * @brief Sorts intersection points by parameterization along the entity (t for lines, angle for curves, arc-len/x-param for splines/parabolas).
 */
std::vector<RS_Vector> LC_Loops::sortPointsAlongEntity(RS_Entity* e, std::vector<RS_Vector> inters) const {
  std::vector<std::pair<double, RS_Vector>> param_points;
  RS2::EntityType type = e->rtti();
  if (type == RS2::EntityLine) {
    auto line = static_cast<RS_Line*>(e);
    RS_Vector start = line->getStartpoint();
    RS_Vector dir = line->getEndpoint() - start;
    double len = dir.magnitude();
    if (len < RS_TOLERANCE) {
        return {};
    }
    RS_Vector unit = dir / len;
    for (RS_Vector v : inters) {
      double t = (v - start).dotP(unit);
      if (t >= 0 - RS_TOLERANCE && t <= len + RS_TOLERANCE) {
          param_points.emplace_back(t, v);
      }
    }
  } else if (type == RS2::EntityArc) {
    auto arc = static_cast<RS_Arc*>(e);
    RS_Vector center = arc->getCenter();
    double a1 = arc->getAngle1();
    bool reversed = arc->isReversed();
    for (RS_Vector v : inters) {
      double ang = (v - center).angle();
      double diff = RS_Math::getAngleDifference(a1, ang, reversed);
      param_points.emplace_back(diff, v);
    }
  } else if (type == RS2::EntityCircle) {
    auto circle = static_cast<RS_Circle*>(e);
    RS_Vector center = circle->getCenter();
    if (!inters.empty()) {
      double ref_ang = (inters[0] - center).angle();
      for (RS_Vector v : inters) {
        double ang = (v - center).angle();
        double diff = RS_Math::correctAngle(ang - ref_ang);
        param_points.emplace_back(diff, v);
      }
    }
  } else if (type == RS2::EntityEllipse) {
    auto ell = static_cast<RS_Ellipse*>(e);
    RS_Vector center = ell->getCenter();
    double rot = ell->getAngle();
    double a1 = ell->getAngle1();
    bool reversed = ell->isReversed();
    for (RS_Vector v : inters) {
      RS_Vector local = (v - center).rotate(-rot);
      double ang = std::atan2(local.y / ell->getMinorRadius(), local.x / ell->getMajorRadius());
      double diff = RS_Math::getAngleDifference(a1, ang, reversed);
      param_points.emplace_back(diff, v);
    }
  } else if (type == RS2::EntitySplinePoints) {
    auto spl = static_cast<LC_SplinePoints*>(e);
    double total_len = spl->getLength();
    if (total_len < RS_TOLERANCE) {
        return {};
    }
    for (RS_Vector v : inters) {
      double dist = spl->getDistanceToPoint(v);
      param_points.emplace_back(dist / total_len, v);
    }
  } else if (type == RS2::EntityParabola) {
    auto para = static_cast<LC_Parabola*>(e);
    LC_ParabolaData& d = para->getData();
    if (!d.m_valid) {
        return {};
    }
    double x_min = std::min({d.m_controlPoints[0].x, d.m_controlPoints[1].x, d.m_controlPoints[2].x});
    double x_max = std::max({d.m_controlPoints[0].x, d.m_controlPoints[1].x, d.m_controlPoints[2].x});
    double x_range = x_max - x_min;
    if (x_range < RS_TOLERANCE) {
        return {};
    }
    for (RS_Vector v : inters) {
      double x_param = d.findX(v);
      double norm_param = (x_param - x_min) / x_range;  // [0,1]
      param_points.emplace_back(norm_param, v);
    }
  }
  std::sort(param_points.begin(), param_points.end(), [](const auto& a, const auto& b){
    return a.first < b.first;
  });
  std::vector<RS_Vector> sorted;
  for (auto& p : param_points) {
      sorted.push_back(p.second);
  }
  return sorted;
}

    /**
     * @brief Recursive overlap check: Any entity bbox overlaps or child overlaps.
     */
    bool LC_Loops::overlap(const LC_Rect& other) const {
        const bool ret = std::any_of(m_loop->begin(), m_loop->end(), [&other](const RS_Entity* entity) {
            return entity != nullptr && other.overlaps(LC_Rect{entity->getMin(), entity->getMax()});
        });
        return ret || std::any_of(m_children.cbegin(), m_children.cend(), [&other](const LC_Loops& loop) {
            return loop.overlap(other);
        });
    }
} // namespace LC_LoopUtils
