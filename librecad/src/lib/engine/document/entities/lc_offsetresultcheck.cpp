/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_offsetresultcheck.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "rs_arc.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();
constexpr double kTwoPi = 2.0 * M_PI;

double cross(const RS_Vector& u, const RS_Vector& v) {
    return u.x * v.y - u.y * v.x;
}

double dot(const RS_Vector& u, const RS_Vector& v) {
    return u.x * v.x + u.y * v.y;
}

double norm(const RS_Vector& u) {
    return std::hypot(u.x, u.y);
}

double distance(const RS_Vector& u, const RS_Vector& v) {
    return std::hypot(u.x - v.x, u.y - v.y);
}

RS_Vector direction(const double angle) {
    return RS_Vector{std::cos(angle), std::sin(angle)};
}

/** @p angle in [0, 2 pi). */
double normalized(const double angle) {
    double a = std::fmod(angle, kTwoPi);
    if (a < 0.0) {
        a += kTwoPi;
    }
    return a >= kTwoPi ? 0.0 : a;
}

/** How far along the arc's sweep, from its start, the direction @p angle lies: [0, 2 pi). */
double alongSweep(const LC_OffsetSegment& arc, const double angle) {
    return arc.sweep >= 0.0 ? normalized(angle - arc.startAngle) : normalized(arc.startAngle - angle);
}

/** Whether the direction @p angle from the arc's centre lies within its sweep, widened by @p slack radians. */
bool inSweep(const LC_OffsetSegment& arc, const double angle, const double slack) {
    const double span = std::abs(arc.sweep);
    if (span + 2.0 * slack >= kTwoPi) {
        return true;
    }
    const double along = alongSweep(arc, angle);
    return along <= span + slack || along >= kTwoPi - slack;
}

/** The arc as an anticlockwise interval of angles [lo, lo + span], lo in [0, 2 pi). */
void angularInterval(const LC_OffsetSegment& arc, double& lo, double& span) {
    lo = normalized(arc.sweep >= 0.0 ? arc.startAngle : arc.startAngle + arc.sweep);
    span = std::abs(arc.sweep);
}

/** The angles bounding the stretches two arcs of one circle share, widened by @p slack radians. */
std::vector<double> sharedAngles(const LC_OffsetSegment& s, const LC_OffsetSegment& t, const double slack) {
    double lo1 = 0.0;
    double span1 = 0.0;
    double lo2 = 0.0;
    double span2 = 0.0;
    angularInterval(s, lo1, span1);
    angularInterval(t, lo2, span2);
    std::vector<double> angles;
    for (const int k : {-1, 0, 1}) {
        const double start = lo2 + kTwoPi * k;
        const double from = std::max(lo1, start);
        const double to = std::min(lo1 + span1, start + span2);
        if (from <= to + slack) {
            angles.push_back(from);
            if (to - from > slack) {
                angles.push_back(to);
            }
        }
    }
    return angles;
}

/** A tolerance never below the rounding of the coordinates involved. */
double slackFor(const double tolerance, const double scale) {
    return std::max(tolerance, 1e-13 * scale);
}

double pointLineDistance(const RS_Vector& p, const LC_OffsetSegment& s) {
    const RS_Vector r = s.b - s.a;
    const double length2 = dot(r, r);
    if (!(length2 > 0.0)) {
        return distance(p, s.a);
    }
    const double u = std::clamp(dot(p - s.a, r) / length2, 0.0, 1.0);
    return distance(p, s.a + r * u);
}

double pointArcDistance(const RS_Vector& p, const LC_OffsetSegment& s) {
    const RS_Vector v = p - s.centre;
    const double m = norm(v);
    if (m <= 1e-15 * std::max(1.0, s.radius)) {
        return s.radius; // every point of the arc is as far
    }
    if (inSweep(s, std::atan2(v.y, v.x), 0.0)) {
        return std::abs(m - s.radius);
    }
    return std::min(distance(p, s.a), distance(p, s.b));
}

