/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2023 librecad (www.librecad.org)
** Copyright (C) 2023 dxli (github.com/dxli)
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
#include <set>
#include <QPen>
#include <QPainterPath>

#include "lc_looputils.h"
#include "lc_rect.h"
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
#include "rs.h"
#include "lc_splinepoints.h"  // ADDED: For LC_SplinePoints support
#include "lc_parabola.h"      // ADDED: For LC_Parabola support

namespace {
// Definition for buildLC_Loops (moved to namespace level for accessibility)
/**
 * @brief Recursively builds an LC_Loops hierarchy from a container and all loops.
 * Clones atomic entities and traverses children via parent pointers.
 * @param cont The root container.
 * @param allLoops All loops for parent-child lookup.
 * @return The built LC_Loops tree.
 */
LC_LoopUtils::LC_Loops buildLoops(const RS_EntityContainer* cont, const std::vector<std::unique_ptr<RS_EntityContainer>>& allLoops) {
  auto loopCopy = std::make_shared<RS_EntityContainer>(nullptr, true);
  for (RS_Entity* e : *cont) {
    if (e && !e->isContainer()) {
      loopCopy->addEntity(e->clone());  // Clone atomics for independent ownership
    }
  }
  LC_LoopUtils::LC_Loops lc(loopCopy, true);
  for (const auto& p : allLoops) {
    if (p.get()->getParent() == cont) {
      lc.addChild(buildLoops(p.get(), allLoops));
    }
  }
  return lc;
}

/**
 * @brief Extends a line to cover the bounding box by computing intersections with box sides.
 * @param line The line to extend.
 * @param bbox The bounding rectangle.
 */
void extendLineToBBox(RS_Line& line, const LC_Rect& bbox) {
  const double maxSize = 1.5 * std::max(bbox.width(), bbox.height());
  const RS_Vector offset = line.getTangentDirection({}).normalized() * maxSize;

  line.setStartpoint(line.getStartpoint() - offset);
  line.setEndpoint(line.getEndpoint() + offset);
}

// Compare double with tolerance
struct DoublePredicate {
  bool operator()(double a, double b) const {
    // If the absolute difference is within epsilon, consider them equal
    if (std::abs(a - b) <= RS_TOLERANCE * 100.) {
      return false; // They are considered equal, so neither is "less than" the other in a strict sense
    }
    return a < b; // Otherwise, use standard less-than comparison
  }
};

// whether the entity is a single closed
// arc//ellipticArc with angular length 0 is considered to be a whole circle/ellipse
bool isClosed(const RS_Entity& en) {
  switch(en.rtti()) {
  case RS2::EntityCircle:
    return true;
  case RS2::EntityEllipse: {
    const RS_Ellipse& ellipse=static_cast<const RS_Ellipse&>(en);
    if (!ellipse.isArc())
      return true;
  }
    [[fallthrough]];
  case RS2::EntityArc:
    return en.getStartpoint() == en.getEndpoint();
  case RS2::EntitySpline: {
    const LC_SplinePoints& spline = static_cast<const LC_SplinePoints&>(en);
    return spline.isClosed();
  }
  case RS2::EntityParabola:
  default:
    return false;
  }
}

/**
 * @brief PathBuilder is a utility class to build a QPainterPath by appending RS_Entity objects.
 * All appendages are transformed to UI coordinates using the provided RS_Painter.
 * Handles common entity types: Line, Arc, Circle, Ellipse, Spline, Parabola.
 * Supports moveTo for starting new subpaths and ensures continuity where possible.
 */
class PathBuilder {
public:
  /**
   * @brief Constructor.
   * @param painter RS_Painter for UI coordinate transformations (required for UI mode).
   */
  explicit PathBuilder(RS_Painter* painter = nullptr);

  /**
   * @brief Appends the given entity to the path, transforming to UI coordinates.
   * Handles startpoint continuity; moves to start if needed.
   * @param entity The RS_Entity to append.
   */
  void append(RS_Entity* entity);

  /**
   * @brief Moves to the given position (in WCS; transformed to UI).
   * Starts a new subpath.
   * @param pos The position in WCS.
   */
  void moveTo(const RS_Vector& pos);
  void lineTo(const RS_Vector& pos);

  /**
   * @brief Closes the current subpath.
   */
  void closeSubpath();

  /**
   * @brief Gets the built path.
   * @return Reference to the QPainterPath.
   */
  QPainterPath& getPath() { return m_path; }
  const QPainterPath& getPath() const { return m_path; }

  /**
   * @brief Clears the path.
   */
  void clear();
  QPointF toGuiPoint(const RS_Vector& vp) const
  {
    RS_Vector guiVp = m_painter->toGui(vp);
    return {guiVp.x, guiVp.y};
  }

private:
  /**
   * @brief Appends a line in UI coordinates.
   * @param line The RS_Line.
   */
  void appendLine(RS_Line* line);

  /**
   * @brief Appends an arc using cubic Bezier approximation in UI coordinates.
   * Follows entity direction (reversed or not).
   * @param arc The RS_Arc.
   */
  void appendArc(RS_Arc* arc);

  /**
   * @brief Appends a circle as a full arc in UI coordinates.
   * @param circle The RS_Circle.
   */
  void appendCircle(RS_Circle* circle);

