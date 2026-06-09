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
  if (!entity || entity->isDeleted()) {
      return;
  }

  RS_Vector startp = entity->getStartpoint();
  if (startp.valid) {
    if (m_hasLastPoint) {
      lineTo(startp);
    } else {
      m_firstPoint = startp;
      moveTo(startp);
    }
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
  case RS2::EntitySplinePoints:
    // LC_SplinePoints (rtti EntitySplinePoints) — analytical quadratic
    // segments via the spline-points helper. The legacy case label here
    // was RS2::EntitySpline which never matched (RS_Spline is non-atomic
    // and a different class); the cast to LC_SplinePoints* was unsound.
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
  const QPointF uiPos = toGuiPoint(pos);
  m_path.moveTo(uiPos);
  m_lastPoint = pos;
  m_hasLastPoint = true;
}

void PathBuilder::lineTo(const RS_Vector& pos) {
  const QPointF uiPos = toGuiPoint(pos);
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
  if (!line) {
      return;
  }
  m_path.lineTo(toGuiPoint(line->getEndpoint()));
}

void PathBuilder::appendArc(RS_Arc* arc) {
  if (!arc || !m_painter) {
      return;
  }
  const double radius = arc->getRadius();
  RS_Vector uiCenter = m_painter->toGui(arc->getCenter());
  RS_Vector uiRadii{m_painter->toGuiDX(radius), m_painter->toGuiDY(radius)};
  RS_Vector minCorner = uiCenter - uiRadii;
  RS_Vector uiSize = uiRadii + uiRadii;
  double startAngleDeg = m_painter->toUCSAngleDegrees(arc->getData().startAngleDegrees);
  double angularLength = arc->getData().angularLength;
  // arcTo without arcMoveTo connects from current position without starting a new subpath
  m_path.arcTo(minCorner.x, minCorner.y, uiSize.x, uiSize.y, startAngleDeg, angularLength);
}

void PathBuilder::appendCircle(RS_Circle* circle) {
  if (!circle || !m_painter) {
      return;
  }
  circle->createPainterPath(m_painter, m_path);
}

void PathBuilder::appendEllipse(RS_Ellipse* ellipse) {
  if (!ellipse || !m_painter) {
      return;
  }

  ellipse->createPainterPath(m_painter, m_path);
}

void PathBuilder::appendSplinePoints(LC_SplinePoints* spline) {
  if (!spline || !m_painter) {
      return;
  }

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
    return;

  const bool closed = spline->isClosed();
  const size_t iSplines = closed ? n : (n >= 3 ? n - 2 : 0);
  if (iSplines == 0) {
    // Degenerate: 2-point open spline degenerates to a line.
    if (!closed && n == 2) {
      lineTo(spline->getData().controlPoints[1]);
    }
    return;
  }

  bool emittedMoveTo = false;
  for (size_t i = 1; i <= iSplines; ++i) {
    RS_Vector start, ctrl, end;
    int npts = spline->GetQuadPoints(int(i), &start, &ctrl, &end);
    if (npts < 3) {
      if (npts >= 2 && start.valid && end.valid) {
        if (!emittedMoveTo) {
          m_path.moveTo(toGuiPoint(start));
          emittedMoveTo = true;
        }
        m_path.lineTo(toGuiPoint(end));
      }
      continue;
    }
    if (!emittedMoveTo) {
      m_path.moveTo(toGuiPoint(start));
      emittedMoveTo = true;
    }
    m_path.quadTo(toGuiPoint(ctrl), toGuiPoint(end));
  }
}

void PathBuilder::appendParabola(LC_Parabola* parabola) {
  if (!parabola) {
      return;
  }
  // Parabolas delegate to spline logic (quadratic Beziers)
  appendSplinePoints(parabola);
}

} // namespace LC_LoopUtils
