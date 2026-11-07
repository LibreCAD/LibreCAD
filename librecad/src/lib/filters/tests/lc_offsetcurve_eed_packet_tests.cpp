/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include "drw_entities.h"
#include "intern/drw_textcodec.h"
#include "intern/dxfreader.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgreader.h"
#include "intern/dwgsafety.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
using Tag = std::shared_ptr<DRW_Variant>;
using Tags = std::vector<Tag>;
using Ref = DRW_Entity::PendingHandleRef;
using Bytes = std::vector<std::uint8_t>;
Tag text(int code, std::string value) { return std::make_shared<DRW_Variant>(code, std::move(value)); }
Tag integer(std::int32_t value) { return std::make_shared<DRW_Variant>(1070, value); }
struct Fixture {
    Tags tags;
    std::vector<Ref> apps, layers;
    explicit Fixture(Tags input) : tags(std::move(input)) {
        for (std::size_t i = 0; i < tags.size(); ++i)
            if (tags[i] && tags[i]->code() == 1001) apps.push_back({i, 0x23});
    }
    std::vector<DRW_Variant*> raw() const {
        std::vector<DRW_Variant*> values;
        values.reserve(tags.size());
        for (const auto &tag : tags) values.push_back(tag.get());
        return values;
    }
};
struct Encoded { bool accepted; Bytes bytes; };
Encoded encode(const Fixture &fixture, DRW::Version version = DRW::AC1027,
               const char *page = "ANSI_1252") {
    DRW_TextCodec codec;
    codec.setVersion(version, false); codec.setCodePage(page, false);
    dwgBufferW output(&codec);
    const bool accepted = DRW_Entity::encodeDwgEed(version, fixture.raw(), fixture.apps,
        fixture.layers, dwgCodePageId(page), output);
    return {accepted, output.data()};
}
struct Packet { std::uint32_t app; Bytes payload; };
std::vector<Packet> packets(Bytes bytes) {
    dwgBuffer input(bytes.data(), bytes.size());
    std::vector<Packet> result;
    for (;;) {
        const auto size = input.getBitShort();
        REQUIRE(input.isGood());
        if (!size) break;
        REQUIRE(result.size() < dwgSafety::MaxEedChunks);
        const auto app = input.getHandle();
        REQUIRE(input.isGood()); REQUIRE(app.code == 5);
        Packet packet{app.ref, Bytes(size)};
        REQUIRE(input.getBytes(packet.payload.data(), packet.payload.size()));
        result.push_back(std::move(packet));
    }
    return result;
}
std::vector<DwgEedChunk> decoded(Bytes bytes, DRW::Version version = DRW::AC1027) {
    DRW_TextCodec codec; codec.setVersion(version, false);
    dwgBuffer input(bytes.data(), bytes.size(), &codec);
    std::vector<DwgEedChunk> chunks;
    REQUIRE(readDwgEed(version, input, chunks));
    return chunks;
}
void sameBraces(const Bytes &payload) {
    REQUIRE(payload.size() >= 4);
    CHECK(payload[0] == 2); CHECK(payload[1] == 0);
    CHECK(payload[payload.size() - 2] == 2); CHECK(payload.back() == 1);
}
class EntityProbe final : public DRW_Line {
public:
    using DRW_Entity::dwgEedAppIdWriteRefs;
    using DRW_Entity::dwgEedLayerWriteRefs;
    bool writeCommon(DRW::Version version, dwgBufferW &out) {
        return DRW_Entity::encodeDwgCommon(version, &out);
    }
    bool readCommon(DRW::Version version, dwgBuffer &in) {
        return DRW_Entity::parseDwg(version, &in, nullptr, 0);
    }
    bool readCode(int code, const std::unique_ptr<dxfReader> &in) {
        return DRW_Entity::parseCode(code, in);
    }
};
}

