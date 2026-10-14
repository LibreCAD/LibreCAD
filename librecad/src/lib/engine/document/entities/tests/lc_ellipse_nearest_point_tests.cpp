/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

// RS_Ellipse::getNearestPointOnEntity() on an elliptic arc. It found the nearest point of the whole ellipse,
// and when that point was off the arc it took the nearer end of the arc. Another critical point of the
// distance can lie on the arc and be nearer than both ends, as for a point inside the upper half of an
// ellipse just below the major axis: the whole ellipse is nearest on the lower half, the end is a vertex,
// and the nearest point of the arc is on its upper half. The checks compare the distance with a scan of the
// arc, for points near the evolute of the ellipse, the centers of curvature, where the number of critical
// points changes.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "rs_ellipse.h"
#include "rs_math.h"
#include "rs_vector.h"

namespace {

constexpr double kTwoPi = 2. * M_PI;

/// The elliptic angles of an arc counterclockwise: from m_start by m_sweep.
struct AngleRange {
    double m_start = 0.;
    double m_sweep = kTwoPi;
};

AngleRange counterclockwise(const RS_Ellipse& arc) {
    const RS_EllipseData& d = arc.getData();
    return {d.reversed ? d.angle2 : d.angle1, RS_Math::getAngleDifference(d.angle1, d.angle2, d.reversed)};
}

/// The distance from a point to an arc by a scan: the samples are the start of a golden-section search
/// around each sample that is no farther than its neighbours. Sampled once for many points.
class ArcScan {
public:
    ArcScan(const RS_Ellipse& arc, const AngleRange range) : m_arc{arc}, m_range{range} {
        m_samples.reserve(kSamples + 1);
        for (int i = 0; i <= kSamples; ++i) {
            m_samples.push_back(m_arc.getEllipsePoint(angle(i)));
        }
    }

    double distanceTo(const RS_Vector& point) const {
        std::vector<double> sampled;
        sampled.reserve(m_samples.size());
        for (const RS_Vector& sample : m_samples) {
            sampled.push_back(sample.distanceTo(point));
        }
        // the ends themselves
        double best = std::min(sampled.front(), sampled.back());
        for (int i = 0; i <= kSamples; ++i) {
            if ((i > 0 && sampled[i - 1] < sampled[i]) || (i < kSamples && sampled[i + 1] < sampled[i])) {
                continue;
            }
            best = std::min(best, goldenSection(point, angle(std::max(i - 1, 0)), angle(std::min(i + 1, kSamples))));
        }
        return best;
    }

private:
    static constexpr int kSamples = 20000;

    double angle(const int i) const {
        return m_range.m_start + (m_range.m_sweep * i / kSamples);
    }

    double goldenSection(const RS_Vector& point, double low, double high) const {
        const double ratio = (std::sqrt(5.) - 1.) / 2.;
        const auto distanceAt = [this, &point](const double t) {
            return m_arc.getEllipsePoint(t).distanceTo(point);
        };
        double u = high - ratio * (high - low);
        double v = low + ratio * (high - low);
        double du = distanceAt(u);
        double dv = distanceAt(v);
        for (int k = 0; k < 120; ++k) {
            if (du < dv) {
                high = v;
                v = u;
                dv = du;
                u = high - ratio * (high - low);
                du = distanceAt(u);
            }
            else {
                low = u;
                u = v;
                du = dv;
                v = low + ratio * (high - low);
                dv = distanceAt(v);
            }
        }
        return std::min(du, dv);
    }

    const RS_Ellipse& m_arc;
    const AngleRange m_range;
    std::vector<RS_Vector> m_samples;
};

/// a point given in the frame of the axes of \p ellipse, in the drawing
RS_Vector fromAxes(const RS_Ellipse& ellipse, RS_Vector local) {
    local.rotate(ellipse.getAngle());
    local.move(ellipse.getCenter());
    return local;
}

/// The nearest point of \p arc to \p point is on the arc, at the distance it reports, and as near as the scan.
void requireNearestOnArc(const RS_Ellipse& arc, const ArcScan& scan, const RS_Vector& point) {
    const double a = arc.getMajorRadius();
    double dist = -1.;
    const RS_Vector nearest = arc.getNearestPointOnEntity(point, true, &dist);
    const double scanned = scan.distanceTo(point);
    CAPTURE(point.x, point.y, nearest.x, nearest.y, dist, scanned);
    REQUIRE(nearest.valid);
    REQUIRE(arc.isPointOnEntity(nearest, 1e-9));
    REQUIRE(std::abs(nearest.distanceTo(point) - dist) <= 1e-12 * a);
    REQUIRE(std::abs(dist - scanned) <= 1e-9 * a);
}

/// Off the entity an arc is its whole ellipse: the same point and distance.
void requireWholeEllipseOffEntity(const RS_Ellipse& arc, const RS_Ellipse& whole, const RS_Vector& point) {
    double distArc = -1.;
    double distWhole = -2.;
    const RS_Vector fromArc = arc.getNearestPointOnEntity(point, false, &distArc);
    const RS_Vector fromWhole = whole.getNearestPointOnEntity(point, true, &distWhole);
    CAPTURE(point.x, point.y);
    REQUIRE(fromArc.x == fromWhole.x);
    REQUIRE(fromArc.y == fromWhole.y);
    REQUIRE(distArc == distWhole);
}

} // namespace

