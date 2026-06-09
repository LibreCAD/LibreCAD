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

LC_MLeader::LC_MLeader() : RS_AtomicEntity(nullptr) {
}

LC_MLeader::LC_MLeader(RS_EntityContainer* parent, LC_MLeaderData d)
    : RS_AtomicEntity(parent), m_data(std::move(d)) {
    calculateBorders();
}

RS_Entity* LC_MLeader::clone() const {
    auto* m = new LC_MLeader(*this);
    m->initId();
    return m;
}

void LC_MLeader::calculateBorders() {
    RS_Vector lo(false), hi(false);
    auto extend = [&](const RS_Vector& v)
    {
        if (!lo.valid) {
            lo = v;
            hi = v;
            return;
        }
        lo.x = std::min(lo.x, v.x);
        lo.y = std::min(lo.y, v.y);
        hi.x = std::max(hi.x, v.x);
        hi.y = std::max(hi.y, v.y);
    };
    for (const auto& r : m_data.roots) {
        extend(r.connectionPoint);
        for (const auto& ll : r.leaderLines)
            for (const auto& p : ll.points)
                extend(p);
    }
    if (m_data.contentBasePoint.valid)
        extend(m_data.contentBasePoint);
    if (m_data.basePoint.valid)
        extend(m_data.basePoint);
    if (m_data.hasTextContents && m_data.textLocation.valid)
        extend(m_data.textLocation);
    if (m_data.hasBlockContents && m_data.blockLocation.valid)
        extend(m_data.blockLocation);

    if (lo.valid) {
        m_minV = lo;
        m_maxV = hi;
    }
    else {
        m_minV = RS_Vector(0., 0.);
        m_maxV = RS_Vector(0., 0.);
    }
}

void LC_MLeader::draw(RS_Painter* painter) {
    if (painter == nullptr)
        return;

    // For each leader line: draw the polyline + a filled arrowhead at the
    // tip (the last point — the spec orders points from connection-side
    // to arrow-side).  Spline leader type renders as polyline for now;
    // proper spline-leader fitting is a follow-up.
    //
    // Arrowhead geometry: simple triangle with width = arrowSize, depth
    // = arrowSize.  Aligned with the last segment direction.  AutoCAD's
    // default arrow blocks (closed-filled, dot, etc.) require resolving
    // the arrow-handle from MLEADERSTYLE — deferred; the closed-filled
    // triangle here matches AutoCAD's default appearance closely enough
    // for read-only ingestion.
    const double arrowSize =
        m_data.arrowSize > 0.0 ? m_data.arrowSize : m_data.scaleFactor * 1.0;

    for (const auto& r : m_data.roots) {
        for (const auto& ll : r.leaderLines) {
            const size_t n = ll.points.size();
            if (n < 2)
                continue;
            for (size_t i = 1; i < n; ++i) {
                painter->drawLineWCS(ll.points[i - 1], ll.points[i]);
            }
            // Arrowhead at the tip: triangle from tip back along the
            // last segment by `arrowSize`, fanned out by `arrowSize/3`
            // perpendicular on each side.
            const RS_Vector& tip = ll.points[n - 1];
            const RS_Vector& prev = ll.points[n - 2];
            const RS_Vector seg = tip - prev;
            const double segLen = seg.magnitude();
            if (segLen < RS_TOLERANCE)
                continue;
            const RS_Vector dir = seg / segLen;
            const RS_Vector perp{-dir.y, dir.x};
            const RS_Vector basePt = tip - dir * arrowSize;
            const RS_Vector w1 = basePt + perp * (arrowSize / 6.0);
            const RS_Vector w2 = basePt - perp * (arrowSize / 6.0);
            painter->drawSolidWCS({tip, w1, w2});
        }
    }
}

RS_Vector LC_MLeader::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    RS_Vector nearest{false};
    double bestSq = RS_MAXDOUBLE;
    auto check = [&](const RS_Vector& v)
    {
        const double dSq = (v - coord).squared();
        if (dSq < bestSq) {
            bestSq = dSq;
            nearest = v;
        }
    };
    for (const auto& r : m_data.roots) {
        check(r.connectionPoint);
        for (const auto& ll : r.leaderLines)
            for (const auto& p : ll.points)
                check(p);
    }
    if (dist != nullptr && nearest.valid) {
        *dist = std::sqrt(bestSq);
    }
    return nearest;
}

RS_Vector LC_MLeader::doGetNearestPointOnEntity(const RS_Vector& coord,
                                                bool /*onEntity*/, double* dist,
                                                RS_Entity** entity) const {
    if (entity != nullptr)
        *entity = const_cast<LC_MLeader*>(this);
    return getNearestEndpoint(coord, nullptr, dist);
}