std::vector<RS_Vector> lineLine(const LC_OffsetSegment& s, const LC_OffsetSegment& t, const double tolerance) {
    std::vector<RS_Vector> out;
    const RS_Vector r = s.b - s.a;
    const RS_Vector q = t.b - t.a;
    const double lr = norm(r);
    const double lq = norm(q);
    const double slack = slackFor(tolerance, lr + lq + norm(s.a) + norm(t.a));
    if (!(lr > 0.0) || !(lq > 0.0)) {
        // a segment of no length meets the other where it lies on it
        if (!(lr > 0.0)) {
            if (pointLineDistance(s.a, t) <= slack) {
                out.push_back(s.a);
            }
        }
        else if (pointLineDistance(t.a, s) <= slack) {
            out.push_back(t.a);
        }
        return out;
    }
    const RS_Vector w = t.a - s.a;
    const double denominator = cross(r, q);
    if (std::abs(denominator) <= 1e-12 * lr * lq) {
        // parallel: they meet only if collinear, over the stretch they share
        if (std::abs(cross(r, w)) / lr > slack) {
            return out;
        }
        const double ta = dot(w, r) / (lr * lr);
        const double tb = dot(t.b - s.a, r) / (lr * lr);
        const double ext = slack / lr;
        const double from = std::max(0.0, std::min(ta, tb));
        const double to = std::min(1.0, std::max(ta, tb));
        if (std::min(ta, tb) > 1.0 + ext || std::max(ta, tb) < -ext) {
            return out;
        }
        const double a = std::min(from, to);
        const double b = std::max(from, to);
        out.push_back(s.a + r * a);
        if ((b - a) * lr > slack) {
            out.push_back(s.a + r * b);
        }
        return out;
    }
    const double u = cross(w, q) / denominator;
    const double v = cross(w, r) / denominator;
    if (u >= -slack / lr && u <= 1.0 + slack / lr && v >= -slack / lq && v <= 1.0 + slack / lq) {
        out.push_back(s.a + r * std::clamp(u, 0.0, 1.0));
    }
    return out;
}

std::vector<RS_Vector> lineArc(const LC_OffsetSegment& line, const LC_OffsetSegment& arc, const double tolerance) {
    std::vector<RS_Vector> out;
    const RS_Vector r = line.b - line.a;
    const double length = norm(r);
    const double radius = arc.radius;
    const double slack = slackFor(tolerance, length + radius + norm(line.a) + norm(arc.centre));
    if (!(length > 0.0)) {
        if (pointArcDistance(line.a, arc) <= slack) {
            out.push_back(line.a);
        }
        return out;
    }
    const RS_Vector dir = r / length;
    const double along = dot(arc.centre - line.a, dir);
    const RS_Vector foot = line.a + dir * along;
    const double h = distance(arc.centre, foot);
    if (h > radius + slack) {
        return out;
    }
    const double w2 = radius * radius - h * h;
    const double w = w2 > 0.0 ? std::sqrt(w2) : 0.0;
    std::vector<double> params;
    if (w <= slack) {
        params.push_back(along);
    }
    else {
        params.push_back(along - w);
        params.push_back(along + w);
    }
    const double angleSlack = radius > 0.0 ? slack / radius : kTwoPi;
    for (const double p : params) {
        if (p < -slack || p > length + slack) {
            continue;
        }
        const RS_Vector point = line.a + dir * std::clamp(p, 0.0, length);
        const RS_Vector v = point - arc.centre;
        if (inSweep(arc, std::atan2(v.y, v.x), angleSlack)) {
            out.push_back(point);
        }
    }
    return out;
}