TEST_CASE("RS_Ellipse::getNearestPointOnEntity on an arc takes a nearer point of the arc over its ends",
          "[rs_ellipse][nearest]") {
    // the upper half of the ellipse; the end (-10, 0) is 3.0539 from the point, the arc 3.0000
    const RS_Ellipse arc(nullptr, {RS_Vector(0., 0.), RS_Vector(10., 0.), 0.5, 0., M_PI, false});
    const ArcScan scan(arc, counterclockwise(arc));

    SECTION("a point inside, just below the major axis") {
        const RS_Vector point(-6.94615, -0.02215);
        double dist = -1.;
        const RS_Vector nearest = arc.getNearestPointOnEntity(point, true, &dist);
        CAPTURE(nearest.x, nearest.y, dist);
        REQUIRE(nearest.y > 1.);
        REQUIRE(dist < 3.0001);
        requireNearestOnArc(arc, scan, point);
        // the distance that picks an entity
        REQUIRE(std::abs(arc.getDistanceToPoint(point) - dist) <= 1e-12);
    }

    SECTION("a point just below the center") {
        // the whole ellipse is nearest near the lower vertex of the minor axis, the arc near the upper one,
        // 5.007 away, not at the end (10, 0), 9.7 away
        const RS_Vector point(0.3, -0.01);
        double dist = -1.;
        const RS_Vector nearest = arc.getNearestPointOnEntity(point, true, &dist);
        CAPTURE(nearest.x, nearest.y, dist);
        REQUIRE(nearest.y > 4.9);
        requireNearestOnArc(arc, scan, point);
    }

    SECTION("the inward offset of the arc by 3") {
        // past the centers of curvature near the vertices, whose radius of curvature is 2.5
        for (int i = 0; i <= 512; ++i) {
            const double t = M_PI * i / 512.;
            const RS_Vector normal = RS_Vector(5. * std::cos(t), 10. * std::sin(t)).normalized();
            const RS_Vector point = RS_Vector(10. * std::cos(t), 5. * std::sin(t)) - normal * 3.;
            CAPTURE(i);
            requireNearestOnArc(arc, scan, point);
        }
    }
}

TEST_CASE("RS_Ellipse::getNearestPointOnEntity on elliptic arcs near the evolute", "[rs_ellipse][nearest]") {
    struct Placement {
        RS_Vector m_center;
        double m_rotation = 0.;
    };
    const Placement placements[] = {{RS_Vector(0., 0.), 0.}, {RS_Vector(12., -7.), 0.9}};
    // sweeps below and above 180 degrees; the last arc crosses the angle 0
    const AngleRange ranges[] = {{0., M_PI}, {0.4, 2.2}, {-2., 4.1}, {2.6, 5.6}};
    constexpr double a = 10.;

    // a ratio above 1 makes the minor axis the longer one
    for (const double ratio : {0.5, 0.3, 0.8, 1.6}) {
        const double b = a * ratio;
        for (const Placement& placement : placements) {
            const RS_Vector majorP = RS_Vector::polar(a, placement.m_rotation);
            const RS_Ellipse whole(nullptr, {placement.m_center, majorP, ratio, 0., 0., false});
            for (const AngleRange& range : ranges) {
                for (const bool reversed : {false, true}) {
                    const double end = range.m_start + range.m_sweep;
                    const RS_Ellipse arc(nullptr, {placement.m_center, majorP, ratio, reversed ? end : range.m_start,
                                                   reversed ? range.m_start : end, reversed});
                    REQUIRE(arc.isEllipticArc());
                    const ArcScan scan(arc, counterclockwise(arc));
                    CAPTURE(ratio, placement.m_rotation, range.m_start, range.m_sweep, reversed);
                    for (int k = 0; k < 16; ++k) {
                        // the evolute, and points inside and outside it
                        const double t = kTwoPi * (k + 0.37) / 16.;
                        const RS_Vector onEvolute{(a * a - b * b) / a * std::pow(std::cos(t), 3),
                                                  (b * b - a * a) / b * std::pow(std::sin(t), 3)};
                        for (const double scale : {0.97, 1., 1.03}) {
                            const RS_Vector point = fromAxes(arc, onEvolute * scale);
                            CAPTURE(k, scale);
                            requireNearestOnArc(arc, scan, point);
                            requireWholeEllipseOffEntity(arc, whole, point);
                        }
                    }
                }
            }
        }
    }
}
