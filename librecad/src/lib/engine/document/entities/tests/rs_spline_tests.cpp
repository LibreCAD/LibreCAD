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

TEST_CASE("RS_Spline Clamped Uniform Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    std::vector<double> original_knots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    s.setKnotVector(original_knots);
    s.update();

    std::vector<RS_Vector> points;
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected = {
        {0.0, 0.0},
        {0.09677419354838711, 0.09066496592930752},
        {0.19354838709677422, 0.16964855157597933},
        {0.29032258064516125, 0.2377563693733006},
        {0.3870967741935483, 0.2957940317545567}
    };
    REQUIRE(comparePoints(start_points, start_expected));

    std::vector<RS_Vector> end_points(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected = {
        {2.612903225806451, 0.7042059682454431},
        {2.7096774193548385, 0.7622436306266993},
        {2.806451612903225, 0.8303514484240205},
        {2.903225806451613, 0.9093350340706926},
        {3.0, 1.0}
    };
    REQUIRE(comparePoints(end_points, end_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(3.0));
    REQUIRE(s.getMax().y == Catch::Approx(1.0));

    // Set to closed
    s.setClosed(true);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points_closed(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected_closed = {
        {0.0, 0.0},
        {0.363017913687579, 0.31629239255703623},
        {0.6814586060667092, 0.5074463204771016},
        {0.9606928266926252, 0.5981672317142761},
        {1.206091325120562, 0.6131605742226399}
    };
    REQUIRE(comparePoints(start_points_closed, start_expected_closed));

    std::vector<RS_Vector> end_points_closed(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected_closed = {
        {0.5755317601512762, 0.49193828113636107},
        {0.6515726226041421, 0.5554921508733062},
        {0.7533930829221351, 0.6115269712329225},
        {0.8723999418168799, 0.6514495429268347},
        {1.0, 0.6666666666666666}
    };
    REQUIRE(comparePoints(end_points_closed, end_expected_closed));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.4673782910));
    REQUIRE(s.getMax().y == Catch::Approx(0.6666666667));

    // Round-trip back to open
    s.setClosed(false);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> rt_points = {points[0], points[31]};
    std::vector<std::pair<double, double>> rt_expected = {
        {0.0, 0.0},
        {3.0, 1.0}
    };
    REQUIRE(comparePoints(rt_points, rt_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(3.0));
    REQUIRE(s.getMax().y == Catch::Approx(1.0));

    const auto& restored_knots = s.getKnotVector();
    REQUIRE(restored_knots.size() == original_knots.size());
    for (size_t i = 0; i < restored_knots.size(); ++i) {
        REQUIRE(restored_knots[i] == Catch::Approx(original_knots[i]));
    }
}

TEST_CASE("RS_Spline Non-Uniform Clamped Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    std::vector<double> original_knots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.5, 2.5, 3.0};
    s.setKnotVector(original_knots);
    s.update();

    std::vector<RS_Vector> points;
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected = {
        {0.0, 0.0},
        {0.09573883238412793, 0.09165109522264367},
        {0.18942782868800795, 0.1733752550173617},
        {0.2810983182840455, 0.24565137121949582},
        {0.37078163054464625, 0.30895833566438785}
    };
    REQUIRE(comparePoints(start_points, start_expected));

    std::vector<RS_Vector> end_points(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected = {
        {1.9570944244906183, 0.3909838541841496},
        {2.0084842924223945, 0.38269872705776165},
        {2.0586702173288725, 0.3774167440576759},
        {2.1076835285824576, 0.375616797019234},
        {2.155555555555556, 0.37777777777777777}
    };
    REQUIRE(comparePoints(end_points, end_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.1555555556));
    REQUIRE(s.getMax().y == Catch::Approx(0.5541263394));

    // Set to closed
    s.setClosed(true);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points_closed(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected_closed = {
        {0.0, 0.0},
        {0.2810983182840455, 0.24565137121949582},
        {0.5443120405491589, 0.4105803766238124},
        {0.7904870598502902, 0.5077170957671779},
        {1.0204692692423885, 0.5499916082038199}
    };
    REQUIRE(comparePoints(start_points_closed, start_expected_closed));

    std::vector<RS_Vector> end_points_closed(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected_closed = {
        {0.5232687724480548, 0.4436272699808666},
        {0.5660535060924439, 0.5186767815783289},
        {0.6521801886475778, 0.5964653754489608},
        {0.7680339699909369, 0.6619280990903292},
        {0.9000000000000001, 0.7000000000000002}
    };
    REQUIRE(comparePoints(end_points_closed, end_expected_closed));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.5115348036));
    REQUIRE(s.getMax().y == Catch::Approx(0.6924238864));

    // Round-trip back to open
    s.setClosed(false);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> rt_points = {points[0], points[31]};
    std::vector<std::pair<double, double>> rt_expected = {
        {0.0, 0.0},
        {2.155555555555556, 0.37777777777777777}
    };
    REQUIRE(comparePoints(rt_points, rt_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.1555555556));
    REQUIRE(s.getMax().y == Catch::Approx(0.5541263394));

    const auto& restored_knots = s.getKnotVector();
    REQUIRE(restored_knots.size() == original_knots.size());
    for (size_t i = 0; i < restored_knots.size(); ++i) {
        REQUIRE(restored_knots[i] == Catch::Approx(original_knots[i]));
    }
}

TEST_CASE("RS_Spline Non-Uniform Open Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    std::vector<double> original_knots = {0.0, 2.0, 3.0, 5.0, 6.0, 8.0, 9.0, 11.0};
    s.setKnotVector(original_knots);
    s.update();

    std::vector<RS_Vector> points;
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected = {
        {1.25, 0.5833333333},
        {1.2734387716, 0.5750465393},
        {1.2968851725, 0.5658671061},
        {1.3203468323, 0.5558789571},
        {1.3438313802, 0.5451660156}
    };
    REQUIRE(comparePoints(start_points, start_expected));

    std::vector<RS_Vector> end_points(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected = {
        {1.9341634115, 0.2888183594},
        {1.9606997172, 0.2872606913},
        {1.9874572754, 0.2871602376},
        {2.0144437154, 0.2886009216},
        {2.0416666667, 0.2916666667}
    };
    REQUIRE(comparePoints(end_points, end_expected));

    REQUIRE(s.getMin().x == Catch::Approx(1.25));
    REQUIRE(s.getMin().y == Catch::Approx(0.2871602376));
    REQUIRE(s.getMax().x == Catch::Approx(2.0416666667));
    REQUIRE(s.getMax().y == Catch::Approx(0.5833333333));

    // Set to closed
    s.setClosed(true);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    // Round-trip back to open
    s.setClosed(false);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> rt_points = {points[0], points[31]};
    std::vector<std::pair<double, double>> rt_expected = {
        {1.25, 0.5833333333},
        {2.0416666667, 0.2916666667}
    };
    REQUIRE(comparePoints(rt_points, rt_expected));

    REQUIRE(s.getMin().x == Catch::Approx(1.25));
    REQUIRE(s.getMin().y == Catch::Approx(0.2871602376));
    REQUIRE(s.getMax().x == Catch::Approx(2.0416666667));
    REQUIRE(s.getMax().y == Catch::Approx(0.5833333333));

    const auto& restored_knots = s.getKnotVector();
    REQUIRE(restored_knots.size() == original_knots.size());
    for (size_t i = 0; i < restored_knots.size(); ++i) {
        REQUIRE(restored_knots[i] == Catch::Approx(original_knots[i]));
    }
}

TEST_CASE("RS_Spline Internal Multiplicity Knots", "[spline]") {
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    std::vector<double> original_knots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 2.0, 3.0};
    s.setKnotVector(original_knots);
    s.update();

    std::vector<RS_Vector> points;
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> start_points(points.begin(), points.begin() + 5);
    std::vector<std::pair<double, double>> start_expected = {
        {0.0, 0.0},
        {0.0937347412, 0.0879974365},
        {0.1873779297, 0.1649169922},
        {0.2808380127, 0.2313995361},
        {0.3740234375, 0.2880859375}
    };
    REQUIRE(comparePoints(start_points, start_expected));

    std::vector<RS_Vector> end_points(points.begin() + 27, points.end());
    std::vector<std::pair<double, double>> end_expected = {
        {2.2900390625, 0.3759765625},
        {2.3466033936, 0.3960418701},
        {2.4005126953, 0.4229736328},
        {2.4516754150, 0.4574127197},
        {2.5, 0.5}
    };
    REQUIRE(comparePoints(end_points, end_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.5));
    REQUIRE(s.getMax().y == Catch::Approx(0.5));

    // Set to closed
    s.setClosed(true);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    // Round-trip back to open
    s.setClosed(false);
    s.update();
    points.clear();
    s.fillStrokePoints(8, points);
    REQUIRE(points.size() == 32);

    std::vector<RS_Vector> rt_points = {points[0], points[31]};
    std::vector<std::pair<double, double>> rt_expected = {
        {0.0, 0.0},
        {2.5, 0.5}
    };
    REQUIRE(comparePoints(rt_points, rt_expected));

    REQUIRE(s.getMin().x == Catch::Approx(0.0));
    REQUIRE(s.getMin().y == Catch::Approx(0.0));
    REQUIRE(s.getMax().x == Catch::Approx(2.5));
    REQUIRE(s.getMax().y == Catch::Approx(0.5));

    const auto& restored_knots = s.getKnotVector();
    REQUIRE(restored_knots.size() == original_knots.size());
    for (size_t i = 0; i < restored_knots.size(); ++i) {
        REQUIRE(restored_knots[i] == Catch::Approx(original_knots[i]));
    }
}

TEST_CASE("RS_Spline Edge Cases", "[spline]") {
    // Minimal open spline (degree 3, 4 points)
    RS_Spline s(nullptr, RS_SplineData(3, false));
    s.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    s.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    s.addControlPoint(RS_Vector(3.0, 1.0), 1.0);
    REQUIRE_NOTHROW(s.update());

    // Minimal closed spline (3 points)
    RS_Spline min_closed(nullptr, RS_SplineData(3, true));
    min_closed.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    min_closed.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    min_closed.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    REQUIRE_NOTHROW(min_closed.update());

    // Invalid: too few for open
    RS_Spline invalid_open(nullptr, RS_SplineData(3, false));
    invalid_open.addControlPoint(RS_Vector(0.0, 0.0), 1.0);
    invalid_open.addControlPoint(RS_Vector(1.0, 1.0), 1.0);
    invalid_open.addControlPoint(RS_Vector(2.0, 0.0), 1.0);
    REQUIRE(invalid_open.getNumberOfControlPoints() == 3);
    invalid_open.update(); // Should log warning, but no throw
    std::vector<RS_Vector> points;
    invalid_open.fillStrokePoints(8, points);
    REQUIRE(points.empty());

    // Invalid degree
    REQUIRE_THROWS_AS(RS_Spline(nullptr, RS_SplineData(4, false)), std::invalid_argument);
}