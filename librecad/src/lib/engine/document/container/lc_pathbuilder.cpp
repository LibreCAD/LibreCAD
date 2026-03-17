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
  m_path.setFillRule(Qt::WindingFill);  // Critical for correct hatch hole rendering
  m_hasLastPoint = false;
}

void PathBuilder::append(RS_Entity* entity) {
  if (!entity || entity->isUndone()) return;

  RS_Vector startp = entity->getStartpoint();
  if (m_hasLastPoint)
    lineTo(startp);
  else
    moveTo(startp);
  const double tol = 1e-6;
  if (!m_hasLastPoint || m_lastPoint.distanceTo(startp) > tol) {
    LC_ERR<<__func__<<": line "<<__LINE__<<": found gap at "<<m_lastPoint<<" to "<<startp;
  }
  if (!m_hasLastPoint)
    m_firstPoint = startp;

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

void PathBuilder::appendLine(RS_Line* line) {
  if (!line) return;
  m_path.moveTo(toGuiPoint(line->getStartpoint()));
  m_path.lineTo(toGuiPoint(line->getEndpoint()));
  //LC_LOG<<"adding line: now at: "<<toGuiPoint(line->getEndpoint()).y();
}

void PathBuilder::appendArc(RS_Arc* arc) {
  if (!arc || !m_painter) return;
  arc->createPainterPath(m_painter, m_path);
  //LC_LOG<<"adding arc: now at: "<<m_path.currentPosition().y();
}

void PathBuilder::appendCircle(RS_Circle* circle) {
  if (!circle || !m_painter) return;
  circle->createPainterPath(m_painter, m_path);
}

void PathBuilder::appendEllipse(RS_Ellipse* ellipse) {
  if (!ellipse || !m_painter) return;

  ellipse->createPainterPath(m_painter, m_path);
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