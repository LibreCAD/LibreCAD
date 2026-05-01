/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li github.com/dxli
**
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <cmath>
#include <memory>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QImage>
#include <QPainter>

#include "lc_graphicviewport.h"
#include "lc_looputils.h"
#include "lc_secondmoment.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_vector.h"

namespace {

/**
 * Build a solid hatch whose single boundary loop is a CCW-wound rectangle
 * from (x1, y1) to (x2, y2).
 *
 * Ownership: caller takes ownership of the returned pointer.
 */
RS_Hatch* makeRectHatch(double x1, double y1, double x2, double y2)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));

    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);

    // CCW winding: bottom → right → top → left
    loop->addEntity(new RS_Line(loop, RS_Vector(x1, y1), RS_Vector(x2, y1)));
    loop->addEntity(new RS_Line(loop, RS_Vector(x2, y1), RS_Vector(x2, y2)));
    loop->addEntity(new RS_Line(loop, RS_Vector(x2, y2), RS_Vector(x1, y2)));
    loop->addEntity(new RS_Line(loop, RS_Vector(x1, y2), RS_Vector(x1, y1)));

    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose single boundary loop is a full circle
 * centered at (cx, cy) with the given radius.
 *
 * Ownership: caller takes ownership of the returned pointer.
 */
RS_Hatch* makeCircleHatch(double cx, double cy, double radius)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));

    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    loop->addEntity(new RS_Circle(loop, RS_CircleData(RS_Vector(cx, cy), radius)));

    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch with two circular boundary loops (annulus):
 *   - outer circle: center (cx,cy), radius r_outer
 *   - inner circle: center (cx,cy), radius r_inner  (hole)
 *
 * Analytical area = π (r_outer² - r_inner²)
 * Centroid = (cx, cy)
 *
 * Ownership: caller takes ownership of the returned pointer.
 */
RS_Hatch* makeConcentricCircleHatch(double cx, double cy, double r_outer, double r_inner)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));

    auto* outer = new RS_EntityContainer(hatch);
    hatch->addEntity(outer);
    outer->addEntity(new RS_Circle(outer, RS_CircleData(RS_Vector(cx, cy), r_outer)));

    auto* inner = new RS_EntityContainer(hatch);
    hatch->addEntity(inner);
    inner->addEntity(new RS_Circle(inner, RS_CircleData(RS_Vector(cx, cy), r_inner)));

    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch with two boundary loops:
 *   - outer CCW rectangle from (ox1,oy1) to (ox2,oy2)
 *   - inner CW rectangle from (ix1,iy1) to (ix2,iy2)  (hole)
 *
 * The LoopOptimizer detects the inner loop as a hole from its CW winding
 * and nesting inside the outer boundary.
 *
 * Ownership: caller takes ownership of the returned pointer.
 */
RS_Hatch* makeAnnularRectHatch(double ox1, double oy1, double ox2, double oy2,
                                double ix1, double iy1, double ix2, double iy2)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));

    // Outer loop: CCW
    auto* outer = new RS_EntityContainer(hatch);
    hatch->addEntity(outer);
    outer->addEntity(new RS_Line(outer, RS_Vector(ox1, oy1), RS_Vector(ox2, oy1)));
    outer->addEntity(new RS_Line(outer, RS_Vector(ox2, oy1), RS_Vector(ox2, oy2)));
    outer->addEntity(new RS_Line(outer, RS_Vector(ox2, oy2), RS_Vector(ox1, oy2)));
    outer->addEntity(new RS_Line(outer, RS_Vector(ox1, oy2), RS_Vector(ox1, oy1)));

    // Inner hole: CW (reverse traversal)
    auto* inner = new RS_EntityContainer(hatch);
    hatch->addEntity(inner);
    inner->addEntity(new RS_Line(inner, RS_Vector(ix1, iy1), RS_Vector(ix1, iy2)));
    inner->addEntity(new RS_Line(inner, RS_Vector(ix1, iy2), RS_Vector(ix2, iy2)));
    inner->addEntity(new RS_Line(inner, RS_Vector(ix2, iy2), RS_Vector(ix2, iy1)));
    inner->addEntity(new RS_Line(inner, RS_Vector(ix2, iy1), RS_Vector(ix1, iy1)));

    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose single boundary loop is a right triangle with
 * the right angle at the origin: (0,0) → (b,0) → (0,h) → (0,0), wound CCW.
 *
 * Analytical properties (b=3, h=4):
 *   Area        = b·h/2
 *   Centroid    = (b/3, h/3)
 *   ixx         = b³·h / 36        (∬(x−cx)² dA)
 *   iyy         = b·h³ / 36        (∬(y−cy)² dA)
 *   ixy         = −b²·h² / 72      (∬(x−cx)(y−cy) dA)  ← non-zero!
 *
 * Ownership: caller takes ownership of the returned pointer.
 */
