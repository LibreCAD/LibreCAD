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
// File: lc_pathbuilder.h
#ifndef LC_PATHBUILDER_H
#define LC_PATHBUILDER_H

#include <QPainterPath>

#include "rs_vector.h"

class RS_Entity;
class RS_Painter;
class RS_Vector;
class RS_Line;
class RS_Arc;
class RS_Circle;
class RS_Ellipse;

class LC_SplinePoints;
class LC_Parabola;

namespace LC_LoopUtils {

/**
 * @brief PathBuilder - builds QPainterPath from RS_Entity contours.
 * All geometry is transformed to UI (screen) coordinates via RS_Painter.
 * Handles lines, arcs, circles, ellipses, splines, parabolas.
 * Ensures continuity and uses OddEvenFill for correct hole/island rendering in hatches.
 */
class PathBuilder {
public:
  /**
   * @brief Constructor.
   * @param painter Required for WCS → UI transforms (toGui/toGuiDX/etc.).
   */
  explicit PathBuilder(RS_Painter* painter);
  ~PathBuilder() = default;

  /**
   * @brief Appends entity to path (handles direction, start continuity).
   */
  void append(RS_Entity* entity);

  /**
   * @brief Starts new subpath at position (WCS).
   */
  void moveTo(const RS_Vector& pos);

  /**
   * @brief Line to position (WCS).
   */
  void lineTo(const RS_Vector& pos);

  /**
   * @brief Closes current subpath.
   */
  void closeSubpath();

  /**
   * @brief Access built path (for LC_Loops::getPainterPath).
   */
  QPainterPath& getPath() { return m_path; }
  const QPainterPath& getPath() const { return m_path; }

  /**
   * @brief Resets path.
   */
  void clear();

  friend class LC_Loops;

private:
  /**
   * @brief Converts WCS point to UI (screen) coordinates.
   */
  QPointF toGuiPoint(const RS_Vector& vp) const;

  void appendLine(RS_Line* line);
  void appendArc(RS_Arc* arc);
  void appendCircle(RS_Circle* circle);
  void appendEllipse(RS_Ellipse* ellipse);
  void appendSplinePoints(LC_SplinePoints* spline);
  void appendParabola(LC_Parabola* parabola);

  RS_Painter* m_painter = nullptr;
  QPainterPath m_path;
  RS_Vector m_firstPoint{};  ///< First WCS point for continuity.
  RS_Vector m_lastPoint{};  ///< Last WCS point for continuity.
  bool m_hasLastPoint = false;
};

} // namespace LC_LoopUtils

#endif // LC_PATHBUILDER_H