/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

/**
 * Phase 4a self-consistency tests for per-entity DRW_*::encodeDwg.
 *
 * Pattern mirrors dwg_header_encode_round_trip_tests.cpp: encode an
 * entity via the protected encodeDwg, wrap the bytes as a dwgBuffer,
 * and parse them back into a fresh entity instance.  Compare the
 * relevant fields field-by-field.  Any bit-stream desync in the
 * common preamble or per-entity body shifts every following bit and
 * trips a downstream assertion.
 *
 * Each test runs for both AC1015 (R2000) and AC1018 (R2004) to cover the
 * version-specific preamble path (xDictFlag bit vs haveNextLinks bit) and
 * the unconditional null XDicObjH handle emitted by encodeDwgEntHandle.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

using Catch::Approx;

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"

/// Friend accessor for the protected encodeDwg / parseDwg + private
/// post-parse fields.  Declared as a friend by drw_entities.h via
/// SETENTFRIENDS.
class DrwEntityEncodeTestAccess {
public:
    static bool encode(DRW_Entity& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf, /*bs=*/0);
    }
    static bool encode(DRW_Entity& e, DRW::Version v, dwgBufferW* body,
                       dwgBufferW* strings, dwgBufferW* handles) {
        return e.encodeDwg(v, body, /*bs=*/0, strings, handles);
    }
    static bool parse(DRW_Entity& e, DRW::Version v, dwgBuffer* buf) {
        // Per-entity parseDwg overrides the abstract; both Point/Line/etc.
        // call DRW_Entity::parseDwg(buf, NULL, bs) themselves.  Use a
        // named std::uint32_t to disambiguate from the 4-arg parseDwg overload
        // (which has dwgBuffer* as its 3rd parameter).
        std::uint32_t bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static bool parse(DRW_Entity& e, DRW::Version v, dwgBuffer* buf,
                      std::uint32_t bodyBitSize) {
        return e.parseDwg(v, buf, bodyBitSize);
    }
    static bool parseCommon(DRW_Entity& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, static_cast<dwgBuffer*>(nullptr), 0);
    }
    static bool parseHandles(DRW_Entity& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwgEntHandle(v, buf, false);
    }
    static bool encodeCommon(DRW_Entity& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwgCommon(v, buf);
    }
    static bool encodeHandles(DRW_Entity& e, DRW::Version v,
                              dwgBufferW* body, dwgBufferW* handles) {
        return e.encodeDwgEntHandle(v, body, handles);
    }
    static bool parseProxy(DRW_ProxyEntity& e, DRW::Version v,
                           dwgBuffer* buf) {
        return e.parseDwg(v, buf, /*bs=*/0);
    }
    static bool parseProxy(DRW_ProxyEntity& e, DRW::Version v,
                           dwgBuffer* buf, std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static dwgHandle& layerH(DRW_Entity& e) { return e.layerH; }
    static std::uint16_t oType(const DRW_Entity& e) { return e.oType; }
    static void setOType(DRW_Entity& e, std::uint16_t value) { e.oType = value; }
    static std::uint8_t& hasDsData(DRW_Entity& e) { return e.hasDsData; }
    static std::uint8_t& ltFlags(DRW_Entity& e) { return e.ltFlags; }
    static std::uint8_t materialFlag(const DRW_Entity& e) {
        return e.materialFlag;
    }
    static std::uint8_t plotFlags(const DRW_Entity& e) {
        return e.plotFlags;
    }
    static std::uint8_t fullVisualFlag(const DRW_Entity& e) {
        return e.hasFullVisualStyle;
    }
    static std::uint8_t faceVisualFlag(const DRW_Entity& e) {
        return e.hasFaceVisualStyle;
    }
    static std::uint8_t edgeVisualFlag(const DRW_Entity& e) {
        return e.hasEdgeVisualStyle;
    }
};

namespace {

std::vector<std::uint8_t> snapshot(const dwgBufferW& w) { return w.data(); }

struct EncodedEntityFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBitSize = 0;
};

bool encodeThreeStreams(DRW_Entity& source, DRW::Version version,
                        EncodedEntityFrame& frame) {
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!DrwEntityEncodeTestAccess::encode(
            source, version, &body, &strings, &handles)) {
        return false;
    }

    body.alignToByte();
    const std::size_t stringBytes = strings.data().size();
    if (stringBytes != 0)
        body.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBytes * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(stringBytes != 0 ? 1 : 0);
    body.alignToByte();

    handles.alignToByte();
    const std::size_t handleStart = body.size();
    if (!handles.data().empty())
        body.putBytes(handles.data().data(), handles.data().size());
    const std::size_t handleBytes = body.size() - handleStart;
    if (handleBytes > std::numeric_limits<std::uint32_t>::max() / 8u)
        return false;

    frame.bytes = body.data();
    frame.handleBitSize = static_cast<std::uint32_t>(handleBytes * 8u);
    return true;
}

std::vector<std::uint8_t> makeProxyEntityBody(DRW::Version version,
                                              bool truncated = false,
                                              std::uint8_t objectIdCode = 3,
                                              bool nonFiniteLtypeScale = false) {
    dwgBufferW body;
    body.putObjType(version, DRW_ProxyEntity::kDwgType);
    const std::uint32_t objectSizeBit = body.bitCount();
    body.putRawLong32(0);             // AC1015/AC1018 object-size field

    dwgHandle entityHandle;
    entityHandle.code = 4;
    entityHandle.ref = 0x89;
    body.putHandle(entityHandle);
    body.putBitShort(0);              // no EED
    body.putBit(0);                   // no proxy graphics
    body.put2Bits(2);                 // model space, no owner
    body.putBitLong(0);               // no reactors
    if (version == DRW::AC1015)
        body.putBit(1);               // no previous/next links
    else
        body.putBit(1);               // no extension dictionary
    body.putEnColor(version, 256);    // BYLAYER
    body.putBitDouble(nonFiniteLtypeScale
                          ? std::numeric_limits<double>::quiet_NaN()
                          : 1.0);
    body.put2Bits(0);                 // BYLAYER linetype
    body.put2Bits(0);                 // default plot style
    body.putBitShort(0);              // visible
    body.putRawChar8(29);             // BYLAYER lineweight

    if (truncated) {
        const std::uint32_t declaredEnd = body.bitCount() + 1;
        body.put2Bits(0);             // overrun BL prefix after the frame
        body.patchRawLong32AtBit(objectSizeBit, declaredEnd);
    } else {
        body.putBitLong(501);         // proxy carrier/class id
        if (version == DRW::AC1018)
            body.putVariableText(version, "ProxyEntity");
        body.putBitLong(0x12340056);  // combined drawing format
        body.putBit(1);               // source was DXF
        body.putRawChar8(0xAB);
        body.putRawChar8(0xCD);
        body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
    }

    if (version == DRW::AC1015)
        body.putHandle(dwgHandle{});  // extension dictionary
    dwgHandle layerHandle;
    layerHandle.code = 5;
    layerHandle.ref = 0x12;
    body.putHandle(layerHandle);
    dwgHandle objectId;
    objectId.code = objectIdCode;
    objectId.ref = 0x40;
    body.putHandle(objectId);
    body.putHandle(dwgHandle{});      // proxy reference terminator
    return body.data();
}

struct ModernProxyEntityFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBitSize = 0;
};

ModernProxyEntityFixture makeModernProxyEntityBody() {
    DRW_ProxyEntity source;
    source.handle = 0x8Au;
    source.color = 256;
    DrwEntityEncodeTestAccess::setOType(source, DRW_ProxyEntity::kDwgType);

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!DrwEntityEncodeTestAccess::encodeCommon(
            source, DRW::AC1027, &body)
        || !DrwEntityEncodeTestAccess::encodeHandles(
            source, DRW::AC1027, &body, &handles)) {
        return {};
    }

    body.putBitLong(501); // proxy carrier/class id
    body.putBitLong(0x12340056); // combined drawing format
    body.putBit(true); // source was DXF
    body.putRawChar8(0xAB);
    body.putRawChar8(0xCD);

    strings.putVariableText(DRW::AC1027, "");
    const std::vector<std::uint8_t> marker = {0xD1u, 0xE2u, 0xF3u, 0x04u};
    strings.putBytes(marker.data(), marker.size());
    strings.alignToByte();
    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int bit = 0; bit < 7; ++bit)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(1);
    body.alignToByte();

    dwgHandle nullHandle;
    handles.putHandle(nullHandle); // end of proxy reference tail
    handles.alignToByte();
    const std::uint32_t handleBitSize = handles.bitCount();
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBitSize};
}

struct GeoPositionMarkerFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBitSize = 0;
};

GeoPositionMarkerFixture makeGeoPositionMarkerWithEmbeddedMText() {
    DRW_GeoPositionMarker source;
    source.handle = 0x89;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;
    source.color = 256;
    source.ltypeScale = 1.0;
    DrwEntityEncodeTestAccess::setOType(source,
                                        DRW_GeoPositionMarker::kDwgType);

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        source, DRW::AC1027, &body));

    body.putBitLong(0); // class_version
    body.put3BitDouble(DRW_Coord{100.0, 200.0, 5.0});
    body.putBitDouble(3.5);
    strings.putVariableText(DRW::AC1027, "Survey note");
    body.putBitDouble(1.25);
    body.putBit(1); // mtext_visible
    body.putRawChar8(2); // text_alignment
    body.putBit(1); // enable_frame_text

    // Minimal AcDbMTextObjectEmbedded body. The zero/default choices select
    // BYLAYER/default handles while still exercising the nested stream.
    body.put2Bits(2); // no owner
    body.putBitLong(0); // reactors
    body.putBit(1); // no xdictionary
    body.putBit(0); // no DataStorage payload
    body.putEnColor(DRW::AC1027, 256, -1, "Entry", "Book",
                    0x03000080u, true);
    body.putBitDouble(1.0);
    body.put2Bits(0); // linetype BYLAYER
    body.put2Bits(0); // plot style default
    body.put2Bits(0); // material BYLAYER
    body.putRawChar8(0); // no shadow
    body.putBit(0); // no full visual style
    body.putBit(0); // no face visual style
    body.putBit(0); // no edge visual style
    body.putBitShort(0); // visible
    body.putRawChar8(0); // lineweight BYLAYER
    body.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
    body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0});
    body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0});
    body.putBitDouble(1.0); // width scale
    body.putBitDouble(0.0); // R2007 rectangle height
    body.putBitDouble(1.0); // text height
    body.putBitShort(1); // text generation
    body.putBitShort(1); // attachment
    body.putBitDouble(0.0); // extents height
    body.putBitDouble(0.0); // extents width
    strings.putVariableText(DRW::AC1027, "Embedded text");
    body.putBitShort(0); // column type padding
    body.putBitDouble(1.0); // line spacing factor
    body.putBit(0); // unknown flag
    body.putBitLong(0); // background flags
    body.putBitShort(0); // no annotative data

    REQUIRE(DrwEntityEncodeTestAccess::encodeHandles(
        source, DRW::AC1027, &body, &handles));

    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u);
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(stringBitSize);
    body.putBit(1); // strings present
    body.alignToByte();

    const std::size_t bodySize = body.size();
    body.putBytes(handles.data().data(), handles.data().size());
    auto putHardHandle = [&body](std::uint32_t ref) {
        dwgHandle h;
        h.code = 5;
        h.ref = ref;
        h.size = 1;
        body.putHandle(h);
    };
    putHardHandle(0x35); // embedded MText AcDbColor
    putHardHandle(0x12); // embedded MText layer
    putHardHandle(0x13); // embedded MText style

    const std::uint32_t handleBitSize = static_cast<std::uint32_t>(
        (body.size() - bodySize) * 8u);
    return {body.data(), handleBitSize};
}

} // namespace

TEST_CASE("DRW_ProxyEntity decodes proxy metadata and reference tails",
          "[dwg-read][proxy-entity]") {
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018}) {
        const auto bytes = makeProxyEntityBody(version);
        dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        DRW_ProxyEntity entity;
        REQUIRE(DrwEntityEncodeTestAccess::parseProxy(entity, version, &buffer));
        CHECK(entity.handle == 0x89);
        CHECK(entity.m_hasProxyCarrierId);
        CHECK(entity.m_proxyCarrierId == 501);
        CHECK(entity.m_hasProxyDrawingFormat);
        CHECK(entity.m_proxyDrawingFormat == 0x12340056u);
        CHECK(entity.m_proxyDwgVersion == 0x0056u);
        CHECK(entity.m_proxyMaintenanceVersion == 0x1234u);
        CHECK(entity.m_hasFromDxf);
        CHECK(entity.m_fromDxf);
        if (version == DRW::AC1018)
            CHECK(entity.m_proxySubclass == "ProxyEntity");
        else
            CHECK(entity.m_proxySubclass.empty());
        REQUIRE(entity.m_entityData.size() >= 2);
        CHECK(entity.m_entityDataBitSize == 16);
        CHECK(entity.m_entityData[0] == 0xAB);
        CHECK(entity.m_entityData[1] == 0xCD);
        REQUIRE(entity.m_objectIdRefs.size() == 1);
        CHECK(entity.m_objectIdRefs[0].m_handleCode == 3);
        CHECK(entity.m_objectIdRefs[0].m_dxfCode == 340);
        CHECK(entity.m_objectIdRefs[0].m_rawHandle == 0x40);
        CHECK(buffer.isGood());
    }

    for (const std::uint8_t handleCode : {4u, 6u, 8u, 10u, 12u}) {
        const auto bytes = makeProxyEntityBody(DRW::AC1018, false, handleCode);
        dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        DRW_ProxyEntity entity;
        REQUIRE(DrwEntityEncodeTestAccess::parseProxy(
            entity, DRW::AC1018, &buffer));
        REQUIRE(entity.m_objectIdRefs.size() == 1);
        CHECK(entity.m_objectIdRefs[0].m_handleCode == handleCode);
        CHECK(entity.m_objectIdRefs[0].m_dxfCode == 350);
    }
}

TEST_CASE("DRW_ProxyEntity rejects an overrun metadata field",
          "[dwg-read][proxy-entity][safety]") {
    const auto bytes = makeProxyEntityBody(DRW::AC1015, /*truncated=*/true);
    dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    DRW_ProxyEntity entity;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parseProxy(
        entity, DRW::AC1015, &buffer));
}

TEST_CASE("DRW_ProxyEntity rejects non-finite numeric fields",
          "[dwg-read][proxy-entity][safety]") {
    const auto bytes = makeProxyEntityBody(
        DRW::AC1018, /*truncated=*/false, /*objectIdCode=*/3,
        /*nonFiniteLtypeScale=*/true);
    dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    DRW_ProxyEntity entity;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parseProxy(
        entity, DRW::AC1018, &buffer));
    CHECK_FALSE(entity.m_hasProxyCarrierId);
}

TEST_CASE("DRW_ProxyEntity excludes detached strings from opaque data",
          "[dwg-read][proxy-entity][safety]") {
    const auto fixture = makeModernProxyEntityBody();
    REQUIRE(!fixture.bytes.empty());

    dwgBuffer buffer(const_cast<std::uint8_t*>(fixture.bytes.data()),
                     fixture.bytes.size());
    DRW_ProxyEntity entity;
    REQUIRE(DrwEntityEncodeTestAccess::parseProxy(
        entity, DRW::AC1027, &buffer, fixture.handleBitSize));
    CHECK(entity.m_entityDataBitSize >= 16);
    REQUIRE(entity.m_entityData.size() >= 2);
    CHECK(entity.m_entityData[0] == 0xABu);
    CHECK(entity.m_entityData[1] == 0xCDu);
    const std::vector<std::uint8_t> marker = {0xD1u, 0xE2u, 0xF3u, 0x04u};
    CHECK(std::search(entity.m_entityData.cbegin(), entity.m_entityData.cend(),
                      marker.cbegin(), marker.cend())
          == entity.m_entityData.cend());
}

TEST_CASE("DRW_GeoPositionMarker consumes its embedded MText handle tail",
          "[dwg-read][geo-position-marker]") {
    const auto fixture = makeGeoPositionMarkerWithEmbeddedMText();
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    dwgBuffer buffer(const_cast<std::uint8_t*>(fixture.bytes.data()),
                     fixture.bytes.size(), &codec);
    DRW_GeoPositionMarker marker;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        marker, DRW::AC1027, &buffer, fixture.handleBitSize));
    CHECK(marker.handle == 0x89);
    CHECK(marker.m_classVersion == 0);
    CHECK(marker.m_position.x == Approx(100.0));
    CHECK(marker.m_position.y == Approx(200.0));
    CHECK(marker.m_position.z == Approx(5.0));
    CHECK(marker.m_radius == Approx(3.5));
    CHECK(marker.m_notes == "Survey note");
    CHECK(marker.m_landingGap == Approx(1.25));
    CHECK(marker.m_mtextVisible);
    CHECK(marker.m_textAlignment == 2);
    CHECK(marker.m_enableFrameText);
    REQUIRE(marker.mtext != nullptr);
    CHECK(marker.mtext->text == "Embedded text");
    CHECK(marker.mtext->colorName == "Book$Entry");
    CHECK(marker.mtext->transparency == 0x03000080);
    CHECK(marker.mtext->hasDwgAcDbColorHandle());
    CHECK(marker.mtext->dwgAcDbColorHandle() == 0x35u);
    CHECK(marker.mtext->dwgLayerHandle() == 0x12u);
    CHECK(marker.mtext->basePoint.x == Approx(0.0));
    CHECK(marker.mtext->height == Approx(1.0));
    CHECK(buffer.isGood());
}

