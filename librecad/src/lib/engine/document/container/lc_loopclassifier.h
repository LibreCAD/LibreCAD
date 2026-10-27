/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2026 librecad (www.librecad.org)
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

#ifndef LC_LOOPCLASSIFIER_H
#define LC_LOOPCLASSIFIER_H

#include <functional>
#include <memory>
#include <vector>

#include "rs_vector.h"

class RS_Entity;
class RS_EntityContainer;

namespace lc::geo {
    class Area;
}

using LC_Rect = lc::geo::Area;

namespace LC_LoopUtils {

/**
 * Textbook even-odd point classification for one or more oriented or
 * unoriented edge loops. Every edge is split into y-monotone pieces with
 * closed-form xAt(y); the half-open ray y = p.y + 0 crosses a piece whose
 * ylo <= p.y < yhi, so horizontal pieces never count.
 *
 * The library is used by two callers:
 *   - RS_Information::isPointInsideContour (unoriented contour selection);
 *   - LC_Loops::classify (chain-of-edges loop).
 */

enum class PointClass {
    Outside,
    Inside,
    On
};

/**
 * The one-per-hatch tolerance. D is the bbox diagonal of all boundary edges
 * (include closed-curve bboxes; closed curves have no valid endpoints), M is
 * the largest |x| or |y| of the endpoints only. u = DBL_EPSILON / 2 = 2^-53.
 *
 *   epsilon = max(64*u*M, 1e-9*D, min(1e-8, 1e-6*D))
 *
 * The floor min(1e-8, 1e-6*D) keeps the tolerance usable in small-coordinate
 * frames; the 64*u*M term keeps it finite for far-off-origin geometry.
 * D must be nonnegative; non-finite inputs return a non-finite result that
 * callers must guard before dividing by eps.
 */
double hatchTolerance(double diagonal, double maxAbsCoordinate);

/**
 * One y-monotone piece. The piece covers y in [ylo, yhi] (closed) with
 * ylo = a.y and yhi = b.y, a.y <= b.y guaranteed by appendMonotonePieces().
 *
 * xAt(y) returns the x-coordinate of the piece at y, clamped to [ylo, yhi].
 * The piece carries its own kind-specific parameters; the clamp means a
 * near-miss ray at a vertex ends up at the vertex x, which keeps parity
 * sampling stable across y = vertex.y.
 */
struct MonoPiece {
    enum class Kind {
        Line,
        Arc,
        Ellipse,
        Quadratic,     // LC_SplinePoints quadratic segment or LC_Parabola
        Hyperbola,
        Generic        // bisection fallback
    };

    Kind kind = Kind::Line;
    RS_Vector a{};              // low-y end (y = ylo)
    RS_Vector b{};              // high-y end (y = yhi)
    double ylo = 0.0;
    double yhi = 0.0;

    // Arc / Circle: center (cx, cy), radius r, side s = +1 (right half) or -1 (left half).
    double cx = 0.0;
    double cy = 0.0;
    double r = 0.0;
    double s = 1.0;

    // Ellipse arc: parameters that let us invert y(t) = cy + R*cos(t-alpha).
    //    center (cx, cy), M (Mx, My), N (Nx, Ny), R = hypot(My, Ny), alpha = atan2(Ny, My).
    //    half = 0 -> t in [alpha + 2*pi*m, alpha + 2*pi*m + pi], t = alpha + 2*pi*m + acos(v)
    //    half = 1 -> t in [alpha + 2*pi*m + pi, alpha + 2*pi*m + 2*pi], t = alpha + 2*pi*m + 2*pi - acos(v)
    double mx = 0.0;
    double my = 0.0;
    double nx = 0.0;
    double ny = 0.0;
    double majorAngle = 0.0;    // alpha
    double R = 0.0;
    double tLo = 0.0;           // parameter at the piece's lower-y end (not the lower parameter)
    double tHi = 0.0;
    int half = 0;

    // Quadratic Bezier (LC_SplinePoints / LC_Parabola). y0,y1,y2 pack the
    // control polygon's y; the x triple is x0,x1,x2. w ranges over [wLo, wHi].
    double qx0 = 0.0, qx1 = 0.0, qx2 = 0.0;
    double qy0 = 0.0, qy1 = 0.0, qy2 = 0.0;
    double wLo = 0.0;
    double wHi = 1.0;

    // Hyperbola: center, side sx = reversed ? -1 : +1, axis angle th, semiAxes a,b.
    //    y(phi) = cy + A*cosh(phi) + B*sinh(phi), A = sx*a*sin(th), B = b*cos(th).
    double hA = 0.0;
    double hB = 0.0;
    double hCX = 0.0;
    double hCY = 0.0;
    double hSx = 1.0;
    double hTh = 0.0;
    double hSemiA = 0.0;
    double hSemiB = 0.0;
    double phiLo = 0.0;
    double phiHi = 0.0;

    // Generic fallback: cached sample ys to bisect on. Only used when
    // forceGeneric is true, or for an entity whose closed form is not
    // supported.
    double gLo = 0.0;
    double gHi = 0.0;
    const RS_Entity* genericEdge = nullptr;

