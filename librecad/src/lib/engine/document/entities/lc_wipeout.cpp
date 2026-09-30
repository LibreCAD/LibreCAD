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

void LC_WipeoutData::rebuildWorldVertices() {
  if (!hasNativeFrame)
    return;
  std::vector<RS_Vector> boundary = clipPath;
  if (clipBoundaryType == 1 && clipPath.size() == 2) {
    const RS_Vector& lowerLeft = clipPath.front();
    const RS_Vector& upperRight = clipPath.back();
    boundary = {lowerLeft, RS_Vector(upperRight.x, lowerLeft.y),
                upperRight, RS_Vector(lowerLeft.x, upperRight.y)};
  }
  vertices.clear();
  vertices.reserve(boundary.size());
  for (const RS_Vector &clipPoint : boundary) {
    // IMAGE/WIPEOUT group 11/12 are single-pixel vectors.  Group 14/24 clip
    // coordinates already span the image dimensions, so sizeU/sizeV do not
    // participate in this mapping.
    vertices.push_back(insertionPoint + uPixel * (clipPoint.x + 0.5)
                       + vPixel * (clipPoint.y + 0.5));
  }
}

LC_Wipeout::LC_Wipeout(RS_EntityContainer *parent, LC_WipeoutData d)
    : RS_AtomicEntity(parent), m_data(std::move(d)) {
  m_data.rebuildWorldVertices();
  calculateBorders();
}

RS_Entity *LC_Wipeout::clone() const {
  auto *w = new LC_Wipeout(*this);
  w->initId();
  return w;
}

void LC_Wipeout::calculateBorders() {
  if (m_data.vertices.empty()) {
    m_minV = RS_Vector(false);
    m_maxV = RS_Vector(false);
    return;
  }
  RS_Vector lo = m_data.vertices.front();
  RS_Vector hi = lo;
  for (const RS_Vector &v : m_data.vertices) {
    lo.x = std::min(lo.x, v.x);
    lo.y = std::min(lo.y, v.y);
    hi.x = std::max(hi.x, v.x);
    hi.y = std::max(hi.y, v.y);
  }
  m_minV = lo;
  m_maxV = hi;
}

void LC_Wipeout::draw(RS_Painter *painter) {
  if (painter == nullptr || m_data.vertices.size() < 3) {
    return;
  }

  QPolygonF uiPoly;
  uiPoly.reserve(static_cast<int>(m_data.vertices.size()));
  for (const RS_Vector &v : m_data.vertices) {
    const RS_Vector ui = painter->toGui(v);
    uiPoly << QPointF(ui.x, ui.y);
  }

  const RS_Pen savedPen = painter->getPen();
  RS_Pen fillPen = savedPen;
  fillPen.setColor(painter->getBackgroundColor());
  painter->setPen(fillPen);
  painter->fillPolygonUI(uiPoly);
  painter->setPen(savedPen);

  for (size_t i = 0; i < m_data.vertices.size(); ++i) {
    const RS_Vector &a = m_data.vertices[i];
    const RS_Vector &b = m_data.vertices[(i + 1) % m_data.vertices.size()];
    painter->drawLineWCS(a, b);
  }
}

RS_Vector LC_Wipeout::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity**) const{
  RS_Vector nearest{false};
  double bestSq = RS_MAXDOUBLE;
  for (const RS_Vector &v : m_data.vertices) {
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

RS_Vector LC_Wipeout::doGetNearestPointOnEntity(const RS_Vector& coord, bool, double* dist,
    RS_Entity** entity) const {
  if (entity != nullptr) {
    *entity = const_cast<LC_Wipeout *>(this);
  }
  return doGetNearestEndpoint(coord, dist, nullptr);
}

RS_Vector LC_Wipeout::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity**) const {
  if (m_data.vertices.empty()) {
    return RS_Vector{false};
  }
  RS_Vector centroid(0., 0.);
  for (const RS_Vector &v : m_data.vertices) {
    centroid += v;
  }
  centroid /= static_cast<double>(m_data.vertices.size());
  if (dist != nullptr) {
    *dist = (centroid - coord).magnitude();
  }
  return centroid;
}

RS_Vector LC_Wipeout::doGetNearestMiddle(const RS_Vector& coord, double* dist, int) const {
  return getNearestCenter(coord, dist);
}

RS_Vector LC_Wipeout::doGetNearestDist(double, const RS_Vector& coord, double* dist) const {
  return doGetNearestEndpoint(coord, dist, nullptr);
}

double LC_Wipeout::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel,
    double) const {
  double dist = RS_MAXDOUBLE;
  doGetNearestEndpoint(coord, &dist, nullptr);
  if (entity != nullptr) {
    *entity = const_cast<LC_Wipeout *>(this);
  }
  return dist;
}