  /**
   * @brief Appends an ellipse arc in UI coordinates using Bezier approximation.
   * @param ellipse The RS_Ellipse.
   */
  void appendEllipse(RS_Ellipse* ellipse);

  /**
   * @brief Appends a spline (LC_SplinePoints) by appending quadratic Bezier segments in UI coordinates.
   * Assumes current position is at startpoint; uses quadTo for each quadratic segment.
   * @param spline The LC_SplinePoints.
   */
  void appendSplinePoints(LC_SplinePoints* spline);

  /**
   * @brief Appends a parabola (LC_Parabola) by sampling stroke points in UI coordinates.
   * Assumes current position is at startpoint; uses lineTo for polyline approximation.
   * @param parabola The LC_Parabola.
   */
  void appendParabola(LC_Parabola* parabola);

  RS_Painter* m_painter = nullptr;
  QPainterPath m_path;
  RS_Vector m_lastPoint{};  ///< Last point in WCS for continuity check.
  bool m_hasLastPoint = false;  ///< Flag indicating if m_lastPoint is valid.
};

PathBuilder::PathBuilder(RS_Painter* painter)
    : m_painter(painter){
  assert(m_painter != nullptr);
  m_path.setFillRule(Qt::OddEvenFill);
  m_hasLastPoint = false;
}

void PathBuilder::append(RS_Entity* entity) {
  if (!entity || entity->isUndone()) return;

  RS_Vector startp = entity->getStartpoint();
  const double tol = 1e-6;
  if (!m_hasLastPoint || m_lastPoint.distanceTo(startp) > tol) {
    moveTo(startp);
  }

  RS2::EntityType type = entity->rtti();

  switch (type) {
  case RS2::EntityLine:
    appendLine(static_cast<RS_Line*>(entity));
    break;
  case RS2::EntityArc:
    appendArc(static_cast<RS_Arc*>(entity));
    break;
  case RS2::EntityCircle:
    appendCircle(static_cast<RS_Circle*>(entity));
    break;
  case RS2::EntityEllipse:
    appendEllipse(static_cast<RS_Ellipse*>(entity));
    break;
  case RS2::EntitySpline:
    appendSplinePoints(static_cast<LC_SplinePoints*>(entity));
    break;
  case RS2::EntityParabola:
    appendParabola(static_cast<LC_Parabola*>(entity));
    break;
  default:
    RS_DEBUG->print(RS_Debug::D_WARNING, "PathBuilder::append: Unsupported entity type %d", static_cast<int>(type));
    break;
  }

  m_lastPoint = entity->getEndpoint();
  m_hasLastPoint = true;
}

void PathBuilder::moveTo(const RS_Vector& pos) {
  QPointF uiPos = toGuiPoint(pos);
  m_path.moveTo(uiPos);
  m_lastPoint = pos;
  m_hasLastPoint = true;
}

void PathBuilder::lineTo(const RS_Vector& pos) {
  QPointF uiPos = toGuiPoint(pos);
  m_path.lineTo(uiPos);
  m_lastPoint = pos;
  m_hasLastPoint = true;
}

void PathBuilder::closeSubpath() {
  m_path.closeSubpath();
  // Do not reset m_hasLastPoint; keep for potential next append
}

void PathBuilder::clear() {
  m_path = QPainterPath();
  m_lastPoint = RS_Vector(0., 0.);
  m_hasLastPoint = false;
}

void PathBuilder::appendLine(RS_Line* line) {
  if (!line)
    return;

  QPointF uiEnd = toGuiPoint(line->getEndpoint());

  m_path.lineTo(uiEnd);
}

void PathBuilder::appendArc(RS_Arc* arc) {
  // TODO: need pixel level precision: issue #2035
  if (!arc || !m_painter)
    return;

  double startAngle = arc->getAngle1();
  double endAngle = arc->getAngle2();
  if (arc->isReversed())
    endAngle = startAngle - RS_Math::correctAngle(startAngle - endAngle);
  else
    endAngle = startAngle + RS_Math::correctAngle(endAngle - startAngle);

  double startDeg = RS_Math::rad2deg(startAngle);
  double sweepDeg = RS_Math::rad2deg(endAngle - startAngle);

  QPointF center = toGuiPoint(arc->getCenter());
  double radiusX = m_painter->toGuiDX(arc->getRadius());
  double radiusY = m_painter->toGuiDY(arc->getRadius());
  QPointF halfSize{radiusX, radiusY};
  QRectF arcRect{center - halfSize, center + halfSize};

  m_path.arcTo(arcRect, startDeg, sweepDeg);
}

void PathBuilder::appendEllipse(RS_Ellipse* ellipse) {
  if (!ellipse || !m_painter) return;

  // TODO: need pixel level precision: issue #2035
  m_painter->drawEllipseBySplinePointsUI(*ellipse, m_path);
}

void PathBuilder::appendCircle(RS_Circle* circle) {
  if (!circle || !m_painter) return;

  // TODO: need pixel level precision: issue #2035
  QPointF center = toGuiPoint(circle->getCenter());
  double radiusX = m_painter->toGuiDX(circle->getRadius());
  double radiusY = m_painter->toGuiDY(circle->getRadius());
  QPointF halfSize{radiusX, radiusY};
  QRectF circleRect{center - halfSize, center + halfSize};

  m_path.addEllipse(circleRect);
}

void PathBuilder::appendSplinePoints(LC_SplinePoints* spline) {
  if (!spline || !m_painter) return;

  const auto& points = spline->getPoints();
  if (points.empty()) return;

  size_t n_points = points.size();
  size_t num_segs = spline->isClosed() ? n_points : n_points - 1;
  if (num_segs == 0) {
    // Degenerate case: connect to endpoint
    lineTo(spline->getEndpoint());
    return;
  }

         // Assume current path position is at the startpoint of the first segment
  for (size_t i = 0; i < num_segs; ++i) {
    RS_Vector start, ctrl, end;
    if (spline->GetQuadPoints(int(i), &start, &ctrl, &end) != 0) {
      m_path.moveTo(toGuiPoint(start));
      m_path.quadTo(toGuiPoint(ctrl), toGuiPoint(end));
    } else {
      // Fallback: line to end if quad points unavailable
      lineTo(end);
    }
  }
}

void PathBuilder::appendParabola(LC_Parabola* parabola) {
  if (!parabola) return;

         // Inherit from LC_SplinePoints for quadratic Bezier handling
  appendSplinePoints(parabola);
}

}