TEST_CASE("EED packets flush before a complete opening braced unit", "[offset-xdata][eed-packets]") {
    for (const std::size_t count : {32765u, 32766u}) {
        CAPTURE(count);
        const Fixture fixture{{text(1001, "APP"), text(1000, std::string(count, 'x')),
            text(1002, "{"), text(1002, "}")}};
        const auto output = encode(fixture);
        REQUIRE(output.accepted);
        const auto actual = packets(output.bytes);
        REQUIRE(actual.size() == 2);
        CHECK(actual[0].payload.size() == 3 + 2*count);
        CHECK(actual[0].app == 0x23); CHECK(actual[1].app == 0x23);
        CHECK((actual[1].payload == Bytes{2, 0, 2, 1}));
        const auto parsed = decoded(output.bytes);
        REQUIRE(parsed.size() == 2);
        CHECK(parsed[0].items.size() == 1); CHECK(parsed[1].items.size() == 2);
    }
}

TEST_CASE("EED indivisible unit capacity includes its closing brace", "[offset-xdata][eed-packets]") {
    Fixture fixture{{text(1001, "APP"), text(1002, "{"),
        text(1000, std::string(32764, 'x')), text(1002, "}")}};
    auto output = encode(fixture);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 1); REQUIRE(actual[0].payload.size() == 65535);
    sameBraces(actual[0].payload);
    CHECK(decoded(output.bytes).front().items.size() == 3);
    fixture.tags[2] = text(1000, std::string(32765, 'x'));
    output = encode(fixture);
    CHECK_FALSE(output.accepted);
    CHECK(output.bytes.empty()); // No half-open first packet was published.
}

TEST_CASE("EED scalar units roll packets without changing their encoded order", "[offset-xdata][eed-packets]") {
    const Fixture fixture{{text(1001, "APP"), text(1000, std::string(32766, 'x')), integer(0x1234)}};
    const auto output = encode(fixture);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 2);
    CHECK(actual[0].payload.size() == 65535);
    CHECK((actual[1].payload == Bytes{70, 0x34, 0x12}));
    const auto parsed = decoded(output.bytes);
    REQUIRE(parsed.size() == 2);
    REQUIRE(parsed[1].items.size() == 1);
    CHECK(parsed[1].items[0].i_val() == 0x1234);
}

TEST_CASE("EED nested groups and appended definition marker retain exact unit order", "[offset-xdata][eed-packets]") {
    const Fixture fixture{{text(1001, "LibreCad"), text(1000, std::string(32748, 'x')), integer(7),
        text(1000, "LC_OFFSET_CURVE"), text(1002, "{"), integer(1),
        text(1002, "{"), integer(9), text(1002, "}"), text(1002, "}")}};
    const auto output = encode(fixture);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 2); REQUIRE(actual[0].payload.size() == 65535);
    const Bytes tail{2, 0, 70, 1, 0, 2, 0, 70, 9, 0, 2, 1, 2, 1};
    CHECK(actual[1].payload == tail);
    const auto parsed = decoded(output.bytes);
    REQUIRE(parsed.size() == 2); REQUIRE(parsed[0].items.size() == 3);
    REQUIRE(parsed[0].items.back().type() == DRW_Variant::STRING);
    CHECK(*parsed[0].items.back().content.s == "LC_OFFSET_CURVE");
    CHECK(parsed[1].items.size() == 6);
}

TEST_CASE("EED malformed braces cannot become separate valid packets", "[offset-xdata][eed-packets]") {
    const std::vector<Tags> bad{
        {text(1001, "APP"), text(1002, "}")},
        {text(1001, "APP"), text(1002, "{")},
        {text(1001, "APP"), text(1002, "{"), integer(1), text(1001, "OTHER"), text(1002, "}")},
        {text(1001, "APP"), text(1002, "not a brace")},
        {text(1001, "APP"), std::make_shared<DRW_Variant>(1002, std::int32_t{1})},
        {text(1001, "APP"), text(1002, "{"), nullptr, text(1002, "}")}};
    for (const auto &tags : bad) CHECK_FALSE(encode(Fixture{tags}).accepted);
    Fixture omitted{bad[1]}; omitted.apps[0].leftOut = true;
    CHECK_FALSE(encode(omitted).accepted); // Explicit omission does not authorize bad grouping.
}

TEST_CASE("EED resolved handles are not APPID or layer name text", "[offset-xdata][eed-packets]") {
    Fixture fixture{{text(1001, std::string(1000, 'x')), text(1003, std::string(1000, 'y'))}};
    fixture.layers.push_back({1, 0, 0x123456789abcdef0ULL, true});
    const auto output = encode(fixture, DRW::AC1018);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 1);
    CHECK(actual[0].app == 0x23);
    CHECK((actual[0].payload == Bytes{3, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0}));
}

