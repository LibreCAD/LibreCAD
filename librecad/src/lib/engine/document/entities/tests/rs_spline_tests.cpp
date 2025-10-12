/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

#include "rs_spline.h"

// Helper function to compare vectors of RS_Vector with approximation
bool comparePoints(const std::vector<RS_Vector>& actual, const std::vector<std::pair<double, double>>& expected, double epsilon = 1e-6) {
    if (actual.size() != expected.size()) return false;
    for (size_t i = 0; i < actual.size(); ++i) {
        if (std::fabs(actual[i].x - expected[i].first) > epsilon ||
            std::fabs(actual[i].y - expected[i].second) > epsilon) {
            return false;
        }
    }
    return true;
}

// Helper function to normalize knot vector to [0,1]
std::vector<double> normalizeKnots(const std::vector<double>& knots) {
    if (knots.empty()) return {};
    double min_k = *std::min_element(knots.begin(), knots.end());
    double max_k = *std::max_element(knots.begin(), knots.end());
    double range = max_k - min_k;
    if (range < 1e-10) return knots; // Constant knots, no normalization needed
    std::vector<double> norm(knots.size());
    for (size_t i = 0; i < knots.size(); ++i) {
        norm[i] = (knots[i] - min_k) / range;
    }
    return norm;
}

// Helper function to compare knot vectors with approximation, after scaling to [0,1]
bool compareKnots(const std::vector<double>& actual, const std::vector<double>& expected, double epsilon = 1e-6) {
    if (actual.size() != expected.size()) return false;
    auto norm_actual = normalizeKnots(actual);
    auto norm_expected = normalizeKnots(expected);
    for (size_t i = 0; i < actual.size(); ++i) {
        if (std::fabs(norm_actual[i] - norm_expected[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

TEST_CASE("RS_Spline Basics", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);

    REQUIRE(s.getDegree() == 3);
    REQUIRE(s.getNumberOfControlPoints() == 4);
    REQUIRE(!s.isClosed());
}

TEST_CASE("RS_Spline Uniform Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    std::vector<double> original_knots = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    s.setKnotVector(original_knots);
    s.update();

    std::vector<RS_Vector> points;
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected = {
        {1.0, 0.6666666666666666},
        {1.032258064516129, 0.6656484620634867},
        {1.064516129032258, 0.6626833607465342},
        {1.096774193548387, 0.6579056314546899},
        {1.129032258064516, 0.6514495429268347}
    };
    REQUIRE(comparePoints(start_points, start_expected));

    std::vector<RS_Vector> end_points(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected = {
        {1.870967741935484, 0.34855045707316523},
        {1.903225806451613, 0.3420943685453101},
        {1.935483870967742, 0.33731663925346583},
        {1.9677419354838708, 0.3343515379365133},
        {2.0, 0.3333333333333333}
    };
    REQUIRE(comparePoints(end_points, end_expected));

    REQUIRE(s.getMin().x == Catch::Approx(1.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.3333333333));
    REQUIRE(s.getMax().x == Catch::Approx(2.0));
    REQUIRE(s.getMax().y == Catch::Approx(0.6666666667));

    // Set to closed
    s.setClosed(true);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points_closed(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected_closed = {
        {1.0, 0.6666666666666666},
        {1.129032258064516, 0.6514495429268347},
        {1.258064516129032, 0.6115269712329228},
        {1.3870967741935483, 0.5554921508733062},
        {1.5161290322580643, 0.49193828113636107}
    };
    REQUIRE(comparePoints(start_points_closed, start_expected_closed));

    std::vector<RS_Vector> end_points_closed(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected_closed = {
        {0.5755317601512763, 0.4919382811363613},
        {0.6515726226041425, 0.5554921508733064},
        {0.7533930829221356, 0.6115269712329228},
        {0.8723999418168799, 0.6514495429268347},
        {1.0, 0.6666666666666666}
    };
    REQUIRE(comparePoints(end_points_closed, end_expected_closed));

    REQUIRE(s.getMin().x == Catch::Approx(0.5338636949));
    REQUIRE(s.getMin().y == Catch::Approx(0.3343515379));
    REQUIRE(s.getMax().x == Catch::Approx(2.4701755564));
    REQUIRE(s.getMax().y == Catch::Approx(0.6666666667));

    // Round-trip back to open
    s.setClosed(false);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> rt_points = {points[0], points[31]};
    std::vector<std::pair<double, double>> rt_expected = {
        {1.0, 0.6666666666666666},
        {2.0, 0.3333333333333333}
    };
    REQUIRE(comparePoints(rt_points, rt_expected));

    REQUIRE(s.getMin().x == Catch::Approx(1.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.3333333333));
    REQUIRE(s.getMax().x == Catch::Approx(2.0));
    REQUIRE(s.getMax().y == Catch::Approx(0.6666666667));

    const auto& restored_knots = s.getKnotVector();
    REQUIRE(restored_knots.size() == original_knots.size());
    for (size_t i = 0; i < restored_knots.size(); ++i) {
        REQUIRE(restored_knots[i] == Catch::Approx(original_knots[i]));
    }
}

TEST_CASE("RS_Spline Add Control Points Validate Scaled Uniform Knots", "[spline]") {
    for (size_t num = 4; num <= 10; ++num) {
        RS_Spline s(nullptr, RS_SplineData(3, false));

        for (size_t i = 0; i < num; ++i) {
            double x = static_cast<double>(i);
            double y = (i % 2 == 0) ? 0.0 : 1.0;
            s.addControlPoint(RS_Vector(x, y), 1.0);
        }

        size_t order = 4;
        std::vector<double> expected_knots(num + order, 0.0);
        std::iota(expected_knots.begin() + order, expected_knots.begin() + num + 1, 1.0);
        std::fill(expected_knots.begin() + num + 1, expected_knots.end(), expected_knots[num]);

        REQUIRE(compareKnots(s.getKnotVector(), expected_knots));
    }
}

TEST_CASE("RS_Spline addControlPoint Incremental Open Uniform", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));

    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    REQUIRE(s.getKnotVector().empty());

    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    REQUIRE(s.getKnotVector().empty());

    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    REQUIRE(s.getKnotVector().empty());

    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}));

    s.addControlPoint(RS_Vector(4.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 2.0, 2.0}));

    s.addControlPoint(RS_Vector(5.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 3.0, 3.0, 3.0}));

    s.addControlPoint(RS_Vector(6.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 4.0, 4.0, 4.0}));

    s.addControlPoint(RS_Vector(7.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 5.0, 5.0, 5.0}));

    s.addControlPoint(RS_Vector(8.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 6.0, 6.0, 6.0}));

    s.addControlPoint(RS_Vector(9.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 7.0, 7.0, 7.0}));
}

