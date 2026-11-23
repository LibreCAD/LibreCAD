/*
 * This file is part of the LibreCAD project.
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 * Licensed under GPL-2.0-or-later.
 */
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>

#include "lc_curvesplit.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"

TEST_CASE("curve splitting keeps line endpoints", "[modify][trim][batch]") {
    RS_Line source{RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    const auto segment = lc::curvesplit::subCurve(source, 0.2, {2.0, 0.0}, 0.8, {8.0, 0.0});
    REQUIRE(segment != nullptr);
    CHECK(segment->getStartpoint().distanceTo({2.0, 0.0}) < 1e-8);
    CHECK(segment->getEndpoint().distanceTo({8.0, 0.0}) < 1e-8);
    CHECK(lc::curvesplit::paramOf(source, {5.0, 0.0}) == Catch::Approx(0.5));
}

TEST_CASE("curve splitting follows a reversed arc through the angle seam", "[modify][trim][batch]") {
    RS_Arc source{RS_ArcData{{0.0, 0.0}, 10.0, 0.2, 6.0, true}};
    const RS_Vector start = source.getCenter() + RS_Vector::polar(10.0, 0.1);
    const RS_Vector end = source.getCenter() + RS_Vector::polar(10.0, 6.1);
    const auto segment = lc::curvesplit::subCurve(source, 0.1, start, 0.2, end);
    REQUIRE(segment != nullptr);
    CHECK(static_cast<RS_Arc*>(segment.get())->isReversed());
    CHECK(segment->getStartpoint().distanceTo(start) < 1e-7);
    CHECK(segment->getEndpoint().distanceTo(end) < 1e-7);
}

TEST_CASE("a circle is split into a counterclockwise arc", "[modify][trim][batch]") {
    RS_Circle source{RS_CircleData{{0.0, 0.0}, 5.0}};
    const RS_Vector start{5.0, 0.0};
    const RS_Vector end{0.0, 5.0};
    const auto segment = lc::curvesplit::subCurve(source, 0.0, start, M_PI_2, end);
    REQUIRE(segment != nullptr);
    CHECK(segment->rtti() == RS2::EntityArc);
    CHECK(segment->getStartpoint().distanceTo(start) < 1e-7);
    CHECK(segment->getEndpoint().distanceTo(end) < 1e-7);
}

TEST_CASE("ellipse arc splitting preserves its direction", "[modify][trim][batch]") {
    RS_Ellipse source{nullptr, RS_EllipseData{{0.0, 0.0}, {10.0, 0.0}, 0.5, 1.5, 0.1, true}};
    const RS_Vector start = source.getEllipsePoint(1.1);
    const RS_Vector end = source.getEllipsePoint(0.4);
    const auto segment = lc::curvesplit::subCurve(source, 0.4, start, 1.1, end);
    REQUIRE(segment != nullptr);
    const auto* ellipseSegment = static_cast<const RS_Ellipse*>(segment.get());
    CHECK(ellipseSegment->isReversed());
    CHECK(segment->getStartpoint().distanceTo(start) < 1e-7);
    CHECK(segment->getEndpoint().distanceTo(end) < 1e-7);
    CHECK(lc::curvesplit::paramOf(source, start) == Catch::Approx(0.4));
}