RS_Hatch* makeTriangleHatch(double b, double h)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));

    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);

    // CCW winding: (0,0) → (b,0) → (0,h) → (0,0)
    loop->addEntity(new RS_Line(loop, RS_Vector(0.0, 0.0), RS_Vector(b,   0.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(b,   0.0), RS_Vector(0.0, h  )));
    loop->addEntity(new RS_Line(loop, RS_Vector(0.0, h  ), RS_Vector(0.0, 0.0)));

    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose boundary is the upper half-disk of radius r:
 *   - CCW arc from (r,0) to (-r,0) (angle1=0, angle2=π, not reversed)
 *   - Closing line from (-r,0) to (r,0)
 *
 * Analytical properties (r=1):
 *   Area        = π r² / 2
 *   Centroid    = (0,  4r/(3π))
 *   ixx         = π r⁴ / 8            (centroidal, ∬(x−cx)² dA)
 *   iyy         = π r⁴/8 − 8r²/(9π)  (centroidal)
 *   ixy         = 0
 */
RS_Hatch* makeHalfDiskHatch(double r)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    loop->addEntity(new RS_Arc(loop, RS_ArcData(RS_Vector(0.0, 0.0), r, 0.0, M_PI, false)));
    loop->addEntity(new RS_Line(loop, RS_Vector(-r, 0.0), RS_Vector(r, 0.0)));
    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose boundary is the quarter-disk of radius r
 * in the first quadrant:
 *   - Line from (0,0) to (r,0)
 *   - CCW arc from (r,0) to (0,r) (angle1=0, angle2=π/2)
 *   - Line from (0,r) to (0,0)
 *
 * Analytical properties (r=1):
 *   Area        = π r² / 4
 *   Centroid    = (4r/(3π),  4r/(3π))
 *   ixx = iyy   = π r⁴/16 − 4r²/(9π)
 *   ixy         = r⁴/8 − 4r²/(9π)   ← non-zero (≈ −0.0165 for r=1)
 */
RS_Hatch* makeQuarterDiskHatch(double r)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    loop->addEntity(new RS_Line(loop, RS_Vector(0.0, 0.0), RS_Vector(r, 0.0)));
    loop->addEntity(new RS_Arc(loop, RS_ArcData(RS_Vector(0.0, 0.0), r, 0.0, M_PI / 2.0, false)));
    loop->addEntity(new RS_Line(loop, RS_Vector(0.0, r), RS_Vector(0.0, 0.0)));
    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose boundary is a full ellipse centred at (cx,cy)
 * with semi-major axis a (at angle phi) and semi-minor axis b.
 *
 * A full ellipse is specified by angle1 = angle2 = 0 (isArc = false).
 *
 * Analytical centroidal moments for center at origin:
 *   Area  = π a b
 *   ixx   = π a³ b / 4   (centroidal)
 *   iyy   = π a b³ / 4   (centroidal)
 *   ixy   = 0             (phi=0)
 *   ixy   = π ab(a²−b²)sin(2φ)/8   for arbitrary phi
 */
RS_Hatch* makeFullEllipseHatch(double cx, double cy, double a, double b, double phi)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    RS_EllipseData ed;
    ed.center  = RS_Vector(cx, cy);
    ed.majorP  = RS_Vector(a * std::cos(phi), a * std::sin(phi));
    ed.ratio   = b / a;
    ed.angle1  = 0.0;
    ed.angle2  = 0.0;
    ed.reversed = false;
    loop->addEntity(new RS_Ellipse(loop, ed));
    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose boundary is the upper half of the ellipse
 * x²/a² + y²/b² ≤ 1 (center at origin, major axis along x):
 *   - CCW arc from (a,0) to (-a,0) (angle1=0, angle2=π)
 *   - Closing line from (-a,0) to (a,0)
 *
 * Analytical properties:
 *   Area        = π a b / 2
 *   Centroid    = (0,  4b/(3π))
 *   ixx         = π a³ b / 8         (centroidal)
 *   iyy         = π a b³/8 − 2b·A·(4b/(3π))²/... = π ab³/8 − b²A(4/(3π))²/2
 *                 simplified: π a b³/8 − 2 a b³/(3π) · (8/3) ... let iyy = πab³/8 − (Area)(4b/(3π))²
 *               = πab³/8 − (πab/2)(16b²/(9π²)) = πab³/8 − 8b²a/(9π)
 *               For a=3,b=2: iyy_c = 3π − 64/(3π)
 *   ixy         = 0
 */
RS_Hatch* makeHalfEllipseHatch(double a, double b)
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    RS_EllipseData ed;
    ed.center  = RS_Vector(0.0, 0.0);
    ed.majorP  = RS_Vector(a, 0.0);
    ed.ratio   = b / a;
    ed.angle1  = 0.0;
    ed.angle2  = M_PI;
    ed.reversed = false;
    loop->addEntity(new RS_Ellipse(loop, ed));
    loop->addEntity(new RS_Line(loop, RS_Vector(-a, 0.0), RS_Vector(a, 0.0)));
    hatch->update();
    return hatch;
}

