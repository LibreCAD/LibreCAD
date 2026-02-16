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

namespace LC_LoopUtils {

PathBuilder::PathBuilder(RS_Painter* painter)
    : m_painter(painter) {
  assert(m_painter != nullptr);
  m_path.setFillRule(Qt::OddEvenFill);  // Critical for correct hatch hole rendering
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

void PathBuilder::appendLine(RS_Line* line) {
  if (!line) return;
  m_path.lineTo(toGuiPoint(line->getEndpoint()));
}

void PathBuilder::appendArc(RS_Arc* arc) {
  if (!arc || !m_painter) return;

  // TODO: pixel-level precision (issue #2035)
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

void PathBuilder::appendCircle(RS_Circle* circle) {
  if (!circle || !m_painter) return;

  // TODO: pixel-level precision (issue #2035)
  QPointF center = toGuiPoint(circle->getCenter());
  double radiusX = m_painter->toGuiDX(circle->getRadius());
  double radiusY = m_painter->toGuiDY(circle->getRadius());
  QPointF halfSize{radiusX, radiusY};
  QRectF circleRect{center - halfSize, center + halfSize};

  m_path.addEllipse(circleRect);
}

void PathBuilder::appendEllipse(RS_Ellipse* ellipse) {
  if (!ellipse || !m_painter) return;

  // TODO: pixel-level precision (issue #2035)
  m_painter->drawEllipseBySplinePointsUI(*ellipse, m_path);
}

void PathBuilder::appendSplinePoints(LC_SplinePoints* spline) {
  if (!spline || !m_painter) return;

  const auto& points = spline->getPoints();
  if (points.empty()) return;

  size_t n_points = points.size();
  size_t num_segs = spline->isClosed() ? n_points : n_points - 1;
  if (num_segs == 0) {
    lineTo(spline->getEndpoint());
    return;
  }

  // Current position assumed at start of first segment
  for (size_t i = 0; i < num_segs; ++i) {
    RS_Vector start, ctrl, end;
    if (spline->GetQuadPoints(int(i), &start, &ctrl, &end) != 0) {
      m_path.moveTo(toGuiPoint(start));  // Ensure start (rare fallback)
      m_path.quadTo(toGuiPoint(ctrl), toGuiPoint(end));
    } else {
      lineTo(end);  // Linear fallback
    }
  }
}

void PathBuilder::appendParabola(LC_Parabola* parabola) {
  if (!parabola) return;
  // Parabolas delegate to spline logic (quadratic Beziers)
  appendSplinePoints(parabola);
}

} // namespace LC_LoopUtils