TEST_CASE("DRW_GeoPositionMarker encodes AC1027 and AC1032 wire forms",
          "[dwg-write][geo-position-marker]") {
    auto encodeThreeStreams = [](DRW_GeoPositionMarker &source,
                                 DRW::Version version) {
        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        REQUIRE(DrwEntityEncodeTestAccess::encode(
            source, version, &body, &strings, &handles));

        body.alignToByte();
        const std::size_t stringBytes = strings.data().size();
        if (stringBytes != 0)
            body.putBytes(strings.data().data(), stringBytes);
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        const auto stringBitSize = static_cast<std::uint16_t>(
            stringBytes == 0 ? 0 : stringBytes * 8u + 7u);
        body.putRawShort16(stringBitSize);
        body.putBit(stringBytes != 0 ? 1 : 0);
        body.alignToByte();

        handles.alignToByte();
        const std::size_t dataBytes = body.size();
        if (!handles.data().empty())
            body.putBytes(handles.data().data(), handles.data().size());
        const std::uint32_t handleBitSize = static_cast<std::uint32_t>(
            (body.size() - dataBytes) * 8u);
        return std::make_pair(body.data(), handleBitSize);
    };

    DRW_GeoPositionMarker source;
    source.handle = 0x89;
    source.color = 256;
    source.ltypeScale = 1.0;
    source.m_classVersion = 3;
    source.m_position = DRW_Coord{10.0, 20.0, 30.0};
    source.m_radius = 4.5;
    source.m_notes = "Marker note";
    source.m_landingGap = 0.75;
    source.m_mtextVisible = true;
    source.m_textAlignment = 2;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;

    for (const DRW::Version version : {DRW::AC1027, DRW::AC1032}) {
        auto encoded = encodeThreeStreams(source, version);
        REQUIRE(!encoded.first.empty());

        DRW_TextCodec codec;
        codec.setVersion(version, false);
        dwgBuffer buffer(encoded.first.data(), encoded.first.size(), &codec);
        DRW_GeoPositionMarker parsed;
        REQUIRE(DrwEntityEncodeTestAccess::parse(
            parsed, version, &buffer, encoded.second));
        CHECK(parsed.handle == source.handle);
        CHECK(parsed.m_classVersion == source.m_classVersion);
        CHECK(parsed.m_position.x == Approx(source.m_position.x));
        CHECK(parsed.m_position.y == Approx(source.m_position.y));
        CHECK(parsed.m_position.z == Approx(source.m_position.z));
        CHECK(parsed.m_radius == Approx(source.m_radius));
        CHECK(parsed.m_notes == source.m_notes);
        CHECK(parsed.m_landingGap == Approx(source.m_landingGap));
        CHECK(parsed.m_mtextVisible == source.m_mtextVisible);
        CHECK(parsed.m_textAlignment == source.m_textAlignment);
        CHECK_FALSE(parsed.m_enableFrameText);
        CHECK(buffer.isGood());
    }

    source.m_enableFrameText = true;
    dwgBufferW rejected;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        source, DRW::AC1027, &rejected));

    source.mtext = std::make_unique<DRW_MText>();
    source.mtext->text = "Embedded marker text";
    source.mtext->basePoint = source.m_position;
    source.mtext->height = 1.0;
    source.mtext->widthscale = 1.0;
    source.mtext->interlin = 1.0;
    source.mtext->color = 256;
    source.mtext->visible = true;
    source.mtext->parentHandle = DRW::NoHandle;
    DrwEntityEncodeTestAccess::layerH(*source.mtext).ref = 0x12;

    auto encoded = encodeThreeStreams(source, DRW::AC1032);
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer buffer(encoded.first.data(), encoded.first.size(), &codec);
    DRW_GeoPositionMarker parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1032, &buffer, encoded.second));
    REQUIRE(parsed.mtext != nullptr);
    CHECK(parsed.m_enableFrameText);
    CHECK(parsed.mtext->text == source.mtext->text);
    CHECK(parsed.mtext->basePoint.x == Approx(source.mtext->basePoint.x));
    CHECK(parsed.mtext->basePoint.y == Approx(source.mtext->basePoint.y));
    CHECK(parsed.mtext->basePoint.z == Approx(source.mtext->basePoint.z));
    CHECK(buffer.isGood());
}

TEST_CASE("DRW_SectionObject encodes the AC1021 field order",
          "[dwg-write][section-object]") {
    DRW_SectionObject source;
    source.handle = 0x8A;
    source.parentHandle = 0x17;
    source.m_state = 1;
    source.m_flags = 5;
    source.m_name = "Section Plane";
    source.m_vertDir = DRW_Coord{0.0, 0.0, 1.0};
    source.m_topHeight = 5.0;
    source.m_bottomHeight = 15.0;
    source.m_indicatorAlpha = 70;
    source.m_indicatorColor = 7;
    source.m_verts = {DRW_Coord{1.0, 2.0, 0.0},
                      DRW_Coord{3.0, 4.0, 0.0}};
    source.m_blVerts = {DRW_Coord{5.0, 6.0, 0.0}};
    source.m_sectionSettingsHandle = 0x22A;
    source.color = 256;
    source.ltypeScale = 1.0;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;

    dwgBufferW encoded;
    REQUIRE(DrwEntityEncodeTestAccess::encode(source, DRW::AC1021, &encoded));
    REQUIRE(!encoded.data().empty());
    dwgBuffer typeReader(encoded.data().data(), encoded.data().size());
    CHECK(typeReader.getObjType(DRW::AC1021)
          == DRW_SectionObject::kDwgClassNum);

    DRW_SectionObject invalid;
    invalid.m_verts.resize(DRW_SectionObject::kMaxVertices + 1);
    dwgBufferW invalidEncoded;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalid, DRW::AC1021, &invalidEncoded));
}

TEST_CASE("DRW_SectionObject::parseDwg rejects counts outside the body",
          "[dwg-read][entity-encode][section-object][safety]") {
    const auto makeBody = [](bool oversizedBackLineCount) {
        DRW_SectionObject section;
        section.handle = 0x8B;
        DrwEntityEncodeTestAccess::setOType(
            section, DRW_SectionObject::kDwgClassNum);

        dwgBufferW body;
        if (!DrwEntityEncodeTestAccess::encodeCommon(
                section, DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        body.putBitLong(0); // state
        body.putBitLong(0); // flags
        body.putVariableText(DRW::AC1018, "Section");
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0});
        body.putBitDouble(0.0); // top height
        body.putBitDouble(0.0); // bottom height
        body.putBitShort(0); // indicator alpha
        body.putCmColor(DRW::AC1018, 7); // indicator color
        body.putBitLong(oversizedBackLineCount
                            ? 0
                            : static_cast<std::int32_t>(
                                  DRW_SectionObject::kMaxVertices));
        body.putBitLong(oversizedBackLineCount
                            ? static_cast<std::int32_t>(
                                  DRW_SectionObject::kMaxVertices)
                            : 0);

        const std::uint8_t bsCode =
            static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
        const std::size_t objectSizeBit =
            bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
        body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
        return body.data();
    };

    for (const bool oversizedBackLineCount : {false, true}) {
        auto bytes = makeBody(oversizedBackLineCount);
        REQUIRE_FALSE(bytes.empty());
        DRW_SectionObject section;
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
            section, DRW::AC1018, &reader));
        CHECK(section.m_verts.empty());
        CHECK(section.m_blVerts.empty());
    }
}

TEST_CASE("DRW_PointCloud::parseDwg rejects nested counts outside the body",
          "[dwg-read][entity-encode][pointcloud][safety]") {
    const auto makeBody = [](bool oversizedClipVertices) {
        DRW_PointCloud pointCloud;
        pointCloud.handle = 0x8C;
        DrwEntityEncodeTestAccess::setOType(
            pointCloud, DRW_PointCloud::kDwgClassNum);

        dwgBufferW body;
        dwgBufferW strings;
        if (!DrwEntityEncodeTestAccess::encodeCommon(
                pointCloud, DRW::AC1027, &body))
            return std::vector<std::uint8_t>{};
        body.putBitShort(0); // class version
        body.put3BitDouble(DRW_Coord{}); // origin
        strings.putVariableText(DRW::AC1027, ""); // saved filename
        body.putBitLong(oversizedClipVertices ? 0 :
                        static_cast<std::int32_t>(DRW_PointCloud::kMaxItems));
        if (!oversizedClipVertices) {
            body.alignToByte();
        } else {
            body.put3BitDouble(DRW_Coord{}); // extents min
            body.put3BitDouble(DRW_Coord{}); // extents max
            body.putRawLong64(0); // point count
            strings.putVariableText(DRW::AC1027, ""); // UCS name
            body.put3BitDouble(DRW_Coord{}); // UCS origin
            body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0}); // UCS X
            body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0}); // UCS Y
            body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0}); // UCS Z
            body.putBit(0); // show intensity
            body.putBitShort(0); // intensity scheme
            body.putBitDouble(0.0);
            body.putBitDouble(0.0);
            body.putBitDouble(0.0);
            body.putBitDouble(0.0);
            body.putBit(1); // show clipping
            body.putBitLong(1); // one clipping record
            body.putBit(0); // not inverted
            body.putBitShort(3); // arbitrary polygon
            body.putBitLong(static_cast<std::int32_t>(
                DRW_PointCloud::kMaxItems));
        }
        body.alignToByte();
        if (!strings.data().empty())
            body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        return body.data();
    };

    for (const bool oversizedClipVertices : {false, true}) {
        auto bytes = makeBody(oversizedClipVertices);
        REQUIRE_FALSE(bytes.empty());
        DRW_PointCloud pointCloud;
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
            pointCloud, DRW::AC1027, &reader));
        CHECK(pointCloud.sourceFiles.empty());
        CHECK(pointCloud.clippings.empty());
    }
}

TEST_CASE("DRW_PointCloudEx::parseDwg rejects nested counts outside the body",
          "[dwg-read][entity-encode][pointcloud][safety]") {
    const auto makeBody = [](bool oversizedPointCount) {
        DRW_PointCloudEx pointCloud;
        pointCloud.handle = 0x8D;
        DrwEntityEncodeTestAccess::setOType(
            pointCloud, DRW_PointCloudEx::kDwgClassNum);

        dwgBufferW body;
        dwgBufferW strings;
        if (!DrwEntityEncodeTestAccess::encodeCommon(
                pointCloud, DRW::AC1027, &body))
            return std::vector<std::uint8_t>{};
        body.putBitShort(0); // class version
        body.put3BitDouble(DRW_Coord{}); // extents min
        body.put3BitDouble(DRW_Coord{}); // extents max
        body.put3BitDouble(DRW_Coord{}); // UCS origin
        body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0}); // UCS X
        body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0}); // UCS Y
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0}); // UCS Z
        body.putBit(0); // locked
        strings.putVariableText(DRW::AC1027, ""); // name
        body.putBit(0); // show intensity
        body.putBit(1); // show cropping
        body.putBitLong(oversizedPointCount ? 1 :
                        static_cast<std::int32_t>(DRW_PointCloudEx::kMaxItems));
        if (oversizedPointCount) {
            body.putBitShort(3); // cropping type
            body.putBit(1); // inside
            body.putBit(0); // not inverted
            body.put3BitDouble(DRW_Coord{}); // crop plane
            body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0}); // crop X
            body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0}); // crop Y
            body.putBitLong(static_cast<std::int32_t>(
                DRW_PointCloudEx::kMaxItems));
        }
        body.alignToByte();
        if (!strings.data().empty())
            body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        return body.data();
    };

    for (const bool oversizedPointCount : {false, true}) {
        auto bytes = makeBody(oversizedPointCount);
        REQUIRE_FALSE(bytes.empty());
        DRW_PointCloudEx pointCloud;
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
            pointCloud, DRW::AC1027, &reader));
        CHECK(pointCloud.croppings.empty());
    }
}

TEST_CASE("DRW_Point::encodeDwg round-trips coordinates and thickness",
          "[dwg-write][entity-encode]") {
    DRW_Point src;
    src.handle = 0x33;          // arbitrary user-handle past the reserved set
    src.color = 7;              // ACI white
    src.ltypeScale = 1.0;
    src.basePoint = DRW_Coord{12.5, -34.75, 100.0};
    src.thickness = 0.0;        // BT shortcut path
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};  // BE shortcut path
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;  // layer "0"

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle        == 0x33u);
        REQUIRE(dst.color         == 7);
        REQUIRE(dst.ltypeScale    == 1.0);
        REQUIRE(dst.basePoint.x   == 12.5);
        REQUIRE(dst.basePoint.y   == -34.75);
        REQUIRE(dst.basePoint.z   == 100.0);
        REQUIRE(dst.thickness     == 0.0);
        REQUIRE(dst.extPoint.x    == 0.0);
        REQUIRE(dst.extPoint.y    == 0.0);
        REQUIRE(dst.extPoint.z    == 1.0);
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
    }
}

TEST_CASE("DRW_Wipeout uses fixed DWG entity type 1109",
          "[dwg-write][entity-encode][wipeout]") {
    DRW_Wipeout source;
    source.handle = 0x34;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;
    source.basePoint = DRW_Coord{10.0, 20.0, 0.0};
    source.secPoint = DRW_Coord{1.0, 0.0, 0.0};
    source.vVector = DRW_Coord{0.0, 1.0, 0.0};
    source.sizeu = 5.0;
    source.sizev = 4.0;
    source.m_clipBoundaryType = 1;
    source.clipPath = {DRW_Coord{-0.5, -0.5, 0.0},
                       DRW_Coord{4.5, 3.5, 0.0}};

    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW encoded;
        REQUIRE(DrwEntityEncodeTestAccess::encode(source, version, &encoded));
        REQUIRE(!encoded.data().empty());

        dwgBuffer typeReader(encoded.data().data(), encoded.data().size());
        CHECK(typeReader.getObjType(version) == 1109);

        dwgBuffer bodyReader(encoded.data().data(), encoded.data().size());
        DRW_Wipeout parsed;
        REQUIRE(DrwEntityEncodeTestAccess::parse(parsed, version, &bodyReader));
        CHECK(parsed.m_clipBoundaryType == 1);
        REQUIRE(parsed.clipPath.size() == 2);
        CHECK(parsed.clipPath.front().x == Approx(-0.5));
        CHECK(parsed.clipPath.back().x == Approx(4.5));
    }

    DRW_Wipeout invalid;
    dwgBufferW invalidEncoded;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalid, DRW::AC1015, &invalidEncoded));
}

TEST_CASE("DRW_Point::encodeDwg round-trips non-default thickness + extrusion",
          "[dwg-write][entity-encode]") {
    DRW_Point src;
    src.handle = 0x4A;
    src.color = 256;            // BYLAYER
    src.ltypeScale = 2.5;
    src.basePoint = DRW_Coord{0.5, 0.25, -1.0};
    src.thickness = 0.125;
    src.extPoint = DRW_Coord{0.1, 0.2, 0.97};  // forces full BE emit
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x4Au);
        REQUIRE(dst.color       == 256);
        REQUIRE(dst.ltypeScale  == 2.5);
        REQUIRE(dst.basePoint.x == 0.5);
        REQUIRE(dst.basePoint.y == 0.25);
        REQUIRE(dst.basePoint.z == -1.0);
        REQUIRE(dst.thickness   == 0.125);
        REQUIRE(dst.extPoint.x  == 0.1);
        REQUIRE(dst.extPoint.y  == 0.2);
        REQUIRE(dst.extPoint.z  == 0.97);
    }
}

TEST_CASE("DRW_Line::encodeDwg round-trips both Z paths",
          "[dwg-write][entity-encode]") {
    // Z==0 shortcut: zIsZero=1 path.
    {
        DRW_Line src;
        src.handle = 0x55;
        src.color = 1;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        src.secPoint  = DRW_Coord{10.0, 20.0, 0.0};
        src.thickness = 0.0;
        src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x == 1.0);
            REQUIRE(dst.basePoint.y == 2.0);
            REQUIRE(dst.basePoint.z == 0.0);
            REQUIRE(dst.secPoint.x  == 10.0);
            REQUIRE(dst.secPoint.y  == 20.0);
            REQUIRE(dst.secPoint.z  == 0.0);
        }
    }
    // Z!=0 full path: zIsZero=0, RD z + DD secZ.
    {
        DRW_Line src;
        src.handle = 0x56;
        src.color = 5;
        src.basePoint = DRW_Coord{3.0, 4.0, 5.0};
        src.secPoint  = DRW_Coord{6.0, 7.0, 8.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x == 3.0);
            REQUIRE(dst.basePoint.y == 4.0);
            REQUIRE(dst.basePoint.z == 5.0);
            REQUIRE(dst.secPoint.x  == 6.0);
            REQUIRE(dst.secPoint.y  == 7.0);
            REQUIRE(dst.secPoint.z  == 8.0);
        }
    }
}

