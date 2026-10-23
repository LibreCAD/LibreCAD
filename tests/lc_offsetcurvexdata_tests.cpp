/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include "lc_offsetcurvexdata.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

#include "intern/drw_textcodec.h"
#include "rs_line.h"

namespace xd = LC_OffsetCurveXData;
namespace {
using Tags = xd::XData;
using Tag = std::shared_ptr<DRW_Variant>;
Tag str(int code, std::string s) { return std::make_shared<DRW_Variant>(code, std::move(s)); }
Tag integer(int code, std::int32_t i) { return std::make_shared<DRW_Variant>(code, i); }
Tag real(int code, double d) { return std::make_shared<DRW_Variant>(code, d); }
Tag point(int code, double x, double y, double z = 0) {
    return std::make_shared<DRW_Variant>(code, DRW_Coord{x, y, z});
}
void append(Tags &to, const Tags &from) { to.insert(to.end(), from.begin(), from.end()); }
Tags deepTags(const Tags &input) {
    Tags snapshot;
    snapshot.reserve(input.size());
    for (const auto &tag : input)
        snapshot.push_back(tag ? std::make_shared<DRW_Variant>(*tag) : nullptr);
    return snapshot;
}
std::uint64_t bits(double value) {
    std::uint64_t out;
    static_assert(sizeof(out) == sizeof(value));
    std::memcpy(&out, &value, sizeof(out));
    return out;
}

// Independent schema fixture, not built with forExport or native construction.
Tags known(std::int32_t flags = 0, std::string app = "LibreCad") {
    return {str(1001, std::move(app)), str(1000, "LC_OFFSET_CURVE"), str(1002, "{"),
        integer(1070, 1), str(1000, "ELLIPSE"), point(1011, 10, 20), point(1012, 4, 0),
        real(1040, .5), real(1040, 0), real(1040, 0), integer(1070, flags),
        real(1041, 1), real(1040, 1e-4), str(1002, "}")};
}
xd::Definition definition() {
    xd::Definition d;
    d.geometry.basis.center = {10, 20};
    d.geometry.basis.majorP = {4, 0};
    d.geometry.basis.ratio = .5;
    d.geometry.distance = 1;
    d.geometry.relTolerance = 1e-4;
    return d;
}
xd::Options eed() {
    xd::Options options;
    options.eed = true;
    options.textEncoding = xd::TextEncoding::Utf16;
    return options;
}
bool has(const xd::Diagnostics &diagnostics, xd::Reason reason) {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [=](const auto &d) { return d.reason == reason; });
}
void sameTags(const Tags &actual, const Tags &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i);
        if (!expected[i]) { CHECK_FALSE(actual[i]); continue; }
        REQUIRE(actual[i]);
        CHECK(actual[i]->code() == expected[i]->code());
        REQUIRE(actual[i]->type() == expected[i]->type());
        switch (actual[i]->type()) {
        case DRW_Variant::STRING:
            REQUIRE(actual[i]->content.s);
            CHECK(*actual[i]->content.s == *expected[i]->content.s);
            break;
        case DRW_Variant::INTEGER: CHECK(actual[i]->i_val() == expected[i]->i_val()); break;
        case DRW_Variant::INTEGER64: CHECK(actual[i]->i64_val() == expected[i]->i64_val()); break;
        case DRW_Variant::DOUBLE: CHECK(bits(actual[i]->d_val()) == bits(expected[i]->d_val())); break;
        case DRW_Variant::COORD:
            REQUIRE(actual[i]->coord());
            CHECK(bits(actual[i]->coord()->x) == bits(expected[i]->coord()->x));
            CHECK(bits(actual[i]->coord()->y) == bits(expected[i]->coord()->y));
            CHECK(bits(actual[i]->coord()->z) == bits(expected[i]->coord()->z));
            break;
        case DRW_Variant::BINARY: CHECK(*actual[i]->binary() == *expected[i]->binary()); break;
        case DRW_Variant::INVALID: break;
        }
    }
}
void putTail(Tags &tags, const Tags &tail) { tags.insert(tags.end() - 1, tail.begin(), tail.end()); }
}

TEST_CASE("offset XDATA parses independent typed schema and owns values", "[offset-xdata]") {
    auto input = known();
    putTail(input, {str(1000, "retained"), integer(1070, -32768), integer(1071, 2147483647)});
    auto result = xd::parseAndStrip(input);
    REQUIRE(result.status == xd::Status::Schema1);
    REQUIRE(result.definition);
    REQUIRE(result.remaining);
    sameTags(*result.remaining, {input[0]});
    CHECK(result.knownCandidates == 1);
    CHECK(result.diagnostics.empty());
    const auto &g = result.definition->geometry;
    CHECK(g.basis.center.x == 10);
    CHECK(g.basis.center.y == 20);
    CHECK(g.basis.majorP.x == 4);
    CHECK(g.basis.majorP.y == 0);
    CHECK(g.basis.ratio == .5);
    CHECK(g.distance == 1);
    CHECK(g.relTolerance == 1e-4);
    REQUIRE(result.definition->tail.size() == 3);
    CHECK(std::get<std::int16_t>(result.definition->tail[1].value) == -32768);
    CHECK(std::get<std::int32_t>(result.definition->tail[2].value) == 2147483647);
    input[13]->addString(1000, "edited input");
    CHECK(std::get<QString>(result.definition->tail[0].value) == QStringLiteral("retained"));
}

TEST_CASE("offset XDATA writer emits independently expected codes types and values", "[offset-xdata]") {
    const auto output = xd::forExport({}, definition());
    REQUIRE(output);
    sameTags(*output, known());
    const std::array<int, 14> codes{1001, 1000, 1002, 1070, 1000, 1011, 1012, 1040, 1040, 1040, 1070, 1041, 1040, 1002};
    const std::array<DRW_Variant::TYPE, 14> types{DRW_Variant::STRING, DRW_Variant::STRING,
        DRW_Variant::STRING, DRW_Variant::INTEGER, DRW_Variant::STRING, DRW_Variant::COORD,
        DRW_Variant::COORD, DRW_Variant::DOUBLE, DRW_Variant::DOUBLE, DRW_Variant::DOUBLE,
        DRW_Variant::INTEGER, DRW_Variant::DOUBLE, DRW_Variant::DOUBLE, DRW_Variant::STRING};
    for (std::size_t i = 0; i < codes.size(); ++i) {
        CAPTURE(i);
        CHECK((*output)[i]->code() == codes[i]);
        CHECK((*output)[i]->type() == types[i]);
    }
}

