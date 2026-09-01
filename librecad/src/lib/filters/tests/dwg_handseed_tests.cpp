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
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
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
          "[dwg-write][header][handseed]") {
    const DRW::Version versions[] = {
        DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032
    };

    for (const DRW::Version version : versions) {
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

    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    constexpr std::size_t headerOffset = 0x80;
    constexpr std::size_t headerPageSize = 0x400;
    REQUIRE(bytes.size() >= headerOffset + headerPageSize * 2u);
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
    CHECK(readLittleEndian32(decoded.data() + 28) == 0U);
    CHECK(dwgUtil::crc64Normal(
              dwgUtil::updateSeed1(0, 0x110), decoded.data() + 32, 0x110)
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
    for (std::uint64_t i = 0; i < pageCount; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i * 16U);
        const std::uint64_t pageSize = readLittleEndian64(
            decodedPageMap.data() + offset);
        const std::uint64_t pageId = readLittleEndian64(
            decodedPageMap.data() + offset + 8);
        if (pageId == sectionMapId)
            sectionMapAddress = pageAddress;
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
        REQUIRE(nameSize == expectedName.size() * 2u);
        std::string actualName;
        actualName.reserve(expectedName.size());
        for (std::size_t i = 0; i < nameSize; i += 2u) {
            REQUIRE(descriptor[64u + i + 1u] == 0u);
            actualName.push_back(static_cast<char>(descriptor[64u + i]));
        }
        CHECK(actualName == expectedName);
        CHECK(readLittleEndian64(descriptor + 24)
              == expectedR2007SectionHash(actualName));
        sectionOffset += 64u + static_cast<std::size_t>(nameSize)
            + static_cast<std::size_t>(pageCount) * 56u;
    }

    constexpr std::size_t checkDataOffset = headerOffset + 0x3d8;
    const std::uint64_t random1 = readLittleEndian64(bytes.data() + checkDataOffset);
    const std::uint64_t random2 = readLittleEndian64(bytes.data() + checkDataOffset + 8);
    const std::uint64_t normalCrc = readLittleEndian64(bytes.data() + checkDataOffset + 24);
    CHECK(dwgUtil::decodeCrcSeed(
              readLittleEndian64(bytes.data() + checkDataOffset + 16)) == 0U);
    CHECK(normalCrc == r2007CheckNormalCrc(random1, random2));
    CHECK(readLittleEndian64(bytes.data() + checkDataOffset + 32)
          == r2007CheckMirroredCrc(random1, random2, normalCrc));
    std::filesystem::remove(path);
}
