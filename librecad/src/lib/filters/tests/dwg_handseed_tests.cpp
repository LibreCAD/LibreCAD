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

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgobjectframe.h"
#include "intern/dwgreader.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/rscodec.h"
#include "intern/dwgutil.h"

class DwgHandseedTestAccess {
public:
    static bool encode(DRW_Header& header, DRW::Version version,
                       dwgBufferW *data, dwgBufferW *handles) {
        return header.encodeDwg(version, data, handles);
    }

    static bool parse(DRW_Header& header, DRW::Version version,
                      dwgBuffer *data, dwgBuffer *handles) {
        return header.parseDwg(version, data, handles, /*mv=*/0);
    }
};

namespace {

std::unique_ptr<dwgWriter> makeDwgWriter(DRW::Version version,
                                         std::ofstream *stream,
                                         DRW_Header *header) {
    switch (version) {
    case DRW::AC1015: return std::make_unique<dwgWriter15>(stream, header);
    case DRW::AC1018: return std::make_unique<dwgWriter18>(stream, header);
    case DRW::AC1021: return std::make_unique<dwgWriter21>(stream, header);
    case DRW::AC1024: return std::make_unique<dwgWriter24>(stream, header);
    case DRW::AC1027: return std::make_unique<dwgWriter27>(stream, header);
    case DRW::AC1032: return std::make_unique<dwgWriter32>(stream, header);
    default: return nullptr;
    }
}

class FailingDwgWriter final : public dwgWriter24 {
public:
    using dwgWriter24::dwgWriter24;

protected:
    void finishObject() override { m_writeError = true; }
};

class ManualHandleDwgWriter15 final : public dwgWriter15 {
public:
    using dwgWriter15::dwgWriter15;

    void emitManualObject(std::uint32_t handle) {
        reserveHandle(handle);
        dwgBufferW& body = beginObject(handle);
        body.putObjType(DRW::AC1015, dwgType::LINE);
        body.putRawLong32(0);
        finishObject();
    }
};

std::uint64_t readLittleEndian64(const std::uint8_t* bytes) {
    std::uint64_t value = 0;
    for (unsigned int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
    return value;
}

std::uint32_t readLittleEndian32(const std::uint8_t* bytes) {
    std::uint32_t value = 0;
    for (unsigned int i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes[i]) << (i * 8);
    return value;
}

std::uint64_t expectedR2007SectionHash(std::string_view name) {
    if (name == "AcDb:Security") return 0x4a0204eau;
    if (name == "AcDb:FileDepList") return 0x6c4205cau;
    if (name == "AcDb:VBAProject") return 0x586e0544u;
    if (name == "AcDb:AppInfoHistory") return 0x96de0737u;
    if (name == "AcDb:AppInfo") return 0x3fa0043eu;
    if (name == "AcDb:Preview") return 0x40aa0473u;
    if (name == "AcDb:SummaryInfo") return 0x717a060fu;
    if (name == "AcDb:RevHistory") return 0x60a205b3u;
    if (name == "AcDb:AcDbObjects") return 0x674c05a9u;
    if (name == "AcDb:ObjFreeSpace") return 0x77e2061fu;
    if (name == "AcDb:Template") return 0x4a1404ceu;
    if (name == "AcDb:Handles") return 0x3f6e0450u;
    if (name == "AcDb:Classes") return 0x3f54045fu;
    if (name == "AcDb:AuxHeader") return 0x54f0050au;
    if (name == "AcDb:Header") return 0x32b803d9u;
    return 0;
}

void writeLittleEndian64(std::uint8_t* bytes, std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i)
        bytes[i] = static_cast<std::uint8_t>(value >> (i * 8));
}

std::uint64_t rotateR2007CheckValue(std::uint64_t value,
                                    std::uint64_t control) {
    const unsigned int shift = static_cast<unsigned int>(control & 0x1fU);
    return shift == 0 ? value : (value << shift) | (value >> (64u - shift));
}

std::uint64_t r2007CheckNormalCrc(std::uint64_t random1,
                                  std::uint64_t random2) {
    const std::uint64_t value0 = rotateR2007CheckValue(random1, random2);
    const std::uint64_t value1 = rotateR2007CheckValue(value0, value0);
    const std::uint64_t value2 = rotateR2007CheckValue(random2, value1);
    const std::uint64_t value3 = rotateR2007CheckValue(value2, value2);
    const std::uint64_t value4 = rotateR2007CheckValue(random1, value3);
    const std::uint64_t value5 = rotateR2007CheckValue(value4, value4);
    const std::uint64_t value6 = rotateR2007CheckValue(value5, value5);
    const std::uint64_t value7 = rotateR2007CheckValue(value6, value6);
    const std::array<std::uint64_t, 8> values = {
        value0, value1, value2, value3,
        value4, value5, value6, value7};
    std::array<std::uint8_t, 64> bytes{};
    for (std::size_t i = 0; i < values.size(); ++i)
        writeLittleEndian64(bytes.data() + i * 8, values[i]);
    return dwgUtil::crc64Normal(~random2, bytes.data(), bytes.size());
}

std::uint64_t r2007CheckMirroredCrc(std::uint64_t random1,
                                    std::uint64_t random2,
                                    std::uint64_t normalCrc) {
    const std::uint64_t value0 = rotateR2007CheckValue(random1, random2);
    const std::uint64_t value1 = rotateR2007CheckValue(normalCrc, value0);
    const std::uint64_t value2 = rotateR2007CheckValue(random2, value1);
    const std::uint64_t value3 = rotateR2007CheckValue(normalCrc, value2);
    const std::uint64_t value4 = rotateR2007CheckValue(random1, value3);
    const std::uint64_t value5 = rotateR2007CheckValue(normalCrc, value4);
    const std::uint64_t value6 = rotateR2007CheckValue(random2, value5);
    const std::uint64_t value7 = rotateR2007CheckValue(value6, value6);
    const std::array<std::uint64_t, 8> values = {
        value0, value1, value2, value3,
        value4, value5, value6, value7};
    std::array<std::uint8_t, 64> bytes{};
    for (std::size_t i = 0; i < values.size(); ++i)
        writeLittleEndian64(bytes.data() + i * 8, values[i]);
    return dwgUtil::crc64Mirrored(~random1, bytes.data(), bytes.size());
}

} // namespace

