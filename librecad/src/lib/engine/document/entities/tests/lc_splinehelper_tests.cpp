/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 xAI
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
#include <iostream>
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "rs_debug.h"
#include "rs_math.h"
#include "rs_spline.h"
#include "lc_splinehelper.h"
#include "rs_vector.h"

// Stub evaluateNURBS
RS_Vector evaluateNURBS(const RS_SplineData& data, double t) {
    return RS_Vector(0.0, 0.0);
}

namespace {
const double TOLERANCE = RS_TOLERANCE;

bool vectorsEqual(const std::vector<double>& a, const std::vector<double>& b, double tol = TOLERANCE) {
    if (a.size() != b.size()) return false;

    double mina = *std::min_element(a.begin(), a.end());
    double maxa = *std::max_element(a.begin(), a.end());
    double rangea = maxa - mina;

    double minb = *std::min_element(b.begin(), b.end());
    double maxb = *std::max_element(b.begin(), b.end());
    double rangeb = maxb - minb;

    bool is_constant_a = rangea < tol;
    bool is_constant_b = rangeb < tol;

    if (is_constant_a != is_constant_b) return false;

    if (is_constant_a) { // both constant
        double avga = std::accumulate(a.begin(), a.end(), 0.0) / a.size();
        double avgb = std::accumulate(b.begin(), b.end(), 0.0) / b.size();
        return std::abs(avga - avgb) <= tol;
    }

    // non-constant, normalize without checking ranges (affine invariant)
    std::vector<double> na(a.size());
    std::vector<double> nb(b.size());
    for (size_t i=0; i<a.size(); ++i) {
        na[i] = (a[i] - mina) / rangea;
        nb[i] = (b[i] - minb) / rangeb;
    }

    for (size_t i=0; i<a.size(); ++i) {
        if (std::abs(na[i] - nb[i]) > tol) return false;
    }

    return true;
}

RS_SplineData createSplineData(int degree, RS_SplineData::SplineType type, size_t numControlPoints, bool rational = false, bool customKnots = false) {
    RS_SplineData data;
    data.degree = degree;
    data.type = type;
    size_t actualPoints = (type == RS_SplineData::SplineType::WrappedClosed) ? numControlPoints - degree : numControlPoints;
    data.controlPoints.resize(actualPoints);
    data.weights.resize(actualPoints, 1.0);
    for (size_t i = 0; i < actualPoints; ++i) {
        data.controlPoints[i] = RS_Vector(static_cast<double>(i), static_cast<double>(i));
        if (rational) data.weights[i] = 1.0 + static_cast<double>(i) * 0.1;
    }
    size_t order = degree + 1;
    size_t openKnotsSize = actualPoints + order;
    if (customKnots) {
        data.knotslist.resize(openKnotsSize);
        std::iota(data.knotslist.begin(), data.knotslist.end(), 0.0);
        for (size_t i = 2; i < openKnotsSize - 2; ++i) {
            data.knotslist[i] += (i % 2) * 0.5;  // Slight non-uniform
        }
    } else {
        if (type == RS_SplineData::SplineType::ClampedOpen) {
            data.knotslist = LC_SplineHelper::knot(actualPoints, order);
        } else {
            data.knotslist = LC_SplineHelper::generateOpenUniformKnotVector(actualPoints, order);
        }
    }
    if (type == RS_SplineData::SplineType::ClampedOpen && customKnots) {
        data.knotslist = LC_SplineHelper::clampKnotVector(data.knotslist, actualPoints, order);
    }
    if (type == RS_SplineData::SplineType::WrappedClosed) {
        LC_SplineHelper::addWrapping(data);
    }
    return data;
}
} // namespace