TEST_CASE("RS_Spline addControlPoint Incremental Closed Uniform", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, true));

    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    REQUIRE(s.getKnotVector().empty());

    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    REQUIRE(s.getKnotVector().empty());

    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0}));

    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}));

    s.addControlPoint(RS_Vector(4.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}));

    s.addControlPoint(RS_Vector(5.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}));

    s.addControlPoint(RS_Vector(6.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}));

    s.addControlPoint(RS_Vector(7.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0}));

    s.addControlPoint(RS_Vector(8.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0}));

    s.addControlPoint(RS_Vector(9.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0}));
}


TEST_CASE("RS_Spline addControlPoint Incremental Closed Non-Uniform", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, true));

    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);

    std::vector<double> custom_knots = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5};
    s.setKnotVector(custom_knots);
    REQUIRE(compareKnots(s.getKnotVector(), custom_knots));

    s.addControlPoint(RS_Vector(4.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0}));

    s.addControlPoint(RS_Vector(5.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5}));

    s.addControlPoint(RS_Vector(6.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}));

    s.addControlPoint(RS_Vector(7.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5}));

    s.addControlPoint(RS_Vector(8.0, 0.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5, 6.0}));

    s.addControlPoint(RS_Vector(9.0, 1.0), 1.0);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5, 6.0, 6.5}));
}


