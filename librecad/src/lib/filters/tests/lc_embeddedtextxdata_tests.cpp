/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_embeddedtextxdata.h"
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <limits>

namespace {
using Codec = LC_EmbeddedTextXData;
double real(std::uint64_t bits) { double result; std::memcpy(&result, &bits, sizeof(result)); return result; }
std::uint64_t bits(double value) { std::uint64_t result; std::memcpy(&result, &value, sizeof(result)); return result; }
void same(const Codec::Tags &actual, const Codec::Tags &expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        INFO("tag " << index);
        REQUIRE(bool(actual[index]) == bool(expected[index]));
        if (!expected[index]) continue;
        const auto &lhs = *actual[index], &rhs = *expected[index];
        CHECK(lhs.code() == rhs.code()); CHECK(lhs.type() == rhs.type());
        switch (rhs.type()) {
        case DRW_Variant::STRING:
            CHECK(*lhs.content.s == *rhs.content.s); CHECK(lhs.isLayerRef() == rhs.isLayerRef());
            REQUIRE(bool(lhs.activeDwgLayerReference()) == bool(rhs.activeDwgLayerReference()));
            if (const auto reference = rhs.activeDwgLayerReference()) {
                CHECK(lhs.activeDwgLayerReference()->reference == reference->reference);
                CHECK(lhs.activeDwgLayerReference()->sourceVersion == reference->sourceVersion);
            }
            break;
        case DRW_Variant::INTEGER: CHECK(lhs.i_val() == rhs.i_val()); break;
        case DRW_Variant::INTEGER64: CHECK(lhs.i64_val() == rhs.i64_val()); break;
        case DRW_Variant::DOUBLE: CHECK(bits(lhs.d_val()) == bits(rhs.d_val())); break;
        case DRW_Variant::COORD:
            CHECK(bits(lhs.coord()->x) == bits(rhs.coord()->x)); CHECK(bits(lhs.coord()->y) == bits(rhs.coord()->y)); CHECK(bits(lhs.coord()->z) == bits(rhs.coord()->z)); break;
        case DRW_Variant::BINARY: CHECK(*lhs.binary() == *rhs.binary()); break;
        case DRW_Variant::INVALID: break;
        }
    }
}
Codec::Tags input() {
    return {nullptr, std::make_shared<DRW_Variant>(),
        std::make_shared<DRW_Variant>(std::numeric_limits<int>::min(), std::int32_t{-12}),
        std::make_shared<DRW_Variant>(std::numeric_limits<int>::max(), std::numeric_limits<std::int64_t>::min()),
        std::make_shared<DRW_Variant>(1000, std::string("NUL\0invalid\xff\xfe", 14)),
        std::make_shared<DRW_Variant>(1040, real(0x7ff8000000001234ULL)),
        std::make_shared<DRW_Variant>(1010, DRW_Coord{real(0x7ff0000000000000ULL), real(0x8000000000000000ULL), real(0xfff800000000abcdULL)}),
        std::make_shared<DRW_Variant>(1004, Codec::Bytes{0, 255, 128, 0, 1})};
}
}

TEST_CASE("Embedded XDATA capsules retain exact owned raw values without wire activation", "[embedded-xdata][headless]") {
    auto raw = input();
    const auto encoded = Codec::encode(raw); REQUIRE(encoded);
    const auto before = *encoded.bytes;
    const auto decoded = Codec::decode(before); REQUIRE(decoded);
    same(*decoded.tags, raw);
    CHECK(*encoded.bytes == before);
    CHECK(decoded.tags->at(4) != raw[4]);
    *raw[4]->content.s = "mutated input";
    CHECK(*decoded.tags->at(4)->content.s == std::string("NUL\0invalid\xff\xfe", 14));
    CHECK(*encoded.bytes == before);
    const auto second = Codec::encode(*decoded.tags); REQUIRE(second);
    CHECK(*second.bytes == before);
    Codec::Tags many;
    for (int index = 0; index < 4097; ++index)
        many.push_back(index % 2 ? std::make_shared<DRW_Variant>(1000, std::to_string(index)) :
                       std::make_shared<DRW_Variant>(1070 + index, std::int32_t{-index}));
    const auto large = Codec::encode(many); REQUIRE(large);
    const auto read = Codec::decode(*large.bytes); REQUIRE(read);
    same(*read.tags, many);
}