/**
 * Build a solid hatch whose boundary is a closed quadratic B-spline
 * with four control points at (1,0), (0,1), (-1,0), (0,-1).
 *
 * The loop decomposes into four quadratic Bézier segments (one per
 * control point), each spanning from the midpoint between consecutive
 * controls to the next midpoint:
 *   seg 0: (0.5,−0.5) → ctrl (1,0) → (0.5,0.5)
 *   seg 1: (0.5, 0.5) → ctrl (0,1) → (−0.5,0.5)
 *   seg 2: (−0.5,0.5) → ctrl (−1,0) → (−0.5,−0.5)
 *   seg 3: (−0.5,−0.5) → ctrl (0,−1) → (0.5,−0.5)
 *
 * Analytical properties (exact, via 5-point Gauss-Legendre):
 *   Area        = 5/3
 *   Centroid    = (0, 0)
 *   ixx = iyy   = 31/140
 *   ixy         = 0
 */
RS_Hatch* makeClosedSplineHatch()
{
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    LC_SplinePointsData sd(true, false);
    sd.useControlPoints = true;
    sd.controlPoints = {
        RS_Vector( 1.0,  0.0),
        RS_Vector( 0.0,  1.0),
        RS_Vector(-1.0,  0.0),
        RS_Vector( 0.0, -1.0)
    };
    loop->addEntity(new LC_SplinePoints(loop, std::move(sd)));
    hatch->update();
    return hatch;
}

/**
 * Minimal painter rig for rendering tests.
 *
 * Coordinate system: 1 WCS unit = 1 pixel, y-axis flipped.
 *   screen_x = wcs_x
 *   screen_y = H - wcs_y
 *
 * Canvas is W×H pixels, filled with white.  Call draw() on an RS_Hatch with
 * a black pen and the interior pixels will be painted black.
 */
struct TestPainter {
    static constexpr int W = 256;
    static constexpr int H = 256;

    QImage           image{W, H, QImage::Format_RGB32};
    LC_GraphicViewport viewport;
    RS_Painter       painter;

    TestPainter()
        : painter(&image)
    {
        image.fill(Qt::white);
        // factor = 1, offsets = 0 → 1:1 mapping, y-flipped
        viewport.setSize(W, H);
        viewport.justSetOffsetAndFactor(0, 0, 1.0);
        painter.setViewPort(&viewport);

        // Bounding rect covers the whole canvas (plus margin) so no arc segment
        // is mistakenly clipped as "out of viewport" during path construction.
        LC_Rect bounds{RS_Vector(-W, -H), RS_Vector(2 * W, 2 * H)};
        painter.setWorldBoundingRect(bounds);

        // Use black pen so drawSolidFill picks black as the fill colour.
        painter.setPen(RS_Color(0, 0, 0));
    }

    /** True when the pixel at WCS (wcsX, wcsY) has been painted (non-white). */
    bool filledAt(double wcsX, double wcsY) const {
        const int px = static_cast<int>(std::round(wcsX));
        const int py = static_cast<int>(std::round(H - wcsY));
        if (px < 0 || px >= W || py < 0 || py >= H) return false;
        return image.pixel(px, py) != qRgb(255, 255, 255);
    }
};

} // anonymous namespace

// ============================================================
// AREA
// ============================================================

TEST_CASE("RS_Hatch area - unit square", "[rs_hatch][area]")
{
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 1.0, 1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(1.0, 1e-6));
}

TEST_CASE("RS_Hatch area - 4×3 rectangle", "[rs_hatch][area]")
{
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 4.0, 3.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(12.0, 1e-6));
}