TEST_CASE("offset XDATA recognizes exact top-level markers and case-only APPIDs", "[offset-xdata]") {
    for (const std::string app : {"LibreCad", "LIBRECAD", "lIbReCaD"}) {
        CAPTURE(app);
        auto input = known(0, app);
        CHECK(xd::parseAndStrip(input).status == xd::Status::Schema1);
        const auto output = xd::forExport(input, definition());
        REQUIRE(output);
        CHECK(*(*output)[0]->content.s == app);
    }
    for (const std::string app : {" LibreCad ", "LibreCad ", "LibreCadX", "FOREIGN"}) {
        CAPTURE(app);
        auto input = known(0, app);
        auto parsed = xd::parseAndStrip(input);
        CHECK(parsed.status == xd::Status::NoSubList);
        REQUIRE(parsed.remaining);
        sameTags(*parsed.remaining, input);
    }
    auto nested = known();
    nested.insert(nested.begin() + 1, str(1002, "{"));
    nested.push_back(str(1002, "}"));
    auto parsed = xd::parseAndStrip(nested);
    CHECK(parsed.status == xd::Status::NoSubList);
    REQUIRE(parsed.remaining);
    sameTags(*parsed.remaining, nested);
    auto unrelated = known();
    unrelated[2] = str(1000, "not an opening brace");
    unrelated.pop_back();
    parsed = xd::parseAndStrip(unrelated);
    CHECK(parsed.status == xd::Status::NoSubList);
    REQUIRE(parsed.remaining);
    sameTags(*parsed.remaining, unrelated);
}

TEST_CASE("offset XDATA unknown schema is opaque including nested apparent schema one", "[offset-xdata]") {
    Tags unknown{str(1001, "LibreCad"), str(1000, "LC_OFFSET_CURVE"), str(1002, "{"), integer(1070, 2)};
    auto inner = known();
    unknown.insert(unknown.end(), inner.begin() + 1, inner.end());
    unknown.push_back(str(1002, "}"));
    auto parsed = xd::parseAndStrip(unknown);
    CHECK(parsed.status == xd::Status::Other);
    CHECK(parsed.knownCandidates == 0);
    REQUIRE(parsed.remaining);
    sameTags(*parsed.remaining, unknown);
    auto coexist = unknown;
    append(coexist, known());
    parsed = xd::parseAndStrip(coexist);
    CHECK(parsed.status == xd::Status::Schema1);
    REQUIRE(parsed.definition);
    REQUIRE(parsed.remaining);
    auto expected = unknown;
    expected.push_back(coexist[unknown.size()]);
    sameTags(*parsed.remaining, expected);
    unknown.pop_back();
    parsed = xd::parseAndStrip(unknown);
    CHECK(parsed.status == xd::Status::Other);
    CHECK(has(parsed.diagnostics, xd::Reason::MalformedUnknown));
    REQUIRE(parsed.remaining);
    sameTags(*parsed.remaining, unknown);
    CHECK_FALSE(xd::forExport(unknown, definition()));
}

TEST_CASE("offset XDATA malformed known fields strip and invalidate without union coercion", "[offset-xdata]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const std::vector<std::pair<std::size_t, Tag>> bad{
        {3, real(1070, 1)}, {3, std::make_shared<DRW_Variant>(1070, std::int64_t{2})},
        {3, integer(1070, 32768)}, {3, str(1070, "1")}, {3, nullptr},
        {4, str(1000, "LINE")}, {4, integer(1000, 1)},
        {5, real(1011, 10)}, {5, point(1011, 10, 20, 1e-300)}, {5, point(1011, nan, 20)},
        {6, point(1011, 14, 20)}, {6, point(1012, 0, 0)}, {6, point(1012, 4, 0, 1e-300)},
        {7, real(1040, 0)}, {7, real(1040, inf)}, {8, real(1040, nan)},
        {9, real(1040, inf)}, {10, integer(1070, -1)}, {10, integer(1070, 32768)},
        {10, integer(1070, 8)}, {10, real(1070, 0)},
        {11, real(1041, 0)}, {11, real(1041, -1)}, {11, real(1041, inf)},
        {12, real(1040, 0)}, {12, real(1040, 1e-6)}, {12, real(1040, .01)}};
    for (const auto &change : bad) {
        CAPTURE(change.first);
        auto input = known(); input[change.first] = change.second;
        auto parsed = xd::parseAndStrip(input);
        CHECK(parsed.status == xd::Status::Invalid);
        CHECK_FALSE(parsed.definition);
        CHECK(parsed.knownCandidates == 1);
        CHECK(has(parsed.diagnostics, xd::Reason::MalformedKnown));
        REQUIRE(parsed.remaining);
        sameTags(*parsed.remaining, {input[0]});
        const auto plain = xd::forExport(input, std::nullopt);
        REQUIRE(plain);
        CHECK(plain->empty());
    }
    auto nullString = known();
    nullString[4] = str(1000, "ELLIPSE");
    nullString[4]->content.s = nullptr;
    CHECK(xd::parseAndStrip(nullString).status == xd::Status::Invalid);
    auto nullCoord = known();
    nullCoord[5]->content.v = nullptr;
    CHECK(xd::parseAndStrip(nullCoord).status == xd::Status::Invalid);
}

TEST_CASE("offset XDATA raw angles normalize only after classification", "[offset-xdata]") {
    auto fullTurn = known();
    fullTurn[9] = real(1040, 2 * std::acos(-1.0));
    auto parsed = xd::parseAndStrip(fullTurn);
    REQUIRE(parsed.definition);
    const auto sweep = LC_EllipseOffsetGeometry::sweep(parsed.definition->geometry.basis);
    REQUIRE(sweep);
    CHECK(sweep->whole);
    auto tinyArc = known();
    tinyArc[8] = real(1040, .25);
    tinyArc[9] = real(1040, .2501);
    parsed = xd::parseAndStrip(tinyArc);
    REQUIRE(parsed.definition);
    const auto small = LC_EllipseOffsetGeometry::sweep(parsed.definition->geometry.basis);
    REQUIRE(small);
    CHECK_FALSE(small->whole);
    CHECK(parsed.definition->geometry.basis.angle1 == .25);
    CHECK(parsed.definition->geometry.basis.angle2 == .2501);
    for (const double tolerance : {.1 * LC_CurveOffset::kEllipseRelativeOffsetTolerance,
        10 * LC_CurveOffset::kEllipseRelativeOffsetTolerance}) {
        auto input = known(); input[12] = real(1040, tolerance);
        CHECK(xd::parseAndStrip(input).status == xd::Status::Schema1);
    }
    auto swapped = known(); swapped[7] = real(1040, 2);
    parsed = xd::parseAndStrip(swapped);
    REQUIRE(parsed.definition);
    CHECK(parsed.definition->geometry.basis.ratio == .5);
    for (const double delta : {0.0, .5 * RS_TOLERANCE_ANGLE, 4 * std::acos(-1.0)}) {
        auto ambiguous = known();
        ambiguous[8] = real(1040, .7);
        ambiguous[9] = real(1040, .7 + delta);
        CHECK(xd::parseAndStrip(ambiguous).status == xd::Status::Invalid);
    }
}

