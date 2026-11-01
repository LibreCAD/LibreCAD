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
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
****************************************************************************/

#include "lc_patternlinededup.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "rs_vector.h"

namespace LC_LoopUtils {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Lexicographic order on (x, y) end points.
bool lexLess(const RS_Vector& a, const RS_Vector& b) {
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

struct DirectedPiece {
    double theta;   // canonical direction in [0, pi); may be shifted to < 0 on wrap-around.
    RS_LineData data;
};

struct DirSortKey {
    bool operator()(const DirectedPiece& a, const DirectedPiece& b) const {
        if (a.theta != b.theta) return a.theta < b.theta;
        if (a.data.startpoint.x != b.data.startpoint.x)
            return a.data.startpoint.x < b.data.startpoint.x;
        if (a.data.startpoint.y != b.data.startpoint.y)
            return a.data.startpoint.y < b.data.startpoint.y;
        if (a.data.endpoint.x != b.data.endpoint.x)
            return a.data.endpoint.x < b.data.endpoint.x;
        return a.data.endpoint.y < b.data.endpoint.y;
    }
};

struct Row {
    double c;    // signed perpendicular offset.
    double t0;   // smaller tangential coordinate.
    double t1;   // larger tangential coordinate.
    RS_Vector p0;   // the input piece's endpoint at the lower t.
    RS_Vector p1;   // the input piece's endpoint at the higher t.
};

struct RowSortKey {
    bool operator()(const Row& a, const Row& b) const {
        if (a.c != b.c) return a.c < b.c;
        if (a.t0 != b.t0) return a.t0 < b.t0;
        return a.t1 < b.t1;
    }
};

struct Interval {
    double t0;
    double t1;
    RS_Vector p0;        // keeps the lo-t endpoint of the piece that currently owns t0.
    RS_Vector p1;        // keeps the hi-t endpoint of the piece that currently owns t1.
    bool merged = false; // becomes true once this interval absorbed another piece.
};

bool intervalLess(const Interval& a, const Interval& b) {
    if (a.t0 != b.t0) return a.t0 < b.t0;
    return a.t1 < b.t1;
}

// Shortest perpendicular distance from point to the closed segment [a, b].
double distancePointSegment(const RS_Vector& p, const RS_Vector& a, const RS_Vector& b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double l2 = dx * dx + dy * dy;
    if (l2 == 0.0) {
        const double qx = p.x - a.x;
        const double qy = p.y - a.y;
        return std::sqrt(qx * qx + qy * qy);
    }
    double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / l2;
    if (t < 0.0) t = 0.0;
    else if (t > 1.0) t = 1.0;
    const double qx = p.x - (a.x + t * dx);
    const double qy = p.y - (a.y + t * dy);
    return std::sqrt(qx * qx + qy * qy);
}

struct CellKey {
    long long x;
    long long y;
    bool operator==(const CellKey& o) const noexcept { return x == o.x && y == o.y; }
};

struct CellKeyHash {
    std::size_t operator()(const CellKey& k) const noexcept {
        // 64-bit FNV-1a over the two coordinates (keyed independently).
        std::uint64_t h = 1469598103934665603ULL;
        auto mix = [&](std::uint64_t v) {
            for (int i = 0; i < 8; ++i) {
                h ^= v & 0xffULL;
                h *= 1099511628211ULL;
                v >>= 8;
            }
        };
        mix(static_cast<std::uint64_t>(k.x));
        mix(static_cast<std::uint64_t>(k.y));
        return static_cast<std::size_t>(h);
    }
};

}  // namespace

std::vector<RS_LineData> deduplicatePatternLines(
    std::vector<RS_LineData> pieces,
    double tolOffset,
    double tolAngle) {
    if (!(tolOffset > 0.0) || !(tolAngle > 0.0) || pieces.empty()) {
        return pieces;
    }

    std::vector<DirectedPiece> lines;
    lines.reserve(pieces.size());
    std::vector<RS_LineData> shorts;

    // Step 1: canonicalise and split into long "lines" and "shorts".
    for (RS_LineData& p : pieces) {
        if (lexLess(p.endpoint, p.startpoint)) {
            std::swap(p.startpoint, p.endpoint);
        }
        const double dx = p.endpoint.x - p.startpoint.x;
        const double dy = p.endpoint.y - p.startpoint.y;
        const double length = std::sqrt(dx * dx + dy * dy);
        if (!(length > tolOffset)) {
            shorts.push_back(p);
            continue;
        }
        double theta = std::atan2(dy, dx);
        if (theta < 0.0) theta += kPi;
        if (theta >= kPi) theta -= kPi;
        lines.push_back({theta, p});
    }

    // Step 2: sort by (theta, start, end); cluster within tolAngle; wrap-around.
    std::sort(lines.begin(), lines.end(), DirSortKey{});

    std::vector<std::vector<DirectedPiece>> clusters;
    for (DirectedPiece& r : lines) {
        if (!clusters.empty() && r.theta - clusters.back().back().theta <= tolAngle) {
            clusters.back().push_back(std::move(r));
        } else {
            clusters.push_back({std::move(r)});
        }
    }
    // Wrap-around: a cluster near theta = pi joins the one near theta = 0.
    if (clusters.size() >= 2) {
        const double first = clusters.front().front().theta;
        const double last = clusters.back().back().theta;
        if (first + kPi - last <= tolAngle) {
            std::vector<DirectedPiece> wrap = std::move(clusters.back());
            clusters.pop_back();
            for (DirectedPiece& r : wrap) r.theta -= kPi;
            clusters.front().insert(clusters.front().begin(),
                                    std::make_move_iterator(wrap.begin()),
                                    std::make_move_iterator(wrap.end()));
        }
    }

    std::vector<RS_LineData> merged;
    merged.reserve(lines.size());

    // Step 3: within each cluster, group by signed offset c and merge intervals.
    for (const std::vector<DirectedPiece>& cluster : clusters) {
        if (cluster.empty()) continue;
        double sumTheta = 0.0;
        for (const DirectedPiece& r : cluster) sumTheta += r.theta;
        const double thetaMean = sumTheta / static_cast<double>(cluster.size());
        const double ux = std::cos(thetaMean);
        const double uy = std::sin(thetaMean);
        const double nx = -uy;
        const double ny = ux;

        std::vector<Row> rows;
        rows.reserve(cluster.size());
        for (const DirectedPiece& r : cluster) {
            const RS_Vector& a = r.data.startpoint;
            const RS_Vector& b = r.data.endpoint;
            const double c = 0.5 * ((a.x + b.x) * nx + (a.y + b.y) * ny);
            double t0 = a.x * ux + a.y * uy;
            double t1 = b.x * ux + b.y * uy;
            RS_Vector p0 = a;
            RS_Vector p1 = b;
            if (t1 < t0) { std::swap(t0, t1); std::swap(p0, p1); }
            rows.push_back({c, t0, t1, p0, p1});
        }
        std::sort(rows.begin(), rows.end(), RowSortKey{});

        // Group by c with tolOffset single-linkage.
        std::vector<std::vector<Row>> groups;
        for (const Row& r : rows) {
            if (!groups.empty() && r.c - groups.back().back().c <= tolOffset) {
                groups.back().push_back(r);
            } else {
                groups.push_back({r});
            }
        }
        for (const std::vector<Row>& group : groups) {
            if (group.empty()) continue;
            std::vector<Interval> ivs;
            ivs.reserve(group.size());
            for (const Row& r : group) ivs.push_back({r.t0, r.t1, r.p0, r.p1, false});
            std::sort(ivs.begin(), ivs.end(), intervalLess);
            // Emit each (possibly extended) interval using the original pieces'
            // endpoint values at the extremes. The direction u is held in the
            // cluster, but the per-piece c may differ by a tolOffset: using the
            // original endpoints keeps a lone or extreme piece bit-exact and
            // prevents merged pieces from drifting outside the hatch boundary.
            auto emitInterval = [&](const Interval& iv) {
                merged.push_back(RS_LineData(iv.p0, iv.p1));
            };
            Interval cur = ivs.front();
            for (std::size_t k = 1; k < ivs.size(); ++k) {
                const Interval& next = ivs[k];
                if (next.t0 <= cur.t1 + tolOffset) {
                    if (next.t1 > cur.t1) {
                        cur.t1 = next.t1;
                        cur.p1 = next.p1;
                    }
                    cur.merged = true;
                } else {
                    emitInterval(cur);
                    cur = next;
                }
            }
            emitInterval(cur);
        }
    }

    // Step 4: shorts. Drop any whose midpoint falls within tolOffset of a merged
    // segment; de-duplicate the rest on a tolOffset grid with 3x3 neighbour
    // lookup. Preserve the original short piece: a zero-length RS_Line is
    // degenerate elsewhere, so we never collapse a sliver into a point.
    std::vector<RS_LineData> keptShorts;
    if (!shorts.empty()) {
        std::vector<std::pair<RS_Vector, std::size_t>> midsIndexed;
        midsIndexed.reserve(shorts.size());
        for (std::size_t i = 0; i < shorts.size(); ++i) {
            const RS_LineData& s = shorts[i];
            midsIndexed.push_back({
                RS_Vector{0.5 * (s.startpoint.x + s.endpoint.x),
                          0.5 * (s.startpoint.y + s.endpoint.y)},
                i});
        }

        // Drop shorts near any merged segment. Merged counts are small
        // (hundreds to a few thousand per hatch), so an O(shorts x merged)
        // brute-force scan is faster and simpler than a grid that would need
        // one bucket per tolOffset step along a long diagonal segment.
        if (!merged.empty()) {
            std::vector<std::pair<RS_Vector, std::size_t>> survivors;
            survivors.reserve(midsIndexed.size());
            for (const auto& mi : midsIndexed) {
                const RS_Vector& mid = mi.first;
                bool near = false;
                for (const RS_LineData& m : merged) {
                    if (distancePointSegment(mid, m.startpoint, m.endpoint) <= tolOffset) {
                        near = true;
                        break;
                    }
                }
                if (!near) survivors.push_back(mi);
            }
            midsIndexed = std::move(survivors);
        }

        // Sort survivors by midpoint, then de-duplicate on a tolOffset grid.
        std::sort(midsIndexed.begin(), midsIndexed.end(),
                  [](const std::pair<RS_Vector, std::size_t>& a,
                     const std::pair<RS_Vector, std::size_t>& b) {
                      if (a.first.x != b.first.x) return a.first.x < b.first.x;
                      return a.first.y < b.first.y;
                  });
        std::unordered_set<CellKey, CellKeyHash> seen;
        const double limit = 1e15;
        for (const auto& mi : midsIndexed) {
            const RS_Vector& mid = mi.first;
            const double sx = mid.x / tolOffset;
            const double sy = mid.y / tolOffset;
            if (!(std::abs(sx) <= limit) || !(std::abs(sy) <= limit)) {
                // Out-of-range midpoints bypass the key grid; keep as is.
                keptShorts.push_back(shorts[mi.second]);
                continue;
            }
            const long long kx = static_cast<long long>(std::llround(sx));
            const long long ky = static_cast<long long>(std::llround(sy));
            bool taken = false;
            for (long long dx = -1; dx <= 1 && !taken; ++dx) {
                for (long long dy = -1; dy <= 1 && !taken; ++dy) {
                    if (seen.find({kx + dx, ky + dy}) != seen.end()) taken = true;
                }
            }
            if (taken) continue;
            seen.insert({kx, ky});
            keptShorts.push_back(shorts[mi.second]);
        }
    }

    for (const RS_LineData& s : keptShorts) merged.push_back(s);
    return merged;
}

}  // namespace LC_LoopUtils
