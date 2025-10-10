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