TEST_CASE("offset XDATA every direction combination preserves reserved flags", "[offset-xdata]") {
    for (unsigned bits = 0; bits < 8; ++bits) {
        CAPTURE(bits);
        const unsigned flags = bits | ((bool(bits & 2) != bool(bits & 1)) ? 8u : 0u) | xd::ReservedFlags;
        auto input = known(static_cast<std::int32_t>(flags));
        auto parsed = xd::parseAndStrip(input);
        REQUIRE(parsed.definition);
        CHECK(parsed.definition->reservedFlags == xd::ReservedFlags);
        CHECK(parsed.definition->geometry.basis.reversed == bool(bits & 1));
        CHECK(parsed.definition->geometry.towardCentre == bool(bits & 2));
        CHECK(parsed.definition->geometry.cacheReversed == bool(bits & 4));
        const auto output = xd::forExport(input, parsed.definition);
        REQUIRE(output);
        CHECK((*output)[10]->i_val() == static_cast<std::int32_t>(flags));
        input[10] = integer(1070, static_cast<std::int32_t>(flags ^ 8));
        CHECK(xd::parseAndStrip(input).status == xd::Status::Invalid);
    }
}

TEST_CASE("offset XDATA duplicates including malformed candidates prohibit convenient adoption", "[offset-xdata]") {
    for (bool malformedFirst : {false, true}) for (bool sameGroup : {false, true}) {
        CAPTURE(malformedFirst, sameGroup);
        auto first = known(); auto second = known();
        (malformedFirst ? first : second)[11] = real(1041, -1);
        if (sameGroup) first.insert(first.end(), second.begin() + 1, second.end());
        else append(first, second);
        auto parsed = xd::parseAndStrip(first);
        CHECK(parsed.status == xd::Status::Invalid);
        CHECK(parsed.knownCandidates == 2);
        CHECK_FALSE(parsed.definition);
        CHECK(has(parsed.diagnostics, xd::Reason::DuplicateKnown));
        const auto replacement = xd::forExport(first, definition());
        REQUIRE(replacement);
        sameTags(*replacement, known());
    }
    auto twoValid = known(); append(twoValid, known());
    CHECK(xd::parseAndStrip(twoValid).status == xd::Status::Invalid);
}

TEST_CASE("offset XDATA stripping stops at every APPID regardless of marker type", "[offset-xdata]") {
    auto input = known(); input.pop_back();
    const Tags foreign{str(1001, "FOREIGN"), str(1000, "kept"), integer(1071, 7)};
    append(input, foreign);
    auto parsed = xd::parseAndStrip(input);
    CHECK(parsed.status == xd::Status::Invalid);
    REQUIRE(parsed.remaining);
    Tags expected{input[0]}; append(expected, foreign);
    sameTags(*parsed.remaining, expected);
    const auto output = xd::forExport(input, std::nullopt);
    REQUIRE(output);
    sameTags(*output, foreign);
    input = known(); input.pop_back();
    const auto boundary = integer(1001, 99);
    input.push_back(boundary); input.push_back(str(1000, "kept"));
    parsed = xd::parseAndStrip(input);
    REQUIRE(parsed.remaining);
    sameTags(*parsed.remaining, {input[0], boundary, input.back()});
    CHECK_FALSE(xd::forExport(input, definition()));
}

TEST_CASE("offset XDATA merge preserves foreign payload identity preamble and first spelling", "[offset-xdata]") {
    const Tags preamble{integer(1071, 17)};
    const Tags foreign{str(1001, "XAPP"), str(1003, "Layer"), str(1004, "00FE"), str(1005, "1A")};
    Tags input = preamble;
    auto first = known(0, "lIbReCaD"); first.insert(first.begin() + 1, str(1000, "first"));
    append(input, first); append(input, foreign);
    append(input, {str(1001, "LIBRECAD"), str(1000, "second"),
        str(1000, "LC_OFFSET_CURVE"), str(1002, "{"), integer(1070, 7), str(1000, "future"), str(1002, "}")});
    const auto original = deepTags(input);
    const auto output = xd::forExport(input, definition());
    REQUIRE(output);
    Tags expected = preamble;
    append(expected, {input[1], str(1000, "first"), str(1000, "second"), str(1000, "LC_OFFSET_CURVE"),
        str(1002, "{"), integer(1070, 7), str(1000, "future"), str(1002, "}")});
    const auto current = known(); expected.insert(expected.end(), current.begin() + 1, current.end());
    append(expected, foreign);
    sameTags(*output, expected);
    sameTags(input, original);
    CHECK((*output)[0] == preamble[0]);
    CHECK((*output)[output->size() - 3] == foreign[1]);
    CHECK(std::count_if(output->begin(), output->end(), [](const Tag &v) {
        return v && v->code() == 1001;
    }) == 2);
}

TEST_CASE("offset XDATA safe tails retain all value kinds and drop only unsupported references", "[offset-xdata]") {
    auto input = known();
    const Tags tail{str(1000, "extension"), str(1002, "{"), point(1010, 1, 2, 3),
        point(1011, 4, 5, 6), point(1012, 7, 8, 9), point(1013, 10, 11, 12),
        real(1040, .1), real(1041, .2), real(1042, .3), integer(1070, 32767),
        integer(1071, std::numeric_limits<std::int32_t>::min()), str(1002, "}")};
    putTail(input, tail);
    putTail(input, {str(1003, "layer"), str(1004, "FFFF"), str(1005, "12")});
    auto parsed = xd::parseAndStrip(input);
    REQUIRE(parsed.definition);
    CHECK(parsed.status == xd::Status::Schema1);
    CHECK(has(parsed.diagnostics, xd::Reason::TailUnsupported));
    CHECK(parsed.definition->tail.size() == tail.size());
    auto output = xd::forExport({}, parsed.definition);
    REQUIRE(output);
    auto expected = known(); putTail(expected, tail);
    sameTags(*output, expected);
    CHECK(input[input.size() - 2]->code() == 1005);
}