TEST_CASE("DRW_Circle::encodeDwg round-trips center + radius",
          "[dwg-write][entity-encode]") {
    DRW_Circle src;
    src.handle = 0x60;
    src.color = 3;
    src.basePoint = DRW_Coord{50.0, 50.0, 0.0};
    src.radious = 12.5;
    src.thickness = 0.0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Circle dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x60u);
        REQUIRE(dst.basePoint.x == 50.0);
        REQUIRE(dst.basePoint.y == 50.0);
        REQUIRE(dst.radious     == 12.5);
    }
}

TEST_CASE("DRW_Arc::encodeDwg round-trips center + radius + angles",
          "[dwg-write][entity-encode]") {
    DRW_Arc src;
    src.handle = 0x70;
    src.color = 6;
    src.basePoint = DRW_Coord{-10.0, 25.5, 0.0};
    src.radious = 8.0;
    src.staangle = 0.0;
    src.endangle = 1.5707963267948966;  // π/2
    src.thickness = 0.0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Arc dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x70u);
        REQUIRE(dst.basePoint.x == -10.0);
        REQUIRE(dst.basePoint.y == 25.5);
        REQUIRE(dst.radious     == 8.0);
        REQUIRE(dst.staangle    == 0.0);
        REQUIRE(dst.endangle    == 1.5707963267948966);
    }
}

TEST_CASE("DRW_Text::encodeDwg round-trips string + position + style",
          "[dwg-write][entity-encode]") {
    DRW_Text src;
    src.handle = 0x90;
    src.color = 7;
    src.basePoint = DRW_Coord{100.0, 50.0, 0.0};
    src.secPoint = src.basePoint;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    src.height = 2.5;
    src.angle = 45.0;            // degrees
    src.widthscale = 1.0;
    src.oblique = 0.0;
    src.text = "HELLO";
    src.textgen = 0;
    src.alignH = DRW_Text::HLeft;
    src.alignV = DRW_Text::VBaseLine;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Text dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x90u);
        REQUIRE(dst.basePoint.x == 100.0);
        REQUIRE(dst.basePoint.y == 50.0);
        REQUIRE(dst.height      == 2.5);
        REQUIRE(dst.text        == "HELLO");
        REQUIRE(dst.widthscale  == 1.0);
        REQUIRE(dst.oblique     == 0.0);
        // angle round-trips degrees → radians on disk → degrees on read.
        // Floating-point exact match holds for the canonical π/4 conversion
        // path, but allow a tiny epsilon for safety.
        REQUIRE(std::abs(dst.angle - 45.0) < 1e-9);
    }
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a control-point cubic",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF0;
    src.color = 5;
    src.flags = 8;           // planar
    src.degree = 3;
    src.tolknot    = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist  = {0, 0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 0.0, 0.0}));
    src.nknots = 8;
    src.ncontrol = 4;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.degree            == 3);
        REQUIRE(dst.nknots            == 8);
        REQUIRE(dst.ncontrol          == 4);
        REQUIRE(dst.knotslist.size()  == 8);
        REQUIRE(dst.controllist.size() == 4);
        REQUIRE(dst.knotslist[0]      == 0.0);
        REQUIRE(dst.knotslist[7]      == 1.0);
        REQUIRE(dst.controllist[0]->x == 0.0);
        REQUIRE(dst.controllist[3]->x == 3.0);
    }
}

TEST_CASE("DRW_Spline::encodeDwg preserves rational quadratic weights",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF2;
    src.color = 2;
    src.flags = 8; // planar; encoder derives rational from non-default weights
    src.degree = 2;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 1.0, 0.0}));
    src.weightlist = {1.0, std::sqrt(0.5), 1.0};
    src.nknots = 6;
    src.ncontrol = 3;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE((dst.flags & 0x04) == 0x04);
        REQUIRE(dst.weightlist.size() == 3);
        CHECK(dst.weightlist[1] == Approx(std::sqrt(0.5)));
    }
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a fit-point spline",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF1;
    src.color = 6;
    src.flags = 8;
    src.degree = 3;
    src.tolfit = 1e-9;
    src.tgStart = DRW_Coord{1.0, 0.0, 0.0};
    src.tgEnd   = DRW_Coord{0.0, 1.0, 0.0};
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{5.0, 5.0, 0.0}));
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{10.0, 0.0, 0.0}));
    src.nfit = 3;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.degree         == 3);
        REQUIRE(dst.nfit           == 3);
        REQUIRE(dst.fitlist.size() == 3);
        REQUIRE(dst.fitlist[1]->x  == 5.0);
        REQUIRE(dst.fitlist[1]->y  == 5.0);
        REQUIRE(dst.tgStart.x      == 1.0);
        REQUIRE(dst.tgEnd.y        == 1.0);
    }
}

TEST_CASE("DRW_Spline::parseDwg rejects impossible control-point layout",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF3;
    src.color = 3;
    src.flags = 8;
    src.degree = 3;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 0.0, 0.0}));
    src.nknots = 7;
    src.ncontrol = 3; // degree 3 requires at least 4 controls
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(src, DRW::AC1018, &w));
    // Build the malformed body directly. The public encoder must reject this
    // layout, while the parser still needs a regression fixture for it.
    w.putBitLong(1); // control-point scenario
    w.putBit(0);     // rational
    w.putBit(0);     // closed
    w.putBit(0);     // periodic
    w.putBitDouble(src.tolknot);
    w.putBitDouble(src.tolcontrol);
    w.putBitLong(src.nknots);
    w.putBitLong(src.ncontrol);
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Spline dst;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1018, &r));
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a high-degree control spline",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF4;
    src.color = 4;
    src.flags = 8;
    src.degree = 4;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.5, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{4.0, 0.0, 0.0}));
    src.nknots = 10;
    src.ncontrol = 5;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1018, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Spline dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1018, &r));

    REQUIRE(dst.degree == 4);
    REQUIRE(dst.ncontrol == 5);
    REQUIRE(dst.nknots == 10);
    REQUIRE(dst.controllist.size() == 5);
}

TEST_CASE("DRW_MText::encodeDwg round-trips multi-line text",
          "[dwg-write][entity-encode]") {
    DRW_MText src;
    src.handle = 0xE0;
    src.color = 7;
    src.basePoint = DRW_Coord{10.0, 20.0, 0.0};
    src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
    src.secPoint  = DRW_Coord{1.0, 0.0, 0.0};   // X-axis = horizontal
    src.widthscale = 100.0;
    src.height = 3.5;
    src.textgen = static_cast<int>(DRW_MText::TopLeft);
    src.alignH = static_cast<DRW_Text::HAlign>(1);  // LtoR
    src.text = "Line one\\PLine two\\PLine three";
    src.interlin = 1.5;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_MText dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0xE0u);
        REQUIRE(dst.basePoint.x == 10.0);
        REQUIRE(dst.basePoint.y == 20.0);
        REQUIRE(dst.widthscale  == 100.0);
        REQUIRE(dst.height      == 3.5);
        REQUIRE(dst.textgen     == 1);  // TopLeft
        REQUIRE(dst.text        == "Line one\\PLine two\\PLine three");
        REQUIRE(dst.interlin    == 1.5);
    }
}

TEST_CASE("DRW_MText preserves AC1032 column heights",
          "[dwg-write][dwg-read][mtext][ac1032]") {
    DRW_MText source;
    source.handle = 0xE1;
    source.color = 256;
    source.basePoint = DRW_Coord{10.0, 20.0, 0.0};
    source.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    source.secPoint = DRW_Coord{1.0, 0.0, 0.0};
    source.widthscale = 40.0;
    source.height = 3.5;
    source.textgen = static_cast<int>(DRW_MText::TopLeft);
    source.alignH = static_cast<DRW_Text::HAlign>(1);
    source.text = "Column one\\PColumn two";
    source.interlin = 1.5;
    source.m_r2018IsNotAnnotative = true;
    source.m_r2018Version = 1;
    source.m_r2018DefaultFlag = true;
    source.m_r2018Attachment = DRW_MText::MiddleCenter;
    source.m_r2018XAxisDir = DRW_Coord{0.0, 1.0, 0.0};
    source.m_r2018InsertionPoint = source.basePoint;
    source.m_r2018RectWidth = 40.0;
    source.m_r2018RectHeight = 12.0;
    source.m_r2018ExtentsHeight = 8.0;
    source.m_r2018ExtentsWidth = 32.0;
    source.m_r2018ColumnType = 2;
    source.m_r2018ColumnCount = 2;
    source.m_r2018ColumnWidth = 40.0;
    source.m_r2018ColumnGutter = 5.0;
    source.m_r2018ColumnAutoHeight = false;
    source.m_r2018ColumnFlowReversed = true;
    source.m_r2018ColumnHeights = {12.5, 25.0};
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;

    EncodedEntityFrame frame;
    REQUIRE(encodeThreeStreams(source, DRW::AC1032, frame));
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer reader(frame.bytes.data(), frame.bytes.size(), &codec);
    DRW_MText parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1032, &reader, frame.handleBitSize));
    CHECK(parsed.m_r2018ColumnType == 2);
    CHECK(parsed.m_r2018ColumnCount == 2);
    CHECK(parsed.m_r2018Version == source.m_r2018Version);
    CHECK(parsed.m_r2018DefaultFlag == source.m_r2018DefaultFlag);
    CHECK(parsed.m_r2018Attachment == source.m_r2018Attachment);
    CHECK(parsed.m_r2018XAxisDir.x == Approx(source.m_r2018XAxisDir.x));
    CHECK(parsed.m_r2018XAxisDir.y == Approx(source.m_r2018XAxisDir.y));
    CHECK(parsed.m_r2018RectHeight == Approx(source.m_r2018RectHeight));
    CHECK(parsed.m_r2018ColumnWidth == Approx(source.m_r2018ColumnWidth));
    CHECK(parsed.m_r2018ColumnGutter == Approx(source.m_r2018ColumnGutter));
    CHECK_FALSE(parsed.m_r2018ColumnAutoHeight);
    CHECK(parsed.m_r2018ColumnFlowReversed);
    REQUIRE(parsed.m_r2018ColumnHeights.size() == 2u);
    CHECK(parsed.m_r2018ColumnHeights[0]
          == Approx(source.m_r2018ColumnHeights[0]));
    CHECK(parsed.m_r2018ColumnHeights[1]
          == Approx(source.m_r2018ColumnHeights[1]));
    CHECK(reader.isGood());
}

TEST_CASE("DRW_Ray::encodeDwg round-trips base + direction",
          "[dwg-write][entity-encode]") {
    DRW_Ray src;
    src.handle = 0xB0;
    src.color = 7;
    src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    src.secPoint  = DRW_Coord{4.0, 5.0, 6.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Ray dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x == 1.0);
        REQUIRE(dst.basePoint.y == 2.0);
        REQUIRE(dst.basePoint.z == 3.0);
        REQUIRE(dst.secPoint.x  == 4.0);
        REQUIRE(dst.secPoint.y  == 5.0);
        REQUIRE(dst.secPoint.z  == 6.0);
    }
}

TEST_CASE("DRW_3Dface::encodeDwg round-trips four corners + invisibility flags",
          "[dwg-write][entity-encode]") {
    // Case 1: no invisible edges, z=0 (both bit shortcuts on).
    {
        DRW_3Dface src;
        src.handle = 0xD0;
        src.color = 7;
        src.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        src.secPoint  = DRW_Coord{1.0, 0.0, 0.0};
        src.thirdPoint = DRW_Coord{1.0, 1.0, 0.0};
        src.fourPoint  = DRW_Coord{0.0, 1.0, 0.0};
        src.invisibleflag = 0;  // NoEdge
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            dwgBuffer r(w.data().data(), w.data().size());
            DRW_3Dface dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x  == 0.0);
            REQUIRE(dst.secPoint.x   == 1.0);
            REQUIRE(dst.thirdPoint.y == 1.0);
            REQUIRE(dst.fourPoint.y  == 1.0);
            REQUIRE(dst.invisibleflag == 0);
        }
    }
    // Case 2: invisible flag set + non-zero z (both bit shortcuts off).
    {
        DRW_3Dface src;
        src.handle = 0xD1;
        src.basePoint = DRW_Coord{0.0, 0.0, 5.0};
        src.secPoint  = DRW_Coord{2.0, 0.0, 5.0};
        src.thirdPoint = DRW_Coord{2.0, 2.0, 5.0};
        src.fourPoint  = DRW_Coord{0.0, 2.0, 5.0};
        src.invisibleflag = 0xF;  // AllEdges
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            dwgBuffer r(w.data().data(), w.data().size());
            DRW_3Dface dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.z   == 5.0);
            REQUIRE(dst.thirdPoint.z  == 5.0);
            REQUIRE(dst.invisibleflag == 0xF);
        }
    }
}

TEST_CASE("DRW_Trace::encodeDwg round-trips four corners",
          "[dwg-write][entity-encode]") {
    DRW_Trace src;
    src.handle = 0xBA;
    src.color = 4;
    src.basePoint = DRW_Coord{1.0, 2.0, 0.5};  // base.z is the elevation
    src.secPoint  = DRW_Coord{11.0, 2.0, 0.5};
    src.thirdPoint = DRW_Coord{1.0, 12.0, 0.5};
    src.fourPoint = DRW_Coord{11.0, 12.0, 0.5};
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Trace dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x  == 1.0);
        REQUIRE(dst.basePoint.z  == 0.5);
        REQUIRE(dst.secPoint.x   == 11.0);
        REQUIRE(dst.thirdPoint.y == 12.0);
        REQUIRE(dst.fourPoint.x  == 11.0);
        REQUIRE(dst.fourPoint.y  == 12.0);
    }
}

TEST_CASE("DRW_Solid::encodeDwg round-trips four corners",
          "[dwg-write][entity-encode]") {
    DRW_Solid src;
    src.handle = 0xC0;
    src.color = 3;
    src.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint  = DRW_Coord{10.0, 0.0, 0.0};
    src.thirdPoint = DRW_Coord{0.0, 10.0, 0.0};
    src.fourPoint = DRW_Coord{10.0, 10.0, 0.0};
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Solid dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x  == 0.0);
        REQUIRE(dst.secPoint.x   == 10.0);
        REQUIRE(dst.thirdPoint.y == 10.0);
        REQUIRE(dst.fourPoint.x  == 10.0);
        REQUIRE(dst.fourPoint.y  == 10.0);
    }
}

TEST_CASE("DRW_LWPolyline::encodeDwg round-trips closed quad with no bulges",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline src;
    src.handle = 0xA0;
    src.color = 1;
    src.flags = 1;  // closed (DXF bit 0)
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};

    auto v0 = src.addVertex(); v0->x = 0.0;  v0->y = 0.0;
    auto v1 = src.addVertex(); v1->x = 10.0; v1->y = 0.0;
    auto v2 = src.addVertex(); v2->x = 10.0; v2->y = 10.0;
    auto v3 = src.addVertex(); v3->x = 0.0;  v3->y = 10.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_LWPolyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle == 0xA0u);
        REQUIRE(dst.flags  == 1);  // closed bit survives the DWG-flag round-trip
        REQUIRE(dst.vertlist.size() == 4);
        REQUIRE(dst.vertlist[0]->x == 0.0);
        REQUIRE(dst.vertlist[0]->y == 0.0);
        REQUIRE(dst.vertlist[1]->x == 10.0);
        REQUIRE(dst.vertlist[2]->x == 10.0);
        REQUIRE(dst.vertlist[2]->y == 10.0);
        REQUIRE(dst.vertlist[3]->x == 0.0);
        REQUIRE(dst.vertlist[3]->y == 10.0);
    }
}

TEST_CASE("DRW_LWPolyline::encodeDwg round-trips bulges + widths",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline src;
    src.handle = 0xA1;
    src.color = 2;
    src.flags = 0;
    src.extPoint = DRW_Coord{0.0, 0.0, -1.0};
    src.elevation = 7.0;
    src.thickness = 0.25;

    auto v0 = src.addVertex(); v0->x = 0.0; v0->y = 0.0;
        v0->bulge = 0.5;  v0->stawidth = 0.1; v0->endwidth = 0.2;
    auto v1 = src.addVertex(); v1->x = 5.0; v1->y = 0.0;
        v1->bulge = -0.5; v1->stawidth = 0.2; v1->endwidth = 0.1;
    auto v2 = src.addVertex(); v2->x = 5.0; v2->y = 5.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_LWPolyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.vertlist.size() == 3);
        REQUIRE(dst.vertlist[0]->bulge    == 0.5);
        REQUIRE(dst.vertlist[1]->bulge    == -0.5);
        REQUIRE(dst.vertlist[0]->stawidth == 0.1);
        REQUIRE(dst.vertlist[0]->endwidth == 0.2);
        REQUIRE(dst.vertlist[1]->stawidth == 0.2);
        REQUIRE(dst.elevation == 7.0);
        REQUIRE(dst.thickness == 0.25);
        REQUIRE(dst.extPoint.z == -1.0);
    }
}

// D-5: DRW_LWPolyline copy + assignment must carry vertexnum and must NOT leave
// the transient `vertex` build pointer aliasing the source's vertlist (Rule of 3).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LWPolyline copy and assignment carry vertexnum + reset the build pointer",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline a;
    a.flags = 1;
    a.vertexnum = 2;
    auto v0 = a.addVertex();
    a.addVertex();
    a.vertex = v0;  // simulate a transient build pointer into a's vertlist

    // copy-construct
    DRW_LWPolyline b(a);
    CHECK(b.vertexnum == 2);
    REQUIRE(b.vertlist.size() == 2u);
    CHECK(b.vertex == nullptr);                              // not aliased
    CHECK(b.vertlist[0].get() != a.vertlist[0].get());       // deep copy

    // copy-assign
    DRW_LWPolyline c;
    c = a;
    CHECK(c.vertexnum == 2);
    REQUIRE(c.vertlist.size() == 2u);
    CHECK(c.vertex == nullptr);
    CHECK(c.vertlist[0].get() != a.vertlist[0].get());       // deep copy
    c.vertlist[0]->bulge = 99.0;
    CHECK(a.vertlist[0]->bulge != 99.0);                     // mutating c leaves a intact
}