TEST_CASE("DWG R2007 CRCs match external header check vectors",
          "[dwg-write][r2007][crc]") {
    struct CheckVector {
        std::uint64_t random1;
        std::uint64_t random2;
        std::uint64_t normalCrc;
        std::uint64_t mirroredCrc;
    };
    // Extracted from the unmodified libreDWG test fixtures' check-data words
    // at 0x458: normal CRC, mirrored CRC, random1, random2, encoded seed.
    const CheckVector vectors[] = {
        // test/test-data/2007/Line.dwg
        {0xd3dd80f1bf036224ULL, 0xe2e60fa9c8cc28beULL,
         0xcd403b912d162648ULL, 0xdaacf0d817e61234ULL},
        // test/test-data/example_2007.dwg
        {0xe58b031fda4636c8ULL, 0xd49341d42d360991ULL,
         0x126ca9c6c0c47a9bULL, 0x594da447079d8e81ULL}
    };
    for (const auto& vector : vectors) {
        CAPTURE(vector.random1, vector.random2);
        CHECK(r2007CheckNormalCrc(vector.random1, vector.random2)
              == vector.normalCrc);
        CHECK(r2007CheckMirroredCrc(vector.random1, vector.random2,
                                  vector.normalCrc) == vector.mirroredCrc);
    }
}

TEST_CASE("DWG R2007 CRCs order partial blocks according to the specification",
          "[dwg-write][r2007][crc]") {
    const std::array<std::uint8_t, 15> bytes = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    struct CrcVector {
        std::size_t size;
        std::uint64_t normalCrc;
        std::uint64_t mirroredCrc;
    };
    // Seed zero. Independent bitwise calculations using ODA 5.12's byte
    // ordering and the published normal/mirrored tables, not writer output.
    const CrcVector vectors[] = {
        {5u, 0x9b895620f7b29de7ULL, 0x9da22f6e10ade5bbULL},
        {6u, 0xddd932f6d0d8fb9dULL, 0x8c88d81dcba78edbULL},
        {7u, 0x5c7b616af77a7afeULL, 0x269a47788330ed56ULL},
        {13u, 0x78e5c853d7d19affULL, 0xc3254eff07d6520eULL},
        {14u, 0x04b5b94ae3f93faaULL, 0x8fabb416af594dd9ULL},
        {15u, 0x38c2a39878ebab60ULL, 0x78016930802bff12ULL}
    };
    for (const auto& vector : vectors) {
        CAPTURE(vector.size);
        CHECK(dwgUtil::crc64Normal(0, bytes.data(), vector.size)
              == vector.normalCrc);
        CHECK(dwgUtil::crc64Mirrored(0, bytes.data(), vector.size)
              == vector.mirroredCrc);
    }
}