TEST_CASE("offset XDATA tail item byte and nesting caps never return a prefix", "[offset-xdata]") {
    auto withTail = [](const Tags &tail) { auto input = known(); putTail(input, tail); return input; };
    Tags tail(64, integer(1070, 1));
    auto parsed = xd::parseAndStrip(withTail(tail));
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.size() == 64);
    tail.push_back(integer(1070, 2));
    parsed = xd::parseAndStrip(withTail(tail));
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.empty());
    CHECK(has(parsed.diagnostics, xd::Reason::TailLimit));
    tail.assign(15, str(1000, std::string(255, 'x')));
    tail.push_back(str(1000, std::string(223, 'x'))); // 15*(255+3)+(223+3) = 4096.
    parsed = xd::parseAndStrip(withTail(tail));
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.size() == 16);
    tail.back() = str(1000, std::string(224, 'x'));
    parsed = xd::parseAndStrip(withTail(tail));
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.empty());
    CHECK(has(parsed.diagnostics, xd::Reason::TailLimit));
    for (unsigned nesting : {8u, 9u}) {
        tail.assign(nesting, str(1002, "{"));
        tail.push_back(str(1000, "nested"));
        tail.insert(tail.end(), nesting, str(1002, "}"));
        parsed = xd::parseAndStrip(withTail(tail));
        REQUIRE(parsed.definition);
        CHECK(parsed.definition->tail.size() == (nesting == 8 ? 17u : 0u));
    }
    for (const auto &bad : Tags{real(1040, std::numeric_limits<double>::infinity()),
        integer(1070, 32768), real(1071, 2),
        std::make_shared<DRW_Variant>(1071, std::int64_t{2}), str(1006, "unsupported"), nullptr}) {
        parsed = xd::parseAndStrip(withTail({str(1000, "not a retained prefix"), bad}));
        REQUIRE(parsed.definition);
        CHECK(parsed.definition->tail.empty());
        CHECK(has(parsed.diagnostics, xd::Reason::TailInvalid));
    }
}

TEST_CASE("offset XDATA export refuses invalid current definitions and unsafe owned tails", "[offset-xdata]") {
    auto d = definition(); d.geometry.distance = -1;
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport(known(), d, {}, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::InvalidDefinition));
    d = definition(); d.reservedFlags = 1;
    CHECK_FALSE(xd::forExport({}, d));
    d = definition(); d.geometry.relTolerance = .01;
    CHECK_FALSE(xd::forExport({}, d));
    d = definition(); d.geometry.basis.center.z = 1e-300;
    CHECK_FALSE(xd::forExport({}, d));
    const std::vector<xd::OwnedTail> tails{
        {{1003, QStringLiteral("layer")}}, {{1000, double{1}}}, {{1070, std::int32_t{7}}},
        {{1002, QStringLiteral("{")}}, {{1002, QStringLiteral("}")}},
        {{1000, QString(256, QChar('x'))}}};
    for (const auto &tail : tails) {
        d = definition(); d.tail = tail;
        CHECK_FALSE(xd::forExport(known(), d));
    }
    d = definition(); d.tail.assign(65, {1070, std::int16_t{1}});
    CHECK_FALSE(xd::forExport({}, d));
    d = definition();
    d.tail.assign(15, {1000, QString(255, QChar('x'))});
    d.tail.push_back({1000, QString(223, QChar('x'))});
    REQUIRE(xd::forExport({}, d));
    d.tail.back() = {1000, QString(224, QChar('x'))};
    CHECK_FALSE(xd::forExport({}, d));
}

TEST_CASE("offset XDATA EED complete payload boundaries include closing braces", "[offset-xdata]") {
    auto options = eed();
    Tags input{str(1001, "LibreCad"), str(1002, "{"), str(1000, std::string(32764, 'x')), str(1002, "}")};
    // 2 + (3 + 2*32764) + 2 = 65535, including both braces.
    REQUIRE(xd::forExport(input, std::nullopt, options));
    xd::Diagnostics diagnostics;
    input[2] = str(1000, std::string(32765, 'x'));
    CHECK_FALSE(xd::forExport(input, std::nullopt, options, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::EedChunkLimit));
    input = {str(1001, "LibreCad"), str(1000, std::string(32766, 'x'))};
    REQUIRE(xd::forExport(input, std::nullopt, options));
    CHECK_FALSE(xd::forExport(input, definition(), options));
    input.push_back(str(1001, "OTHER")); input.push_back(str(1000, "independent chunk"));
    REQUIRE(xd::forExport(input, std::nullopt, options));
    input[2] = str(1001, "LIBRECAD"); // Merging groups cannot conceal uint16 overflow.
    CHECK_FALSE(xd::forExport(input, std::nullopt, options));
    CHECK_FALSE(xd::forExport({str(1001, "FOREIGN")}, std::nullopt, options));
    CHECK_FALSE(xd::forExport({str(1000, "preamble")}, definition(), options));
}

TEST_CASE("offset XDATA legacy EED accounts length byte and codepage per string", "[offset-xdata]") {
    auto options = eed();
    options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [](std::string_view s) { return std::optional<std::size_t>(s.size()); };
    Tags input{str(1001, "LibreCad")};
    input.insert(input.end(), 253, str(1000, std::string(255, 'x')));
    input.push_back(str(1000, "four")); // 253*(1+1+2+255) + (1+1+2+4) = 65535.
    REQUIRE(xd::forExport(input, std::nullopt, options));
    input.push_back(str(1000, ""));
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport(input, std::nullopt, options, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::EedChunkLimit));
}

TEST_CASE("offset XDATA no edit export is idempotent and does not alter shared source tags", "[offset-xdata]") {
    auto input = known(0x4010);
    const Tags extra{str(1000, "future"), str(1002, "{"), integer(1071, 42), str(1002, "}")};
    putTail(input, extra);
    const auto parsed = xd::parseAndStrip(input);
    REQUIRE(parsed.definition);
    const auto first = xd::forExport(input, parsed.definition);
    REQUIRE(first);
    sameTags(*first, input);
    const auto second = xd::forExport(*first, parsed.definition);
    REQUIRE(second);
    sameTags(*second, input);
    REQUIRE(xd::forExport(input, definition()));
    auto expected = known(0x4010); putTail(expected, extra);
    sameTags(input, expected);
}

TEST_CASE("offset XDATA encoding uses actual target units and requires a legacy codec", "[offset-xdata]") {
    auto options = eed();
    auto input = known();
    const std::string smile = "\xF0\x9F\x99\x82";
    putTail(input, {str(1000, smile)});
    auto parsed = xd::parseAndStrip(input, options);
    REQUIRE(parsed.definition);
    REQUIRE(parsed.definition->tail.size() == 1);
    const auto output = xd::forExport({}, parsed.definition, options);
    REQUIRE(output);
    sameTags(*output, input);
    for (const std::string unsafe : {std::string("\xC0\xAF"), std::string("a\0b", 3), std::string("a\nb")}) {
        auto bad = known(); putTail(bad, {str(1000, unsafe)});
        parsed = xd::parseAndStrip(bad);
        REQUIRE(parsed.definition);
        CHECK(parsed.definition->tail.empty());
        CHECK_FALSE(xd::forExport({str(1001, "X"), str(1000, unsafe)}, definition()));
    }
    options.textEncoding = xd::TextEncoding::Legacy;
    CHECK_FALSE(xd::forExport({}, definition(), options));
    CHECK(xd::parseAndStrip(known(), options).status == xd::Status::Invalid);
    options.legacyEncodedBytes = [](std::string_view s) -> std::optional<std::size_t> {
        return s == "unencodable" ? std::nullopt : std::optional<std::size_t>(s.size());
    };
    REQUIRE(xd::forExport({}, definition(), options));
    auto current = definition(); current.tail = {{1000, QStringLiteral("unencodable")}};
    CHECK_FALSE(xd::forExport({}, current, options));
    current.tail = {{1000, QString(255, QChar('x'))}};
    REQUIRE(xd::forExport({}, current, options));
    current.tail = {{1000, QString(256, QChar('x'))}};
    CHECK_FALSE(xd::forExport({}, current, options));
    options.textEncoding = xd::TextEncoding::Utf8;
    CHECK_FALSE(xd::forExport({}, definition(), options));
    options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [](std::string_view) -> std::optional<std::size_t> {
        throw std::runtime_error("codec refused");
    };
    CHECK_FALSE(xd::forExport({}, definition(), options));
}