// G-2: copy/assign must also deep-copy the base extData (XDATA), not just
// vertlist — the implicit DRW_Entity base copy aliases the shared_ptr variants,
// which parseAttribs mutates (addString/setLayerRefName).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LWPolyline copy/assign deep-copy base extData (no XDATA aliasing)",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline a;
    a.extData.push_back(std::make_shared<DRW_Variant>(1000, UTF8STRING("orig")));

    DRW_LWPolyline b(a);
    REQUIRE(b.extData.size() == 1u);
    CHECK(b.extData[0].get() != a.extData[0].get());  // distinct shared_ptr (deep copy)

    DRW_LWPolyline c;
    c = a;
    REQUIRE(c.extData.size() == 1u);
    CHECK(c.extData[0].get() != a.extData[0].get());
    // Mutating the copy's XDATA variant must not touch the source's.
    c.extData[0]->addString(1000, UTF8STRING("changed"));
    CHECK(std::string(a.extData[0]->c_str()) == "orig");
}

TEST_CASE("DRW_Ellipse::encodeDwg round-trips center + axis + ratio + params",
          "[dwg-write][entity-encode]") {
    DRW_Ellipse src;
    src.handle = 0x80;
    src.color = 4;
    src.basePoint = DRW_Coord{10.0, 20.0, 0.0};
    src.secPoint  = DRW_Coord{3.0, 0.0, 0.0};  // major axis vector
    src.extPoint  = DRW_Coord{0.0, 0.0, -1.0};
    src.ratio = 0.5;
    src.staparam = 0.2;
    src.endparam = 1.3;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Ellipse dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x80u);
        REQUIRE(dst.basePoint.x == 10.0);
        REQUIRE(dst.basePoint.y == 20.0);
        REQUIRE(dst.secPoint.x  == 3.0);
        REQUIRE(dst.extPoint.z  == -1.0);
        REQUIRE(dst.ratio       == 0.5);
        REQUIRE(dst.staparam    == 0.2);
        REQUIRE(dst.endparam    == 1.3);
    }
}

TEST_CASE("DRW_Attrib::encodeDwg round-trips basic single-line attribute",
          "[dwg-write][entity-encode]") {
    DRW_Attrib src;
    src.handle     = 0xA1;
    src.color      = 256;         // BYLAYER
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{3.0, 7.5, 0.0};
    src.secPoint   = DRW_Coord{3.0, 7.5, 0.0};  // same as base → DD shortcut
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 2.5;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;        // degrees — stored in struct after ARAD multiply
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "HELLO";
    src.tag        = "TAGNAME";
    src.attribFlags = 0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attrib dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle        == 0xA1u);
        REQUIRE(dst.basePoint.x   == 3.0);
        REQUIRE(dst.basePoint.y   == 7.5);
        REQUIRE(dst.basePoint.z   == 0.0);
        REQUIRE(dst.height        == 2.5);
        REQUIRE(dst.widthscale    == 1.0);
        REQUIRE(dst.text          == "HELLO");
        REQUIRE(dst.tag           == "TAGNAME");
        REQUIRE(dst.attribFlags   == 0);
        REQUIRE(dst.styleH.ref    == 0x13u);  // STANDARD textstyle (default)
    }
}

TEST_CASE("DRW_Attrib preserves AC1032 multiline annotation data",
          "[dwg-write][dwg-read][attrib][ac1032]") {
    DRW_Attrib source;
    source.handle = 0xA6;
    source.basePoint = DRW_Coord{3.0, 7.5, 0.0};
    source.secPoint = source.basePoint;
    source.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    source.height = 2.5;
    source.widthscale = 1.0;
    source.text = "VALUE";
    source.tag = "NOTE";
    source.attVersion = 1;
    source.m_attributeType = 2;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;
    source.styleH.ref = 0x13;

    source.mtext = std::make_unique<DRW_MText>();
    source.mtext->parentHandle = DRW::NoHandle;
    source.mtext->basePoint = source.basePoint;
    source.mtext->secPoint = DRW_Coord{1.0, 0.0, 0.0};
    source.mtext->extPoint = DRW_Coord{0.0, 0.0, 1.0};
    source.mtext->widthscale = 1.0;
    source.mtext->height = 2.5;
    source.mtext->interlin = 1.0;
    source.mtext->text = "MULTILINE VALUE";
    source.mtext->m_r2018AnnotativeData = {0x10, 0x20, 0x30};
    source.mtext->m_r2018AnnotativeUnknown = 0x72;
    source.mtext->m_r2018AnnotativeAppHandle = 0x24;
    source.mtext->setDwgLayerHandle(0x12);
    source.mtext->styleH.ref = 0x13;

    EncodedEntityFrame frame;
    REQUIRE(encodeThreeStreams(source, DRW::AC1032, frame));
    REQUIRE(!frame.bytes.empty());

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer reader(frame.bytes.data(), frame.bytes.size(), &codec);
    DRW_Attrib parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1032, &reader, frame.handleBitSize));
    REQUIRE(parsed.mtext != nullptr);
    CHECK(parsed.m_attributeType == 2);
    CHECK(parsed.text == source.text);
    CHECK(parsed.tag == source.tag);
    CHECK(parsed.mtext->text == source.mtext->text);
    CHECK(parsed.mtext->m_r2018AnnotativeData
          == source.mtext->m_r2018AnnotativeData);
    CHECK(parsed.mtext->m_r2018AnnotativeUnknown
          == source.mtext->m_r2018AnnotativeUnknown);
    CHECK(parsed.mtext->m_r2018AnnotativeAppHandle
          == source.mtext->m_r2018AnnotativeAppHandle);
    CHECK(parsed.styleH.ref == source.styleH.ref);
    CHECK(reader.isGood());

    frame.bytes.pop_back();
    dwgBuffer truncated(frame.bytes.data(), frame.bytes.size(), &codec);
    DRW_Attrib rejected;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        rejected, DRW::AC1032, &truncated, frame.handleBitSize));
}

TEST_CASE("DRW_Attdef preserves AC1032 multiline annotation data",
          "[dwg-write][dwg-read][attdef][ac1032]") {
    DRW_Attdef source;
    source.handle = 0xA7;
    source.basePoint = DRW_Coord{1.0, 2.0, 0.0};
    source.secPoint = source.basePoint;
    source.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    source.height = 1.5;
    source.widthscale = 1.0;
    source.text = "DEFAULT";
    source.tag = "PARTNO";
    source.prompt = "Enter part number:";
    source.attVersion = 1;
    source.m_attributeType = 4;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;
    source.styleH.ref = 0x13;

    source.mtext = std::make_unique<DRW_MText>();
    source.mtext->parentHandle = DRW::NoHandle;
    source.mtext->basePoint = source.basePoint;
    source.mtext->secPoint = DRW_Coord{1.0, 0.0, 0.0};
    source.mtext->extPoint = DRW_Coord{0.0, 0.0, 1.0};
    source.mtext->widthscale = 1.0;
    source.mtext->height = 1.5;
    source.mtext->interlin = 1.0;
    source.mtext->text = "DEFAULT MULTILINE";
    source.mtext->m_r2018AnnotativeData = {0xAB, 0xCD};
    source.mtext->m_r2018AnnotativeUnknown = 0x55;
    source.mtext->m_r2018AnnotativeAppHandle = 0x25;
    source.mtext->setDwgLayerHandle(0x12);
    source.mtext->styleH.ref = 0x13;

    EncodedEntityFrame frame;
    REQUIRE(encodeThreeStreams(source, DRW::AC1032, frame));
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer reader(frame.bytes.data(), frame.bytes.size(), &codec);
    DRW_Attdef parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1032, &reader, frame.handleBitSize));
    REQUIRE(parsed.mtext != nullptr);
    CHECK(parsed.m_attributeType == 4);
    CHECK(parsed.tag == source.tag);
    CHECK(parsed.prompt == source.prompt);
    CHECK(parsed.mtext->text == source.mtext->text);
    CHECK(parsed.mtext->m_r2018AnnotativeData
          == source.mtext->m_r2018AnnotativeData);
    CHECK(parsed.mtext->m_r2018AnnotativeUnknown
          == source.mtext->m_r2018AnnotativeUnknown);
    CHECK(parsed.mtext->m_r2018AnnotativeAppHandle
          == source.mtext->m_r2018AnnotativeAppHandle);
    CHECK(parsed.styleH.ref == source.styleH.ref);
    CHECK(reader.isGood());
}

TEST_CASE("DRW_Attdef::encodeDwg round-trips tag + prompt",
          "[dwg-write][entity-encode]") {
    DRW_Attdef src;
    src.handle     = 0xA2;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 1.0;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "DEFAULT";
    src.tag        = "PARTNO";
    src.prompt     = "Enter part number:";
    src.attribFlags = 4;  // verify flag set
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attdef dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle       == 0xA2u);
        REQUIRE(dst.text         == "DEFAULT");
        REQUIRE(dst.tag          == "PARTNO");
        REQUIRE(dst.prompt       == "Enter part number:");
        REQUIRE(dst.attribFlags  == 4);
        REQUIRE(dst.height       == 1.0);
    }
}

TEST_CASE("ATTRIB and ATTDEF encode the AC1021 lock-position bit",
          "[dwg-write][entity-encode][attrib]") {
    DRW_Attrib attrib;
    attrib.handle = 0xA3;
    attrib.color = 256;
    attrib.ltypeScale = 1.0;
    attrib.basePoint = DRW_Coord{1.0, 2.0, 0.0};
    attrib.secPoint = attrib.basePoint;
    attrib.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attrib.height = 1.0;
    attrib.widthscale = 1.0;
    attrib.text = "VALUE";
    attrib.tag = "TAG";
    attrib.lockPosition = true;
    DrwEntityEncodeTestAccess::layerH(attrib).ref = 0x12;

    DRW_Attdef attdef;
    attdef.handle = 0xA4;
    attdef.color = 256;
    attdef.ltypeScale = 1.0;
    attdef.basePoint = DRW_Coord{1.0, 2.0, 0.0};
    attdef.secPoint = attdef.basePoint;
    attdef.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attdef.height = 1.0;
    attdef.widthscale = 1.0;
    attdef.text = "DEFAULT";
    attdef.tag = "TAG";
    attdef.prompt = "Prompt";
    attdef.lockPosition = true;
    DrwEntityEncodeTestAccess::layerH(attdef).ref = 0x12;

    for (DRW_Entity* source : {static_cast<DRW_Entity*>(&attrib),
                               static_cast<DRW_Entity*>(&attdef)}) {
        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        REQUIRE(DrwEntityEncodeTestAccess::encode(
            *source, DRW::AC1021, &body, &strings, &handles));

        // AC1021 stores data, the separate R2007 string stream, and handles
        // in one frame. Mirror dwgWriter21::finishObject so parseDwg can
        // seek to the handle stream using the back-patched object size.
        body.alignToByte();
        const std::size_t stringBytes = strings.data().size();
        body.putBytes(strings.data().data(), stringBytes);
        if (stringBytes != 0) {
            for (int i = 0; i < 7; ++i)
                body.putBit(0);
            const auto stringBitSize = static_cast<std::uint16_t>(
                stringBytes * 8u + 7u);
            body.putRawShort16(stringBitSize);
            body.putBit(1);
        }
        body.alignToByte();
        handles.alignToByte();
        const std::uint32_t dataBitSize =
            static_cast<std::uint32_t>(body.size() * 8u);
        const std::uint8_t bsCode = (body.data()[0] >> 6) & 0x03;
        const std::size_t objectSizeBit =
            (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        body.patchRawLong32AtBit(objectSizeBit, dataBitSize);

        auto bytes = snapshot(body);
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer input(bytes.data(), bytes.size(), &codec);

        if (source == &attrib) {
            DRW_Attrib parsed;
            REQUIRE(DrwEntityEncodeTestAccess::parse(
                parsed, DRW::AC1021, &input));
            CHECK(parsed.lockPosition);
            CHECK(parsed.tag == "TAG");
        } else {
            DRW_Attdef parsed;
            REQUIRE(DrwEntityEncodeTestAccess::parse(
                parsed, DRW::AC1021, &input));
            CHECK(parsed.lockPosition);
            CHECK(parsed.tag == "TAG");
            CHECK(parsed.prompt == "Prompt");
        }
        CHECK(input.isGood());
    }
}

// 2a.2 (gap entity-reactors-xdict-dropped-roundtrip): reactor handles and the
// xdict handle are now persisted on read and re-emitted on write (numReactors
// BL + per-handle stream + real-or-null xdic). Empty case stays byte-stable
// (covered by the existing [entity-encode] round-trips); this proves a
// populated entity round-trips reactors + xdict without a handle-stream desync
// (the layer handle still resolves, proving alignment).
TEST_CASE("DRW entity reactors + xdict round-trip through encode (2a.2)",
          "[dwg-write][entity-encode][phase2a]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        DRW_Point src;
        src.handle = 0x71;
        src.color = 7;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        src.reactorHandles = {0xA0, 0xA1};
        src.xDictHandle = 0xB0;
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.reactorHandles.size() == 2u);
        REQUIRE(dst.reactorHandles[0] == 0xA0u);
        REQUIRE(dst.reactorHandles[1] == 0xA1u);
        REQUIRE(dst.xDictHandle == 0xB0u);
        // Alignment proof: the layer handle still lands correctly after the
        // reactor + xdic handles.
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
        REQUIRE(dst.basePoint.x == 1.0);
    }
}

// Mixed: reactors empty but xdict present → reader still reads the xdic handle
// and the layer follows correctly.
TEST_CASE("DRW entity xdict-only round-trip keeps alignment (2a.2)",
          "[dwg-write][entity-encode][phase2a]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        DRW_Point src;
        src.handle = 0x72;
        src.color = 7;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{4.0, 5.0, 6.0};
        src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        src.xDictHandle = 0xC0;   // reactors empty
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.reactorHandles.empty());
        REQUIRE(dst.xDictHandle == 0xC0u);
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
    }
}

TEST_CASE("DRW entity common references preserve modern handle flags",
          "[dwg-write][entity-encode][common-references]") {
    for (DRW::Version ver : {DRW::AC1021, DRW::AC1024,
                             DRW::AC1027, DRW::AC1032}) {
        DRW_Point src;
        src.handle = 0x90;
        src.color = 7;
        src.reactorHandles = {0xA0, 0xA1};
        src.xDictHandle = 0xB0;
        src.material = 0x44;
        src.plotStyle = 0x55;
        src.shadow = ver > DRW::AC1021
            ? DRW::IgnoreShadows : DRW::ReceiveShadows;
        src.shadowHandle = ver > DRW::AC1021 ? 0x66 : 0;
        src.fullVisualStyleHandle = ver > DRW::AC1021 ? 0x77 : 0;
        src.faceVisualStyleHandle = ver > DRW::AC1021 ? 0x88 : 0;
        src.edgeVisualStyleHandle = ver > DRW::AC1021 ? 0x99 : 0;
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW body;
        dwgBufferW handles;
        REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(src, ver, &body));
        REQUIRE(DrwEntityEncodeTestAccess::encodeHandles(src, ver, &body,
                                                         &handles));

        auto bodyBytes = snapshot(body);
        auto handleBytes = snapshot(handles);
        dwgBuffer bodyReader(bodyBytes.data(), bodyBytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parseCommon(dst, ver,
                                                        &bodyReader));
        dwgBuffer handleReader(handleBytes.data(), handleBytes.size());
        REQUIRE(DrwEntityEncodeTestAccess::parseHandles(dst, ver,
                                                         &handleReader));

        REQUIRE(dst.material == 0x44u);
        REQUIRE(dst.plotStyle == 0x55);
        REQUIRE(dst.shadow == src.shadow);
        REQUIRE(dst.shadowHandle == src.shadowHandle);
        REQUIRE(dst.reactorHandles == src.reactorHandles);
        REQUIRE(dst.xDictHandle == 0xB0u);
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);

        const bool modernVisuals = ver > DRW::AC1021;
        REQUIRE(DrwEntityEncodeTestAccess::materialFlag(dst) == 3u);
        REQUIRE(DrwEntityEncodeTestAccess::plotFlags(dst) == 3u);
        REQUIRE(DrwEntityEncodeTestAccess::fullVisualFlag(dst)
                == static_cast<std::uint8_t>(modernVisuals));
        REQUIRE(DrwEntityEncodeTestAccess::faceVisualFlag(dst)
                == static_cast<std::uint8_t>(modernVisuals));
        REQUIRE(DrwEntityEncodeTestAccess::edgeVisualFlag(dst)
                == static_cast<std::uint8_t>(modernVisuals));
        if (!modernVisuals) {
            REQUIRE(dst.fullVisualStyleHandle == 0u);
            REQUIRE(dst.faceVisualStyleHandle == 0u);
            REQUIRE(dst.edgeVisualStyleHandle == 0u);
        } else {
            REQUIRE(dst.fullVisualStyleHandle == 0x77u);
            REQUIRE(dst.faceVisualStyleHandle == 0x88u);
            REQUIRE(dst.edgeVisualStyleHandle == 0x99u);
        }

        DRW_Point clean;
        clean.handle = 0xA0;
        clean.color = 7;
        DrwEntityEncodeTestAccess::layerH(clean).ref = 0x12;
        dwgBufferW cleanBody;
        dwgBufferW cleanHandles;
        REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(clean, ver,
                                                         &cleanBody));
        REQUIRE(DrwEntityEncodeTestAccess::encodeHandles(clean, ver,
                                                          &cleanBody,
                                                          &cleanHandles));
        auto cleanBodyBytes = snapshot(cleanBody);
        auto cleanHandleBytes = snapshot(cleanHandles);
        dwgBuffer cleanBodyReader(cleanBodyBytes.data(), cleanBodyBytes.size());
        REQUIRE(DrwEntityEncodeTestAccess::parseCommon(dst, ver,
                                                        &cleanBodyReader));
        dwgBuffer cleanHandleReader(cleanHandleBytes.data(),
                                    cleanHandleBytes.size());
        REQUIRE(DrwEntityEncodeTestAccess::parseHandles(dst, ver,
                                                         &cleanHandleReader));
        REQUIRE(dst.material == DRW::MaterialByLayer);
        REQUIRE(dst.plotStyle == DRW::DefaultPlotStyle);
        REQUIRE(dst.shadowHandle == 0u);
        REQUIRE(dst.fullVisualStyleHandle == 0u);
        REQUIRE(dst.faceVisualStyleHandle == 0u);
        REQUIRE(dst.edgeVisualStyleHandle == 0u);
    }
}

