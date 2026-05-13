// File: lc_wipeout.cpp

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD (librecad.org)
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "lc_wipeout.h"

#include <QPolygonF>

#include "lc_rect.h"
#include "rs_painter.h"

LC_Wipeout::LC_Wipeout(RS_EntityContainer *parent, LC_WipeoutData d)
    : RS_AtomicEntity(parent), data(std::move(d)) {
  calculateBorders();
}

RS_Entity *LC_Wipeout::clone() const {
  auto *w = new LC_Wipeout(*this);
  w->initId();
  return w;
}

void LC_Wipeout::calculateBorders() {
  if (data.vertices.empty()) {
    minV = RS_Vector(0., 0.);
    maxV = RS_Vector(0., 0.);
    return;
  }
  RS_Vector lo = data.vertices.front();
  RS_Vector hi = lo;
  for (const RS_Vector &v : data.vertices) {
    lo.x = std::min(lo.x, v.x);
    lo.y = std::min(lo.y, v.y);
    hi.x = std::max(hi.x, v.x);
    hi.y = std::max(hi.y, v.y);
  }
  minV = lo;
  maxV = hi;
}

void LC_Wipeout::draw(RS_Painter *painter) {
  if (painter == nullptr || data.vertices.size() < 3) {
    return;
  }

  QPolygonF uiPoly;
  uiPoly.reserve(static_cast<int>(data.vertices.size()));
  for (const RS_Vector &v : data.vertices) {
    const RS_Vector ui = painter->toGui(v);
    uiPoly << QPointF(ui.x, ui.y);
  }

  const RS_Pen savedPen = painter->getPen();
  RS_Pen fillPen = savedPen;
  fillPen.setColor(painter->getBackgroundColor());
  painter->setPen(fillPen);
  painter->fillPolygonUI(uiPoly);
  painter->setPen(savedPen);

  for (size_t i = 0; i < data.vertices.size(); ++i) {
    const RS_Vector &a = data.vertices[i];
    const RS_Vector &b = data.vertices[(i + 1) % data.vertices.size()];
    painter->drawLineWCS(a, b);
  }
}

RS_Vector LC_Wipeout::getNearestEndpoint(const RS_Vector &coord,
                                         double *dist) const {
  RS_Vector nearest{false};
  double bestSq = RS_MAXDOUBLE;
  for (const RS_Vector &v : data.vertices) {
    const double dSq = (v - coord).squared();
    if (dSq < bestSq) {
      bestSq = dSq;
      nearest = v;
    }
  }
  if (dist != nullptr && nearest.valid) {
    *dist = std::sqrt(bestSq);
  }
  return nearest;
}

RS_Vector LC_Wipeout::getNearestPointOnEntity(const RS_Vector &coord,
                                              bool /*onEntity*/, double *dist,
                                              RS_Entity **entity) const {
  if (entity != nullptr) {
    *entity = const_cast<LC_Wipeout *>(this);
  }
  return getNearestEndpoint(coord, dist);
}

RS_Vector LC_Wipeout::getNearestCenter(const RS_Vector &coord,
                                       double *dist) const {
  if (data.vertices.empty()) {
    return RS_Vector{false};
  }
  RS_Vector centroid(0., 0.);
  for (const RS_Vector &v : data.vertices) {
    centroid += v;
  }
  centroid /= static_cast<double>(data.vertices.size());
  if (dist != nullptr) {
    *dist = (centroid - coord).magnitude();
  }
  return centroid;
}

RS_Vector LC_Wipeout::getNearestMiddle(const RS_Vector &coord, double *dist,
                                       int /*middlePoints*/) const {
  return getNearestCenter(coord, dist);
}

RS_Vector LC_Wipeout::getNearestDist(double /*distance*/,
                                     const RS_Vector &coord,
                                     double *dist) const {
  return getNearestEndpoint(coord, dist);
}

double LC_Wipeout::getDistanceToPoint(const RS_Vector &coord,
                                      RS_Entity **entity,
                                      RS2::ResolveLevel /*level*/,
                                      double /*solidDist*/) const {
  double dist = RS_MAXDOUBLE;
  getNearestEndpoint(coord, &dist);
  if (entity != nullptr) {
    *entity = const_cast<LC_Wipeout *>(this);
  }
  return dist;
}

void LC_Wipeout::move(const RS_Vector &offset) {
  for (RS_Vector &v : data.vertices) {
    v.move(offset);
  }
  calculateBorders();
}

void LC_Wipeout::rotate(const RS_Vector &center, double angle) {
  rotate(center, RS_Vector(angle));
}

void LC_Wipeout::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  for (RS_Vector &v : data.vertices) {
    v.rotate(center, angleVector);
  }
  calculateBorders();
}

void LC_Wipeout::scale(const RS_Vector &center, const RS_Vector &factor) {
  for (RS_Vector &v : data.vertices) {
    v.scale(center, factor);
  }
  calculateBorders();
}

void LC_Wipeout::mirror(const RS_Vector &axisPoint1,
                        const RS_Vector &axisPoint2) {
  for (RS_Vector &v : data.vertices) {
    v.mirror(axisPoint1, axisPoint2);
  }
  calculateBorders();
}