TEST_CASE("offset XDATA UTF16 tail byte cap counts units not UTF8 bytes", "[offset-xdata]") {
    const auto options = eed();
    // U+4E2D is three UTF-8 bytes but one two-byte UTF-16 unit.
    const std::string cjk = "\xE4\xB8\xAD";
    std::string payload;
    for (unsigned i = 0; i < 2046; ++i) payload += cjk;
    auto input = known(); putTail(input, {str(1000, payload)});
    auto parsed = xd::parseAndStrip(input, options);
    REQUIRE(parsed.definition);
    REQUIRE(parsed.definition->tail.size() == 1);
    CHECK(std::get<QString>(parsed.definition->tail[0].value).size() == 2046);
    const auto output = xd::forExport({}, parsed.definition, options);
    REQUIRE(output);
    sameTags(*output, input); // 3 + 2*2046 = 4095.
    putTail(input, {integer(1070, 1)}); // 4098: discard the WHOLE tail.
    parsed = xd::parseAndStrip(input, options);
    REQUIRE(parsed.definition);
    CHECK(parsed.definition->tail.empty());
    CHECK(has(parsed.diagnostics, xd::Reason::TailLimit));
}

TEST_CASE("offset XDATA malformed owned braces cannot hide another group", "[offset-xdata]") {
    auto input = known();
    putTail(input, {str(1002, "{"), str(1000, "unclosed tail")});
    append(input, known());
    auto parsed = xd::parseAndStrip(input);
    CHECK(parsed.status == xd::Status::Invalid);
    CHECK(parsed.knownCandidates == 2);
    CHECK_FALSE(parsed.definition);
    REQUIRE(parsed.remaining);
    CHECK(parsed.remaining->size() == 2);
    input = known(); putTail(input, {integer(1002, 7)});
    parsed = xd::parseAndStrip(input);
    CHECK(parsed.status == xd::Status::Invalid);
    CHECK(has(parsed.diagnostics, xd::Reason::MalformedKnown));
    REQUIRE(parsed.remaining);
    CHECK(parsed.remaining->size() == 1);
    input = known(); input.push_back(str(1002, "}"));
    parsed = xd::parseAndStrip(input);
    CHECK(parsed.status == xd::Status::Invalid);
    CHECK(has(parsed.diagnostics, xd::Reason::MalformedGroup));
    CHECK_FALSE(xd::forExport(input, definition()));
}

TEST_CASE("offset XDATA resource admission and diagnostics are bounded before copies", "[offset-xdata]") {
    auto options = xd::Options{};
    options.maximumItems = 13;
    auto parsed = xd::parseAndStrip(known(), options);
    CHECK(parsed.status == xd::Status::Invalid);
    CHECK_FALSE(parsed.remaining);
    CHECK(has(parsed.diagnostics, xd::Reason::ResourceLimit));
    CHECK_FALSE(xd::forExport({}, definition(), options));
    options.maximumItems = 14;
    REQUIRE(xd::forExport({}, definition(), options));
    REQUIRE(xd::forExport(known(), definition(), options));
    options.maximumBytes = 1;
    parsed = xd::parseAndStrip(known(), options);
    CHECK_FALSE(parsed.remaining);
    CHECK_FALSE(xd::forExport({}, definition(), options));
    options = {};
    options.maximumDiagnostics = 2;
    Tags many;
    for (unsigned i = 0; i < 20; ++i) {
        auto invalid = known(); invalid[11] = real(1041, -1); append(many, invalid);
    }
    parsed = xd::parseAndStrip(many, options);
    CHECK(parsed.status == xd::Status::Invalid);
    CHECK(parsed.knownCandidates == 20);
    CHECK(parsed.diagnostics.size() == 2);
    options.maximumDiagnostics = 0;
    CHECK(xd::parseAndStrip(many, options).diagnostics.empty());
    options.maximumItems = std::numeric_limits<std::size_t>::max();
    options.maximumBytes = std::numeric_limits<std::size_t>::max();
    CHECK(xd::parseAndStrip(known(), options).status == xd::Status::Schema1);
}

TEST_CASE("offset XDATA parse preserves even malformed foreign variants without mutation", "[offset-xdata]") {
    auto nullString = str(1000, "x"); nullString->content.s = nullptr;
    Tags foreign{str(1001, "FOREIGN"), nullptr, nullString,
        real(1070, 2), std::make_shared<DRW_Variant>()};
    auto parsed = xd::parseAndStrip(foreign);
    CHECK(parsed.status == xd::Status::NoSubList);
    REQUIRE(parsed.remaining);
    REQUIRE(parsed.remaining->size() == foreign.size());
    for (std::size_t i = 0; i < foreign.size(); ++i) CHECK((*parsed.remaining)[i] == foreign[i]);
    CHECK_FALSE(xd::forExport(foreign, definition()));
    CHECK(foreign[2]->content.s == nullptr);
    auto input = known(); input[0] = integer(1001, 7);
    parsed = xd::parseAndStrip(input);
    CHECK(parsed.status == xd::Status::NoSubList);
    REQUIRE(parsed.remaining);
    for (std::size_t i = 0; i < input.size(); ++i) CHECK((*parsed.remaining)[i] == input[i]);
}

TEST_CASE("offset XDATA ASCII control sizing follows the writer before text encoding", "[offset-xdata]") {
    auto current = definition();
    current.tail = {{1000, QString(127, QChar('\t'))}};
    REQUIRE(xd::forExport({}, current)); // 127 tabs become 254 ASCII bytes.
    current.tail = {{1000, QString(128, QChar('\t'))}};
    CHECK_FALSE(xd::forExport({}, current)); // 256 encoded bytes, not 128.
    auto input = known(); putTail(input, {str(1000, std::string(128, '\t'))});
    auto parsed = xd::parseAndStrip(input);
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.empty());

    auto binary = xd::Options{}; binary.asciiDxf = false;
    REQUIRE(xd::forExport({}, current, binary));
    REQUIRE(xd::forExport({}, current, eed()));
    current.tail.assign(15, {1000, QString(127, QChar('\t')) + QStringLiteral("x")});
    current.tail.push_back({1000, QString(223, QChar('x'))});
    REQUIRE(xd::forExport({}, current)); // 15*(254+1+3) + (223+3) = 4096.
    current.tail.back() = {1000, QString(224, QChar('x'))};
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport({}, current, {}, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::TailLimit));

    std::vector<std::string> calls;
    auto legacy = xd::Options{}; legacy.textEncoding = xd::TextEncoding::Legacy;
    legacy.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> {
        calls.emplace_back(s); return s.size();
    };
    const std::string controls("\x01\t\x1f", 3);
    current.tail = {{1000, QString::fromLatin1(controls.data(), static_cast<int>(controls.size()))}};
    const auto output = xd::forExport({}, current, legacy);
    REQUIRE(output);
    CHECK(std::find(calls.begin(), calls.end(), "^A^I^_") != calls.end());
    CHECK(std::find(calls.begin(), calls.end(), controls) == calls.end());
    CHECK(*(*output)[13]->content.s == controls); // Sizing does not rewrite the tag.
    calls.clear(); legacy.eed = true;
    REQUIRE(xd::forExport({}, current, legacy));
    CHECK(std::find(calls.begin(), calls.end(), controls) != calls.end());
}

