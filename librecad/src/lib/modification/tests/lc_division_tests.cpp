/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for LC_Division
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <memory>

#include "lc_division.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_vector.h"

namespace {

constexpr double HALF_TURN = 3.14159265358979323846;

void addEndpointCutters(RS_EntityContainer& container) {
    container.addEntity(new RS_Line{&container, RS_Vector{0.0, -5.0}, RS_Vector{0.0, 5.0}});
    container.addEntity(new RS_Line{&container, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
}

void addArcEndpointCutters(RS_EntityContainer& container) {
    container.addEntity(new RS_Line{&container, RS_Vector{-10.0, -5.0}, RS_Vector{-10.0, 5.0}});
    container.addEntity(new RS_Line{&container, RS_Vector{10.0, -5.0}, RS_Vector{10.0, 5.0}});
}

} // namespace

TEST_CASE("LC_Division identifies an unbounded selection as the whole entity",
          "[lc_division][break_divide][whole_entity]") {
    RS_EntityContainer emptyContainer;
    LC_Division division{&emptyContainer};

    SECTION("line") {
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
        CHECK(data->snapSegmentStart == line.getStartpoint());
        CHECK(data->snapSegmentEnd == line.getEndpoint());
    }

    SECTION("arc") {
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, false}};
        const std::unique_ptr<LC_Division::ArcSegmentData> data{
            division.findArcSegmentBetweenIntersections(&arc, RS_Vector{0.0, 10.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }

    SECTION("circle") {
        RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 10.0}};
        const std::unique_ptr<LC_Division::CircleSegmentData> data{
            division.findCircleSegmentBetweenIntersections(&circle, RS_Vector{10.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }
}

TEST_CASE("endpoint-only intersections do not masquerade as split boundaries",
          "[lc_division][break_divide][whole_entity]") {
    SECTION("line") {
        RS_EntityContainer container;
        addEndpointCutters(container);
        LC_Division division{&container};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }

    SECTION("reversed arc") {
        RS_EntityContainer container;
        addArcEndpointCutters(container);
        LC_Division division{&container};
        RS_Arc arc{nullptr, RS_ArcData{RS_Vector{0.0, 0.0}, 10.0, 0.0, HALF_TURN, true}};

        const std::unique_ptr<LC_Division::ArcSegmentData> data{
            division.findArcSegmentBetweenIntersections(&arc, RS_Vector{0.0, -10.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_ENTIRE);
    }
}

TEST_CASE("LC_Division keeps ordinary partial-segment behavior",
          "[lc_division][break_divide]") {
    SECTION("whole entity selection must be explicitly enabled") {
        RS_EntityContainer emptyContainer;
        LC_Division division{&emptyContainer};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{5.0, 0.0}, false)};

        CHECK(data == nullptr);
    }

    SECTION("an interior intersection still bounds a partial segment") {
        RS_EntityContainer container;
        container.addEntity(new RS_Line{&container, RS_Vector{5.0, -5.0}, RS_Vector{5.0, 5.0}});
        LC_Division division{&container};
        RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};

        const std::unique_ptr<LC_Division::LineSegmentData> data{
            division.findLineSegmentBetweenIntersections(&line, RS_Vector{2.0, 0.0}, true)};

        REQUIRE(data != nullptr);
        CHECK(data->segmentDisposition == LC_Division::SEGMENT_TO_START);
    }
}