TEST_CASE("EED reference tables reject missing conflicting or misindexed resolutions", "[offset-xdata][eed-packets]") {
    const Fixture base{{text(1001, "APP"), text(1003, "Layer")}};
    for (unsigned fault = 0; fault < 10; ++fault) {
        CAPTURE(fault);
        auto fixture = base; fixture.layers = {{1, 0x45}};
        switch (fault) {
        case 0: fixture.apps.clear(); break;
        case 1: fixture.apps[0].handleRef = 0; break;
        case 2: fixture.apps.push_back(fixture.apps[0]); break;
        case 3: fixture.apps[0].indexInExtData = 1; break;
        case 4: fixture.apps[0].indexInExtData = fixture.tags.size(); break;
        case 5: fixture.layers.clear(); break;
        case 6: fixture.layers[0].handleRef = 0; break;
        case 7: fixture.layers.push_back(fixture.layers[0]); break;
        case 8: fixture.layers[0].indexInExtData = 0; break;
        case 9: fixture.layers[0].leftOut = true; break;
        }
        CHECK_FALSE(encode(fixture).accepted);
    }
    Fixture unordered{{text(1001, "A"), integer(1), text(1001, "B"), integer(2)}};
    unordered.apps[1].handleRef = 0x67;
    std::reverse(unordered.apps.begin(), unordered.apps.end());
    const auto output = encode(unordered);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 2);
    CHECK(actual[0].app == 0x23); CHECK(actual[1].app == 0x67);
}

TEST_CASE("EED legacy and Unicode strings have independent literal byte oracles", "[offset-xdata][eed-packets]") {
    const std::string cyrillic = "\xd0\xb0";
    const Fixture fixture{{text(1001, "APP"), text(1000, cyrillic)}};
    REQUIRE(dwgCodePageId("ANSI_1251") == 29);
    const auto legacy = encode(fixture, DRW::AC1018, "ANSI_1251");
    REQUIRE(legacy.accepted);
    CHECK((packets(legacy.bytes).front().payload == Bytes{0, 1, 0, 29, 0xe0}));
    const auto parsed = decoded(legacy.bytes, DRW::AC1018);
    REQUIRE(parsed.size() == 1); REQUIRE(parsed[0].items.size() == 1);
    CHECK(*parsed[0].items[0].content.s == cyrillic);
    const auto unicode = encode(fixture);
    REQUIRE(unicode.accepted);
    CHECK((packets(unicode.bytes).front().payload == Bytes{0, 1, 0, 0x30, 0x04}));
    const auto supplementary = encode(Fixture{{text(1001, "APP"), text(1000, "\xf0\x9f\x99\x82")}});
    REQUIRE(supplementary.accepted);
    CHECK((packets(supplementary.bytes).front().payload == Bytes{0, 2, 0, 0x3d, 0xd8, 0x42, 0xde}));
}

TEST_CASE("EED never applies DXF caret escaping to strings", "[offset-xdata][eed-packets]") {
    const Fixture fixture{{text(1001, "APP"), text(1000, "^\t")}};
    const auto legacy = encode(fixture, DRW::AC1018);
    REQUIRE(legacy.accepted);
    CHECK((packets(legacy.bytes).front().payload == Bytes{0, 2, 0, 30, '^', '\t'}));
    const auto unicode = encode(fixture);
    REQUIRE(unicode.accepted);
    CHECK((packets(unicode.bytes).front().payload == Bytes{0, 2, 0, '^', 0, '\t', 0}));
}

TEST_CASE("EED string validation refuses malformed UTF8 and oversized scalars before publication", "[offset-xdata][eed-packets]") {
    const std::vector<std::string> invalid{"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xf0\x9f", "\x80"};
    for (const auto &value : invalid) {
        for (const auto version : {DRW::AC1018, DRW::AC1027}) {
            const auto output = encode(Fixture{{text(1001, "APP"), text(1000, value)}}, version);
            CHECK_FALSE(output.accepted); CHECK(output.bytes.empty());
        }
    }
    CHECK_FALSE(encode(Fixture{{text(1001, "APP"), text(1000, std::string(32767, 'x'))}}).accepted);
    CHECK_FALSE(encode(Fixture{{text(1001, "APP"), text(1000, std::string(256, 'x'))}}, DRW::AC1018).accepted);
}