TEST_CASE("offset XDATA foreign binary chunk expansion admits actual target items and bytes", "[offset-xdata]") {
    for (const std::size_t count : {127u, 128u, 254u, 255u, 256u, 2048u}) {
        const std::size_t chunks = count/127 + (count % 127 != 0);
        for (const bool hexText : {false, true}) {
            CAPTURE(count, hexText, chunks);
            const Tag payload = hexText ? str(1004, std::string(2*count, '0')) :
                std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(count, 0));
            Tags input{str(1001, "X"), payload};
            const auto snapshot = deepTags(input);
            auto options = xd::Options{};
            options.maximumItems = chunks + 1;
            const auto output = xd::forExport(input, std::nullopt, options);
            REQUIRE(output); sameTags(*output, snapshot); sameTags(input, snapshot);
            REQUIRE(output->size() == 2); CHECK((*output)[1] == payload);
            options.maximumItems = chunks;
            CHECK_FALSE(xd::forExport(input, std::nullopt, options));
        }
    }
    Tags input{str(1001, "X"), std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(2048, 0))};
    for (const unsigned mode : {0u, 1u, 2u}) {
        auto options = xd::Options{}; options.asciiDxf = mode == 0; options.binaryDxfR12 = mode == 2;
        // 17 emitted binary chunks; ASCII uses hex plus common typed framing.
        // Binary DXF uses a two-byte group code and one-byte chunk length.
        // R12 additionally emits 0xff before each extended group code.
        const std::size_t bytes = 4 + (mode == 0 ? 4096 + 2*17 : 2048 + (mode == 2 ? 4 : 3)*17);
        options.maximumBytes = bytes;
        REQUIRE(xd::forExport(input, std::nullopt, options));
        options.maximumBytes = bytes - 1;
        xd::Diagnostics diagnostics;
        CHECK_FALSE(xd::forExport(input, std::nullopt, options, &diagnostics));
        CHECK(has(diagnostics, xd::Reason::ResourceLimit));
    }
    auto options = eed();
    input[1] = std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(255, 0));
    REQUIRE(xd::forExport(input, std::nullopt, options));
    input[1] = std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(256, 0));
    CHECK_FALSE(xd::forExport(input, std::nullopt, options));
    input[1] = std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>{1});
    input[1]->content.b = nullptr;
    CHECK_FALSE(xd::forExport(input, std::nullopt));
}

TEST_CASE("offset XDATA export refuses counts and source bytes before codec callbacks", "[offset-xdata]") {
    std::size_t calls = 0;
    auto options = xd::Options{}; options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> { ++calls; return s.size(); };
    auto current = definition(); current.tail = {{1000, QStringLiteral("codec")}};
    for (const std::size_t cap : {0u, 13u, 14u}) {
        options.maximumItems = cap;
        CHECK_FALSE(xd::forExport({}, current, options)); CHECK(calls == 0);
    }
    options.maximumItems = 15;
    options.maximumBytes = 0;
    CHECK_FALSE(xd::forExport({}, current, options)); CHECK(calls == 0);
    // Fifteen shared-pointer slots and 37 bytes of source string payload.
    const auto storage = 15*sizeof(Tag) + 37;
    options.maximumBytes = storage - 1;
    CHECK_FALSE(xd::forExport({}, current, options)); CHECK(calls == 0);
    options.maximumBytes = storage;
    REQUIRE(xd::forExport({}, current, options)); CHECK(calls > 0);
    calls = 0;
    auto stale = known(); putTail(stale, {str(1000, "codec")});
    const auto stripped = xd::forExport(stale, std::nullopt, options);
    REQUIRE(stripped); CHECK(stripped->empty()); CHECK(calls == 0);

    options.maximumBytes = 1024; options.maximumItems = 2;
    const Tags foreign{str(1001, "X"), std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(128, 0))};
    CHECK_FALSE(xd::forExport(foreign, std::nullopt, options)); CHECK(calls == 0);
}

TEST_CASE("offset XDATA raised resource limits cannot bypass the hard ceilings", "[offset-xdata]") {
    std::size_t calls = 0;
    auto options = xd::Options{};
    options.maximumItems = options.maximumBytes = options.maximumDiagnostics = std::numeric_limits<std::size_t>::max();
    options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> { ++calls; return s.size(); };
    {
        const Tags tooMany(1000001, integer(1071, 1));
        const auto parsed = xd::parseAndStrip(tooMany, options);
        CHECK(parsed.status == xd::Status::Invalid); CHECK_FALSE(parsed.remaining);
        CHECK(has(parsed.diagnostics, xd::Reason::ResourceLimit));
        CHECK_FALSE(xd::forExport(tooMany, std::nullopt, options)); CHECK(calls == 0);
    }
    {
        // One shared 1-KiB string, but every occurrence is charged. Fixture
        // allocation stays small while the admitted source envelope exceeds 64 MiB.
        const Tags tooLarge(65536, str(1000, std::string(1024, 'x')));
        const auto parsed = xd::parseAndStrip(tooLarge, options);
        CHECK_FALSE(parsed.remaining); CHECK(has(parsed.diagnostics, xd::Reason::ResourceLimit));
        CHECK_FALSE(xd::forExport(tooLarge, std::nullopt, options)); CHECK(calls == 0);
    }
    Tags invalid;
    for (unsigned i = 0; i < 40; ++i) {
        auto item = known(); item[11] = real(1041, -1); append(invalid, item);
    }
    const auto parsed = xd::parseAndStrip(invalid, options);
    CHECK(parsed.status == xd::Status::Invalid); CHECK(parsed.diagnostics.size() == 32);
}

