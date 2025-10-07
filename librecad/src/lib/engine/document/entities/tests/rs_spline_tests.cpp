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
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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

    REQUIRE(points[0].x == Catch::Approx(1.0));
    REQUIRE(points[0].y == Catch::Approx(0.6666666666666666));
    REQUIRE(points[1].x == Catch::Approx(1.032258064516129));
    REQUIRE(points[1].y == Catch::Approx(0.6656484620634867));
    REQUIRE(points[2].x == Catch::Approx(1.064516129032258));
    REQUIRE(points[2].y == Catch::Approx(0.6626833607465342));
    REQUIRE(points[3].x == Catch::Approx(1.096774193548387));
    REQUIRE(points[3].y == Catch::Approx(0.6579056314546899));
    REQUIRE(points[4].x == Catch::Approx(1.129032258064516));
    REQUIRE(points[4].y == Catch::Approx(0.6514495429268347));

    REQUIRE(points[27].x == Catch::Approx(1.870967741935484));
    REQUIRE(points[27].y == Catch::Approx(0.34855045707316523));
    REQUIRE(points[28].x == Catch::Approx(1.903225806451613));
    REQUIRE(points[28].y == Catch::Approx(0.3420943685453101));
    REQUIRE(points[29].x == Catch::Approx(1.935483870967742));
    REQUIRE(points[29].y == Catch::Approx(0.33731663925346583));
    REQUIRE(points[30].x == Catch::Approx(1.9677419354838708));
    REQUIRE(points[30].y == Catch::Approx(0.3343515379365133));
    REQUIRE(points[31].x == Catch::Approx(2.0));
    REQUIRE(points[31].y == Catch::Approx(0.3333333333333333));

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

    REQUIRE(points[0].x == Catch::Approx(1.0));
    REQUIRE(points[0].y == Catch::Approx(0.6666666666666666));
    REQUIRE(points[1].x == Catch::Approx(1.129032258064516));
    REQUIRE(points[1].y == Catch::Approx(0.6514495429268347));
    REQUIRE(points[2].x == Catch::Approx(1.258064516129032));
    REQUIRE(points[2].y == Catch::Approx(0.6115269712329228));
    REQUIRE(points[3].x == Catch::Approx(1.3870967741935483));
    REQUIRE(points[3].y == Catch::Approx(0.5554921508733062));
    REQUIRE(points[4].x == Catch::Approx(1.5161290322580643));
    REQUIRE(points[4].y == Catch::Approx(0.49193828113636107));

    REQUIRE(points[27].x == Catch::Approx(0.5755317601512763));
    REQUIRE(points[27].y == Catch::Approx(0.4919382811363613));
    REQUIRE(points[28].x == Catch::Approx(0.6515726226041425));
    REQUIRE(points[28].y == Catch::Approx(0.5554921508733064));
    REQUIRE(points[29].x == Catch::Approx(0.7533930829221356));
    REQUIRE(points[29].y == Catch::Approx(0.6115269712329228));
    REQUIRE(points[30].x == Catch::Approx(0.8723999418168799));
    REQUIRE(points[30].y == Catch::Approx(0.6514495429268347));
    REQUIRE(points[31].x == Catch::Approx(1.0));
    REQUIRE(points[31].y == Catch::Approx(0.6666666666666666));

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

    REQUIRE(points[0].x == Catch::Approx(1.0));
    REQUIRE(points[0].y == Catch::Approx(0.6666666666666666));
    REQUIRE(points[31].x == Catch::Approx(2.0));
    REQUIRE(points[31].y == Catch::Approx(0.3333333333333333));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[1].x == Catch::Approx(0.09677419354838711));
    REQUIRE(points[1].y == Catch::Approx(0.09066496592930752));
    REQUIRE(points[2].x == Catch::Approx(0.19354838709677422));
    REQUIRE(points[2].y == Catch::Approx(0.16964855157597933));
    REQUIRE(points[3].x == Catch::Approx(0.29032258064516125));
    REQUIRE(points[3].y == Catch::Approx(0.2377563693733006));
    REQUIRE(points[4].x == Catch::Approx(0.3870967741935483));
    REQUIRE(points[4].y == Catch::Approx(0.2957940317545567));

    REQUIRE(points[27].x == Catch::Approx(2.612903225806451));
    REQUIRE(points[27].y == Catch::Approx(0.7042059682454431));
    REQUIRE(points[28].x == Catch::Approx(2.7096774193548385));
    REQUIRE(points[28].y == Catch::Approx(0.7622436306266993));
    REQUIRE(points[29].x == Catch::Approx(2.806451612903225));
    REQUIRE(points[29].y == Catch::Approx(0.8303514484240205));
    REQUIRE(points[30].x == Catch::Approx(2.903225806451613));
    REQUIRE(points[30].y == Catch::Approx(0.9093350340706926));
    REQUIRE(points[31].x == Catch::Approx(3.0));
    REQUIRE(points[31].y == Catch::Approx(1.0));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[1].x == Catch::Approx(0.363017913687579));
    REQUIRE(points[1].y == Catch::Approx(0.31629239255703623));
    REQUIRE(points[2].x == Catch::Approx(0.6814586060667092));
    REQUIRE(points[2].y == Catch::Approx(0.5074463204771016));
    REQUIRE(points[3].x == Catch::Approx(0.9606928266926252));
    REQUIRE(points[3].y == Catch::Approx(0.5981672317142761));
    REQUIRE(points[4].x == Catch::Approx(1.206091325120562));
    REQUIRE(points[4].y == Catch::Approx(0.6131605742226399));

    REQUIRE(points[27].x == Catch::Approx(0.5755317601512762));
    REQUIRE(points[27].y == Catch::Approx(0.49193828113636107));
    REQUIRE(points[28].x == Catch::Approx(0.6515726226041421));
    REQUIRE(points[28].y == Catch::Approx(0.5554921508733062));
    REQUIRE(points[29].x == Catch::Approx(0.7533930829221351));
    REQUIRE(points[29].y == Catch::Approx(0.6115269712329225));
    REQUIRE(points[30].x == Catch::Approx(0.8723999418168799));
    REQUIRE(points[30].y == Catch::Approx(0.6514495429268347));
    REQUIRE(points[31].x == Catch::Approx(1.0));
    REQUIRE(points[31].y == Catch::Approx(0.6666666666666666));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[31].x == Catch::Approx(3.0));
    REQUIRE(points[31].y == Catch::Approx(1.0));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[1].x == Catch::Approx(0.09573883238412793));
    REQUIRE(points[1].y == Catch::Approx(0.09165109522264367));
    REQUIRE(points[2].x == Catch::Approx(0.18942782868800795));
    REQUIRE(points[2].y == Catch::Approx(0.1733752550173617));
    REQUIRE(points[3].x == Catch::Approx(0.2810983182840455));
    REQUIRE(points[3].y == Catch::Approx(0.24565137121949582));
    REQUIRE(points[4].x == Catch::Approx(0.37078163054464625));
    REQUIRE(points[4].y == Catch::Approx(0.30895833566438785));

    REQUIRE(points[27].x == Catch::Approx(1.9570944244906183));
    REQUIRE(points[27].y == Catch::Approx(0.3909838541841496));
    REQUIRE(points[28].x == Catch::Approx(2.0084842924223945));
    REQUIRE(points[28].y == Catch::Approx(0.38269872705776165));
    REQUIRE(points[29].x == Catch::Approx(2.0586702173288725));
    REQUIRE(points[29].y == Catch::Approx(0.3774167440576759));
    REQUIRE(points[30].x == Catch::Approx(2.1076835285824576));
    REQUIRE(points[30].y == Catch::Approx(0.375616797019234));
    REQUIRE(points[31].x == Catch::Approx(2.155555555555556));
    REQUIRE(points[31].y == Catch::Approx(0.37777777777777777));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[1].x == Catch::Approx(0.2810983182840455));
    REQUIRE(points[1].y == Catch::Approx(0.24565137121949582));
    REQUIRE(points[2].x == Catch::Approx(0.5443120405491589));
    REQUIRE(points[2].y == Catch::Approx(0.4105803766238124));
    REQUIRE(points[3].x == Catch::Approx(0.7904870598502902));
    REQUIRE(points[3].y == Catch::Approx(0.5077170957671779));
    REQUIRE(points[4].x == Catch::Approx(1.0204692692423885));
    REQUIRE(points[4].y == Catch::Approx(0.5499916082038199));

    REQUIRE(points[27].x == Catch::Approx(0.5232687724480548));
    REQUIRE(points[27].y == Catch::Approx(0.4436272699808666));
    REQUIRE(points[28].x == Catch::Approx(0.5660535060924439));
    REQUIRE(points[28].y == Catch::Approx(0.5186767815783289));
    REQUIRE(points[29].x == Catch::Approx(0.6521801886475778));
    REQUIRE(points[29].y == Catch::Approx(0.5964653754489608));
    REQUIRE(points[30].x == Catch::Approx(0.7680339699909369));
    REQUIRE(points[30].y == Catch::Approx(0.6619280990903292));
    REQUIRE(points[31].x == Catch::Approx(0.9000000000000001));
    REQUIRE(points[31].y == Catch::Approx(0.7000000000000002));

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

    REQUIRE(points[0].x == Catch::Approx(0.0));
    REQUIRE(points[0].y == Catch::Approx(0.0));
    REQUIRE(points[31].x == Catch::Approx(2.155555555555556));
    REQUIRE(points[31].y == Catch::Approx(0.37777777777777777));

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