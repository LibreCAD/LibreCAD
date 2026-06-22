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
// File: lc_pathbuilder.cpp

#include "lc_pathbuilder.h"

#include "rs_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "lc_splinepoints.h"
#include "lc_parabola.h"
#include "rs_painter.h"
#include "rs_math.h"
#include "rs_debug.h"

#include <cmath>

namespace {

bool hasFiniteValue(double value) {
  return std::isfinite(value);
}

bool hasUsableVector(const RS_Vector& vector) {
  return vector.valid
      && hasFiniteValue(vector.x)
      && hasFiniteValue(vector.y)
      && hasFiniteValue(vector.z);
}

bool hasUsablePoint(const QPointF& point) {
  return hasFiniteValue(point.x()) && hasFiniteValue(point.y());
}

bool hasUsableArcParameters(double x, double y, double width, double height, double startAngle, double sweepAngle) {
  return hasFiniteValue(x)
      && hasFiniteValue(y)
      && hasFiniteValue(width)
      && hasFiniteValue(height)
      && width > 0.
      && height > 0.
      && hasFiniteValue(startAngle)
      && hasFiniteValue(sweepAngle);
}

}

namespace LC_LoopUtils {

PathBuilder::PathBuilder(RS_Painter* painter)
    : m_painter(painter) {
  assert(m_painter != nullptr);
  m_path.setFillRule(Qt::WindingFill);  // Critical for correct hatch hole rendering
  m_hasLastPoint = false;
}

void PathBuilder::append(RS_Entity* entity) {
  if (!entity || entity->isUndone()) return;

  RS2::EntityType type = entity->rtti();
  bool appended = false;

  switch (type) {
  case RS2::EntityLine:
    appended = appendLine(static_cast<RS_Line*>(entity));
    break;
  case RS2::EntityArc:
    appended = appendArc(static_cast<RS_Arc*>(entity));
    break;
  case RS2::EntityCircle:
    appended = appendCircle(static_cast<RS_Circle*>(entity));
    break;
  case RS2::EntityEllipse:
    appended = appendEllipse(static_cast<RS_Ellipse*>(entity));
    break;
  case RS2::EntitySplinePoints:
    // LC_SplinePoints (rtti EntitySplinePoints) — analytical quadratic
    // segments via the spline-points helper. The legacy case label here
    // was RS2::EntitySpline which never matched (RS_Spline is non-atomic
    // and a different class); the cast to LC_SplinePoints* was unsound.
    appended = appendSplinePoints(static_cast<LC_SplinePoints*>(entity));
    break;
  case RS2::EntityParabola:
    appended = appendParabola(static_cast<LC_Parabola*>(entity));
    break;
  default:
    RS_DEBUG->print(RS_Debug::D_WARNING, "PathBuilder::append: Unsupported entity type %d", static_cast<int>(type));
    break;
  }

  if (appended)
    updateLastPoint(entity);
}

void PathBuilder::moveTo(const RS_Vector& pos) {
  if (!hasUsableVector(pos))
    return;

  QPointF uiPos = toGuiPoint(pos);
  if (!hasUsablePoint(uiPos))
    return;

  m_path.moveTo(uiPos);
  m_lastPoint = pos;
  m_hasLastPoint = true;
}

void PathBuilder::lineTo(const RS_Vector& pos) {
  if (!hasUsableVector(pos))
    return;

  QPointF uiPos = toGuiPoint(pos);
  if (!hasUsablePoint(uiPos))
    return;

  m_path.lineTo(uiPos);
  m_lastPoint = pos;
  m_hasLastPoint = true;
}

void PathBuilder::closeSubpath() {
  if (m_path.isEmpty())
    return;

  //m_path.lineTo(toGuiPoint(m_firstPoint));
  m_path = m_path.toReversed();
  m_path.closeSubpath();
  // Keep m_hasLastPoint for next append (continuity)
}

void PathBuilder::clear() {
  m_path = QPainterPath();
  m_lastPoint = RS_Vector(0., 0.);
  m_hasLastPoint = false;
}

QPointF PathBuilder::toGuiPoint(const RS_Vector& vp) const {
  RS_Vector guiVp = m_painter->toGui(vp);
  return {guiVp.x, guiVp.y};
}

bool PathBuilder::appendStartPoint(const RS_Vector& startPoint) {
  if (!startPoint.valid)
    return true;

  if (!hasUsableVector(startPoint))
    return false;

  const QPointF uiStart = toGuiPoint(startPoint);
  if (!hasUsablePoint(uiStart))
    return false;

  if (m_hasLastPoint) {
    m_path.lineTo(uiStart);
  } else {
    m_firstPoint = startPoint;
    m_path.moveTo(uiStart);
  }
  return true;
}

void PathBuilder::updateLastPoint(const RS_Entity* entity) {
  const RS_Vector endpoint = entity->getEndpoint();
  if (hasUsableVector(endpoint)) {
    m_lastPoint = endpoint;
    m_hasLastPoint = true;
  } else {
    m_hasLastPoint = false;
  }
}

bool PathBuilder::appendLine(RS_Line* line) {
  if (!line)
    return false;

  const RS_Vector endpoint = line->getEndpoint();
  if (!hasUsableVector(endpoint))
    return false;

  const QPointF uiEndpoint = toGuiPoint(endpoint);
  if (!hasUsablePoint(uiEndpoint) || !appendStartPoint(line->getStartpoint()))
    return false;

  m_path.lineTo(uiEndpoint);
  return true;
}

bool PathBuilder::appendArc(RS_Arc* arc) {
  if (!arc || !m_painter)
    return false;

  const double radius = arc->getRadius();
  if (!hasFiniteValue(radius) || radius <= 0.)
    return false;

  RS_Vector uiCenter = m_painter->toGui(arc->getCenter());
  RS_Vector uiRadii{m_painter->toGuiDX(radius), m_painter->toGuiDY(radius)};
  RS_Vector minCorner = uiCenter - uiRadii;
  RS_Vector uiSize = uiRadii + uiRadii;
  double startAngleDeg = m_painter->toUCSAngleDegrees(arc->getData().startAngleDegrees);
  double angularLength = arc->getData().angularLength;
  if (!hasUsableArcParameters(minCorner.x, minCorner.y, uiSize.x, uiSize.y, startAngleDeg, angularLength))
    return false;

  if (!appendStartPoint(arc->getStartpoint()))
    return false;

  // arcTo without arcMoveTo connects from current position without starting a new subpath
  m_path.arcTo(minCorner.x, minCorner.y, uiSize.x, uiSize.y, startAngleDeg, angularLength);
  return true;
}

bool PathBuilder::appendCircle(RS_Circle* circle) {
  if (!circle || !m_painter || !hasUsableVector(circle->getCenter()) || !hasFiniteValue(circle->getRadius()) || circle->getRadius() <= 0.)
    return false;

  circle->createPainterPath(m_painter, m_path);
  return true;
}

bool PathBuilder::appendEllipse(RS_Ellipse* ellipse) {
  if (!ellipse || !m_painter || !hasUsableVector(ellipse->getCenter()) || !hasUsableVector(ellipse->getMajorP())
      || !hasFiniteValue(ellipse->getRatio()) || ellipse->getRatio() <= 0.)
    return false;

  ellipse->createPainterPath(m_painter, m_path);
  return true;
}

bool PathBuilder::appendSplinePoints(LC_SplinePoints* spline) {
  if (!spline || !m_painter)
    return false;

  const QPainterPath savedPath = m_path;
  const RS_Vector savedFirstPoint = m_firstPoint;
  const RS_Vector savedLastPoint = m_lastPoint;
  const bool savedHasLastPoint = m_hasLastPoint;
  const auto fail = [&]() {
    m_path = savedPath;
    m_firstPoint = savedFirstPoint;
    m_lastPoint = savedLastPoint;
    m_hasLastPoint = savedHasLastPoint;
    return false;
  };

  if (!appendStartPoint(spline->getStartpoint()))
    return false;

  // Iterate quadratic segments using the same indexing as
  // LC_SplinePoints::fillStrokePoints: i = 1..iSplines, where
  //   closed: iSplines = N control points
  //   open:   iSplines = N - 2 control points
  // GetQuadPoints uses the internal control-point representation regardless
  // of whether the source was useControlPoints=true or splinePoints-based
  // (UpdateControlPoints regenerates them either way), so iterating over
  // controlPoints is correct for both modes.
  const size_t n = spline->getData().controlPoints.size();
  if (n < 2)
    return fail();

  const bool closed = spline->isClosed();
  const size_t iSplines = closed ? n : (n >= 3 ? n - 2 : 0);
  if (iSplines == 0) {
    // Degenerate: 2-point open spline degenerates to a line.
    if (!closed && n == 2) {
      const RS_Vector endpoint = spline->getData().controlPoints[1];
      const QPointF uiEndpoint = toGuiPoint(endpoint);
      if (!hasUsableVector(endpoint) || !hasUsablePoint(uiEndpoint))
        return fail();

      m_path.lineTo(uiEndpoint);
      return true;
    }
    return fail();
  }

  bool emittedMoveTo = false;
  for (size_t i = 1; i <= iSplines; ++i) {
    RS_Vector start, ctrl, end;
    int npts = spline->GetQuadPoints(int(i), &start, &ctrl, &end);
    if (npts < 3) {
      if (npts >= 2 && start.valid && end.valid) {
        const QPointF uiStart = toGuiPoint(start);
        const QPointF uiEnd = toGuiPoint(end);
        if (!hasUsableVector(start) || !hasUsableVector(end) || !hasUsablePoint(uiStart) || !hasUsablePoint(uiEnd))
          return fail();

        if (!emittedMoveTo) {
          m_path.moveTo(uiStart);
          emittedMoveTo = true;
        }
        m_path.lineTo(uiEnd);
      }
      continue;
    }

    const QPointF uiStart = toGuiPoint(start);
    const QPointF uiCtrl = toGuiPoint(ctrl);
    const QPointF uiEnd = toGuiPoint(end);
    if (!hasUsableVector(start) || !hasUsableVector(ctrl) || !hasUsableVector(end)
        || !hasUsablePoint(uiStart) || !hasUsablePoint(uiCtrl) || !hasUsablePoint(uiEnd)) {
      return fail();
    }

    if (!emittedMoveTo) {
      m_path.moveTo(uiStart);
      emittedMoveTo = true;
    }
    m_path.quadTo(uiCtrl, uiEnd);
  }
  return emittedMoveTo ? true : fail();
}

bool PathBuilder::appendParabola(LC_Parabola* parabola) {
  if (!parabola)
    return false;

  // Parabolas delegate to spline logic (quadratic Beziers)
  return appendSplinePoints(parabola);
}

} // namespace LC_LoopUtils