TEST_CASE("offset XDATA UTF16 safe tails admit exactly 4096 encoded bytes and refuse 4097", "[offset-xdata]") {
    for (const bool surrogate : {false, true}) {
        QString payload(surrogate ? 2043 : 2045, QChar(char16_t{0x4e2d}));
        if (surrogate) payload += QString::fromUtf8("\xF0\x9F\x99\x82");
        REQUIRE(payload.size() == 2045);
        const auto utf8 = payload.toUtf8();
        auto input = known();
        putTail(input, {str(1000, std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()))), integer(1070, 7)});
        const auto parsed = xd::parseAndStrip(input, eed());
        REQUIRE(parsed.definition); REQUIRE(parsed.definition->tail.size() == 2);
        REQUIRE(xd::forExport({}, parsed.definition, eed())); // 3+2*2045 + 3 = 4096.
        auto above = *parsed.definition;
        above.tail.pop_back();
        above.tail.push_back({1002, QStringLiteral("{")});
        above.tail.push_back({1002, QStringLiteral("}")}); // Replace 3-byte int16 with 4 balanced bytes.
        xd::Diagnostics diagnostics;
        CHECK_FALSE(xd::forExport({}, above, eed(), &diagnostics));
        CHECK(has(diagnostics, xd::Reason::TailLimit));
        input.erase(input.end() - 2);
        putTail(input, {str(1002, "{"), str(1002, "}")});
        const auto refused = xd::parseAndStrip(input, eed());
        REQUIRE(refused.definition); CHECK(refused.definition->tail.empty());
        CHECK(has(refused.diagnostics, xd::Reason::TailLimit));
    }
}

TEST_CASE("offset XDATA Legacy safe tails use actual codec expansion at the exact byte boundary", "[offset-xdata]") {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1018, false); codec.setCodePage("ANSI_1252", false);
    const std::string cjk = "\xE4\xB8\xAD";
    REQUIRE(codec.fromUtf8(cjk) == "\\U+4E2D");
    auto options = eed(); options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> { return codec.fromUtf8(s).size(); };
    auto current = definition();
    const QString full = QString(36, QChar(char16_t{0x4e2d})) + QStringLiteral("abc");
    const QString last = QString(29, QChar(char16_t{0x4e2d})) + QStringLiteral("abcd");
    current.tail.assign(15, {1000, full}); current.tail.push_back({1000, last});
    REQUIRE(xd::forExport({}, current, options)); // 15*(4+36*7+3) + (4+29*7+4) = 4096.
    auto input = known();
    Tags tail;
    for (unsigned i = 0; i < 15; ++i) tail.push_back(str(1000, full.toUtf8().toStdString()));
    tail.push_back(str(1000, last.toUtf8().toStdString()));
    putTail(input, tail);
    auto parsed = xd::parseAndStrip(input, options);
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.size() == 16);
    current.tail.back() = {1000, last + QStringLiteral("e")};
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport({}, current, options, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::TailLimit));
    input[input.size() - 2] = str(1000, (last + QStringLiteral("e")).toUtf8().toStdString());
    parsed = xd::parseAndStrip(input, options);
    REQUIRE(parsed.definition); CHECK(parsed.definition->tail.empty());
    CHECK(has(parsed.diagnostics, xd::Reason::TailLimit));
}

TEST_CASE("offset XDATA DXF layer names charge target encoding without caret escaping", "[offset-xdata]") {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1018, true); codec.setCodePage("ANSI_1252", true);
    const std::string cjk = "\xE4\xB8\xAD";
    std::string name, encoded;
    for (unsigned i = 0; i < 36; ++i) {
        name += cjk;
        encoded += "\\U+4E2D";
    }
    REQUIRE(codec.fromUtf8(name) == encoded);
    REQUIRE(encoded.size() == 252);
    const Tag layer = std::make_shared<DRW_Variant>(1003, name, true);
    const Tags input{str(1001, "X"), layer};
    const auto snapshot = deepTags(input);
    for (const bool ascii : {false, true}) {
        CAPTURE(ascii);
        auto options = xd::Options{};
        options.asciiDxf = ascii;
        options.textEncoding = xd::TextEncoding::Legacy;
        options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> {
            return codec.fromUtf8(s).size();
        };
        options.maximumBytes = 4 + 3 + encoded.size(); // APPID X, then the encoded layer name.
        const auto output = xd::forExport(input, std::nullopt, options);
        REQUIRE(output); sameTags(*output, snapshot);
        CHECK((*output)[1] == layer); CHECK(layer->isLayerRef());
        --options.maximumBytes;
        xd::Diagnostics diagnostics;
        CHECK_FALSE(xd::forExport(input, std::nullopt, options, &diagnostics));
        CHECK(has(diagnostics, xd::Reason::ResourceLimit));
        options.maximumBytes = 240; // Source envelope fits, encoded layer payload alone does not.
        CHECK_FALSE(xd::forExport(input, std::nullopt, options, &diagnostics));
        CHECK(has(diagnostics, xd::Reason::ResourceLimit));
        sameTags(input, snapshot);
    }

    std::vector<std::string> calls;
    auto options = xd::Options{};
    options.textEncoding = xd::TextEncoding::Legacy;
    options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> {
        calls.emplace_back(s); return codec.fromUtf8(s).size();
    };
    const Tags controlName{str(1001, "X"), str(1003, "layer\tname")};
    REQUIRE(xd::forExport(controlName, std::nullopt, options));
    CHECK((calls == std::vector<std::string>{"X", "layer\tname"}));
    CHECK(*controlName[1]->content.s == "layer\tname");

    options = {};
    std::string utf8Name;
    for (unsigned i = 0; i < 85; ++i) utf8Name += cjk;
    REQUIRE(utf8Name.size() == 255);
    REQUIRE(xd::forExport({str(1001, "X"), str(1003, utf8Name)}, std::nullopt, options));
    utf8Name += cjk;
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport({str(1001, "X"), str(1003, utf8Name)}, std::nullopt, options, &diagnostics));
    CHECK(has(diagnostics, xd::Reason::Encoding));
}