TEST_CASE("RS_Hatch area - circle radius 2", "[rs_hatch][area]")
{
    // A = π r² = 4π
    std::unique_ptr<RS_Hatch> hatch(makeCircleHatch(0.0, 0.0, 2.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(4.0 * M_PI, 1e-4));
}

TEST_CASE("RS_Hatch area - concentric circles center (100,100) r=40 r=20", "[rs_hatch][area]")
{
    // net area = π (40² - 20²) = π * 1200
    std::unique_ptr<RS_Hatch> hatch(makeConcentricCircleHatch(100.0, 100.0, 40.0, 20.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(1200.0 * M_PI, 1e-3));
}

TEST_CASE("RS_Hatch centroid - concentric circles center (100,100) r=40 r=20", "[rs_hatch][centroid]")
{
    std::unique_ptr<RS_Hatch> hatch(makeConcentricCircleHatch(100.0, 100.0, 40.0, 20.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(100.0, 1e-3));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(100.0, 1e-3));
}

TEST_CASE("RS_Hatch area - annular rectangle (outer 6×6, hole 2×2)", "[rs_hatch][area]")
{
    // net area = 36 - 4 = 32
    std::unique_ptr<RS_Hatch> hatch(
        makeAnnularRectHatch(0.0, 0.0, 6.0, 6.0, 2.0, 2.0, 4.0, 4.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(32.0, 1e-5));
}

// ============================================================
// CENTROID
// ============================================================

TEST_CASE("RS_Hatch centroid - unit square", "[rs_hatch][centroid]")
{
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 1.0, 1.0));
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(0.5, 1e-6));
}

TEST_CASE("RS_Hatch centroid - 4×3 rectangle", "[rs_hatch][centroid]")
{
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 4.0, 3.0));
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(2.0, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(1.5, 1e-6));
}

TEST_CASE("RS_Hatch centroid - shifted rectangle (3,2)-(7,6)", "[rs_hatch][centroid]")
{
    // centroid at (5, 4)
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(3.0, 2.0, 7.0, 6.0));
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(5.0, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(4.0, 1e-6));
}

TEST_CASE("RS_Hatch centroid - circle at (2, 3)", "[rs_hatch][centroid]")
{
    std::unique_ptr<RS_Hatch> hatch(makeCircleHatch(2.0, 3.0, 1.5));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(2.0, 1e-4));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(3.0, 1e-4));
}

TEST_CASE("RS_Hatch centroid - annular rectangle symmetric about (3,3)", "[rs_hatch][centroid]")
{
    std::unique_ptr<RS_Hatch> hatch(
        makeAnnularRectHatch(0.0, 0.0, 6.0, 6.0, 2.0, 2.0, 4.0, 4.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(3.0, 1e-5));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(3.0, 1e-5));
}

// ============================================================
// SECOND MOMENT OF AREA (moment of inertia)
//
// getMomentOfInertia() returns LC_SecondMoment centred at the centroid:
//   ixx = ∬ (x − cx)² dA   (2nd moment about the vertical centroidal axis)
//   iyy = ∬ (y − cy)² dA   (2nd moment about the horizontal centroidal axis)
//   ixy = ∬ (x − cx)(y − cy) dA   (product of inertia)
// ============================================================

