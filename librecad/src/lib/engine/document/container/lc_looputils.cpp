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
#include <cfloat>
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
//
// For an RS_Arc or an elliptic arc, "closed" means the stored sweep rounds to
// a full turn AND the start/end points of the stored geometry coincide within
// eps. The second check guards against polyline bulges whose sweep tolerance
// snaps to 2*pi but whose endpoints are far apart in model space; callers that
// want today's angle-only behaviour pass eps = 0 and only an exact start/end
// match is accepted.
bool isClosed(const RS_Entity& en, double eps = 0.0) {
  switch (en.rtti()) {
  case RS2::EntityCircle:
    return true;
  case RS2::EntityEllipse: {
    const auto& ell = static_cast<const RS_Ellipse&>(en);
    // RS_Entity::isArc() returns true for every ellipse; the m_data.isArc
    // flag — exposed through isEllipticArc() — says whether this is actually
    // an arc rather than a full ellipse.
    if (!ell.isEllipticArc()) return true;
    if (std::abs(ell.getAngleLength() - 2.0 * M_PI) >= RS_TOLERANCE_ANGLE)
      return false;
    return ell.getStartpoint().distanceTo(ell.getEndpoint()) <= eps;
  }
  case RS2::EntityArc: {
    const auto& arc = static_cast<const RS_Arc&>(en);
    if (std::abs(arc.getAngleLength() - 2.0 * M_PI) >= RS_TOLERANCE_ANGLE)
      return false;
    return arc.getStartpoint().distanceTo(arc.getEndpoint()) <= eps;
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

/**
 * One sub-loop produced by splitAtRepeatedVertices: the owned cloned edges and
 * their start-vertex table (coordinates and welded ids) in traversal order.
 * The returned clones keep the traversal direction of the input chain; a
 * caller orients each sub-loop to CCW before storing it.
 */
struct SplitSubLoop {
    std::unique_ptr<RS_EntityContainer> edges;
    std::vector<RS_Vector> vertices;
    std::vector<int> vertexIds;
};

/**
 * Splits a closed chain at every repeated start-vertex into simple sub-loops.
 *
 * Two edges share a start-vertex id when their welded representative is the
 * same (edgeStartIds carries one id per input edge, parallel to the
 * loop.entityAt() order; vertexCoords carries the matching welded coordinate).
 * When an edge whose start id matches an earlier entry on the current stack
 * arrives, the stack is truncated at that earlier entry and the popped edges
 * form their own closed sub-loop; the remainder closes at the initial start
 * vertex when the chain is traversed to the end. Each sub-loop's edges are
 * cloned into its own container, preserving the input traversal order.
 */
std::vector<SplitSubLoop> splitAtRepeatedVertices(
    const RS_EntityContainer& loop,
    const std::vector<int>& edgeStartIds,
    const std::vector<RS_Vector>& vertexCoords) {
    std::vector<SplitSubLoop> out;
    const unsigned n = loop.count();
    if (n == 0) return out;
    // If the id table is missing or short, treat the whole chain as one loop.
    const bool haveIds = (edgeStartIds.size() == n) && (vertexCoords.size() == n);
    if (!haveIds) {
        auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
        for (unsigned i = 0; i < n; ++i) {
            RS_Entity* copy = loop.entityAt(i)->clone();
            copy->setParent(nullptr);
            copy->setLayer(nullptr);
            cont->addEntity(copy);
        }
        out.push_back({std::move(cont), {}, {}});
        return out;
    }

    auto flush = [&](const std::vector<unsigned>& edgeIdx,
                     const std::vector<RS_Vector>& vs,
                     const std::vector<int>& ids) {
        if (edgeIdx.empty()) return;
        auto cont = std::make_unique<RS_EntityContainer>(nullptr, true);
        for (unsigned i : edgeIdx) {
            RS_Entity* copy = loop.entityAt(i)->clone();
            copy->setParent(nullptr);
            copy->setLayer(nullptr);
            cont->addEntity(copy);
        }
        out.push_back({std::move(cont), vs, ids});
    };

    std::unordered_map<int, unsigned> pos;
    std::vector<unsigned> stackEdge;
    std::vector<RS_Vector> stackV;
    std::vector<int> stackIds;

    for (unsigned i = 0; i < n; ++i) {
        const int idHere = edgeStartIds[i];
        auto it = pos.find(idHere);
        if (it != pos.end()) {
            const unsigned k = it->second;
            std::vector<unsigned> subEdge(stackEdge.begin() + k, stackEdge.end());
            std::vector<RS_Vector> subV(stackV.begin() + k, stackV.end());
            std::vector<int> subIds(stackIds.begin() + k, stackIds.end());
            flush(subEdge, subV, subIds);
            // Pop the sub-loop's positions out of the stack and the map.
            for (unsigned j = k; j < stackIds.size(); ++j) {
                pos.erase(stackIds[j]);
            }
            stackEdge.resize(k);
            stackV.resize(k);
            stackIds.resize(k);
        }
        pos[idHere] = static_cast<unsigned>(stackEdge.size());
        stackEdge.push_back(i);
        stackV.push_back(vertexCoords[i]);
        stackIds.push_back(idHere);
    }
    // The leftover stack closes at stackIds[0].
    flush(stackEdge, stackV, stackIds);
    return out;
}

}  // anonymous namespace

namespace LC_LoopUtils {

// Private implementation for LoopExtractor
struct LoopExtractor::LoopData {
  std::vector<RS_Entity*> unprocessed;
  std::map<RS_Entity*, bool> processed;
  RS_Entity* current = nullptr;
  // Per-hatch tolerance in effect for this extractor. Reused for the length
  // filter, join welding, the near-flat arc rule, isClosed() ends, chain
  // validation and the degenerate-area cut.
  double eps = 0.0;
  // Welded vertex ids per original edge. endId[e] and startId[e] exist only
  // for non-closed-atomic edges.
  std::unordered_map<const RS_Entity*, int> startId;
  std::unordered_map<const RS_Entity*, int> endId;
  // adjacency[v] lists every edge whose startId or endId equals v, in input
  // (container) order; an edge is listed twice when it has startId == endId.
  std::vector<std::vector<RS_Entity*>> adjacency;
  // Welded coordinate per representative vertex id.
  WeldResult weld;
  // Chain walk state: the welded id of the target (the chain's start) and of
  // the chain's current end. -1 while a chain is a single closed atomic.
  int targetId = -1;
  int curId = -1;
  // Stable container-order index, filled in the ctor. Used by findFirst() and
  // findOutermost() for a deterministic tie-break between candidate edges.
  std::unordered_map<const RS_Entity*, std::size_t> order;
};

/**
 * @brief Constructs LoopExtractor and initialises its welded adjacency.
 *
 * Keeps every atomic edge whose stored length exceeds eps. For non-closed-atomic
 * edges the start and end point enter a joint endpoint list; near-flat arcs
 * carry a locally-loose tolerance max(eps, 64*u*(|c|+r)) so a tiny arc
 * whose chord ends sit above eps still welds. The resulting WeldResult
 * assigns each endpoint a representative id; startId/endId map every kept
 * edge to its two ids, and adjacency[v] lists the edges that touch vertex v.
 * Closed-atomic edges are kept in unprocessed with no id entries; they are
 * emitted as single-entity loops by extractLoops().
 *
 * eps > 0 overrides any computation; eps <= 0 asks the extractor to compute
 * its own per-hatch tolerance from the input bbox and endpoint extrema (the
 * form used by LoopOptimizer and the test harness).
 */
LoopExtractor::LoopExtractor(const RS_EntityContainer& edges, double eps) :
                                                                m_data(std::make_unique<LoopData>())
{
  // First pass: record container-order index and gather candidate atomic edges.
  std::vector<RS_Entity*> candidates;
  {
    std::size_t idx = 0;
    for (RS_Entity* e : edges) {
      if (e && e->isAtomic()) {
        candidates.push_back(e);
        m_data->order[e] = idx;
      }
      ++idx;
    }
  }

  // Derive eps from the input geometry when the caller did not provide one.
  if (!(eps > 0.0) || !std::isfinite(eps)) {
    RS_Vector bbMin(RS_MAXDOUBLE, RS_MAXDOUBLE);
    RS_Vector bbMax(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
    double maxAbs = 0.0;
    bool anyBox = false;
    for (RS_Entity* e : candidates) {
      const RS_Vector mn = e->getMin();
      const RS_Vector mx = e->getMax();
      if (mn.valid && mx.valid) {
        bbMin.x = std::min(bbMin.x, mn.x);
        bbMin.y = std::min(bbMin.y, mn.y);
        bbMax.x = std::max(bbMax.x, mx.x);
        bbMax.y = std::max(bbMax.y, mx.y);
        anyBox = true;
      }
      if (isClosed(*e, 0.0)) {
        if (mn.valid && mx.valid) {
          maxAbs = std::max({maxAbs, std::abs(mn.x), std::abs(mn.y),
                                       std::abs(mx.x), std::abs(mx.y)});
        }
      } else {
        const RS_Vector s = e->getStartpoint();
        const RS_Vector t = e->getEndpoint();
        if (s.valid)
          maxAbs = std::max({maxAbs, std::abs(s.x), std::abs(s.y)});
        if (t.valid)
          maxAbs = std::max({maxAbs, std::abs(t.x), std::abs(t.y)});
      }
    }
    const double diag = anyBox ? (bbMax - bbMin).magnitude() : 0.0;
    eps = hatchTolerance(diag, maxAbs);
  }
  m_data->eps = eps;

  // Second pass: filter by length, keep track of each kept edge's endpoint
  // slots in the global list, and record per-edge arc tolerance.
  std::vector<RS_Vector> endpoints;
  std::vector<double> looseTol;
  std::unordered_map<const RS_Entity*, std::pair<int, int>> endpointIdx;
  constexpr double u = DBL_EPSILON / 2.0;
  for (RS_Entity* e : candidates) {
    if (!(e->getLength() > eps)) continue;
    m_data->unprocessed.push_back(e);
    m_data->processed[e] = false;
    if (isClosed(*e, eps)) {
      // Closed atomic has no vertex ids and never enters adjacency.
      continue;
    }
    const int sSlot = static_cast<int>(endpoints.size());
    const int eSlot = sSlot + 1;
    endpoints.push_back(e->getStartpoint());
    endpoints.push_back(e->getEndpoint());
    double tol = eps;
    if (e->rtti() == RS2::EntityArc) {
      const auto& arc = static_cast<const RS_Arc&>(*e);
      const double absR = std::abs(arc.getRadius());
      const double absC = std::abs(arc.getCenter().x) + std::abs(arc.getCenter().y);
      tol = std::max(eps, 64.0 * u * (absC + absR));
    }
    looseTol.push_back(tol);
    looseTol.push_back(tol);
    endpointIdx[e] = {sSlot, eSlot};
  }

  // Weld coincident endpoints using the per-edge arc tolerance where set.
  m_data->weld = weldPoints(endpoints, eps, looseTol);

  // Build the id → edges adjacency.
  m_data->adjacency.assign(m_data->weld.rep.size(), {});
  for (RS_Entity* e : m_data->unprocessed) {
    auto it = endpointIdx.find(e);
    if (it == endpointIdx.end()) continue;  // closed-atomic
    const int sId = m_data->weld.id[it->second.first];
    const int eId = m_data->weld.id[it->second.second];
    m_data->startId[e] = sId;
    m_data->endId[e] = eId;
    if (sId >= 0 && sId < static_cast<int>(m_data->adjacency.size())) {
      m_data->adjacency[sId].push_back(e);
    }
    if (eId != sId && eId >= 0 && eId < static_cast<int>(m_data->adjacency.size())) {
      m_data->adjacency[eId].push_back(e);
    }
  }
}

LoopExtractor::~LoopExtractor() = default;

/**
 * @brief Extracts closed loops iteratively until no unprocessed edges remain.
 * Builds each loop by chaining connected edges through the welded vertex ids,
 * clones and orients each sub-loop for positive area, snaps RS_Line endpoints
 * to their welded representatives (curves keep their geometry), and records
 * both the per-edge start-point coordinate and its welded id for use by the
 * monotone classifier and splitter.
 */
std::vector<ExtractedLoop> LoopExtractor::extractLoops() const {
  std::vector<ExtractedLoop> results;
  const double eps = m_data->eps;
  while (!m_data->unprocessed.empty()) {
    m_loop = std::make_unique<RS_EntityContainer>();
    const FirstEdge choice = findFirst();
    RS_Entity* first = choice.edge;
    if (!first) break;

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
    m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), first), m_data->unprocessed.end());

    // Chain-walk ids for this loop. For a closed-atomic single-edge loop both
    // walk ids stay at the sentinel -1 and the ids table stays empty.
    std::vector<int> walkStartIds;
    const bool firstClosed = isClosed(*first, eps);
    if (!firstClosed) {
      const auto sIt = m_data->startId.find(first);
      const auto eIt = m_data->endId.find(first);
      if (sIt != m_data->startId.end() && eIt != m_data->endId.end()) {
        const int sId = sIt->second;
        const int eId = eIt->second;
        if (choice.reverse) {
          m_data->targetId = eId;
          m_data->curId = sId;
        } else {
          m_data->targetId = sId;
          m_data->curId = eId;
        }
        walkStartIds.push_back(m_data->targetId);
      } else {
        m_data->targetId = -1;
        m_data->curId = -1;
      }
    } else {
      m_data->targetId = -1;
      m_data->curId = -1;
    }

    size_t iteration = 0;
    const size_t maxIterations = m_data->unprocessed.size() + 2;
    while (!firstClosed && m_data->curId >= 0 && m_data->curId != m_data->targetId) {
      if (++iteration > maxIterations) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "LoopExtractor: possible degenerate loop detected");
        break;
      }
      const int before = m_data->curId;
      if (findNext()) {
        walkStartIds.push_back(before);
        if (m_data->curId == m_data->targetId) break;
      } else {
        break;
      }
    }

    if (!validate()) {
      RS_DEBUG->print("LoopExtractor: Invalid loop discarded");
      continue;
    }

    // Build the per-edge welded coordinate table from the walk ids.
    std::vector<RS_Vector> walkCoords;
    walkCoords.reserve(walkStartIds.size());
    for (int id : walkStartIds) {
      if (id >= 0 && id < static_cast<int>(m_data->weld.rep.size())) {
        walkCoords.push_back(m_data->weld.rep[static_cast<std::size_t>(id)]);
      } else {
        walkCoords.push_back(RS_Vector(false));
      }
    }

    // Split pinched loops at any repeated start vertex: a 7-edge figure-8
    // chain in one container must come back as two simple sub-loops.
    const unsigned chainN = m_loop->count();
    std::vector<SplitSubLoop> subLoops;
    if (chainN <= 1) {
      SplitSubLoop single;
      single.edges = std::move(m_loop);
      if (chainN == 1 && !isClosed(*single.edges->entityAt(0), eps)) {
        if (!walkCoords.empty()) {
          single.vertices.push_back(walkCoords.front());
          single.vertexIds.push_back(walkStartIds.front());
        } else {
          single.vertices.push_back(single.edges->entityAt(0)->getStartpoint());
          single.vertexIds.push_back(-1);
        }
      }
      subLoops.push_back(std::move(single));
    } else {
      subLoops = splitAtRepeatedVertices(*m_loop, walkStartIds, walkCoords);
      m_loop.reset();
    }

    for (SplitSubLoop& sub : subLoops) {
      if (!sub.edges || sub.edges->count() == 0) continue;
      const double area = sub.edges->areaLineIntegral();
      // Drop degenerate sub-loops: |A| <= eps * perimeter.
      double perimeter = 0.0;
      for (unsigned k = 0; k < sub.edges->count(); ++k) {
        perimeter += sub.edges->entityAt(k)->getLength();
      }
      if (std::abs(area) <= eps * perimeter) continue;
      if (area < 0.0) {
        auto new_loop = std::make_unique<RS_EntityContainer>(nullptr, true);
        const unsigned m = sub.edges->count();
        for (int i = static_cast<int>(m) - 1; i >= 0; --i) {
          RS_Entity* e = sub.edges->entityAt(static_cast<unsigned>(i))->clone();
          e->setParent(nullptr);
          e->setLayer(nullptr);
          e->revertDirection();
          new_loop->addEntity(e);
        }
        sub.edges = std::move(new_loop);
        // Reverse the vertex table so vertices[i] still names the start of
        // edge i. Chain is v0 -> e0 -> v1 -> e1 -> ... -> v_{n-1} -> e_{n-1} -> v0;
        // reversed chain starts at v0 (the old last end = the old chain start),
        // so the new vertex table is v0, v_{n-1}, v_{n-2}, ..., v_1.
        if (!sub.vertices.empty()) {
          std::vector<RS_Vector> rv;
          std::vector<int> rids;
          rv.reserve(sub.vertices.size());
          rids.reserve(sub.vertexIds.size());
          rv.push_back(sub.vertices.front());
          rids.push_back(sub.vertexIds.empty() ? -1 : sub.vertexIds.front());
          for (int k = static_cast<int>(sub.vertices.size()) - 1; k >= 1; --k) {
            rv.push_back(sub.vertices[static_cast<std::size_t>(k)]);
            if (!sub.vertexIds.empty()) {
              rids.push_back(sub.vertexIds[static_cast<std::size_t>(k)]);
            }
          }
          sub.vertices = std::move(rv);
          sub.vertexIds = std::move(rids);
        }
      }

      // Snap RS_Line endpoints to their welded representatives: this is the
      // one place line-only chains collapse their 1e-10 noise to exact equal
      // vertices, without touching curved edge geometry.
      const unsigned m = sub.edges->count();
      if (!sub.vertices.empty() && m == sub.vertices.size()) {
        for (unsigned k = 0; k < m; ++k) {
          RS_Entity* e = sub.edges->entityAt(k);
          if (e && e->rtti() == RS2::EntityLine) {
            auto* line = static_cast<RS_Line*>(e);
            const RS_Vector& v0 = sub.vertices[k];
            const RS_Vector& v1 = sub.vertices[(k + 1) % m];
            if (v0.valid) line->setStartpoint(v0);
            if (v1.valid) line->setEndpoint(v1);
          }
        }
        // Recompute borders after snapping.
        sub.edges->forcedCalculateBorders();
      }

      ExtractedLoop el;
      el.vertices = std::move(sub.vertices);
      el.vertexIds = std::move(sub.vertexIds);
      el.edges = std::move(sub.edges);
      results.push_back(std::move(el));
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
 * @brief Validates loop closure and connectivity using the extractor's eps.
 * Multi-entity chains must connect end-to-start within eps and close within
 * eps; single-entity loops must be a recognised closed-atomic primitive or a
 * degenerate self-closing line. Loops with |A| <= eps * perimeter are
 * rejected as geometrically zero-area.
 */
bool LoopExtractor::validate() const {
  const size_t n = m_loop->count();
  if (n == 0) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "LoopExtractor::validate: Empty loop");
    return false;
  }
  const double eps = m_data->eps;

         // Special case: single closed entity (no chain to check)
  if (n == 1) {
    RS_Entity* e = m_loop->entityAt(0);
    const RS2::EntityType type = e->rtti();
    if (type == RS2::EntityCircle) {
        return true;
    }
    if (type == RS2::EntityEllipse) {
        const auto* ell = static_cast<RS_Ellipse*>(e);
        if (!ell->isEllipticArc()) return true;  // full ellipse: closed by construction
        if (ell->getAngleLength() < 2 * M_PI - RS_TOLERANCE) return false;
        return ell->getStartpoint().distanceTo(ell->getEndpoint()) <= eps;
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
    return e->getStartpoint().distanceTo(e->getEndpoint()) <= eps;
  }

  // Multi-entity chain: trust the id-based walk. The extractor only appends
  // edges whose vertex ids match the current chain head, and the main loop
  // exits only when curId reaches targetId. A chain this validate sees is
  // therefore guaranteed to connect edge-by-edge on ids; adjacent clones'
  // raw endpoints may differ by up to the per-edge welded tolerance (a
  // near-flat arc's local tolerance can exceed eps), so we do not reject
  // on raw distance. Line endpoints get snapped to the welded coordinates
  // after validation so the stored chain is id-consistent on export.
  if (m_data->curId != m_data->targetId) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LoopExtractor::validate: Chain did not close (curId=%d, targetId=%d)",
                    m_data->curId, m_data->targetId);
    return false;
  }

  // Reject geometrically degenerate loops: |A| <= eps * perimeter.
  double perimeter = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const RS_Entity* en = m_loop->entityAt(i);
    if (en) perimeter += en->getLength();
  }
  const double area = m_loop->areaLineIntegral();
  if (std::abs(area) <= eps * perimeter) {
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "LoopExtractor::validate: Degenerate zero-area loop "
                    "(n=%zu, area=%.4e, limit=%.4e)",
                    n, area, eps * perimeter);
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

  // The leftmost point is a vertex: resolve its welded id from best.edge's
  // own startId/endId, then collect every edge attached at that id and pick
  // the one whose travel direction leaving the vertex has the smallest CCW
  // angle measured from -y.
  int idP = -1;
  {
    const double epsAttach = (m_data->eps > 0.0 ? m_data->eps : 1e-8);
    const RS_Vector s = best.edge->getStartpoint();
    const RS_Vector e = best.edge->getEndpoint();
    const auto sIt = m_data->startId.find(best.edge);
    const auto eIt = m_data->endId.find(best.edge);
    if (sIt != m_data->startId.end() && s.valid && s.distanceTo(best.point) <= epsAttach) {
      idP = sIt->second;
    } else if (eIt != m_data->endId.end() && e.valid && e.distanceTo(best.point) <= epsAttach) {
      idP = eIt->second;
    }
  }
  if (idP < 0 || idP >= static_cast<int>(m_data->adjacency.size())) {
    result.edge = best.edge;
    result.reverse = false;
    return result;
  }

  struct Attach {
    RS_Entity* edge = nullptr;
    bool atStart = false;
    double ccw = 0.0;
    double curv = 0.0;
    std::size_t idx = 0;
  };
  std::vector<Attach> attached;
  attached.reserve(m_data->adjacency[idP].size());
  for (RS_Entity* ae : m_data->adjacency[idP]) {
    auto pit = m_data->processed.find(ae);
    if (pit != m_data->processed.end() && pit->second) continue;
    const int sId = m_data->startId.count(ae) ? m_data->startId.at(ae) : -1;
    const int eId = m_data->endId.count(ae) ? m_data->endId.at(ae) : -1;
    const bool atStart = (sId == idP);
    const bool atEnd = (eId == idP);
    if (!atStart && !atEnd) continue;
    // Prefer start attachment if both ends land at this vertex.
    const bool pickStart = atStart;
    RS_Vector h = LC_LoopUtils::travelDirection(*ae, pickStart);
    if (!(h.valid && (std::abs(h.x) + std::abs(h.y) > 0.0))) continue;
    if (!pickStart) h = h * (-1.0);
    const double a = std::atan2(h.y, h.x);
    double ccw = a + M_PI_2;
    while (ccw < 0.0) ccw += 2.0 * M_PI;
    while (ccw >= 2.0 * M_PI) ccw -= 2.0 * M_PI;
    double curv = LC_LoopUtils::signedCurvature(*ae, pickStart);
    if (!pickStart) curv = -curv;
    const auto it = m_data->order.find(ae);
    const std::size_t idx = (it != m_data->order.end())
                               ? it->second
                               : static_cast<std::size_t>(-1);
    attached.push_back({ae, pickStart, ccw, curv, idx});
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
 * @brief Selects the next edge making the strongest left turn (maximum CCW
 * turn) using the type-uniform travelDirection and signedCurvature helpers.
 *
 * Ties within 1e-12 radians resolve to the larger signed left curvature
 * leaving the vertex (signedCurvature is negated when the vertex is the
 * stored end), then to the lower stored container index.
 */
RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const
{
  if (edges.empty()) {
      return nullptr;
  }
  if (edges.size() == 1) {
      return edges[0];
  }

  // Arrival direction: the travel tangent of the current edge at its end.
  const RS_Vector dinV =
      LC_LoopUtils::travelDirection(*m_data->current, /*atStart=*/false);
  const double din = std::atan2(dinV.y, dinV.x);

  struct Candidate {
      RS_Entity* edge = nullptr;
      double turn = 0.0;
      double curv = 0.0;
      std::size_t idx = 0;
  };
  std::vector<Candidate> candidates;
  candidates.reserve(edges.size());

  for (RS_Entity* e : edges) {
    const int sId = m_data->startId.count(e) ? m_data->startId.at(e) : -1;
    const bool attachAtStart = (sId == m_data->curId);

    RS_Vector doutV = LC_LoopUtils::travelDirection(*e, attachAtStart);
    if (!attachAtStart) {
      doutV = doutV * (-1.0);  // reversed traversal flips the leaving tangent
    }
    const double dout = std::atan2(doutV.y, doutV.x);
    double turn = dout - din;
    while (turn > M_PI) turn -= 2.0 * M_PI;
    while (turn <= -M_PI) turn += 2.0 * M_PI;

    double curv = LC_LoopUtils::signedCurvature(*e, attachAtStart);
    if (!attachAtStart) curv = -curv;

    const auto it = m_data->order.find(e);
    const std::size_t idx = (it != m_data->order.end())
                                 ? it->second
                                 : static_cast<std::size_t>(-1);
    candidates.push_back({e, turn, curv, idx});
  }

  auto better = [](const Candidate& a, const Candidate& b) {
      if (std::abs(a.turn - b.turn) > 1e-12) return a.turn > b.turn;
      if (a.curv != b.curv) return a.curv > b.curv;
      return a.idx < b.idx;
  };
  return std::min_element(candidates.begin(), candidates.end(),
                           [&](const Candidate& a, const Candidate& b) {
                               return better(a, b);
                           })->edge;
}

/**
 * @brief Returns unprocessed entities attached at the current welded vertex id.
 * O(degree) via the id-based adjacency.
 */
std::vector<RS_Entity*> LoopExtractor::getConnected() const {
  std::vector<RS_Entity*> ret;
  const int v = m_data->curId;
  if (v < 0 || v >= static_cast<int>(m_data->adjacency.size())) return ret;
  for (RS_Entity* e : m_data->adjacency[v]) {
    auto pit = m_data->processed.find(e);
    if (pit != m_data->processed.end() && !pit->second) {
      ret.push_back(e);
    }
  }
  return ret;
}

/**
 * @brief Finds and appends the next connected edge to the current loop.
 * Chooses by id-based attach: a candidate whose startId matches curId is
 * walked forward, else the clone is reversed. Updates curId to the other
 * welded id of the chosen edge.
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
    const int sId = m_data->startId.count(next) ? m_data->startId.at(next) : -1;
    const int eId = m_data->endId.count(next) ? m_data->endId.at(next) : -1;
    if (sId == m_data->curId) {
      // startId already matches: walk forward; new end is eId.
      m_data->curId = eId;
    } else {
      // endId matches: reverse the clone so its start is at curId; new end is sId.
      cloned->revertDirection();
      m_data->curId = sId;
    }
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
        // Interim classifier isClosed(): no ends-match band yet; the sorter
        // only skips closed-type atomics while gathering the vertex table.
        constexpr double kInterimEps = 0.0;
        for (auto& p : m_data->loops) {
            p->forcedCalculateBorders();
            bbMin.x = std::min(bbMin.x, p->getMin().x);
            bbMin.y = std::min(bbMin.y, p->getMin().y);
            bbMax.x = std::max(bbMax.x, p->getMax().x);
            bbMax.y = std::max(bbMax.y, p->getMax().y);
            std::vector<RS_Vector> vs;
            const unsigned n = p->count();
            if (n > 1 || (n == 1 && !isClosed(*p->entityAt(0), kInterimEps))) {
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
        // visits every candidate in ascending (|area|, extraction index). A
        // loop is degenerate when |A| <= eps * perimeter (the one tolerance
        // welded the vertices; the same cut filters near-zero signed area).
        m_data->order.clear();
        m_data->order.reserve(n);
        const double eps = m_data->eps;
        for (std::size_t i = 0; i < n; ++i) {
            if (m_data->absArea[i] <= eps * m_data->perimeter[i]) {
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
            if (n > 1 || (n == 1 && !isClosed(*m_loop->entityAt(0), m_eps))) {
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
     * @brief Computes net area: Outer area minus children's total areas (subtracts holes, adds islands).
     */
    double LC_Loops::getTotalArea() const {
        return std::accumulate(m_children.begin(), m_children.end(), m_loop->areaLineIntegral(), [](double area, const LC_Loops& loop) {
            return area - loop.getTotalArea(); // Recursive subtraction
        });
    }

    LC_AreaMoments LC_Loops::getTotalMoments(const RS_Vector& origin) const {
        // Mirror the recursive hole-subtraction of getTotalArea():
        //   net = outer - sum(children)
        return std::accumulate(
            m_children.begin(), m_children.end(),
            m_loop->momentLineIntegrals(origin),
            [&origin](LC_AreaMoments acc, const LC_Loops& child) {
                return acc - child.getTotalMoments(origin);
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
  return isClosed(*e, m_eps);
}

    /**
     * @brief The half-open [floor, ceil) tile index range on each axis.
     * Returns false when the pattern is empty on either axis; iFirst..kEnd
     * stay unmodified in that case.
     *
     * Indices stay in double/long long because |bbMin - anchor|/P can
     * exceed 2^31 for a tiny pattern tile far from the origin, and the
     * cast would be UB in int. Never name these j0/j1 (glibc Bessel).
     */
    bool LC_Loops::tileIndexRange(const RS_Pattern& pattern,
                                  const RS_Vector& latticeAnchor,
                                  double& iFirst, double& iEnd,
                                  double& kFirst, double& kEnd) const {
        const LC_Rect bBox = getBoundingBox();
        const LC_Rect pBox{pattern.getMin(), pattern.getMax()};
        const double pWidth = pBox.width();
        const double pHeight = pBox.height();
        if (pWidth < 1e-6 || pHeight < 1e-6) return false;
        iFirst = std::floor((bBox.minP().x - latticeAnchor.x) / pWidth);
        iEnd = std::ceil((bBox.maxP().x - latticeAnchor.x) / pWidth);
        kFirst = std::floor((bBox.minP().y - latticeAnchor.y) / pHeight);
        kEnd = std::ceil((bBox.maxP().y - latticeAnchor.y) / pHeight);
        return true;
    }

    namespace {
        // Collects (bbMin, bbMax, epsMax) over all roots' outer loops; returns
        // true when at least one root's loop has a finite bbox.
        bool unionLoopBox(const std::vector<LC_Loops>& roots,
                          RS_Vector& bbMin, RS_Vector& bbMax, double& epsMax) {
            bool have = false;
            bbMin = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
            bbMax = RS_Vector(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
            epsMax = 0.0;
            for (const LC_Loops& r : roots) {
                auto outer = r.getOuterLoop();
                if (!outer) continue;
                outer->calculateBorders();
                const RS_Vector mn = outer->getMin();
                const RS_Vector mx = outer->getMax();
                if (!mn.valid || !mx.valid) continue;
                bbMin.x = std::min(bbMin.x, mn.x);
                bbMin.y = std::min(bbMin.y, mn.y);
                bbMax.x = std::max(bbMax.x, mx.x);
                bbMax.y = std::max(bbMax.y, mx.y);
                epsMax = std::max(epsMax, r.tolerance());
                have = true;
            }
            return have;
        }

        // Walks every node of every root in pre-order and collects atomic
        // boundary entities. Pointers remain valid for the lifetime of the
        // tree.
        std::vector<RS_Entity*> allBoundaryEntities(
            const std::vector<LC_Loops>& roots) {
            std::vector<RS_Entity*> out;
            std::function<void(const LC_Loops&)> walk = [&](const LC_Loops& n) {
                auto outer = n.getOuterLoop();
                if (outer) {
                    for (RS_Entity* e : *outer) {
                        if (e != nullptr && e->isAtomic()) out.push_back(e);
                    }
                }
                for (const LC_Loops& c : n.getChildren()) walk(c);
            };
            for (const LC_Loops& r : roots) walk(r);
            return out;
        }

        // Half-open [floor, ceil) tile index range over the union bbox.
        bool unionTileIndexRange(const std::vector<LC_Loops>& roots,
                                 const RS_Pattern& pattern,
                                 const RS_Vector& latticeAnchor,
                                 double& iFirst, double& iEnd,
                                 double& kFirst, double& kEnd) {
            RS_Vector bbMin, bbMax;
            double epsMax = 0.0;
            if (!unionLoopBox(roots, bbMin, bbMax, epsMax)) return false;
            const LC_Rect pBox{pattern.getMin(), pattern.getMax()};
            const double pWidth = pBox.width();
            const double pHeight = pBox.height();
            if (pWidth < 1e-6 || pHeight < 1e-6) return false;
            iFirst = std::floor((bbMin.x - latticeAnchor.x) / pWidth);
            iEnd = std::ceil((bbMax.x - latticeAnchor.x) / pWidth);
            kFirst = std::floor((bbMin.y - latticeAnchor.y) / pHeight);
            kEnd = std::ceil((bbMax.y - latticeAnchor.y) / pHeight);
            return true;
        }
    }  // anonymous namespace

PointClass classifyAll(const RS_Vector& point,
                       const std::vector<LC_Loops>& roots) {
    // ON band of any root's edges wins over parity, matching per-node classify().
    for (const LC_Loops& r : roots) {
        const double eps = r.tolerance();
        if (eps > 0.0) {
            std::vector<const RS_EntityContainer*> conts;
            r.getAllLoops(conts);
            if (isNearAnyEdge(point, conts, eps)) {
                return PointClass::On;
            }
        }
    }
    // Global even-odd parity: sum crossingsRight over every node of every root.
    long long crossings = 0;
    std::function<void(const LC_Loops&)> walk = [&](const LC_Loops& n) {
        crossings += n.geometry().crossingsRight(point);
        for (const LC_Loops& c : n.getChildren()) walk(c);
    };
    for (const LC_Loops& r : roots) walk(r);
    return (crossings & 1) ? PointClass::Inside : PointClass::Outside;
}

std::vector<LC_TileSlot> createTiles(const std::vector<LC_Loops>& roots,
                                     const RS_Pattern& pattern,
                                     const RS_Vector& latticeAnchor) {
    double iFirst = 0.0, iEnd = 0.0, kFirst = 0.0, kEnd = 0.0;
    if (!unionTileIndexRange(roots, pattern, latticeAnchor,
                             iFirst, iEnd, kFirst, kEnd)) {
        return {};
    }
    RS_Vector bbMin, bbMax;
    double epsMax = 0.0;
    unionLoopBox(roots, bbMin, bbMax, epsMax);
    const LC_Rect bBox{bbMin, bbMax};
    const LC_Rect pBox{pattern.getMin(), pattern.getMax()};
    const double pWidth = pBox.width();
    const double pHeight = pBox.height();
    const RS_Vector pCenter = (pBox.lowerLeftCorner() + pBox.upperRightCorner()) / 2.0;
    const long long nIx = static_cast<long long>(iEnd - iFirst);
    const long long nIy = static_cast<long long>(kEnd - kFirst);
    const double slack = 2.0 * ((epsMax > 0.0) ? epsMax : RS_TOLERANCE);
    std::vector<LC_TileSlot> tiles;
    tiles.reserve(static_cast<std::size_t>(std::max<long long>(0, nIx * nIy)));
    for (long long ci = 0; ci < nIx; ++ci) {
        for (long long ck = 0; ck < nIy; ++ck) {
            const RS_Vector tile = latticeAnchor
                + RS_Vector{(iFirst + static_cast<double>(ci)) * pWidth,
                            (kFirst + static_cast<double>(ck)) * pHeight}
                - pBox.lowerLeftCorner();
            LC_Rect tileRect{pBox.lowerLeftCorner() + tile,
                             pBox.upperRightCorner() + tile};
            if (!tileRect.intersects(bBox)) {
                continue;
            }
            bool crossed = false;
            for (const LC_Loops& r : roots) {
                if (r.overlap(tileRect)) { crossed = true; break; }
            }
            const bool centreInside =
                classifyAll(tile + pCenter, roots) != PointClass::Outside;
            if (!(crossed || centreInside)) {
                continue;
            }
            const LC_Rect expandedRect{tileRect.minP() - RS_Vector{slack, slack},
                                       tileRect.maxP() + RS_Vector{slack, slack}};
            bool nearBoundary = crossed;
            if (!nearBoundary) {
                for (const LC_Loops& r : roots) {
                    if (r.overlap(expandedRect)) { nearBoundary = true; break; }
                }
            }
            tiles.push_back({tile, !nearBoundary});
        }
    }
    return tiles;
}

double gridWork(const std::vector<LC_Loops>& roots,
                const RS_Pattern& pattern,
                const RS_Vector& latticeAnchor) {
    double iFirst = 0.0, iEnd = 0.0, kFirst = 0.0, kEnd = 0.0;
    if (!unionTileIndexRange(roots, pattern, latticeAnchor,
                             iFirst, iEnd, kFirst, kEnd)) {
        return 0.0;
    }
    const double cells = (iEnd - iFirst) * (kEnd - kFirst);
    const double edges = static_cast<double>(allBoundaryEntities(roots).size());
    if (!std::isfinite(cells) || !std::isfinite(edges) || cells <= 0.0) {
        return 0.0;
    }
    return cells * edges;
}

double tileWork(const std::vector<LC_Loops>& roots,
                const RS_Pattern& pattern,
                const std::vector<LC_TileSlot>& tiles,
                double stopAbove) {
    const LC_Rect pBox{pattern.getMin(), pattern.getMax()};
    double E = 0.0;
    for (const RS_Entity* e : pattern) {
        if (e != nullptr && e->isAtomic()) E += 1.0;
    }
    if (E <= 0.0 || tiles.empty()) return 0.0;
    auto boundaries = allBoundaryEntities(roots);
    if (boundaries.empty()) return 0.0;
    double total = 0.0;
    for (const LC_TileSlot& slot : tiles) {
        const LC_Rect tileRect{pBox.lowerLeftCorner() + slot.offset,
                               pBox.upperRightCorner() + slot.offset};
        double B = 0.0;
        for (const RS_Entity* b : boundaries) {
            if (b == nullptr) continue;
            if (tileRect.overlaps(LC_Rect{b->getMin(), b->getMax()})) {
                B += 1.0;
            }
        }
        total += E * B;
        if (std::isfinite(stopAbove) && total > stopAbove) return total;
    }
    return total;
}

std::unique_ptr<RS_EntityContainer> trimPatternEntities(
    const std::vector<LC_Loops>& roots,
    const RS_Pattern& pattern,
    const std::vector<LC_TileSlot>& tiles,
    LC_HatchPieceBudget& budget,
    bool interiorFastPath) {
    auto trimmed = std::make_unique<RS_EntityContainer>();
    auto boundaries = allBoundaryEntities(roots);
    PointClassifier classifier = [&roots](const RS_Vector& q) {
        return classifyAll(q, roots);
    };
    double epsOn = 0.0;
    for (const LC_Loops& r : roots) {
        epsOn = std::max(epsOn, r.tolerance());
    }
    if (epsOn <= 0.0) epsOn = RS_TOLERANCE;
    // emitPiece() takes ownership of ent and charges the shared piece
    // budget. Returns false after the budget trips so the caller aborts
    // deterministically and discards partial output. Not named `emit` to
    // avoid Qt's moc macro.
    auto emitPiece = [&](std::unique_ptr<RS_Entity> ent) -> bool {
        if (!budget.take()) {
            return false;
        }
        ent->setVisible(true);
        trimmed->addEntity(ent.release());
        return true;
    };
    // One sort/sub helper per entity kind. Reuse the first root's helpers:
    // these methods on LC_Loops do not depend on `this`'s own state and the
    // free function stays stateless.
    auto sortAlong = [&](RS_Entity* e, std::vector<RS_Vector> inters) {
        return roots.empty() ? std::vector<RS_Vector>{}
                             : roots.front().sortPointsAlongEntity(e, std::move(inters));
    };
    auto subEntity = [&](RS_Entity* e, const RS_Vector& p1, const RS_Vector& p2) {
        return roots.empty() ? static_cast<RS_Entity*>(nullptr)
                             : roots.front().createSubEntity(e, p1, p2);
    };
    auto entityIsClosed = [&](const RS_Entity* e) {
        return !roots.empty() && roots.front().isEntityClosed(e);
    };

    for (const LC_TileSlot& slot : tiles) {
        for (RS_Entity* e : pattern) {
            if (!e->isAtomic()) {
                continue;
            }
            auto cloned = std::unique_ptr<RS_Entity>(e->clone());
            cloned->move(slot.offset);

            if (interiorFastPath && slot.interior) {
                if (!emitPiece(std::move(cloned))) return trimmed;
                continue;
            }

            std::vector<RS_Vector> all_inters;
            for (auto* b : boundaries) {
                RS_VectorSolutions sol = RS_Information::getIntersection(cloned.get(), b, true);
                for (RS_Vector v : sol) {
                    all_inters.push_back(v);
                }
            }
            std::sort(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
                return a.x < b.x || (RS_Math::equal(a.x, b.x) && a.y < b.y);
            });
            auto last = std::unique(all_inters.begin(), all_inters.end(), [](const RS_Vector& a, const RS_Vector& b){
                return a.distanceTo(b) < RS_TOLERANCE;
            });
            all_inters.erase(last, all_inters.end());
            auto sorted_inters = sortAlong(cloned.get(), all_inters);
            const bool closed = entityIsClosed(cloned.get());
            if (sorted_inters.empty()) {
                if (keepPiece(*cloned, classifier, epsOn)) {
                    if (!emitPiece(std::move(cloned))) return trimmed;
                }
                continue;
            }
            if (closed && sorted_inters.size() % 2 != 0) {
                RS_DEBUG->print("trimPatternEntities: Odd intersections for closed entity, skipping");
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
                auto sub = std::unique_ptr<RS_Entity>(subEntity(cloned.get(), p1, p2));
                if (sub && sub->getLength() > RS_TOLERANCE && keepPiece(*sub, classifier, epsOn)) {
                    if (!emitPiece(std::move(sub))) return trimmed;
                }
            }
            if (closed && sorted_inters.size() > 0) {
                RS_Vector pw1 = points.back();
                RS_Vector pw2 = points[0];
                if (pw1.distanceTo(pw2) >= RS_TOLERANCE) {
                    auto sub = std::unique_ptr<RS_Entity>(subEntity(cloned.get(), pw1, pw2));
                    if (sub && sub->getLength() > RS_TOLERANCE && keepPiece(*sub, classifier, epsOn)) {
                        if (!emitPiece(std::move(sub))) return trimmed;
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

QPainterPath buildLoopPath(RS_Painter* painter, const RS_EntityContainer& cont,
                           double eps) {
    PathBuilder builder{painter};
    QPainterPath& path = builder.getPath();
    if (cont.empty()) {
        return path;
    }

    // single closed entities may not have defined start/end points
    RS_Entity* first = cont.first();
    if (isClosed(*first, eps)) {
        builder.append(first);
        builder.closeSubpath();
        return path;
    }

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

namespace {

// A boundary candidate pulled from one node of the loop tree. The vertex
// table and edges are read-only views; the owning LC_Loops must outlive
// the countDuplicateLoops call.
struct DupCandidate {
    const RS_EntityContainer* loop = nullptr;
    const std::vector<RS_Vector>* vertices = nullptr;
    std::vector<RS_Entity*> edges;
    const RS_Entity* closed = nullptr;  // non-null only for a single-closed loop
    double area = 0.0;
    double perimeter = 0.0;
    double eps = 0.0;
};

void collectDupCandidates(const LC_Loops& node,
                          std::vector<DupCandidate>& out) {
    DupCandidate c;
    c.loop = node.getOuterLoop().get();
    c.vertices = &node.geometry().vertices();
    c.eps = node.tolerance();
    if (c.loop != nullptr) {
        for (RS_Entity* e : *c.loop) {
            if (e != nullptr && e->isAtomic()) {
                c.edges.push_back(e);
                c.perimeter += e->getLength();
            }
        }
        c.area = std::abs(c.loop->areaLineIntegral());
        if (c.edges.size() == 1 && isClosed(*c.edges.front(), c.eps)) {
            c.closed = c.edges.front();
        }
    }
    out.push_back(std::move(c));
    for (const auto& child : node.getChildren()) {
        collectDupCandidates(child, out);
    }
}

bool matchClosed(const RS_Entity& a, const RS_Entity& b, double eps) {
    if (a.rtti() != b.rtti()) return false;
    switch (a.rtti()) {
    case RS2::EntityCircle: {
        const auto& ca = static_cast<const RS_Circle&>(a);
        const auto& cb = static_cast<const RS_Circle&>(b);
        if ((ca.getCenter() - cb.getCenter()).magnitude() > eps) return false;
        return std::abs(ca.getRadius() - cb.getRadius()) <= eps;
    }
    case RS2::EntityEllipse: {
        const auto& ea = static_cast<const RS_Ellipse&>(a);
        const auto& eb = static_cast<const RS_Ellipse&>(b);
        if ((ea.getCenter() - eb.getCenter()).magnitude() > eps) return false;
        // Compare the major axis vectors and ratios directly; ellipses with
        // reversed major-axis directions still describe the same curve.
        const RS_Vector pA = ea.getMajorP();
        const RS_Vector pB = eb.getMajorP();
        const bool axisSame = (pA - pB).magnitude() <= eps
                              || (pA + pB).magnitude() <= eps;
        if (!axisSame) return false;
        return std::abs(ea.getRatio() - eb.getRatio()) <= eps;
    }
    case RS2::EntitySplinePoints: {
        const auto& sa = static_cast<const LC_SplinePoints&>(a);
        const auto& sb = static_cast<const LC_SplinePoints&>(b);
        const auto& pa = sa.getPoints();
        const auto& pb = sb.getPoints();
        if (pa.size() != pb.size() || pa.empty()) return false;
        // Closed spline control polygons can rotate: try every start.
        const std::size_t n = pa.size();
        for (std::size_t k = 0; k < n; ++k) {
            bool ok = true;
            for (std::size_t i = 0; i < n && ok; ++i) {
                if ((pa[i] - pb[(i + k) % n]).magnitude() > eps) ok = false;
            }
            if (ok) return true;
        }
        return false;
    }
    default:
        return false;
    }
}

bool duplicateMatch(const DupCandidate& a, const DupCandidate& b) {
    // Single closed curves on both sides.
    if (a.closed != nullptr && b.closed != nullptr) {
        const double eps = std::max(a.eps, b.eps);
        return matchClosed(*a.closed, *b.closed, eps);
    }
    if (a.closed != nullptr || b.closed != nullptr) return false;
    if (a.vertices == nullptr || b.vertices == nullptr) return false;
    const std::size_t n = a.vertices->size();
    if (n == 0 || n != b.vertices->size()) return false;
    if (a.edges.size() != n || b.edges.size() != n) return false;
    const double eps = std::max(a.eps, b.eps);
    for (std::size_t k = 0; k < n; ++k) {
        bool ok = true;
        for (std::size_t i = 0; i < n && ok; ++i) {
            const std::size_t j = (i + k) % n;
            if (((*a.vertices)[i] - (*b.vertices)[j]).magnitude() > eps) { ok = false; break; }
            if (a.edges[i]->rtti() != b.edges[j]->rtti()) { ok = false; break; }
            const RS_Vector midA = a.edges[i]->getMiddlePoint();
            const RS_Vector midB = b.edges[j]->getMiddlePoint();
            if (!midA.valid || !midB.valid) { ok = false; break; }
            if ((midA - midB).magnitude() > eps) { ok = false; break; }
        }
        if (ok) return true;
    }
    return false;
}

}  // namespace

int countDuplicateLoops(const std::vector<LC_Loops>& roots) {
    std::vector<DupCandidate> cands;
    for (const auto& r : roots) {
        collectDupCandidates(r, cands);
    }
    std::vector<char> isDup(cands.size(), 0);
    int count = 0;
    for (std::size_t i = 0; i < cands.size(); ++i) {
        if (isDup[i] != 0) continue;
        for (std::size_t j = i + 1; j < cands.size(); ++j) {
            if (isDup[j] != 0) continue;
            const double maxP = std::max(cands[i].perimeter, cands[j].perimeter);
            const double eps = std::max(cands[i].eps, cands[j].eps);
            if (std::abs(cands[i].area - cands[j].area) > eps * std::max(maxP, 1.0)) {
                continue;
            }
            if (duplicateMatch(cands[i], cands[j])) {
                isDup[j] = 1;
                ++count;
            }
        }
    }
    return count;
}

} // namespace LC_LoopUtils