TEST_CASE("offset XDATA EED layer references do not encode their names and retain provenance", "[offset-xdata]") {
    const std::string cjk = "\xE4\xB8\xAD";
    std::string name;
    for (unsigned i = 0; i < 128; ++i) name += cjk;
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1018, false); codec.setCodePage("ANSI_1252", false);
    REQUIRE(codec.fromUtf8(name).size() > 255);
    const Tag layer = std::make_shared<DRW_Variant>(1003, name, true);
    layer->setDwgRawLayerReference(0x123456789abcdef0ULL, DRW::AC1018);
    const Tags input{str(1001, "X"), layer};
    const auto snapshot = deepTags(input);
    for (const auto encoding : {xd::TextEncoding::Legacy, xd::TextEncoding::Utf16}) {
        CAPTURE(static_cast<int>(encoding));
        auto options = eed(); options.textEncoding = encoding;
        std::vector<std::string> calls;
        options.legacyEncodedBytes = [&](std::string_view s) -> std::optional<std::size_t> {
            calls.emplace_back(s);
            if (s == name) throw std::runtime_error("layer spelling must not reach the EED text encoder");
            return codec.fromUtf8(s).size();
        };
        const auto output = xd::forExport(input, std::nullopt, options);
        REQUIRE(output); sameTags(*output, snapshot); sameTags(input, snapshot);
        CHECK((*output)[1] == layer); CHECK(layer->isLayerRef());
        const auto provenance = (*output)[1]->activeDwgLayerReference();
        REQUIRE(provenance);
        CHECK(provenance->reference == 0x123456789abcdef0ULL);
        CHECK(provenance->sourceVersion == DRW::AC1018);
        if (encoding == xd::TextEncoding::Legacy) CHECK(calls == std::vector<std::string>{"X"});
        else CHECK(calls.empty());
    }
    for (const Tag invalid : {integer(1003, 7), str(1003, "null")}) {
        if (invalid->type() == DRW_Variant::STRING) invalid->content.s = nullptr;
        xd::Diagnostics diagnostics;
        CHECK_FALSE(xd::forExport({input[0], invalid}, std::nullopt, eed(), &diagnostics));
        CHECK(has(diagnostics, xd::Reason::Encoding));
    }
}

TEST_CASE("offset XDATA EED layer handle charges nine bytes at the full payload boundary", "[offset-xdata]") {
    // The spelling is not in the EED payload: 9 + (3 + 2*32760) + 3 = 65535.
    const Tag layer = std::make_shared<DRW_Variant>(1003, std::string(512, 'x'), true);
    const Tags input{str(1001, "X"), layer, str(1000, std::string(32760, 'y')), integer(1070, 1)};
    const auto snapshot = deepTags(input);
    const auto output = xd::forExport(input, std::nullopt, eed());
    REQUIRE(output); sameTags(*output, snapshot); CHECK((*output)[1] == layer);
    auto above = input; above.push_back(integer(1070, 2));
    xd::Diagnostics diagnostics;
    CHECK_FALSE(xd::forExport(above, std::nullopt, eed(), &diagnostics));
    CHECK(has(diagnostics, xd::Reason::EedChunkLimit));
    sameTags(input, snapshot); CHECK(layer->isLayerRef());
}

TEST_CASE("offset XDATA parsing and merging preserve independent snapshots of shared variant values", "[offset-xdata]") {
    auto input = known(0x4010, "lIbReCaD");
    const std::string bom = "\xEF\xBB\xBF" "owned";
    putTail(input, {str(1000, "retained"), point(1010, -0.0, 2, 3), real(1040, -0.0), integer(1070, -32768), str(1000, bom)});
    const Tag layer = std::make_shared<DRW_Variant>(1003, std::string("Layer"), true);
    const Tag binary = std::make_shared<DRW_Variant>(1004, std::vector<std::uint8_t>(128, 0x5a));
    append(input, {str(1001, "FOREIGN"), layer, binary, str(1005, "1A"),
        std::make_shared<DRW_Variant>(1071, std::int64_t{42}), point(1013, 4, 5, 6)});
    append(input, {str(1001, "LIBRECAD"), str(1000, "LC_OFFSET_CURVE"), str(1002, "{"),
        integer(1070, 2), str(1000, "opaque future"), str(1002, "}")});
    const auto aliases = input, snapshot = deepTags(input);
    auto parsed = xd::parseAndStrip(input);
    REQUIRE(parsed.definition); sameTags(input, snapshot);
    REQUIRE(parsed.definition->tail.size() == 5);
    CHECK(std::get<QString>(parsed.definition->tail[4].value).toUtf8().toStdString() == bom);
    const auto output = xd::forExport(input, parsed.definition);
    REQUIRE(output); sameTags(input, snapshot);
    const auto reopened = xd::parseAndStrip(*output);
    REQUIRE(reopened.definition); REQUIRE(reopened.definition->tail.size() == 5);
    CHECK(std::get<QString>(reopened.definition->tail[4].value).toUtf8().toStdString() == bom);
    CHECK(std::find(output->begin(), output->end(), layer) != output->end());
    CHECK(std::find(output->begin(), output->end(), binary) != output->end());
    CHECK(layer->isLayerRef());
    std::get<QString>(parsed.definition->tail[0].value) = QStringLiteral("changed owned value");
    sameTags(input, snapshot);
    auto invalid = definition(); invalid.geometry.distance = -1;
    CHECK_FALSE(xd::forExport(input, invalid)); sameTags(input, snapshot);
    for (std::size_t i = 0; i < input.size(); ++i) CHECK(input[i] == aliases[i]);
}

TEST_CASE("schema accepted encoding boundary tails survive native storage and serialization exactly",
          "[offset-xdata][lc-offsetcurve][owned-tail]") {
    for (unsigned lane : {0u, 1u, 2u}) {
        CAPTURE(lane);
        const auto options = lane == 0 ? xd::Options{} : eed();
        auto input = known(0x10);
        input[5] = point(1011, 0, 0);
        input[6] = point(1012, 1, 0);
        input[7] = real(1040, 1);
        input[8] = real(1040, .25);
        input[9] = real(1040, .2501);
        if (lane == 0) {
            Tags tail(15, str(1000, std::string(255, 'x')));
            tail.push_back(str(1000, std::string(223, 'x')));
            putTail(input, tail); // 15*(255+3)+(223+3) = 4096.
        } else {
            QString payload(lane == 2 ? 2043 : 2045, QChar(char16_t{0x4e2d}));
            if (lane == 2) payload += QString::fromUtf8("\xF0\x9F\x99\x82");
            REQUIRE(payload.size() == 2045);
            putTail(input, {str(1000, payload.toUtf8().toStdString()), integer(1070, 7)});
        }
        const auto snapshot = deepTags(input);
        const auto parsed = xd::parseAndStrip(input, options);
        REQUIRE(parsed.status == xd::Status::Schema1);
        REQUIRE(parsed.definition);
        REQUIRE(parsed.remaining);
        REQUIRE(parsed.diagnostics.empty());
        const RS_Line cache{nullptr, {2*std::cos(.25), 2*std::sin(.25)},
                                     {2*std::cos(.2501), 2*std::sin(.2501)}};
        LC_OffsetCheckResult checked;
        auto curve = LC_OffsetCurve::adopt(cache, parsed.definition->geometry, 1e-4, &checked);
        CAPTURE(checked.reason);
        REQUIRE(checked);
        REQUIRE(curve);
        REQUIRE(curve->setOwnedExtensions(parsed.definition->reservedFlags, parsed.definition->tail));
        REQUIRE(curve->hasEditableDefinition());
        const xd::Definition stored{curve->definition(), curve->unknownOwnedFlags(), curve->unknownOwnedTail()};
        const auto encoded = xd::forExport(*parsed.remaining, stored, options);
        REQUIRE(encoded);
        sameTags(*encoded, snapshot);
        sameTags(input, snapshot);
    }
}