TEST_CASE("DRW entity common reference handles reject truncation",
          "[dwg][safety][entity-encode][common-references]") {
    for (DRW::Version ver : {DRW::AC1021, DRW::AC1024,
                             DRW::AC1027, DRW::AC1032}) {
        DRW_Point src;
        src.handle = 0x90;
        src.color = 7;
        src.reactorHandles = {0xA0};
        src.xDictHandle = 0xB0;
        src.material = 0x44;
        src.plotStyle = 0x55;
        src.shadow = DRW::IgnoreShadows;
        src.shadowHandle = 0x66;
        src.fullVisualStyleHandle = 0x77;
        src.faceVisualStyleHandle = 0x88;
        src.edgeVisualStyleHandle = 0x99;
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW body;
        dwgBufferW handles;
        REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(src, ver, &body));
        REQUIRE(DrwEntityEncodeTestAccess::encodeHandles(src, ver, &body,
                                                         &handles));
        auto bodyBytes = snapshot(body);
        auto handleBytes = snapshot(handles);
        REQUIRE(handleBytes.size() > 1u);
        handleBytes.pop_back();

        dwgBuffer bodyReader(bodyBytes.data(), bodyBytes.size());
        DRW_Point dst;
        dst.material = 0xDEAD;
        dst.plotStyle = 0xBEEF;
        dst.shadowHandle = 0xCAFE;
        dst.fullVisualStyleHandle = 0x1111;
        dst.faceVisualStyleHandle = 0x2222;
        dst.edgeVisualStyleHandle = 0x3333;
        REQUIRE(DrwEntityEncodeTestAccess::parseCommon(dst, ver,
                                                        &bodyReader));
        dwgBuffer handleReader(handleBytes.data(), handleBytes.size());
        CHECK_FALSE(DrwEntityEncodeTestAccess::parseHandles(dst, ver,
                                                             &handleReader));
        CHECK(dst.material == 0xDEADu);
        CHECK(dst.plotStyle == 0xBEEF);
        CHECK(dst.shadowHandle == 0xCAFEu);
        CHECK(dst.fullVisualStyleHandle == 0x1111u);
        CHECK(dst.faceVisualStyleHandle == 0x2222u);
        CHECK(dst.edgeVisualStyleHandle == 0x3333u);
    }
}

// 2a.1 (gap entity-visible-code60-dropped-dwg-read): the encoder now emits
// the invisibleFlag BS (DXF 60) from `visible` instead of a hardcoded 0,
// paired with the read assign. A real encode→parse round-trip now preserves
// visible=false; a default visible=true entity stays byte-stable.
TEST_CASE("DRW entity visibility round-trips through encode (2a.1)",
          "[dwg-write][entity-encode][visibility]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        // visible=false survives the real encoder.
        {
            DRW_Point src;
            src.handle = 0x61;
            src.color = 7;
            src.ltypeScale = 1.0;
            src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
            src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
            src.visible = false;
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Point dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE(dst.visible == false);
            REQUIRE(dst.basePoint.x == 1.0);
        }
        // visible=true (default) round-trips visible.
        {
            DRW_Point src;
            src.handle = 0x62;
            src.color = 7;
            src.ltypeScale = 1.0;
            src.basePoint = DRW_Coord{4.0, 5.0, 6.0};
            src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Point dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE(dst.visible == true);
        }
    }
}

// T1 (gap attrib-attdef-lockpos-version-gate-r2007): the lockPosition (DXF
// 280) READ gate was lowered AC1024->AC1021 so R2007/8/9 imports preserve
// it; the ENCODE gate stays AC1024.  This test pins the LOWER boundary: at
// AC1015/AC1018 (below AC1021) the bit is neither written nor read, so a
// source with lockPosition=true round-trips to false with the body still
// aligned (tag/prompt intact).  The AC1021+ POSITIVE path cannot be exercised
// by this single-buffer harness — the encoder emits lockPosition only at
// AC1024, and AC1024+ parse needs the separate handle/string sections + a
// back-patched objSize the harness does not provide (existing entity tests
// likewise cap at AC1018).  AC1021+ positive coverage is deferred to the
// Phase-1 external write->reread validator / a real R2007 fixture.
TEST_CASE("DRW_Attrib lockPosition below AC1021 is not read (gate lower boundary)",
          "[dwg-write][entity-encode]") {
    DRW_Attrib src;
    src.handle     = 0xA1;
    src.color      = 256;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{3.0, 7.5, 0.0};
    src.secPoint   = DRW_Coord{3.0, 7.5, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 2.5;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "HELLO";
    src.tag        = "TAGNAME";
    src.attribFlags = 0;
    src.lockPosition = true;  // set on source; must NOT survive below AC1021
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attrib dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        REQUIRE(dst.lockPosition == false);   // below gate: not written/read
        REQUIRE(dst.tag == "TAGNAME");         // body stayed aligned
    }
}

TEST_CASE("DRW_Attdef lockPosition below AC1021 is not read (gate lower boundary)",
          "[dwg-write][entity-encode]") {
    DRW_Attdef src;
    src.handle     = 0xA2;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 1.0;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "DEFAULT";
    src.tag        = "PARTNO";
    src.prompt     = "Enter part number:";
    src.attribFlags = 4;
    src.lockPosition = true;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attdef dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        REQUIRE(dst.lockPosition == false);
        REQUIRE(dst.prompt == "Enter part number:");
    }
}

// T2 (gap polyline-mesh-density-dropped-read): the POLYLINE_MESH (0x1E) DWG
// path read the smooth-surface M/N density (DXF 73/74) into discarded locals
// and wrote literal 0s.  Now round-tripped via smoothM/smoothN.
TEST_CASE("DRW_Polyline POLYLINE_MESH round-trips smoothM/smoothN density",
          "[dwg-write][entity-encode]") {
    DRW_Polyline src;
    src.handle     = 0xB1;
    src.color      = 7;
    src.ltypeScale = 1.0;
    src.flags      = 16;          // bit 4 → POLYLINE_MESH (oType 0x1E)
    src.curvetype  = 0;
    src.vertexcount = 3;          // M count
    src.facecount   = 4;          // N count
    src.smoothM     = 4;          // DXF 73
    src.smoothN     = 5;          // DXF 74
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Polyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.smoothM == 4);     // was 0 before the fix
        REQUIRE(dst.smoothN == 5);     // was 0 before the fix
        REQUIRE(dst.vertexcount == 3); // unchanged through round-trip
        REQUIRE(dst.facecount == 4);
        REQUIRE(dst.curvetype == 0);
        // reader sets bit 4 (3D mesh); writer strips it, reader re-adds it.
        REQUIRE((dst.flags & 16) == 16);
    }
}

TEST_CASE("DRW_Attrib and DRW_Attdef block unsupported multiline DWG writes",
          "[dwg-write][entity-encode]") {
    DRW_Attrib attrib;
    attrib.handle = 0xA3;
    attrib.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    attrib.secPoint = attrib.basePoint;
    attrib.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attrib.height = 1.0;
    attrib.widthscale = 1.0;
    attrib.text = "MULTILINE";
    attrib.tag = "NOTE";
    attrib.m_attributeType = 2;
    DrwEntityEncodeTestAccess::layerH(attrib).ref = 0x12;
    dwgBufferW attribWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(attrib, DRW::AC1032, &attribWriter));

    DRW_Attdef attdef;
    attdef.handle = 0xA4;
    attdef.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    attdef.secPoint = attdef.basePoint;
    attdef.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attdef.height = 1.0;
    attdef.widthscale = 1.0;
    attdef.text = "DEFAULT";
    attdef.tag = "PARTNO";
    attdef.prompt = "Enter part number:";
    attdef.m_attributeType = 4;
    DrwEntityEncodeTestAccess::layerH(attdef).ref = 0x12;
    dwgBufferW attdefWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(attdef, DRW::AC1032, &attdefWriter));

    DRW_Attrib legacyMTextAttrib;
    legacyMTextAttrib.handle = 0xA5;
    legacyMTextAttrib.attVersion = 1;
    legacyMTextAttrib.m_attributeType = 1;
    DrwEntityEncodeTestAccess::layerH(legacyMTextAttrib).ref = 0x12;
    dwgBufferW legacyWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(legacyMTextAttrib, DRW::AC1024, &legacyWriter));
}

TEST_CASE("DRW_Hatch::encodeDwg round-trips solid fill with polyline boundary",
          "[dwg-write][entity-encode]") {
    // Build a rectangular solid hatch: one polyline loop, 4 vertices.
    DRW_Hatch src;
    src.handle     = 0xB0;
    src.color      = 256;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.name       = "SOLID";
    src.solid      = 1;
    src.associative = 0;
    src.hstyle     = 1;   // outermost
    src.hpattern   = 1;   // predefined
    src.loopsnum   = 1;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    // Polyline boundary loop (type bit 2 = polyline path)
    auto loop = std::make_shared<DRW_HatchLoop>(2);
    auto pline = std::make_shared<DRW_LWPolyline>();
    pline->flags = 1;  // closed
    pline->addVertex(DRW_Vertex2D{0.0, 0.0, 0.0});
    pline->addVertex(DRW_Vertex2D{10.0, 0.0, 0.0});
    pline->addVertex(DRW_Vertex2D{10.0, 5.0, 0.0});
    pline->addVertex(DRW_Vertex2D{0.0, 5.0, 0.0});
    loop->objlist.push_back(pline);
    src.looplist.push_back(loop);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Hatch dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle     == 0xB0u);
        REQUIRE(dst.name       == "SOLID");
        REQUIRE(dst.solid      == 1);
        REQUIRE(dst.associative == 0);
        REQUIRE(dst.hstyle     == 1);
        REQUIRE(dst.hpattern   == 1);
        REQUIRE(dst.looplist.size() == 1);
        // Polyline boundary: one object in objlist
        REQUIRE(dst.looplist[0]->objlist.size() == 1);
        const DRW_LWPolyline* rp = dynamic_cast<DRW_LWPolyline*>(
            dst.looplist[0]->objlist[0].get());
        REQUIRE(rp != nullptr);
        REQUIRE(rp->vertlist.size() == 4);
        REQUIRE(rp->vertlist[0]->x  == 0.0);
        REQUIRE(rp->vertlist[1]->x  == 10.0);
        REQUIRE(rp->vertlist[2]->y  == 5.0);
    }
}

TEST_CASE("DRW_Hatch::encodeDwg round-trips non-solid fill with line-segment boundary",
          "[dwg-write][entity-encode]") {
    DRW_Hatch src;
    src.handle     = 0xB1;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 2.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.name       = "ANSI31";
    src.solid      = 0;
    src.associative = 0;
    src.hstyle     = 0;
    src.hpattern   = 1;
    src.angle      = 45.0;
    src.scale      = 1.0;
    src.doubleflag = 0;
    src.pixelSize  = 0.125;
    src.loopsnum   = 1;
    DRW_Hatch::PatternLine patternLine;
    patternLine.angle = 0.5;
    patternLine.baseX = 1.0;
    patternLine.baseY = 2.0;
    patternLine.offsetX = 3.0;
    patternLine.offsetY = 4.0;
    patternLine.dashList = {5.0, -2.0};
    src.patternLines.push_back(patternLine);
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    // Non-polyline loop with two line segments forming a triangle
    auto loop = std::make_shared<DRW_HatchLoop>(4); // derived boundary
    auto l1 = std::make_shared<DRW_Line>();
    l1->basePoint = DRW_Coord{0.0, 0.0, 0.0};
    l1->secPoint  = DRW_Coord{4.0, 0.0, 0.0};
    auto l2 = std::make_shared<DRW_Line>();
    l2->basePoint = DRW_Coord{4.0, 0.0, 0.0};
    l2->secPoint  = DRW_Coord{0.0, 3.0, 0.0};
    loop->objlist.push_back(l1);
    loop->objlist.push_back(l2);
    src.looplist.push_back(loop);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Hatch dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle  == 0xB1u);
        REQUIRE(dst.name    == "ANSI31");
        REQUIRE(dst.solid   == 0);
        REQUIRE(dst.angle   == 45.0);
        REQUIRE(dst.scale   == 1.0);
        REQUIRE(dst.pixelSize == 0.125);
        REQUIRE(dst.patternLines.size() == 1);
        REQUIRE(dst.patternLines[0].angle == 0.5);
        REQUIRE(dst.patternLines[0].baseX == 1.0);
        REQUIRE(dst.patternLines[0].baseY == 2.0);
        REQUIRE(dst.patternLines[0].offsetX == 3.0);
        REQUIRE(dst.patternLines[0].offsetY == 4.0);
        REQUIRE(dst.patternLines[0].dashList == std::vector<double>{5.0, -2.0});
        REQUIRE(dst.looplist.size() == 1);
        REQUIRE(dst.looplist[0]->objlist.size() == 2);
        // Two line edges
        const DRW_Line* rl1 = dynamic_cast<DRW_Line*>(
            dst.looplist[0]->objlist[0].get());
        REQUIRE(rl1 != nullptr);
        REQUIRE(rl1->basePoint.x == 0.0);
        REQUIRE(rl1->secPoint.x  == 4.0);
        const DRW_Line* rl2 = dynamic_cast<DRW_Line*>(
            dst.looplist[0]->objlist[1].get());
        REQUIRE(rl2 != nullptr);
        REQUIRE(rl2->basePoint.x == 4.0);
        REQUIRE(rl2->secPoint.y  == 3.0);
    }
}

TEST_CASE("DRW_DimAligned::encodeDwg round-trips definition points",
          "[dwg-write][entity-encode]") {
    DRW_DimAligned src;
    src.handle = 0xC1;
    src.type   = 1;      // bit0 set — the aligned subtype flag added by parseDwg
    src.setDef1Point({1.0, 2.0, 0.0});
    src.setDef2Point({5.0, 2.0, 0.0});
    src.setDimPoint ({3.0, 4.0, 0.0});
    src.setTextPoint({3.0, 3.5, 0.0});
    src.setHDir(0.0);    // hdir not set by default ctor — must be explicit

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAligned dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 22u);
        REQUIRE(dst.type  == 1);
        REQUIRE(dst.getDef1Point().x == 1.0);
        REQUIRE(dst.getDef1Point().y == 2.0);
        REQUIRE(dst.getDef2Point().x == 5.0);
        REQUIRE(dst.getDimPoint().x  == 3.0);
        REQUIRE(dst.getDimPoint().y  == 4.0);
    }
}

TEST_CASE("DRW_DimLinear::encodeDwg round-trips rotation angle and oblique",
          "[dwg-write][entity-encode]") {
    DRW_DimLinear src;
    src.handle = 0xC2;
    src.type   = 0;      // no subtype flags for linear
    src.setDef1Point({0.0, 0.0, 0.0});
    src.setDef2Point({6.0, 0.0, 0.0});
    src.setDimPoint ({3.0, 2.0, 0.0});
    src.setAngle(45.0);  // 45 degrees rotation
    src.setOblique(10.0);
    src.setTextPoint({3.0, 2.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimLinear dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 21u);
        REQUIRE(dst.type  == 0);
        REQUIRE(dst.getDef1Point().x == 0.0);
        REQUIRE(dst.getDef2Point().x == 6.0);
        REQUIRE(dst.getDimPoint().x  == 3.0);
        REQUIRE(dst.getAngle()       == Approx(45.0).margin(1e-9));
        // oblique round-trips degrees → radians on disk → degrees on read
        REQUIRE(dst.getOblique()     == Approx(10.0).margin(1e-9));
    }
}