TEST_CASE("RS_Spline setClosed from Closed to Open Multiple Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, true));

    for (size_t i = 0; i < 5; ++i) {
        s.addControlPoint(RS_Vector(static_cast<double>(i), i % 2), 1.0);
    }

    s.update();

    REQUIRE(s.isClosed());
    REQUIRE(s.getNumberOfControlPoints() == 5);
    REQUIRE(s.getKnotVector().size() == 9);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6}));

    s.setClosed(false);

    REQUIRE(!s.isClosed());
    REQUIRE(s.getNumberOfControlPoints() == 5);
    REQUIRE(s.getKnotVector().size() == 9);
    REQUIRE(compareKnots(s.getKnotVector(), {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 2.0, 2.0, 2.0}));
}

namespace {
// Helper function to compare vectors with tolerance
bool vectorsEqual(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-10) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::fabs(a[i] - b[i]) > tol) return false;
    }
    return true;
}
}

#include <iostream>
TEST_CASE("RS_Spline AddControlPointUniformOpen") {
  std::cout<<"line "<<__LINE__<<std::endl;
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    spline.addControlPoint(RS_Vector(0,0));
    spline.addControlPoint(RS_Vector(1,0));
    spline.addControlPoint(RS_Vector(2,0));
    spline.addControlPoint(RS_Vector(3,0));
    auto knots = spline.getKnotVector();
    REQUIRE(knots.size() == 8);
    std::vector<double> expected = {0,0,0,0,1,1,1,1};
    REQUIRE(vectorsEqual(knots, expected));

    spline.addControlPoint(RS_Vector(4,0));
    knots = spline.getKnotVector();
    REQUIRE(knots.size() == 9);
    expected = {0,0,0,0,1,2,2,2,2};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline AddControlPointCustomOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 4; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    std::vector<double> custom_knots = {0,1,2,3,4,5,6,7};
    spline.setKnotVector(custom_knots);
    auto knots = spline.getKnotVector();
    REQUIRE(vectorsEqual(knots, custom_knots));

    spline.addControlPoint(RS_Vector(4,0));
    knots = spline.getKnotVector();
    REQUIRE(knots.size() == 9);
    // fallback_delta = average =1
    std::vector<double> expected = {0,1,2,3,4,5,6,7,8};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline InsertControlPointUniformOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    spline.addControlPoint(RS_Vector(0,0));
    spline.addControlPoint(RS_Vector(1,0));
    spline.addControlPoint(RS_Vector(2,0));
    spline.addControlPoint(RS_Vector(3,0));
    auto knots = spline.getKnotVector();
    std::vector<double> expected = {0,0,0,0,1,1,1,1};
    REQUIRE(vectorsEqual(knots, expected));

    spline.insertControlPoint(2, RS_Vector(1.5,0.));
    knots = spline.getKnotVector();
    REQUIRE(knots.size() == 9);
    expected = {0,0,0,0,1,2,2,2,2};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline InsertControlPointCustomOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 4; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    std::vector<double> custom_knots = {0,1,2,3,4,5,6,7};
    spline.setKnotVector(custom_knots);

    spline.insertControlPoint(2, RS_Vector(1.5,0));
    auto knots = spline.getKnotVector();
    REQUIRE(knots.size() == 9);
    // insert at pos = 2 + 3 =5, new_k = (4 + 5)/2 =4.5
    std::vector<double> expected = {0,1,2,3,4,4.5,5,6,7};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline RemoveControlPointUniformOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 5; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    auto knots = spline.getKnotVector();
    std::vector<double> expected = {0,0,0,0,1,2,2,2,2};
    REQUIRE(vectorsEqual(knots, expected));

    spline.removeControlPoint(2);
    knots = spline.getKnotVector();
    REQUIRE(knots.size() == 8);
    expected = {0,0,0,0,1,1,1,1};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline RemoveControlPointCustomOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 5; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    std::vector<double> custom_knots = {0,1,2,3,4,5,6,7,8};
    spline.setKnotVector(custom_knots);

    spline.removeControlPoint(2);
    auto knots = spline.getKnotVector();
    REQUIRE(knots.size() == 8);
    // remove at 2 + 3 =5, remove knots[5]=5, so {0,1,2,3,4,6,7,8}
    std::vector<double> expected = {0,1,2,3,4,6,7,8};
    REQUIRE(vectorsEqual(knots, expected));
}

