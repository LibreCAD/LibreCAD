/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD.org
** Copyright (C) 2025 sand1024
** Copyright (C) 2026 LibreCAD contributors
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
**********************************************************************/

#include "lc_mleader.h"

#include <algorithm>

#include "rs_painter.h"

LC_MLeader::LC_MLeader() : RS_AtomicEntity(nullptr) {}

LC_MLeader::LC_MLeader(RS_EntityContainer* parent, LC_MLeaderData d)
    : RS_AtomicEntity(parent), data(std::move(d)) {
    calculateBorders();
}

RS_Entity* LC_MLeader::clone() const {
    auto* m = new LC_MLeader(*this);
    m->initId();
    return m;
}

void LC_MLeader::calculateBorders() {
    RS_Vector lo(false), hi(false);
    auto extend = [&](const RS_Vector& v) {
        if (!lo.valid) { lo = v; hi = v; return; }
        lo.x = std::min(lo.x, v.x); lo.y = std::min(lo.y, v.y);
        hi.x = std::max(hi.x, v.x); hi.y = std::max(hi.y, v.y);
    };
    for (const auto& r : data.roots) {
        extend(r.connectionPoint);
        for (const auto& ll : r.leaderLines)
            for (const auto& p : ll.points) extend(p);
    }
    if (data.contentBasePoint.valid) extend(data.contentBasePoint);
    if (data.basePoint.valid) extend(data.basePoint);
    if (data.hasTextContents && data.textLocation.valid) extend(data.textLocation);
    if (data.hasBlockContents && data.blockLocation.valid) extend(data.blockLocation);

    if (lo.valid) { minV = lo; maxV = hi; }
    else { minV = RS_Vector(0., 0.); maxV = RS_Vector(0., 0.); }
}

void LC_MLeader::draw(RS_Painter* painter) {
    if (painter == nullptr) return;

    // Phase 6 placeholder render: draw each leader line as a polyline.
    // Phase 7 adds arrowheads at the leader-line tips and routes the
    // content (text or block) through the renderer's MText/Insert paths.
    // Spline leader type (data.leaderType == 2) is rendered as a polyline
    // here too — same start/end points, slightly different visual weight.
    for (const auto& r : data.roots) {
        for (const auto& ll : r.leaderLines) {
            for (size_t i = 1; i < ll.points.size(); ++i) {
                painter->drawLineWCS(ll.points[i - 1], ll.points[i]);
            }
        }
    }
}

RS_Vector LC_MLeader::getNearestEndpoint(const RS_Vector& coord, double* dist) const {
    RS_Vector nearest{false};
    double bestSq = RS_MAXDOUBLE;
    auto check = [&](const RS_Vector& v) {
        const double dSq = (v - coord).squared();
        if (dSq < bestSq) { bestSq = dSq; nearest = v; }
    };
    for (const auto& r : data.roots) {
        check(r.connectionPoint);
        for (const auto& ll : r.leaderLines)
            for (const auto& p : ll.points) check(p);
    }
    if (dist != nullptr && nearest.valid) {
        *dist = std::sqrt(bestSq);
    }
    return nearest;
}

RS_Vector LC_MLeader::getNearestPointOnEntity(const RS_Vector& coord, bool /*onEntity*/,
                                              double* dist, RS_Entity** entity) const {
    if (entity != nullptr) *entity = const_cast<LC_MLeader*>(this);
    return getNearestEndpoint(coord, dist);
}

RS_Vector LC_MLeader::getNearestCenter(const RS_Vector& coord, double* dist) const {
    if (data.contentBasePoint.valid) {
        if (dist != nullptr) *dist = (data.contentBasePoint - coord).magnitude();
        return data.contentBasePoint;
    }
    return getNearestEndpoint(coord, dist);
}

RS_Vector LC_MLeader::getNearestMiddle(const RS_Vector& coord, double* dist,
                                       int /*middlePoints*/) const {
    return getNearestCenter(coord, dist);
}

RS_Vector LC_MLeader::getNearestDist(double /*distance*/, const RS_Vector& coord,
                                     double* dist) const {
    return getNearestEndpoint(coord, dist);
}

double LC_MLeader::getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity,
                                      RS2::ResolveLevel /*level*/,
                                      double /*solidDist*/) const {
    double d = RS_MAXDOUBLE;
    getNearestEndpoint(coord, &d);
    if (entity != nullptr) *entity = const_cast<LC_MLeader*>(this);
    return d;
}

void LC_MLeader::move(const RS_Vector& offset) {
    for (auto& r : data.roots) {
        r.connectionPoint.move(offset);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points) p.move(offset);
    }
    if (data.contentBasePoint.valid) data.contentBasePoint.move(offset);
    if (data.basePoint.valid) data.basePoint.move(offset);
    if (data.hasTextContents && data.textLocation.valid) data.textLocation.move(offset);
    if (data.hasBlockContents && data.blockLocation.valid) data.blockLocation.move(offset);
    calculateBorders();
}

void LC_MLeader::rotate(const RS_Vector& center, double angle) {
    rotate(center, RS_Vector(angle));
}

void LC_MLeader::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& r : data.roots) {
        r.connectionPoint.rotate(center, angleVector);
        r.direction.rotate(angleVector);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points) p.rotate(center, angleVector);
    }
    if (data.contentBasePoint.valid) data.contentBasePoint.rotate(center, angleVector);
    if (data.basePoint.valid) data.basePoint.rotate(center, angleVector);
    if (data.hasTextContents && data.textLocation.valid) data.textLocation.rotate(center, angleVector);
    if (data.hasBlockContents && data.blockLocation.valid) data.blockLocation.rotate(center, angleVector);
    data.textRotation += angleVector.angle();
    data.blockRotation += angleVector.angle();
    calculateBorders();
}

void LC_MLeader::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& r : data.roots) {
        r.connectionPoint.scale(center, factor);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points) p.scale(center, factor);
    }
    if (data.contentBasePoint.valid) data.contentBasePoint.scale(center, factor);
    if (data.basePoint.valid) data.basePoint.scale(center, factor);
    if (data.hasTextContents && data.textLocation.valid) data.textLocation.scale(center, factor);
    if (data.hasBlockContents && data.blockLocation.valid) data.blockLocation.scale(center, factor);
    const double scaleAvg = 0.5 * (std::abs(factor.x) + std::abs(factor.y));
    data.textHeight *= scaleAvg;
    data.boundaryWidth *= std::abs(factor.x);
    data.boundaryHeight *= std::abs(factor.y);
    data.landingDistance *= scaleAvg;
    data.arrowSize *= scaleAvg;
    data.scaleFactor *= scaleAvg;
    calculateBorders();
}

void LC_MLeader::mirror(const RS_Vector& a1, const RS_Vector& a2) {
    for (auto& r : data.roots) {
        r.connectionPoint.mirror(a1, a2);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points) p.mirror(a1, a2);
    }
    if (data.contentBasePoint.valid) data.contentBasePoint.mirror(a1, a2);
    if (data.basePoint.valid) data.basePoint.mirror(a1, a2);
    if (data.hasTextContents && data.textLocation.valid) data.textLocation.mirror(a1, a2);
    if (data.hasBlockContents && data.blockLocation.valid) data.blockLocation.mirror(a1, a2);
    calculateBorders();
}