TEST_CASE("DRW_DimRadial::encodeDwg round-trips center, radius point, leader length",
          "[dwg-write][entity-encode]") {
    DRW_DimRadial src;
    src.handle = 0xC3;
    src.type   = 4;      // bit2 — radial subtype flag
    src.setCenterPoint({2.0, 3.0, 0.0});
    src.setDiameterPoint({5.0, 3.0, 0.0});
    src.setLeaderLength(1.5);
    src.setTextPoint({3.5, 4.0, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimRadial dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 25u);
        REQUIRE(dst.type  == 4);
        REQUIRE(dst.getCenterPoint().x   == 2.0);
        REQUIRE(dst.getCenterPoint().y   == 3.0);
        REQUIRE(dst.getDiameterPoint().x == 5.0);
        REQUIRE(dst.getLeaderLength()    == 1.5);
    }
}

TEST_CASE("DRW_DimAngular::encodeDwg round-trips arc point and definition lines",
          "[dwg-write][entity-encode]") {
    DRW_DimAngular src;
    src.handle = 0xC4;
    src.type   = 2;      // bit1 — angular subtype flag
    // arcPoint (code 16) is only 2D — use setDimPoint (public wrapper for setPt6)
    src.setDimPoint({7.0, 4.0, 0.0});
    src.setFirstLine1({1.0, 0.0, 0.0});
    src.setFirstLine2({4.0, 0.0, 0.0});
    src.setSecondLine1({4.0, 3.0, 0.0});
    src.setSecondLine2({0.0, 3.0, 0.0});
    src.setTextPoint({2.0, 1.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAngular dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 24u);
        REQUIRE(dst.type  == 2);
        REQUIRE(dst.getDimPoint().x    == 7.0);
        REQUIRE(dst.getDimPoint().y    == 4.0);
        REQUIRE(dst.getFirstLine1().x  == 1.0);
        REQUIRE(dst.getFirstLine2().x  == 4.0);
        REQUIRE(dst.getSecondLine1().x == 4.0);
        REQUIRE(dst.getSecondLine1().y == 3.0);
    }
}

TEST_CASE("DRW_DimAngular3p::encodeDwg round-trips vertex and line definition points",
          "[dwg-write][entity-encode]") {
    DRW_DimAngular3p src;
    src.handle = 0xC5;
    src.type   = 5;      // bits 0+2 — angular3p subtype flags
    src.setDimPoint ({5.0, 5.0, 0.0});  // vertex / defPoint (code 10)
    src.setFirstLine ({1.0, 0.0, 0.0}); // pt3: first line endpoint
    src.setSecondLine({6.0, 0.0, 0.0}); // pt4: second line endpoint
    src.SetVertexPoint({3.0, 3.0, 0.0}); // pt5: circlePoint / vertex
    src.setTextPoint({4.0, 4.0, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAngular3p dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 23u);
        REQUIRE(dst.type  == 5);
        REQUIRE(dst.getDimPoint().x  == 5.0);
        REQUIRE(dst.getDimPoint().y  == 5.0);
        REQUIRE(dst.getFirstLine().x  == 1.0);
        REQUIRE(dst.getSecondLine().x == 6.0);
        REQUIRE(dst.getVertexPoint().x == 3.0);
        REQUIRE(dst.getVertexPoint().y == 3.0);
    }
}

TEST_CASE("DRW_DimOrdinate::encodeDwg round-trips origin and leader endpoints",
          "[dwg-write][entity-encode]") {
    DRW_DimOrdinate src;
    src.handle = 0xC6;
    src.type   = 6;      // bits 1+2 — ordinate subtype flags
    src.setOriginPoint({2.0, 1.0, 0.0}); // defPoint (code 10)
    src.setFirstLine  ({2.0, 5.0, 0.0}); // pt3: feature location
    src.setSecondLine ({4.0, 5.0, 0.0}); // pt4: leader end
    src.setTextPoint  ({3.0, 5.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimOrdinate dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 20u);
        REQUIRE(dst.type  == 6);
        REQUIRE(dst.getOriginPoint().x == 2.0);
        REQUIRE(dst.getOriginPoint().y == 1.0);
        REQUIRE(dst.getFirstLine().x   == 2.0);
        REQUIRE(dst.getFirstLine().y   == 5.0);
        REQUIRE(dst.getSecondLine().x  == 4.0);
    }
}

// 0B.1 (gap dim-ordinate-xy-flag-wrong-bit): the DWG ordinate X/Y type flag
// is DXF group-70 bit 6 (0x40) — the bit the filter (`type & 64`) and the
// DXF parseCode path use.  The DWG parse/encode previously used bit 7 (0x80),
// so the byte round-tripped but the filter check never fired.  Note: a
// DWG<->DWG byte round-trip alone is INSUFFICIENT to catch this bug (the old
// 0x80 code round-trips its own byte fine); the load-bearing assertion is
// that the surviving bit is 0x40 so the filter sees the X-type.
TEST_CASE("DRW_DimOrdinate X/Y flag round-trips on bit 0x40 (filter parity)",
          "[dwg-write][entity-encode]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        // X-type ordinate: group-70 bit 6 (0x40) set.
        {
            DRW_DimOrdinate src;
            src.handle = 0xC7;
            src.type   = 6 | 0x40;   // ordinate subtype bits + X-type flag
            src.setOriginPoint({2.0, 1.0, 0.0});
            src.setFirstLine  ({2.0, 5.0, 0.0});
            src.setSecondLine ({4.0, 5.0, 0.0});
            src.setTextPoint  ({3.0, 5.5, 0.0});
            src.setHDir(0.0);

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_DimOrdinate dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE((dst.type & 0x40) != 0);   // X-type survives → filter fires
            REQUIRE((dst.type & 0x80) == 0);   // base type bit not corrupted
        }
        // Y-type ordinate: group-70 bit 6 (0x40) clear.
        {
            DRW_DimOrdinate src;
            src.handle = 0xC8;
            src.type   = 6;          // no 0x40 → Y-type
            src.setOriginPoint({2.0, 1.0, 0.0});
            src.setFirstLine  ({2.0, 5.0, 0.0});
            src.setSecondLine ({4.0, 5.0, 0.0});
            src.setTextPoint  ({3.0, 5.5, 0.0});
            src.setHDir(0.0);

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_DimOrdinate dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE((dst.type & 0x40) == 0);   // Y-type stays Y
        }
    }
}

TEST_CASE("DRW_Leader::encodeDwg round-trips vertices, arrow, leadertype, extrusion",
          "[dwg-write][entity-encode][leader]") {
    DRW_Leader src;
    src.handle      = 0xD0;
    src.style       = "Standard";
    src.arrow       = 1;
    src.leadertype  = 1;            // spline path — verifies P1 pathType fix
    src.flag        = 3;
    src.hookline    = 1;
    src.hookflag    = 1;
    src.textheight  = 2.5;
    src.textwidth   = 10.0;
    src.origin         = DRW_Coord{2.0, 3.0, 0.0};
    src.extrusionPoint = DRW_Coord{0.0, 0.0, 1.0};   // default Z
    src.horizdir       = DRW_Coord{1.0, 0.0, 0.0};   // ezdxf default X
    src.offsetblock    = DRW_Coord{0.0, 0.0, 0.0};
    src.offsettext     = DRW_Coord{0.0, -0.09, 0.0};
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(5.0, 3.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(10.0, 3.0, 0.0));
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Leader dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 45u);
        REQUIRE(dst.arrow      == 1);
        REQUIRE(dst.leadertype == 1);                       // P1
        REQUIRE(dst.textheight == Approx(2.5));             // pre-2010 branch
        REQUIRE(dst.textwidth  == Approx(10.0));
        REQUIRE(dst.origin.x   == Approx(2.0));
        REQUIRE(dst.vertexlist.size() == 3u);
        REQUIRE(dst.vertexlist[0]->x == Approx(0.0));
        REQUIRE(dst.vertexlist[1]->x == Approx(5.0));
        REQUIRE(dst.vertexlist[1]->y == Approx(3.0));
        REQUIRE(dst.vertexlist[2]->x == Approx(10.0));
        REQUIRE(dst.vertexlist[2]->y == Approx(3.0));
        REQUIRE(dst.extrusionPoint.z == Approx(1.0));       // P0 wire alignment
        REQUIRE(dst.horizdir.x       == Approx(1.0));
        REQUIRE(dst.offsettext.y     == Approx(-0.09));
    }

    for (DRW::Version modernVersion : {DRW::AC1024, DRW::AC1027,
                                       DRW::AC1032}) {
        EncodedEntityFrame frame;
        REQUIRE(encodeThreeStreams(src, modernVersion, frame));
        DRW_TextCodec codec;
        codec.setVersion(modernVersion, false);
        dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
        DRW_Leader modern;
        REQUIRE(DrwEntityEncodeTestAccess::parse(
            modern, modernVersion, &input, frame.handleBitSize));
        CHECK(modern.origin.x == Approx(src.origin.x));
        CHECK(modern.offsettext.x == Approx(src.offsettext.x));
        CHECK(modern.leadertype == src.leadertype);
        CHECK(modern.vertexlist.size() == src.vertexlist.size());
        CHECK(input.isGood());
    }
}

TEST_CASE("DRW_Leader rejects invalid vertex declarations",
          "[dwg-write][entity-encode][leader][safety]") {
    DRW_Leader leader;
    leader.vertexlist.push_back(nullptr);
    dwgBufferW nullVertex;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        leader, DRW::AC1018, &nullVertex));
    CHECK(nullVertex.data().empty());

    leader.vertexlist.clear();
    leader.leadertype = 2;
    dwgBufferW invalidPath;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        leader, DRW::AC1018, &invalidPath));
    CHECK(invalidPath.data().empty());
}

TEST_CASE("DRW_Surface rejects invalid payloads before writing the common body",
          "[dwg-write][entity-encode][surface][safety]") {
    DRW_LoftedSurface invalidModeler;
    invalidModeler.setDwgClassNum(DRW_LoftedSurface::kDwgClassNum);
    invalidModeler.modelerFormatVersion = 4;
    dwgBufferW modelerBody;
    modelerBody.putRawChar8(0xA5);
    const auto modelerBytes = snapshot(modelerBody);
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalidModeler, DRW::AC1021, &modelerBody));
    CHECK(modelerBody.data() == modelerBytes);

    DRW_ExtrudedSurface invalidMatrix;
    invalidMatrix.setDwgClassNum(DRW_ExtrudedSurface::kDwgClassNum);
    invalidMatrix.extrudedTransform[0] = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW matrixBody;
    matrixBody.putRawChar8(0x5A);
    const auto matrixBytes = snapshot(matrixBody);
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalidMatrix, DRW::AC1021, &matrixBody));
    CHECK(matrixBody.data() == matrixBytes);

    DRW_PlaneSurface invalidRaw;
    invalidRaw.setDwgClassNum(DRW_PlaneSurface::kDwgClassNum);
    invalidRaw.hasRawDwgBody = true;
    invalidRaw.rawDwgBodyVersion = DRW::AC1021;
    invalidRaw.rawDwgBodyBitSize = 9;
    invalidRaw.rawAcisData = {0x80};
    dwgBufferW rawBody;
    rawBody.putRawChar8(0x3C);
    const auto rawBytes = snapshot(rawBody);
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalidRaw, DRW::AC1021, &rawBody));
    CHECK(rawBody.data() == rawBytes);
}

TEST_CASE("DRW_Viewport preserves AC1024 body fields and handles",
          "[dwg-write][entity-encode][viewport]") {
    DRW_Viewport source;
    source.handle = 0xD1;
    source.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    source.pswidth = 40.0;
    source.psheight = 30.0;
    source.viewTarget = DRW_Coord{4.0, 5.0, 6.0};
    source.viewDir = DRW_Coord{0.0, 1.0, 0.0};
    source.twistAngle = 0.25;
    source.viewHeight = 20.0;
    source.viewLength = 75.0;
    source.frontClip = 0.5;
    source.backClip = 100.0;
    source.snapAngle = 0.75;
    source.centerPX = 7.0;
    source.centerPY = 8.0;
    source.snapPX = 9.0;
    source.snapPY = 10.0;
    source.snapSpPX = 11.0;
    source.snapSpPY = 12.0;
    source.gridSpX = 13.0;
    source.gridSpY = 14.0;
    source.circleZoom = 80.0;
    source.majorGridLines = 5;
    source.statusFlags = 0x23;
    source.styleSheet = "viewport.ctb";
    source.renderMode = 2;
    source.ucsAtOrigin = true;
    source.ucsPerViewport = true;
    source.ucsOrigin = DRW_Coord{15.0, 16.0, 17.0};
    source.ucsXAxis = DRW_Coord{0.0, 1.0, 0.0};
    source.ucsYAxis = DRW_Coord{-1.0, 0.0, 0.0};
    source.ucsElevation = 18.0;
    source.ucsOrthographicType = 3;
    source.shadePlotMode = 4;
    source.useDefaultLighting = false;
    source.defaultLightingType = 2;
    source.brightness = 0.4;
    source.contrast = 0.8;
    source.ambientColor = 7;
    source.ambientColorRgb = 0x123456;
    source.ambientColorName = "AmbientName";
    source.frozenLayerHandles = {0x21, 0x22};
    // AC1024+ has no VIEWPORT ENT HEADER handle slot.
    source.vpHeaderHandle = 0;
    source.clipBoundaryHandle = 0x24;
    source.namedUcsHandle = 0x25;
    source.baseUcsHandle = 0x26;
    source.backgroundHandle = 0x27;
    source.visualStyleHandle = 0x28;
    source.shadePlotHandle = 0x29;
    source.m_sunHandle = 0x2A;
    DrwEntityEncodeTestAccess::layerH(source).ref = 0x12;

    EncodedEntityFrame frame;
    REQUIRE(encodeThreeStreams(source, DRW::AC1024, frame));
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    DRW_Viewport parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1024, &input, frame.handleBitSize));

    CHECK(parsed.basePoint.x == Approx(source.basePoint.x));
    CHECK(parsed.basePoint.z == Approx(source.basePoint.z));
    CHECK(parsed.viewTarget.y == Approx(source.viewTarget.y));
    CHECK(parsed.viewDir.y == Approx(source.viewDir.y));
    CHECK(parsed.viewHeight == Approx(source.viewHeight));
    CHECK(parsed.viewLength == Approx(source.viewLength));
    CHECK(parsed.centerPX == Approx(source.centerPX));
    CHECK(parsed.snapSpPY == Approx(source.snapSpPY));
    CHECK(parsed.gridSpX == Approx(source.gridSpX));
    CHECK(parsed.circleZoom == Approx(source.circleZoom));
    CHECK(parsed.majorGridLines == source.majorGridLines);
    CHECK(parsed.statusFlags == source.statusFlags);
    CHECK(parsed.styleSheet == source.styleSheet);
    CHECK(parsed.renderMode == source.renderMode);
    CHECK(parsed.ucsAtOrigin == source.ucsAtOrigin);
    CHECK(parsed.ucsPerViewport == source.ucsPerViewport);
    CHECK(parsed.ucsXAxis.y == Approx(source.ucsXAxis.y));
    CHECK(parsed.ucsYAxis.x == Approx(source.ucsYAxis.x));
    CHECK(parsed.ucsOrthographicType == source.ucsOrthographicType);
    CHECK(parsed.shadePlotMode == source.shadePlotMode);
    CHECK(parsed.useDefaultLighting == source.useDefaultLighting);
    CHECK(parsed.defaultLightingType == source.defaultLightingType);
    CHECK(parsed.brightness == Approx(source.brightness));
    CHECK(parsed.contrast == Approx(source.contrast));
    CHECK(parsed.ambientColor == 256u);
    CHECK(parsed.ambientColorRgb == source.ambientColorRgb);
    CHECK(parsed.ambientColorName == source.ambientColorName);
    CHECK(parsed.frozenLayerHandles == source.frozenLayerHandles);
    CHECK(parsed.vpHeaderHandle == source.vpHeaderHandle);
    CHECK(parsed.clipBoundaryHandle == source.clipBoundaryHandle);
    CHECK(parsed.namedUcsHandle == source.namedUcsHandle);
    CHECK(parsed.baseUcsHandle == source.baseUcsHandle);
    CHECK(parsed.backgroundHandle == source.backgroundHandle);
    CHECK(parsed.visualStyleHandle == source.visualStyleHandle);
    CHECK(parsed.shadePlotHandle == source.shadePlotHandle);
    CHECK(parsed.m_sunHandle == source.m_sunHandle);
    CHECK(input.isGood());
}

TEST_CASE("DRW_Viewport rejects invalid payloads before writing",
          "[dwg-write][entity-encode][viewport][safety]") {
    DRW_Viewport invalidRenderMode;
    invalidRenderMode.renderMode = 256;
    dwgBufferW renderBody;
    renderBody.putRawChar8(0xA5);
    const auto renderBytes = snapshot(renderBody);
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalidRenderMode, DRW::AC1024, &renderBody));
    CHECK(renderBody.data() == renderBytes);

    DRW_Viewport invalidCoordinate;
    invalidCoordinate.basePoint.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW coordinateBody;
    coordinateBody.putRawChar8(0x5A);
    const auto coordinateBytes = snapshot(coordinateBody);
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        invalidCoordinate, DRW::AC1024, &coordinateBody));
    CHECK(coordinateBody.data() == coordinateBytes);
}