TEST_CASE("RS_Hatch moment of inertia - unit square", "[rs_hatch][moment]")
{
    // ixx = iyy = w³h/12 = 1/12,  ixy = 0
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 1.0, 1.0));
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    const double expected = 1.0 / 12.0;
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(expected, 1e-6));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(expected, 1e-6));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,      1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - 4×3 rectangle", "[rs_hatch][moment]")
{
    // ixx = w³h/12 = 64·3/12 = 16
    // iyy = wh³/12 = 4·27/12 = 9
    // ixy = 0  (axis-aligned rectangle)
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(0.0, 0.0, 4.0, 3.0));
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(16.0, 1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs( 9.0, 1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs( 0.0, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - same rectangle shifted by (3,2)", "[rs_hatch][moment]")
{
    // Translation does not change the centroidal moments.
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(3.0, 2.0, 7.0, 5.0));
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(16.0, 1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs( 9.0, 1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs( 0.0, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - circle radius 1 at origin", "[rs_hatch][moment]")
{
    // ixx = iyy = πr⁴/4 = π/4,  ixy = 0
    std::unique_ptr<RS_Hatch> hatch(makeCircleHatch(0.0, 0.0, 1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    const double expected = M_PI / 4.0;
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,      1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - circle radius 1 shifted to (2,3)", "[rs_hatch][moment]")
{
    // Centroidal moments are translation-invariant.
    std::unique_ptr<RS_Hatch> hatch(makeCircleHatch(2.0, 3.0, 1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    const double expected = M_PI / 4.0;
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,      1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - annular rectangle (outer 6×6, hole 2×2)", "[rs_hatch][moment]")
{
    // Both outer and hole are centred at (3, 3), so no parallel-axis shift is needed.
    //   ixx_outer = w³h/12 = 216·6/12 = 108
    //   ixx_hole  = 2³·2/12 = 4/3
    //   ixx_net   = 108 − 4/3 = 320/3  ≈ 106.6667
    // By symmetry iyy_net = ixx_net,  ixy_net = 0.
    std::unique_ptr<RS_Hatch> hatch(
        makeAnnularRectHatch(0.0, 0.0, 6.0, 6.0, 2.0, 2.0, 4.0, 4.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    const double expected = 320.0 / 3.0;
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(expected, 1e-4));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,      1e-5));
}

// ============================================================
// RIGHT TRIANGLE  (tests non-zero product of inertia ixy)
//
// Vertices: (0,0) → (b,0) → (0,h)   b=3, h=4
//   Area        = b·h / 2               =  6
//   Centroid    = (b/3, h/3)            =  (1, 4/3)
//   ixx         = b³·h / 36            =  3
//   iyy         = b·h³ / 36            =  16/3
//   ixy         = −b²·h² / 72          = −2   ← unique non-symmetric case
// ============================================================

TEST_CASE("RS_Hatch area - right triangle (3,4)", "[rs_hatch][area]")
{
    const double b = 3.0, h = 4.0;
    std::unique_ptr<RS_Hatch> hatch(makeTriangleHatch(b, h));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(b * h / 2.0, 1e-6));
}

TEST_CASE("RS_Hatch centroid - right triangle (3,4)", "[rs_hatch][centroid]")
{
    const double b = 3.0, h = 4.0;
    std::unique_ptr<RS_Hatch> hatch(makeTriangleHatch(b, h));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(b / 3.0, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(h / 3.0, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - right triangle (3,4)", "[rs_hatch][moment]")
{
    // This is the primary test for ixy ≠ 0.
    //   ixx = b³·h/36 = 27·4/36 = 3
    //   iyy = b·h³/36 =  3·64/36 = 16/3
    //   ixy = −b²·h²/72 = −9·16/72 = −2
    const double b = 3.0, h = 4.0;
    std::unique_ptr<RS_Hatch> hatch(makeTriangleHatch(b, h));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(b * b * b * h / 36.0, 1e-6));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(b * h * h * h / 36.0, 1e-6));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(-b * b * h * h / 72.0, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - right triangle translation-invariant", "[rs_hatch][moment]")
{
    // Shifting the same triangle to (5, 7) must not change the centroidal moments.
    const double b = 3.0, h = 4.0;
    const double dx = 5.0, dy = 7.0;

    auto* hatch_raw = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch_raw);
    hatch_raw->addEntity(loop);
    loop->addEntity(new RS_Line(loop, RS_Vector(dx,     dy    ), RS_Vector(dx + b, dy    )));
    loop->addEntity(new RS_Line(loop, RS_Vector(dx + b, dy    ), RS_Vector(dx,     dy + h)));
    loop->addEntity(new RS_Line(loop, RS_Vector(dx,     dy + h), RS_Vector(dx,     dy    )));
    hatch_raw->update();
    std::unique_ptr<RS_Hatch> hatch(hatch_raw);

    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(b * b * b * h / 36.0, 1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(b * h * h * h / 36.0, 1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(-b * b * h * h / 72.0, 1e-5));
}

// ============================================================
// RS_ARC BOUNDARY
//
// Half-disk (r=1): upper semicircle arc + diameter line
//   Area        = π/2
//   Centroid    = (0, 4/(3π))
//   ixx         = π/8
//   iyy         = π/8 − 8/(9π)
//   ixy         = 0
//
// Quarter-disk (r=1): first-quadrant wedge
//   Area        = π/4
//   Centroid    = (4/(3π), 4/(3π))
//   ixx = iyy   = π/16 − 4/(9π)
//   ixy         = 1/8 − 4/(9π)   (≈ −0.0165, non-zero)
// ============================================================

TEST_CASE("RS_Hatch area - half-disk r=1", "[rs_hatch][area][arc]")
{
    std::unique_ptr<RS_Hatch> hatch(makeHalfDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(M_PI / 2.0, 1e-6));
}

TEST_CASE("RS_Hatch centroid - half-disk r=1", "[rs_hatch][centroid][arc]")
{
    std::unique_ptr<RS_Hatch> hatch(makeHalfDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.0,               1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(4.0 / (3.0 * M_PI), 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - half-disk r=1", "[rs_hatch][moment][arc]")
{
    // ixx = π/8,   iyy = π/8 − 8/(9π),   ixy = 0
    std::unique_ptr<RS_Hatch> hatch(makeHalfDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(M_PI / 8.0,                   1e-6));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(M_PI / 8.0 - 8.0 / (9.0 * M_PI), 1e-6));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,                           1e-6));
}

TEST_CASE("RS_Hatch area - quarter-disk r=1", "[rs_hatch][area][arc]")
{
    std::unique_ptr<RS_Hatch> hatch(makeQuarterDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(M_PI / 4.0, 1e-6));
}

TEST_CASE("RS_Hatch centroid - quarter-disk r=1", "[rs_hatch][centroid][arc]")
{
    const double cy_expected = 4.0 / (3.0 * M_PI);
    std::unique_ptr<RS_Hatch> hatch(makeQuarterDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(cy_expected, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(cy_expected, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - quarter-disk r=1", "[rs_hatch][moment][arc]")
{
    // Tests non-zero centroidal ixy for an arc boundary.
    // ixx = iyy = π/16 − 4/(9π)
    // ixy       = 1/8  − 4/(9π)   (≈ −0.0165)
    const double k = 4.0 / (9.0 * M_PI);
    std::unique_ptr<RS_Hatch> hatch(makeQuarterDiskHatch(1.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(M_PI / 16.0 - k, 1e-6));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(M_PI / 16.0 - k, 1e-6));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(1.0 / 8.0   - k, 1e-6));
}

// ============================================================
// RS_ELLIPSE BOUNDARY
//
// Full ellipse (a=3, b=2, φ=0, centre=origin):
//   Area        = 6π
//   Centroid    = (0, 0)
//   ixx         = π a³ b / 4 = 27π/2
//   iyy         = π a b³ / 4 = 6π
//   ixy         = 0
//
// Full ellipse (a=3, b=2, φ=π/4, centre=origin):
//   Area        = 6π
//   Centroid    = (0, 0)
//   ixx = iyy   = 39π/4
//   ixy         = 15π/4   (positive – principal axes at ±45°)
//
// Half-ellipse arc (a=3, b=2, upper half):
//   Area        = 3π
//   Centroid    = (0,  8/(3π))
//   ixx         = 27π/4
//   iyy         = 3π − 64/(3π)
//   ixy         = 0
// ============================================================

TEST_CASE("RS_Hatch area - full ellipse a=3 b=2", "[rs_hatch][area][ellipse]")
{
    std::unique_ptr<RS_Hatch> hatch(makeFullEllipseHatch(0.0, 0.0, 3.0, 2.0, 0.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(6.0 * M_PI, 1e-6));
}

TEST_CASE("RS_Hatch centroid - full ellipse a=3 b=2 at origin", "[rs_hatch][centroid][ellipse]")
{
    std::unique_ptr<RS_Hatch> hatch(makeFullEllipseHatch(0.0, 0.0, 3.0, 2.0, 0.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(0.0, 1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - full ellipse a=3 b=2 phi=0", "[rs_hatch][moment][ellipse]")
{
    // ixx = πa³b/4 = 27π/2,  iyy = πab³/4 = 6π,  ixy = 0
    std::unique_ptr<RS_Hatch> hatch(makeFullEllipseHatch(0.0, 0.0, 3.0, 2.0, 0.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(27.0 * M_PI / 2.0, 1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs( 6.0 * M_PI,       1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs( 0.0,               1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - full ellipse a=3 b=2 phi=pi/4", "[rs_hatch][moment][ellipse]")
{
    // Rotating the ellipse by 45° yields non-zero ixy:
    //   ixx = iyy = (27π/2 + 6π)/2 = 39π/4
    //   ixy = (a²−b²)πab sin(2φ)/8 = 5·6π·(1/2)/4 = 15π/4  (positive)
    std::unique_ptr<RS_Hatch> hatch(makeFullEllipseHatch(0.0, 0.0, 3.0, 2.0, M_PI / 4.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(39.0 * M_PI / 4.0, 1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(39.0 * M_PI / 4.0, 1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(15.0 * M_PI / 4.0, 1e-5));
}

TEST_CASE("RS_Hatch area - half-ellipse arc a=3 b=2", "[rs_hatch][area][ellipse]")
{
    std::unique_ptr<RS_Hatch> hatch(makeHalfEllipseHatch(3.0, 2.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(3.0 * M_PI, 1e-6));
}

TEST_CASE("RS_Hatch centroid - half-ellipse arc a=3 b=2", "[rs_hatch][centroid][ellipse]")
{
    // cy = 4b/(3π) = 8/(3π)
    std::unique_ptr<RS_Hatch> hatch(makeHalfEllipseHatch(3.0, 2.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.0,                  1e-6));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(8.0 / (3.0 * M_PI),   1e-6));
}

TEST_CASE("RS_Hatch moment of inertia - half-ellipse arc a=3 b=2", "[rs_hatch][moment][ellipse]")
{
    // ixx = πa³b/8 = 27π/4
    // iyy = πab³/8 − (πab/2)(4b/(3π))² = πab³/8 − 8ab²/(9π) = 3π − 64/(3π)
    // ixy = 0
    std::unique_ptr<RS_Hatch> hatch(makeHalfEllipseHatch(3.0, 2.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(27.0 * M_PI / 4.0,         1e-5));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(3.0 * M_PI - 64.0/(3.0*M_PI), 1e-5));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,                         1e-6));
}

// ============================================================
// LC_SPLINEPOINTS BOUNDARY
//
// Closed quadratic B-spline with control points at the four cardinal
// directions on the unit circle.  The spline forms a "rounded diamond"
// shape strictly inside the unit circle.
//
//   Area        = 5/3      (exact, via Green's theorem polynomial formula)
//   Centroid    = (0, 0)   (4-fold symmetry)
//   ixx = iyy   = 31/140   (exact, via 5-point Gauss-Legendre, degree-7)
//   ixy         = 0        (4-fold symmetry)
// ============================================================

TEST_CASE("RS_Hatch area - closed spline diamond", "[rs_hatch][area][spline]")
{
    std::unique_ptr<RS_Hatch> hatch(makeClosedSplineHatch());
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE_THAT(hatch->getTotalArea(), Catch::Matchers::WithinAbs(5.0 / 3.0, 1e-9));
}

TEST_CASE("RS_Hatch centroid - closed spline diamond", "[rs_hatch][centroid][spline]")
{
    std::unique_ptr<RS_Hatch> hatch(makeClosedSplineHatch());
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const RS_Vector c = hatch->getCentroid();
    REQUIRE(c.isValid());
    REQUIRE_THAT(c.x, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(c.y, Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("RS_Hatch moment of inertia - closed spline diamond", "[rs_hatch][moment][spline]")
{
    // Gauss-Legendre quadrature is exact for these degree-7 integrands.
    std::unique_ptr<RS_Hatch> hatch(makeClosedSplineHatch());
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const LC_SecondMoment m = hatch->getMomentOfInertia();
    REQUIRE_THAT(m.ixx, Catch::Matchers::WithinAbs(31.0 / 140.0, 1e-9));
    REQUIRE_THAT(m.iyy, Catch::Matchers::WithinAbs(31.0 / 140.0, 1e-9));
    REQUIRE_THAT(m.ixy, Catch::Matchers::WithinAbs(0.0,           1e-9));
}

// ============================================================
// SOLID FILL RENDERING TESTS
//
// These tests verify that drawSolidFill() paints pixels inside the hatch
// boundary and leaves pixels outside unpainted.
//
// Canvas: 256×256 pixels, white background.
// Mapping: screen_x = wcs_x, screen_y = 256 − wcs_y  (1:1 scale, y-flipped).
// Fill colour: black (pen set to black before drawing).
// ============================================================

TEST_CASE("RS_Hatch solid fill - rectangle interior is painted", "[rs_hatch][solidfill]")
{
    // Rectangle WCS (20,20)-(120,120).  Centre at (70,70) should be filled.
    // Points outside the rectangle should remain white.
    std::unique_ptr<RS_Hatch> hatch(makeRectHatch(20.0, 20.0, 120.0, 120.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);

    TestPainter tp;
    hatch->draw(&tp.painter);

    // Interior point — must be painted
    REQUIRE(tp.filledAt(70.0, 70.0));

    // Exterior points — must remain white
    REQUIRE_FALSE(tp.filledAt( 5.0,  5.0));   // bottom-left, far outside
    REQUIRE_FALSE(tp.filledAt(200.0, 70.0));   // right of rect
    REQUIRE_FALSE(tp.filledAt(70.0, 200.0));   // above rect
}

TEST_CASE("RS_Hatch solid fill - L-shaped polygon interior is painted", "[rs_hatch][solidfill]")
{
    // Reproduces the h4.dxf regression: 6-segment L-shaped polygon.
    // Before the fix, appendLine called moveTo per entity, producing disconnected
    // subpaths with zero area, so nothing was painted.
    //
    // Vertices (CCW): (20,50)→(80,50)→(80,30)→(60,30)→(60,10)→(20,10)→(20,50)
    // Interior point: (40, 30) — clearly inside the rectangle leg
    // Interior point: (50, 40) — inside the top bar
    // Exterior point: (70, 20) — inside the notch (cut-out region)
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    loop->addEntity(new RS_Line(loop, RS_Vector(20.0, 50.0), RS_Vector(80.0, 50.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(80.0, 50.0), RS_Vector(80.0, 30.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(80.0, 30.0), RS_Vector(60.0, 30.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(60.0, 30.0), RS_Vector(60.0, 10.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(60.0, 10.0), RS_Vector(20.0, 10.0)));
    loop->addEntity(new RS_Line(loop, RS_Vector(20.0, 10.0), RS_Vector(20.0, 50.0)));
    hatch->update();
    std::unique_ptr<RS_Hatch> owned(hatch);

    REQUIRE(owned->getUpdateError() == RS_Hatch::HATCH_OK);

    TestPainter tp;
    owned->draw(&tp.painter);

    // Both legs of the L must be filled
    REQUIRE(tp.filledAt(40.0, 30.0));   // left vertical leg
    REQUIRE(tp.filledAt(50.0, 40.0));   // top horizontal bar (above step)

    // The notch cut-out must NOT be filled
    REQUIRE_FALSE(tp.filledAt(70.0, 20.0));

    // Well outside the shape
    REQUIRE_FALSE(tp.filledAt( 5.0, 5.0));
    REQUIRE_FALSE(tp.filledAt(200.0, 200.0));
}

TEST_CASE("RS_Hatch solid fill - circle interior is painted", "[rs_hatch][solidfill]")
{
    // Circle centred at (100, 100) radius 40.
    // Centre pixel must be filled; a point well outside must not be.
    std::unique_ptr<RS_Hatch> hatch(makeCircleHatch(100.0, 100.0, 40.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);

    TestPainter tp;
    hatch->draw(&tp.painter);

    REQUIRE(tp.filledAt(100.0, 100.0));   // dead centre
    REQUIRE(tp.filledAt(110.0, 100.0));   // slightly off-centre, still inside

    REQUIRE_FALSE(tp.filledAt(100.0, 150.0));  // above the circle
    REQUIRE_FALSE(tp.filledAt(  5.0,   5.0));  // far outside
}

TEST_CASE("RS_Hatch solid fill - annular rectangle hole excluded", "[rs_hatch][solidfill]")
{
    // Outer rect (10,10)-(110,110), inner hole (40,40)-(80,80).
    // Ring region must be filled; hole and outside must not be.
    std::unique_ptr<RS_Hatch> hatch(
        makeAnnularRectHatch(10.0, 10.0, 110.0, 110.0,
                             40.0, 40.0,  80.0,  80.0));
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);

    TestPainter tp;
    hatch->draw(&tp.painter);

    // Ring (between outer and hole) must be filled
    REQUIRE(tp.filledAt(20.0,  60.0));   // left strip of ring
    REQUIRE(tp.filledAt(60.0, 100.0));   // top strip of ring

    // Inside the hole must NOT be filled
    REQUIRE_FALSE(tp.filledAt(60.0, 60.0));

    // Outside the outer rect must NOT be filled
    REQUIRE_FALSE(tp.filledAt(5.0, 5.0));
    REQUIRE_FALSE(tp.filledAt(200.0, 200.0));
}

TEST_CASE("RS_Hatch solid fill - arc boundary (half-disk) interior is painted", "[rs_hatch][solidfill][arc]")
{
    // Upper half-disk: arc from (50,50) angle 0→π, radius 30, closing line.
    // Centre of the flat face at (50, 50), centroid above it.
    // Interior point (50, 65) should be inside; (50, 30) is below the flat face.
    auto* hatch = new RS_Hatch(nullptr, RS_HatchData(true, 1.0, 0.0, "SOLID"));
    auto* loop = new RS_EntityContainer(hatch);
    hatch->addEntity(loop);
    // Arc from (80,50) to (20,50) CCW (upper semicircle centred at (50,50))
    loop->addEntity(new RS_Arc(loop,
        RS_ArcData(RS_Vector(50.0, 50.0), 30.0, 0.0, M_PI, false)));
    // Close with a line from (-r,0)+(50,50)=(20,50) back to (r,0)+(50,50)=(80,50)
    loop->addEntity(new RS_Line(loop,
        RS_Vector(20.0, 50.0), RS_Vector(80.0, 50.0)));
    hatch->update();
    std::unique_ptr<RS_Hatch> owned(hatch);

    REQUIRE(owned->getUpdateError() == RS_Hatch::HATCH_OK);

    TestPainter tp;
    owned->draw(&tp.painter);

    REQUIRE(tp.filledAt(50.0, 65.0));    // inside upper half-disk

    REQUIRE_FALSE(tp.filledAt(50.0, 30.0));  // below flat edge, outside
    REQUIRE_FALSE(tp.filledAt( 5.0,  5.0));  // far outside
}