TEST_CASE("DWG R2007 compressed header CRC matches an external fixture",
          "[dwg-write][r2007][crc]") {
    // Unmodified libreDWG test/test-data/2007/Line.dwg: the first protected
    // header block contains these 163 bytes and stores CRC 3EB32C14C27D9D50.
    const std::array<std::uint8_t, 163> compressed = {
        0x20, 0x00, 0x00, 0x02, 0x00, 0x70, 0x60, 0x03, 0x01, 0x24, 0x80, 0x58,
        0x00, 0x01, 0x9a, 0xa8, 0x84, 0x61, 0xc9, 0x14, 0x19, 0x07, 0x54, 0x77,
        0x08, 0x02, 0x04, 0x96, 0x31, 0x15, 0x00, 0x44, 0xda, 0x05, 0x00, 0x88,
        0x60, 0x09, 0x18, 0x77, 0x08, 0x86, 0x01, 0x17, 0x88, 0x01, 0x1c, 0x6f,
        0x19, 0x72, 0x7f, 0x01, 0x40, 0x6e, 0x02, 0x14, 0x00, 0x7f, 0x00, 0x87,
        0x19, 0x20, 0x8f, 0x08, 0x77, 0x00, 0x02, 0xf8, 0xa7, 0x23, 0xc1, 0x40,
        0x81, 0xa6, 0x0b, 0x00, 0x1a, 0x60, 0x08, 0x76, 0x31, 0x00, 0x7e, 0x1a,
        0x0e, 0x00, 0x77, 0x10, 0x02, 0x03, 0xd9, 0xc4, 0x23, 0x69, 0x3d, 0xd2,
        0x5a, 0x8c, 0x50, 0x60, 0x08, 0x87, 0x29, 0x13, 0x7f, 0x0a, 0x08, 0x8c,
        0x68, 0x00, 0x00, 0x92, 0xa4, 0x14, 0x81, 0xdf, 0x56, 0x3e, 0x9c, 0x87,
        0x20, 0x03, 0x06, 0x34, 0xdf, 0x43, 0x29, 0xc6, 0xc0, 0x00, 0x01, 0x06,
        0xc6, 0x79, 0x10, 0x66, 0x00, 0x0f, 0x01, 0x08, 0xc3, 0x42, 0x66, 0xf2,
        0xf5, 0xf4, 0x7f, 0x4d, 0x95, 0xe2, 0xed, 0xde, 0x5a, 0x7f, 0xa8, 0x95,
        0x13, 0x31, 0x56, 0x21, 0x19, 0xdf, 0x21
    };
    CHECK(dwgUtil::crc64Normal(dwgUtil::updateSeed2(0, compressed.size()),
                               compressed.data(), compressed.size())
          == 0x3eb32c14c27d9d50ULL);
}

TEST_CASE("DWG entity encoding propagates frame finalization failure",
          "[dwg-write][safety]") {
    DRW_Header header;
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_failing_writer.dwg";
    std::filesystem::remove(path);
    std::ofstream stream(path,
                         std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());

    FailingDwgWriter writer(&stream, &header);
    DRW_Line line;
    line.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    line.secPoint = DRW_Coord{1.0, 1.0, 0.0};
    CHECK_FALSE(writer.encodeEntity(&line));
    stream.close();
    std::filesystem::remove(path);
}