TEST_CASE("DRW_Leader::encodeDwg straight-leader with arrow=0 round-trips",
          "[dwg-write][entity-encode][leader]") {
    DRW_Leader src;
    src.handle      = 0xD1;
    src.style       = "Standard";
    src.arrow       = 0;
    src.leadertype  = 0;            // straight segments
    src.textheight  = 1.0;
    src.textwidth   = 1.0;
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Leader dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.arrow      == 0);
        REQUIRE(dst.leadertype == 0);
        REQUIRE(dst.vertexlist.size() == 2u);
    }
}

// Phase 6.1 — SHAPE encoder (fixed oType 33). Round-trips the 8 body fields +
// the trailing SHAPEFILE style hard handle.
TEST_CASE("DRW_Shape::encodeDwg round-trips body and style handle",
          "[dwg-write][entity-encode][shape]") {
    DRW_Shape src;
    src.handle = 0x40;
    src.color = 7;
    src.ltypeScale = 1.0;
    src.m_insertionPoint = DRW_Coord{3.0, 4.0, 5.0};
    src.m_scale = 2.5;
    src.m_rotation = 0.75;
    src.m_widthFactor = 1.25;
    src.m_oblique = 0.1;
    src.m_thickness = 0.5;
    src.m_shapeIndex = 17;
    src.m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
    src.m_shapeFileHandle = 0x41;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);
        // Real DWG objects are followed by a 2-byte CRC; the SHAPE parser
        // gates the trailing style handle on numRemainingBytes() > 2 (i.e.
        // beyond that CRC). Append 2 padding bytes so the handle is read.
        bytes.push_back(0);
        bytes.push_back(0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Shape dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 33);
        CHECK(dst.handle == 0x40u);
        CHECK(dst.m_insertionPoint.x == Approx(3.0));
        CHECK(dst.m_insertionPoint.y == Approx(4.0));
        CHECK(dst.m_insertionPoint.z == Approx(5.0));
        CHECK(dst.m_scale == Approx(2.5));
        CHECK(dst.m_rotation == Approx(0.75));
        CHECK(dst.m_widthFactor == Approx(1.25));
        CHECK(dst.m_oblique == Approx(0.1));
        CHECK(dst.m_thickness == Approx(0.5));
        CHECK(dst.m_shapeIndex == 17);
        CHECK(dst.m_extrusion.z == Approx(1.0));
        CHECK(dst.m_shapeFileHandle == 0x41u);
    }
}

// B-6 (item 3): a directly-CONSTRUCTED OLE2FRAME (m_hasR2000TrailingByte stays
// false) must still emit the R2000+ Unknown RC byte the parser reads
// unconditionally before the handle stream. Omitting it made the parser consume
// the first handle byte as the RC and shift the entity handle stream, corrupting
// the layer handle. The distinctive layer must survive the round-trip.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Ole2Frame::encodeDwg round-trips a constructed entity without handle desync",
          "[dwg-write][entity-encode]") {
    DRW_Ole2Frame src;
    src.handle      = 0x90;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.m_flags     = 2;
    src.m_mode      = 1;
    src.m_payloadBytes = {0x01, 0x02, 0x03, 0x04};
    // m_hasR2000TrailingByte deliberately left false (constructed, not parsed).
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x2A;   // distinctive layer

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Ole2Frame dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        // Layer handle survives only if the handle stream is aligned — a missing
        // trailing RC shifts it by one byte (the discriminating check).
        CHECK(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x2Au);
        REQUIRE(dst.m_payloadBytes.size() == 4u);
        CHECK(dst.m_payloadBytes[0] == 0x01);
        CHECK(dst.m_payloadBytes[3] == 0x04);
        CHECK(dst.m_flags == 2);
    }
}

TEST_CASE("DRW_OleFrame::encodeDwg preserves the bounded opaque payload",
          "[dwg-write][entity-encode][oleframe]") {
    DRW_OleFrame src;
    src.handle = 0x91;
    src.color = 7;
    src.ltypeScale = 1.0;
    src.m_flags = 2;
    src.m_mode = 1;
    src.m_payloadPresent = true;
    src.m_declaredPayloadLength = 4;
    src.m_payloadBytes = {0xDE, 0xAD, 0xBE, 0xEF};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x2A;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_OleFrame dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        CHECK(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x2Au);
        CHECK(dst.m_flags == 2u);
        CHECK(dst.m_mode == (ver > DRW::AC1014 ? 1u : 0u));
        CHECK(dst.m_declaredPayloadLength == 4u);
        CHECK(dst.m_payloadBytes ==
              std::vector<std::uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});
    }
}

TEST_CASE("SHAPE and OLE2FRAME writers reject incomplete typed state",
          "[dwg-write][entity-encode][shape][ole2frame][safety]") {
    DRW_Shape shape;
    shape.handle = 0x401;
    shape.m_insertionPoint = DRW_Coord{0.0, 0.0, 0.0};
    shape.m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
    DrwEntityEncodeTestAccess::layerH(shape).ref = 0x12;
    {
        dwgBufferW w;
        CHECK_FALSE(DrwEntityEncodeTestAccess::encode(shape, DRW::AC1015, &w));
        CHECK(w.data().empty());
    }

    DRW_Ole2Frame frame;
    frame.handle = 0x402;
    frame.m_payloadPresent = true;
    frame.m_declaredPayloadLength = 2;
    frame.m_payloadBytes = {0x01};
    DrwEntityEncodeTestAccess::layerH(frame).ref = 0x12;
    {
        dwgBufferW w;
        CHECK_FALSE(DrwEntityEncodeTestAccess::encode(frame, DRW::AC1015, &w));
        CHECK(w.data().empty());
    }
}

// IMAGE encoder round-trip (Phase 6.3): DRW_Image::encodeDwg must shadow
// DRW_Line::encodeDwg (oType 101, not 19/LINE), round-trip the body fields,
// and preserve BOTH trailing handles (imagedef + imagedefreactor) plus the
// display-props field that the reader previously discarded.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Image::encodeDwg round-trips body + both handles",
          "[dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle      = 0x70;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.basePoint   = DRW_Coord{1.0, 2.0, 3.0};
    src.secPoint    = DRW_Coord{0.5, 0.0, 0.0};   // uvec
    src.vVector     = DRW_Coord{0.0, 0.5, 0.0};
    src.m_classVersion = 4;
    src.sizeu       = 640.0;
    src.sizev       = 480.0;
    src.m_displayProps = 5;
    src.clip        = 1;
    src.brightness  = 60;
    src.contrast    = 40;
    src.fade        = 10;
    src.ref         = 0x100;                       // imagedef handle (340)
    src.m_imageDefReactorHandle = 0x101;           // imagedefreactor (360)
    // Non-empty polygon clip boundary (forces clip_boundary_type 2).
    src.clipPath = {DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{10.0, 0.0, 0.0},
                    DRW_Coord{10.0, 8.0, 0.0}, DRW_Coord{0.0, 8.0, 0.0}};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        // Proves the override shadows DRW_Line::encodeDwg (oType 19).
        CHECK(DrwEntityEncodeTestAccess::oType(src) == 101);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Image dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 101);
        CHECK(dst.handle == 0x70u);
        CHECK(dst.basePoint.x == Approx(1.0));
        CHECK(dst.basePoint.y == Approx(2.0));
        CHECK(dst.basePoint.z == Approx(3.0));
        CHECK(dst.secPoint.x  == Approx(0.5));
        CHECK(dst.vVector.y   == Approx(0.5));
        CHECK(dst.sizeu       == Approx(640.0));
        CHECK(dst.sizev       == Approx(480.0));
        CHECK(dst.m_displayProps == 5);
        CHECK(dst.m_classVersion == 4);
        CHECK(dst.clip        == 1);
        CHECK(dst.brightness  == 60);
        CHECK(dst.contrast    == 40);
        CHECK(dst.fade        == 10);
        CHECK(dst.clipPath.size() == 4u);
        CHECK(dst.clipPath[2].x == Approx(10.0));
        CHECK(dst.clipPath[2].y == Approx(8.0));
        CHECK(dst.ref == 0x100u);                       // imagedef survives
        CHECK(dst.m_imageDefReactorHandle == 0x101u);   // reactor survives
    }
}

TEST_CASE("DRW_Image rejects out-of-range class versions",
          "[dwg-read][dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle = 0x72;
    src.m_classVersion = DRW_Image::kMaxClassVersion + 1;

    dwgBufferW writer;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1018, &writer));

    DRW_Image valid;
    valid.handle = 0x73;
    valid.m_classVersion = 0;
    dwgBufferW malformed;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(valid, DRW::AC1018,
                                                    &malformed));
    malformed.putBitLong(DRW_Image::kMaxClassVersion + 1);
    auto bytes = snapshot(malformed);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Image parsed;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(parsed, DRW::AC1018, &reader));
}

TEST_CASE("DRW_Image and WIPEOUT reject non-finite payload fields",
          "[dwg-write][entity-encode][image][wipeout][safety]") {
    DRW_Image image;
    image.basePoint.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW imageWriter;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        image, DRW::AC1018, &imageWriter));
    CHECK(imageWriter.data().empty());

    DRW_Wipeout wipeout;
    wipeout.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    wipeout.clipPath = {DRW_Coord{0.0, 0.0, 0.0},
                        DRW_Coord{1.0, 0.0, 0.0},
                        DRW_Coord{0.0, 1.0, 0.0}};
    wipeout.m_clipBoundaryType = 2;
    wipeout.sizeu = std::numeric_limits<double>::infinity();
    dwgBufferW wipeoutWriter;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(
        wipeout, DRW::AC1018, &wipeoutWriter));
    CHECK(wipeoutWriter.data().empty());
}

// B-4: a DXF-sourced clip path can exceed the 100000-vertex cap that
// DRW_Image::parseDwg enforces on clipType==2 (the DXF parseCode path has no
// such bound). DWG writing must reject the entity rather than silently discard
// a vertex to make an otherwise malformed record appear valid.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Image::encodeDwg rejects an oversized clip path",
          "[dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle      = 0x71;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.basePoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint    = DRW_Coord{1.0, 0.0, 0.0};
    src.vVector     = DRW_Coord{0.0, 1.0, 0.0};
    src.sizeu       = 1.0;
    src.sizev       = 1.0;
    src.m_displayProps = 1;
    src.clip        = 1;
    src.ref         = 0x100;
    src.m_imageDefReactorHandle = 0x101;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;
    // 100001 vertices — one over the parser's cap.
    src.clipPath.reserve(100001);
    for (int i = 0; i < 100001; ++i)
        src.clipPath.emplace_back(static_cast<double>(i), 0.0, 0.0);

    dwgBufferW w;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1018, &w));
}

TEST_CASE("DRW_Image::parseDwg rejects a clip count outside the body",
          "[dwg-read][entity-encode][image][safety]") {
    DRW_Image image;
    image.handle = 0x73;
    DrwEntityEncodeTestAccess::setOType(image, 101);

    dwgBufferW body;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        image, DRW::AC1018, &body));
    body.putBitLong(0); // class version
    body.put3BitDouble(DRW_Coord{}); // insertion point
    body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0}); // U vector
    body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0}); // V vector
    body.putRawDouble(1.0); // image size U
    body.putRawDouble(1.0); // image size V
    body.putBitShort(0); // display properties
    body.putBit(0); // clipping enabled
    body.putRawChar8(50); // brightness
    body.putRawChar8(50); // contrast
    body.putRawChar8(0); // fade
    body.putBitShort(2); // polygon clip boundary
    body.putBitLong(static_cast<std::int32_t>(DRW_Image::kMaxClipVertices));

    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t objectSizeBit =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
    REQUIRE(DrwEntityEncodeTestAccess::encodeHandles(
        image, DRW::AC1018, &body, nullptr));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Image parsed;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK(parsed.clipPath.empty());
}

TEST_CASE("DRW_Underlay::parseDwg rejects a clip count outside the body",
          "[dwg-read][entity-encode][underlay][safety]") {
    DRW_Underlay underlay;
    underlay.handle = 0x74;
    DrwEntityEncodeTestAccess::setOType(
        underlay, DRW_Underlay::kDwgClassNumPdf);

    dwgBufferW body;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        underlay, DRW::AC1018, &body));
    body.putExtrusion(DRW_Coord{0.0, 0.0, 1.0}, false);
    body.put3BitDouble(DRW_Coord{}); // insertion point
    body.putBitDouble(0.0); // rotation
    body.putBitDouble(1.0); // X scale
    body.putBitDouble(1.0); // Y scale
    body.putBitDouble(1.0); // Z scale
    body.putRawChar8(2); // visible
    body.putRawChar8(100); // contrast
    body.putRawChar8(0); // fade
    body.putBitLong(static_cast<std::int32_t>(
        DRW_Underlay::kMaxClipVertices));

    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t objectSizeBit =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.patchRawLong32AtBit(objectSizeBit, body.bitCount());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Underlay parsed;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK(parsed.clipBoundary.empty());

    DRW_Underlay modern;
    modern.handle = 0x75;
    DrwEntityEncodeTestAccess::setOType(
        modern, DRW_Underlay::kDwgClassNumPdf);
    dwgBufferW modernBody;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        modern, DRW::AC1027, &modernBody));
    modernBody.putExtrusion(DRW_Coord{0.0, 0.0, 1.0}, false);
    modernBody.put3BitDouble(DRW_Coord{}); // insertion point
    modernBody.putBitDouble(0.0); // rotation
    modernBody.putBitDouble(1.0); // X scale
    modernBody.putBitDouble(1.0); // Y scale
    modernBody.putBitDouble(1.0); // Z scale
    modernBody.putRawChar8(0x10); // inverse clipping present
    modernBody.putRawChar8(100); // contrast
    modernBody.putRawChar8(0); // fade
    modernBody.putBitLong(0); // primary clip count
    modernBody.putBitShort(static_cast<std::uint16_t>(
        DRW_Underlay::kMaxClipVertices));

    auto modernBytes = snapshot(modernBody);
    dwgBuffer modernReader(modernBytes.data(), modernBytes.size());
    DRW_Underlay modernParsed;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        modernParsed, DRW::AC1027, &modernReader));
    CHECK(modernParsed.inverseClipBoundary.empty());
}

// IMAGE empty-clip variant: clip_boundary_type 0, both handles still emitted.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Image::encodeDwg round-trips empty clip boundary",
          "[dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle      = 0x71;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.basePoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint    = DRW_Coord{1.0, 0.0, 0.0};
    src.vVector     = DRW_Coord{0.0, 1.0, 0.0};
    src.sizeu       = 100.0;
    src.sizev       = 100.0;
    src.clip        = 0;
    src.ref         = 0x200;
    src.m_imageDefReactorHandle = 0x201;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Image dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 101);
        CHECK(dst.clipPath.empty());
        CHECK(dst.ref == 0x200u);
        CHECK(dst.m_imageDefReactorHandle == 0x201u);
    }
}

TEST_CASE("DRW_Image::parseDwg resets reused clip state",
          "[dwg-read][entity-encode][image][reuse]") {
    DRW_Image polygon;
    polygon.handle = 0x710;
    polygon.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    polygon.secPoint = DRW_Coord{1.0, 0.0, 0.0};
    polygon.vVector = DRW_Coord{0.0, 1.0, 0.0};
    polygon.sizeu = 10.0;
    polygon.sizev = 10.0;
    polygon.clip = 1;
    polygon.clipPath = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0},
                        {10.0, 10.0, 0.0}};
    polygon.ref = 0x720;
    polygon.m_imageDefReactorHandle = 0x721;
    DrwEntityEncodeTestAccess::layerH(polygon).ref = 0x12;

    DRW_Image empty = polygon;
    empty.handle = 0x711;
    empty.clip = 0;
    empty.clipPath.clear();
    empty.m_clipBoundaryType = 0;
    empty.ref = 0x722;
    empty.m_imageDefReactorHandle = 0x723;

    dwgBufferW polygonBytes;
    dwgBufferW emptyBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encode(polygon, DRW::AC1018,
                                               &polygonBytes));
    REQUIRE(DrwEntityEncodeTestAccess::encode(empty, DRW::AC1018,
                                               &emptyBytes));

    DRW_Image reused;
    dwgBuffer polygonReader(polygonBytes.data().data(), polygonBytes.data().size());
    REQUIRE(DrwEntityEncodeTestAccess::parse(reused, DRW::AC1018,
                                              &polygonReader));
    REQUIRE(reused.clipPath.size() == 3);

    dwgBuffer emptyReader(emptyBytes.data().data(), emptyBytes.data().size());
    REQUIRE(DrwEntityEncodeTestAccess::parse(reused, DRW::AC1018,
                                              &emptyReader));
    CHECK(reused.clipPath.empty());
    CHECK(reused.m_clipBoundaryType == 0);
    CHECK(reused.ref == 0x722u);
    CHECK(reused.m_imageDefReactorHandle == 0x723u);
}