std::vector<RS_Vector> arcArc(const LC_OffsetSegment& s, const LC_OffsetSegment& t, const double tolerance) {
    std::vector<RS_Vector> out;
    const double r1 = s.radius;
    const double r2 = t.radius;
    const RS_Vector between = t.centre - s.centre;
    const double d = norm(between);
    const double slack = slackFor(tolerance, r1 + r2 + norm(s.centre) + norm(t.centre));
    const double angleSlack1 = r1 > 0.0 ? slack / r1 : kTwoPi;
    const double angleSlack2 = r2 > 0.0 ? slack / r2 : kTwoPi;
    if (d <= slack) {
        // concentric: they meet only on one circle, over the stretches they share
        if (std::abs(r1 - r2) > slack) {
            return out;
        }
        for (const double angle : sharedAngles(s, t, angleSlack1)) {
            out.push_back(s.centre + direction(angle) * r1);
        }
        return out;
    }
    if (d > r1 + r2 + slack || d < std::abs(r1 - r2) - slack) {
        return out;
    }
    const double a = (d * d + r1 * r1 - r2 * r2) / (2.0 * d);
    const double h2 = r1 * r1 - a * a;
    const double h = h2 > 0.0 ? std::sqrt(h2) : 0.0;
    const RS_Vector u = between / d;
    const RS_Vector perp{-u.y, u.x};
    const RS_Vector base = s.centre + u * a;
    std::vector<RS_Vector> candidates;
    if (h <= slack) {
        candidates.push_back(base);
    }
    else {
        candidates.push_back(base + perp * h);
        candidates.push_back(base - perp * h);
    }
    for (const RS_Vector& p : candidates) {
        const RS_Vector v1 = p - s.centre;
        const RS_Vector v2 = p - t.centre;
        if (inSweep(s, std::atan2(v1.y, v1.x), angleSlack1) && inSweep(t, std::atan2(v2.y, v2.x), angleSlack2)) {
            out.push_back(p);
        }
    }
    return out;
}

/** Whether two arcs about one centre share a direction. */
bool sweepsOverlap(const LC_OffsetSegment& s, const LC_OffsetSegment& t) {
    return !sharedAngles(s, t, 0.0).empty();
}

// ---------------------------------------------------------------------------
// A uniform grid over segment boxes.
// ---------------------------------------------------------------------------

struct Box {
    double x0 = 0.0;
    double y0 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;

    Box grown(const double by) const {
        return {x0 - by, y0 - by, x1 + by, y1 + by};
    }

    bool meets(const Box& other) const {
        return x0 <= other.x1 && other.x0 <= x1 && y0 <= other.y1 && other.y0 <= y1;
    }
};

Box boxOf(const LC_OffsetSegment& s) {
    RS_Vector lo{false};
    RS_Vector hi{false};
    s.box(lo, hi);
    return {lo.x, lo.y, hi.x, hi.y};
}

class SegmentGrid {
public:
    /**
     * Cells no smaller than @p minCell, and sized so that there are O(n) of
     * them, each holding O(1) boxes for evenly spread segments, even when
     * the segments lie along a thin strip.
     */
    SegmentGrid(const std::vector<LC_OffsetSegment>& segments, const double minCell, std::size_t& work)
        : m_work(work) {
        const std::size_t n = segments.size();
        m_boxes.reserve(n);
        for (const LC_OffsetSegment& s : segments) {
            m_boxes.push_back(boxOf(s));
        }
        m_stamps.assign(n, 0);
        if (n == 0) {
            return;
        }
        m_bounds = m_boxes.front();
        for (const Box& b : m_boxes) {
            m_bounds.x0 = std::min(m_bounds.x0, b.x0);
            m_bounds.y0 = std::min(m_bounds.y0, b.y0);
            m_bounds.x1 = std::max(m_bounds.x1, b.x1);
            m_bounds.y1 = std::max(m_bounds.y1, b.y1);
        }
        const double width = m_bounds.x1 - m_bounds.x0;
        const double height = m_bounds.y1 - m_bounds.y0;
        const double diagonal = std::hypot(width, height);
        const auto count = static_cast<double>(n);
        m_cell = std::max({minCell, std::sqrt(width * height / count), diagonal / count});
        if (!(m_cell > 0.0) || !std::isfinite(m_cell)) {
            m_cell = 1.0;
        }
        m_nx = static_cast<std::size_t>(std::min(width / m_cell, count * 4.0)) + 1;
        m_ny = static_cast<std::size_t>(std::min(height / m_cell, count * 4.0)) + 1;
        // two passes: count the entries of each cell, then fill them
        m_start.assign(m_nx * m_ny + 1, 0);
        for (const Box& b : m_boxes) {
            forCells(b, [this](const std::size_t cell) { ++m_start[cell + 1]; });
        }
        for (std::size_t c = 0; c < m_nx * m_ny; ++c) {
            m_start[c + 1] += m_start[c];
        }
        m_entries.resize(m_start.back());
        std::vector<std::size_t> fill(m_start.begin(), m_start.end() - 1);
        for (std::size_t i = 0; i < n; ++i) {
            forCells(m_boxes[i], [&](const std::size_t cell) { m_entries[fill[cell]++] = i; });
        }
    }