TEST_CASE("Embedded XDATA capsules match an independent literal v2 fixture", "[embedded-xdata][headless]") {
    // Hand-authored little-endian bytes; this is not produced by the encoder.
    const Codec::Bytes literal{
        0x4c,0x54,0x43,0x58, 0x02, 0x07,0x00,0x00,0x00,
        0x00, 0x00,0x00,0x00,0x00, // null slot
        0x01, 0x00,0x00,0x00,0x00, // INVALID slot
        0x03, 0xf9,0xff,0xff,0xff, 0xfe,0xff,0xff,0xff, // code -7, integer -2
        0x04, 0x34,0x12,0x00,0x00, 0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,
        0x02, 0xe8,0x03,0x00,0x00, 0x03,0x00,0x00,0x00, 0x41,0x00,0x42, 0x01,0x00,
        0x07, 0xec,0x03,0x00,0x00, 0x03,0x00,0x00,0x00, 0x00,0xff,0x7f,
        0x02, 0xeb,0x03,0x00,0x00, 0x01,0x00,0x00,0x00, 0x4c, 0x01,0x01,
        0x11,0x00,0x00,0x00, 0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe};
    auto active = std::make_shared<DRW_Variant>(1003, std::string("L"), true);
    active->setDwgRawLayerReference(0xfedcba9876543210ULL, DRW::AC1027);
    const Codec::Tags expected{nullptr, std::make_shared<DRW_Variant>(),
        std::make_shared<DRW_Variant>(-7, std::int32_t{-2}),
        std::make_shared<DRW_Variant>(0x1234, std::int64_t{0x0102030405060708LL}),
        std::make_shared<DRW_Variant>(1000, std::string("A\0B", 3), true),
        std::make_shared<DRW_Variant>(1004, Codec::Bytes{0, 255, 127}), active};
    REQUIRE(static_cast<int>(DRW::AC1027) == 17);
    const auto decoded = Codec::decode(literal); REQUIRE(decoded);
    same(*decoded.tags, expected);
    const auto encoded = Codec::encode(expected); REQUIRE(encoded);
    CHECK(*encoded.bytes == literal);
    auto unknownVersion = literal;
    // The active provenance's source-version field is an unsigned wire value,
    // not an invalid C++ enum object passed through DRW's typed setter.
    for (std::size_t index = literal.size() - 12; index < literal.size() - 8; ++index)
        unknownVersion[index] = 0xff;
    CHECK_FALSE(Codec::decode(unknownVersion));
}

TEST_CASE("Embedded XDATA capsules preserve only active layer-reference replay provenance", "[embedded-xdata][headless]") {
    auto active = std::make_shared<DRW_Variant>(1003, std::string("UNRESOLVED"), true);
    active->setDwgRawLayerReference(0xfedcba9876543210ULL, DRW::AC1027);
    REQUIRE(active->activeDwgLayerReference());
    CHECK(active->activeDwgLayerReference()->sourceVersion == DRW::AC1027);
    auto stale = std::make_shared<DRW_Variant>(*active);
    *stale->content.s = "edited spelling";
    CHECK_FALSE(stale->activeDwgLayerReference());
    auto inactive = std::make_shared<DRW_Variant>(1003, std::string("named-layer"), true);
    const Codec::Tags raw{active, stale, inactive};
    const auto encoded = Codec::encode(raw); REQUIRE(encoded);
    const auto decoded = Codec::decode(*encoded.bytes); REQUIRE(decoded);
    same(*decoded.tags, raw);
    CHECK(decoded.tags->at(0)->canReplayDwgRawLayerReference(DRW::AC1027));
    CHECK_FALSE(decoded.tags->at(0)->canReplayDwgRawLayerReference(DRW::AC1032));
    for (int version = DRW::UNKNOWNV; version <= DRW::AC1032; ++version) {
        CHECK_FALSE(decoded.tags->at(1)->canReplayDwgRawLayerReference(static_cast<DRW::Version>(version)));
        CHECK_FALSE(decoded.tags->at(2)->canReplayDwgRawLayerReference(static_cast<DRW::Version>(version)));
    }
    std::string reboundText = "rebound live spelling";
    auto *originalStorage = active->content.s;
    active->content.s = &reboundText;
    CHECK_FALSE(active->activeDwgLayerReference());
    const auto rebound = Codec::encode({active}); REQUIRE(rebound);
    const auto reboundRead = Codec::decode(*rebound.bytes); REQUIRE(reboundRead);
    CHECK(*reboundRead.tags->front()->content.s == reboundText);
    CHECK_FALSE(reboundRead.tags->front()->activeDwgLayerReference());
    active->content.s = originalStorage;
}