TEST_CASE("DRW_Underlay::parseDwg resets reused clip state",
          "[dwg-read][entity-encode][underlay][reuse]") {
    DRW_Underlay polygon;
    polygon.handle = 0x740u;
    polygon.definitionHandle = 0x741u;
    polygon.position = DRW_Coord{1.0, 2.0, 0.0};
    polygon.clipBoundary = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0},
                            {10.0, 10.0, 0.0}};
    polygon.flags = 2;

    DRW_Underlay empty = polygon;
    empty.handle = 0x742u;
    empty.definitionHandle = 0x743u;
    empty.clipBoundary.clear();
    empty.inverseClipBoundary.clear();
    empty.flags = 2;

    dwgBufferW polygonBytes;
    dwgBufferW emptyBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encode(
        polygon, DRW::AC1018, &polygonBytes));
    REQUIRE(DrwEntityEncodeTestAccess::encode(
        empty, DRW::AC1018, &emptyBytes));

    DRW_Underlay reused;
    reused.inverseClipBoundary = {{2.0, 2.0, 0.0}, {8.0, 2.0, 0.0}};
    dwgBuffer polygonReader(polygonBytes.data().data(), polygonBytes.data().size());
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        reused, DRW::AC1018, &polygonReader));
    REQUIRE(reused.clipBoundary.size() == 3);
    CHECK(reused.inverseClipBoundary.empty());

    dwgBuffer emptyReader(emptyBytes.data().data(), emptyBytes.data().size());
    REQUIRE(DrwEntityEncodeTestAccess::parse(
        reused, DRW::AC1018, &emptyReader));
    CHECK(reused.clipBoundary.empty());
    CHECK(reused.inverseClipBoundary.empty());
    CHECK(reused.flags == 2);
    CHECK(reused.definitionHandle == 0x743u);
}

// HELIX encoder round-trip (Phase 8a-1): a HELIX encodes as a SPLINE body
// (oType 503, custom class AcDbHelix) followed by the AcDbHelix trailer. The
// spline body must stay intact (degree/control points) and every trailer field
// (radius/turns/turnHeight/axis vectors/handedness/constraint) must survive.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Helix::encodeDwg round-trips spline body + AcDbHelix trailer",
          "[dwg-write][entity-encode][helix]") {
    DRW_Helix src;
    src.handle = 0xF8;
    src.color = 5;
    src.flags = 8;           // planar
    src.degree = 3;
    src.tolknot    = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist  = {0, 0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 0.0, 0.0}));
    src.nknots = 8;
    src.ncontrol = 4;
    // AcDbHelix trailer fields.
    src.m_majorVersion = 29;
    src.m_maintVersion = 63;
    src.axisBasePt = DRW_Coord{1.0, 2.0, 3.0};
    src.startPt    = DRW_Coord{4.0, 5.0, 6.0};
    src.axisVector = DRW_Coord{0.0, 0.0, 1.0};
    src.radius      = 2.5;
    src.turns       = 7.0;
    src.turnHeight  = 1.5;
    src.handedness  = true;
    src.constraintType = 2;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        CHECK(DrwEntityEncodeTestAccess::oType(src) == 503);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Helix dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        // Spline body intact.
        CHECK(dst.degree == 3);
        CHECK(dst.controllist.size() == 2u + 2u);  // == 4
        CHECK(dst.controllist[0]->x == Approx(0.0));
        CHECK(dst.controllist[3]->x == Approx(3.0));
        // Trailer round-trips.
        CHECK(dst.m_majorVersion == 29);
        CHECK(dst.m_maintVersion == 63);
        CHECK(dst.axisBasePt.x == Approx(1.0));
        CHECK(dst.axisBasePt.z == Approx(3.0));
        CHECK(dst.startPt.y    == Approx(5.0));
        CHECK(dst.axisVector.z == Approx(1.0));
        CHECK(dst.radius      == Approx(2.5));
        CHECK(dst.turns       == Approx(7.0));
        CHECK(dst.turnHeight  == Approx(1.5));
        CHECK(dst.handedness  == true);
        CHECK(dst.constraintType == 2);
    }
}

// B1 (has_ds_data): for R2013+ (version > AC1024) the entity common preamble
// carries a `has_ds_data` bit after xDictFlag (libreDWG common_entity_data.spec).
// The encoder used to emit a literal 1 (mislabeled "haveNextLinks"), falsely
// advertising an inline ACIS SAB datastore. It now emits the hasDsData member
// (default 0). Differential check: flipping the member must change the encoded
// stream -- pre-fix both encodings were byte-identical because the bit was a
// literal. (Robust to the exact bit position; a full R2013+ parse harness is
// not available at this layer, but the file-level smoke tests round-trip
// AC1027/AC1032.)
TEST_CASE("DRW_Entity sources has_ds_data from the member at AC1027/AC1032",
          "[dwg-write][entity-encode][r2013][b1-has-ds-data]") {
    auto encodePoint = [](DRW::Version ver, std::uint8_t ds) {
        DRW_Point src;
        src.handle = 0x33;
        src.color  = 7;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        src.thickness = 0.0;
        src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;
        DrwEntityEncodeTestAccess::hasDsData(src) = ds;
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        return snapshot(w);
    };

    for (DRW::Version ver : {DRW::AC1027, DRW::AC1032}) {
        const auto withZero = encodePoint(ver, 0);
        const auto withOne  = encodePoint(ver, 1);
        REQUIRE(withZero.size() > 0);
        // Flipping has_ds_data must alter the stream (bit is member-sourced now).
        REQUIRE(withZero != withOne);
    }
}

TEST_CASE("DRW_Line::encodeDwg emits CONTINUOUS ltFlags and paperspace entmode",
          "[dwg-write][entity-encode][linetype][paperspace]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        {
            DRW_Line src;
            src.handle = 0x50;
            src.lineType = "CONTINUOUS";
            DrwEntityEncodeTestAccess::ltFlags(src) = 2;
            src.basePoint = DRW_Coord{0, 0, 0};
            src.secPoint = DRW_Coord{1, 0, 0};
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            CHECK(DrwEntityEncodeTestAccess::ltFlags(dst) == 2);
            CHECK(dst.lineType == "CONTINUOUS");
        }
        {
            DRW_Line src;
            src.handle = 0x51;
            src.space = DRW::PaperSpace;
            src.parentHandle = DRW::NoHandle;
            src.basePoint = DRW_Coord{2, 3, 0};
            src.secPoint = DRW_Coord{4, 5, 0};
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            CHECK(dst.space == DRW::PaperSpace);
        }
    }
}

TEST_CASE("DRW_PointCloud::encodeDwg rejects unsupported legacy versions",
          "[dwg-write][entity-encode][pointcloud]") {
    DRW_PointCloud pc;
    dwgBufferW w;
    CHECK_FALSE(DrwEntityEncodeTestAccess::encode(pc, DRW::AC1018, &w));
    CHECK(DrwEntityEncodeTestAccess::encode(pc, DRW::AC1024, &w));
}

// The compact scale forms (dataFlags 3 and 2) are lossless substitutions: the
// reader restores exactly 1.0, or exactly xscale for y and z.  Selecting them
// on a tolerant comparison would silently round the scale that is written, so
// these two cases pin the encoder to a bit-exact match.
TEST_CASE("DRW_Insert::encodeDwg takes the compact scale form only when exact",
          "[dwg-write][entity-encode][insert-scale]") {
    auto roundTrip = [](double sx, double sy, double sz, std::size_t& size) {
        DRW_Insert src;
        src.handle = 0x41;
        src.color = 7;
        src.ltypeScale = 1.0;
        src.name = "BASE";
        src.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        src.xscale = sx;
        src.yscale = sy;
        src.zscale = sz;
        src.angle = 0.0;
        src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
        auto bytes = snapshot(w);
        size = bytes.size();

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Insert dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));
        return dst;
    };

    std::size_t unitSize = 0;
    const DRW_Insert unit = roundTrip(1.0, 1.0, 1.0, unitSize);
    CHECK(unit.xscale == 1.0);
    CHECK(unit.yscale == 1.0);
    CHECK(unit.zscale == 1.0);

    // One ULP above 1.0 is not 1.0, and must survive as itself.
    const double justOver = std::nextafter(1.0, 2.0);
    REQUIRE(justOver != 1.0);
    std::size_t nearSize = 0;
    const DRW_Insert near = roundTrip(justOver, 1.0, 1.0, nearSize);
    CHECK(near.xscale == justOver);   // not collapsed to 1.0
    CHECK(near.yscale == 1.0);
    CHECK(near.zscale == 1.0);

    // And the exact case really is the cheaper encoding, so the shortcut works.
    CHECK(unitSize < nearSize);

    // The uniform-scale form (dataFlags 2) needs the same exactness: x matches
    // y bit-exact, but z is one ULP off, so this must NOT take the compact
    // path - that would restore z as x's value instead of its own.
    const double zJustOver = std::nextafter(2.0, 3.0);
    REQUIRE(zJustOver != 2.0);
    std::size_t uniformSize = 0;
    const DRW_Insert notQuiteUniform = roundTrip(2.0, 2.0, zJustOver, uniformSize);
    CHECK(notQuiteUniform.xscale == 2.0);
    CHECK(notQuiteUniform.yscale == 2.0);
    CHECK(notQuiteUniform.zscale == zJustOver);   // not collapsed onto x/y

    // A genuinely uniform scale still gets the compact form (smaller than
    // the near-uniform case above, which falls through to the general form).
    std::size_t trueUniformSize = 0;
    const DRW_Insert trueUniform = roundTrip(2.0, 2.0, 2.0, trueUniformSize);
    CHECK(trueUniform.xscale == 2.0);
    CHECK(trueUniform.yscale == 2.0);
    CHECK(trueUniform.zscale == 2.0);
    CHECK(trueUniformSize < uniformSize);

    // dataFlags 1 (x defaults to 1, y/z independent) needs the same
    // exactness for x: parseDwg only omits reading x, and leaves it at its
    // constructed default, when dataFlags says so - so encodeDwg may only
    // take that form when x really is 1.0, bit for bit.
    std::size_t xDefaultSize = 0;
    const DRW_Insert xDefault = roundTrip(1.0, 3.0, 4.0, xDefaultSize);
    CHECK(xDefault.xscale == 1.0);
    CHECK(xDefault.yscale == 3.0);
    CHECK(xDefault.zscale == 4.0);

    // General form has to write x explicitly, so the dataFlags-1 shortcut
    // must be smaller for the same y/z.
    std::size_t xNotDefaultSize = 0;
    const DRW_Insert xNotDefault =
        roundTrip(std::nextafter(1.0, 2.0), 3.0, 4.0, xNotDefaultSize);
    CHECK(xNotDefault.xscale == std::nextafter(1.0, 2.0)); // not collapsed to 1.0
    CHECK(xNotDefault.yscale == 3.0);
    CHECK(xNotDefault.zscale == 4.0);
    CHECK(xDefaultSize < xNotDefaultSize);
}

TEST_CASE("DWG point-cloud parsers reject truncated modern bodies",
          "[dwg][safety][pointcloud]") {
    DRW_PointCloud pointCloud;
    pointCloud.handle = 0xB01;
    pointCloud.classVersion = 3;
    pointCloud.origin = DRW_Coord{1.0, 2.0, 3.0};
    pointCloud.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
    pointCloud.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    pointCloud.pointCount = 1234;
    pointCloud.ucsName = "UCS-A";
    pointCloud.definitionHandle = 0xB10;
    pointCloud.reactorHandle = 0xB11;

    dwgBufferW pointCloudBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encode(
        pointCloud, DRW::AC1027, &pointCloudBytes));
    auto truncatedPointCloud = snapshot(pointCloudBytes);
    REQUIRE(truncatedPointCloud.size() > 1);
    truncatedPointCloud.pop_back();
    dwgBuffer pointCloudInput(truncatedPointCloud.data(),
                              truncatedPointCloud.size());
    DRW_PointCloud parsedPointCloud;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsedPointCloud, DRW::AC1027, &pointCloudInput));

    DRW_PointCloudEx pointCloudEx;
    pointCloudEx.handle = 0xB02;
    pointCloudEx.classVersion = 7;
    pointCloudEx.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
    pointCloudEx.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    pointCloudEx.name = "cloud-ex";
    pointCloudEx.definitionHandle = 0xB12;
    pointCloudEx.reactorHandle = 0xB13;

    dwgBufferW pointCloudExBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encode(
        pointCloudEx, DRW::AC1032, &pointCloudExBytes));
    auto truncatedPointCloudEx = snapshot(pointCloudExBytes);
    REQUIRE(truncatedPointCloudEx.size() > 1);
    truncatedPointCloudEx.pop_back();
    dwgBuffer pointCloudExInput(truncatedPointCloudEx.data(),
                                truncatedPointCloudEx.size());
    DRW_PointCloudEx parsedPointCloudEx;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsedPointCloudEx, DRW::AC1032, &pointCloudExInput));
}

TEST_CASE("DWG point-cloud parsers reject non-finite coordinates",
          "[dwg][safety][pointcloud]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();

    DRW_PointCloud pointCloud;
    pointCloud.handle = 0xB03;
    pointCloud.classVersion = 3;
    pointCloud.layer = "0";
    DrwEntityEncodeTestAccess::layerH(pointCloud).ref = 0x12;

    dwgBufferW pointCloudBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        pointCloud, DRW::AC1027, &pointCloudBytes));
    pointCloudBytes.putBitShort(pointCloud.classVersion);
    pointCloudBytes.putBitDouble(nan);
    pointCloudBytes.putBitDouble(2.0);
    pointCloudBytes.putBitDouble(3.0);
    auto pointCloudData = snapshot(pointCloudBytes);
    dwgBuffer pointCloudInput(pointCloudData.data(), pointCloudData.size());
    DRW_PointCloud parsedPointCloud;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsedPointCloud, DRW::AC1027, &pointCloudInput));

    DRW_PointCloudEx pointCloudEx;
    pointCloudEx.handle = 0xB04;
    pointCloudEx.classVersion = 7;
    pointCloudEx.layer = "0";
    DrwEntityEncodeTestAccess::layerH(pointCloudEx).ref = 0x12;

    dwgBufferW pointCloudExBytes;
    REQUIRE(DrwEntityEncodeTestAccess::encodeCommon(
        pointCloudEx, DRW::AC1032, &pointCloudExBytes));
    pointCloudExBytes.putBitShort(pointCloudEx.classVersion);
    pointCloudExBytes.put3BitDouble(DRW_Coord{nan, 1.0, 1.0});
    pointCloudExBytes.put3BitDouble(DRW_Coord{2.0, 2.0, 2.0});
    pointCloudExBytes.put3BitDouble(DRW_Coord{3.0, 3.0, 3.0});
    pointCloudExBytes.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0});
    pointCloudExBytes.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0});
    pointCloudExBytes.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0});
    auto pointCloudExData = snapshot(pointCloudExBytes);
    dwgBuffer pointCloudExInput(pointCloudExData.data(),
                                pointCloudExData.size());
    DRW_PointCloudEx parsedPointCloudEx;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        parsedPointCloudEx, DRW::AC1032, &pointCloudExInput));
}

TEST_CASE("DWG modern 3DLINE reads raw-double body after common prologue",
          "[dwg][3dline]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, 1162);
    body.putRawLong32(0);  // R2000 object-size field; unused by this body.

    dwgHandle entityHandle;
    entityHandle.code = 4;
    entityHandle.ref = 0x89;
    body.putHandle(entityHandle);
    body.putBitShort(0);  // no EED
    body.putBit(0);       // no proxy graphics
    body.put2Bits(2);     // no owner handle
    body.putBitLong(0);   // no reactors
    body.putBit(1);       // no previous/next links
    body.putBitShort(256); // BYLAYER
    body.putBitDouble(1.0); // linetype scale
    body.put2Bits(1);     // no linetype handle
    body.put2Bits(0);     // no plot-style handle
    body.putBitShort(0);  // visible
    body.putRawChar8(0xFF); // lineweight

    for (double value : {1.0, 2.0, 3.0, 4.0, 5.0, 6.0,
                         0.0, 0.0, 1.0, 0.75})
        body.putRawDouble(value);

    dwgHandle nullHandle;
    nullHandle.code = 0;
    nullHandle.ref = 0;
    body.putHandle(nullHandle); // extension dictionary
    dwgHandle layerHandle;
    layerHandle.code = 5;
    layerHandle.ref = 0x12;
    body.putHandle(layerHandle);

    auto bytes = snapshot(body);
    dwgBuffer input(bytes.data(), bytes.size());
    DRW_3DLine parsed;
    REQUIRE(DrwEntityEncodeTestAccess::parse(parsed, DRW::AC1015, &input));
    CHECK(parsed.eType == DRW::THREEDLINE);
    CHECK(parsed.handle == 0x89);
    CHECK(parsed.basePoint.x == 1.0);
    CHECK(parsed.basePoint.y == 2.0);
    CHECK(parsed.basePoint.z == 3.0);
    CHECK(parsed.secPoint.x == 4.0);
    CHECK(parsed.secPoint.y == 5.0);
    CHECK(parsed.secPoint.z == 6.0);
    CHECK(parsed.extPoint.x == 0.0);
    CHECK(parsed.extPoint.y == 0.0);
    CHECK(parsed.extPoint.z == 1.0);
    CHECK(parsed.thickness == 0.75);
    CHECK(parsed.dwgLayerHandle() == 0x12);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 1);
    truncatedBytes.pop_back();
    dwgBuffer truncatedInput(truncatedBytes.data(), truncatedBytes.size());
    DRW_3DLine rejected;
    CHECK_FALSE(DrwEntityEncodeTestAccess::parse(
        rejected, DRW::AC1015, &truncatedInput));
}