    const Box& boxAt(const std::size_t i) const {
        return m_boxes[i];
    }

    /** Calls @p visit once with every segment whose box meets @p query. */
    template <typename Visit>
    void query(const Box& query, Visit&& visit) {
        if (m_boxes.empty() || !query.meets(m_bounds)) {
            return;
        }
        ++m_stamp;
        forCells(query, [&](const std::size_t cell) {
            ++m_work;
            for (std::size_t k = m_start[cell]; k < m_start[cell + 1]; ++k) {
                ++m_work;
                const std::size_t i = m_entries[k];
                if (m_stamps[i] != m_stamp) {
                    m_stamps[i] = m_stamp;
                    if (m_boxes[i].meets(query)) {
                        visit(i);
                    }
                }
            }
        });
    }

private:
    std::size_t column(const double x) const {
        const double c = std::floor((x - m_bounds.x0) / m_cell);
        return static_cast<std::size_t>(std::clamp(c, 0.0, static_cast<double>(m_nx - 1)));
    }

    std::size_t row(const double y) const {
        const double r = std::floor((y - m_bounds.y0) / m_cell);
        return static_cast<std::size_t>(std::clamp(r, 0.0, static_cast<double>(m_ny - 1)));
    }

    template <typename Each>
    void forCells(const Box& b, Each&& each) const {
        const std::size_t cx0 = column(b.x0);
        const std::size_t cx1 = column(b.x1);
        const std::size_t cy0 = row(b.y0);
        const std::size_t cy1 = row(b.y1);
        for (std::size_t y = cy0; y <= cy1; ++y) {
            for (std::size_t x = cx0; x <= cx1; ++x) {
                each(y * m_nx + x);
            }
        }
    }

    std::size_t& m_work;
    std::vector<Box> m_boxes;
    std::vector<unsigned> m_stamps;
    unsigned m_stamp = 0;
    Box m_bounds{};
    double m_cell = 1.0;
    std::size_t m_nx = 1;
    std::size_t m_ny = 1;
    std::vector<std::size_t> m_start;
    std::vector<std::size_t> m_entries;
};

/**
 * Whether two segments of @p chain meet other than where neighbours share an
 * end: any meeting of two that are not neighbours, and a meeting of
 * neighbours farther than @p tolerance from their shared ends.
 */
bool crossesItself(const std::vector<LC_OffsetSegment>& chain, const bool closed, const double tolerance,
                   std::size_t& work) {
    const std::size_t n = chain.size();
    SegmentGrid grid(chain, 0.0, work);
    for (std::size_t i = 0; i < n; ++i) {
        bool crossing = false;
        grid.query(grid.boxAt(i).grown(tolerance), [&](const std::size_t j) {
            if (crossing || j <= i) {
                return;
            }
            // the ends two neighbours share: one, or two for a closed chain of two
            std::vector<RS_Vector> joints;
            if (j == i + 1) {
                joints.push_back(chain[i].b);
            }
            if (closed && i == 0 && j == n - 1) {
                joints.push_back(chain[j].b);
            }
            for (const RS_Vector& p : segmentIntersections(chain[i], chain[j], 0.0)) {
                const bool atJoint = std::any_of(joints.begin(), joints.end(), [&](const RS_Vector& joint) {
                    return distance(p, joint) <= tolerance;
                });
                if (!atJoint) {
                    crossing = true;
                    return;
                }
            }
        });
        if (crossing) {
            return true;
        }
    }
    return false;
}

/** The least distance from @p p to @p source within @p reach; infinity when none is. */
double distanceToSource(const RS_Vector& p, const std::vector<LC_OffsetSegment>& source, SegmentGrid& grid,
                        const double reach) {
    double least = kInfinity;
    grid.query(Box{p.x, p.y, p.x, p.y}.grown(reach),
               [&](const std::size_t j) { least = std::min(least, pointSegmentDistance(p, source[j])); });
    return least;
}