TEST_CASE("EED legacy complete braced units use actual byte capacity", "[offset-xdata][eed-packets]") {
    Tags tags{text(1001, "APP"), text(1002, "{")};
    tags.insert(tags.end(), 253, text(1000, std::string(255, 'x')));
    tags.push_back(text(1000, "")); tags.push_back(text(1002, "}"));
    // 2 + 253*259 + 4 + 2 = 65535.
    auto output = encode(Fixture{tags}, DRW::AC1018);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 1); REQUIRE(actual[0].payload.size() == 65535);
    sameBraces(actual[0].payload);
    tags[tags.size() - 2] = text(1000, "x");
    CHECK_FALSE(encode(Fixture{tags}, DRW::AC1018).accepted);
}

TEST_CASE("EED packet and total source caps are enforced at their exact boundaries", "[offset-xdata][eed-packets]") {
    const auto app = text(1001, "APP"), scalar = text(1000, "");
    Tags tags; tags.reserve(2 * (dwgSafety::MaxEedChunks + 1u));
    for (std::size_t i = 0; i < dwgSafety::MaxEedChunks; ++i) { tags.push_back(app); tags.push_back(scalar); }
    const auto output = encode(Fixture{tags});
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == dwgSafety::MaxEedChunks);
    CHECK((actual.front().payload == Bytes{0, 0, 0}));
    CHECK((actual.back().payload == Bytes{0, 0, 0}));
    tags.push_back(app); tags.push_back(scalar);
    CHECK_FALSE(encode(Fixture{tags}).accepted);
    std::vector<DRW_Variant*> tooMany(dwgSafety::MaxEedTotalItems + 1u, scalar.get());
    tooMany[0] = app.get();
    dwgBufferW writer;
    CHECK_FALSE(DRW_Entity::encodeDwgEed(DRW::AC1027, tooMany, {{0, 0x23}}, {}, 30, writer));
    CHECK(writer.data().empty());
}

TEST_CASE("EED explicit APPID omission remains scoped and does not swallow following groups", "[offset-xdata][eed-packets]") {
    Fixture fixture{{text(1001, "OMITTED"), text(1000, "discard"), text(1001, "KEPT"), integer(4)}};
    fixture.apps[0].leftOut = true; fixture.apps[0].handleRef = 0;
    fixture.apps[1].handleRef = 0x67;
    const auto output = encode(fixture);
    REQUIRE(output.accepted);
    const auto actual = packets(output.bytes);
    REQUIRE(actual.size() == 1);
    CHECK(actual[0].app == 0x67); CHECK((actual[0].payload == Bytes{70, 4, 0}));
    CHECK_FALSE(encode(Fixture{{text(1001, "EMPTY")}}).accepted);
    CHECK_FALSE(encode(Fixture{{integer(1)}}).accepted);
    const auto empty = encode(Fixture{Tags{}});
    REQUIRE(empty.accepted); CHECK(packets(empty.bytes).empty());
}