RS_Vector LC_Wipeout::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    return RS_AtomicEntity::doGetNearestRef(coord, dist);
}

RS_Vector LC_Wipeout::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_AtomicEntity::doGetNearestSelectedRef(coord, dist);
}

void LC_Wipeout::move(const RS_Vector &offset) {
  if (m_data.hasNativeFrame) {
    m_data.insertionPoint.move(offset);
    m_data.rebuildWorldVertices();
    calculateBorders();
    return;
  }
  for (RS_Vector &v : m_data.vertices) {
    v.move(offset);
  }
  calculateBorders();
}

void LC_Wipeout::rotate(const RS_Vector &center, double angle) {
  rotate(center, RS_Vector(angle));
}

void LC_Wipeout::rotate(const RS_Vector &center, const RS_Vector &angleVector) {
  if (m_data.hasNativeFrame) {
    m_data.insertionPoint.rotate(center, angleVector);
    m_data.uPixel.rotate(RS_Vector(0.0, 0.0), angleVector);
    m_data.vPixel.rotate(RS_Vector(0.0, 0.0), angleVector);
    m_data.rebuildWorldVertices();
    calculateBorders();
    return;
  }
  for (RS_Vector &v : m_data.vertices) {
    v.rotate(center, angleVector);
  }
  calculateBorders();
}

void LC_Wipeout::scale(const RS_Vector &center, const RS_Vector &factor) {
  if (m_data.hasNativeFrame) {
    m_data.insertionPoint.scale(center, factor);
    m_data.uPixel.scale(RS_Vector(0.0, 0.0), factor);
    m_data.vPixel.scale(RS_Vector(0.0, 0.0), factor);
    m_data.rebuildWorldVertices();
    calculateBorders();
    return;
  }
  for (RS_Vector &v : m_data.vertices) {
    v.scale(center, factor);
  }
  calculateBorders();
}

void LC_Wipeout::mirror(const RS_Vector &axisPoint1,
                        const RS_Vector &axisPoint2) {
  if (m_data.hasNativeFrame) {
    const RS_Vector originBefore(0.0, 0.0);
    RS_Vector originAfter = originBefore;
    originAfter.mirror(axisPoint1, axisPoint2);
    RS_Vector uEnd = m_data.uPixel;
    RS_Vector vEnd = m_data.vPixel;
    uEnd.mirror(axisPoint1, axisPoint2);
    vEnd.mirror(axisPoint1, axisPoint2);
    m_data.insertionPoint.mirror(axisPoint1, axisPoint2);
    m_data.uPixel = uEnd - originAfter;
    m_data.vPixel = vEnd - originAfter;
    m_data.rebuildWorldVertices();
    calculateBorders();
    return;
  }
  for (RS_Vector &v : m_data.vertices) {
    v.mirror(axisPoint1, axisPoint2);
  }
  calculateBorders();
}

RS_Entity &LC_Wipeout::shear(double k) {
  if (!std::isfinite(k))
    return *this;
  const auto shearVector = [k](RS_Vector &vector) {
    vector.x += k * vector.y;
  };
  if (m_data.hasNativeFrame) {
    shearVector(m_data.insertionPoint);
    shearVector(m_data.uPixel);
    shearVector(m_data.vPixel);
    m_data.rebuildWorldVertices();
  } else {
    for (RS_Vector &vertex : m_data.vertices)
      shearVector(vertex);
  }
  calculateBorders();
  return *this;
}