/** Ends, quarters and middle, and a point every 30 degrees of an arc. */
std::vector<RS_Vector> samplesOf(const LC_OffsetSegment& s) {
    std::vector<double> us{0.0, 0.25, 0.5, 0.75, 1.0};
    if (s.arc) {
        const int steps = static_cast<int>(std::ceil(std::abs(s.sweep) / (M_PI / 6.0)));
        for (int k = 1; k < steps; ++k) {
            us.push_back(static_cast<double>(k) / steps);
        }
    }
    std::vector<RS_Vector> points;
    points.reserve(us.size());
    for (const double u : us) {
        points.push_back(s.pointAt(u));
    }
    return points;
}

} // namespace

double LC_OffsetSegment::length() const {
    return arc ? radius * std::abs(sweep) : distance(a, b);
}

RS_Vector LC_OffsetSegment::pointAt(const double u) const {
    if (u <= 0.0) {
        return a;
    }
    if (u >= 1.0) {
        return b;
    }
    if (!arc) {
        return a + (b - a) * u;
    }
    return centre + direction(startAngle + sweep * u) * radius;
}

void LC_OffsetSegment::box(RS_Vector& lo, RS_Vector& hi) const {
    lo = RS_Vector{std::min(a.x, b.x), std::min(a.y, b.y)};
    hi = RS_Vector{std::max(a.x, b.x), std::max(a.y, b.y)};
    if (!arc) {
        return;
    }
    for (int k = 0; k < 4; ++k) {
        const double angle = k * M_PI_2;
        if (inSweep(*this, angle, 0.0)) {
            const RS_Vector p = centre + direction(angle) * radius;
            lo = RS_Vector{std::min(lo.x, p.x), std::min(lo.y, p.y)};
            hi = RS_Vector{std::max(hi.x, p.x), std::max(hi.y, p.y)};
        }
    }
}

LC_OffsetSegment LC_OffsetSegment::reversed() const {
    LC_OffsetSegment r = *this;
    std::swap(r.a, r.b);
    if (arc) {
        r.startAngle = startAngle + sweep;
        r.sweep = -sweep;
    }
    return r;
}

bool makeOffsetSegment(const RS_Entity& child, LC_OffsetSegment& segment) {
    segment = LC_OffsetSegment{};
    if (child.rtti() == RS2::EntityLine) {
        segment.a = child.getStartpoint();
        segment.b = child.getEndpoint();
        return true;
    }
    if (child.rtti() == RS2::EntityArc) {
        const auto& arc = static_cast<const RS_Arc&>(child);
        segment.arc = true;
        segment.a = arc.getStartpoint();
        segment.b = arc.getEndpoint();
        segment.centre = arc.getCenter();
        segment.radius = arc.getRadius();
        segment.startAngle = arc.getAngle1();
        const double span = arc.getAngleLength();
        segment.sweep = arc.isReversed() ? -span : span;
        return true;
    }
    return false;
}

std::vector<LC_OffsetSegment> storedSegments(const RS_Polyline& polyline) {
    std::vector<LC_OffsetSegment> segments;
    for (const RS_Entity* child : polyline) {
        LC_OffsetSegment s;
        if (child == nullptr || !makeOffsetSegment(*child, s)) {
            return {};
        }
        segments.push_back(s);
    }
    return segments;
}

std::vector<LC_OffsetSegment> orientedSegments(const RS_Polyline& polyline) {
    std::vector<LC_OffsetSegment> segments = storedSegments(polyline);
    const auto nearestEnd = [](const RS_Vector& v, const LC_OffsetSegment& s) {
        return std::min(v.distanceTo(s.a), v.distanceTo(s.b));
    };
    // as RS_Polyline::offset() turns its children, head to tail
    if (segments.size() > 1) {
        if (nearestEnd(segments[0].a, segments[1]) < nearestEnd(segments[0].b, segments[1])) {
            segments[0] = segments[0].reversed();
        }
        for (std::size_t i = 1; i < segments.size(); ++i) {
            if (nearestEnd(segments[i].a, segments[i - 1]) > nearestEnd(segments[i].b, segments[i - 1])) {
                segments[i] = segments[i].reversed();
            }
        }
    }
    return segments;
}