    double xAt(double y) const;
};

/**
 * Appends the y-monotone pieces of edge between its own endpoints va and vb
 * to out. va, vb come from a per-loop vertex table so that pieces at loop
 * vertices snap to exactly the same (x, y) across adjacent edges; for a
 * closed atomic entity (circle, full ellipse, closed LC_SplinePoints) va and
 * vb are ignored and the piece ends come from the curve itself.
 *
 * forceGeneric = true forces the bisection-fallback kind for every piece
 * except RS_Line, so a unit test can exercise it.
 */
void appendMonotonePieces(const RS_Entity& edge,
                          const RS_Vector& va,
                          const RS_Vector& vb,
                          std::vector<MonoPiece>& out,
                          bool forceGeneric = false);

/**
 * Union-find weld of coincident vertices.
 *
 * For each pair of points within eps, their indices share a representative.
 * The representative is the lexicographically smallest member. looseTolerance
 * (optional, one per point) widens the matching radius for a specific point;
 * it is intended for arc-junction pairs where a chord's two near-flat ends
 * are allowed a slightly larger joining tolerance.
 *
 * longChains counts clusters whose diameter > 2*eps; owners inspect it and
 * either reject the input or route it to a checked tile fallback.
 */
struct WeldResult {
    std::vector<int> id;            // id[i] = representative index of point i
    std::vector<RS_Vector> rep;     // rep[r] = coordinate of representative r
    int longChains = 0;
};

WeldResult weldPoints(const std::vector<RS_Vector>& points,
                      double eps,
                      const std::vector<double>& looseTolerance = {});

/**
 * Immutable per-loop geometry: welded vertex table, y-monotone pieces, bbox,
 * and a crossingsRight(point) that counts half-open crossings of the ray
 * y = point.y, x > point.x.
 */
class LoopGeometry {
public:
    LoopGeometry() = default;

    /**
     * Build from a loop (a chain of atomic edges around a closed region).
     * vertices.size() must equal the number of non-closed edges and vertex[i]
     * is edge i's start. For a container of one closed atomic entity, pass an
     * empty vertices vector.
     */
    static std::shared_ptr<const LoopGeometry> fromChain(
        const RS_EntityContainer& loop,
        std::vector<RS_Vector> vertices);

    /**
     * Build from an unordered selection of edges. Edge ends are welded at
     * weldTolerance, which is 1e-4 in isPointInsideContour (inference, not a
     * tested result).
     */
    static std::shared_ptr<const LoopGeometry> fromContour(
        const std::vector<RS_Entity*>& edges,
        double weldTolerance);

    const std::vector<RS_Vector>& vertices() const { return m_vertices; }
    const std::vector<MonoPiece>& pieces() const { return m_pieces; }
    LC_Rect box() const;
    int crossingsRight(const RS_Vector& p) const;

private:
    std::vector<RS_Vector> m_vertices;
    std::vector<MonoPiece> m_pieces;
    double m_xmin = 0.0, m_xmax = 0.0, m_ymin = 0.0, m_ymax = 0.0;
    bool m_hasBox = false;
};

/**
 * Half-open crossing-parity test. loops are a flat list; a point is inside
 * the union of loops iff the total number of right-ray crossings over all
 * loops is odd. For a single-loop fromChain input this matches the standard
 * even-odd rule; for the fromContour of an unordered selection it is the
 * same because every crossed edge counts once.
 */
bool oddParity(const RS_Vector& point,
               const std::vector<const LoopGeometry*>& loops);

/**
 * Returns true if any resolved atomic entity's bbox +- epsOn contains p and
 * its getDistanceToPoint(p) <= epsOn. Used by classifyPoint and by
 * isPointInsideContour's ON band.
 */
bool isNearAnyEdge(const RS_Vector& point,
                   const std::vector<const RS_EntityContainer*>& contours,
                   double epsOn);

/**
 * ON if some atomic edge is within epsOn of point, else Inside/Outside by
 * oddParity over loops.
 */
PointClass classifyPoint(const RS_Vector& point,
                         const std::vector<const RS_EntityContainer*>& contours,
                         const std::vector<const LoopGeometry*>& loops,
                         double epsOn);

/**
 * A trimmer-side point classifier. The piece's own container(s) and
 * geometry/epsOn are captured by the lambda; a fresh classifier is built per
 * trim call.
 */
using PointClassifier = std::function<PointClass(const RS_Vector&)>;

/**
 * Collinear-safe keep rule for one pattern piece.
 *
 * The piece is classified at a representative point m (midpoint for lines and
 * open curves; centre + (r, 0) for circles; the point itself for RS_Point).
 * If m is strictly Inside/Outside the trimmer keeps/drops it; if m is On the
 * piece is reclassified at m displaced by 2*epsOn along the +90-degree normal
 * of its canonical direction (angle reduced to [0, pi) for lines; the inward
 * normal for arcs/circles; (0, 1) for RS_Point). This picks one of two
 * coincident tile edges deterministically: for a 0-degree family the bottom
 * is drawn and the top is not; for 90 degrees the right is drawn and the
 * left is not.
 */
bool keepPiece(const RS_Entity& piece,
               const PointClassifier& classify,
               double epsOn);

} // namespace LC_LoopUtils

#endif // LC_LOOPCLASSIFIER_H