TEST_CASE("EED entity decoder coalesces continuation packets and retains layer resolution indices", "[offset-xdata][eed-packets]") {
    EntityProbe source;
    source.handle = 0x91;
    source.extData = {text(1001, "LibreCad"), text(1000, std::string(32748, 'x')), integer(7),
        text(1000, "LC_OFFSET_CURVE"), text(1002, "{"), text(1003, "Layer"), text(1002, "}"),
        text(1001, "FOREIGN"), integer(9)};
    source.dwgEedAppIdWriteRefs = {{0, 0x23}, {7, 0x67}};
    source.dwgEedLayerWriteRefs = {{5, 0x45}};
    DRW_TextCodec codec; codec.setVersion(DRW::AC1027, false);
    dwgBufferW output(&codec);
    REQUIRE(source.writeCommon(DRW::AC1027, output));
    auto bytes = output.data();
    dwgBuffer input(bytes.data(), bytes.size(), &codec);
    EntityProbe actual;
    REQUIRE(actual.readCommon(DRW::AC1027, input));
    REQUIRE(actual.extData.size() == source.extData.size());
    REQUIRE(actual.pendingAppIdResolutions.size() == 2);
    CHECK(actual.pendingAppIdResolutions[0].indexInExtData == 0);
    CHECK(actual.pendingAppIdResolutions[0].handleRef == 0x23);
    CHECK(actual.pendingAppIdResolutions[1].indexInExtData == 7);
    CHECK(actual.pendingAppIdResolutions[1].handleRef == 0x67);
    REQUIRE(actual.pendingLayerRefResolutions.size() == 1);
    CHECK(actual.pendingLayerRefResolutions[0].indexInExtData == 5);
    CHECK(actual.pendingLayerRefResolutions[0].handleRef == 0x45);
    REQUIRE(actual.extData[3]->type() == DRW_Variant::STRING);
    CHECK(*actual.extData[3]->content.s == "LC_OFFSET_CURVE");
    CHECK(*actual.extData[4]->content.s == "{");
    CHECK(*actual.extData[6]->content.s == "}");
}

TEST_CASE("Entity textual XDATA checks actual source bytes and decodes only textual record kinds", "[offset-xdata][eed-packets][dxf]") {
    for (const bool binary : {false, true}) {
        CAPTURE(binary);
        const std::string raw = std::string(253, 'x') + "^I";
        std::string wire;
        if (binary) {
            wire.push_back(static_cast<char>(1000 & 255));
            wire.push_back(static_cast<char>(1000 >> 8));
            wire += raw; wire.push_back('\0');
        } else wire = "1000\n" + raw + "\n";
        std::istringstream stream(wire);
        std::unique_ptr<dxfReader> reader;
        if (binary) reader = std::make_unique<dxfReaderBinary>(&stream);
        else reader = std::make_unique<dxfReaderAscii>(&stream);
        reader->setVersion("AC1027", true);
        int code = 0; REQUIRE(reader->readRec(&code)); REQUIRE(code == 1000);
        EntityProbe entity;
        REQUIRE(entity.readCode(code, reader)); REQUIRE(entity.extData.size() == 1);
        CHECK(*entity.extData[0]->content.s == (binary ? raw : std::string(253, 'x') + '\t'));
        // A 256-byte wire string is refused even when caret decoding would
        // shrink it below 255. The binary reader must use the same raw bound.
        const auto over = raw + 'x';
        std::string overWire;
        if (binary) { overWire = wire.substr(0, 2) + over; overWire.push_back('\0'); }
        else overWire = "1000\n" + over + "\n";
        std::istringstream overStream(overWire);
        if (binary) reader = std::make_unique<dxfReaderBinary>(&overStream);
        else reader = std::make_unique<dxfReaderAscii>(&overStream);
        reader->setVersion("AC1027", true);
        REQUIRE(reader->readRec(&code));
        CHECK_FALSE(entity.readCode(code, reader)); CHECK(entity.extData.size() == 1);
    }
    for (const int kind : {1000, 1001, 1003, 1002, 1004, 1005}) {
        CAPTURE(kind);
        const std::string value = kind == 1004 ? "ABCD" : kind == 1005 ? "E9FE" : "^I";
        std::istringstream stream(std::to_string(kind) + "\n" + value + "\n");
        std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderAscii>(&stream);
        reader->setVersion("AC1015", true); reader->setCodePage("ANSI_1252");
        int code = 0; REQUIRE(reader->readRec(&code));
        EntityProbe entity; REQUIRE(entity.readCode(code, reader));
        REQUIRE(entity.extData.size() == 1);
        CHECK(*entity.extData[0]->content.s == (kind == 1000 ? "\t" : value));
    }
    for (const int kind : {1000, 1001, 1003}) {
        std::istringstream stream(std::to_string(kind) + "\n\xe0\n");
        std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderAscii>(&stream);
        reader->setVersion("AC1015", true); reader->setCodePage("ANSI_1251");
        int code = 0; REQUIRE(reader->readRec(&code));
        EntityProbe entity; REQUIRE(entity.readCode(code, reader));
        REQUIRE(entity.extData.size() == 1);
        CHECK(*entity.extData[0]->content.s == "\xd0\xb0");
    }
}
