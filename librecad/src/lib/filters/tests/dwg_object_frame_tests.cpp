/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published
** by the Free Software Foundation; either version 2 or later.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include "intern/dwgobjectframe.h"

namespace {

std::vector<std::uint8_t> makeFrame(std::uint8_t bodyByte,
                                    DRW::Version version,
                                    bool validCrc = true,
                                    std::uint8_t handleStreamBits = 0) {
    const bool hasBitSize = version > DRW::AC1021;
    const std::size_t bodyOffset = hasBitSize ? 3 : 2;
    std::vector<std::uint8_t> bytes(bodyOffset + 1 + 2, 0);
    bytes[0] = 1;
    bytes[1] = 0;
    if (hasBitSize)
        bytes[2] = handleStreamBits;
    bytes[bodyOffset] = bodyByte;

    dwgBuffer crcBuffer(bytes.data(), bodyOffset + 1);
    const std::uint16_t crc = crcBuffer.crc8(
        0xC0C1, 0, static_cast<std::int32_t>(bodyOffset + 1));
    bytes[bodyOffset + 1] = static_cast<std::uint8_t>(crc & 0xFF);
    bytes[bodyOffset + 2] = static_cast<std::uint8_t>(crc >> 8);
    if (!validCrc)
        bytes[bodyOffset + 1] ^= 0xFF;
    return bytes;
}

} // namespace

TEST_CASE("DWG object frame derives body and preserves R2010 bit sizes",
          "[dwg][frame]") {
    auto bytes = makeFrame(0x5A, DRW::AC1024, true, 4);
    dwgBuffer buffer(bytes.data(), bytes.size());
    DwgObjectFrame frame;

    REQUIRE(frame.readAt(buffer, DRW::AC1024, 0));
    REQUIRE(frame.bodyBitSize() == 4);
    REQUIRE(frame.objectDataBitSize() == 4);
    REQUIRE(frame.handleStreamBitSize() == 4);
    REQUIRE(frame.body().size() == 1);
    CHECK(frame.body()[0] == 0x5A);
    CHECK(buffer.getPosition() == bytes.size());
}

TEST_CASE("DWG object frame rejects truncation and bad CRC transactionally",
          "[dwg][frame]") {
    const auto valid = makeFrame(0x01, DRW::AC1015);
    DwgObjectFrame frame;

    auto truncated = valid;
    truncated.pop_back();
    dwgBuffer truncatedBuffer(truncated.data(), truncated.size());
    CHECK_FALSE(frame.readAt(truncatedBuffer, DRW::AC1015, 0));
    CHECK(truncatedBuffer.getPosition() == 0);
    CHECK(frame.body().empty());

    auto corrupt = makeFrame(0x01, DRW::AC1015, false);
    dwgBuffer corruptBuffer(corrupt.data(), corrupt.size());
    REQUIRE(corruptBuffer.setPosition(1));
    CHECK_FALSE(frame.readAt(corruptBuffer, DRW::AC1015, 0));
    CHECK(corruptBuffer.isGood());
    CHECK(corruptBuffer.getPosition() == 0);
    CHECK(frame.body().empty());
}

TEST_CASE("DWG object frame failure does not poison the source cursor",
          "[dwg][frame]") {
    auto bytes = makeFrame(0x2A, DRW::AC1015, false);
    dwgBuffer buffer(bytes.data(), bytes.size());
    DwgObjectFrame frame;

    REQUIRE_FALSE(frame.readAt(buffer, DRW::AC1015, 0));
    CHECK(buffer.isGood());
    CHECK(buffer.getPosition() == 0);

    const std::size_t bodyOffset = 2;
    const std::uint16_t crc = buffer.crc8(
        0xC0C1, 0, static_cast<std::int32_t>(bodyOffset + 1));
    bytes[bodyOffset + 1] = static_cast<std::uint8_t>(crc & 0xFF);
    bytes[bodyOffset + 2] = static_cast<std::uint8_t>(crc >> 8);

    REQUIRE(frame.readAt(buffer, DRW::AC1015, 0));
    CHECK(frame.body().front() == 0x2A);
    CHECK(buffer.getPosition() == bytes.size());
}

TEST_CASE("DWG object frame restores the requested offset on early failure",
          "[dwg][frame]") {
    std::vector<std::uint8_t> malformed{0x80};
    dwgBuffer buffer(malformed.data(), malformed.size());
    REQUIRE(buffer.setPosition(1));
    DwgObjectFrame frame;

    CHECK_FALSE(frame.readAt(buffer, DRW::AC1015, 0));
    CHECK(buffer.isGood());
    CHECK(buffer.getPosition() == 0);
}

TEST_CASE("DWG R2010 object frame rejects an oversized handle bit count",
          "[dwg][frame]") {
    auto bytes = makeFrame(0x01, DRW::AC1024, true, 9);
    dwgBuffer buffer(bytes.data(), bytes.size());
    DwgObjectFrame frame;

    CHECK_FALSE(frame.readAt(buffer, DRW::AC1024, 0));
    CHECK(buffer.getPosition() == 0);
    CHECK(frame.body().empty());
}