TEST_CASE("RS_Spline SetClosedUniformOpen") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 4; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    auto open_knots = spline.getKnotVector();
    std::vector<double> expected_open = {0,0,0,0,1,1,1,1};
    REQUIRE(vectorsEqual(open_knots, expected_open));

    spline.setClosed(true);
    auto closed_knots = spline.getKnotVector();
    REQUIRE(closed_knots.size() == 8); // unwrapped
    // But internal data.knotslist has 11
    // For clamped, period=1, start_idx=4, append openKnots[4+i] +1 =1+1=2 for i=0,1,2
    std::vector<double> expected_closed = {0,0,0,0,1,1,1,1,2,2,2};
    REQUIRE(vectorsEqual(spline.getData().knotslist, expected_closed));
    // getKnotVector returns unwrapped, first 8

    spline.setClosed(false);
    auto restored_knots = spline.getKnotVector();
    REQUIRE(vectorsEqual(restored_knots, expected_open));
}

TEST_CASE("RS_Spline SetClosedCustomNonClamped") {
    RS_Spline spline(nullptr, RS_SplineData(3, false));
    for (int i = 0; i < 4; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    std::vector<double> custom_knots = {0,1,2,3,4,5,6,7};
    spline.setKnotVector(custom_knots);

    spline.setClosed(true);
    // not clamped, period=7, start_idx=1, append openKnots[1+i] +7 =1+7,2+7,3+7=8,9,10
    std::vector<double> expected_closed = {0,1,2,3,4,5,6,7,8,9,10};
    REQUIRE(vectorsEqual(spline.getData().knotslist, expected_closed));

    spline.setClosed(false);
    auto restored = spline.getKnotVector();
    REQUIRE(vectorsEqual(restored, custom_knots));
}

TEST_CASE("RS_Spline AddControlPointUniformClosed") {
    RS_Spline spline(nullptr, RS_SplineData(3, true));
    for (int i = 0; i < 4; ++i) {
        spline.addControlPoint(RS_Vector(i,0));
    }
    auto knots = spline.getKnotVector();
    // knotu: delta=0.25, knots 0,0.25,0.5,0.75,1,1.25,1.5,1.75
    std::vector<double> expected = {0,0.25,0.5,0.75,1,1.25,1.5,1.75};
    REQUIRE(vectorsEqual(knots, expected, 1e-6));

    spline.addControlPoint(RS_Vector(4,0));
    knots = spline.getKnotVector();
    // new_size=5, delta=0.2, 0,0.2,0.4,0.6,0.8,1,1.2,1.4,1.6,1.8
    expected = {0,0.2,0.4,0.6,0.8,1,1.2,1.4,1.6,1.8};
    REQUIRE(vectorsEqual(knots, expected, 1e-6));
}

// Helper to compare vectors with tolerance
bool vectorsEqual(const RS_Vector& a, const RS_Vector& b, double tol = 1e-6) {
    return fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol;
}

// Helper to compare knot vectors with tolerance
bool knotsEqual(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-6) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (fabs(a[i] - b[i]) > tol) return false;
    }
    return true;
}

TEST_CASE("RS_Spline Insert Open Uniform Beginning", "[spline]") {
    RS_SplineData data(3, false);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2), RS_Vector(3,3)};
    for (const auto& p : points) {
        spline.addControlPoint(p);
    }
    std::vector<double> expected_knots = spline.knot(4, 4);
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));

    spline.insertControlPoint(0, RS_Vector(-1,-1));

    REQUIRE(spline.getNumberOfControlPoints() == 5);
    REQUIRE(vectorsEqual(spline.getControlPoints()[0], RS_Vector(-1,-1)));
    expected_knots = spline.knot(5, 4);
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));
}