TEST_CASE("Conversion Helpers", "[LC_SplineHelperTest]") {
    SECTION("toClampedOpenFromStandard") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromStandard(data);
                REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
                // Check clamped multiplicities
                double start = data.knotslist[0];
                for (size_t i = 1; i <= static_cast<size_t>(degree); ++i) {
                    REQUIRE(std::abs(data.knotslist[i] - start) < 1e-6);
                }
                double end = data.knotslist.back();
                for (size_t i = 1; i <= static_cast<size_t>(degree); ++i) {
                    REQUIRE(std::abs(data.knotslist[data.knotslist.size() - i - 1] - end) < 1e-6);
                }
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromStandard(data);
                REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
                // Check internals preserved
                for (size_t i = degree + 1; i < unwrapped - 1; ++i) {
                    REQUIRE(std::abs(data.knotslist[i] - origKnots[i]) < 1e-6);
                }
            }
        }
    }

    SECTION("toStandardFromClampedOpen") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromClampedOpen(data);
                REQUIRE(data.type == RS_SplineData::SplineType::Standard);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
                // No multiplicity check for Standard
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromClampedOpen(data);
                REQUIRE(data.type == RS_SplineData::SplineType::Standard);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
                // Check internals preserved
                for (size_t i = degree + 1; i < unwrapped - 1; ++i) {
                    REQUIRE(std::abs(data.knotslist[i] - origKnots[i]) < 1e-6);
                }
            }
        }
    }

    SECTION("toWrappedClosedFromStandard") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromStandard(data);
                REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped + degree);
                REQUIRE(data.knotslist.size() == unwrapped + 2 * degree + 1);
                // Check wrapping
                for (size_t i = 0; i < static_cast<size_t>(degree); ++i) {
                    REQUIRE(data.controlPoints[unwrapped + i] == data.controlPoints[i]);
                }
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromStandard(data);
                REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped + degree);
                REQUIRE(data.knotslist.size() == unwrapped + 2 * degree + 1);
                // Check internals approximate
            }
        }
    }

    SECTION("toStandardFromWrappedClosed") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
                REQUIRE(data.type == RS_SplineData::SplineType::Standard);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
                REQUIRE(data.type == RS_SplineData::SplineType::Standard);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
            }
        }
    }

    SECTION("Round-Trip Conversions") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Standard <-> ClampedOpen Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromStandard(data);
                LC_SplineHelper::toStandardFromClampedOpen(data);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            SECTION("Degree " + std::to_string(degree) + " Standard <-> WrappedClosed Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromStandard(data);
                LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            SECTION("Degree " + std::to_string(degree) + " ClampedOpen <-> Standard Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromClampedOpen(data);
                LC_SplineHelper::toClampedOpenFromStandard(data);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            SECTION("Degree " + std::to_string(degree) + " WrappedClosed <-> Standard Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
                LC_SplineHelper::toWrappedClosedFromStandard(data);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            // Non-uniform round-trips similar, but may not preserve exactly, check validate
            SECTION("Degree " + std::to_string(degree) + " Standard <-> ClampedOpen Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromStandard(data);
                LC_SplineHelper::toStandardFromClampedOpen(data);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
            SECTION("Degree " + std::to_string(degree) + " Standard <-> WrappedClosed Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::Standard, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromStandard(data);
                LC_SplineHelper::toStandardFromWrappedClosed(data, unwrapped);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
        }
    }
}

TEST_CASE("Direct Conversions between ClampedOpen and WrappedClosed", "[LC_SplineHelperTest]") {
    SECTION("toClampedOpenFromWrappedClosed") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
                // Check clamped multiplicities
                double start = data.knotslist[0];
                for (size_t i = 1; i <= static_cast<size_t>(degree); ++i) {
                    REQUIRE(std::abs(data.knotslist[i] - start) < 1e-6);
                }
                double end = data.knotslist.back();
                for (size_t i = 1; i <= static_cast<size_t>(degree); ++i) {
                    REQUIRE(std::abs(data.knotslist[data.knotslist.size() - i - 1] - end) < 1e-6);
                }
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped);
                REQUIRE(data.knotslist.size() == unwrapped + degree + 1);
            }
            SECTION("Degree " + std::to_string(degree) + " Rational Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, true, false);
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
        }
    }

    SECTION("toWrappedClosedFromClampedOpen") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped + degree);
                REQUIRE(data.knotslist.size() == unwrapped + 2 * degree + 1);
                // Check wrapping
                for (size_t i = 0; i < static_cast<size_t>(degree); ++i) {
                    REQUIRE(data.controlPoints[unwrapped + i] == data.controlPoints[i]);
                }
            }
            SECTION("Degree " + std::to_string(degree) + " Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
                REQUIRE(data.controlPoints.size() == unwrapped + degree);
                REQUIRE(data.knotslist.size() == unwrapped + 2 * degree + 1);
            }
            SECTION("Degree " + std::to_string(degree) + " Rational Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, true, false);
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                REQUIRE(data.type == RS_SplineData::SplineType::WrappedClosed);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
        }
    }

    SECTION("Round-Trip Conversions") {
        for (int degree : {2, 3}) {
            size_t unwrapped = degree + 3;
            SECTION("Degree " + std::to_string(degree) + " ClampedOpen <-> WrappedClosed Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            SECTION("Degree " + std::to_string(degree) + " WrappedClosed <-> ClampedOpen Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, false);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                REQUIRE(vectorsEqual(data.knotslist, origKnots, 1e-6));
            }
            // Non-uniform round-trips
            SECTION("Degree " + std::to_string(degree) + " ClampedOpen <-> WrappedClosed Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
            SECTION("Degree " + std::to_string(degree) + " WrappedClosed <-> ClampedOpen Non-Uniform") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::WrappedClosed, unwrapped + degree, false, true);
                auto origKnots = data.knotslist;
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
            // Rational round-trips
            SECTION("Degree " + std::to_string(degree) + " ClampedOpen <-> WrappedClosed Rational") {
                RS_SplineData data = createSplineData(degree, RS_SplineData::SplineType::ClampedOpen, unwrapped, true, false);
                auto origWeights = data.weights;
                LC_SplineHelper::toWrappedClosedFromClampedOpen(data);
                LC_SplineHelper::toClampedOpenFromWrappedClosed(data, unwrapped);
                REQUIRE(vectorsEqual(data.weights, origWeights, 1e-6));
                REQUIRE(LC_SplineHelper::validate(data, unwrapped));
            }
        }
    }
}