namespace LC_LoopUtils {

// Private implementation for LoopExtractor
struct LoopExtractor::LoopData {
  std::vector<RS_Entity*> unprocessed;  ///< Remaining edges to process
  std::map<RS_Entity*, bool> processed; ///< Flag for processed status
  RS_Entity* current = nullptr;         ///< Current entity in loop
  RS_Vector endPoint;                   ///< Current endpoint
  RS_Vector targetPoint;                ///< Target start point for closure
  bool reversed = false;                ///< Direction reversal flag
};

/**
 * @brief Constructs LoopExtractor and initializes unprocessed edges.
 * Filters to atomic entities with length > ENDPOINT_TOLERANCE.
 */
LoopExtractor::LoopExtractor(const RS_EntityContainer& edges) :
                                                                m_data(std::make_unique<LoopData>())
{
  for (RS_Entity* e : edges) {
    if (e->isAtomic() && e->getLength() > ENDPOINT_TOLERANCE) {  // Skip degenerate zero-length edges
      m_data->unprocessed.push_back(e);
      m_data->processed[e] = false;
    }
  }
}

LoopExtractor::~LoopExtractor() = default;

/**
 * @brief Extracts closed loops iteratively until no unprocessed edges remain.
 * Builds each loop by chaining connected edges, clones and orients for positive area.
 * @return Vector of unique_ptr to valid loop containers.
 */
std::vector<std::unique_ptr<RS_EntityContainer>> LoopExtractor::extract() {
  std::vector<std::unique_ptr<RS_EntityContainer>> results;
  while (!m_data->unprocessed.empty()) {
    m_loop = std::make_unique<RS_EntityContainer>();
    RS_Entity* first = findFirst();
    if (first) {
      RS_Entity* cloned_first = first->clone();
      m_loop->addEntity(cloned_first);
      m_data->processed[first] = true;
      m_data->current = cloned_first;
      RS_Vector start = cloned_first->getStartpoint();
      RS_Vector end = cloned_first->getEndpoint();
      m_data->targetPoint = start;
      m_data->endPoint = end;
      m_data->unprocessed.erase(std::remove(m_data->unprocessed.begin(), m_data->unprocessed.end(), first), m_data->unprocessed.end());
      while (m_data->endPoint.distanceTo(m_data->targetPoint) > ENDPOINT_TOLERANCE) {  // Continue until closure within tolerance
        if (findNext()) {
          if (m_data->endPoint.distanceTo(m_data->targetPoint) <= ENDPOINT_TOLERANCE) break;
        } else {
          break;
        }
      }
      if (validate()) {
        double area = m_loop->areaLineIntegral();
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
        results.push_back(std::move(m_loop));
      } else {
        RS_DEBUG->print("LoopExtractor: Invalid loop discarded");  // Log invalid loops
      }
    }
  }
  return results;
}

/**
 * @brief Validates the current loop for closure.
 * @return True if valid.
 */
bool LoopExtractor::validate() const {
  if (m_loop->count() == 0) return false;
  if (m_loop->count() == 1) {
    RS_Entity* e = m_loop->entityAt(0);
    RS2::EntityType type = e->rtti();
    if (type == RS2::EntityCircle) return true;
    if (type == RS2::EntityEllipse) {
      RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
      return ell->getAngleLength() >= 2 * M_PI - RS_TOLERANCE;
    }
    if (type == RS2::EntitySpline) {  // SUPPORT: Closed splines
      LC_SplinePoints* spl = static_cast<LC_SplinePoints*>(e);
      return spl->isClosed();
    }
    if (type == RS2::EntityParabola) {  // SUPPORT: Valid if primitives computed successfully
      LC_Parabola* para = static_cast<LC_Parabola*>(e);
      return para->getData().valid;
    }
    return e->getStartpoint().distanceTo(e->getEndpoint()) <= ENDPOINT_TOLERANCE;
  }
  RS_Entity* first = m_loop->entityAt(0);
  RS_Entity* last = m_loop->last();
  return first->getStartpoint().distanceTo(last->getEndpoint()) <= ENDPOINT_TOLERANCE;
}

/**
 * @brief Finds the first edge to start a loop.
 * @return Starting entity or nullptr.
 */
RS_Entity* LoopExtractor::findFirst() const {
  if (m_data->unprocessed.empty()) return nullptr;
  RS_Entity* firstEdge = m_data->unprocessed[0];
  RS_Vector mid = firstEdge->getMiddlePoint();
  RS_Vector lineStart(mid.x - 1000, mid.y);
  RS_Line testLine(lineStart, mid);
  double minDist = RS_MAXDOUBLE;
  RS_Entity* closest = nullptr;
  for (RS_Entity* e : m_data->unprocessed) {
    RS_VectorSolutions sol = RS_Information::getIntersection(&testLine, e, true);
    if (sol.hasValid()) {
      for (RS_Vector v : sol) {
        double d = v.distanceTo(lineStart);
        if (d < minDist) {
          minDist = d;
          closest = e;
        }
      }
    }
  }
  return closest ? closest : firstEdge;
}

/**
 * @brief Finds and adds the next connected edge to the loop.
 * @return True if found.
 */
bool LoopExtractor::findNext() const {
  std::vector<RS_Entity*> connected = getConnected();
  if (connected.empty()) return false;
  RS_Entity* next = nullptr;
  if (connected.size() == 1) {
    next = connected[0];
  } else {
    next = findOutermost(connected);
  }
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

/**
 * @brief Gets entities connected to the current endpoint.
 * @return Vector of connected entities.
 */
std::vector<RS_Entity*> LoopExtractor::getConnected() const {
  std::vector<RS_Entity*> ret;
  for (RS_Entity* e : m_data->unprocessed) {
    if (e->getStartpoint().distanceTo(m_data->endPoint) <= ENDPOINT_TOLERANCE || e->getEndpoint().distanceTo(m_data->endPoint) <= ENDPOINT_TOLERANCE) {
      ret.push_back(e);
    }
  }
  return ret;
}

/**
 * @brief Selects the outermost (preferred direction) from connected edges.
 * @param edges Connected edges.
 * @return Selected entity.
 */
RS_Entity* LoopExtractor::findOutermost(std::vector<RS_Entity*> edges) const {
  double radius = 1e-6;
  RS_Circle circle(nullptr, RS_CircleData(m_data->endPoint, radius));
  double currentAngle = m_data->current->getDirection2();  // Incoming tangent at junction
  std::vector<std::pair<double, RS_Entity*>> angleDiffs;
  for (RS_Entity* e : edges) {
    // Compute outgoing tangent, accounting for potential reversal
    bool needsReversal = (e->getStartpoint().distanceTo(m_data->endPoint) > ENDPOINT_TOLERANCE);
    double outgoingAngle;
    if (!needsReversal) {
      outgoingAngle = e->getDirection1();  // Tangent at start
    } else {
      outgoingAngle = RS_Math::correctAngle(e->getDirection2() + M_PI);  // Flip by pi for reversal
    }

    RS_VectorSolutions sol = RS_Information::getIntersection(&circle, e, true);
    if (!sol.hasValid()) continue;

           // Select the intersection closest to the outgoing direction
    RS_Vector selected_v;
    double min_adiff = RS_MAXDOUBLE;
    for (size_t k = 0; k < sol.getNumber(); ++k) {
      RS_Vector v = sol.get(k);
      double dist = v.distanceTo(m_data->endPoint);
      if (std::abs(dist - radius) > RS_TOLERANCE) continue;  // Filter valid radius intersections

      double a = (v - m_data->endPoint).angle();
      double adiff = RS_Math::correctAngle(a - outgoingAngle);
      if (adiff > M_PI) adiff = 2 * M_PI - adiff;  // Minimal unsigned difference
      if (adiff < min_adiff) {
        min_adiff = adiff;
        selected_v = v;
      }
    }

    if (min_adiff > 1e-4) continue;  // Skip if no close match (numerical or curvature issues)

    double angle = (selected_v - m_data->endPoint).angle();
    double diff = RS_Math::correctAngle(angle - currentAngle);
    if (diff > M_PI) diff -= 2 * M_PI;  // Signed difference in (-pi, pi]

    angleDiffs.push_back({diff, e});
  }

  if (angleDiffs.empty()) return nullptr;
  // Sort by descending signed diff to prefer left turns (largest positive diff) for CCW outer boundary
  std::sort(angleDiffs.begin(), angleDiffs.end(), [](const auto& a, const auto& b) {
    return a.first > b.first;
  });
  return angleDiffs[0].second;
}

// Private implementation for LoopSorter
struct LoopSorter::Data {
  std::vector<std::unique_ptr<RS_EntityContainer>> loops;  ///< Input loops
  std::shared_ptr<std::vector<LC_Loops>> results;          ///< Output hierarchy
};

/**
 * @brief Predicate for sorting loops: ascending absolute area (small to large for parent assignment), tie-break by bounding box diagonal.
 * Ensures robust ordering for containment detection.
 */
struct LoopSorter::AreaPredicate {
  bool operator() (const RS_EntityContainer* a, const RS_EntityContainer* b) const {
    double areaA = std::abs(a->areaLineIntegral());
    double areaB = std::abs(b->areaLineIntegral());
    if (std::abs(areaA - areaB) < RS_TOLERANCE) {
      double diagA = (a->getMax() - a->getMin()).magnitude();
      double diagB = (b->getMax() - b->getMin()).magnitude();
      return diagA < diagB;
    }
    return areaA < areaB;  // Ascending abs area (small to large for parent assignment)
  }
};

/**
 * @brief Constructs LoopSorter, filters degenerates, sorts, and builds hierarchy.
 */
LoopSorter::LoopSorter(std::vector<std::unique_ptr<RS_EntityContainer>> loops) : m_data(new Data) {
  m_data->loops = std::move(loops);
  sortAndBuild();
}

LoopSorter::~LoopSorter() = default;

/**
 * @brief Main sorting and hierarchy building: Filters zero-area, sorts ascending area, assigns parents small-to-large.
 * Builds forest of roots and converts to LC_Loops.
 */
void LoopSorter::sortAndBuild() {
  std::multimap<double, RS_EntityContainer*> orderedLoops;
  for (auto& p : m_data->loops) {
    p->setParent(nullptr);  // Reset parents
    p->forcedCalculateBorders();
    double area = std::abs(p->areaLineIntegral());
    if (area < RS_TOLERANCE) {
      RS_DEBUG->print("LoopSorter: Skipping degenerate zero-area loop");
      continue;
    }
    orderedLoops.emplace(area, p.get());
  }
  std::vector<RS_EntityContainer*> forest;
  while (!orderedLoops.empty()) {
    RS_EntityContainer* child = orderedLoops.begin()->second;
    findParent(child, orderedLoops);  // Assign immediate parent
    if (child->getParent() == nullptr) {
      forest.push_back(child);  // Root if no parent
    }
    orderedLoops.erase(orderedLoops.begin());
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
  for (RS_EntityContainer* container: forest) {
    loops->push_back(buildLoops(container, m_data->loops));  // Build recursive hierarchy
  }
  return loops;
}

void LoopSorter::init() {
  // Placeholder for future initialization
}

/**
 * @brief Assigns the smallest enclosing parent using bbox inclusion and point-in-contour test.
 * Processes small-to-large to ensure immediate (direct) parent.
 */
void LoopSorter::findParent(RS_EntityContainer* loop, const std::multimap<double, RS_EntityContainer*>& sorted) {
  if (sorted.size() == 1)
    return;
  LC_Rect childBox{loop->getMin(), loop->getMax()};
  double childArea = std::abs(loop->areaLineIntegral());
  RS_Vector testPoint = (loop->getMin() + loop->getMax()) / 2.0;  // Use bbox center for containment test
  for (auto it = sorted.begin(); ++it != sorted.end();) {  // Iterate small to large
    auto* potentialParent = it->second;
    if (potentialParent == loop)
      continue;
    double parentArea = it->first;

           // Skip smaller or equal
    if (parentArea <= childArea + RS_TOLERANCE)
      continue;
    LC_Rect parentBox{potentialParent->getMin(), potentialParent->getMax()};

    if (childBox.numCornersInside(parentBox) != 4)
      continue;  // Quick bbox containment
    bool onContour = false;
    if (RS_Information::isPointInsideContour(testPoint, potentialParent, &onContour)) {
      loop->setParent(potentialParent);  // Track hierarchy via parent pointer only
      RS_DEBUG->print("LoopSorter: Assigned parent for loop with area %f", childArea);
      return;
    }
  }
  RS_DEBUG->print("LoopSorter: No parent found for loop with area %f", childArea);  // Log orphan
}

// Private implementation for LoopOptimizer
struct LoopOptimizer::Data {
  std::shared_ptr<std::vector<LC_Loops>> results;  ///< Processed results
};

/**
 * @brief Constructs LoopOptimizer and processes the input contour.
 */
LoopOptimizer::LoopOptimizer(const RS_EntityContainer& contour) : m_data(new Data) {
  AddContainer(contour);
}

LoopOptimizer::~LoopOptimizer() = default;

/**
 * @brief Accessor for results.
 */
std::shared_ptr<std::vector<LC_Loops>> LoopOptimizer::GetResults() const {
  return m_data->results;
}

/**
 * @brief Processes a contour: Extract loops, sort, and build hierarchy.
 */
void LoopOptimizer::AddContainer(const RS_EntityContainer& contour) {
  LoopExtractor extractor(contour);
  auto loops = extractor.extract();
  LoopSorter sorter(std::move(loops));
  m_data->results = sorter.getResults();
}

// LC_Loops implementation

LC_Loops::LC_Loops(std::shared_ptr<RS_EntityContainer> loop, bool ownsEntities) : m_loop(loop) {
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
void LC_Loops::addEntity(RS_Entity* entity) {
  m_loop->addEntity(entity);
}

/**
 * @brief Non-recursive check for point inside outer loop using winding rule.
 */
bool LC_Loops::isInsideOuter(const RS_Vector& point) const {
  bool onContour = false;
  return RS_Information::isPointInsideContour(point, m_loop.get(), &onContour);
}

/**
 * @brief Recursive inside check using odd-even parity on depth.
 */
bool LC_Loops::isInside(const RS_Vector& point) const {
  return getContainingDepth(point) % 2 == 1;
}

/**
 * @brief Computes recursive depth: 1 for outer + sum of children.
 * Uses outer-only check to avoid cycles.
 */
int LC_Loops::getContainingDepth(const RS_Vector& point) const {
  int depth = 0;
  if (isInsideOuter(point)) {  // Check outer only
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
    QPainterPath childPath = child.getPainterPath(painter, level + 1);
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
    return area - loop.getTotalArea();  // Recursive subtraction
  });
}

/**
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
void LC_Loops::addEllipticArc(QPainterPath& path, const RS_Vector& center, double major, double minor, double rot, double a1, double a2) const {
  double aspect = std::max(major, minor) / std::min(major, minor);
  int extra_segments = static_cast<int>(std::ceil(aspect - 1.0));  // More segments for high aspect
  double sweep = a2 - a1;
  int n = static_cast<int>(std::ceil(std::abs(sweep) * 24. / M_PI )) + extra_segments;  // Quadrants + extra
  if (n <= 0) return;  // Degenerate case
  double dt = sweep / n;
  double lambda = (4.0 / 3.0) * std::tan(dt / 4.0);  // Bezier control factor (constant)
  double current_t = a1;

         // Initial endpoint and tangent for first segment
  RS_Vector p0 = e_point(center, major, minor, rot, current_t);
  RS_Vector prime0 = e_prime(major, minor, rot, current_t) * lambda;

  if ((path.currentPosition() - QPointF{p0.x, p0.y}).manhattanLength() >= RS_TOLERANCE * 100.) {
    path.moveTo(p0.x, p0.y);
  } else {
    path.lineTo(p0.x, p0.y);
  }

  for (int i = 0; i < n; ++i) {
    double t1 = current_t + dt;
    RS_Vector p3 = e_point(center, major, minor, rot, t1);
    RS_Vector prime1 = e_prime(major, minor, rot, t1) * lambda;
    RS_Vector p1 = p0 + prime0;
    RS_Vector p2 = p3 - prime1;
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
  if (cont.empty())
    return path;

         // single closed entities may not have defined start/end points
  RS_Entity* first = cont.first();
  if (isClosed(*first)) {
    builder.append(first);
    builder.closeSubpath();
    return path;
  }

  builder.moveTo(cont.first()->getStartpoint());
  for (RS_Entity* e : cont) {
    if (e->isAtomic()) {
      RS_Vector start = e->getStartpoint();
      // avoid small gaps due to rounding errors
      if ((path.currentPosition() - builder.toGuiPoint({start.x, start.y})).manhattanLength() >= 3.) {
        LC_ERR<<__func__<<"(): added line at "<<start.x<<", "<<start.y;
      }
      builder.append(e);
    }
  }
  builder.closeSubpath();
  return path;
}

/**
 * @brief Recursively collects all descendant loop containers.
 */
void LC_Loops::getAllLoops(std::vector<const RS_EntityContainer*>& loops) const {
  if (m_loop) loops.push_back(m_loop.get());
  for (const auto& child : m_children) {
    child.getAllLoops(loops);
  }
}

/**
 * @brief Gets the bounding box from the outer loop.
 */
LC_Rect LC_Loops::getBoundingBox() const {
  if (m_loop == nullptr)
    return {};
  m_loop->calculateBorders();
  return {m_loop->getMin(), m_loop->getMax()};
}

/**
 * @brief Alias for isInside (odd-even rule).
 */
bool LC_Loops::isPointInside(const RS_Vector& p) const {
  return getContainingDepth(p) % 2 == 1;
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
      if (e->isAtomic()) bounds.push_back(e);
    }
  }
  return bounds;
}

/**
 * @brief Checks if entity represents a closed shape (e.g., full circle, zero-length line).
 */
bool LC_Loops::isEntityClosed(const RS_Entity* e) const {
  RS2::EntityType type = e->rtti();
  if (type == RS2::EntityCircle) return true;
  if (type == RS2::EntityEllipse) {
    const RS_Ellipse* ell = static_cast<const RS_Ellipse*>(e);
    return std::abs(ell->getAngleLength() - 2 * M_PI) < RS_TOLERANCE_ANGLE;
  }
  if (type == RS2::EntityArc) {
    const RS_Arc* arc = static_cast<const RS_Arc*>(e);
    return std::abs(arc->getAngleLength() - 2 * M_PI) < RS_TOLERANCE_ANGLE;
  }
  if (type == RS2::EntitySpline) {  // SUPPORT: Closed splines
    const LC_SplinePoints* spl = static_cast<const LC_SplinePoints*>(e);
    return spl->isClosed();
  }
  if (type == RS2::EntityParabola) {  // SUPPORT: Parabolas are finite open arcs
    return false;
  }
  if (type == RS2::EntityLine) return e->getStartpoint() == e->getEndpoint();
  return false;
}

/**
 * @brief Generates tile offsets for pattern repetition within the bounding box.
 * Filters tiles that intersect or contain points inside the loop.
 */
std::vector<RS_Vector> LC_Loops::createTiles(const RS_Pattern& pattern) const {
  LC_Rect bBox = getBoundingBox();
  LC_Rect pBox{pattern.getMin(), pattern.getMax()};
  const double pWidth = pBox.width();
  const double pHeight = pBox.height();
  if (pWidth < 1e-6 || pHeight < 1e-6)  // Skip degenerate patterns
    return {};
  RS_Vector offsetBase = bBox.lowerLeftCorner() - pBox.lowerLeftCorner();
  std::vector<RS_Vector> tiles;
  // Use ceil for full coverage
  int nx = static_cast<int>(std::ceil(bBox.width() / pWidth)) + 1;
  int ny = static_cast<int>(std::ceil(bBox.height() / pHeight)) + 1;
  // Use pattern center for inside check
  RS_Vector pCenter = (pBox.lowerLeftCorner() + pBox.upperRightCorner()) / 2.0;
  for (int i = 0; i < nx; ++i) {
    for (int j = 0; j < ny; ++j) {
      RS_Vector tile = offsetBase + RS_Vector{pWidth * i, pHeight * j};
      LC_Rect tileRect{pBox.lowerLeftCorner() + tile, pBox.upperRightCorner() + tile};
      // Quick bbox intersection
      if (!tileRect.intersects(bBox))
        continue;
      // Include if overlaps or center inside
      if (overlap(tileRect) || isPointInside(tile + pCenter)) {
        tiles.push_back(tile);
      }
    }
  }
  return tiles;
}

/**
 * @brief Trims pattern entities to loop boundaries: Intersects, sorts params, creates subs inside loop.
 * Handles closed/open entities; skips odd intersections for closed.
 * For RS_Line: extends to bbox, dedups tiles by perpendicular intercept.
 */
std::unique_ptr<RS_EntityContainer> LC_Loops::trimPatternEntities(const RS_Pattern& pattern) const {
  std::unique_ptr<RS_EntityContainer> trimmed = std::make_unique<RS_EntityContainer>();
  std::vector<RS_Vector> tiles = createTiles(pattern);
  auto boundaries = getAllBoundaries();
  std::map<const RS_Entity*, std::set<double, DoublePredicate>> savedIntercepts;
  LC_Rect bBox = getBoundingBox();
  for (const RS_Vector& tile : tiles) {
    for (RS_Entity* e : pattern) {
      if (!e->isAtomic()) continue;
      auto cloned = std::unique_ptr<RS_Entity>(e->clone());
      cloned->move(tile);

             // For RS_Line, extend the line to cover the whole contour, if the pattern line is seen for the first time
             // If the extended line is coincident with a previous one, skip it
      if (e->rtti() == RS2::EntityLine) {
        RS_Line* cline = static_cast<RS_Line*>(cloned.get());
        RS_Vector normal = cline->getNormalVector();
        double intr = normal.dotP(cline->getStartpoint());
        constexpr double TOL = 1e-6;
        double key = std::round(intr / TOL) * TOL;
        std::set<double, DoublePredicate>& sset = savedIntercepts[e];
        if (sset.find(key) != sset.end())
          continue;
        sset.insert(key);
        // Extend to bbox
        extendLineToBBox(*cline, bBox);
      }
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
        if (isPointInside(cloned->getMiddlePoint())) {
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
        if (p1.distanceTo(p2) < RS_TOLERANCE)
          continue;
        auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), p1, p2));
        // Filter short subs and add if inside
        if (sub && sub->getLength() > RS_TOLERANCE && isPointInside(sub->getMiddlePoint())) {
          sub->setVisible(true);
          trimmed->addEntity(sub.release());
        }
      }
      if (closed && sorted_inters.size() > 0) {
        RS_Vector pw1 = points.back();
        RS_Vector pw2 = points[0];
        if (pw1.distanceTo(pw2) >= RS_TOLERANCE) {
          auto sub = std::unique_ptr<RS_Entity>(createSubEntity(cloned.get(), pw1, pw2));
          if (sub && sub->getLength() > RS_TOLERANCE && isPointInside(sub->getMiddlePoint())) {
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
    RS_Arc* arc = static_cast<RS_Arc*>(e);
    RS_Vector center = arc->getCenter();
    double ang1 = (p1 - center).angle();
    double ang2 = (p2 - center).angle();
    return new RS_Arc(nullptr, RS_ArcData(center, arc->getRadius(), ang1, ang2, arc->isReversed()));
  } else if (type == RS2::EntityCircle) {
    RS_Circle* circle = static_cast<RS_Circle*>(e);
    RS_Vector center = circle->getCenter();
    double ang1 = (p1 - center).angle();
    double ang2 = (p2 - center).angle();
    return new RS_Arc(nullptr, RS_ArcData(center, circle->getRadius(), ang1, ang2, false));
  } else if (type == RS2::EntityEllipse) {
    RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
    RS_Vector center = ell->getCenter();
    double rot = ell->getAngle();
    RS_Vector lp1 = (p1 - center).rotate(-rot);
    double lang1 = std::atan2(lp1.y / ell->getMinorRadius(), lp1.x / ell->getMajorRadius());
    RS_Vector lp2 = (p2 - center).rotate(-rot);
    double lang2 = std::atan2(lp2.y / ell->getMinorRadius(), lp2.x / ell->getMajorRadius());
    return new RS_Ellipse(nullptr, {center, ell->getMajorP(), ell->getRatio(), lang1, lang2, ell->isReversed()});
  } else if (type == RS2::EntitySpline) {  // IMPROVED: Use cut() for precise trimming
    LC_SplinePoints* spl = static_cast<LC_SplinePoints*>(e);
    double total_len = spl->getLength();
    if (total_len < RS_TOLERANCE) return nullptr;
    double t1 = spl->getDistanceToPoint(p1) / total_len;
    double t2 = spl->getDistanceToPoint(p2) / total_len;
    if (t1 > t2) std::swap(t1, t2);  // Ensure order
    LC_SplinePoints* seg1 = spl->cut(p1);  // Trims original to start-p1
    if (seg1) {
      LC_SplinePoints* sub = seg1->cut(p2);  // Further trim p1-p2
      if (sub && sub->getLength() > RS_TOLERANCE) return sub;
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
    if (i1 > i2) std::swap(i1, i2);
    LC_SplinePointsData sub_data;
    sub_data.splinePoints.assign(points.begin() + i1, points.begin() + i2 + 1);
    return new LC_SplinePoints(nullptr, sub_data);
  } else if (type == RS2::EntityParabola) {  // IMPROVED: Exact tangents via FromEndPointsTangents
    LC_Parabola* para = static_cast<LC_Parabola*>(e);
    RS_Vector tan1 = para->getTangentDirection(p1).normalize();
    RS_Vector tan2 = para->getTangentDirection(p2).normalize();
    std::array<RS_Vector, 2> ends = {p1, p2};
    std::array<RS_Vector, 2> tans = {tan1, tan2};
    LC_ParabolaData sub_data = LC_ParabolaData::FromEndPointsTangents(ends, tans);
    if (sub_data.valid) {
      return new LC_Parabola(nullptr, sub_data);
    }
    // Fallback: Interpolate controls
    auto cps = para->getData().controlPoints;
    std::sort(cps.begin(), cps.end(), [](const RS_Vector& a, const RS_Vector& b){ return a.x < b.x; });
    double x1 = p1.x, x2 = p2.x;
    if (x1 > x2) std::swap(x1, x2);
    std::array<RS_Vector, 3> sub_cps;
    double range = x2 - x1;
    if (range < RS_TOLERANCE) return nullptr;
    for (int i = 0; i < 3; ++i) {
      double ratio = (cps[i].x - x1) / range;
      if (ratio < 0) ratio = 0; else if (ratio > 1) ratio = 1;
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
    RS_Line* line = static_cast<RS_Line*>(e);
    RS_Vector start = line->getStartpoint();
    RS_Vector dir = line->getEndpoint() - start;
    double len = dir.magnitude();
    if (len < RS_TOLERANCE) return {};
    RS_Vector unit = dir / len;
    for (RS_Vector v : inters) {
      double t = (v - start).dotP(unit);
      if (t >= 0 - RS_TOLERANCE && t <= len + RS_TOLERANCE) param_points.emplace_back(t, v);
    }
  } else if (type == RS2::EntityArc) {
    RS_Arc* arc = static_cast<RS_Arc*>(e);
    RS_Vector center = arc->getCenter();
    double a1 = arc->getAngle1();
    bool reversed = arc->isReversed();
    for (RS_Vector v : inters) {
      double ang = (v - center).angle();
      double diff = RS_Math::getAngleDifference(a1, ang, reversed);
      param_points.emplace_back(diff, v);
    }
  } else if (type == RS2::EntityCircle) {
    RS_Circle* circle = static_cast<RS_Circle*>(e);
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
    RS_Ellipse* ell = static_cast<RS_Ellipse*>(e);
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
  } else if (type == RS2::EntitySpline) {  // IMPROVED: Binary search for arc-length param
    LC_SplinePoints* spl = static_cast<LC_SplinePoints*>(e);
    double total_len = spl->getLength();
    if (total_len < RS_TOLERANCE) return {};
    for (RS_Vector v : inters) {
      // Binary search approx for param t (normalized [0,1])
      double low = 0.0, high = total_len;
      for (int iter = 0; iter < 10; ++iter) {  // ~1e-3 precision
        double mid = (low + high) / 2.0;
        RS_Vector mid_pt = spl->getNearestDist(mid, spl->getStartpoint());  // Point at dist mid
        double mid_dist_to_v = mid_pt.distanceTo(v);
        if (mid_dist_to_v < RS_TOLERANCE) break;
        if (mid < total_len / 2) high = mid; else low = mid;
      }
      double dist_along = (low + high) / 2.0;
      param_points.emplace_back(dist_along / total_len, v);
    }
  } else if (type == RS2::EntityParabola) {  // IMPROVED: Exact x-param normalization
    LC_Parabola* para = static_cast<LC_Parabola*>(e);
    LC_ParabolaData& d = para->getData();
    if (!d.valid) return {};
    double x_min = std::min({d.controlPoints[0].x, d.controlPoints[1].x, d.controlPoints[2].x});
    double x_max = std::max({d.controlPoints[0].x, d.controlPoints[1].x, d.controlPoints[2].x});
    double x_range = x_max - x_min;
    if (x_range < RS_TOLERANCE) return {};
    for (RS_Vector v : inters) {
      double x_param = d.FindX(v);
      double norm_param = (x_param - x_min) / x_range;  // [0,1]
      param_points.emplace_back(norm_param, v);
    }
  }
  std::sort(param_points.begin(), param_points.end(), [](const auto& a, const auto& b){
    return a.first < b.first;
  });
  std::vector<RS_Vector> sorted;
  for (auto& p : param_points) sorted.push_back(p.second);
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
}  // namespace LC_LoopUtils