TEST_CASE("DWG writers finalize HANDSEED after entity allocation",
          "[dwg-write][header][handseed][crc]") {
    const DRW::Version versions[] = {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032
    };

    for (const DRW::Version version : versions) {
        CAPTURE(version);
        const std::filesystem::path path =
            std::filesystem::temp_directory_path()
            / ("libdxfrw_handseed_" + std::to_string(static_cast<int>(version))
               + ".dwg");
        std::filesystem::remove(path);

        DRW_Header header;
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        REQUIRE(stream.good());
        std::unique_ptr<dwgWriter> writer = makeDwgWriter(version, &stream, &header);
        REQUIRE(writer != nullptr);
        REQUIRE(writer->prepareDwgClassManifest());
        REQUIRE(writer->writeFileHeaderStub());
        REQUIRE(writer->writeDwgHeader());
        REQUIRE(writer->writeDwgClasses());
        REQUIRE(writer->writeDwgObjects());

        DRW_Line line;
        line.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        line.secPoint = DRW_Coord{100.0, 100.0, 0.0};
        REQUIRE(writer->encodeEntity(&line));
        REQUIRE(line.handle != 0);
        REQUIRE(writer->finalizeHeaderHandseed());
        CHECK(header.getHandSeed() > line.handle);
        // Check the framed HEADER before modern finalize() compresses it.
        const auto& bytes = writer->buffer();
        const auto startIt = std::search(bytes.cbegin(), bytes.cend(),
            dwgSentinels::HEADER_BEGIN, dwgSentinels::HEADER_BEGIN + 16);
        REQUIRE(startIt != bytes.cend());
        const std::size_t start = static_cast<std::size_t>(startIt - bytes.cbegin());
        const std::size_t highSizeBytes = version >= DRW::AC1027 ? 4u : 0u;
        REQUIRE(bytes.size() - start >= 38u + highSizeBytes);
        const std::size_t payloadSize = readLittleEndian32(bytes.data() + start + 16u);
        REQUIRE(payloadSize <= bytes.size() - start - 38u - highSizeBytes);
        const std::size_t crcOffset = start + 20u + highSizeBytes + payloadSize;
        std::uint16_t crc = 0xC0C1;
        for (std::size_t offset = start + 16u; offset < crcOffset; ++offset) {
            crc ^= bytes[offset];
            for (unsigned int bit = 0; bit < 8; ++bit)
                crc = static_cast<std::uint16_t>(
                    (crc >> 1) ^ ((crc & 1u) != 0 ? 0xA001u : 0u));
        }
        const std::uint16_t stored = static_cast<std::uint16_t>(bytes[crcOffset])
            | static_cast<std::uint16_t>(bytes[crcOffset + 1u] << 8);
        CHECK(stored == crc);
        const auto finalizedBytes = bytes;
        REQUIRE(writer->finalizeHeaderHandseed());
        CHECK(writer->buffer() == finalizedBytes);
        REQUIRE(writer->writeDwgHandles());
        REQUIRE(writer->writeSecondHeader());
        REQUIRE(writer->finalize());
        writer.reset();
        stream.close();
        std::filesystem::remove(path);
    }
}

TEST_CASE("DWG HANDSEED follows the emitted object map",
          "[dwg-write][header][handseed][safety]") {
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / "libdxfrw_handseed_object_map.dwg";
    std::filesystem::remove(path);

    DRW_Header header;
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    ManualHandleDwgWriter15 writer(&stream, &header);
    REQUIRE(writer.prepareDwgClassManifest());
    REQUIRE(writer.writeFileHeaderStub());
    REQUIRE(writer.writeDwgHeader());
    REQUIRE(writer.writeDwgClasses());
    REQUIRE(writer.writeDwgObjects());

    constexpr std::uint32_t manuallyAssignedHandle = 0x100000u;
    writer.emitManualObject(manuallyAssignedHandle);
    REQUIRE(writer.finalizeHeaderHandseed());
    CHECK(header.getHandSeed() > manuallyAssignedHandle);
    REQUIRE(writer.writeDwgHandles());
    REQUIRE(writer.writeSecondHeader());
    REQUIRE(writer.finalize());
    stream.close();
    std::filesystem::remove(path);
}

TEST_CASE("DWG CLASSES sentinel precedes the optional unknown tail",
          "[dwg-write][classes][safety]") {
    const DRW::Version versions[] = {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032
    };
    for (const DRW::Version version : versions) {
        CAPTURE(version);
        DRW_Header header;
        auto writer = makeDwgWriter(version, nullptr, &header);
        REQUIRE(writer != nullptr);
        REQUIRE(writer->prepareDwgClassManifest());
        REQUIRE(writer->writeDwgClasses());
        const auto& bytes = writer->buffer();
        const std::size_t highSizeBytes = version >= DRW::AC1027 ? 4u : 0u;
        REQUIRE(bytes.size() >= 38u + highSizeBytes);
        const std::size_t tailSize = version == DRW::AC1015
            ? 0u : dwgSpec::kClassesUnknownTailBytes;
        const std::size_t payloadSize = readLittleEndian32(bytes.data() + 16u);
        REQUIRE(payloadSize <= bytes.size() - 38u - highSizeBytes);
        const std::size_t endSentinel = 20u + highSizeBytes + payloadSize + 2u;
        REQUIRE(endSentinel + 16u + tailSize == bytes.size());
        CHECK(std::equal(dwgSentinels::CLASSES_END,
                         dwgSentinels::CLASSES_END + 16,
                         bytes.cbegin() + static_cast<std::ptrdiff_t>(endSentinel)));
        CHECK(std::all_of(bytes.cend() - static_cast<std::ptrdiff_t>(tailSize),
                          bytes.cend(), [](std::uint8_t byte) { return byte == 0; }));
    }
}