RS_Vector LC_MLeader::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    if (m_data.contentBasePoint.valid) {
        if (dist != nullptr)
            *dist = (m_data.contentBasePoint - coord).magnitude();
        return m_data.contentBasePoint;
    }
    return getNearestEndpoint(coord, nullptr, dist);
}

RS_Vector LC_MLeader::doGetNearestMiddle(const RS_Vector& coord, double* dist,
                                         int /*middlePoints*/) const {
    return getNearestCenter(coord, dist);
}

RS_Vector LC_MLeader::doGetNearestDist(double /*distance*/,
                                       const RS_Vector& coord,
                                       double* dist) const {
    return getNearestEndpoint(coord, nullptr, dist);
}

bool LC_MLeader::doIsPointOnEntity(const RS_Vector& coord, double tolerance) const {
    return RS_AtomicEntity::doIsPointOnEntity(coord, tolerance);
}

RS_Vector LC_MLeader::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    return RS_AtomicEntity::doGetNearestRef(coord, dist);
}

RS_Vector LC_MLeader::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    return RS_AtomicEntity::doGetNearestSelectedRef(coord, dist);
}

double LC_MLeader::doGetDistanceToPoint(const RS_Vector& coord,
                                        RS_Entity** entity,
                                        RS2::ResolveLevel /*level*/,
                                        double /*solidDist*/) const {
    double d = RS_MAXDOUBLE;
    getNearestEndpoint(coord, nullptr, &d);
    if (entity != nullptr)
        *entity = const_cast<LC_MLeader*>(this);
    return d;
}

void LC_MLeader::move(const RS_Vector& offset) {
    for (auto& r : m_data.roots) {
        r.connectionPoint.move(offset);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points)
                p.move(offset);
    }
    if (m_data.contentBasePoint.valid)
        m_data.contentBasePoint.move(offset);
    if (m_data.basePoint.valid)
        m_data.basePoint.move(offset);
    if (m_data.hasTextContents && m_data.textLocation.valid)
        m_data.textLocation.move(offset);
    if (m_data.hasBlockContents && m_data.blockLocation.valid)
        m_data.blockLocation.move(offset);
    calculateBorders();
}

void LC_MLeader::rotate(const RS_Vector& center, double angle) {
    rotate(center, RS_Vector(angle));
}

void LC_MLeader::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& r : m_data.roots) {
        r.connectionPoint.rotate(center, angleVector);
        r.direction.rotate(angleVector);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points)
                p.rotate(center, angleVector);
    }
    if (m_data.contentBasePoint.valid)
        m_data.contentBasePoint.rotate(center, angleVector);
    if (m_data.basePoint.valid)
        m_data.basePoint.rotate(center, angleVector);
    if (m_data.hasTextContents && m_data.textLocation.valid)
        m_data.textLocation.rotate(center, angleVector);
    if (m_data.hasBlockContents && m_data.blockLocation.valid)
        m_data.blockLocation.rotate(center, angleVector);
    m_data.textRotation += angleVector.angle();
    m_data.blockRotation += angleVector.angle();
    calculateBorders();
}

void LC_MLeader::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& r : m_data.roots) {
        r.connectionPoint.scale(center, factor);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points)
                p.scale(center, factor);
    }
    if (m_data.contentBasePoint.valid)
        m_data.contentBasePoint.scale(center, factor);
    if (m_data.basePoint.valid)
        m_data.basePoint.scale(center, factor);
    if (m_data.hasTextContents && m_data.textLocation.valid)
        m_data.textLocation.scale(center, factor);
    if (m_data.hasBlockContents && m_data.blockLocation.valid)
        m_data.blockLocation.scale(center, factor);
    const double scaleAvg = 0.5 * (std::abs(factor.x) + std::abs(factor.y));
    m_data.textHeight *= scaleAvg;
    m_data.boundaryWidth *= std::abs(factor.x);
    m_data.boundaryHeight *= std::abs(factor.y);
    m_data.landingDistance *= scaleAvg;
    m_data.arrowSize *= scaleAvg;
    m_data.scaleFactor *= scaleAvg;
    calculateBorders();
}

void LC_MLeader::mirror(const RS_Vector& a1, const RS_Vector& a2) {
    for (auto& r : m_data.roots) {
        r.connectionPoint.mirror(a1, a2);
        for (auto& ll : r.leaderLines)
            for (auto& p : ll.points)
                p.mirror(a1, a2);
    }
    if (m_data.contentBasePoint.valid)
        m_data.contentBasePoint.mirror(a1, a2);
    if (m_data.basePoint.valid)
        m_data.basePoint.mirror(a1, a2);
    if (m_data.hasTextContents && m_data.textLocation.valid)
        m_data.textLocation.mirror(a1, a2);
    if (m_data.hasBlockContents && m_data.blockLocation.valid)
        m_data.blockLocation.mirror(a1, a2);
    calculateBorders();
}
