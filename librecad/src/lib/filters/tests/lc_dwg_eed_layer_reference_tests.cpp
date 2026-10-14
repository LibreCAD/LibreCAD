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
 * A layer reference in the extended data of a DWG object (XDATA 1003).
 *
 * The item is code 3 and the handle of the layer in eight bytes, most
 * significant first: the order of the entity handle of code 5. The ODA
 * specification describes both alike ("8 bytes -- even if the leading ones
 * are 0", section 28, Extended Entity Data), and AutoCAD writes both so.
 * libdxfrw read and wrote the layer the other way round. The layer of an
 * AutoCAD drawing was never found, and the export then refused the whole
 * drawing over that one item.
 *
 * Fixtures: the TEXT "XData Layer" (handle 0x91B) of two drawings AutoCAD
 * saved, as byte arrays; see testdata/PROVENANCE.md. Its extended data, for
 * APPID 0x8FD "LAYER_APP", is "{", layer 0x8F5 "APP_LAYER", "}".
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_interface.h"
#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgreader.h"
#include "intern/dwgreader15.h"
#include "intern/dwgwriter15.h"
#include "libdwgr.h"

namespace {

// TEXT 0x91B of ACadSharp's samples/sample_AC1014.dwg (AutoCAD R14), from the
// object type to the end of the handles: the bytes after the record's size.
const std::uint8_t kAutoCadR14Text[] = {
    0x40, 0x40, 0x82, 0x46, 0xD0, 0xD5, 0x20, 0x8F, 0xD0, 0x20, 0x00, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0x50, 0x20, 0x18, 0xBA, 0x04,
    0x00, 0x01, 0x5E, 0xD0, 0x42, 0x26, 0x92, 0x32, 0x4C, 0x02, 0x82, 0x07,
    0xD1, 0x84, 0xB9, 0xBA, 0x01, 0x6A, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
    0x35, 0x2A, 0x18, 0xB0, 0x88, 0xC2, 0xE8, 0xC2, 0x40, 0x98, 0xC2, 0xF2,
    0xCA, 0xE4, 0x01, 0x51, 0x82, 0x88, 0x82, 0x88, 0x8F,
};

// The same TEXT in samples/sample_AC1015.dwg (AutoCAD 2000).
const std::uint8_t kAutoCadR2000Text[] = {
    0x40, 0x46, 0x80, 0x80, 0x00, 0x00, 0x82, 0x46, 0xD0, 0xD5, 0x20, 0x8F,
    0xD0, 0x20, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0x50,
    0x20, 0x19, 0x5D, 0x08, 0x77, 0xFC, 0x21, 0x13, 0x49, 0x19, 0x26, 0x01,
    0x41, 0x03, 0xE8, 0xC2, 0x5C, 0xDD, 0x00, 0xB5, 0x4B, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x43, 0x16, 0x11, 0x18, 0x5D, 0x18,
    0x48, 0x13, 0x18, 0x5E, 0x59, 0x5C, 0x80, 0x0C, 0x14, 0x44, 0x14, 0x44,
    0x7F,
};

// The extended data starts after the object type, the R2000 object size and
// the TEXT's own handle. It is the data size (13), the APPID handle, the 13
// bytes 02 00 / 03 00 00 00 00 00 00 08 F5 / 02 01 and the closing size 0.
constexpr std::uint32_t kR14EedBit = 34;
constexpr std::uint32_t kR2000EedBit = 66;
constexpr std::uint32_t kEedBits = 140;

constexpr std::uint32_t kTextHandle = 0x91B;
constexpr std::uint32_t kAppLayerHandle = 0x8F5;   // LAYER "APP_LAYER"
constexpr std::uint32_t kLayerAppHandle = 0x8FD;   // APPID "LAYER_APP"

struct AutoCadRecord {
    const char* m_name;
    DRW::Version m_version;
    std::vector<std::uint8_t> m_bytes;
    std::uint32_t m_eedBit;
};

std::vector<AutoCadRecord> autoCadRecords() {
    return {
        {"R14", DRW::AC1014,
         {std::begin(kAutoCadR14Text), std::end(kAutoCadR14Text)}, kR14EedBit},
        {"R2000", DRW::AC1015,
         {std::begin(kAutoCadR2000Text), std::end(kAutoCadR2000Text)},
         kR2000EedBit},
    };
}

bool bitAt(const std::vector<std::uint8_t>& bytes, std::uint32_t bit) {
    return ((bytes[bit / 8] >> (7 - bit % 8)) & 1) != 0;
}

bool sameBits(const std::vector<std::uint8_t>& lhs, std::uint32_t lhsStart,
              const std::vector<std::uint8_t>& rhs, std::uint32_t rhsStart,
              std::uint32_t count) {
    if (lhsStart + count > lhs.size() * 8 || rhsStart + count > rhs.size() * 8)
        return false;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (bitAt(lhs, lhsStart + i) != bitAt(rhs, rhsStart + i))
            return false;
    }
    return true;
}

class TextRecord final : public DRW_Text {
public:
    using DRW_Text::parseDwg;
};

// Gives the names behind the handles of a drawing to a parsed object, as a
// DWG reader does once its tables are read.
class NameResolver final : public dwgReader15 {
public:
    NameResolver()
        : dwgReader15(std::make_unique<dwgBuffer>(noFile(), 1), nullptr) {}

    using dwgReader::parseAttribs;

    void addLayer(std::uint32_t handle, const std::string& name) {
        auto layer = std::make_unique<DRW_Layer>();
        layer->name = name;
        layermap.emplace(handle, layer.release());
    }

    void addAppId(std::uint32_t handle, const std::string& name) {
        auto appId = std::make_unique<DRW_AppId>();
        appId->name = name;
        appIdmap.emplace(handle, appId.release());
    }

private:
    // The resolver reads no file.
    static std::uint8_t* noFile() {
        static std::uint8_t byte = 0;
        return &byte;
    }
};

bool parseRecord(const AutoCadRecord& record, TextRecord& text) {
    std::vector<std::uint8_t> bytes = record.m_bytes;
    DRW_TextCodec codec;
    codec.setVersion(record.m_version, false);
    dwgBuffer buffer(bytes.data(), bytes.size(), &codec);
    return text.parseDwg(record.m_version, &buffer, 0);
}

// One chunk of extended data for APPID 0x8FD holding the items given.
std::vector<std::uint8_t> eedChunk(const std::vector<std::uint8_t>& items) {
    dwgBufferW writer;
    writer.putBitShort(static_cast<std::uint16_t>(items.size()));
    dwgHandle appHandle;
    appHandle.code = 5;
    appHandle.ref = kLayerAppHandle;
    writer.putHandle(appHandle);
    writer.putBytes(items.data(), items.size());
    writer.putBitShort(0);
    return writer.data();
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG extended data reads AutoCAD's layer reference",
          "[dwg][eed][xdata]") {
    for (const AutoCadRecord& record : autoCadRecords()) {
        INFO(record.m_name);
        TextRecord text;
        REQUIRE(parseRecord(record, text));
        CHECK(text.handle == kTextHandle);
        CHECK(text.text == "XData Layer");

        // The APPID placeholder, then "{", the layer and "}".
        REQUIRE(text.extData.size() == 4u);
        const auto& layer = text.extData[2];
        REQUIRE(layer != nullptr);
        CHECK(layer->code() == 1003);
        CHECK(layer->rawDwgLayerReference() == kAppLayerHandle);
        REQUIRE(text.pendingLayerRefResolutions.size() == 1u);
        CHECK(text.pendingLayerRefResolutions.front().indexInExtData == 2u);
        CHECK(text.pendingLayerRefResolutions.front().handleRef
              == kAppLayerHandle);

        NameResolver names;
        names.addLayer(kAppLayerHandle, "APP_LAYER");
        names.addAppId(kLayerAppHandle, "LAYER_APP");
        names.parseAttribs(&text);
        CHECK(std::string(text.extData[0]->c_str()) == "LAYER_APP");
        CHECK(std::string(text.extData[1]->c_str()) == "{");
        CHECK(layer->isLayerRef());
        CHECK(std::string(layer->c_str()) == "APP_LAYER");
        CHECK(std::string(text.extData[3]->c_str()) == "}");
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG extended data writes a layer reference as AutoCAD does",
          "[dwg][eed][xdata]") {
    for (const AutoCadRecord& record : autoCadRecords()) {
        INFO(record.m_name);
        TextRecord text;
        REQUIRE(parseRecord(record, text));
        NameResolver names;
        names.addLayer(kAppLayerHandle, "APP_LAYER");
        names.addAppId(kLayerAppHandle, "LAYER_APP");
        names.parseAttribs(&text);

        std::vector<DRW_Variant*> items;
        for (const auto& item : text.extData)
            items.push_back(item.get());
        const std::vector<DRW_Entity::PendingHandleRef> appIds{
            {0u, kLayerAppHandle}};
        const std::vector<DRW_Entity::PendingHandleRef> layers{
            {2u, kAppLayerHandle}};
        dwgBufferW written;
        REQUIRE(DRW_Entity::encodeDwgEed(DRW::AC1015, items, appIds, layers,
                                         30, written));
        REQUIRE(written.bitCount() == kEedBits);
        CHECK(sameBits(record.m_bytes, record.m_eedBit, written.data(), 0,
                       kEedBits));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG extended data reads a layer reference in LibreCAD's old order",
          "[dwg][eed][xdata]") {
    // LibreCAD wrote the handle least significant byte first. Only one of the
    // two orders can be a 32-bit handle, so the layer is taken from the other
    // order only when the written one cannot name a layer at all.
    auto bytes = eedChunk(
        {2, 0, 3, 0xF5, 0x08, 0, 0, 0, 0, 0, 0, 2, 1});
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::vector<DwgEedChunk> chunks;
    REQUIRE(readDwgEed(DRW::AC1015, buffer, chunks));
    REQUIRE(chunks.size() == 1u);
    CHECK(chunks.front().appHandle == kLayerAppHandle);
    REQUIRE(chunks.front().items.size() == 3u);
    const DRW_Variant& layer = chunks.front().items[1];
    CHECK(layer.code() == 1003);
    // The value as written stays the raw reference.
    CHECK(layer.rawDwgLayerReference() == 0xF508000000000000ULL);
    CHECK(std::string(layer.c_str()) == "F508000000000000");
    REQUIRE(chunks.front().layerRefs.size() == 1u);
    CHECK(chunks.front().layerRefs.front().itemIndex == 1u);
    CHECK(chunks.front().layerRefs.front().handleRef == kAppLayerHandle);

    // In AutoCAD's order the reference is taken as written.
    auto autoCad = eedChunk({3, 0, 0, 0, 0, 0, 0, 0x08, 0xF5});
    dwgBuffer autoCadBuffer(autoCad.data(), autoCad.size());
    chunks.clear();
    REQUIRE(readDwgEed(DRW::AC1015, autoCadBuffer, chunks));
    REQUIRE(chunks.size() == 1u);
    CHECK(chunks.front().items.front().rawDwgLayerReference()
          == kAppLayerHandle);
    REQUIRE(chunks.front().layerRefs.size() == 1u);
    CHECK(chunks.front().layerRefs.front().handleRef == kAppLayerHandle);

    // A reference that is no 32-bit handle either way names no layer.
    auto wide = eedChunk({3, 1, 0, 0, 0, 0, 0, 0, 1});
    dwgBuffer wideBuffer(wide.data(), wide.size());
    chunks.clear();
    REQUIRE(readDwgEed(DRW::AC1015, wideBuffer, chunks));
    REQUIRE(chunks.size() == 1u);
    CHECK(chunks.front().items.front().rawDwgLayerReference()
          == 0x0100000000000001ULL);
    CHECK(chunks.front().layerRefs.empty());
}

namespace {

// Writes one line with the extended data of two applications, LC_KEPT and
// LC_LOST, and a layer with LC_LOST's data. LC_LOST names a layer the drawing
// does not have. As a reader it keeps what was read back.
class LeftOutApplicationIface final : public DRW_Interface {
public:
    dwgRW* m_writer = nullptr;
    bool m_lineWritten = false;
    bool m_onlyLostLineWritten = false;
    bool m_lostFirst = false;
    std::vector<DRW_Line> m_lines;
    std::map<std::string, std::size_t> m_layerEedSizes;

    static void addKept(std::vector<std::shared_ptr<DRW_Variant>>& data) {
        data.push_back(std::make_shared<DRW_Variant>(1001, "LC_KEPT"));
        data.push_back(std::make_shared<DRW_Variant>(1002, "{"));
        data.push_back(
            std::make_shared<DRW_Variant>(1003, "KEPT_LAYER", true));
        data.push_back(std::make_shared<DRW_Variant>(1000, "kept"));
        data.push_back(std::make_shared<DRW_Variant>(1002, "}"));
    }

    static void addLost(std::vector<std::shared_ptr<DRW_Variant>>& data) {
        data.push_back(std::make_shared<DRW_Variant>(1001, "LC_LOST"));
        data.push_back(std::make_shared<DRW_Variant>(1002, "{"));
        data.push_back(
            std::make_shared<DRW_Variant>(1003, "KEPT_LAYER", true));
        data.push_back(
            std::make_shared<DRW_Variant>(1003, "NO_SUCH_LAYER", true));
        data.push_back(
            std::make_shared<DRW_Variant>(1003, "ALSO_MISSING", true));
        data.push_back(std::make_shared<DRW_Variant>(1000, "lost"));
        data.push_back(std::make_shared<DRW_Variant>(1002, "}"));
    }

    void writeLayers() override {
        DRW_Layer kept;
        kept.name = "KEPT_LAYER";
        REQUIRE(kept.addExtData(std::make_unique<DRW_Variant>(1001, "LC_LOST")));
        REQUIRE(kept.addExtData(
            std::make_unique<DRW_Variant>(1003, "NO_SUCH_LAYER", true)));
        REQUIRE(m_writer->addLayer(&kept));
    }

    void writeAppId() override {
        for (const char* name : {"LC_KEPT", "LC_LOST"}) {
            DRW_AppId appId;
            appId.name = name;
            REQUIRE(m_writer->addAppId(&appId));
        }
    }

    void writeEntities() override {
        DRW_Line line;
        line.secPoint = DRW_Coord{10.0, 5.0, 0.0};
        if (m_lostFirst)
            addLost(line.extData);
        addKept(line.extData);
        if (!m_lostFirst)
            addLost(line.extData);
        m_lineWritten = m_writer->writeLine(&line);

        DRW_Line onlyLost;
        onlyLost.secPoint = DRW_Coord{-10.0, 5.0, 0.0};
        addLost(onlyLost.extData);
        m_onlyLostLineWritten = m_writer->writeLine(&onlyLost);
    }

    void addLine(const DRW_Line& line) override { m_lines.push_back(line); }
    void addLayer(const DRW_Layer& layer) override {
        m_layerEedSizes[layer.name] = layer.extData.size();
    }

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimArc(const DRW_DimArc*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlockRecords() override {}
    void writeBlocks() override {}
    void writeLTypes() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
};

std::vector<std::string> strings(const DRW_Entity& entity) {
    std::vector<std::string> values;
    for (const auto& value : entity.extData) {
        if (value != nullptr && value->type() == DRW_Variant::STRING)
            values.emplace_back(value->c_str());
    }
    return values;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG export leaves out application data naming a missing layer",
          "[dwg][eed][xdata][writer]") {
    const bool lostFirst = GENERATE(false, true);
    CAPTURE(lostFirst);
    for (const DRW::Version version :
         {DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027,
          DRW::AC1032}) {
        INFO("version " << static_cast<int>(version));
        const std::string path =
            (std::filesystem::temp_directory_path()
             / ("lc_dwg_eed_layer_left_out_"
                + std::to_string(static_cast<int>(version)) + ".dwg"))
                .string();

        LeftOutApplicationIface written;
        written.m_lostFirst = lostFirst;
        std::map<std::string, std::size_t> leftOut;
        {
            dwgRW writer(path.c_str());
            written.m_writer = &writer;
            REQUIRE(writer.write(&written, version, false));
            leftOut = writer.leftOut();
        }
        CHECK(written.m_lineWritten);
        CHECK(written.m_onlyLostLineWritten);
        // Two lines and a layer each lost LC_LOST's data.
        REQUIRE(leftOut.size() == 1u);
        CHECK(leftOut.begin()->first
              == "application data naming a layer not in the drawing");
        CHECK(leftOut.begin()->second == 3u);

        LeftOutApplicationIface read;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&read, false));
        }
        std::remove(path.c_str());
        REQUIRE(read.m_lines.size() == 2u);
        CHECK(strings(read.m_lines[0])
              == std::vector<std::string>{"LC_KEPT", "{", "KEPT_LAYER",
                                          "kept", "}"});
        CHECK(read.m_lines[0].extData[2]->isLayerRef());
        CHECK(read.m_lines[1].extData.empty());
        CHECK(read.m_layerEedSizes["KEPT_LAYER"] == 0u);
    }
}

TEST_CASE("DWG rollback restores application-data omission counts",
          "[dwg][eed][xdata][writer][transaction]") {
    class Writer final : public dwgWriter15 {
    public:
        Writer(std::ofstream* stream, DRW_Header* header)
            : dwgWriter15(stream, header) { m_version = DRW::AC1015; }
    };
    DRW_Header header;
    std::ofstream stream;
    Writer writer(&stream, &header);
    const auto initial = writer.checkpointCompoundWrite();

    DRW_Line line;
    line.extData.push_back(std::make_shared<DRW_Variant>(1001, "ACAD"));
    line.extData.push_back(std::make_shared<DRW_Variant>(1003, "MISSING", true));
    REQUIRE(writer.encodeEntity(&line));
    const std::map<std::string, std::size_t> once{
        {"application data naming a layer not in the drawing", 1u}};
    CHECK(writer.leftOut() == once);

    // The missing block is rejected after preparing its application data.
    DRW_Insert invalid;
    invalid.extData = line.extData;
    CHECK_FALSE(writer.encodeEntity(&invalid));
    CHECK(writer.leftOut() == once);

    const auto committed = writer.checkpointPublicTransaction();
    line.handle = 0;
    REQUIRE(writer.encodeEntity(&line));
    CHECK(writer.leftOut().begin()->second == 2u);
    writer.rollbackCompoundWrite(committed);
    CHECK(writer.leftOut() == once);
    writer.rollbackCompoundWrite(initial);
    CHECK(writer.leftOut().empty());
}