double pointSegmentDistance(const RS_Vector& p, const LC_OffsetSegment& s) {
    return s.arc ? pointArcDistance(p, s) : pointLineDistance(p, s);
}

std::vector<RS_Vector> segmentIntersections(const LC_OffsetSegment& s, const LC_OffsetSegment& t,
                                            const double tolerance) {
    if (!s.arc && !t.arc) {
        return lineLine(s, t, tolerance);
    }
    if (!s.arc) {
        return lineArc(s, t, tolerance);
    }
    if (!t.arc) {
        return lineArc(t, s, tolerance);
    }
    return arcArc(s, t, tolerance);
}

bool segmentsIntersect(const LC_OffsetSegment& s, const LC_OffsetSegment& t, const double tolerance) {
    return !segmentIntersections(s, t, tolerance).empty();
}

double segmentDistance(const LC_OffsetSegment& s, const LC_OffsetSegment& t) {
    if (segmentsIntersect(s, t, 0.0)) {
        return 0.0;
    }
    double best = std::min({pointSegmentDistance(s.a, t), pointSegmentDistance(s.b, t), pointSegmentDistance(t.a, s),
                            pointSegmentDistance(t.b, s)});
    if (!s.arc && !t.arc) {
        return best;
    }
    if (s.arc && t.arc) {
        const RS_Vector between = t.centre - s.centre;
        const double d = norm(between);
        if (d <= 1e-15 * std::max({1.0, s.radius, t.radius})) {
            if (sweepsOverlap(s, t)) {
                best = std::min(best, std::abs(s.radius - t.radius));
            }
            return best;
        }
        // nearest points in between lie on the line of the centres
        const double phi = std::atan2(between.y, between.x);
        for (const double a1 : {phi, phi + M_PI}) {
            for (const double a2 : {phi, phi + M_PI}) {
                if (inSweep(s, a1, 0.0) && inSweep(t, a2, 0.0)) {
                    const RS_Vector p = s.centre + direction(a1) * s.radius;
                    const RS_Vector q = t.centre + direction(a2) * t.radius;
                    best = std::min(best, distance(p, q));
                }
            }
        }
        return best;
    }
    const LC_OffsetSegment& line = s.arc ? t : s;
    const LC_OffsetSegment& arc = s.arc ? s : t;
    const RS_Vector r = line.b - line.a;
    const double length = norm(r);
    if (!(length > 0.0)) {
        return best;
    }
    const RS_Vector dir = r / length;
    const double along = dot(arc.centre - line.a, dir);
    if (along < 0.0 || along > length) {
        return best;
    }
    // the foot of the centre on the line, and the arc's point towards it
    const RS_Vector foot = line.a + dir * along;
    const RS_Vector off = foot - arc.centre;
    const double h = norm(off);
    if (h > 1e-15 * std::max(1.0, arc.radius)) {
        if (inSweep(arc, std::atan2(off.y, off.x), 0.0)) {
            best = std::min(best, std::abs(h - arc.radius));
        }
    }
    else {
        const double normal = std::atan2(dir.y, dir.x) + M_PI_2;
        if (inSweep(arc, normal, 0.0) || inSweep(arc, normal + M_PI, 0.0)) {
            best = std::min(best, arc.radius);
        }
    }
    return best;
}

double signedArea(const std::vector<LC_OffsetSegment>& chain) {
    if (chain.empty()) {
        return 0.0;
    }
    const RS_Vector origin = chain.front().a;
    double area = 0.0;
    for (const LC_OffsetSegment& s : chain) {
        area += 0.5 * cross(s.a - origin, s.b - origin);
        if (s.arc) {
            // the circular segment between the chord and the arc
            area += 0.5 * s.radius * s.radius * (s.sweep - std::sin(s.sweep));
        }
    }
    return area;
}

double chainLength(const std::vector<LC_OffsetSegment>& chain) {
    double length = 0.0;
    for (const LC_OffsetSegment& s : chain) {
        length += s.length();
    }
    return length;
}