TEST_CASE("RS_Spline Insert Open Custom Middle", "[spline]") {
    RS_SplineData data(3, false);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2), RS_Vector(3,3)};
    for (const auto& p : points) {
        spline.addControlPointRaw(p);
    }
    std::vector<double> custom_knots = {0.0, 0.1, 0.3, 0.6, 1.0, 1.0, 1.0, 1.0};
    spline.setKnotVector(custom_knots);

    double fallback_delta = (0.1 + 0.2 + 0.3 + 0.4) / 4.0; // 0.25
    size_t pos = 2 + 1; // 3
    double last = custom_knots[2]; // 0.3
    double new_knot = last + fallback_delta; // 0.55
    std::vector<double> expected_knots;
    expected_knots.insert(expected_knots.end(), custom_knots.begin(), custom_knots.begin() + 3);
    expected_knots.push_back(new_knot);
    for (size_t i = 3; i < custom_knots.size(); ++i) {
        expected_knots.push_back(custom_knots[i] + fallback_delta);
    }

    spline.insertControlPoint(2, RS_Vector(1.5,1.5));

    REQUIRE(spline.getNumberOfControlPoints() == 5);
    REQUIRE(vectorsEqual(spline.getControlPoints()[2], RS_Vector(1.5,1.5)));
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));
}

TEST_CASE("RS_Spline Insert Closed Uniform End", "[spline]") {
    RS_SplineData data(3, true);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2), RS_Vector(3,3)};
    for (const auto& p : points) {
        spline.addControlPoint(p);
    }
    spline.setClosed(true);

    spline.insertControlPoint(4, RS_Vector(4,4));

    REQUIRE(spline.getNumberOfControlPoints() == 5);
    REQUIRE(vectorsEqual(spline.getControlPoints()[4], RS_Vector(4,4)));
    std::vector<double> expected_knots = spline.knotu(5, 4);
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));
}

TEST_CASE("RS_Spline Remove Open Custom Beginning", "[spline]") {
    RS_SplineData data(3, false);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2), RS_Vector(3,3), RS_Vector(4,4)};
    for (const auto& p : points) {
        spline.addControlPointRaw(p);
    }
    std::vector<double> custom_knots = {0.0, 0.1, 0.3, 0.6, 1.0, 1.2, 1.2, 1.2, 1.2};
    spline.setKnotVector(custom_knots);

    double fallback_delta = (0.1 + 0.2 + 0.3 + 0.4 + 0.2) / 5.0; // 0.24
    size_t remove_pos = 0 + 1; // 1
    std::vector<double> expected_knots;
    expected_knots.insert(expected_knots.end(), custom_knots.begin(), custom_knots.begin() + 1);
    for (size_t i = 2; i < custom_knots.size(); ++i) {
        expected_knots.push_back(custom_knots[i] - fallback_delta);
    }

    spline.removeControlPoint(0);

    REQUIRE(spline.getNumberOfControlPoints() == 4);
    REQUIRE(vectorsEqual(spline.getControlPoints()[0], RS_Vector(1,1)));
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));
}

TEST_CASE("RS_Spline Remove Closed Below Min", "[spline]") {
    RS_SplineData data(3, true);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2)};
    for (const auto& p : points) {
        spline.addControlPoint(p);
    }
    spline.setClosed(true);
    std::vector<RS_Vector> original_points = spline.getControlPoints();
    std::vector<double> original_knots = spline.getKnotVector();

    spline.removeControlPoint(1);

    REQUIRE(spline.getNumberOfControlPoints() == 3);
    REQUIRE(spline.getControlPoints() == original_points);
    REQUIRE(knotsEqual(spline.getKnotVector(), original_knots));
}

TEST_CASE("RS_Spline Remove Open Uniform End", "[spline]") {
    RS_SplineData data(3, false);
    RS_Spline spline(nullptr, data);
    std::vector<RS_Vector> points = {RS_Vector(0,0), RS_Vector(1,1), RS_Vector(2,2), RS_Vector(3,3), RS_Vector(4,4)};
    for (const auto& p : points) {
        spline.addControlPoint(p);
    }

    spline.removeControlPoint(4);

    REQUIRE(spline.getNumberOfControlPoints() == 4);
    REQUIRE(vectorsEqual(spline.getControlPoints()[3], RS_Vector(3,3)));
    std::vector<double> expected_knots = spline.knot(4, 4);
    REQUIRE(knotsEqual(spline.getKnotVector(), expected_knots));
}

// Add more tests as needed, e.g., for custom closed, weights, etc.