TEST_CASE("Embedded XDATA capsule limits are aggregate checked and errors are atomic", "[embedded-xdata][headless]") {
    const auto raw = input();
    const auto encoded = Codec::encode(raw); REQUIRE(encoded);
    const auto exact = encoded.bytes->size();
    REQUIRE(Codec::encode(raw, {raw.size(), exact}));
    REQUIRE(Codec::decode(*encoded.bytes, {raw.size(), exact}));
    CHECK_FALSE(Codec::encode(raw, {raw.size() - 1, exact}));
    CHECK_FALSE(Codec::encode(raw, {raw.size(), exact - 1}));
    CHECK_FALSE(Codec::decode(*encoded.bytes, {raw.size() - 1, exact}));
    CHECK_FALSE(Codec::decode(*encoded.bytes, {raw.size(), exact - 1}));
    CHECK_FALSE(Codec::encode({}, {1000001, 64 * 1024 * 1024}));
    CHECK_FALSE(Codec::decode({}, {0, 8}));
    const auto empty = Codec::encode({}, {0, 9}); REQUIRE(empty);
    REQUIRE(Codec::decode(*empty.bytes, {0, 9}));
    Codec::Tags strings{std::make_shared<DRW_Variant>(1000, std::string(30, 'a')),
                        std::make_shared<DRW_Variant>(1000, std::string(30, 'b'))};
    CHECK_FALSE(Codec::encode(strings, {2, 70}));
    for (std::size_t length = 0; length < encoded.bytes->size(); ++length) {
        auto truncated = *encoded.bytes; truncated.resize(length);
        const auto before = truncated;
        auto decoded = Codec::decode(truncated);
        CHECK_FALSE(decoded); CHECK_FALSE(decoded.tags); CHECK(truncated == before);
    }
    auto unknown = *encoded.bytes; unknown[4] = 3;
    CHECK(Codec::decode(unknown).error.code == Codec::ErrorCode::UnknownVersion);
    auto trailing = *encoded.bytes; trailing.push_back(0);
    CHECK_FALSE(Codec::decode(trailing));
    for (const auto count : {0u, 1u, 1000000u, 1000001u, 0xffffffffu}) {
        auto mismatch = *encoded.bytes;
        for (unsigned i = 0; i < 4; ++i) mismatch[5 + i] = std::uint8_t(count >> (i * 8));
        CHECK_FALSE(Codec::decode(mismatch));
    }
    auto invalidPointer = std::make_shared<DRW_Variant>(1000, std::string("retained"));
    auto *originalPointer = invalidPointer->content.s;
    invalidPointer->content.s = nullptr;
    CHECK_FALSE(Codec::encode({invalidPointer}));
    CHECK(invalidPointer->content.s == nullptr);
    invalidPointer->content.s = originalPointer;
    CHECK(*invalidPointer->content.s == "retained");
}

TEST_CASE("Embedded XDATA capsule binary64 storage is fixed-endian and bounded under mutations", "[embedded-xdata][headless]") {
    for (const auto pattern : {0ULL, 0x8000000000000000ULL, 0x7ff0000000000000ULL, 0xfff0000000000000ULL, 0x7ff0000000000001ULL, 0x7ff8123456789abcULL}) {
        const auto encoded = Codec::encode({std::make_shared<DRW_Variant>(1040, real(pattern))}); REQUIRE(encoded);
        REQUIRE(encoded.bytes->size() == 22);
        for (unsigned i = 0; i < 8; ++i) CHECK(encoded.bytes->at(14 + i) == std::uint8_t(pattern >> (i * 8)));
        const auto decoded = Codec::decode(*encoded.bytes); REQUIRE(decoded);
        CHECK(bits(decoded.tags->front()->d_val()) == pattern);
    }
    const auto encoded = Codec::encode(input()); REQUIRE(encoded);
    for (std::size_t index = 0; index < encoded.bytes->size(); ++index) for (const auto byte : {0, 255}) {
        auto altered = *encoded.bytes; altered[index] = static_cast<std::uint8_t>(byte);
        const auto decoded = Codec::decode(altered, {1000, 4096});
        if (decoded) {
            const auto reencoded = Codec::encode(*decoded.tags, {1000, 4096}); REQUIRE(reencoded);
            CHECK(*reencoded.bytes == altered);
        } else CHECK_FALSE(decoded.tags);
    }
}