LC_OffsetCheckReport checkLegacyPolylineOffset(const RS_Polyline& source, const RS_Polyline& offset,
                                               const double distanceValue) {
    LC_OffsetCheckReport report;
    report.nearest = kInfinity;
    const double d = std::abs(distanceValue);
    const std::vector<LC_OffsetSegment> src = orientedSegments(source);
    const std::vector<LC_OffsetSegment> res = storedSegments(offset);
    if (src.empty() || res.empty() || !std::isfinite(d)) {
        report.broken = true;
        report.verdict = LC_OffsetCheckVerdict::Invalid;
        return report;
    }
    RS_Vector lo{false};
    RS_Vector hi{false};
    src.front().box(lo, hi);
    for (const LC_OffsetSegment& s : src) {
        RS_Vector a{false};
        RS_Vector b{false};
        s.box(a, b);
        lo = RS_Vector{std::min(lo.x, a.x), std::min(lo.y, a.y)};
        hi = RS_Vector{std::max(hi.x, b.x), std::max(hi.y, b.y)};
    }
    const double tolerance = 1e-6 * std::max(distance(lo, hi), d);
    const bool closed = source.isClosed();
    const std::size_t n = res.size();

    // broken: a child of no length, or a gap between neighbours
    report.broken = res.size() != src.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (res[i].length() < tolerance) {
            report.broken = true;
        }
        const bool last = i + 1 == n;
        if (!last || closed) {
            const LC_OffsetSegment& next = res[last ? 0 : i + 1];
            if (distance(res[i].b, next.a) > tolerance) {
                report.broken = true;
            }
        }
    }

    // reversed: against its source child's direction, or an arc wrapped or not moved by the distance
    for (std::size_t i = 0; i < std::min(n, src.size()); ++i) {
        const LC_OffsetSegment& s = src[i];
        const LC_OffsetSegment& r = res[i];
        if (s.arc != r.arc) {
            report.reversed = true;
            continue;
        }
        if (!r.arc) {
            if (dot(r.b - r.a, s.b - s.a) <= 0.0) {
                report.reversed = true;
            }
            continue;
        }
        if ((r.sweep > 0.0) != (s.sweep > 0.0) || std::abs(r.sweep) - std::abs(s.sweep) > M_PI ||
            std::abs(std::abs(r.radius - s.radius) - d) > tolerance) {
            report.reversed = true;
        }
    }

    // near: the exact least distance to the source, from the source children within reach
    SegmentGrid sourceGrid(src, d, report.cellsVisited);
    for (const LC_OffsetSegment& r : res) {
        sourceGrid.query(boxOf(r).grown(d), [&](const std::size_t j) {
            report.nearest = std::min(report.nearest, segmentDistance(r, src[j]));
        });
    }
    report.near = report.nearest < d - tolerance;

    // crossing: children of the offset that meet other than at a shared end
    report.crossing = crossesItself(res, closed, tolerance, report.cellsVisited);

    // collapsed: a closed offset inverted, or with next to no area, of a simple closed source with area
    if (closed) {
        const double sourceArea = signedArea(src);
        if (std::abs(sourceArea) > tolerance * chainLength(src) &&
            !crossesItself(src, true, tolerance, report.cellsVisited)) {
            const double area = signedArea(res);
            report.collapsed = area * sourceArea < 0.0 || std::abs(area) <= tolerance * chainLength(res);
        }
    }

    // entirely near: only chooses between the verdicts
    if (report.near) {
        report.entirelyNear = true;
        for (std::size_t i = 0; i < n && report.entirelyNear; ++i) {
            for (const RS_Vector& p : samplesOf(res[i])) {
                if (!(distanceToSource(p, src, sourceGrid, d) < d - tolerance)) {
                    report.entirelyNear = false;
                    break;
                }
            }
        }
    }

    if (report.collapsed || report.entirelyNear) {
        report.verdict = LC_OffsetCheckVerdict::NothingLeft;
    }
    else if (report.broken || report.reversed || report.crossing || report.near) {
        report.verdict = LC_OffsetCheckVerdict::Invalid;
    }
    else {
        report.verdict = LC_OffsetCheckVerdict::Valid;
    }
    return report;
}