TEST_CASE("DWG table control sizes exclude their trailing handles",
          "[dwg-write][header][control][safety]") {
    const DRW::Version versions[] = {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032
    };
    const std::int16_t controlTypes[] = {
        DRW::DwgBlockControlObjectType, DRW::DwgLayerControlObjectType,
        DRW::DwgStyleControlObjectType, DRW::DwgLTypeControlObjectType,
        DRW::DwgViewControlObjectType, DRW::DwgUcsControlObjectType,
        DRW::DwgVPortControlObjectType, DRW::DwgAppIdControlObjectType,
        DRW::DwgDimStyleControlObjectType,
        DRW_ViewportEntityHeader::kDwgControlType
    };
    for (const DRW::Version version : versions) {
        CAPTURE(version);
        DRW_Header header;
        auto writer = makeDwgWriter(version, nullptr, &header);
        REQUIRE(writer != nullptr);
        REQUIRE(writer->prepareDwgClassManifest());
        REQUIRE(writer->writeDwgObjects());
        const std::uint32_t blockHandle = writer->defineBlock("Named", DRW_Coord{});
        REQUIRE(blockHandle != 0u);
        REQUIRE(writer->emitDeferredBlockControl());

        const auto& bytes = writer->buffer();
        dwgBuffer stream(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        if (version >= DRW::AC1018)
            REQUIRE(stream.getRawLong32() == 0x0dcau);
        std::size_t controlsRead = 0;
        while (stream.getPosition() < bytes.size()) {
            DwgObjectFrame frame;
            REQUIRE(frame.readAt(stream, version, stream.getPosition()));
            dwgBuffer body(frame.body().data(), frame.body().size());
            const std::int16_t type = body.getObjType(version);
            if (std::find(std::begin(controlTypes), std::end(controlTypes), type)
                    == std::end(controlTypes))
                continue;
            CAPTURE(type);
            const std::uint32_t dataBits = version < DRW::AC1024
                ? body.getRawLong32() : frame.objectDataBitSize();
            const std::uint32_t ownHandle = body.getHandle().ref;
            REQUIRE(body.isGood());
            REQUIRE(dataBits + 16u <= frame.body().size() * 8u);
            // Independent of parseDwg's legacy inline cursor: seek exactly
            // as external readers do, using objSize (or the R2010+ MC prefix).
            REQUIRE(body.setPosition(dataBits >> 3));
            body.setBitPos(static_cast<std::uint8_t>(dataBits & 7u));
            const dwgHandle owner = body.getHandle();
            CHECK(owner.code == 4u);
            CHECK(owner.ref == 0u);
            const dwgHandle dictionary = body.getHandle();
            CHECK(dictionary.code == 3u);
            CHECK(dictionary.ref == 0u);
            if (type == DRW::DwgBlockControlObjectType) {
                const dwgHandle child = body.getHandle();
                CHECK(child.code == 2u);
                CHECK(child.ref == blockHandle);
                const dwgHandle model = body.getHandle();
                CHECK(model.code == 3u);
                CHECK(model.ref == DRW::DwgModelSpaceBlockRecordHandle);
                const dwgHandle paper = body.getHandle();
                CHECK(paper.code == 3u);
                CHECK(paper.ref == DRW::DwgPaperSpaceBlockRecordHandle);
            }
            REQUIRE(body.isGood());
            body.resetPosition();
            DRW_ObjControl control;
            REQUIRE(control.parseDwg(version, &body, frame.bodyBitSize()));
            CHECK(control.handle == ownHandle);
            if (type == DRW::DwgBlockControlObjectType) {
                CHECK(control.handlesList.size() == 3u);
                CHECK(control.handlesList.front() == blockHandle);
                CHECK(control.phantomHandles.size() == 2u);
            }
            ++controlsRead;
        }
        CHECK(controlsRead == (version == DRW::AC1015 ? 10u : 9u));
    }
}

TEST_CASE("DWG writer closes block ownership before late entity writes",
          "[dwg-write][ownership][phase]") {
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / "libdxfrw_closed_block_ownership.dwg";
    std::filesystem::remove(path);

    DRW_Header header;
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    dwgWriter21 writer(&stream, &header);
    REQUIRE(writer.prepareDwgClassManifest());
    REQUIRE(writer.writeFileHeaderStub());
    REQUIRE(writer.writeDwgHeader());
    REQUIRE(writer.writeDwgClasses());
    REQUIRE(writer.writeDwgObjects());
    REQUIRE(writer.emitDeferredBlockControl());

    CHECK(writer.defineBlock("LateBlock", DRW_Coord{}) == 0u);
    DRW_Line lateLine;
    lateLine.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    lateLine.secPoint = DRW_Coord{1.0, 1.0, 0.0};
    CHECK_FALSE(writer.encodeEntity(&lateLine));

    stream.close();
    std::filesystem::remove(path);
}

TEST_CASE("DWG HANDSEED patch preserves the bit-packed header stream",
          "[dwg-write][header][handseed]") {
    constexpr std::uint32_t handseed = 0x12345678u;
    DRW_Header src;
    dwgBufferW encoded;
    encoded.putRawLong32(0);
    REQUIRE(DwgHandseedTestAccess::encode(src, DRW::AC1015,
                                           &encoded, &encoded));

    const std::uint8_t bytes[] = {
        static_cast<std::uint8_t>(handseed >> 24),
        static_cast<std::uint8_t>(handseed >> 16),
        static_cast<std::uint8_t>(handseed >> 8),
        static_cast<std::uint8_t>(handseed)
    };
    constexpr std::size_t bodyStartBit = 4u * 8u;
    REQUIRE(encoded.patchRawBytesAtBit(bodyStartBit
                                           + src.dwgHandseedBitOffset(), bytes,
                                       sizeof(bytes)));
    encoded.alignToByte();
    encoded.patchRawLong32(0,
                           static_cast<std::uint32_t>(encoded.size() - 4));

    dwgBuffer decoded(encoded.data().data(), encoded.size());
    DRW_Header dst;
    REQUIRE(DwgHandseedTestAccess::parse(dst, DRW::AC1015,
                                          &decoded, &decoded));
    CHECK(dst.getHandSeed() == handseed);
}

TEST_CASE("DWG R2007 writer duplicates the complete file header page",
          "[dwg-write][r2007][container]") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "libdxfrw_r2007_header.dwg";
    std::filesystem::remove(path);

    DRW_Header header;
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    REQUIRE(stream.good());
    std::unique_ptr<dwgWriter> writer =
        makeDwgWriter(DRW::AC1021, &stream, &header);
    REQUIRE(writer != nullptr);
    REQUIRE(writer->prepareDwgClassManifest());
    REQUIRE(writer->writeFileHeaderStub());
    REQUIRE(writer->writeDwgHeader());
    REQUIRE(writer->writeDwgClasses());
    REQUIRE(writer->writeDwgObjects());
    REQUIRE(writer->finalizeHeaderHandseed());
    REQUIRE(writer->writeDwgHandles());
    REQUIRE(writer->writeSecondHeader());
    REQUIRE(writer->finalize());
    writer.reset();
    stream.close();

    std::vector<std::uint8_t> bytes;
    {
        std::ifstream input(path, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
    }
    constexpr std::size_t headerOffset = 0x80;
    constexpr std::size_t headerPageSize = 0x400;
    REQUIRE(bytes.size() >= headerOffset + headerPageSize * 2u);
    CHECK(readLittleEndian32(bytes.data() + 0x28) == headerOffset);
    CHECK(std::equal(bytes.cbegin() + static_cast<std::ptrdiff_t>(headerOffset),
                     bytes.cbegin() + static_cast<std::ptrdiff_t>(
                         headerOffset + headerPageSize),
                     bytes.cend() - static_cast<std::ptrdiff_t>(headerPageSize)));

    std::array<std::uint8_t, 3u * 255u> encoded{};
    std::copy_n(bytes.cbegin() + static_cast<std::ptrdiff_t>(headerOffset),
                encoded.size(), encoded.begin());
    std::array<std::uint8_t, 3u * 239u> decoded{};
    REQUIRE(dwgRSCodec::decode239I(encoded.data(), decoded.data(), 3));
    CHECK(readLittleEndian64(decoded.data() + 40) == bytes.size());
    CHECK(readLittleEndian32(decoded.data() + 24) == 0xfffffef0U);
    CHECK(readLittleEndian32(decoded.data() + 28) == 0xffffffffU);
    CHECK(readLittleEndian64(decoded.data() + 24)
          == static_cast<std::uint64_t>(-std::int64_t{0x110}));
    constexpr std::size_t blockSize = 32u + 0x110u;
    static_assert(2u * blockSize <= 3u * 239u);
    CHECK(std::equal(decoded.cbegin(), decoded.cbegin() + blockSize,
                     decoded.cbegin() + blockSize));
    CHECK(dwgUtil::crc64Normal(
              dwgUtil::updateSeed2(0, 0x110), decoded.data() + 32, 0x110)
          == readLittleEndian64(decoded.data() + 16));

    std::array<std::uint8_t, 16> sequence{};
    const std::uint64_t sequenceFirst = readLittleEndian64(decoded.data() + 8);
    writeLittleEndian64(sequence.data(), sequenceFirst);
    writeLittleEndian64(sequence.data() + 8,
                         rotateR2007CheckValue(sequenceFirst, sequenceFirst));
    CHECK(dwgUtil::crc64Normal(dwgUtil::updateSeed1(0, sequence.size()),
                               sequence.data(), sequence.size())
          == readLittleEndian64(decoded.data()));

    std::array<std::uint8_t, 0x110> headerData{};
    std::copy_n(decoded.cbegin() + 32, headerData.size(), headerData.begin());
    const std::uint64_t headerCrc = readLittleEndian64(headerData.data() + 264);
    std::fill(headerData.begin() + 264, headerData.begin() + 272, 0);
    CHECK(dwgUtil::crc64Normal(dwgUtil::updateSeed2(0, headerData.size()),
                               headerData.data(), headerData.size())
          == headerCrc);
    CHECK(dwgUtil::decodeCrcSeed(readLittleEndian64(headerData.data() + 32))
          == 0U);
    CHECK(dwgUtil::decodeCrcSeed(readLittleEndian64(headerData.data() + 224))
          == 0U);
    CHECK(dwgUtil::decodeCrcSeed(readLittleEndian64(headerData.data() + 248))
          == 0U);
    CHECK(readLittleEndian64(headerData.data() + 240) == 0U);
    CHECK(readLittleEndian64(headerData.data() + 256) == 0U);
    CHECK(readLittleEndian64(headerData.data()) == 0x70U);
    CHECK(readLittleEndian64(headerData.data() + 72)
          == bytes.size() - headerPageSize - 0x480u);

    const std::uint64_t pageMapCompressedSize =
        readLittleEndian64(headerData.data() + 80);
    const std::uint64_t pageMapCorrectionFactor =
        readLittleEndian64(headerData.data() + 24);
    const std::uint64_t pageMapSecondOffset =
        readLittleEndian64(headerData.data() + 40);
    REQUIRE(pageMapCompressedSize != 0U);
    REQUIRE(pageMapCorrectionFactor >= 2U);
    CHECK(pageMapSecondOffset == 0x400U);
    const std::uint64_t pageMapAlignedSize = (pageMapCompressedSize + 7U) & ~7U;
    const std::uint64_t pageMapBlocks =
        (pageMapAlignedSize * pageMapCorrectionFactor + 238U) / 239U;
    REQUIRE(pageMapBlocks * 255U <= pageMapSecondOffset);
    std::vector<std::uint8_t> encodedPageMap(
        static_cast<std::size_t>(pageMapBlocks * 255U));
    std::copy_n(bytes.cbegin() + 0x480, encodedPageMap.size(),
                encodedPageMap.begin());
    std::vector<std::uint8_t> decodedPageMap(
        static_cast<std::size_t>(pageMapBlocks * 239U));
    REQUIRE(dwgRSCodec::decode239I(encodedPageMap.data(), decodedPageMap.data(),
                                    static_cast<std::uint32_t>(pageMapBlocks)));
    CHECK(dwgUtil::crc64Mirrored(
              dwgUtil::updateSeed1(0, pageMapCompressedSize),
              decodedPageMap.data(), pageMapCompressedSize)
          == readLittleEndian64(headerData.data() + 16));
    CHECK(readLittleEndian64(headerData.data() + 128)
          == readLittleEndian64(headerData.data() + 16));
    CHECK(readLittleEndian64(decodedPageMap.data()) == 0x400U);
    CHECK(readLittleEndian64(decodedPageMap.data() + 8) == 1U);

    const std::uint64_t sectionMapId = readLittleEndian64(headerData.data() + 192);
    const std::uint64_t pageCount = readLittleEndian64(headerData.data() + 96);
    REQUIRE(pageMapCompressedSize >= pageCount * 16U);
    std::uint64_t pageAddress = 0x480;
    std::uint64_t sectionMapAddress = 0;
    std::map<std::uint64_t, std::uint64_t> pageAddresses;
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i * 16U);
        const std::uint64_t pageSize = readLittleEndian64(
            decodedPageMap.data() + offset);
        const std::uint64_t pageId = readLittleEndian64(
            decodedPageMap.data() + offset + 8);
        if (pageId == sectionMapId)
            sectionMapAddress = pageAddress;
        REQUIRE(pageAddresses.emplace(pageId, pageAddress).second);
        pageAddress += pageSize;
    }
    CHECK(pageAddress == bytes.size() - headerPageSize);
    REQUIRE(sectionMapAddress != 0U);

    const std::uint64_t sectionMapCompressedSize =
        readLittleEndian64(headerData.data() + 176);
    const std::uint64_t sectionMapCorrectionFactor =
        readLittleEndian64(headerData.data() + 216);
    const std::uint64_t sectionMapAlignedSize =
        (sectionMapCompressedSize + 7U) & ~7U;
    const std::uint64_t sectionMapBlocks =
        (sectionMapAlignedSize * sectionMapCorrectionFactor + 238U) / 239U;
    REQUIRE(sectionMapCorrectionFactor >= 2U);
    REQUIRE(sectionMapBlocks * 255U <= bytes.size() - sectionMapAddress);
    std::vector<std::uint8_t> encodedSectionMap(
        static_cast<std::size_t>(sectionMapBlocks * 255U));
    std::copy_n(bytes.cbegin() + static_cast<std::ptrdiff_t>(sectionMapAddress),
                encodedSectionMap.size(), encodedSectionMap.begin());
    std::vector<std::uint8_t> decodedSectionMap(
        static_cast<std::size_t>(sectionMapBlocks * 239U));
    REQUIRE(dwgRSCodec::decode239I(encodedSectionMap.data(),
                                    decodedSectionMap.data(),
                                    static_cast<std::uint32_t>(sectionMapBlocks)));
    CHECK(dwgUtil::crc64Mirrored(
              dwgUtil::updateSeed1(0, sectionMapCompressedSize),
              decodedSectionMap.data(), sectionMapCompressedSize)
          == readLittleEndian64(headerData.data() + 208));

    const std::string expectedSections[] = {
        "AcDb:AppInfoHistory",
        "AcDb:AppInfo",
        "AcDb:RevHistory",
        "AcDb:AcDbObjects",
        "AcDb:ObjFreeSpace",
        "AcDb:Template",
        "AcDb:Handles",
        "AcDb:Classes",
        "AcDb:AuxHeader",
        "AcDb:Header",
    };
    std::size_t sectionOffset = 0;
    for (const std::string& expectedName : expectedSections) {
        REQUIRE(sectionOffset <= sectionMapCompressedSize);
        REQUIRE(sectionMapCompressedSize - sectionOffset >= 64u);
        const std::uint8_t* descriptor = decodedSectionMap.data()
            + static_cast<std::ptrdiff_t>(sectionOffset);
        const std::uint64_t nameSize = readLittleEndian64(descriptor + 32);
        const std::uint64_t pageCount = readLittleEndian64(descriptor + 56);
        REQUIRE(nameSize <= sectionMapCompressedSize - sectionOffset - 64u);
        REQUIRE(pageCount <=
                (sectionMapCompressedSize - sectionOffset - 64u - nameSize) / 56u);
        REQUIRE(nameSize == (expectedName.size() + 1u) * 2u);
        CHECK(descriptor[64u + nameSize - 2u] == 0u);
        CHECK(descriptor[64u + nameSize - 1u] == 0u);
        std::string actualName;
        actualName.reserve(expectedName.size());
        for (std::size_t i = 0; i + 2u < nameSize; i += 2u) {
            REQUIRE(descriptor[64u + i + 1u] == 0u);
            actualName.push_back(static_cast<char>(descriptor[64u + i]));
        }
        CHECK(actualName == expectedName);
        CHECK(readLittleEndian64(descriptor + 24)
              == expectedR2007SectionHash(actualName));
        if (actualName == "AcDb:AppInfo") {
            REQUIRE(pageCount != 0u);
            const std::uint64_t firstPageId = readLittleEndian64(
                descriptor + 64u + nameSize + 16u);
            REQUIRE(pageAddresses.count(firstPageId) == 1u);
            CHECK(readLittleEndian32(bytes.data() + 0x2c)
                  == pageAddresses.at(firstPageId));
        }
        sectionOffset += 64u + static_cast<std::size_t>(nameSize)
            + static_cast<std::size_t>(pageCount) * 56u;
    }

    constexpr std::size_t checkDataOffset = headerOffset + 0x3d8;
    const std::uint64_t normalCrc = readLittleEndian64(bytes.data() + checkDataOffset);
    const std::uint64_t random1 = readLittleEndian64(bytes.data() + checkDataOffset + 16);
    const std::uint64_t random2 = readLittleEndian64(bytes.data() + checkDataOffset + 24);
    CHECK(dwgUtil::decodeCrcSeed(
              readLittleEndian64(bytes.data() + checkDataOffset + 32)) == 0U);
    CHECK(normalCrc == r2007CheckNormalCrc(random1, random2));
    CHECK(readLittleEndian64(bytes.data() + checkDataOffset + 8)
          == r2007CheckMirroredCrc(random1, random2, normalCrc));
    std::filesystem::remove(path);
}
