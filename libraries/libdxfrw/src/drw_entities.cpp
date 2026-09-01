/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>
#include "drw_entities.h"
#include "intern/dxfreader.h"
#include "intern/dxfparserlimits.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/drw_dbg.h"
#include "intern/drw_reserve.h"
#include "intern/dwgreader.h"
#include "intern/dwgsafety.h"

namespace {

struct EmbeddedMTextHandleInfo {
    bool m_ownerHandle = false;
    int m_numReactors = 0;
    // AC1015 has no xdict bit; AC1018+ supplies it and uses 1 to suppress
    // the embedded xdictionary handle.
    std::uint8_t m_xDictFlag = 0;
    bool m_hasAcDbColorHandle = false;
    int m_ltFlags = 0;
    int m_plotFlags = 0;
    int m_materialFlag = 0;
    int m_shadowFlag = 0;
    bool m_hasFullVisualStyle = false;
    bool m_hasFaceVisualStyle = false;
    bool m_hasEdgeVisualStyle = false;
    bool m_hasStyleHandle = true;
    bool m_hasR2018AppIdHandle = false;
    bool m_hasAnnotativeAppHandle = false;
};
static bool parseEmbeddedMTextDwg(DRW::Version version, dwgBuffer *buf,
                                  dwgBuffer *sBuf, DRW_MText& mtext,
                                  EmbeddedMTextHandleInfo& info,
                                  std::uint64_t bodyEndBit =
                                      std::numeric_limits<std::uint64_t>::max(),
                                  std::uint64_t stringEndBit =
                                      std::numeric_limits<std::uint64_t>::max());
static bool consumeEmbeddedMTextHandles(
    DRW::Version version, dwgBuffer *buf,
    const EmbeddedMTextHandleInfo& info, DRW_MText *mtext,
    std::uint64_t handleEndBit =
        std::numeric_limits<std::uint64_t>::max());

constexpr std::uint32_t kMaxTableRows = 10000;
constexpr std::uint32_t kMaxTableColumns = 1000;
constexpr std::uint32_t kMaxTableCells = 200000;
constexpr std::uint32_t kMaxTableItems = 100000;
constexpr std::uint32_t kMaxTableStringBytes = 16 * 1024 * 1024;
constexpr std::uint32_t kMaxTableCellAttributes = 10000;
constexpr std::int32_t kMaxLWPolylineVertices = 1000000;
constexpr std::int32_t kMaxSplineItems = 1000000;
constexpr std::int32_t kMaxSplineDegree = 1024;
constexpr std::int32_t kMaxHatchItems = DRW_Hatch::kMaxDxfItems;
constexpr std::int32_t kMaxMInsertCount =
    std::numeric_limits<std::int16_t>::max();
constexpr std::int32_t kMaxViewportFrozenLayers = 512;
constexpr std::int32_t kMaxMTextColumnHeights = 4096;
constexpr std::int32_t kMaxMLeaderItems = 5000;
constexpr std::int32_t kMaxLeaderVertices = 5000;
constexpr std::uint16_t kMaxLoftedSurfaceModelerFormatVersion = 3;

constexpr std::int32_t kSplineFlagMethodFitPoints = 1;
constexpr std::int32_t kSplineFlagClosed = 4;
constexpr std::int32_t kSplineFlagUseKnotParameter = 8;
constexpr std::int32_t kSplineKnotParamCustom = 15;

bool isValidCount(std::int32_t count, std::int32_t maxCount) {
    return count >= 0 && count <= maxCount;
}

bool readDxfIntInRange(const std::unique_ptr<dxfReader>& reader,
                       int minimum, int maximum, int& value) {
    value = reader->getInt32();
    return value >= minimum && value <= maximum;
}

int hexNibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

bool decodeHexBytes(const std::string& hex, std::vector<std::uint8_t>& out) {
    if ((hex.size() % 2) != 0)
        return false;

    std::vector<std::uint8_t> decoded;
    const std::size_t byteCount = hex.size() / 2;
    if (byteCount > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(decoded, static_cast<int>(byteCount)))
        return false;
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = hexNibble(hex[i]);
        const int lo = hexNibble(hex[i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        decoded.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }

    out = std::move(decoded);
    return true;
}

bool appendBytesChecked(std::vector<std::uint8_t>& destination,
                        const std::vector<std::uint8_t>& bytes,
                        std::size_t maxSize) {
    if (destination.size() > maxSize
        || bytes.size() > maxSize - destination.size())
        return false;
    const std::size_t newSize = destination.size() + bytes.size();
    if (newSize > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(destination, static_cast<int>(newSize)))
        return false;
    try {
        destination.insert(destination.end(), bytes.begin(), bytes.end());
    } catch (...) {
        return false;
    }
    return true;
}

bool appendTextBytesChecked(std::vector<std::uint8_t>& destination,
                            const std::string& text,
                            std::size_t maxSize) {
    if (destination.size() > maxSize
        || text.size() > maxSize - destination.size())
        return false;
    const std::size_t newSize = destination.size() + text.size();
    if (newSize > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(destination, static_cast<int>(newSize)))
        return false;
    try {
        destination.insert(destination.end(), text.begin(), text.end());
    } catch (...) {
        return false;
    }
    return true;
}

bool appendHexBytesChecked(std::string& destination, const std::string& hex,
                           std::size_t maxSize) {
    if ((hex.size() & 1u) != 0u || destination.size() > maxSize)
        return false;
    const std::size_t byteCount = hex.size() / 2u;
    if (byteCount > maxSize - destination.size()
        || byteCount > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    std::string decoded;
    if (!DRW::reserve(decoded, static_cast<int>(byteCount)))
        return false;
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = hexNibble(hex[i]);
        const int lo = hexNibble(hex[i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        decoded.push_back(static_cast<char>((hi << 4) | lo));
    }
    try {
        destination.append(decoded);
    } catch (...) {
        return false;
    }
    return true;
}

std::uint64_t currentDwgBit(const dwgBuffer *buf) {
    return buf->getPosition() * 8 + buf->getBitPos();
}

bool leaderHasEndpointProjection(DRW::Version version) {
    return version >= DRW::AC1014;
}

bool leaderHasTextBox(DRW::Version version) {
    return version <= DRW::AC1021;
}

std::uint64_t tableBodyEndBit(DRW::Version version,
                              const dwgBuffer *stringBuf,
                              std::uint32_t objectSize) {
    // AC1021 objects may legally omit the string footer when they have no
    // detached text. Keep their complete object body available; AC1024+
    // frames have the unambiguous extended object-data boundary.
    if (version > DRW::AC1021 && stringBuf != nullptr)
        return currentDwgBit(stringBuf);
    return objectSize;
}

bool readTableBodyCount(DRW::Version version, dwgBuffer *buf,
                        std::uint64_t objectSize, std::uint32_t maximum,
                        std::uint32_t minimumBitsPerItem,
                        std::uint32_t& value) {
    if (buf == nullptr || !buf->isGood())
        return false;

    dwgBuffer probe = buf->forkIndependent();
    const std::int32_t parsed = probe.getBitLong();
    if (!probe.isGood() || parsed < 0
        || static_cast<std::uint32_t>(parsed) > maximum)
        return false;

    if (version >= DRW::AC1015 && objectSize != 0) {
        const std::uint64_t current = currentDwgBit(&probe);
        if (current > objectSize || (minimumBitsPerItem != 0
                                     && static_cast<std::uint64_t>(parsed)
                                         > (objectSize - current)
                                             / minimumBitsPerItem))
            return false;
    }

    *buf = probe;
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

std::uint64_t proxyEntityEndBit(const dwgBuffer& buffer,
                                std::uint32_t objectSize) {
    if (objectSize != 0)
        return objectSize;
    return buffer.size() > std::numeric_limits<std::uint64_t>::max() / 8u
        ? std::numeric_limits<std::uint64_t>::max()
        : static_cast<std::uint64_t>(buffer.size()) * 8u;
}

bool entityBodyDataEndBit(const dwgBuffer& stringBuffer,
                          DRW::Version version, std::uint32_t objectSize,
                          std::uint64_t& endBit,
                          std::uint64_t* stringEndBit = nullptr) {
    if (version <= DRW::AC1021 || objectSize == 0) {
        endBit = proxyEntityEndBit(stringBuffer, objectSize);
        if (stringEndBit != nullptr)
            *stringEndBit = endBit;
        return true;
    }

    std::uint64_t stringStartBit = 0;
    std::uint64_t parsedStringEndBit = 0;
    if (!stringBuffer.getR2007StringStreamBounds(
            objectSize, stringStartBit, parsedStringEndBit)
        || stringStartBit > parsedStringEndBit
        || parsedStringEndBit > objectSize
        || currentDwgBit(&stringBuffer) != stringStartBit) {
        return false;
    }
    endBit = stringStartBit;
    if (stringEndBit != nullptr)
        *stringEndBit = parsedStringEndBit;
    return true;
}

bool dwgHandleStreamEndBit(const dwgBuffer& buffer, DRW::Version version,
                           std::uint32_t objectSize, std::uint32_t handleBits,
                           std::uint64_t& endBit) {
    if (version > DRW::AC1021) {
        std::uint64_t totalBits = 0;
        return dwgSafety::add(objectSize, handleBits, endBit)
            && dwgSafety::multiply(buffer.size(), 8, totalBits)
            && endBit <= totalBits;
    }
    return dwgSafety::multiply(buffer.size(), 8, endBit);
}

bool proxyEntityHasBits(const dwgBuffer& buffer, std::uint64_t endBit,
                        std::uint64_t count) {
    const std::uint64_t current = currentDwgBit(&buffer);
    return current <= endBit && count <= endBit - current;
}

bool readProxyEntityOptionalBitLong(dwgBuffer& buffer, std::uint64_t endBit,
                                    bool& present, std::int32_t& value) {
    present = false;
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return true;

    const std::uint8_t prefix = buffer.get2Bits();
    if (!buffer.isGood())
        return false;
    if (prefix == 0) {
        if (!proxyEntityHasBits(buffer, endBit, 32))
            return false;
        value = static_cast<std::int32_t>(buffer.getRawLong32());
    } else if (prefix == 1) {
        if (!proxyEntityHasBits(buffer, endBit, 8))
            return false;
        value = static_cast<std::int32_t>(buffer.getRawChar8());
    } else {
        value = 0;
    }
    present = buffer.isGood();
    return present;
}

bool copyDwgBitRange(const dwgBuffer& source, std::uint64_t startBit,
                     std::uint64_t endBit,
                     std::vector<std::uint8_t>& data) {
    data.clear();
    if (endBit <= startBit)
        return true;

    const std::uint64_t sourceEnd = proxyEntityEndBit(source, 0);
    if (endBit > sourceEnd)
        return false;
    const std::uint64_t startByte = startBit >> 3;
    const std::uint64_t endByte = (endBit >> 3) + ((endBit & 7u) != 0);
    const std::uint64_t byteCount = endByte - startByte;
    if (endByte < startByte
        || byteCount > std::numeric_limits<std::size_t>::max()
        || byteCount > static_cast<std::uint64_t>(std::numeric_limits<int>::max())
        || !DRW::resize(data, static_cast<int>(byteCount)))
        return false;
    dwgBuffer copy = source;
    if (!copy.setPosition(startByte))
        return false;
    if ((startBit & 7u) != 0) {
        copy.setBitPos(static_cast<std::uint8_t>(startBit & 7u));
        if (!copy.isGood())
            return false;
    }
    return data.empty()
        || (copy.getBytes(data.data(), data.size()) && copy.isGood());
}

bool readBoundedBitShort(dwgBuffer& buffer, std::uint64_t endBit,
                         std::uint16_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;

    // BS has a two-bit selector followed by either no payload, one byte, or
    // two bytes. Probe the selector on an independent cursor before touching
    // the publishing cursor, so a frame boundary is checked exactly.
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint8_t selector = probe.get2Bits();
    if (!probe.isGood())
        return false;
    const std::uint64_t bits = selector == 0 ? 18
        : selector == 1 ? 10 : 2;
    if (!proxyEntityHasBits(buffer, endBit, bits))
        return false;
    value = buffer.getBitShort();
    return buffer.isGood();
}

bool readBoundedRawChar8(dwgBuffer& buffer, std::uint64_t endBit,
                         std::uint8_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 8))
        return false;
    value = buffer.getRawChar8();
    return buffer.isGood();
}

bool readBounded2Bits(dwgBuffer& buffer, std::uint64_t endBit,
                      std::uint8_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;
    value = buffer.get2Bits();
    return buffer.isGood();
}

bool readBoundedRawLong32(dwgBuffer& buffer, std::uint64_t endBit,
                          std::uint32_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 32))
        return false;
    value = buffer.getRawLong32();
    return buffer.isGood();
}

bool readBoundedBit(dwgBuffer& buffer, std::uint64_t endBit, bool& value) {
    if (!proxyEntityHasBits(buffer, endBit, 1))
        return false;
    value = buffer.getBit() != 0;
    return buffer.isGood();
}

bool readBoundedBitLong(dwgBuffer& buffer, std::uint64_t endBit,
                        std::int32_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint8_t selector = probe.get2Bits();
    if (!probe.isGood())
        return false;
    const std::uint64_t bits = selector == 0 ? 34
        : selector == 1 ? 10 : 2;
    if (!proxyEntityHasBits(buffer, endBit, bits))
        return false;
    value = buffer.getBitLong();
    return buffer.isGood();
}

bool readBoundedBitLongLong(dwgBuffer& buffer, std::uint64_t endBit,
                            std::uint64_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 3))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint8_t byteCount = probe.get3Bits();
    const std::uint64_t bits = 3u + static_cast<std::uint64_t>(byteCount) * 8u;
    if (!probe.isGood() || !proxyEntityHasBits(buffer, endBit, bits))
        return false;
    value = buffer.getBitLongLong();
    return buffer.isGood();
}

bool readBoundedBitDouble(dwgBuffer& buffer, std::uint64_t endBit,
                          double& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint8_t selector = probe.get2Bits();
    if (!probe.isGood())
        return false;
    const std::uint64_t bits = selector == 0 ? 66 : 2;
    if (!proxyEntityHasBits(buffer, endBit, bits))
        return false;
    dwgBuffer candidate = buffer.forkIndependent();
    const double parsed = candidate.getBitDouble();
    if (!candidate.isGood() || currentDwgBit(&candidate) > endBit
        || !std::isfinite(parsed))
        return false;
    buffer = candidate;
    value = parsed;
    return true;
}

bool readBoundedBitCoord(dwgBuffer& buffer, std::uint64_t endBit,
                         DRW_Coord& value) {
    return readBoundedBitDouble(buffer, endBit, value.x)
        && readBoundedBitDouble(buffer, endBit, value.y)
        && readBoundedBitDouble(buffer, endBit, value.z);
}

bool readBoundedEnColor(dwgBuffer& buffer, std::uint64_t endBit,
                        DRW::Version version, std::uint32_t& value) {
    if (!buffer.isGood() || currentDwgBit(&buffer) > endBit)
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint32_t parsed = probe.getEnColor(version);
    if (!probe.isGood() || currentDwgBit(&probe) > endBit)
        return false;
    buffer = probe;
    buffer.lastEnColorHadDbColorRef = probe.lastEnColorHadDbColorRef;
    buffer.lastEnColorName = probe.lastEnColorName;
    buffer.lastEnColorBookName = probe.lastEnColorBookName;
    buffer.lastEnColorRgb = probe.lastEnColorRgb;
    buffer.lastEnColorAlphaRaw = probe.lastEnColorAlphaRaw;
    value = parsed;
    return true;
}

struct TableDwgBounds {
    std::uint64_t bodyEndBit = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t stringEndBit = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t handleEndBit = std::numeric_limits<std::uint64_t>::max();
};

std::uint64_t tableTextEndBit(const dwgBuffer *bodyBuf,
                              const dwgBuffer *textBuf,
                              const TableDwgBounds& bounds) {
    return textBuf == bodyBuf ? bounds.bodyEndBit : bounds.stringEndBit;
}

bool readBoundedRawShort16(dwgBuffer& buffer, std::uint64_t endBit,
                           std::uint16_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 16))
        return false;
    value = buffer.getRawShort16();
    return buffer.isGood();
}

bool skipTableBit(dwgBuffer& buffer, std::uint64_t endBit) {
    bool value = false;
    return readBoundedBit(buffer, endBit, value);
}

bool skipTableBitShort(dwgBuffer& buffer, std::uint64_t endBit) {
    std::uint16_t value = 0;
    return readBoundedBitShort(buffer, endBit, value);
}

bool skipTableBitLong(dwgBuffer& buffer, std::uint64_t endBit) {
    std::int32_t value = 0;
    return readBoundedBitLong(buffer, endBit, value);
}

bool skipTableBitDouble(dwgBuffer& buffer, std::uint64_t endBit) {
    double value = 0.0;
    return readBoundedBitDouble(buffer, endBit, value);
}

bool skipTableRawChar8(dwgBuffer& buffer, std::uint64_t endBit) {
    std::uint8_t value = 0;
    return readBoundedRawChar8(buffer, endBit, value);
}

bool skipTableRawShort16(dwgBuffer& buffer, std::uint64_t endBit) {
    std::uint16_t value = 0;
    return readBoundedRawShort16(buffer, endBit, value);
}

bool readBounded3BitDouble(dwgBuffer& buffer, std::uint64_t endBit,
                           DRW_Coord& value) {
    dwgBuffer probe = buffer.forkIndependent();
    DRW_Coord parsed;
    if (!readBoundedBitDouble(probe, endBit, parsed.x)
        || !readBoundedBitDouble(probe, endBit, parsed.y)
        || !readBoundedBitDouble(probe, endBit, parsed.z))
        return false;
    buffer = probe;
    value = parsed;
    return true;
}

bool readBoundedBytes(dwgBuffer& buffer, std::uint64_t endBit,
                      std::uint8_t *data, std::size_t size) {
    std::uint64_t bits = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(size), 8, bits)
        || !proxyEntityHasBits(buffer, endBit, bits))
        return false;
    return size == 0 || (buffer.getBytes(data, size) && buffer.isGood());
}

bool readBoundedRawLong64(dwgBuffer& buffer, std::uint64_t endBit,
                          std::uint64_t& value) {
    if (!proxyEntityHasBits(buffer, endBit, 64))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint64_t parsed = probe.getRawLong64();
    if (!probe.isGood())
        return false;
    buffer = probe;
    value = parsed;
    return true;
}

bool readBounded2RawDouble(dwgBuffer& buffer, std::uint64_t endBit,
                           DRW_Coord& value) {
    if (!proxyEntityHasBits(buffer, endBit, 128))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    DRW_Coord parsed = probe.get2RawDouble();
    if (!probe.isGood()
        || !std::isfinite(parsed.x)
        || !std::isfinite(parsed.y)
        || !std::isfinite(parsed.z))
        return false;
    parsed.z = 0.0; // 2RD carries only X and Y; keep the unused component defined.
    buffer = probe;
    value = parsed;
    return true;
}

bool readBoundedRawDouble(dwgBuffer& buffer, std::uint64_t endBit,
                          double& value) {
    if (!proxyEntityHasBits(buffer, endBit, 64))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const double parsed = probe.getRawDouble();
    if (!probe.isGood() || currentDwgBit(&probe) > endBit
        || !std::isfinite(parsed))
        return false;
    buffer = probe;
    value = parsed;
    return true;
}

bool readBoundedDwgHandle(dwgBuffer& buffer, std::uint64_t endBit,
                          std::uint32_t baseHandle, bool offset,
                          dwgHandle& value) {
    if (!buffer.isGood() || currentDwgBit(&buffer) > endBit)
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    dwgHandle parsed;
    if (!readDwgHandleChecked(probe, baseHandle, offset, parsed)
        || currentDwgBit(&probe) > endBit)
        return false;
    buffer = probe;
    value = parsed;
    return true;
}

bool readBoundedDwgObjectId(dwgBuffer& buffer, std::uint64_t endBit,
                            std::uint32_t baseHandle, dwgHandle& raw,
                            dwgHandle& resolved) {
    if (!buffer.isGood() || currentDwgBit(&buffer) > endBit)
        return false;

    dwgBuffer rawProbe = buffer.forkIndependent();
    raw = rawProbe.getHandle();
    if (!rawProbe.isGood() || currentDwgBit(&rawProbe) > endBit)
        return false;

    dwgBuffer resolvedProbe = buffer.forkIndependent();
    resolved = resolvedProbe.getOffsetHandle(baseHandle);
    if (!resolvedProbe.isGood() || currentDwgBit(&resolvedProbe) > endBit)
        return false;

    buffer = resolvedProbe;
    return true;
}

bool readBoundedVariableText(dwgBuffer& buffer, std::uint64_t endBit,
                             DRW::Version version, UTF8STRING& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;

    dwgBuffer probe = buffer.forkIndependent();
    const std::uint16_t units = probe.getBitShort();
    if (!probe.isGood())
        return false;

    const std::uint64_t bitsPerUnit = version > DRW::AC1018 ? 16u : 8u;
    std::uint64_t payloadBits = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(units), bitsPerUnit,
                             payloadBits)
        || !proxyEntityHasBits(probe, endBit, payloadBits))
        return false;

    value = buffer.getVariableText(version, false);
    return buffer.isGood() && currentDwgBit(&buffer) <= endBit;
}

bool readBoundedDefaultDouble(dwgBuffer& buffer, std::uint64_t endBit,
                              double defaultValue, double& value) {
    if (!proxyEntityHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const std::uint8_t selector = probe.get2Bits();
    if (!probe.isGood())
        return false;
    const std::uint64_t bits = selector == 0 ? 2
        : selector == 1 ? 34
        : selector == 2 ? 50 : 66;
    if (!proxyEntityHasBits(buffer, endBit, bits))
        return false;
    value = buffer.getDefaultDouble(defaultValue);
    return buffer.isGood();
}

bool readBoundedThickness(dwgBuffer& buffer, std::uint64_t endBit,
                          bool modern, double& value) {
    if (!modern)
        return readBoundedBitDouble(buffer, endBit, value);
    if (!proxyEntityHasBits(buffer, endBit, 1))
        return false;
    dwgBuffer probe = buffer.forkIndependent();
    const bool isZero = probe.getBit() != 0;
    if (!probe.isGood())
        return false;
    buffer.getBit();
    if (!buffer.isGood())
        return false;
    if (isZero) {
        value = 0.0;
        return true;
    }
    return readBoundedBitDouble(buffer, endBit, value);
}

bool readBoundedExtrusion(dwgBuffer& buffer, std::uint64_t endBit,
                          bool modern, DRW_Coord& value) {
    if (modern) {
        if (!proxyEntityHasBits(buffer, endBit, 1))
            return false;
        dwgBuffer probe = buffer.forkIndependent();
        const bool isDefault = probe.getBit() != 0;
        if (!probe.isGood())
            return false;
        buffer.getBit();
        if (!buffer.isGood())
            return false;
        if (isDefault) {
            value = DRW_Coord{0.0, 0.0, 1.0};
            return true;
        }
    }

    return readBoundedBitDouble(buffer, endBit, value.x)
        && readBoundedBitDouble(buffer, endBit, value.y)
        && readBoundedBitDouble(buffer, endBit, value.z);
}

bool readBoundedCmColor(dwgBuffer& buffer, dwgBuffer *stringBuffer,
                        std::uint64_t endBit, DRW::Version version,
                        std::uint32_t& value, std::int32_t *rgb24 = nullptr,
                        bool *hasRgbColor = nullptr,
                        UTF8STRING *outName = nullptr,
                        UTF8STRING *outBookName = nullptr,
                        std::uint64_t stringEndBit =
                            std::numeric_limits<std::uint64_t>::max()) {
    const bool separateStringBuffer = stringBuffer != nullptr
        && stringBuffer != &buffer;
    dwgBuffer bodyProbe = buffer.forkIndependent();
    dwgBuffer stringProbe = separateStringBuffer
        ? stringBuffer->forkIndependent() : bodyProbe.forkIndependent();
    dwgBuffer *colorStringBuffer = separateStringBuffer
        ? &stringProbe : &bodyProbe;
    UTF8STRING parsedName;
    UTF8STRING parsedBookName;
    std::int32_t parsedRgb = -1;
    bool parsedHasRgb = false;
    const std::uint32_t parsed = bodyProbe.getCmColor(
        version, &parsedRgb, colorStringBuffer, &parsedName, &parsedBookName,
        &parsedHasRgb);
    if (!bodyProbe.isGood() || !colorStringBuffer->isGood()
        || currentDwgBit(&bodyProbe) > endBit
        || (separateStringBuffer
            && currentDwgBit(&stringProbe) > stringEndBit))
        return false;
    buffer = bodyProbe;
    if (separateStringBuffer)
        *stringBuffer = stringProbe;
    value = parsed;
    if (outName != nullptr)
        *outName = std::move(parsedName);
    if (outBookName != nullptr)
        *outBookName = std::move(parsedBookName);
    if (rgb24 != nullptr)
        *rgb24 = parsedRgb;
    if (hasRgbColor != nullptr)
        *hasRgbColor = parsedHasRgb;
    return true;
}

struct DwgTextBodyData {
    DRW_Coord basePoint{0.0, 0.0, 0.0};
    DRW_Coord secPoint{0.0, 0.0, 0.0};
    DRW_Coord extPoint{0.0, 0.0, 1.0};
    double thickness = 0.0;
    double oblique = 0.0;
    double angle = 0.0;
    double height = 0.0;
    double widthscale = 1.0;
    UTF8STRING text;
    int textgen = 0;
    DRW_Text::HAlign alignH = DRW_Text::HLeft;
    DRW_Text::VAlign alignV = DRW_Text::VBaseLine;
};

bool readBoundedDwgTextBody(
    DRW::Version version, dwgBuffer& body, dwgBuffer& stringBuffer,
    std::uint64_t bodyEndBit, std::uint64_t stringStartBit,
    std::uint64_t stringEndBit, DwgTextBodyData& data) {
    std::uint8_t dataFlags = 0;
    if (version > DRW::AC1014) {
        if (!readBoundedRawChar8(body, bodyEndBit, dataFlags))
            return false;
        if (!(dataFlags & 0x01)
            && !readBoundedRawDouble(body, bodyEndBit, data.basePoint.z))
            return false;
    } else if (!readBoundedBitDouble(body, bodyEndBit, data.basePoint.z)) {
        return false;
    }

    if (!readBoundedRawDouble(body, bodyEndBit, data.basePoint.x)
        || !readBoundedRawDouble(body, bodyEndBit, data.basePoint.y))
        return false;

    if (version > DRW::AC1014) {
        if (!(dataFlags & 0x02)) {
            if (!readBoundedDefaultDouble(body, bodyEndBit,
                                          data.basePoint.x, data.secPoint.x)
                || !readBoundedDefaultDouble(body, bodyEndBit,
                                              data.basePoint.y,
                                              data.secPoint.y))
                return false;
        } else {
            data.secPoint = data.basePoint;
        }
    } else if (!readBoundedRawDouble(body, bodyEndBit, data.secPoint.x)
               || !readBoundedRawDouble(body, bodyEndBit, data.secPoint.y)) {
        return false;
    }
    data.secPoint.z = data.basePoint.z;

    if (!readBoundedExtrusion(body, bodyEndBit, version > DRW::AC1014,
                              data.extPoint)
        || !readBoundedThickness(body, bodyEndBit, version > DRW::AC1014,
                                 data.thickness))
        return false;

    if (version > DRW::AC1014) {
        if (!(dataFlags & 0x04)
            && !readBoundedRawDouble(body, bodyEndBit, data.oblique))
            return false;
        if (!(dataFlags & 0x08)
            && !readBoundedRawDouble(body, bodyEndBit, data.angle))
            return false;
        if (!readBoundedRawDouble(body, bodyEndBit, data.height))
            return false;
        if (!(dataFlags & 0x10)
            && !readBoundedRawDouble(body, bodyEndBit, data.widthscale))
            return false;
    } else if (!readBoundedBitDouble(body, bodyEndBit, data.oblique)
               || !readBoundedBitDouble(body, bodyEndBit, data.angle)
               || !readBoundedBitDouble(body, bodyEndBit, data.height)
               || !readBoundedBitDouble(body, bodyEndBit, data.widthscale)) {
        return false;
    }
    data.angle *= ARAD;

    if (version > DRW::AC1018 && stringStartBit >= stringEndBit) {
        data.text.clear();
    } else if (!readBoundedVariableText(stringBuffer, stringEndBit, version,
                                         data.text)) {
        return false;
    }

    std::uint16_t parsedTextgen = 0;
    std::uint16_t parsedAlignH = 0;
    std::uint16_t parsedAlignV = 0;
    if (!(dataFlags & 0x20)
        && !readBoundedBitShort(body, bodyEndBit, parsedTextgen))
        return false;
    if (!(dataFlags & 0x40)
        && !readBoundedBitShort(body, bodyEndBit, parsedAlignH))
        return false;
    if (!(dataFlags & 0x80)
        && !readBoundedBitShort(body, bodyEndBit, parsedAlignV))
        return false;
    data.textgen = parsedTextgen;
    data.alignH = static_cast<DRW_Text::HAlign>(parsedAlignH);
    data.alignV = static_cast<DRW_Text::VAlign>(parsedAlignV);

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    return body.isGood() && stringBuffer.isGood() && finite(data.basePoint)
        && finite(data.secPoint) && finite(data.extPoint)
        && std::isfinite(data.thickness) && std::isfinite(data.oblique)
        && std::isfinite(data.angle) && std::isfinite(data.height)
        && std::isfinite(data.widthscale);
}

bool readBoundedHatchGradient(dwgBuffer& body, dwgBuffer *stringBuffer,
                              std::uint64_t endBit, DRW::Version version,
                              int& isGradient, int& reserved,
                              double& angle, double& shift, int& singleColor,
                              double& tint, UTF8STRING& name,
                              std::vector<DRW_Hatch::GradientStop>& colors,
                              std::uint64_t stringEndBit =
                                  std::numeric_limits<std::uint64_t>::max()) {
    std::int32_t parsedIsGradient = 0;
    std::int32_t parsedReserved = 0;
    std::int32_t parsedSingleColor = 0;
    double parsedAngle = 0.0;
    double parsedShift = 0.0;
    double parsedTint = 0.0;
    if (!readBoundedBitLong(body, endBit, parsedIsGradient)
        || !readBoundedBitLong(body, endBit, parsedReserved)
        || !readBoundedBitDouble(body, endBit, parsedAngle)
        || !readBoundedBitDouble(body, endBit, parsedShift)
        || !readBoundedBitLong(body, endBit, parsedSingleColor)
        || !readBoundedBitDouble(body, endBit, parsedTint))
        return false;

    std::uint32_t colorCount = 0;
    if (!readTableBodyCount(version, &body, endBit, kMaxHatchItems, 14,
                            colorCount))
        return false;
    std::vector<DRW_Hatch::GradientStop> parsedColors;
    if (!DRW::reserve(parsedColors, colorCount))
        return false;
    for (std::uint32_t i = 0; i < colorCount; ++i) {
        DRW_Hatch::GradientStop stop;
        std::uint16_t aciColor = 0;
        std::int32_t rgb = 0;
        std::uint8_t ignoredColor = 0;
        if (!readBoundedBitDouble(body, endBit, stop.value)
            || !readBoundedBitShort(body, endBit, aciColor)
            || !readBoundedBitLong(body, endBit, rgb)
            || !readBoundedRawChar8(body, endBit, ignoredColor)
            || !std::isfinite(stop.value))
            return false;
        stop.aciColor = aciColor;
        stop.rgb = rgb;
        parsedColors.push_back(std::move(stop));
    }

    UTF8STRING parsedName;
    if (stringBuffer == nullptr
        || !readBoundedVariableText(*stringBuffer, stringEndBit, version,
                                    parsedName))
        return false;

    if (!std::isfinite(parsedAngle) || !std::isfinite(parsedShift)
        || !std::isfinite(parsedTint))
        return false;
    isGradient = parsedIsGradient;
    reserved = parsedReserved;
    angle = parsedAngle;
    shift = parsedShift;
    singleColor = parsedSingleColor;
    tint = parsedTint;
    name = std::move(parsedName);
    colors = std::move(parsedColors);
    return true;
}

struct SurfaceDwgPayloadInfo {
    bool acisEmpty = false;
    int acisVersion = 0;
    int modelerFormatVersion = 0;
    int uIsolines = 0;
    int vIsolines = 0;
    bool revolvedFieldsValid = false;
    std::uint32_t revolvedClassId = 0;
    std::uint32_t revolvedId = 0;
    DRW_Coord revolvedAxisPoint;
    DRW_Coord revolvedAxisVector;
    double revolvedAngle = 0.0;
    double revolvedStartAngle = 0.0;
    std::array<double, DRW_RevolvedSurface::kTransformSize> revolvedTransform{};
    double revolvedDraftAngle = 0.0;
    double revolvedDraftStartDistance = 0.0;
    double revolvedDraftEndDistance = 0.0;
    double revolvedTwistAngle = 0.0;
    bool revolvedSolid = false;
    bool revolvedCloseToAxis = false;
    bool extrudedFieldsValid = false;
    DRW_Coord extrudedSweepVector;
    std::array<double, DRW_ExtrudedSurface::kTransformSize>
        extrudedTransform{};
    std::array<double, DRW_ExtrudedSurface::kTransformSize>
        extrudedSweepEntityTransform{};
    std::array<double, DRW_ExtrudedSurface::kTransformSize>
        extrudedPathEntityTransform{};
    double extrudedDraftAngle = 0.0;
    double extrudedDraftStartDistance = 0.0;
    double extrudedDraftEndDistance = 0.0;
    double extrudedTwistAngle = 0.0;
    double extrudedScaleFactor = 0.0;
    double extrudedAlignAngle = 0.0;
    bool extrudedSolid = false;
    std::int32_t extrudedSweepAlignmentFlags = 0;
    std::int32_t extrudedPathFlags = 0;
    bool extrudedAlignStart = false;
    bool extrudedBank = false;
    bool extrudedBasePointSet = false;
    bool extrudedSweepEntityTransformComputed = false;
    bool extrudedPathEntityTransformComputed = false;
    DRW_Coord extrudedReferenceVector;
    bool sweptFieldsValid = false;
    std::uint32_t sweptClassVersion = 0;
    std::uint32_t sweptEntityId = 0;
    std::vector<std::uint8_t> sweptData;
    std::uint32_t sweptPathEntityId = 0;
    std::vector<std::uint8_t> sweptPathData;
    std::array<double, DRW_SweptSurface::kTransformSize>
        sweptEntityTransform{};
    std::array<double, DRW_SweptSurface::kTransformSize>
        sweptPathEntityTransform{};
    double sweptDraftAngle = 0.0;
    double sweptDraftStartDistance = 0.0;
    double sweptDraftEndDistance = 0.0;
    double sweptTwistAngle = 0.0;
    double sweptScaleFactor = 1.0;
    double sweptAlignAngle = 0.0;
    std::array<double, DRW_SweptSurface::kTransformSize>
        sweptEntityTransformed{};
    std::array<double, DRW_SweptSurface::kTransformSize>
        sweptPathEntityTransformed{};
    bool sweptSolid = false;
    std::int32_t sweptAlignmentFlags = 0;
    std::int32_t sweptPathFlags = 0;
    bool sweptAlignStart = false;
    bool sweptBank = false;
    bool sweptBasePointSet = false;
    bool sweptEntityTransformComputed = false;
    bool sweptPathEntityTransformComputed = false;
    DRW_Coord sweptReferenceVector;
    bool loftedFieldsValid = false;
    std::array<double, DRW_LoftedSurface::kTransformSize>
        loftedEntityTransform{};
    std::int32_t loftedPlaneNormalLoftingType = 0;
    double loftedStartDraftAngle = 0.0;
    double loftedEndDraftAngle = 0.0;
    double loftedStartDraftMagnitude = 0.0;
    double loftedEndDraftMagnitude = 0.0;
    bool loftedArcLengthParameterization = false;
    bool loftedNoTwist = true;
    bool loftedAlignDirection = true;
    bool loftedSimpleSurfaces = true;
    bool loftedClosedSurfaces = false;
    bool loftedSolid = false;
    bool loftedRuledSurface = false;
    bool loftedVirtualGuide = false;
    std::int32_t loftedNumCrossSections = 0;
    std::int32_t loftedNumGuideCurves = 0;
    bool nurbsFieldsValid = false;
    std::uint16_t nurbsShort170 = 0;
    bool nurbsCvHullDisplay = false;
    DRW_Coord nurbsUvec1;
    DRW_Coord nurbsVvec1;
    DRW_Coord nurbsUvec2;
    DRW_Coord nurbsVvec2;
};

std::uint64_t surfaceDwgBit(const dwgBuffer& buffer) {
    return buffer.getPosition() * 8u + buffer.getBitPos();
}

bool surfaceDwgHasBits(const dwgBuffer& buffer, std::uint64_t endBit,
                       std::uint64_t count) {
    const std::uint64_t current = surfaceDwgBit(buffer);
    return current <= endBit && count <= endBit - current;
}

bool surfaceDwgReadBit(dwgBuffer& buffer, std::uint64_t endBit,
                       std::uint8_t& value) {
    if (!surfaceDwgHasBits(buffer, endBit, 1))
        return false;
    // Probe variable-width fields on a fork so a short object frame cannot
    // invalidate the reader before the caller re-anchors at its boundary.
    dwgBuffer candidate(buffer);
    value = candidate.getBit();
    if (!candidate.isGood() || surfaceDwgBit(candidate) > endBit)
        return false;
    buffer = candidate;
    return true;
}

bool surfaceDwgReadRawChar(dwgBuffer& buffer, std::uint64_t endBit,
                           std::uint8_t& value) {
    if (!surfaceDwgHasBits(buffer, endBit, 8))
        return false;
    dwgBuffer candidate(buffer);
    value = candidate.getRawChar8();
    if (!candidate.isGood() || surfaceDwgBit(candidate) > endBit)
        return false;
    buffer = candidate;
    return true;
}

bool surfaceDwgReadRawChar(dwgBuffer& buffer, std::uint64_t endBit) {
    std::uint8_t value = 0;
    return surfaceDwgReadRawChar(buffer, endBit, value);
}

bool surfaceDwgReadBitShort(dwgBuffer& buffer, std::uint64_t endBit,
                            std::uint16_t& value) {
    if (!surfaceDwgHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer candidate(buffer);
    value = candidate.getBitShort();
    if (!candidate.isGood() || surfaceDwgBit(candidate) > endBit)
        return false;
    buffer = candidate;
    return true;
}

bool surfaceDwgReadBitLong(dwgBuffer& buffer, std::uint64_t endBit,
                           std::int32_t& value) {
    if (!surfaceDwgHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer candidate(buffer);
    value = candidate.getBitLong();
    if (!candidate.isGood() || surfaceDwgBit(candidate) > endBit)
        return false;
    buffer = candidate;
    return true;
}

bool surfaceDwgReadDouble(dwgBuffer& buffer, std::uint64_t endBit,
                          double& value) {
    if (!surfaceDwgHasBits(buffer, endBit, 2))
        return false;
    dwgBuffer candidate(buffer);
    value = candidate.getBitDouble();
    if (!candidate.isGood() || surfaceDwgBit(candidate) > endBit
        || !std::isfinite(value))
        return false;
    buffer = candidate;
    return true;
}

bool surfaceDwgReadDouble(dwgBuffer& buffer, std::uint64_t endBit) {
    double value = 0.0;
    return surfaceDwgReadDouble(buffer, endBit, value);
}

bool surfaceDwgRead3BitDouble(dwgBuffer& buffer, std::uint64_t endBit) {
    return surfaceDwgReadDouble(buffer, endBit)
        && surfaceDwgReadDouble(buffer, endBit)
        && surfaceDwgReadDouble(buffer, endBit);
}

bool surfaceDwgRead3BitDouble(dwgBuffer& buffer, std::uint64_t endBit,
                              DRW_Coord& value) {
    return surfaceDwgReadDouble(buffer, endBit, value.x)
        && surfaceDwgReadDouble(buffer, endBit, value.y)
        && surfaceDwgReadDouble(buffer, endBit, value.z);
}

bool surfaceDwgSkipBytes(dwgBuffer& buffer, std::uint64_t endBit,
                         std::uint64_t count) {
    const std::uint64_t current = surfaceDwgBit(buffer);
    if (current > endBit
        || count > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) / 8u
        || count > (endBit - current) / 8u)
        return false;
    return buffer.moveBitPos(static_cast<std::int32_t>(count * 8u))
        && buffer.isGood() && surfaceDwgBit(buffer) <= endBit;
}

bool surfaceDwgReadBytes(dwgBuffer& buffer, std::uint64_t endBit,
                         std::int32_t count, std::size_t maxCount,
                         std::vector<std::uint8_t>& data) {
    if (count < 0 || static_cast<std::size_t>(count) > maxCount
        || !surfaceDwgHasBits(buffer, endBit,
                              static_cast<std::uint64_t>(count) * 8u))
        return false;
    dwgBuffer candidate(buffer);
    std::vector<std::uint8_t> decoded;
    if (!DRW::resize(decoded, count))
        return false;
    for (std::uint8_t& value : decoded) {
        if (!surfaceDwgReadRawChar(candidate, endBit, value))
            return false;
    }
    data = std::move(decoded);
    buffer = candidate;
    return true;
}

std::uint8_t surfaceDwgPeekByte(const std::vector<std::uint8_t>& data,
                                std::size_t index, std::uint8_t bit) {
    if (index >= data.size())
        return 0;
    if (bit == 0)
        return data[index];
    const std::uint8_t next = index + 1 < data.size() ? data[index + 1] : 0;
    return static_cast<std::uint8_t>((data[index] << bit) | (next >> (8 - bit)));
}

std::size_t surfaceDwgFindMarker(const std::vector<std::uint8_t>& data,
                                 std::size_t start, std::uint8_t bit,
                                 const std::uint8_t* marker,
                                 std::size_t markerSize) {
    if (marker == nullptr || markerSize == 0 || data.size() < markerSize
        || start > data.size() - markerSize)
        return std::numeric_limits<std::size_t>::max();
    for (std::size_t index = start; index <= data.size() - markerSize; ++index) {
        bool matched = true;
        for (std::size_t offset = 0; offset < markerSize; ++offset) {
            if (surfaceDwgPeekByte(data, index + offset, bit) != marker[offset]) {
                matched = false;
                break;
            }
        }
        if (matched)
            return index;
    }
    return std::numeric_limits<std::size_t>::max();
}

bool surfaceDwgSkipAcis(dwgBuffer& buffer, const std::vector<std::uint8_t>& data,
                        std::uint64_t endBit, bool& acisEmpty, int& acisVersion) {
    std::uint8_t empty = 0;
    if (!surfaceDwgReadBit(buffer, endBit, empty))
        return false;
    acisEmpty = empty != 0;
    if (acisEmpty)
        return true;

    std::uint8_t unused = 0;
    std::uint16_t version = 0;
    if (!surfaceDwgReadBit(buffer, endBit, unused)
        || !surfaceDwgReadBitShort(buffer, endBit, version))
        return false;
    acisVersion = static_cast<int>(version);

    if (version == 2) {
        static constexpr std::uint8_t endAcis[] = {
            0x0E, 0x03, 0x45, 0x6E, 0x64, 0x0E, 0x02, 0x6F, 0x66, 0x0E,
            0x04, 0x41, 0x43, 0x49, 0x53, 0x0D, 0x04, 0x64, 0x61, 0x74,
            0x61};
        static constexpr std::uint8_t endAsm[] = {
            0x0E, 0x03, 0x45, 0x6E, 0x64, 0x0E, 0x02, 0x6F, 0x66, 0x0E,
            0x03, 0x41, 0x53, 0x4D, 0x0D, 0x04, 0x64, 0x61, 0x74, 0x61};
        const std::size_t start = static_cast<std::size_t>(buffer.getPosition());
        const std::uint8_t bit = buffer.getBitPos();
        std::size_t marker = surfaceDwgFindMarker(
            data, start, bit, endAcis, sizeof(endAcis));
        std::size_t markerSize = sizeof(endAcis);
        if (marker == std::numeric_limits<std::size_t>::max()) {
            marker = surfaceDwgFindMarker(
                data, start, bit, endAsm, sizeof(endAsm));
            markerSize = sizeof(endAsm);
        }
        if (marker == std::numeric_limits<std::size_t>::max()
            || marker < start)
            return false;
        return surfaceDwgSkipBytes(buffer, endBit,
                                   marker - start + markerSize);
    }

    for (std::uint32_t block = 0; block < 65536u; ++block) {
        std::int32_t blockSize = 0;
        if (!surfaceDwgReadBitLong(buffer, endBit, blockSize))
            return false;
        if (blockSize == 0)
            return true;
        if (blockSize < 0
            || !surfaceDwgSkipBytes(buffer, endBit,
                                    static_cast<std::uint32_t>(blockSize)))
            return false;
    }
    return false;
}

bool surfaceDwgSkipWire(dwgBuffer& buffer, std::uint64_t endBit,
                        DRW::Version version) {
    if (!surfaceDwgReadRawChar(buffer, endBit))
        return false;
    std::int32_t ignored = 0;
    if (!surfaceDwgReadBitLong(buffer, endBit, ignored))
        return false;
    std::uint16_t color = 0;
    if (version < DRW::AC1018) {
        if (!surfaceDwgReadBitShort(buffer, endBit, color))
            return false;
    } else if (!surfaceDwgReadBitLong(buffer, endBit, ignored)) {
        return false;
    }
    if (!surfaceDwgReadBitLong(buffer, endBit, ignored))
        return false;

    std::int32_t pointCount = 0;
    if (!surfaceDwgReadBitLong(buffer, endBit, pointCount)
        || pointCount < 0 || pointCount > 1000000)
        return false;
    for (std::int32_t point = 0; point < pointCount; ++point) {
        if (!surfaceDwgRead3BitDouble(buffer, endBit))
            return false;
    }

    std::uint8_t transformPresent = 0;
    if (!surfaceDwgReadBit(buffer, endBit, transformPresent))
        return false;
    if (transformPresent != 0) {
        for (int field = 0; field < 5; ++field) {
            if (!surfaceDwgRead3BitDouble(buffer, endBit))
                return false;
        }
        for (int field = 0; field < 3; ++field) {
            if (!surfaceDwgReadBit(buffer, endBit, transformPresent))
                return false;
        }
    }
    return true;
}

bool surfaceDwgSkipCommon3dSolid(dwgBuffer& buffer, std::uint64_t endBit,
                                 DRW::Version version, int acisVersion) {
    std::uint8_t present = 0;
    if (!surfaceDwgReadBit(buffer, endBit, present))
        return false;
    if (present != 0) {
        if (!surfaceDwgReadBit(buffer, endBit, present))
            return false;
        if (present != 0 && !surfaceDwgRead3BitDouble(buffer, endBit))
            return false;

        std::int32_t ignored = 0;
        if (!surfaceDwgReadBitLong(buffer, endBit, ignored)
            || !surfaceDwgReadBit(buffer, endBit, present))
            return false;
        if (present != 0) {
            std::int32_t count = 0;
            if (!surfaceDwgReadBitLong(buffer, endBit, count)
                || count < 0 || count > 1000000)
                return false;
            for (std::int32_t i = 0; i < count; ++i) {
                if (!surfaceDwgSkipWire(buffer, endBit, version))
                    return false;
            }
            if (!surfaceDwgReadBitLong(buffer, endBit, count)
                || count < 0 || count > 1000000)
                return false;
            for (std::int32_t i = 0; i < count; ++i) {
                if (!surfaceDwgReadBitLong(buffer, endBit, ignored)
                    || !surfaceDwgRead3BitDouble(buffer, endBit)
                    || !surfaceDwgRead3BitDouble(buffer, endBit)
                    || !surfaceDwgRead3BitDouble(buffer, endBit)
                    || !surfaceDwgReadBit(buffer, endBit, present)
                    || !surfaceDwgReadBit(buffer, endBit, present))
                    return false;
                if (present != 0) {
                    std::int32_t wireCount = 0;
                    if (!surfaceDwgReadBitLong(buffer, endBit, wireCount)
                        || wireCount < 0 || wireCount > 1000000)
                        return false;
                    for (std::int32_t wire = 0; wire < wireCount; ++wire) {
                        if (!surfaceDwgSkipWire(buffer, endBit, version))
                            return false;
                    }
                }
            }
        }
    }

    if (!surfaceDwgReadBit(buffer, endBit, present))
        return false;
    if (acisVersion > 1 && version >= DRW::AC1021) {
        std::int32_t count = 0;
        if (!surfaceDwgReadBitLong(buffer, endBit, count)
            || count < 0 || count > 1000000)
            return false;
        for (std::int32_t i = 0; i < count; ++i) {
            std::int32_t ignored = 0;
            if (!surfaceDwgReadBitLong(buffer, endBit, ignored)
                || !surfaceDwgReadBitLong(buffer, endBit, ignored))
                return false;
        }
    }
    if (version >= DRW::AC1027) {
        if (!surfaceDwgReadBit(buffer, endBit, present))
            return false;
        std::int32_t ignored = 0;
        std::uint16_t ignoredShort = 0;
        if (!surfaceDwgReadBitLong(buffer, endBit, ignored)
            || !surfaceDwgReadBitShort(buffer, endBit, ignoredShort)
            || !surfaceDwgReadBitShort(buffer, endBit, ignoredShort)
            || !surfaceDwgSkipBytes(buffer, endBit, 8)
            || !surfaceDwgReadBitLong(buffer, endBit, ignored))
            return false;
    }
    return true;
}

bool surfaceHasModelerFormatVersion(DRW::ETYPE type) {
    switch (type) {
    case DRW::PLANESURFACE:
    case DRW::LOFTEDSURFACE:
    case DRW::REVOLVEDSURFACE:
    case DRW::SWEPTSURFACE:
        return true;
    case DRW::EXTRUDEDSURFACE:
    case DRW::NURBSURFACE:
    default:
        return false;
    }
}

std::uint16_t surfaceModelerFormatVersionLimit(DRW::ETYPE type) {
    return type == DRW::LOFTEDSURFACE ? kMaxLoftedSurfaceModelerFormatVersion
                                      : std::numeric_limits<std::uint16_t>::max();
}

bool readSurfaceDwgPayload(const std::vector<std::uint8_t>& data,
                           std::uint64_t bitSize, DRW::Version version,
                           DRW::ETYPE type,
                           bool hasModelerFormatVersion,
                           std::uint16_t modelerFormatVersionLimit,
                           SurfaceDwgPayloadInfo& info) {
    if (data.empty() || bitSize == 0
        || bitSize > static_cast<std::uint64_t>(data.size()) * 8u)
        return false;

    std::vector<std::uint8_t> payload = data;
    dwgBuffer buffer(payload.data(), payload.size());
    const std::uint64_t endBit = bitSize;
    bool acisEmpty = false;
    int acisVersion = 0;
    if (!surfaceDwgSkipAcis(buffer, payload, endBit, acisEmpty, acisVersion))
        return false;
    if (!acisEmpty
        && !surfaceDwgSkipCommon3dSolid(buffer, endBit, version, acisVersion))
        return false;

    std::uint16_t modelerFormatVersion = 0;
    std::uint16_t uIsolines = 0;
    std::uint16_t vIsolines = 0;
    if ((hasModelerFormatVersion
         && !surfaceDwgReadBitShort(buffer, endBit, modelerFormatVersion))
        || !surfaceDwgReadBitShort(buffer, endBit, uIsolines)
        || !surfaceDwgReadBitShort(buffer, endBit, vIsolines)) {
        return false;
    }
    if (hasModelerFormatVersion
        && modelerFormatVersion > modelerFormatVersionLimit)
        return false;

    if (type == DRW::EXTRUDEDSURFACE) {
        // DWG stores SweepOptions before the sweep vector and its transform;
        // keep the candidate isolated because the same fields have a
        // different order in the DXF subclass.
        dwgBuffer candidate = buffer;
        std::array<double, DRW_ExtrudedSurface::kTransformSize>
            sweepEntityTransform{};
        std::array<double, DRW_ExtrudedSurface::kTransformSize>
            pathEntityTransform{};
        std::array<double, DRW_ExtrudedSurface::kTransformSize>
            extrudedTransform{};
        double draftAngle = 0.0;
        double draftStartDistance = 0.0;
        double draftEndDistance = 0.0;
        double twistAngle = 0.0;
        double scaleFactor = 0.0;
        double alignAngle = 0.0;
        std::uint8_t solid = 0;
        std::uint16_t sweepAlignmentFlags = 0;
        std::uint16_t pathFlags = 0;
        std::uint8_t alignStart = 0;
        std::uint8_t bank = 0;
        std::uint8_t basePointSet = 0;
        std::uint8_t sweepEntityTransformComputed = 0;
        std::uint8_t pathEntityTransformComputed = 0;
        DRW_Coord referenceVector;
        DRW_Coord sweepVector;
        auto readMatrix = [&](std::array<double,
                              DRW_ExtrudedSurface::kTransformSize>& matrix) {
            for (double& value : matrix) {
                if (!surfaceDwgReadDouble(candidate, endBit, value))
                    return false;
            }
            return true;
        };
        bool valid = surfaceDwgReadDouble(candidate, endBit, draftAngle)
            && surfaceDwgReadDouble(candidate, endBit, draftStartDistance)
            && surfaceDwgReadDouble(candidate, endBit, draftEndDistance)
            && surfaceDwgReadDouble(candidate, endBit, twistAngle)
            && surfaceDwgReadDouble(candidate, endBit, scaleFactor)
            && surfaceDwgReadDouble(candidate, endBit, alignAngle)
            && readMatrix(sweepEntityTransform)
            && readMatrix(pathEntityTransform)
            && surfaceDwgReadBit(candidate, endBit, solid)
            && surfaceDwgReadBitShort(candidate, endBit, sweepAlignmentFlags)
            && surfaceDwgReadBitShort(candidate, endBit, pathFlags)
            && surfaceDwgReadBit(candidate, endBit, alignStart)
            && surfaceDwgReadBit(candidate, endBit, bank)
            && surfaceDwgReadBit(candidate, endBit, basePointSet)
            && surfaceDwgReadBit(candidate, endBit, sweepEntityTransformComputed)
            && surfaceDwgReadBit(candidate, endBit, pathEntityTransformComputed)
            && surfaceDwgRead3BitDouble(candidate, endBit, referenceVector)
            && surfaceDwgRead3BitDouble(candidate, endBit, sweepVector)
            && readMatrix(extrudedTransform);
        if (valid) {
            info.extrudedFieldsValid = true;
            info.extrudedSweepVector = sweepVector;
            info.extrudedTransform = extrudedTransform;
            info.extrudedSweepEntityTransform = sweepEntityTransform;
            info.extrudedPathEntityTransform = pathEntityTransform;
            info.extrudedDraftAngle = draftAngle;
            info.extrudedDraftStartDistance = draftStartDistance;
            info.extrudedDraftEndDistance = draftEndDistance;
            info.extrudedTwistAngle = twistAngle;
            info.extrudedScaleFactor = scaleFactor;
            info.extrudedAlignAngle = alignAngle;
            info.extrudedSolid = solid != 0;
            info.extrudedSweepAlignmentFlags = sweepAlignmentFlags;
            info.extrudedPathFlags = pathFlags;
            info.extrudedAlignStart = alignStart != 0;
            info.extrudedBank = bank != 0;
            info.extrudedBasePointSet = basePointSet != 0;
            info.extrudedSweepEntityTransformComputed =
                sweepEntityTransformComputed != 0;
            info.extrudedPathEntityTransformComputed =
                pathEntityTransformComputed != 0;
            info.extrudedReferenceVector = referenceVector;
        }
    } else if (type == DRW::LOFTEDSURFACE) {
        dwgBuffer candidate = buffer;
        std::array<double, DRW_LoftedSurface::kTransformSize>
            loftEntityTransform{};
        std::int32_t planeNormalLoftingType = 0;
        double startDraftAngle = 0.0;
        double endDraftAngle = 0.0;
        double startDraftMagnitude = 0.0;
        double endDraftMagnitude = 0.0;
        std::uint8_t arcLengthParameterization = 0;
        std::uint8_t noTwist = 0;
        std::uint8_t alignDirection = 0;
        std::uint8_t simpleSurfaces = 0;
        std::uint8_t closedSurfaces = 0;
        std::uint8_t solid = 0;
        std::uint8_t ruledSurface = 0;
        std::uint8_t virtualGuide = 0;
        std::uint16_t numCrossSections = 0;
        std::uint16_t numGuideCurves = 0;
        auto readMatrix = [&](std::array<double,
                              DRW_LoftedSurface::kTransformSize>& matrix) {
            for (double& value : matrix) {
                if (!surfaceDwgReadDouble(candidate, endBit, value))
                    return false;
            }
            return true;
        };
        const bool valid = readMatrix(loftEntityTransform)
            && surfaceDwgReadBitLong(candidate, endBit,
                                     planeNormalLoftingType)
            && surfaceDwgReadDouble(candidate, endBit, startDraftAngle)
            && surfaceDwgReadDouble(candidate, endBit, endDraftAngle)
            && surfaceDwgReadDouble(candidate, endBit, startDraftMagnitude)
            && surfaceDwgReadDouble(candidate, endBit, endDraftMagnitude)
            && surfaceDwgReadBit(candidate, endBit, arcLengthParameterization)
            && surfaceDwgReadBit(candidate, endBit, noTwist)
            && surfaceDwgReadBit(candidate, endBit, alignDirection)
            && surfaceDwgReadBit(candidate, endBit, simpleSurfaces)
            && surfaceDwgReadBit(candidate, endBit, closedSurfaces)
            && surfaceDwgReadBit(candidate, endBit, solid)
            && surfaceDwgReadBit(candidate, endBit, ruledSurface)
            && surfaceDwgReadBit(candidate, endBit, virtualGuide)
            && surfaceDwgReadBitShort(candidate, endBit, numCrossSections)
            && surfaceDwgReadBitShort(candidate, endBit, numGuideCurves)
            && numCrossSections <= DRW_LoftedSurface::kMaxReferenceTokenCount
            && numGuideCurves <= DRW_LoftedSurface::kMaxReferenceTokenCount;
        if (valid) {
            info.loftedFieldsValid = true;
            info.loftedEntityTransform = loftEntityTransform;
            info.loftedPlaneNormalLoftingType = planeNormalLoftingType;
            info.loftedStartDraftAngle = startDraftAngle;
            info.loftedEndDraftAngle = endDraftAngle;
            info.loftedStartDraftMagnitude = startDraftMagnitude;
            info.loftedEndDraftMagnitude = endDraftMagnitude;
            info.loftedArcLengthParameterization = arcLengthParameterization != 0;
            info.loftedNoTwist = noTwist != 0;
            info.loftedAlignDirection = alignDirection != 0;
            info.loftedSimpleSurfaces = simpleSurfaces != 0;
            info.loftedClosedSurfaces = closedSurfaces != 0;
            info.loftedSolid = solid != 0;
            info.loftedRuledSurface = ruledSurface != 0;
            info.loftedVirtualGuide = virtualGuide != 0;
            info.loftedNumCrossSections = numCrossSections;
            info.loftedNumGuideCurves = numGuideCurves;
        }
    } else if (type == DRW::SWEPTSURFACE) {
        dwgBuffer candidate = buffer;
        std::int32_t classVersion = 0;
        std::int32_t sweepEntityId = 0;
        std::int32_t sweepDataSize = 0;
        std::int32_t pathEntityId = 0;
        std::int32_t pathDataSize = 0;
        std::vector<std::uint8_t> sweepData;
        std::vector<std::uint8_t> pathData;
        std::array<double, DRW_SweptSurface::kTransformSize>
            sweepEntityTransform{};
        std::array<double, DRW_SweptSurface::kTransformSize>
            pathEntityTransform{};
        double draftAngle = 0.0;
        double draftStartDistance = 0.0;
        double draftEndDistance = 0.0;
        double twistAngle = 0.0;
        double scaleFactor = 1.0;
        double alignAngle = 0.0;
        std::uint8_t solid = 0;
        std::uint16_t sweepAlignmentFlags = 0;
        std::uint16_t pathFlags = 0;
        std::uint8_t alignStart = 0;
        std::uint8_t bank = 0;
        std::uint8_t basePointSet = 0;
        std::uint8_t sweepEntityTransformComputed = 0;
        std::uint8_t pathEntityTransformComputed = 0;
        DRW_Coord referenceVector;
        auto readMatrix = [&](std::array<double,
                              DRW_SweptSurface::kTransformSize>& matrix) {
            for (double& value : matrix) {
                if (!surfaceDwgReadDouble(candidate, endBit, value))
                    return false;
            }
            return true;
        };
        bool valid = surfaceDwgReadBitLong(candidate, endBit, classVersion)
            && classVersion >= 0 && classVersion <= 10
            && surfaceDwgReadBitLong(candidate, endBit, sweepEntityId)
            && sweepEntityId >= 0
            && surfaceDwgReadBitLong(candidate, endBit, sweepDataSize)
            && surfaceDwgReadBytes(candidate, endBit, sweepDataSize,
                                    DRW_SweptSurface::kMaxSweepDataSize,
                                    sweepData)
            && surfaceDwgReadBitLong(candidate, endBit, pathEntityId)
            && pathEntityId >= 0
            && surfaceDwgReadBitLong(candidate, endBit, pathDataSize)
            && surfaceDwgReadBytes(candidate, endBit, pathDataSize,
                                    DRW_SweptSurface::kMaxSweepDataSize,
                                    pathData)
            && surfaceDwgReadDouble(candidate, endBit, draftAngle)
            && surfaceDwgReadDouble(candidate, endBit, draftStartDistance)
            && surfaceDwgReadDouble(candidate, endBit, draftEndDistance)
            && surfaceDwgReadDouble(candidate, endBit, twistAngle)
            && surfaceDwgReadDouble(candidate, endBit, scaleFactor)
            && surfaceDwgReadDouble(candidate, endBit, alignAngle)
            && readMatrix(sweepEntityTransform)
            && readMatrix(pathEntityTransform)
            && surfaceDwgReadBit(candidate, endBit, solid)
            && surfaceDwgReadBitShort(candidate, endBit, sweepAlignmentFlags)
            && surfaceDwgReadBitShort(candidate, endBit, pathFlags)
            && surfaceDwgReadBit(candidate, endBit, alignStart)
            && surfaceDwgReadBit(candidate, endBit, bank)
            && surfaceDwgReadBit(candidate, endBit, basePointSet)
            && surfaceDwgReadBit(candidate, endBit, sweepEntityTransformComputed)
            && surfaceDwgReadBit(candidate, endBit, pathEntityTransformComputed)
            && surfaceDwgRead3BitDouble(candidate, endBit, referenceVector);
        if (valid) {
            info.sweptFieldsValid = true;
            info.sweptClassVersion = static_cast<std::uint32_t>(classVersion);
            info.sweptEntityId = static_cast<std::uint32_t>(sweepEntityId);
            info.sweptData = std::move(sweepData);
            info.sweptPathEntityId = static_cast<std::uint32_t>(pathEntityId);
            info.sweptPathData = std::move(pathData);
            info.sweptEntityTransform = sweepEntityTransform;
            info.sweptPathEntityTransform = pathEntityTransform;
            info.sweptDraftAngle = draftAngle;
            info.sweptDraftStartDistance = draftStartDistance;
            info.sweptDraftEndDistance = draftEndDistance;
            info.sweptTwistAngle = twistAngle;
            info.sweptScaleFactor = scaleFactor;
            info.sweptAlignAngle = alignAngle;
            info.sweptSolid = solid != 0;
            info.sweptAlignmentFlags = sweepAlignmentFlags;
            info.sweptPathFlags = pathFlags;
            info.sweptAlignStart = alignStart != 0;
            info.sweptBank = bank != 0;
            info.sweptBasePointSet = basePointSet != 0;
            info.sweptEntityTransformComputed =
                sweepEntityTransformComputed != 0;
            info.sweptPathEntityTransformComputed =
                pathEntityTransformComputed != 0;
            info.sweptReferenceVector = referenceVector;
        }
    } else if (type == DRW::NURBSURFACE && version >= DRW::AC1027) {
        dwgBuffer candidate = buffer;
        std::uint16_t short170 = 0;
        std::uint8_t cvHullDisplay = 0;
        DRW_Coord uvec1;
        DRW_Coord vvec1;
        DRW_Coord uvec2;
        DRW_Coord vvec2;
        const bool valid = surfaceDwgReadBitShort(candidate, endBit, short170)
            && surfaceDwgReadBit(candidate, endBit, cvHullDisplay)
            && surfaceDwgRead3BitDouble(candidate, endBit, uvec1)
            && surfaceDwgRead3BitDouble(candidate, endBit, vvec1)
            && surfaceDwgRead3BitDouble(candidate, endBit, uvec2)
            && surfaceDwgRead3BitDouble(candidate, endBit, vvec2);
        if (valid) {
            info.nurbsFieldsValid = true;
            info.nurbsShort170 = short170;
            info.nurbsCvHullDisplay = cvHullDisplay != 0;
            info.nurbsUvec1 = uvec1;
            info.nurbsVvec1 = vvec1;
            info.nurbsUvec2 = uvec2;
            info.nurbsVvec2 = vvec2;
        }
    } else if (type == DRW::REVOLVEDSURFACE) {
        // The AcDbRevolvedSurface tail is optional from the carrier's point
        // of view: retain the complete bounded body when a producer omits or
        // truncates it, but publish the typed values only as one transaction.
        dwgBuffer candidate = buffer;
        std::int32_t classVersion = 0;
        std::int32_t id = 0;
        DRW_Coord axisPoint;
        DRW_Coord axisVector;
        std::array<double, DRW_RevolvedSurface::kTransformSize> transform{};
        double revolveAngle = 0.0;
        double startAngle = 0.0;
        double draftAngle = 0.0;
        double draftStartDistance = 0.0;
        double draftEndDistance = 0.0;
        double twistAngle = 0.0;
        std::uint8_t solid = 0;
        std::uint8_t closeToAxis = 0;
        bool valid = surfaceDwgReadBitLong(candidate, endBit, classVersion)
            && classVersion >= 0 && classVersion <= 10
            && surfaceDwgReadBitLong(candidate, endBit, id)
            && id >= 0
            && surfaceDwgRead3BitDouble(candidate, endBit, axisPoint)
            && surfaceDwgRead3BitDouble(candidate, endBit, axisVector)
            && surfaceDwgReadDouble(candidate, endBit, revolveAngle)
            && surfaceDwgReadDouble(candidate, endBit, startAngle);
        for (double& value : transform) {
            valid = valid && surfaceDwgReadDouble(candidate, endBit, value);
        }
        valid = valid
            && surfaceDwgReadDouble(candidate, endBit, draftAngle)
            && surfaceDwgReadDouble(candidate, endBit, draftStartDistance)
            && surfaceDwgReadDouble(candidate, endBit, draftEndDistance)
            && surfaceDwgReadDouble(candidate, endBit, twistAngle)
            && surfaceDwgReadBit(candidate, endBit, solid)
            && surfaceDwgReadBit(candidate, endBit, closeToAxis);
        if (valid) {
            info.revolvedFieldsValid = true;
            info.revolvedClassId = static_cast<std::uint32_t>(classVersion);
            info.revolvedId = static_cast<std::uint32_t>(id);
            info.revolvedAxisPoint = axisPoint;
            info.revolvedAxisVector = axisVector;
            info.revolvedAngle = revolveAngle;
            info.revolvedStartAngle = startAngle;
            info.revolvedTransform = transform;
            info.revolvedDraftAngle = draftAngle;
            info.revolvedDraftStartDistance = draftStartDistance;
            info.revolvedDraftEndDistance = draftEndDistance;
            info.revolvedTwistAngle = twistAngle;
            info.revolvedSolid = solid != 0;
            info.revolvedCloseToAxis = closeToAxis != 0;
        }
    }

    info.acisEmpty = acisEmpty;
    info.acisVersion = acisVersion;
    info.modelerFormatVersion = static_cast<int>(modelerFormatVersion);
    info.uIsolines = static_cast<int>(uIsolines);
    info.vIsolines = static_cast<int>(vIsolines);
    return true;
}

int proxyEntityDxfCode(std::uint8_t handleCode) {
    switch (handleCode) {
    case 3: return 340;
    case 4:
    case 6:
    case 8:
    case 10:
    case 12: return 350;
    case 5: return 360;
    default: return 330;
    }
}

DRW_DwgSubrecordRange makeDwgSubrecordRange(const char *name, std::uint64_t startBit,
                                            std::uint64_t endBit, DRW::Version version,
                                            std::uint32_t count, bool parseComplete) {
    DRW_DwgSubrecordRange range;
    range.m_name = name;
    range.m_startBit = startBit;
    range.m_bitSize = endBit >= startBit ? endBit - startBit : 0;
    range.m_version = version;
    range.m_count = count;
    range.m_parseComplete = parseComplete;
    return range;
}

bool isValidSplineDegree(int degree) {
    return degree >= 1 && degree <= kMaxSplineDegree;
}

bool isValidControlSplineLayout(int degree, std::int32_t knotCount, std::int32_t controlCount) {
    if (!isValidSplineDegree(degree) || !isValidCount(knotCount, kMaxSplineItems) ||
        !isValidCount(controlCount, kMaxSplineItems)) {
        return false;
    }

    if (controlCount < degree + 1) {
        return false;
    }

    const std::int64_t expectedKnots = static_cast<std::int64_t>(controlCount) + degree + 1;
    return expectedKnots <= kMaxSplineItems && knotCount == expectedKnots;
}

bool isValidFitSplineLayout(int degree, std::int32_t fitCount) {
    return isValidSplineDegree(degree) && isValidCount(fitCount, kMaxSplineItems) &&
           fitCount >= 2;
}

bool differsFromUnitWeight(double weight) {
    return std::fabs(weight - 1.0) > 1e-12;
}

//! \brief Compare two doubles by stored representation.
//! For values a compact DWG form restores verbatim, the match has to be exact:
//! dataFlags 3 makes the reader return exactly 1.0 and dataFlags 2 makes it
//! return xscale for y and z (see DRW_Insert::parseDwg), so picking those forms
//! on a tolerant match would silently round the scale that gets written. This
//! asks the question that is actually meant - "will the reader reconstruct this
//! value?" - without floating point comparison semantics, and as a side effect
//! keeps -0.0 distinct from 0.0, which `==` does not.
bool sameStoredDouble(double a, double b) {
    return std::memcmp(&a, &b, sizeof a) == 0;
}

void putDwgReference(dwgBufferW *buf, std::uint8_t code,
                     std::uint32_t ref) {
    dwgHandle h;
    h.code = code;
    h.ref = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) {
            t >>= 8;
            ++h.size;
        }
    }
    buf->putHandle(h);
}

void putHardPointerHandle(dwgBufferW *buf, std::uint32_t ref) {
    putDwgReference(buf, DRW::DwgHardPointer, ref);
}

void appendBitBuffer(dwgBufferW *destination, const dwgBufferW& source) {
    if (destination == nullptr)
        return;
    const auto& bytes = source.data();
    for (std::uint32_t bit = 0; bit < source.bitCount(); ++bit) {
        const std::uint8_t value =
            static_cast<std::uint8_t>((bytes[bit / 8] >> (7 - (bit % 8))) & 1);
        destination->putBit(value);
    }
}

void putNullableHardPointerHandle(dwgBufferW *buf, std::uint32_t ref) {
    dwgHandle h;
    h.code = ref == 0 ? 0 : 5;
    h.ref = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) {
            t >>= 8;
            ++h.size;
        }
    }
    buf->putHandle(h);
}

void putSurfaceHandle(dwgBufferW *buf, std::uint32_t ref) {
    dwgHandle h;
    h.code = 5;
    h.ref = ref;
    h.size = 0;
    for (std::uint32_t value = ref; value != 0; value >>= 8)
        ++h.size;
    buf->putHandle(h);
}

std::size_t surfaceRawBitCapacity(const std::vector<std::uint8_t>& data) {
    constexpr std::size_t maxBitSize =
        std::numeric_limits<std::uint32_t>::max();
    constexpr std::size_t maxBytes = maxBitSize / 8u;
    return data.size() > maxBytes ? maxBitSize : data.size() * 8u;
}

bool putSurfaceRawBits(dwgBufferW *buf, const std::vector<std::uint8_t>& data,
                       std::uint32_t bitSize) {
    const std::size_t bitCapacity = surfaceRawBitCapacity(data);
    if (buf == nullptr || bitSize == 0 || bitSize > bitCapacity)
        return false;
    for (std::uint32_t bit = 0; bit < bitSize; ++bit) {
        const std::uint8_t byte = data[bit >> 3];
        buf->putBit((byte >> (7u - (bit & 7u))) & 1u);
    }
    return true;
}

bool finiteSurfaceCoord(const DRW_Coord& value) {
    return std::isfinite(value.x) && std::isfinite(value.y)
        && std::isfinite(value.z);
}

template <std::size_t N>
bool finiteSurfaceMatrix(const std::array<double, N>& values) {
    return std::all_of(values.begin(), values.end(),
                       [](double value) { return std::isfinite(value); });
}

std::uint16_t bitShortFromInt(int value) {
    if (value < 0)
        return 0;
    if (value > 0xffff)
        return 0xffff;
    return static_cast<std::uint16_t>(value);
}

std::uint32_t readTableHandle(
    dwgBuffer *hdlBuf,
    std::uint64_t endBit = std::numeric_limits<std::uint64_t>::max()) {
    if (hdlBuf == nullptr || !hdlBuf->isGood())
        return 0;
    dwgBuffer probe = hdlBuf->forkIndependent();
    dwgHandle h;
    if (!readBoundedDwgHandle(probe, endBit, 0, false, h)) {
        hdlBuf->invalidate();
        return 0;
    }
    *hdlBuf = probe;
    return h.ref;
}

void seekTableObjectHandleStream(DRW::Version version, dwgBuffer *buf, std::uint32_t objSize) {
    if (version > DRW::AC1018) {
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
}

bool readTableObjectCommonHandles(dwgBuffer *buf, std::uint32_t baseHandle,
                                  std::int32_t numReactors, std::uint8_t xDictFlag,
                                  std::uint32_t *parentHandle,
                                  std::vector<std::uint32_t> *reactorHandles,
                                  std::uint32_t *xDictHandle,
                                  std::uint64_t handleEndBit =
                                      std::numeric_limits<std::uint64_t>::max()) {
    if (buf == nullptr || !buf->isGood()
        || !dwgSafety::validReactorCount(numReactors)) {
        if (buf != nullptr)
            buf->invalidate();
        return false;
    }
    std::vector<std::uint32_t> parsedReactors;
    dwgHandle parentH;
    if (!readBoundedDwgHandle(*buf, handleEndBit, baseHandle, true, parentH))
        return false;
    if (!DRW::reserve(parsedReactors, numReactors))
        return false;
    for (std::int32_t i = 0; i < numReactors; ++i) {
        dwgHandle reactor;
        if (!readBoundedDwgHandle(*buf, handleEndBit, baseHandle, true,
                                  reactor))
            return false;
        parsedReactors.push_back(reactor.ref);
    }

    std::uint32_t parsedXDictHandle = 0;
    if (xDictFlag != 1) {
        dwgHandle xDict;
        if (!readBoundedDwgHandle(*buf, handleEndBit, baseHandle, true,
                                  xDict))
            return false;
        parsedXDictHandle = xDict.ref;
    }
    if (parentHandle)
        *parentHandle = parentH.ref;
    if (reactorHandles)
        *reactorHandles = std::move(parsedReactors);
    if (xDictHandle)
        *xDictHandle = parsedXDictHandle;
    return true;
}

bool readTableValueBytes(dwgBuffer *buf, std::uint64_t bodyEndBit,
                         std::vector<std::uint8_t>& raw, const char *label) {
    std::int32_t parsedByteCount = 0;
    if (buf == nullptr
        || !readBoundedBitLong(*buf, bodyEndBit, parsedByteCount)
        || parsedByteCount < 0
        || static_cast<std::uint32_t>(parsedByteCount) > kMaxTableStringBytes) {
        if (buf != nullptr)
            buf->invalidate();
        return false;
    }
    const std::uint32_t byteCount = static_cast<std::uint32_t>(parsedByteCount);
    if (byteCount > kMaxTableStringBytes) {
        DRW_DBG(label); DRW_DBG(" too large: "); DRW_DBG(byteCount); DRW_DBG("\n");
        return false;
    }
    if (!DRW::resize(raw, static_cast<int>(byteCount)))
        return false;
    const bool good = readBoundedBytes(*buf, bodyEndBit, raw.data(), raw.size());
    if (!good) {
        DRW_DBG(label); DRW_DBG(" byte payload read failed, size: "); DRW_DBG(byteCount);
        DRW_DBG(" remaining: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
    return good;
}

UTF8STRING decodeTableValueText(DRW::Version version, dwgBuffer *buf, const std::vector<std::uint8_t>& raw) {
    if (raw.empty())
        return UTF8STRING();
    std::string s(reinterpret_cast<const char*>(raw.data()), raw.size());
    if (version > DRW::AC1018 && s.size() >= 2 && s[s.size() - 1] == '\0'
        && s[s.size() - 2] == '\0') {
        s.resize(s.size() - 2);
    } else {
        while (!s.empty() && s.back() == '\0')
            s.pop_back();
    }
    if (buf->decoder)
        s = buf->decoder->toUtf8(s);
    return s;
}

UTF8STRING readTableText(DRW::Version version, dwgBuffer *buf,
                         std::uint64_t endBit) {
    if (!buf)
        return UTF8STRING();
    if (version <= DRW::AC1018) {
        UTF8STRING value;
        if (!readBoundedVariableText(*buf, endBit, version, value))
            buf->invalidate();
        return value;
    }

    std::uint16_t parsedByteLen = 0;
    if (!readBoundedBitShort(*buf, endBit, parsedByteLen)) {
        buf->invalidate();
        return UTF8STRING();
    }
    const std::uint32_t byteLen = parsedByteLen;
    if (byteLen == 0)
        return UTF8STRING();
    if (byteLen > kMaxTableStringBytes) {
        DRW_DBG("TABLE text byte length invalid: "); DRW_DBG(byteLen); DRW_DBG("\n");
        return UTF8STRING();
    }

    std::vector<std::uint8_t> raw;
    if (!DRW::resize(raw, static_cast<int>(byteLen + 2)))
        return UTF8STRING();
    std::fill(raw.begin(), raw.end(), 0);
    if (!readBoundedBytes(*buf, endBit, raw.data(), byteLen)) {
        buf->invalidate();
        return UTF8STRING();
    }

    std::string s(reinterpret_cast<const char*>(raw.data()), byteLen);
    if (buf->decoder)
        s = buf->decoder->toUtf8(s);
    return s;
}

bool readTableValuePoint(dwgBuffer *buf, std::uint64_t bodyEndBit,
                         DRW_CadValue& value, int dimensions) {
    std::int32_t parsedDataSize = 0;
    if (buf == nullptr
        || !readBoundedBitLong(*buf, bodyEndBit, parsedDataSize)
        || parsedDataSize < 0) {
        if (buf != nullptr)
            buf->invalidate();
        return false;
    }
    value.m_dataSize = static_cast<std::uint32_t>(parsedDataSize);
    const std::uint32_t expectedSize = static_cast<std::uint32_t>(dimensions) * 8;
    if (value.m_dataSize > kMaxTableStringBytes)
        return false;
    if (value.m_dataSize < expectedSize) {
        if (!DRW::resize(value.m_rawData,
                         static_cast<int>(value.m_dataSize)))
            return false;
        if (!readBoundedBytes(*buf, bodyEndBit, value.m_rawData.data(),
                              value.m_rawData.size()))
            return false;
        value.m_value.addBinary(310, value.m_rawData);
        return true;
    }

    DRW_Coord c;
    if (!readBoundedRawDouble(*buf, bodyEndBit, c.x)
        || !readBoundedRawDouble(*buf, bodyEndBit, c.y)
        || (dimensions == 3
            && !readBoundedRawDouble(*buf, bodyEndBit, c.z)))
        return false;
    if (dimensions != 3)
        c.z = 0.0;
    value.m_value.addCoord(11, c);

    const std::uint32_t extraBytes = value.m_dataSize - expectedSize;
    if (!DRW::resize(value.m_rawData, static_cast<int>(extraBytes)))
        return false;
    return readBoundedBytes(*buf, bodyEndBit, value.m_rawData.data(),
                            value.m_rawData.size());
}

bool readTableCadValue(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf,
                       dwgBuffer *hdlBuf, DRW_CadValue& value,
                       const TableDwgBounds& bounds) {
    if (version > DRW::AC1018) {
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, value.m_formatFlags))
            return false;
    }

    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, value.m_dataType))
        return false;
    const bool emptyR2007Value = version > DRW::AC1018 && (value.m_formatFlags & 3);
    if (!emptyR2007Value) {
        switch (value.m_dataType) {
        case 0:
        case 1: {
            std::int32_t intValue = 0;
            if (!readBoundedBitLong(*buf, bounds.bodyEndBit, intValue))
                return false;
            value.m_value.addInt(91, intValue);
            break;
        }
        case 2: {
            double doubleValue = 0.0;
            if (!readBoundedBitDouble(*buf, bounds.bodyEndBit, doubleValue))
                return false;
            value.m_value.addDouble(140, doubleValue);
            break;
        }
        case 4:
        case 512:
            if (!readTableValueBytes(buf, bounds.bodyEndBit, value.m_rawData,
                                     "TABLE value byte payload"))
                return false;
            value.m_dataSize = static_cast<std::uint32_t>(value.m_rawData.size());
            value.m_value.addString(1, decodeTableValueText(version, buf, value.m_rawData));
            break;
        case 8: {
            if (!readTableValueBytes(buf, bounds.bodyEndBit, value.m_rawData,
                                     "TABLE value date payload"))
                return false;
            value.m_dataSize = static_cast<std::uint32_t>(value.m_rawData.size());
            value.m_value.addBinary(310, value.m_rawData);
            break;
        }
        case 16:
            if (!readTableValuePoint(buf, bounds.bodyEndBit, value, 2))
                return false;
            break;
        case 32:
            if (!readTableValuePoint(buf, bounds.bodyEndBit, value, 3))
                return false;
            break;
        case 64:
            value.m_handle = readTableHandle(hdlBuf, bounds.handleEndBit);
            value.m_value.addInt(330, static_cast<std::uint32_t>(value.m_handle));
            break;
        case 128:
        case 256:
            DRW_DBG("unsupported TABLE CadValue buffer data type: "); DRW_DBG(value.m_dataType); DRW_DBG("\n");
            return false;
        default:
            DRW_DBG("unsupported TABLE CadValue data type: "); DRW_DBG(value.m_dataType); DRW_DBG("\n");
            return false;
        }
    }

    if (version > DRW::AC1018) {
        dwgBuffer *textBuf = strBuf ? strBuf : buf;
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, value.m_unitType))
            return false;
        const std::uint64_t textEndBit = tableTextEndBit(buf, textBuf, bounds);
        value.m_formatString = readTableText(version, textBuf, textEndBit);
        value.m_valueString = readTableText(version, textBuf, textEndBit);
    }

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (!good) {
        DRW_DBG("TABLE CadValue stream failed, flags: "); DRW_DBG(value.m_formatFlags);
        DRW_DBG(" type: "); DRW_DBG(value.m_dataType);
        DRW_DBG(" unit: "); DRW_DBG(value.m_unitType);
        DRW_DBG(" bufGood: "); DRW_DBG(buf->isGood() ? 1 : 0);
        DRW_DBG(" strGood: "); DRW_DBG((!strBuf || strBuf->isGood()) ? 1 : 0);
        DRW_DBG(" hdlGood: "); DRW_DBG((!hdlBuf || hdlBuf->isGood()) ? 1 : 0);
        DRW_DBG(" bufPos: "); DRW_DBG(buf->getPosition());
        DRW_DBG(" strPos: "); if (strBuf) DRW_DBG(strBuf->getPosition()); else DRW_DBG(-1);
        DRW_DBG(" hdlPos: "); if (hdlBuf) DRW_DBG(hdlBuf->getPosition()); else DRW_DBG(-1);
        DRW_DBG("\n");
    }
    return good;
}

bool skipTableCustomData(DRW::Version version, dwgBuffer *buf,
                         dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                         const TableDwgBounds& bounds) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    UTF8STRING key = readTableText(
        version, textBuf, tableTextEndBit(buf, textBuf, bounds));
    if (!textBuf->isGood()) {
        DRW_DBG("TABLE custom data key string read failed\n");
        return false;
    }
    DRW_CadValue value;
    const bool good = readTableCadValue(version, buf, strBuf, hdlBuf, value,
                                        bounds);
    if (!good) {
        DRW_DBG("TABLE custom data key failed: "); DRW_DBG(key.c_str()); DRW_DBG("\n");
    }
    return good;
}

bool readTableCmColor(DRW::Version version, dwgBuffer *buf,
                      dwgBuffer *strBuf, const TableDwgBounds& bounds) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    if (version < DRW::AC1018) {
        std::uint16_t ignoredColor = 0;
        return readBoundedBitShort(*buf, bounds.bodyEndBit, ignoredColor);
    }

    std::uint16_t ignoredColorType = 0;
    std::int32_t rgb = 0;
    std::uint8_t colorFlags = 0;
    if (!readBoundedBitShort(*buf, bounds.bodyEndBit, ignoredColorType)
        || !readBoundedBitLong(*buf, bounds.bodyEndBit, rgb)
        || !readBoundedRawChar8(*buf, bounds.bodyEndBit, colorFlags))
        return false;
    DRW_DBG("\ntype COLOR: "); DRW_DBGH(rgb >> 24);
    DRW_DBG("\nRGB COLOR: "); DRW_DBGH(rgb);
    DRW_DBG("\nbyte COLOR: "); DRW_DBGH(colorFlags);
    const std::uint64_t textEndBit = tableTextEndBit(buf, textBuf, bounds);
    if (colorFlags & 1)
        readTableText(version, textBuf, textEndBit);
    if (colorFlags & 2)
        readTableText(version, textBuf, textEndBit);
    return textBuf->isGood();
}

bool skipR2007TableCellOverrides(DRW::Version version, dwgBuffer *buf,
                                 dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                                 DRW_TableCell& cell,
                                 std::vector<DRW_DwgSubrecordRange> *ranges,
                                 const TableDwgBounds& bounds) {
    const std::uint64_t startBit = currentDwgBit(buf);
    std::int32_t overrideFlags = 0;
    std::uint8_t virtualEdgeFlags = 0;
    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, overrideFlags)
        || !readBoundedRawChar8(*buf, bounds.bodyEndBit, virtualEdgeFlags))
        return false;
    cell.m_overrideFlags = static_cast<std::uint32_t>(overrideFlags);
    cell.m_virtualEdgeFlags = virtualEdgeFlags;

    if (cell.m_overrideFlags & 0x00001
        && !skipTableRawShort16(*buf, bounds.bodyEndBit))
        return false;
    if (cell.m_overrideFlags & 0x00002
        && !skipTableBit(*buf, bounds.bodyEndBit))
        return false;
    if ((cell.m_overrideFlags & 0x00004)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if ((cell.m_overrideFlags & 0x00008)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if (cell.m_overrideFlags & 0x00010)
        cell.m_textStyleOverrideHandle = readTableHandle(hdlBuf, bounds.handleEndBit);
    if (cell.m_overrideFlags & 0x00020
        && !skipTableBitDouble(*buf, bounds.bodyEndBit))
        return false;
    if ((cell.m_overrideFlags & 0x00040)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if (cell.m_overrideFlags & 0x00400
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if (cell.m_overrideFlags & 0x04000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if ((cell.m_overrideFlags & 0x00080)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if (cell.m_overrideFlags & 0x00800
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if (cell.m_overrideFlags & 0x08000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if ((cell.m_overrideFlags & 0x00100)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if (cell.m_overrideFlags & 0x01000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if (cell.m_overrideFlags & 0x10000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if ((cell.m_overrideFlags & 0x00200)
        && !readTableCmColor(version, buf, strBuf, bounds))
        return false;
    if (cell.m_overrideFlags & 0x02000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;
    if (cell.m_overrideFlags & 0x20000
        && !skipTableBitShort(*buf, bounds.bodyEndBit))
        return false;

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr) {
        ranges->push_back(makeDwgSubrecordRange(
            "r2007-table-cell-overrides", startBit, currentDwgBit(buf),
            version, cell.m_overrideFlags, good));
    }
    return good;
}

bool parseR2007TableCell(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf,
                         dwgBuffer *hdlBuf, DRW_TableCell& cell,
                         std::vector<DRW_DwgSubrecordRange> *ranges,
    const TableDwgBounds& bounds) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    std::uint16_t type = 0;
    std::uint8_t edgeFlags = 0;
    if (!readBoundedBitShort(*buf, bounds.bodyEndBit, type)
        || !readBoundedRawChar8(*buf, bounds.bodyEndBit, edgeFlags)
        || !readBoundedBit(*buf, bounds.bodyEndBit, cell.m_isMerged)
        || !readBoundedBit(*buf, bounds.bodyEndBit, cell.m_autoFit)
        || !readBoundedBitLong(*buf, bounds.bodyEndBit, cell.m_mergedWidth)
        || !readBoundedBitLong(*buf, bounds.bodyEndBit, cell.m_mergedHeight)
        || !readBoundedBitDouble(*buf, bounds.bodyEndBit, cell.m_rotation))
        return false;
    cell.m_type = type;
    cell.m_edgeFlags = edgeFlags;
    cell.m_valueHandle = readTableHandle(hdlBuf, bounds.handleEndBit);

    if (cell.m_type == 1) {
        cell.m_textStyleHandle = cell.m_valueHandle;
        if (cell.m_textStyleHandle == 0 && version < DRW::AC1021) {
            DRW_TableCellContent content;
            content.m_type = 1;
            content.m_text = readTableText(
                version, textBuf, tableTextEndBit(buf, textBuf, bounds));
            content.m_value.m_dataType = 4;
            content.m_value.m_value.addString(1, content.m_text);
            cell.m_contents.push_back(content);
        }
    } else if (cell.m_type == 2) {
        cell.m_blockHandle = cell.m_valueHandle;
        if (!readBoundedBitDouble(*buf, bounds.bodyEndBit, cell.m_blockScale))
            return false;
        bool hasAttributes = false;
        if (!readBoundedBit(*buf, bounds.bodyEndBit, hasAttributes))
            return false;
        if (hasAttributes) {
            std::uint16_t numAttributes = 0;
            if (!readBoundedBitShort(*buf, bounds.bodyEndBit,
                                     numAttributes)
                || numAttributes > kMaxTableCellAttributes)
                return false;
            if (!DRW::reserve(cell.m_attributes, numAttributes))
                return false;
            for (std::uint16_t i = 0; i < numAttributes; ++i) {
                DRW_TableCellAttribute attribute;
                attribute.m_attdefHandle = readTableHandle(
                    hdlBuf, bounds.handleEndBit);
                std::uint16_t attributeIndex = 0;
                if (!readBoundedBitShort(*buf, bounds.bodyEndBit,
                                         attributeIndex))
                    return false;
                attribute.m_index = attributeIndex;
                attribute.m_text = readTableText(
                    version, textBuf, tableTextEndBit(buf, textBuf, bounds));
                cell.m_attributes.push_back(attribute);
            }
        }

        DRW_TableCellContent content;
        content.m_type = 4;
        content.m_handle = cell.m_blockHandle;
        cell.m_contents.push_back(content);
    }

    bool hasOverrides = false;
    if (!readBoundedBit(*buf, bounds.bodyEndBit, hasOverrides))
        return false;
    if (hasOverrides
        && !skipR2007TableCellOverrides(version, buf, strBuf, hdlBuf, cell,
                                        ranges, bounds))
        return false;

    if (version > DRW::AC1018) {
        if (!skipTableBitLong(*buf, bounds.bodyEndBit))
            return false;
        DRW_TableCellContent content;
        content.m_type = 1;
        if (!readTableCadValue(version, buf, strBuf, hdlBuf, content.m_value,
                               bounds))
            return false;
        if (content.m_value.m_value.type() == DRW_Variant::STRING)
            content.m_text = content.m_value.m_value.c_str();
        else if (!content.m_value.m_valueString.empty())
            content.m_text = content.m_value.m_valueString;
        cell.m_contents.push_back(content);
    }

    return buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
}

bool skipR2007TableOverrides(DRW::Version version, dwgBuffer *buf,
                             dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                             std::vector<DRW_DwgSubrecordRange> *ranges,
                             const TableDwgBounds& bounds) {
    const std::uint64_t startBit = currentDwgBit(buf);
    std::uint32_t maskCount = 0;
    auto readFlags = [&](std::uint32_t& flags) {
        std::int32_t parsed = 0;
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, parsed))
            return false;
        flags = static_cast<std::uint32_t>(parsed);
        return true;
    };
    bool hasMask = false;
    if (!readBoundedBit(*buf, bounds.bodyEndBit, hasMask))
        return false;
    if (hasMask) {
        std::uint32_t flags = 0;
        if (!readFlags(flags))
            return false;
        ++maskCount;
        if (flags & 0x000001)
            if (!skipTableBit(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x000004)
            if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x000008)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x000010)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
        if ((flags & 0x000020)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if ((flags & 0x000040)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if ((flags & 0x000080)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if (flags & 0x000100)
            if (!skipTableBit(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x000200)
            if (!skipTableBit(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x000400)
            if (!skipTableBit(*buf, bounds.bodyEndBit))
                return false;
        if ((flags & 0x000800)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if ((flags & 0x001000)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if ((flags & 0x002000)
            && !readTableCmColor(version, buf, strBuf, bounds))
            return false;
        if (flags & 0x004000)
            if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x008000)
            if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x010000)
            if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x020000)
            if (readTableHandle(hdlBuf, bounds.handleEndBit) == 0
                && hdlBuf != nullptr && !hdlBuf->isGood())
                return false;
        if (flags & 0x040000)
            if (readTableHandle(hdlBuf, bounds.handleEndBit) == 0
                && hdlBuf != nullptr && !hdlBuf->isGood())
                return false;
        if (flags & 0x080000)
            if (readTableHandle(hdlBuf, bounds.handleEndBit) == 0
                && hdlBuf != nullptr && !hdlBuf->isGood())
                return false;
        if (flags & 0x100000)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x200000)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
        if (flags & 0x400000)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
    }

    if (!readBoundedBit(*buf, bounds.bodyEndBit, hasMask))
        return false;
    if (hasMask) {
        std::uint32_t flags = 0;
        if (!readFlags(flags))
            return false;
        ++maskCount;
        for (int i = 0; i < 18; ++i) {
            if (flags & (1u << i)
                && !readTableCmColor(version, buf, strBuf, bounds))
                return false;
        }
    }

    if (!readBoundedBit(*buf, bounds.bodyEndBit, hasMask))
        return false;
    if (hasMask) {
        std::uint32_t flags = 0;
        if (!readFlags(flags))
            return false;
        ++maskCount;
        for (int i = 0; i < 18; ++i) {
            if (flags & (1u << i))
                if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                    return false;
        }
    }

    if (!readBoundedBit(*buf, bounds.bodyEndBit, hasMask))
        return false;
    if (hasMask) {
        std::uint32_t flags = 0;
        if (!readFlags(flags))
            return false;
        ++maskCount;
        for (int i = 0; i < 18; ++i) {
            if (flags & (1u << i))
                if (!skipTableBitShort(*buf, bounds.bodyEndBit))
                    return false;
        }
    }

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr && (maskCount != 0 || currentDwgBit(buf) != startBit)) {
        ranges->push_back(makeDwgSubrecordRange(
            "r2007-table-overrides", startBit, currentDwgBit(buf),
            version, maskCount, good));
    }
    return good;
}

bool skipTableContentFormat(DRW::Version version, dwgBuffer *buf,
                            dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                            std::vector<DRW_DwgSubrecordRange> *ranges,
                            const TableDwgBounds& bounds) {
    const std::uint64_t startBit = currentDwgBit(buf);
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    if (!skipTableBitLong(*buf, bounds.bodyEndBit) // property override flags
        || !skipTableBitLong(*buf, bounds.bodyEndBit) // property flags
        || !skipTableBitLong(*buf, bounds.bodyEndBit) // value data type
        || !skipTableBitLong(*buf, bounds.bodyEndBit)) { // value unit type
        return false;
    }
    readTableText(version, textBuf, tableTextEndBit(buf, textBuf, bounds));
    if (!textBuf->isGood())
        return false;
    if (!skipTableBitDouble(*buf, bounds.bodyEndBit) // rotation
        || !skipTableBitDouble(*buf, bounds.bodyEndBit) // block scale
        || !skipTableBitLong(*buf, bounds.bodyEndBit)) // alignment
        return false;
    std::int32_t rgb = -1;
    UTF8STRING name;
    UTF8STRING book;
    std::uint32_t color = 0;
    if (!readBoundedCmColor(*buf, strBuf, bounds.bodyEndBit, version,
                            color, &rgb, nullptr, &name, &book,
                            bounds.stringEndBit)
        || (hdlBuf != nullptr
            && readTableHandle(hdlBuf, bounds.handleEndBit) == 0
            && !hdlBuf->isGood())
        || !skipTableBitDouble(*buf, bounds.bodyEndBit)) // text height
        return false;
    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr) {
        ranges->push_back(makeDwgSubrecordRange(
            "table-content-format", startBit, currentDwgBit(buf),
            version, 1, good));
    }
    return good;
}

bool skipTableCellStyle(DRW::Version version, dwgBuffer *buf,
                        dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                        std::vector<DRW_DwgSubrecordRange> *ranges,
                        const TableDwgBounds& bounds) {
    const std::uint64_t startBit = currentDwgBit(buf);
    if (!skipTableBitLong(*buf, bounds.bodyEndBit)) // style type
        return false;
    std::uint16_t dataFlags = 0;
    if (!readBoundedBitShort(*buf, bounds.bodyEndBit, dataFlags))
        return false;
    const bool hasData = dataFlags != 0;
    if (!hasData) {
        if (ranges != nullptr) {
            ranges->push_back(makeDwgSubrecordRange(
                "table-cell-style", startBit, currentDwgBit(buf),
                version, 0, buf->isGood()));
        }
        return buf->isGood();
    }

    if (!skipTableBitLong(*buf, bounds.bodyEndBit) // property override flags
        || !skipTableBitLong(*buf, bounds.bodyEndBit)) // merge flags
        return false;
    std::int32_t rgb = -1;
    UTF8STRING name;
    UTF8STRING book;
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    std::uint32_t color = 0;
    if (!readBoundedCmColor(*buf, strBuf, bounds.bodyEndBit, version,
                            color, &rgb, nullptr, &name, &book,
                            bounds.stringEndBit)
        || !skipTableBitLong(*buf, bounds.bodyEndBit)) // content layout
        return false;
    if (!skipTableContentFormat(version, buf, strBuf, hdlBuf, ranges,
                                bounds))
        return false;

    std::uint16_t marginFlags = 0;
    if (!readBoundedBitShort(*buf, bounds.bodyEndBit, marginFlags))
        return false;
    if (marginFlags != 0) {
        for (int i = 0; i < 6; ++i)
            if (!skipTableBitDouble(*buf, bounds.bodyEndBit))
                return false;
    }

    std::uint32_t borders = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit, 6, 2,
                            borders)) {
        DRW_DBG("TABLE cell style border count out of range: "); DRW_DBG(borders); DRW_DBG("\n");
        return false;
    }
    for (std::uint32_t i = 0; i < borders; ++i) {
        std::int32_t edgeFlags = 0;
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, edgeFlags))
            return false;
        if (edgeFlags == 0)
            continue;
        if (!skipTableBitLong(*buf, bounds.bodyEndBit) // border overrides
            || !skipTableBitLong(*buf, bounds.bodyEndBit)) // border type
            return false;
        if (!readBoundedCmColor(*buf, strBuf, bounds.bodyEndBit, version,
                                color, &rgb, nullptr, &name, &book,
                                bounds.stringEndBit)
            || (hdlBuf != nullptr
                && readTableHandle(hdlBuf, bounds.handleEndBit) == 0
                && !hdlBuf->isGood())
            || !skipTableBitLong(*buf, bounds.bodyEndBit) // visible/invisible
            || !skipTableBitDouble(*buf, bounds.bodyEndBit)) // double line spacing
            return false;
    }

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr) {
        ranges->push_back(makeDwgSubrecordRange(
            "table-cell-style", startBit, currentDwgBit(buf),
            version, borders, good));
    }
    return good;
}

bool parseTableCell(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf,
                    dwgBuffer *hdlBuf, DRW_TableCell& cell,
                    std::vector<DRW_DwgSubrecordRange> *ranges,
                    const TableDwgBounds& bounds) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, cell.m_flags))
        return false;
    cell.m_toolTip = readTableText(
        version, textBuf, tableTextEndBit(buf, textBuf, bounds));
    if (!textBuf->isGood()) {
        DRW_DBG("TABLE cell tooltip string read failed\n");
        return false;
    }
    if (!skipTableBitLong(*buf, bounds.bodyEndBit)) // custom data
        return false;

    std::uint32_t customItems = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit, kMaxTableItems, 2,
                            customItems)) {
        DRW_DBG("TABLE cell custom item count out of range: "); DRW_DBG(customItems); DRW_DBG("\n");
        return false;
    }
    for (std::uint32_t i = 0; i < customItems; ++i) {
        if (!skipTableCustomData(version, buf, strBuf, hdlBuf, bounds)) {
            DRW_DBG("TABLE cell custom data parse incomplete\n");
            return false;
        }
    }

    std::int32_t hasLinkedData = 0;
    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, hasLinkedData))
        return false;
    if (hasLinkedData != 0) {
        if (hdlBuf != nullptr
            && readTableHandle(hdlBuf, bounds.handleEndBit) == 0
            && !hdlBuf->isGood())
            return false;
        if (!skipTableBitLong(*buf, bounds.bodyEndBit)
            || !skipTableBitLong(*buf, bounds.bodyEndBit)
            || !skipTableBitLong(*buf, bounds.bodyEndBit))
            return false;
    }

    std::uint32_t contentCount = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit, kMaxTableItems, 2,
                            contentCount)) {
        DRW_DBG("TABLE cell content count out of range: "); DRW_DBG(contentCount); DRW_DBG("\n");
        return false;
    }
    if (!DRW::reserve(cell.m_contents, static_cast<int>(contentCount)))
        return false;
    for (std::uint32_t i = 0; i < contentCount; ++i) {
        DRW_TableCellContent content;
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, content.m_type))
            return false;
        if (content.m_type == 1) {
            if (!readTableCadValue(version, buf, strBuf, hdlBuf,
                                   content.m_value, bounds)) {
                DRW_DBG("TABLE cell value parse incomplete\n");
                return false;
            }
            if (content.m_value.m_value.type() == DRW_Variant::STRING)
                content.m_text = content.m_value.m_value.c_str();
            else if (!content.m_value.m_valueString.empty())
                content.m_text = content.m_value.m_valueString;
        } else if (content.m_type == 2 || content.m_type == 4) {
            content.m_handle = readTableHandle(hdlBuf, bounds.handleEndBit);
        }

        std::uint32_t numAttrs = 0;
        if (!readTableBodyCount(version, buf, bounds.bodyEndBit, kMaxTableItems, 2,
                                numAttrs)) {
            DRW_DBG("TABLE cell attribute count out of range: "); DRW_DBG(numAttrs); DRW_DBG("\n");
            return false;
        }
        for (std::uint32_t attr = 0; attr < numAttrs; ++attr) {
            if (hdlBuf != nullptr
                && readTableHandle(hdlBuf, bounds.handleEndBit) == 0
                && !hdlBuf->isGood())
                return false;
            readTableText(version, textBuf,
                          tableTextEndBit(buf, textBuf, bounds));
            if (!textBuf->isGood()
                || !skipTableBitLong(*buf, bounds.bodyEndBit))
                return false;
        }

        std::uint16_t contentFormatFlag = 0;
        if (!readBoundedBitShort(*buf, bounds.bodyEndBit, contentFormatFlag))
            return false;
        const bool hasContentFormat = contentFormatFlag != 0;
        if (hasContentFormat
            && !skipTableContentFormat(version, buf, strBuf, hdlBuf, ranges,
                                       bounds)) {
            DRW_DBG("TABLE cell content format parse incomplete\n");
            return false;
        }
        cell.m_contents.push_back(content);
    }

    if (!skipTableCellStyle(version, buf, strBuf, hdlBuf, ranges, bounds)) {
        DRW_DBG("TABLE cell style override parse incomplete\n");
        return false;
    }

    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, cell.m_styleId))
        return false;
    const std::uint64_t geometryStartBit = currentDwgBit(buf);
    std::int32_t hasGeometry = 0;
    if (!readBoundedBitLong(*buf, bounds.bodyEndBit, hasGeometry))
        return false;
    if (hasGeometry != 0) {
        std::int32_t geometryFlags = 0;
        if (!skipTableBitLong(*buf, bounds.bodyEndBit) // unknown AC1027+ geometry marker
            || !readBoundedBitDouble(*buf, bounds.bodyEndBit, cell.m_width)
            || !readBoundedBitDouble(*buf, bounds.bodyEndBit, cell.m_height)
            || !readBoundedBitLong(*buf, bounds.bodyEndBit, geometryFlags))
            return false;
        cell.m_geometryFlags = geometryFlags;
        cell.m_geometryHandle = readTableHandle(hdlBuf, bounds.handleEndBit);
        if (cell.m_geometryFlags != 0) {
            std::int32_t geometryRecordFlags = 0;
            if (!readBounded3BitDouble(*buf, bounds.bodyEndBit,
                                       cell.m_geometryTopLeft)
                || !readBounded3BitDouble(*buf, bounds.bodyEndBit,
                                          cell.m_geometryCenter)
                || !readBoundedBitDouble(*buf, bounds.bodyEndBit,
                                         cell.m_contentWidth)
                || !readBoundedBitDouble(*buf, bounds.bodyEndBit,
                                         cell.m_contentHeight)
                || !readBoundedBitDouble(*buf, bounds.bodyEndBit,
                                         cell.m_geometryWidth)
                || !readBoundedBitDouble(*buf, bounds.bodyEndBit,
                                         cell.m_geometryHeight)
                || !readBoundedBitLong(*buf, bounds.bodyEndBit,
                                       geometryRecordFlags))
                return false;
            cell.m_geometryRecordFlags = geometryRecordFlags;
        }
        if (ranges != nullptr) {
            const bool geometryGood = buf->isGood() && (!hdlBuf || hdlBuf->isGood());
            ranges->push_back(makeDwgSubrecordRange(
                "table-cell-geometry-tail", geometryStartBit, currentDwgBit(buf),
                version, cell.m_geometryFlags, geometryGood));
        }
    }

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (!good)
        DRW_DBG("TABLE cell stream ended unexpectedly\n");
    return good;
}

bool parseTableContent(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf,
                       dwgBuffer *hdlBuf, DRW_TableContent& content,
                       const TableDwgBounds& bounds) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    const std::uint64_t textEndBit = tableTextEndBit(buf, textBuf, bounds);
    content.m_name = readTableText(version, textBuf, textEndBit);
    content.m_description = readTableText(version, textBuf, textEndBit);
    if (!textBuf->isGood())
        return false;

    std::uint32_t columns = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                            kMaxTableColumns, 2,
                            columns)) {
        DRW_DBG("TABLECONTENT column count out of range: "); DRW_DBG(columns); DRW_DBG("\n");
        return false;
    }
    content.m_columns.clear();
    if (!DRW::reserve(content.m_columns, static_cast<int>(columns)))
        return false;
    for (std::uint32_t col = 0; col < columns; ++col) {
        DRW_TableColumn column;
        column.m_name = readTableText(version, textBuf, textEndBit);
        if (!textBuf->isGood()
            || !skipTableBitLong(*buf, bounds.bodyEndBit)) // custom data
            return false;
        std::uint32_t customItems = 0;
        if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                                kMaxTableItems, 2,
                                customItems)) {
            DRW_DBG("TABLECONTENT column custom item count out of range: "); DRW_DBG(customItems); DRW_DBG("\n");
            return false;
        }
        for (std::uint32_t i = 0; i < customItems; ++i) {
            if (!skipTableCustomData(version, buf, strBuf, hdlBuf, bounds)) {
                DRW_DBG("TABLECONTENT column custom data parse incomplete\n");
                return false;
            }
        }
        if (!skipTableCellStyle(version, buf, strBuf, hdlBuf,
                                &content.m_subrecordRanges, bounds)) {
            DRW_DBG("TABLECONTENT column cell style parse incomplete\n");
            return false;
        }
        if (!skipTableBitLong(*buf, bounds.bodyEndBit) // style id
            || !readBoundedBitDouble(*buf, bounds.bodyEndBit,
                                     column.m_width))
            return false;
        content.m_columns.push_back(column);
    }

    std::uint32_t rows = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                            kMaxTableRows, 2,
                            rows)
        || (columns != 0 && rows > kMaxTableCells / columns)) {
        DRW_DBG("TABLECONTENT row count out of range: "); DRW_DBG(rows); DRW_DBG("\n");
        return false;
    }
    content.m_rows.clear();
    if (!DRW::reserve(content.m_rows, static_cast<int>(rows)))
        return false;
    for (std::uint32_t rowIndex = 0; rowIndex < rows; ++rowIndex) {
        DRW_TableRow row;
        std::uint32_t cells = 0;
        if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                                kMaxTableColumns, 2,
                                cells)
            || cells > kMaxTableItems) {
            DRW_DBG("TABLECONTENT row cell count out of range: "); DRW_DBG(cells); DRW_DBG("\n");
            return false;
        }
        if (!DRW::reserve(row.m_cells, static_cast<int>(cells)))
            return false;
        for (std::uint32_t cellIndex = 0; cellIndex < cells; ++cellIndex) {
            DRW_TableCell cell;
            if (!parseTableCell(version, buf, strBuf, hdlBuf, cell,
                                &content.m_subrecordRanges, bounds)) {
                DRW_DBG("TABLECONTENT cell parse incomplete at row "); DRW_DBG(rowIndex);
                DRW_DBG(" cell "); DRW_DBG(cellIndex); DRW_DBG("\n");
                return false;
            }
            row.m_cells.push_back(cell);
        }

        if (!skipTableBitLong(*buf, bounds.bodyEndBit)) // custom data
            return false;
        std::uint32_t customItems = 0;
        if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                                kMaxTableItems, 2,
                                customItems)) {
            DRW_DBG("TABLECONTENT row custom item count out of range: "); DRW_DBG(customItems); DRW_DBG("\n");
            return false;
        }
        for (std::uint32_t i = 0; i < customItems; ++i) {
            if (!skipTableCustomData(version, buf, strBuf, hdlBuf, bounds)) {
                DRW_DBG("TABLECONTENT row custom data parse incomplete\n");
                return false;
            }
        }
        if (!skipTableCellStyle(version, buf, strBuf, hdlBuf,
                                &content.m_subrecordRanges, bounds)) {
            DRW_DBG("TABLECONTENT row cell style parse incomplete\n");
            return false;
        }
        if (!skipTableBitLong(*buf, bounds.bodyEndBit) // style id
            || !readBoundedBitDouble(*buf, bounds.bodyEndBit, row.m_height))
            return false;
        content.m_rows.push_back(row);
    }

    std::uint32_t fieldRefs = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                            kMaxTableItems, 0,
                            fieldRefs)) {
        DRW_DBG("TABLECONTENT field reference count out of range: "); DRW_DBG(fieldRefs); DRW_DBG("\n");
        return false;
    }
    content.m_fieldHandles.clear();
    if (!DRW::reserve(content.m_fieldHandles, static_cast<int>(fieldRefs)))
        return false;
    for (std::uint32_t i = 0; i < fieldRefs; ++i) {
        const std::uint32_t ref = readTableHandle(hdlBuf, bounds.handleEndBit);
        if (hdlBuf != nullptr && !hdlBuf->isGood())
            return false;
        if (ref != 0)
            content.m_fieldHandles.push_back(ref);
    }

    if (!skipTableCellStyle(version, buf, strBuf, hdlBuf,
                            &content.m_subrecordRanges, bounds)) {
        DRW_DBG("TABLECONTENT table cell style parse incomplete\n");
        return false;
    }

    std::uint32_t mergedRanges = 0;
    if (!readTableBodyCount(version, buf, bounds.bodyEndBit,
                            kMaxTableItems, 8,
                            mergedRanges)) {
        DRW_DBG("TABLECONTENT merged range count out of range: "); DRW_DBG(mergedRanges); DRW_DBG("\n");
        return false;
    }
    content.m_mergedRanges.clear();
    if (!DRW::reserve(content.m_mergedRanges,
                      static_cast<int>(mergedRanges)))
        return false;
    for (std::uint32_t i = 0; i < mergedRanges; ++i) {
        DRW_TableMergedRange range;
        if (!readBoundedBitLong(*buf, bounds.bodyEndBit, range.m_topRow)
            || !readBoundedBitLong(*buf, bounds.bodyEndBit,
                                   range.m_leftColumn)
            || !readBoundedBitLong(*buf, bounds.bodyEndBit,
                                   range.m_bottomRow)
            || !readBoundedBitLong(*buf, bounds.bodyEndBit,
                                   range.m_rightColumn))
            return false;
        content.m_mergedRanges.push_back(range);
    }

    content.m_tableStyleHandle = readTableHandle(hdlBuf, bounds.handleEndBit);
    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (!good)
        DRW_DBG("TABLECONTENT stream ended unexpectedly\n");
    return good;
}

} // namespace

//! Calculate arbitrary axis
/*!
*   Calculate arbitrary axis for apply extrusions
*  @author Rallaz
*/
void DRW_Entity::calculateAxis(DRW_Coord extPoint){
    //Follow the arbitrary DXF definitions for extrusion axes.
    if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625) {
        //If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
        //The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
        //Factoring in the fixed values for Wy gives N.z,0,-N.x
        extAxisX.x = extPoint.z;
        extAxisX.y = 0;
        extAxisX.z = -extPoint.x;
    } else {
        //Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
        //The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
        //Factoring in the fixed values for Wz gives -N.y,N.x,0.
        extAxisX.x = -extPoint.y;
        extAxisX.y = extPoint.x;
        extAxisX.z = 0;
    }

    extAxisX.unitize();

    //Ay = N x Ax
    extAxisY.x = (extPoint.y * extAxisX.z) - (extAxisX.y * extPoint.z);
    extAxisY.y = (extPoint.z * extAxisX.x) - (extAxisX.z * extPoint.x);
    extAxisY.z = (extPoint.x * extAxisX.y) - (extAxisX.x * extPoint.y);

    extAxisY.unitize();
}

//! Extrude a point using arbitrary axis
/*!
*   apply extrusion in a point using arbitrary axis (previous calculated)
*  @author Rallaz
*/
void DRW_Entity::extrudePoint(DRW_Coord extPoint, DRW_Coord *point){
    double px, py, pz;
    px = (extAxisX.x*point->x)+(extAxisY.x*point->y)+(extPoint.x*point->z);
    py = (extAxisX.y*point->x)+(extAxisY.y*point->y)+(extPoint.y*point->z);
    pz = (extAxisX.z*point->x)+(extAxisY.z*point->y)+(extPoint.z*point->z);

    point->x = px;
    point->y = py;
    point->z = pz;
}

bool DRW_Entity::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    try {
    switch (code) {
    case DRW::dxfCode::HANDLE:
        if (!reader->isValidHandleString() || !reader->registerSelfHandle())
            return false;
        handle = reader->getHandleString();
        break;
    case DRW::dxfCode::OWNER_HANDLE:
        parentHandle = reader->getHandleString();
        break;
    case DRW::dxfCode::LAYER:
        layer = reader->getUtf8String();
        break;
    case 6:
        lineType = reader->getUtf8String();
        break;
    case DRW::dxfCode::COLOR:
        color = reader->getInt32();
        break;
    case DRW::dxfCode::LINEWEIGHT:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case 48:
        ltypeScale = reader->getDouble();
        break;
    case DRW::dxfCode::INVISIBLE:
        visible = (reader->getInt32() & 1) == 0;
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    case 430:
        colorName = reader->getString();
        break;
    case 67:
        {
            const int value = reader->getInt32();
            if (value < DRW::ModelSpace || value > DRW::PaperSpace)
                return false;
            space = static_cast<DRW::Space>(value);
        }
        break;
    case 102:
        return parseDxfGroups(code, reader);
    case 284:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, DRW::CastAndReceieveShadows,
                                   DRW::IgnoreShadows, value))
                return false;
            shadow = static_cast<DRW::ShadowMode>(value);
        }
        break;
    case 347:
        material = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 348:
        fullVisualStyleHandle = reader->getHandleString();
        break;
    case 360:
        xDictHandle = reader->getHandleString();
        break;
    case DRW::dxfCode::PLOTSTYLE:
        plotStyle = reader->getHandleString();
        break;
    case 440:
        transparency = reader->getInt32();
        break;
    case 92: {
        // Proxy entity graphics byte count (ODA §20.4.95): 92 for R13–R2007.
        // Entities that repurpose 92 (for example MESH) handle it before this
        // base parser is reached.
        const std::int32_t value = reader->getInt32();
        if (value < 0
            || static_cast<std::uint32_t>(value)
                   > DRW::kMaxDxfBinaryPayloadBytes)
            return false;
        numProxyGraph = value;
        proxyGraphics.clear();
        break;
    }
    case 160: {
        // R2010+ uses a 64-bit DXF count (group 160), not group 92's 32-bit
        // storage. Keep the public legacy field bounded and representable.
        const std::int64_t value = reader->getInt64();
        if (value < 0
            || static_cast<std::uint64_t>(value)
                   > DRW::kMaxDxfBinaryPayloadBytes)
            return false;
        numProxyGraph = static_cast<int>(value);
        proxyGraphics.clear();
        break;
    }
    case 310:
        if (numProxyGraph != 0) {
            // Proxy graphics binary, hex-encoded across many ≤254-char chunks.
            const std::string& hex = reader->getString();
            if (!appendHexBytesChecked(
                    proxyGraphics, hex,
                    std::min<std::size_t>(
                        DRW::kMaxDxfBinaryPayloadBytes,
                        static_cast<std::size_t>(numProxyGraph))))
                return false;
        }
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
        if (extData.size() >= DRW_TableEntry::kMaxExtendedDataItems)
            return false;
        extData.push_back(std::make_shared<DRW_Variant>(code,
                                                         reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
        if (extData.size() >= DRW_TableEntry::kMaxExtendedDataItems)
            return false;
        curr = std::make_shared<DRW_Variant>(
            code, DRW_Coord(reader->getDouble(), 0.0, 0.0));
        extData.push_back(curr);
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        if (!curr)
            return false;
        curr->setCoordY(reader->getDouble());
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        if (!curr)
            return false;
        curr->setCoordZ(reader->getDouble());
        curr.reset();
        break;
    case 1040:
    case 1041:
    case 1042:
        if (extData.size() >= DRW_TableEntry::kMaxExtendedDataItems)
            return false;
        extData.push_back(std::make_shared<DRW_Variant>(code,
                                                         reader->getDouble()));
        break;
    case 1070:
    case 1071:
        if (extData.size() >= DRW_TableEntry::kMaxExtendedDataItems)
            return false;
        extData.push_back(std::make_shared<DRW_Variant>(code,
                                                         reader->getInt32()));
        break;
    default:
        break;
    }
    return true;
    } catch (...) {
        reset();
        return false;
    }
}

//parses dxf 102 groups to read entity
bool DRW_Entity::parseDxfGroups(int code, const std::unique_ptr<dxfReader>& reader){
    if (appData.size() >= DRW::kMaxDxfApplicationGroups)
        return false;
    std::list<DRW_Variant> ls;
    DRW_Variant curr;
    std::string appName= reader->getString();
    if (appName.size() <= 1 || appName.front() != '{')
        return false;

    const bool isReactors = appName == "{ACAD_REACTORS";
    const bool isXDictionary = appName == "{ACAD_XDICTIONARY";
    std::vector<std::uint32_t> parsedReactors;
    std::uint32_t parsedXDictionary = 0;

    curr.addString(code, appName.substr(1));
    ls.push_back(curr);
    int depth = 1;
    int nextCode = 0;
    while (depth > 0 && reader->readRec(&nextCode)) {
        if (ls.size() >= DRW::kMaxDxfApplicationGroupPairs)
            return false;

        // A section/entity boundary before the matching 102 close would be
        // consumed irreversibly and leave the caller out of sync.
        if (nextCode == 0)
            return false;

        DRW_Variant value;
        if (nextCode == 102) {
            const std::string marker = reader->getString();
            if (marker.empty() || (marker.front() == '{' && marker.size() == 1))
                return false;
            if (marker.front() == '{') {
                if (++depth > DRW::kMaxDxfApplicationGroupNesting)
                    return false;
            } else if (marker == "}") {
                --depth;
            } else {
                return false;
            }
            value.addString(nextCode, marker);
        } else if ((nextCode >= 320 && nextCode <= 369)
                   || (nextCode >= 390 && nextCode <= 399)
                   || nextCode == 480 || nextCode == 481
                   || nextCode == 1005) {
            const std::string rawHandle = reader->getString();
            value.addString(nextCode, rawHandle);
            if (depth == 1 && isReactors && nextCode == 330) {
                const auto reactor = reader->getHandleString();
                if (reactor != 0) {
                    if (parsedReactors.size() >= dwgSafety::MaxReactorCount)
                        return false;
                    parsedReactors.push_back(static_cast<std::uint32_t>(reactor));
                }
            } else if (depth == 1 && isXDictionary && nextCode == 360
                       && parsedXDictionary == 0) {
                parsedXDictionary = static_cast<std::uint32_t>(
                    reader->getHandleString());
            }
        } else {
            switch (reader->type) {
            case dxfReader::STRING:
            case dxfReader::BINARY:
                value.addString(nextCode, reader->getString());
                break;
            case dxfReader::INT32:
            case dxfReader::BOOL:
                value.addInt(nextCode, reader->getInt32());
                break;
            case dxfReader::INT64:
                value.addInt64(nextCode, static_cast<std::int64_t>(reader->getInt64()));
                break;
            case dxfReader::DOUBLE:
                value.addDouble(nextCode, reader->getDouble());
                break;
            default:
                return false;
            }
        }
        ls.push_back(value);
    }

    if (depth != 0)
        return false;
    if (isReactors)
        reactorHandles = std::move(parsedReactors);
    if (isXDictionary && parsedXDictionary != 0)
        xDictHandle = parsedXDictionary;
    appData.push_back(std::move(ls));
    return true;
}

bool DRW_Entity::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer* strBuf, std::uint32_t bs){
    // This overload is also used by the two-stage common/handle tests and
    // callers; concrete record parsers own carrier reset so handle parsing can
    // remain transactional.
    if (buf == nullptr)
        return false;

    DRW_DBG("\n***************************** parsing entity *********************************************\n");
    oType = buf->getObjType(version);
    if (!buf->isGood())
        return false;
    DRW_DBG("Object type: "); DRW_DBG(oType); DRW_DBG(", "); DRW_DBGH(oType);

    if (version > DRW::AC1014 && version < DRW::AC1024) {//2000 & 2004
        const std::uint64_t totalBits = static_cast<std::uint64_t>(buf->size()) * 8u;
        std::uint32_t parsedObjectSize = 0;
        if (!readBoundedRawLong32(*buf, totalBits, parsedObjectSize))
            return false;
        objSize = parsedObjectSize;  //RL 32bits object size in bits
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }
    if (version > DRW::AC1021) {//2010+
        const std::uint64_t totalBits = static_cast<std::uint64_t>(buf->size()) * 8u;
        // Clamp: a corrupt bs > ms*8 would underflow objSize (unsigned) to a
        // huge value and drive strBuf->moveBitPos(objSize-1) past the buffer.
        if (totalBits > std::numeric_limits<std::uint32_t>::max()
            || bs > totalBits)
            return false;
        objSize = static_cast<std::uint32_t>(totalBits - bs);
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }

    if (strBuf != NULL && version > DRW::AC1018) {//2007+
        if (!strBuf->seekR2007StringStream(objSize))
            return false;
        DRW_DBG(" strBuf strbit pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
        DRW_DBG("strBuf start pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
    }

    // R2007+ objSize includes the detached string stream and its footer.  A
    // common entity prefix must stay in the data portion just like the
    // entity-specific readers below.  Only a caller that supplies the real
    // modern string cursor can require the footer here; the null-cursor form
    // is retained for isolated common-prefix tests without object framing.
    std::uint64_t bodyEndBit = proxyEntityEndBit(*buf, objSize);
    if (version > DRW::AC1021 && strBuf != nullptr) {
        dwgBuffer stringBoundary = strBuf->forkIndependent();
        if (!stringBoundary.seekR2007StringStream(objSize))
            return false;
        bodyEndBit = currentDwgBit(&stringBoundary);
    }
    dwgDataEndBit = bodyEndBit;
    const auto bodyWithinBounds = [buf, &bodyEndBit]() {
        return currentDwgBit(buf) <= bodyEndBit;
    };

    dwgHandle ho;
    if (!readBoundedDwgHandle(*buf, bodyEndBit, 0, false, ho))
        return false;
    handle = ho.ref;
    DRW_DBG("Entity Handle: "); DRW_DBGHL(ho.code, ho.size, ho.ref);
    // EED is a bounded, repeated chunk stream. Parse all chunks before
    // changing the entity so a malformed item cannot leak partial metadata.
    std::vector<DwgEedChunk> eedChunks;
    if (!readDwgEed(version, *buf, eedChunks, bodyEndBit))
        return false;
    if (!bodyWithinBounds())
        return false;
    std::vector<std::shared_ptr<DRW_Variant>> parsedExtData;
    std::vector<PendingHandleRef> parsedAppIdResolutions;
    std::vector<PendingHandleRef> parsedLayerRefResolutions;
    try {
        for (const DwgEedChunk& chunk : eedChunks) {
            const std::size_t appIndex =
                extData.size() + parsedExtData.size();
            parsedExtData.push_back(
                std::make_shared<DRW_Variant>(1001, std::string{}));
            parsedAppIdResolutions.push_back({appIndex, chunk.appHandle});
            for (const DRW_Variant& item : chunk.items)
                parsedExtData.push_back(std::make_shared<DRW_Variant>(item));
            for (const DwgEedHandleRef& ref : chunk.layerRefs) {
                parsedLayerRefResolutions.push_back(
                    {appIndex + 1 + ref.itemIndex, ref.handleRef});
            }
        }
    } catch (...) {
        return false;
    }
    DRW_DBG(" [bidi-debug pre-graphFlag bufpos="); DRW_DBG(buf->getPosition()); DRW_DBG(" bitpos="); DRW_DBG(buf->getBitPos()); DRW_DBG("]\n");
    bool graphFlag = false;
    if (!readBoundedBit(*buf, bodyEndBit, graphFlag))
        return false;
    DRW_DBG(" graphFlag: "); DRW_DBG(graphFlag); DRW_DBG("\n");
    if (!bodyWithinBounds())
        return false;
    if (graphFlag) {
        DRW_DBG(" [bidi-debug pre-graphSize bufpos="); DRW_DBG(buf->getPosition()); DRW_DBG(" bitpos="); DRW_DBG(buf->getBitPos()); DRW_DBG("]\n");
        std::uint64_t graphDataSize = 0;
        if (version >= DRW::AC1024) {
            if (!readBoundedBitLongLong(*buf, bodyEndBit, graphDataSize))
                return false;
        } else {
            std::uint32_t parsedGraphDataSize = 0;
            if (!readBoundedRawLong32(*buf, bodyEndBit,
                                      parsedGraphDataSize))
                return false;
            graphDataSize = parsedGraphDataSize;
        }
        DRW_DBG("graphData in bytes: "); DRW_DBG(static_cast<std::uint32_t>(graphDataSize)); DRW_DBG("\n");
        if (!bodyWithinBounds())
            return false;
        const std::uint64_t maxMoveBytes = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max() / 8);
        const std::uint64_t currentBit = currentDwgBit(buf);
        std::uint64_t graphBits = 0;
        if (!dwgSafety::multiply(graphDataSize, 8, graphBits)
            || currentBit > bodyEndBit
            || graphBits > bodyEndBit - currentBit
            || graphDataSize > static_cast<std::uint64_t>(buf->numRemainingBytes())
            || graphDataSize > maxMoveBytes) {
            DRW_DBG("graphData size outside object body\n");
            return false;
        }
        // Capture the proxy-graphics byte stream instead of skipping it. These
        // are cached drawable primitives (lines/arcs/polylines/text) that any
        // reader can render for proxy/custom entities (STDPART2D, AEC_*, tables)
        // — previously discarded via moveBitPos, leaving proxyGraphics empty.
        // dwgBuffer::getBytes is bit-aware (reconstructs each byte at a non-zero
        // bitPos), so it lands at the exact same position moveBitPos(8N) did.
        // (write-review #32 / read-coverage gap #1)
        if (graphDataSize > 0) {
            if (!DRW::resize(proxyGraphics, static_cast<int>(graphDataSize)))
                return false;
            if (!readBoundedBytes(
                    *buf, bodyEndBit,
                    reinterpret_cast<std::uint8_t*>(&proxyGraphics[0]),
                    static_cast<std::size_t>(graphDataSize)))
                return false;
            numProxyGraph = static_cast<int>(graphDataSize);
        }
    }
    if (version < DRW::AC1015) {//14-
        std::uint32_t parsedObjectSize = 0;
        if (!readBoundedRawLong32(*buf, proxyEntityEndBit(*buf, 0),
                                  parsedObjectSize))
            return false;
        objSize = parsedObjectSize;  //RL 32bits object size in bits
        DRW_DBG(" Object size in bits: "); DRW_DBG(objSize); DRW_DBG("\n");
        bodyEndBit = proxyEntityEndBit(*buf, objSize);
    }

    std::uint8_t entmode = 0;
    if (!readBounded2Bits(*buf, bodyEndBit, entmode))
        return false;
    if (entmode == 0)
        ownerHandle= true;
    //        entmode = 2;
    else if(entmode ==2)
        entmode = 0;
    space = (DRW::Space)entmode; //RLZ verify cast values
    DRW_DBG("entmode: "); DRW_DBG(entmode);
    if (!readBoundedBitLong(*buf, bodyEndBit, numReactors))
        return false; //BL per spec §20.4.1
    DRW_DBG(", numReactors: "); DRW_DBG(numReactors);
    if (!buf->isGood() || !dwgSafety::validReactorCount(numReactors))
        return false;

    if (version < DRW::AC1015) {//14-
        bool isByLayer = false;
        if (!readBoundedBit(*buf, bodyEndBit, isByLayer))
            return false;
        if (isByLayer) {//is bylayer line type
            lineType = "BYLAYER";
            ltFlags = 0;
        } else {
            lineType = "";
            ltFlags = 3;
        }
        DRW_DBG(" lineType: "); DRW_DBG(lineType.c_str());
        DRW_DBG(" ltFlags: "); DRW_DBG(ltFlags);
    }
    if (version > DRW::AC1015) {//2004+
        bool xDict = false;
        if (!readBoundedBit(*buf, bodyEndBit, xDict))
            return false;
        xDictFlag = xDict;
        DRW_DBG(" xDictFlag: "); DRW_DBG(xDictFlag); DRW_DBG("\n");
    }

    // libreDWG common_entity_data.spec — the bit at this stream position has two
    // disjoint meanings by version:
    //   * R13..R2002 (version < AC1018): `nolinks` (B). 1 = no prev/next handles
    //     in the handle section; 0 = read prev+next at parseDwgEntHandle.
    //   * R2004..R2010 (AC1018..AC1024): NO bit in the stream — reader forces
    //     haveNextLinks=1 to skip the prev/next handle reads.
    //   * R2013+ (version > AC1024): `has_ds_data` (B). 1 = inline ACIS SAB
    //     datastore present. Stored separately because it gates SAB handling,
    //     not prev/next links (which are already version<AC1018 gated).
    // Total bit consumption is unchanged for every version.
    if (version < DRW::AC1018) {
        bool parsedHaveNextLinks = false;
        if (!readBoundedBit(*buf, bodyEndBit, parsedHaveNextLinks))
            return false; //nolinks //B
        haveNextLinks = parsedHaveNextLinks ? 1 : 0;
        DRW_DBG(", haveNextLinks (0 yes, 1 prev next): "); DRW_DBG(haveNextLinks); DRW_DBG("\n");
    } else {
        haveNextLinks = 1; //AC1018+: not in stream, force 1 (no prev/next)
        DRW_DBG(", haveNextLinks (forced): "); DRW_DBG(haveNextLinks); DRW_DBG("\n");
    }
    if (version > DRW::AC1024) {
        bool parsedHasDsData = false;
        if (!readBoundedBit(*buf, bodyEndBit, parsedHasDsData))
            return false; //has_ds_data //B (R2013+)
        hasDsData = parsedHasDsData ? 1 : 0;
        DRW_DBG(", hasDsData (R2013+): "); DRW_DBG(hasDsData); DRW_DBG("\n");
    }
//ENC color
    std::uint32_t parsedColor = 0;
    if (!readBoundedEnColor(*buf, bodyEndBit, version, parsedColor))
        return false; //BS or CMC //ok for R14 or negate
    color = static_cast<int>(parsedColor);
    // Capture the AcDbColor side-channel before the common handle stream.
    // The handle follows owner/reactors/xdictionary and precedes layer in the
    // R2004+ common entity handle data.
    hasAcDbColorH = buf->lastEnColorHadDbColorRef;
    // libreDWG common_entity_data.spec:432-453 — ENC alpha_raw (DXF code
    // 440) is encoded as (alpha_type<<24) | alpha. Stored verbatim; the
    // filter (RS_FilterDXFRW::setEntityAttributes) decodes alpha_type==3
    // into a per-entity pen alpha, otherwise inherits from layer/block.
    if (buf->lastEnColorAlphaRaw != 0) {
        transparency = static_cast<int>(buf->lastEnColorAlphaRaw);
    }
    // libreDWG common_entity_data.spec:468-475 — inline TV name/book name
    // (flags 0x41/0x42) override any dbColorMap-resolved name. Captured
    // immediately; entryParse will skip the override only if colorName is
    // already populated here.
    if (!buf->lastEnColorName.empty()) {
        colorName = buf->lastEnColorBookName.empty()
            ? buf->lastEnColorName
            : (buf->lastEnColorBookName + "$" + buf->lastEnColorName);
    }
    color24 = buf->lastEnColorRgb;
    if (!readBoundedBitDouble(*buf, bodyEndBit, ltypeScale))
        return false; //BD
    DRW_DBG(" entity color: "); DRW_DBG(color);
    DRW_DBG(" ltScale: "); DRW_DBG(ltypeScale); DRW_DBG("\n");
    if (version > DRW::AC1014) {//2000+ — §19.4.1: linetype-flags BB then plot-flags BB
        std::uint8_t parsedLtFlags = 0;
        if (!readBounded2Bits(*buf, bodyEndBit, parsedLtFlags))
            return false; //BB
        ltFlags = parsedLtFlags;
        if (ltFlags == 0)      lineType = "BYLAYER";
        else if (ltFlags == 1) lineType = "BYBLOCK";
        else if (ltFlags == 2) lineType = "CONTINUOUS";
        else                   lineType = ""; //3 → handle at end
        DRW_DBG("ltFlags: "); DRW_DBG(ltFlags);
        DRW_DBG(" lineType: "); DRW_DBG(lineType.c_str());

        std::uint8_t parsedPlotFlags = 0;
        if (!readBounded2Bits(*buf, bodyEndBit, parsedPlotFlags))
            return false; //BB
        plotFlags = parsedPlotFlags;
        DRW_DBG(", plotFlags: "); DRW_DBG(plotFlags);
    }
    if (version > DRW::AC1018) {//2007+
        std::uint8_t parsedMaterialFlag = 0;
        if (!readBounded2Bits(*buf, bodyEndBit, parsedMaterialFlag))
            return false; //BB
        materialFlag = parsedMaterialFlag;
        DRW_DBG("materialFlag: "); DRW_DBG(materialFlag);
        std::uint8_t parsedShadowFlag = 0;
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedShadowFlag))
            return false; //RC, low 2 bits is shadow mode 0..3
        shadowFlag = parsedShadowFlag;
        DRW_DBG("shadowFlag: "); DRW_DBG(shadowFlag); DRW_DBG("\n");
        shadow = static_cast<DRW::ShadowMode>(shadowFlag & 0x3);
    }
    if (version > DRW::AC1021) {//2010+ — §19.4.1: three single-bit flags
        // Ground-truth: libreDWG common_entity_data.spec lines 523-528
        // and ODA spec v5.4.1 §19.4.1 both define three FIELD_B (single bit)
        // flags here, one each for full/face/edge visual style. Total bit
        // consumption (3 bits) is identical to the historical BB+B shape;
        // only the semantics differ. The corresponding handles are read
        // conditionally in parseDwgEntHandle after the plotstyle handle.
        bool parsedHasFullVisualStyle = false;
        bool parsedHasFaceVisualStyle = false;
        bool parsedHasEdgeVisualStyle = false;
        if (!readBoundedBit(*buf, bodyEndBit, parsedHasFullVisualStyle)
            || !readBoundedBit(*buf, bodyEndBit, parsedHasFaceVisualStyle)
            || !readBoundedBit(*buf, bodyEndBit, parsedHasEdgeVisualStyle))
            return false; //B
        hasFullVisualStyle = parsedHasFullVisualStyle ? 1 : 0;
        hasFaceVisualStyle = parsedHasFaceVisualStyle ? 1 : 0;
        hasEdgeVisualStyle = parsedHasEdgeVisualStyle ? 1 : 0;
        DRW_DBG("hasFull/Face/Edge VisualStyle: ");
        DRW_DBG(hasFullVisualStyle); DRW_DBG(" ");
        DRW_DBG(hasFaceVisualStyle); DRW_DBG(" ");
        DRW_DBG(hasEdgeVisualStyle); DRW_DBG("\n");
    }
    std::uint16_t parsedInvisibleFlag = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedInvisibleFlag))
        return false; //BS
    const std::int16_t invisibleFlag =
        static_cast<std::int16_t>(parsedInvisibleFlag);
    DRW_DBG(" invisibleFlag: "); DRW_DBG(invisibleFlag);
    // DXF group 60: bit 0 = invisible (1) / visible (0). libreDWG
    // common_entity_data.spec masks bit 0 only (`invisible & 1`) and ignores
    // the higher bits, so use the same mask rather than `== 0`. Paired with
    // the encode emit below.
    visible = ((invisibleFlag & 1) == 0);
    if (version > DRW::AC1014) {//2000+
        std::uint8_t parsedLineWeight = 0;
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedLineWeight))
            return false; //RC
        lWeight = DRW_LW_Conv::dwgInt2lineWidth(parsedLineWeight);
        DRW_DBG(" lwFlag (lWeight): "); DRW_DBG(lWeight); DRW_DBG("\n");
    }
    //Only in blocks ????????
//    if (version > DRW::AC1018) {//2007+
//        std::uint8_t unk = buf->getBit();
//        DRW_DBG("unknown bit: "); DRW_DBG(unk); DRW_DBG("\n");
//    }
    if (!buf->isGood() || !bodyWithinBounds())
        return false;

    const std::size_t oldExtDataSize = extData.size();
    const std::size_t oldAppResolutionSize =
        pendingAppIdResolutions.size();
    const std::size_t oldLayerResolutionSize =
        pendingLayerRefResolutions.size();
    const auto rollbackEed = [this, oldExtDataSize,
                              oldAppResolutionSize, oldLayerResolutionSize]() {
        extData.resize(oldExtDataSize);
        pendingAppIdResolutions.resize(oldAppResolutionSize);
        pendingLayerRefResolutions.resize(oldLayerResolutionSize);
    };
    if (parsedExtData.size() > DRW_TableEntry::kMaxExtendedDataItems
        || extData.size() > DRW_TableEntry::kMaxExtendedDataItems
        || parsedExtData.size()
               > DRW_TableEntry::kMaxExtendedDataItems - extData.size())
        return false;
    try {
        extData.insert(extData.end(), parsedExtData.begin(),
                       parsedExtData.end());
        pendingAppIdResolutions.insert(
            pendingAppIdResolutions.end(), parsedAppIdResolutions.begin(),
            parsedAppIdResolutions.end());
        pendingLayerRefResolutions.insert(
            pendingLayerRefResolutions.end(),
            parsedLayerRefResolutions.begin(), parsedLayerRefResolutions.end());
    } catch (...) {
        rollbackEed();
        return false;
    }
    return true;
}

bool DRW_Entity::parseDwgEntHandle(
    DRW::Version version, dwgBuffer *buf, bool resetHandleStream,
    std::uint64_t handleEndBit){
    if (buf == nullptr || !buf->isGood()
        || !dwgSafety::validReactorCount(numReactors))
        return false;
    if (resetHandleStream && version > DRW::AC1018) {//2007+ skip string area
        if (!buf->setPosition(objSize >> 3))
            return false;
        buf->setBitPos(objSize & 7);
        if (!buf->isGood())
            return false;
    }
    if (handleEndBit != std::numeric_limits<std::uint64_t>::max()
        && currentDwgBit(buf) > handleEndBit)
        return false;

    std::uint32_t newAcDbColorHandle = acDbColorHandle;
    std::uint32_t newParentHandle = ownerHandle ? parentHandle : DRW::NoHandle;
    std::vector<std::uint32_t> newReactorHandles;
    const bool hasXDictionary = version <= DRW::AC1015 || xDictFlag != 1;
    std::uint32_t newXDictHandle = hasXDictionary ? xDictHandle : 0;
    dwgHandle newLayerH = layerH;
    dwgHandle newLTypeH = lTypeH;
    std::uint32_t newNextEntLink = nextEntLink;
    std::uint32_t newPrevEntLink = prevEntLink;
    const DRW::ShadowMode newShadow = version > DRW::AC1018
        ? shadow : DRW::CastAndReceieveShadows;
    std::uint32_t newMaterial = materialFlag == 3 ? material
                                                  : DRW::MaterialByLayer;
    int newPlotStyle = plotFlags == 3 ? plotStyle
                                      : DRW::DefaultPlotStyle;
    std::uint32_t newShadowHandle = shadowFlag == 3 ? shadowHandle : 0;
    std::uint32_t newFullVisualStyleHandle = hasFullVisualStyle
        ? fullVisualStyleHandle : 0;
    std::uint32_t newFaceVisualStyleHandle = hasFaceVisualStyle
        ? faceVisualStyleHandle : 0;
    std::uint32_t newEdgeVisualStyleHandle = hasEdgeVisualStyle
        ? edgeVisualStyleHandle : 0;
    if (version <= DRW::AC1018) {
        newMaterial = DRW::MaterialByLayer;
        newShadowHandle = 0;
    }
    if (version <= DRW::AC1021) {
        newFullVisualStyleHandle = 0;
        newFaceVisualStyleHandle = 0;
        newEdgeVisualStyleHandle = 0;
    }
    auto readOffsetHandle = [&](dwgHandle& out) {
        return handleEndBit == std::numeric_limits<std::uint64_t>::max()
            ? readDwgHandleChecked(*buf, handle, true, out)
            : readBoundedDwgHandle(*buf, handleEndBit, handle, true, out);
    };
    auto readRawHandle = [&](dwgHandle& out) {
        return handleEndBit == std::numeric_limits<std::uint64_t>::max()
            ? readDwgHandleChecked(*buf, handle, false, out)
            : readBoundedDwgHandle(*buf, handleEndBit, handle, false, out);
    };
    auto readHandleList = [&]() {
        if (handleEndBit == std::numeric_limits<std::uint64_t>::max())
            return readDwgHandleList(*buf, handle, numReactors, false,
                                     &newReactorHandles);
        if (!dwgSafety::validReactorCount(numReactors))
            return false;
        std::vector<std::uint32_t> parsed;
        if (!DRW::reserve(parsed, numReactors))
            return false;
        for (std::int32_t i = 0; i < numReactors; ++i) {
            dwgHandle reactor;
            if (!readBoundedDwgHandle(*buf, handleEndBit, handle, false,
                                      reactor))
                return false;
            parsed.push_back(reactor.ref);
        }
        newReactorHandles = std::move(parsed);
        return true;
    };

    if(ownerHandle){//entity are in block or in a polyline
        dwgHandle ownerH;
        if (!readOffsetHandle(ownerH))
            return false;
        DRW_DBG("owner (parent) Handle: "); DRW_DBGHL(ownerH.code, ownerH.size, ownerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        newParentHandle = ownerH.ref;
        DRW_DBG("Block (parent) Handle: "); DRW_DBGHL(ownerH.code, ownerH.size, newParentHandle); DRW_DBG("\n");
    } else
        DRW_DBG("NO Block (parent) Handle\n");

    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    if (!readHandleList())
        return false;
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH;
        if (!readRawHandle(XDicObjH))
            return false;
        newXDictHandle = XDicObjH.ref;  // 2a.2: persist xdict
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    // ENC's optional AcDbColor reference follows the common owner, reactor,
    // and xdictionary handles in R2004+ files. The dwgReader resolves this
    // handle against dbColorMap after parsing the complete entity.
    if (hasAcDbColorH && version > DRW::AC1015) {
        dwgHandle dbcH;
        if (!readOffsetHandle(dbcH))
            return false;
        newAcDbColorHandle = dbcH.ref;
        DRW_DBG(" AcDbColor Handle: ");
        DRW_DBGHL(dbcH.code, dbcH.size, dbcH.ref); DRW_DBG("\n");
    }

    if (version < DRW::AC1015) {//R14-
        //layer handle
        if (!readOffsetHandle(newLayerH))
            return false;
        DRW_DBG(" layer Handle: "); DRW_DBGHL(newLayerH.code, newLayerH.size, newLayerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        //lineType handle
        if(ltFlags == 3){
            if (!readOffsetHandle(newLTypeH))
                return false;
            DRW_DBG("linetype Handle: "); DRW_DBGHL(newLTypeH.code, newLTypeH.size, newLTypeH.ref); DRW_DBG("\n");
            DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
    }
    if (version < DRW::AC1018) {//2000+
        if (haveNextLinks == 0) {
            dwgHandle nextLinkH;
            if (!readOffsetHandle(nextLinkH))
                return false;
            DRW_DBG(" prev nextLinkers Handle: "); DRW_DBGHL(nextLinkH.code, nextLinkH.size, nextLinkH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            const std::uint32_t prevLink = nextLinkH.ref;
            if (!readOffsetHandle(nextLinkH))
                return false;
            DRW_DBG(" next nextLinkers Handle: "); DRW_DBGHL(nextLinkH.code, nextLinkH.size, nextLinkH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            newPrevEntLink = prevLink;
            newNextEntLink = nextLinkH.ref;
        } else {
            if (handle == 0 || handle == std::numeric_limits<std::uint32_t>::max())
                return false;
            newNextEntLink = handle + 1;
            newPrevEntLink = handle - 1;
        }
    }
    if (version > DRW::AC1015) {//2004+
        //Parses Bookcolor handle
    }
    if (version > DRW::AC1014) {//2000+
        //layer handle
        if (!readOffsetHandle(newLayerH))
            return false;
        DRW_DBG(" layer Handle: "); DRW_DBGHL(newLayerH.code, newLayerH.size, newLayerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        //lineType handle
        if(ltFlags == 3){
            if (!readOffsetHandle(newLTypeH))
                return false;
            DRW_DBG("linetype Handle: "); DRW_DBGHL(newLTypeH.code, newLTypeH.size, newLTypeH.ref); DRW_DBG("\n");
            DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
    }
    if (version > DRW::AC1014) {//2000+
        if (version > DRW::AC1018) {//2007+
            if (materialFlag == 3) {
                dwgHandle materialH;
                if (!readOffsetHandle(materialH))
                    return false;
                newMaterial = materialH.ref;
                DRW_DBG(" material Handle: "); DRW_DBGHL(materialH.code, materialH.size, materialH.ref); DRW_DBG("\n");
                DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            }
            if (shadowFlag == 3) {
                // AcDbShadow object handle is separate from the mode encoded
                // in shadowFlag. Keep it even though LibreCAD has no render
                // consumer for the shadow object itself.
                dwgHandle shadowH;
                if (!readOffsetHandle(shadowH))
                    return false;
                newShadowHandle = shadowH.ref;
                DRW_DBG(" shadow Handle: "); DRW_DBGHL(shadowH.code, shadowH.size, shadowH.ref); DRW_DBG("\n");
                DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            }
        }
        if (plotFlags == 3) {
            dwgHandle plotStyleH;
            if (!readOffsetHandle(plotStyleH))
                return false;
            newPlotStyle = static_cast<int>(plotStyleH.ref);
            DRW_DBG(" plot style Handle: "); DRW_DBGHL(plotStyleH.code, plotStyleH.size, plotStyleH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
        if (version > DRW::AC1021) {//2010+ — §19.4.2: visual-style handles
            // Ground-truth: libreDWG common_entity_handle_data.spec lines
            // 141-150 and ODA spec v5.4.1 §19.4.2. Order matches: full,
            // face, edge — each conditional on its single-bit flag from
            // §19.4.1 (set in parseDwg above). All three are hard pointers
            // (libreDWG FIELD_HANDLE code 5), matching the existing
            // material/shadow/plotstyle handles in this block.
            if (hasFullVisualStyle) {
                dwgHandle h;
                if (!readOffsetHandle(h))
                    return false;
                newFullVisualStyleHandle = h.ref;
                DRW_DBG(" full visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
            if (hasFaceVisualStyle) {
                dwgHandle h;
                if (!readOffsetHandle(h))
                    return false;
                newFaceVisualStyleHandle = h.ref;
                DRW_DBG(" face visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
            if (hasEdgeVisualStyle) {
                dwgHandle h;
                if (!readOffsetHandle(h))
                    return false;
                newEdgeVisualStyleHandle = h.ref;
                DRW_DBG(" edge visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
        }
    }
    const int rb = buf->numRemainingBytes();
    DRW_DBG("\n DRW_Entity::parseDwgEntHandle Remaining bytes: "); DRW_DBG(rb); DRW_DBG("\n");
    if (rb > 4) {  // 2-byte CRC + slack
        DRW_DBG("\n*** parseDwgEntHandle leftover ");
        DRW_DBG(rb);
        DRW_DBG(" bytes; entity handle ");
        DRW_DBGH(handle);
        DRW_DBG(" oType ");
        DRW_DBG(oType);
        DRW_DBG(" — possible bit-stream misalignment ***\n");
    }
    if (!buf->isGood()
        || (handleEndBit != std::numeric_limits<std::uint64_t>::max()
            && currentDwgBit(buf) > handleEndBit))
        return false;

    acDbColorHandle = newAcDbColorHandle;
    parentHandle = newParentHandle;
    reactorHandles = std::move(newReactorHandles);
    xDictHandle = newXDictHandle;
    layerH = newLayerH;
    lTypeH = newLTypeH;
    nextEntLink = newNextEntLink;
    prevEntLink = newPrevEntLink;
    material = newMaterial;
    plotStyle = newPlotStyle;
    shadow = newShadow;
    if (version <= DRW::AC1018) {
        materialFlag = 0;
        shadowFlag = 0;
    }
    shadowHandle = newShadowHandle;
    if (version <= DRW::AC1021) {
        hasFullVisualStyle = false;
        hasFaceVisualStyle = false;
        hasEdgeVisualStyle = false;
    }
    fullVisualStyleHandle = newFullVisualStyleHandle;
    faceVisualStyleHandle = newFaceVisualStyleHandle;
    edgeVisualStyleHandle = newEdgeVisualStyleHandle;
    // R2007+ separates this tail from the object data stream. Earlier typed
    // parsers may consume inline handles but cannot give raw metadata an
    // independently bounded common-link proof.
    commonLinkTailValidated = version > DRW::AC1018;
    return true;
}

bool DRW_Entity::parseDwgCommon(DRW::Version version, dwgBuffer *buf,
                                std::uint32_t bs) {
    if (buf == nullptr)
        return false;
    // R2007 (AC1021) omits the footer when an object has no string stream;
    // the concrete readers using this helper have no detached text fields.
    if (version <= DRW::AC1021)
        return parseDwg(version, buf, nullptr, bs);

    // Keep the publishing cursor transactional: a missing string footer is
    // legal for body-only R2007-era inputs and for the low-level entity test
    // API, but must not leave the cursor half-consumed before that fallback.
    dwgBuffer bodyProbe = buf->forkIndependent();
    dwgBuffer stringProbe = bodyProbe.forkIndependent();
    if (parseDwg(version, &bodyProbe, &stringProbe, bs)) {
        *buf = bodyProbe;
        return true;
    }
    return parseDwg(version, buf, nullptr, bs);
}

bool DRW_ProxyEntity::parseDwg(DRW::Version version, dwgBuffer *buf,
                               std::uint32_t bs) {
    if (buf == nullptr || version < DRW::AC1012)
        return false;

    DRW_Entity::reset();
    eType = DRW::PROXYENTITY;
    m_hasProxyClassId = false;
    m_proxyClassId = 0;
    m_hasProxyCarrierId = false;
    m_proxyCarrierId = 0;
    m_proxySubclass.clear();
    m_hasProxyDrawingFormat = false;
    m_proxyDrawingFormat = 0;
    m_proxyDwgVersion = 0;
    m_proxyMaintenanceVersion = 0;
    m_hasFromDxf = false;
    m_fromDxf = false;
    m_hasProxyGraphicsByteSize = false;
    m_proxyGraphicsByteSize = 0;
    m_hasEntityDataBitSize = false;
    m_entityData.clear();
    m_entityDataBitSize = 0;
    m_hasUnknownDataByteSize = false;
    m_unknownDataByteSize = 0;
    m_unknownData.clear();
    m_objectIdRefs.clear();

    dwgBuffer stringBuffer = *buf;
    dwgBuffer *stringBuf = version > DRW::AC1018 ? &stringBuffer : buf;
    if (!DRW_Entity::parseDwg(version, buf, stringBuf, bs))
        return false;

    std::uint64_t dataEndBit = 0;
    std::uint64_t stringEndBit = 0;
    if (!entityBodyDataEndBit(*stringBuf, version, objSize, dataEndBit,
                              &stringEndBit)
        || dataEndBit > proxyEntityEndBit(*buf, 0))
        return false;

    bool present = false;
    std::int32_t value = 0;
    if (!readProxyEntityOptionalBitLong(*buf, dataEndBit, present, value))
        return false;
    if (present) {
        m_hasProxyCarrierId = true;
        m_proxyCarrierId = value;
    }

    if (version >= DRW::AC1018
        && proxyEntityHasBits(*stringBuf, stringEndBit, 2)) {
        if (!readBoundedVariableText(*stringBuf, stringEndBit, version,
                                     m_proxySubclass))
            return false;
    }

    if (version >= DRW::AC1032) {
        bool hasDwgVersion = false;
        bool hasMaintenanceVersion = false;
        std::int32_t dwgVersion = 0;
        std::int32_t maintenanceVersion = 0;
        if (!readProxyEntityOptionalBitLong(*buf, dataEndBit,
                                            hasDwgVersion, dwgVersion)
            || !readProxyEntityOptionalBitLong(*buf, dataEndBit,
                                               hasMaintenanceVersion,
                                               maintenanceVersion)) {
            return false;
        }
        if (hasDwgVersion || hasMaintenanceVersion) {
            m_hasProxyDrawingFormat = true;
            m_proxyDwgVersion = static_cast<std::uint16_t>(dwgVersion);
            m_proxyMaintenanceVersion =
                static_cast<std::uint16_t>(maintenanceVersion);
            m_proxyDrawingFormat =
                (static_cast<std::uint32_t>(m_proxyMaintenanceVersion) << 16u)
                | m_proxyDwgVersion;
        }
    } else if (!readProxyEntityOptionalBitLong(*buf, dataEndBit,
                                               present, value)) {
        return false;
    } else if (present) {
        m_hasProxyDrawingFormat = true;
        m_proxyDrawingFormat = static_cast<std::uint32_t>(value);
        m_proxyDwgVersion = static_cast<std::uint16_t>(value);
        m_proxyMaintenanceVersion = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(value) >> 16u);
    }

    if (version >= DRW::AC1015 && proxyEntityHasBits(*buf, dataEndBit, 1)) {
        if (!readBoundedBit(*buf, dataEndBit, m_fromDxf))
            return false;
        m_hasFromDxf = true;
    }

    const std::uint64_t payloadStartBit = currentDwgBit(buf);
    if (payloadStartBit > dataEndBit
        || !copyDwgBitRange(*buf, payloadStartBit, dataEndBit, m_entityData)) {
        return false;
    }
    m_entityDataBitSize = static_cast<std::uint32_t>(
        dataEndBit - payloadStartBit);

    dwgBuffer handleBuffer = *buf;
    if (!handleBuffer.setPosition(dataEndBit >> 3))
        return false;
    handleBuffer.setBitPos(static_cast<std::uint8_t>(dataEndBit & 7u));
    if (!handleBuffer.isGood())
        return false;
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*buf, version, objSize, bs, handleEndBit)
        || !DRW_Entity::parseDwgEntHandle(version, &handleBuffer, true,
                                           handleEndBit))
        return false;
    while (proxyEntityHasBits(handleBuffer, handleEndBit, 8)) {
        dwgHandle rawHandle;
        dwgHandle resolvedHandle;
        if (!readBoundedDwgObjectId(handleBuffer, handleEndBit, handle,
                                    rawHandle, resolvedHandle))
            return false;
        if (rawHandle.code == 0 && rawHandle.ref64 == 0)
            break;

        DRW_ProxyObjectIdRef ref;
        ref.m_dxfCode = proxyEntityDxfCode(rawHandle.code);
        ref.m_handleCode = rawHandle.code;
        ref.m_handle = resolvedHandle.ref64;
        ref.m_rawHandle = rawHandle.ref64;
        m_objectIdRefs.push_back(ref);
    }

    return buf->isGood() && stringBuf->isGood() && handleBuffer.isGood();
}

bool DRW_Point::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 50:
        // DXF code 50 is in degrees; the field is radians (matching the DWG
        // path, drw_entities.cpp:1787). degrees -> radians is /ARAD (x pi/180);
        // the prior *ARAD only canceled with the writer's /ARAD on DXF->DXF and
        // corrupted the value for DXF->DWG. ARAD = 180/pi.
        xAxisAngle = reader->getDouble() / ARAD;  // DXF degrees -> radians
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

// ---------------------------------------------------------------------------
// Phase 4a (drafted 2026-05-15)
// ---------------------------------------------------------------------------
// `DRW_Entity::encodeDwgCommon` and `encodeDwgEntHandle` mirror the common
// entity-header fragments parsed above.  Version gates intentionally follow
// the DWG reader so newer headers retain their reference flags and handles.
//
// Discarded fields (Risk 4i):
//   - haveNextLinks B — we emit 1 (no prev/next chain).
//   - none of the common color side channels are discarded for R2004+;
//     ENC names remain the byte-oriented TV fields defined by the DWG
//     common-entity specification, while ordinary entity text uses strBuf.
//
// We still emit these fixed defaults:
//   - entmode = 2 (modelspace, no owner-handle in stream — caller can
//     override before calling encodeDwgCommon if entity needs an owner).
//   - numReactors = 0 (reactor handles are emitted by the handle phase when
//     present).
//   - ltFlags = 0 (BYLAYER); plotFlags follows plotStyle.
//   - invisibleFlag = 0 (visible)
//
// Caller must:
//   - Pre-populate `eType`, `handle`, `color`, `ltypeScale`, `lWeight`,
//     `layerH.ref` (handle of the layer this entity belongs to).
//   - The body emit between encodeDwgCommon and encodeDwgEntHandle is
//     per-entity (3BD basePoint for Point, etc.).

// Phase-2a kill switch for the full common-entity-header write contract
// (entity reactors/xdict/EED/visibility/entmode emission). Default ON. The
// emission is gated by DATA PRESENCE (empty reactorHandles/extData + visible
// == today's hardcoded zeros), so flipping this OFF restores the legacy
// byte-identical output as an emergency escape hatch. The per-field emission
// arms land in 2a.1..2a.5; this scaffolding commit changes no bytes.
#ifndef LIBDXFRW_FULL_COMMON_HEADER
#define LIBDXFRW_FULL_COMMON_HEADER 1
#endif

namespace {

const DRW_Entity::PendingHandleRef* findEedWriteRef(
    const std::vector<DRW_Entity::PendingHandleRef>& refs,
    std::size_t index) {
    const auto it = std::find_if(refs.cbegin(), refs.cend(),
                                 [index](const auto& ref) {
                                     return ref.indexInExtData == index;
                                 });
    if (it == refs.cend())
        return nullptr;
    return &*it;
}

bool parseEedHexHandle(const std::string& text, std::uint64_t& handle) {
    if (text.empty())
        return false;
    std::size_t index = 0;
    if (text.size() > 2 && text[0] == '0'
        && (text[1] == 'x' || text[1] == 'X')) {
        index = 2;
    }
    if (index == text.size())
        return false;

    std::uint64_t parsed = 0;
    for (; index < text.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(text[index]);
        std::uint8_t digit = 0;
        if (character >= '0' && character <= '9')
            digit = static_cast<std::uint8_t>(character - '0');
        else if (character >= 'a' && character <= 'f')
            digit = static_cast<std::uint8_t>(character - 'a' + 10);
        else if (character >= 'A' && character <= 'F')
            digit = static_cast<std::uint8_t>(character - 'A' + 10);
        else
            return false;
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 16U)
            return false;
        parsed = parsed * 16U + digit;
    }
    handle = parsed;
    return true;
}

void putEedRawHandleBE(dwgBufferW& buffer, std::uint64_t handle) {
    for (int index = 7; index >= 0; --index) {
        buffer.putRawChar8(static_cast<std::uint8_t>(
            (handle >> (index * 8)) & 0xFFU));
    }
}

void putEedRawHandleLE(dwgBufferW& buffer, std::uint64_t handle) {
    for (unsigned index = 0; index < 8; ++index) {
        buffer.putRawChar8(static_cast<std::uint8_t>(
            (handle >> (index * 8U)) & 0xFFU));
    }
}

bool encodeEedItem(DRW::Version version, const DRW_Variant& value,
                   dwgBufferW& textEncoder, dwgBufferW& output,
                   std::uint64_t resolvedLayerHandle,
                   std::uint16_t codePage) {
    const int groupCode = value.code();
    switch (groupCode) {
    case 1000: {
        if (value.type() != DRW_Variant::STRING || value.content.s == nullptr)
            return false;
        const std::string encoded = textEncoder.encodeTextAreaString(
            *value.content.s, version > DRW::AC1018);
        if (version > DRW::AC1018) {
            if ((encoded.size() & 1U) != 0
                || encoded.size() / 2U > std::numeric_limits<std::uint16_t>::max()) {
                return false;
            }
            output.putRawChar8(0);
            output.putRawShort16(static_cast<std::uint16_t>(encoded.size() / 2U));
        } else {
            if (encoded.size() > std::numeric_limits<std::uint8_t>::max())
                return false;
            output.putRawChar8(0);
            output.putRawChar8(static_cast<std::uint8_t>(encoded.size()));
            output.putBERawShort16(codePage);
        }
        output.putBytes(reinterpret_cast<const std::uint8_t*>(encoded.data()),
                        encoded.size());
        return output.isGood();
    }
    case 1002: {
        if (value.type() != DRW_Variant::STRING || value.content.s == nullptr
            || (*value.content.s != "{" && *value.content.s != "}")) {
            return false;
        }
        output.putRawChar8(2);
        output.putRawChar8(*value.content.s == "{" ? 0 : 1);
        return output.isGood();
    }
    case 1003:
        if (value.type() != DRW_Variant::STRING || resolvedLayerHandle == 0)
            return false;
        output.putRawChar8(3);
        putEedRawHandleLE(output, resolvedLayerHandle);
        return output.isGood();
    case 1004: {
        if (value.type() != DRW_Variant::BINARY || value.binary() == nullptr
            || value.binary()->size() > std::numeric_limits<std::uint8_t>::max()) {
            return false;
        }
        output.putRawChar8(4);
        output.putRawChar8(static_cast<std::uint8_t>(value.binary()->size()));
        output.putBytes(value.binary()->data(), value.binary()->size());
        return output.isGood();
    }
    case 1005: {
        if (value.type() != DRW_Variant::STRING || value.content.s == nullptr)
            return false;
        std::uint64_t handle = 0;
        if (!parseEedHexHandle(*value.content.s, handle))
            return false;
        output.putRawChar8(5);
        putEedRawHandleBE(output, handle);
        return output.isGood();
    }
    case 1010:
    case 1011:
    case 1012:
    case 1013: {
        if (value.type() != DRW_Variant::COORD || value.coord() == nullptr)
            return false;
        const DRW_Coord& coordinate = *value.coord();
        if (!std::isfinite(coordinate.x) || !std::isfinite(coordinate.y)
            || !std::isfinite(coordinate.z)) {
            return false;
        }
        output.putRawChar8(static_cast<std::uint8_t>(groupCode - 1000));
        output.putRawDouble(coordinate.x);
        output.putRawDouble(coordinate.y);
        output.putRawDouble(coordinate.z);
        return output.isGood();
    }
    case 1040:
    case 1041:
    case 1042:
        if (value.type() != DRW_Variant::DOUBLE || !std::isfinite(value.d_val()))
            return false;
        output.putRawChar8(static_cast<std::uint8_t>(groupCode - 1000));
        output.putRawDouble(value.d_val());
        return output.isGood();
    case 1070:
        if (value.type() != DRW_Variant::INTEGER
            || value.i_val() < std::numeric_limits<std::int16_t>::min()
            || value.i_val() > std::numeric_limits<std::int16_t>::max()) {
            return false;
        }
        output.putRawChar8(70);
        output.putRawShort16(static_cast<std::uint16_t>(
            static_cast<std::int16_t>(value.i_val())));
        return output.isGood();
    case 1071: {
        std::int64_t integer = 0;
        if (value.type() == DRW_Variant::INTEGER)
            integer = value.i_val();
        else if (value.type() == DRW_Variant::INTEGER64)
            integer = value.i64_val();
        else
            return false;
        if (integer < std::numeric_limits<std::int32_t>::min()
            || integer > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        output.putRawChar8(71);
        output.putRawLong32(static_cast<std::uint32_t>(
            static_cast<std::int32_t>(integer)));
        return output.isGood();
    }
    default:
        return false;
    }
}

} // namespace

bool DRW_Entity::encodeDwgEed(
    DRW::Version version, const std::vector<DRW_Variant*>& extData,
    const std::vector<PendingHandleRef>& appIdRefs,
    const std::vector<PendingHandleRef>& layerRefs,
    std::uint16_t codePage, dwgBufferW& output) {
    if (extData.empty()) {
        output.putBitShort(0);
        return output.isGood();
    }
    if (extData.size() > dwgSafety::MaxEedTotalItems)
        return false;

    std::size_t currentAppStart = 0;
    std::uint32_t appHandle = 0;
    std::uint32_t groupDepth = 0;
    std::uint32_t chunkCount = 0;
    dwgBufferW payload;
    const auto flush = [&]() {
        if (payload.data().empty() || payload.data().size()
                > std::numeric_limits<std::uint16_t>::max()
            || appHandle == 0 || ++chunkCount > dwgSafety::MaxEedChunks) {
            return false;
        }
        output.putBitShort(static_cast<std::uint16_t>(payload.data().size()));
        dwgHandle appId;
        appId.code = 5;
        appId.ref = appHandle;
        output.putHandle(appId);
        output.putBytes(payload.data().data(), payload.data().size());
        payload.reset();
        return output.isGood();
    };

    for (std::size_t index = 0; index < extData.size(); ++index) {
        const DRW_Variant* value = extData[index];
        if (value == nullptr)
            return false;
        if (value->code() == 1001) {
            if (index != currentAppStart) {
                if (groupDepth != 0 || !flush())
                    return false;
            }
            const PendingHandleRef* appIdRef = findEedWriteRef(appIdRefs, index);
            if (appIdRef == nullptr || appIdRef->handleRef == 0) {
                return false;
            }
            appHandle = appIdRef->handleRef;
            currentAppStart = index + 1;
            groupDepth = 0;
            continue;
        }
        if (index < currentAppStart)
            return false;
        if (value->code() == 1002) {
            if (value->type() != DRW_Variant::STRING || value->content.s == nullptr)
                return false;
            if (*value->content.s == "{") {
                ++groupDepth;
            } else if (*value->content.s == "}") {
                if (groupDepth == 0)
                    return false;
                --groupDepth;
            } else {
                return false;
            }
        }

        std::uint64_t layerHandle = 0;
        if (value->code() == 1003) {
            const PendingHandleRef* layerRef = findEedWriteRef(layerRefs, index);
            if (layerRef == nullptr)
                return false;
            layerHandle = layerRef->useRawHandleRef
                ? layerRef->rawHandleRef : layerRef->handleRef;
            if (layerHandle == 0)
                return false;
        }
        dwgBufferW item;
        if (!encodeEedItem(version, *value, output, item, layerHandle,
                           codePage))
            return false;
        if (item.data().size() > std::numeric_limits<std::uint16_t>::max())
            return false;
        if (!payload.data().empty()
            && payload.data().size() + item.data().size()
                > std::numeric_limits<std::uint16_t>::max()) {
            if (groupDepth != 0 || !flush())
                return false;
        }
        payload.putBytes(item.data().data(), item.data().size());
    }
    if (currentAppStart == 0 || groupDepth != 0 || !flush())
        return false;
    output.putBitShort(0);
    return output.isGood();
}

bool DRW_Entity::encodeDwgCommon(DRW::Version version, dwgBufferW *buf,
                                  dwgBufferW *strBuf) {
    (void)strBuf;  // ENC names are byte-oriented TV fields in the data stream
    if (buf == nullptr || reactorHandles.size() > dwgSafety::MaxReactorCount ||
        (version != DRW::AC1015 && version != DRW::AC1018 &&
        version != DRW::AC1021 &&
        version != DRW::AC1024 && version != DRW::AC1027 &&
        version != DRW::AC1032)) return false;

    // Object type: BS for AC1015/AC1018, OT for AC1024+.
    buf->putObjType(version, static_cast<std::uint16_t>(oType));

    // objSize stub — back-patched for AC1015/AC1018 only.  AC1024 derives
    // objSize from the body buffer size, so no RL is emitted.
    if (version < DRW::AC1024) {
        buf->putRawLong32(0);
    }

    // Own handle: code 0 per spec §20.4.1.
    dwgHandle ownH;
    ownH.code = 0;
    ownH.ref  = handle;
    ownH.size = 0;
    if (handle != 0) {
        std::uint32_t t = handle;
        while (t != 0) { t >>= 8; ++ownH.size; }
    }
    buf->putHandle(ownH);

    std::vector<DRW_Variant*> eedValues;
    try {
        eedValues.reserve(extData.size());
        for (const auto& value : extData)
            eedValues.push_back(value.get());
    } catch (...) {
        return false;
    }
    if (!encodeDwgEed(version, eedValues, dwgEedAppIdWriteRefs,
                      dwgEedLayerWriteRefs, dwgEedCodePage, *buf))
        return false;

    // Proxy graphics lives in the common data stream. Preserve it when the
    // entity has a bounded byte stream; it is the cached fallback geometry
    // used for unrecognised custom/proxy entities.
    if (proxyGraphics.size()
        > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()
                                   / 8)) {
        return false;
    }
    const bool hasProxyGraphics = !proxyGraphics.empty();
    buf->putBit(hasProxyGraphics);
    if (hasProxyGraphics) {
        const auto graphDataSize = static_cast<std::uint64_t>(
            proxyGraphics.size());
        if (version >= DRW::AC1024)
            buf->putBitLongLong(graphDataSize);
        else
            buf->putRawLong32(static_cast<std::uint32_t>(graphDataSize));
        buf->putBytes(reinterpret_cast<const std::uint8_t*>(
                          proxyGraphics.data()),
                      proxyGraphics.size());
        if (!buf->isGood())
            return false;
    }

    const bool hasOwner = parentHandle != DRW::NoHandle;
    // entmode BB (ODA §20.4.1 / Open Design FE):
    //   0 = owner handle follows in the handle stream
    //   1 = paperspace entity without owner-relative handle
    //   2 = modelspace entity without owner-relative handle
    // Prefer owner when present; otherwise honor DRW_Entity::space.
    std::uint8_t entmode = 2;
    if (hasOwner)
        entmode = 0;
    else if (space == DRW::PaperSpace)
        entmode = 1;
    buf->put2Bits(entmode);

    // numReactors (BL per spec §20.4.1). 2a.2: emit the real count; empty
    // reactorHandles → 0 → byte-identical to legacy.
#if LIBDXFRW_FULL_COMMON_HEADER
    buf->putBitLong(static_cast<std::int32_t>(reactorHandles.size()));
#else
    buf->putBitLong(0);
#endif

    // R2004/R2010 (AC1018, AC1024): reader reads xDictFlag bit (version > AC1015)
    // then forces haveNextLinks=1 (no bit in stream).  We always emit
    // xDictFlag=0 (xdic-present) so the reader reads exactly one xdic handle
    // in the handle section — we emit the real handle when xDictHandle!=0 and
    // a null handle otherwise. This keeps the empty case byte-identical to the
    // legacy path (bit 0 + null handle) while round-tripping a real xdict.
    // R2000 (AC1015): no xDictFlag bit; reader's xDictFlag stays 0 so it ALWAYS
    // reads an xdic handle — same emit rule applies.
    // R2013+ (AC1027+): reader reads xDictFlag then reads haveNextLinks (bit restored).
    if (version == DRW::AC1015) {
        buf->putBit(1);  // nolinks=1 (R2000: no prev/next chain)
    } else {
        buf->putBit(0);  // xDictFlag=0 (xdic present; real-or-null handle follows)
        if (version > DRW::AC1024) {
            // libreDWG common_entity_data.spec — R2013+ has_ds_data (B). libdxfrw
            // never inlines an ACIS SAB datastore, so emit hasDsData (default 0).
            // The old code emitted literal 1 (mislabeled haveNextLinks), falsely
            // advertising an SAB blob and risking misparse in strict readers.
            buf->putBit(hasDsData);
        }
    }

    // ENC color.  R2004+ retains the true-color, transparency, DBCOLOR, and
    // byte-oriented color/book-name side channels; AC1015 falls back to BS.
    UTF8STRING encColorName = colorName;
    UTF8STRING encBookName;
    const std::string::size_type separator = encColorName.find('$');
    if (separator != std::string::npos) {
        encBookName = encColorName.substr(0, separator);
        encColorName.erase(0, separator + 1);
    }
    buf->putEnColor(version, static_cast<std::uint16_t>(color), color24,
                    encColorName, encBookName,
                    static_cast<std::uint32_t>(transparency),
                    version >= DRW::AC1018 && hasAcDbColorH);

    // ltypeScale BD.
    buf->putBitDouble(ltypeScale);

    // ltFlags BB: 0=BYLAYER, 1=BYBLOCK, 2=CONTINUOUS, 3=lTypeH present.
    // Prefer an already-set ltFlags; otherwise derive from lineType / lTypeH.
    {
        auto upper = [](std::string s) {
            for (char& c : s)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return s;
        };
        std::uint8_t flags = ltFlags;
        if (flags > 3)
            flags = 0;
        if (flags == 0 && lTypeH.ref != 0)
            flags = 3;
        if (flags == 0) {
            const std::string lt = upper(lineType);
            if (lt.empty() || lt == "BYLAYER")
                flags = 0;
            else if (lt == "BYBLOCK")
                flags = 1;
            else if (lt == "CONTINUOUS")
                flags = 2;
            else
                flags = 3;  // named linetype — handle required
        }
        ltFlags = flags;
    }
    buf->put2Bits(ltFlags);
    // plotFlags BB: a nonzero plot-style handle requires the hard-pointer
    // handle in the common handle stream. Keep the default zero path
    // unchanged for entities without an override.
    plotFlags = plotStyle > 0 ? 3 : 0;
    buf->put2Bits(plotFlags);

    // R2010 (AC1024): materialFlag BB + shadowFlag RC (version > AC1018).
    if (version > DRW::AC1018) {
        materialFlag = material == DRW::MaterialByLayer ? 0 : 3;
        shadowFlag = static_cast<std::uint8_t>(shadow) & 0x3;
        buf->put2Bits(materialFlag);
        buf->putRawChar8(shadowFlag);
    }

    // R2010 (AC1024): three visual-style flag bits (version > AC1021).
    if (version > DRW::AC1021) {
        hasFullVisualStyle = fullVisualStyleHandle != 0;
        hasFaceVisualStyle = faceVisualStyleHandle != 0;
        hasEdgeVisualStyle = edgeVisualStyleHandle != 0;
        buf->putBit(hasFullVisualStyle);
        buf->putBit(hasFaceVisualStyle);
        buf->putBit(hasEdgeVisualStyle);
    }

    // invisibleFlag BS (DXF 60). 2a.1: emit from `visible` (bit 0 = invisible)
    // instead of a hardcoded 0. visible==true → 0 → byte-identical to legacy.
#if LIBDXFRW_FULL_COMMON_HEADER
    buf->putBitShort(visible ? 0 : 1);
#else
    buf->putBitShort(0);
#endif

    // lWeight RC (0 = byLayer per DRW_LW_Conv).
    buf->putRawChar8(static_cast<std::uint8_t>(lWeight));

    return buf->isGood();
}

bool DRW_Entity::encodeDwgEntHandle(DRW::Version version, dwgBufferW *buf,
                                     dwgBufferW *handleBuf) {
    if (buf == nullptr || reactorHandles.size() > dwgSafety::MaxReactorCount ||
        (version != DRW::AC1015 && version != DRW::AC1018 &&
        version != DRW::AC1021 &&
        version != DRW::AC1024 && version != DRW::AC1027 &&
        version != DRW::AC1032)) return false;

    // For AC1024, handles are directed to handleBuf (the separate handle section);
    // for AC1015/AC1018, handles go into buf alongside the data.
    dwgBufferW *hb = (handleBuf != nullptr) ? handleBuf : buf;

    // Owner handle is present only when encodeDwgCommon emitted entmode=0.
    if (parentHandle != DRW::NoHandle) {
        dwgHandle owner;
        owner.code = 4;  // soft pointer owner, read via getOffsetHandle()
        owner.ref = parentHandle;
        owner.size = 0;
        std::uint32_t t = parentHandle;
        while (t != 0) { t >>= 8; ++owner.size; }
        hb->putHandle(owner);
    }

    // Reactor handles (2a.2): emitted before xdic, one per numReactors written
    // in the DATA section, as ABSOLUTE handles (reader uses getHandle()). Empty
    // reactorHandles → nothing emitted → byte-identical to legacy.
#if LIBDXFRW_FULL_COMMON_HEADER
    for (std::uint32_t ref : reactorHandles) {
        dwgHandle rh;
        rh.code = 4;  // soft pointer
        rh.ref  = ref;
        rh.size = 0;
        if (ref != 0) { std::uint32_t t = ref; while (t != 0) { t >>= 8; ++rh.size; } }
        hb->putHandle(rh);
    }
#endif

    // XDic handle — xDictFlag=0 in the DATA section means the reader reads one
    // XDicObj handle here: emit the real handle when xDictHandle!=0, else the
    // null handle (matching the legacy byte-for-byte for the empty case).
    dwgHandle xDic;
    xDic.code = 3;
#if LIBDXFRW_FULL_COMMON_HEADER
    xDic.ref  = xDictHandle;
    xDic.size = 0;
    if (xDictHandle != 0) {
        std::uint32_t t = xDictHandle; while (t != 0) { t >>= 8; ++xDic.size; }
    }
#else
    xDic.ref  = 0;
    xDic.size = 0;
#endif
    hb->putHandle(xDic);

    // R2004+ stores the optional AcDbColor reference after the common owner,
    // reactor, and xdictionary handles. Keep this order aligned with the
    // reader and the cross-project handle-stream contract.
    if (version > DRW::AC1015 && hasAcDbColorH)
        putHardPointerHandle(hb, acDbColorHandle);

    // Layer handle (R2000+ unconditional).  Hard pointer.
    dwgHandle lH;
    lH.code = layerH.ref == 0 ? 0 : 5;  // 5 = hard pointer for layer ref
    lH.ref  = layerH.ref;
    lH.size = 0;
    if (lH.ref != 0) {
        std::uint32_t t = lH.ref;
        while (t != 0) { t >>= 8; ++lH.size; }
    }
    hb->putHandle(lH);

    // ltFlags=3 → lTypeH (hard pointer, code 5) present; else omit.
    if (ltFlags == 3) {
        dwgHandle ltH;
        ltH.code = lTypeH.ref == 0 ? 0 : 5;
        ltH.ref = lTypeH.ref;
        ltH.size = 0;
        if (ltH.ref != 0) {
            std::uint32_t t = ltH.ref;
            while (t != 0) { t >>= 8; ++ltH.size; }
        }
        hb->putHandle(ltH);
    }
    auto putHardPointer = [hb](std::uint32_t ref) {
        dwgHandle h;
        h.code = ref == 0 ? 0 : 5;
        h.ref = ref;
        h.size = 0;
        if (ref != 0) {
            std::uint32_t t = ref;
            while (t != 0) {
                t >>= 8;
                ++h.size;
            }
        }
        hb->putHandle(h);
    };

    if (version > DRW::AC1018) {
        if (materialFlag == 3)
            putHardPointer(material);
        if (shadowFlag == 3)
            putHardPointer(shadowHandle);
    }
    if (plotFlags == 3)
        putHardPointer(static_cast<std::uint32_t>(plotStyle));
    if (version > DRW::AC1021) {
        if (hasFullVisualStyle)
            putHardPointer(fullVisualStyleHandle);
        if (hasFaceVisualStyle)
            putHardPointer(faceVisualStyleHandle);
        if (hasEdgeVisualStyle)
            putHardPointer(edgeVisualStyleHandle);
    }

    return buf->isGood() && hb->isGood();
}

bool DRW_Point::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 27;  // POINT class id — see dwgreader.cpp:1111 dispatch
    if (!encodeDwgCommon(version, buf)) return false;

    // Point body — mirror of DRW_Point::parseDwg below.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putBitDouble(xAxisAngle);  // ODA §20.4.31 code 50

    return encodeDwgEntHandle(version, buf, handleBuf);
}

void DRW_Point::resetDwgState() {
    DRW_Entity::reset();
    basePoint = DRW_Coord{0.0, 0.0, 0.0};
    thickness = 0.0;
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
    xAxisAngle = 0.0;
}

bool DRW_Point::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtrusion;
    double parsedThickness = 0.0;
    double parsedXAxisAngle = 0.0;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedBasePoint)
        || !readBoundedThickness(bodyProbe, bodyEndBit,
                                  version > DRW::AC1014, parsedThickness)
        || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                 version > DRW::AC1014, parsedExtrusion)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedXAxisAngle)
        || !std::isfinite(parsedBasePoint.x)
        || !std::isfinite(parsedBasePoint.y)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedThickness)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z)
        || !std::isfinite(parsedXAxisAngle))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedBasePoint;
    thickness = parsedThickness;
    extPoint = parsedExtrusion;
    xAxisAngle = parsedXAxisAngle;
    *sourceBuf = handleProbe;
    return true;
}

void DRW_Line::resetDwgState() {
    DRW_Point::resetDwgState();
    secPoint = DRW_Coord{0.0, 0.0, 0.0};
}

bool DRW_Line::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 11:
        secPoint.x = reader->getDouble();
        break;
    case 21:
        secPoint.y = reader->getDouble();
        break;
    case 31:
        secPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Line::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 19;  // LINE class id — see dwgreader.cpp:1105
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ Line body — zIsZero shortcut: if both z's are 0, omit the
    // z fields entirely.  Reader reads `zIsZero` first, then RD x +
    // DD secX (default = x), RD y + DD secY (default = y), and
    // conditionally RD z + DD secZ.  Our putDefaultDouble always emits
    // the full RD via code 0b11; reader's getDefaultDouble with code
    // 0b11 returns the raw double.
    bool zIsZero = (basePoint.z == 0.0 && secPoint.z == 0.0);
    buf->putBit(zIsZero ? 1 : 0);
    buf->putRawDouble(basePoint.x);
    buf->putDefaultDouble(basePoint.x, secPoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    if (!zIsZero) {
        buf->putRawDouble(basePoint.z);
        buf->putDefaultDouble(basePoint.z, secPoint.z);
    }
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_3DLine::parseDwg(DRW::Version version, dwgBuffer *buf,
                          std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    double values[10]{};
    for (double& value : values) {
        if (!readBoundedRawDouble(bodyProbe, bodyEndBit, value)
            || !std::isfinite(value))
            return fail();
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = DRW_Coord{values[0], values[1], values[2]};
    secPoint = DRW_Coord{values[3], values[4], values[5]};
    extPoint = DRW_Coord{values[6], values[7], values[8]};
    thickness = values[9];
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_3DLine::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           std::uint32_t bs, dwgBufferW *strBuf,
                           dwgBufferW *handleBuf) {
    (void)bs;
    if (version < DRW::AC1015 || buf == nullptr)
        return false;

    // 3DLINE is a modern custom class. Its class ordinal is writer-local,
    // while the body remains the fixed raw-double layout used by the reader.
    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    for (const double coordinate : {basePoint.x, basePoint.y, basePoint.z,
                                   secPoint.x, secPoint.y, secPoint.z,
                                   extPoint.x, extPoint.y, extPoint.z,
                                   thickness}) {
        buf->putRawDouble(coordinate);
    }

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Circle::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 18;  // CIRCLE class id — see dwgreader.cpp:1099
    if (!encodeDwgCommon(version, buf)) return false;

    // Circle body — mirror of DRW_Circle::parseDwg.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(radious);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Ray::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    // Ray = 40, Xline = 41 — derive from runtime type so DRW_Xline can
    // share this encoder (it inherits from DRW_Ray).
    oType = (eType == DRW::XLINE) ? 41 : 40;
    if (!encodeDwgCommon(version, buf)) return false;

    // 3 BD basePoint + 3 BD vector — same layout as parseDwg.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(secPoint.x);
    buf->putBitDouble(secPoint.y);
    buf->putBitDouble(secPoint.z);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Trace::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 32;  // TRACE = 32 — see dwgreader.cpp:1317
    if (!encodeDwgCommon(version, buf)) return false;

    // Trace body — mirror of parseDwg.  Note the unusual layout:
    // thickness FIRST, then elevation (basePoint.z) as BD, then 4
    // corners as 2RD (z values share basePoint.z).
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putBitDouble(basePoint.z);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putRawDouble(secPoint.x);
    buf->putRawDouble(secPoint.y);
    buf->putRawDouble(thirdPoint.x);
    buf->putRawDouble(thirdPoint.y);
    buf->putRawDouble(fourPoint.x);
    buf->putRawDouble(fourPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Spline::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    oType = 36;  // SPLINE class id — see dwgreader.cpp:1329
    if (!encodeDwgCommon(version, buf)) return false;
    encodeDwgSplineBody(version, buf);
    return encodeDwgEntHandle(version, buf, handleBuf);
}

// Spline body encode: the scenario/degree/knots/ctrl/fit section, WITHOUT the
// leading encodeDwgCommon or the trailing encodeDwgEntHandle. Factored out so
// DRW_Helix::encodeDwg can reuse the identical payload (Phase 8a-1).
// Omits the DXF-only flag70/extrusion (210-230) which the DWG stream never
// carries here.
void DRW_Spline::encodeDwgSplineBody(DRW::Version version, dwgBufferW *buf) const {
    // Scenario:
    //   1 = control-point / rational / planar (uses knots + control + weights)
    //   2 = fit-point (uses fit points + tangents + tolerance)
    // When both lists are populated (e.g. DXF-sourced splines), prefer scenario 1
    // (ctrl + knots) and drop the fit list from the DWG stream — scenario 1 has no
    // fit-point section, so writing both would corrupt all subsequent entities.
    const bool hasFit  = !fitlist.empty();
    const bool hasCtrl = !controllist.empty();
    std::int32_t scenario = (hasFit && !hasCtrl) ? 2 : 1;
    if (m_scenario == 1 && hasCtrl) {
        scenario = 1;
    } else if (m_scenario == 2 && hasFit) {
        scenario = 2;
    }
    buf->putBitLong(scenario);
    if (version > DRW::AC1024) {
        // splFlag1 bit 0: method = fit points; bit 2: closed;
        // bit 3: knotParam participates in R2013+ scenario selection.
        std::int32_t splFlag1 = m_splineFlags1;
        splFlag1 &= ~(kSplineFlagMethodFitPoints | kSplineFlagUseKnotParameter | kSplineFlagClosed);
        if (scenario == 2) {
            splFlag1 |= kSplineFlagMethodFitPoints | kSplineFlagUseKnotParameter;
            if (flags & 0x01) splFlag1 |= kSplineFlagClosed;
        } else {
            if (flags & 0x01) splFlag1 |= kSplineFlagClosed;
        }
        buf->putBitLong(splFlag1);
        std::int32_t knotParam = m_knotParam;
        if (scenario == 1) {
            knotParam = kSplineKnotParamCustom;
        } else if (knotParam == kSplineKnotParamCustom) {
            knotParam = 0;
        }
        buf->putBitLong(knotParam);
    }
    buf->putBitLong(static_cast<std::int32_t>(degree));

    if (scenario == 2) {
        buf->putBitDouble(tolfit);
        buf->put3BitDouble(tgStart);
        buf->put3BitDouble(tgEnd);
        const std::int32_t nFit = static_cast<std::int32_t>(fitlist.size());
        buf->putBitLong(nFit);
    } else {
        // scenario == 1
        // Reader at parseDwg reads three flag bits in this order:
        //   rational bit  (flags bit 2 → 0x04)
        //   closed bit    (flags bit 0 → 0x01)
        //   periodic bit  (flags bit 1 → 0x02)
        const bool hasNonDefaultWeights = std::any_of(weightlist.begin(), weightlist.end(), differsFromUnitWeight);
        buf->putBit(((flags & 0x4) || hasNonDefaultWeights) ? 1 : 0);  // rational
        buf->putBit((flags & 0x1) ? 1 : 0);  // closed
        buf->putBit((flags & 0x2) ? 1 : 0);  // periodic
        buf->putBitDouble(tolknot);
        buf->putBitDouble(tolcontrol);
        const std::int32_t nKnots = static_cast<std::int32_t>(knotslist.size());
        const std::int32_t nCtrl  = static_cast<std::int32_t>(controllist.size());
        buf->putBitLong(nKnots);
        buf->putBitLong(nCtrl);
        // weight bit: caller populates weightlist when each control point
        // has a non-default weight (NURBS conics).
        bool hasWeights = !weightlist.empty();
        buf->putBit(hasWeights ? 1 : 0);
    }

    // Data sections are scenario-gated to avoid stream corruption:
    // parseDwg reads knots+ctrl only for scenario 1, fit only for scenario 2.
    if (scenario == 1) {
        for (double k : knotslist) buf->putBitDouble(k);
        for (size_t i = 0; i < controllist.size(); ++i) {
            buf->put3BitDouble(*controllist[i]);
            if (!weightlist.empty()) {
                double w = (i < weightlist.size()) ? weightlist[i] : 1.0;
                buf->putBitDouble(w);
            }
        }
    } else {
        for (const auto& fp : fitlist) buf->put3BitDouble(*fp);
    }
}

// DRW_Helix::encodeDwg — spline body (oType = HELIX class 503) + AcDbHelix
// trailer, then the common entity handle data. Trailer field order MUST match
// DRW_Helix::parseDwg (Phase 8a-1).
bool DRW_Helix::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (buf == nullptr || !validatePayloadFields()
        || !finite(axisBasePt) || !finite(startPt) || !finite(axisVector)
        || !std::isfinite(radius) || !std::isfinite(turns)
        || !std::isfinite(turnHeight))
        return false;
    oType = kDwgClassNum;  // HELIX custom class 503
    if (!encodeDwgCommon(version, buf)) return false;
    encodeDwgSplineBody(version, buf);

    // AcDbHelix trailer (same order as parseDwg):
    buf->putBitLong(m_majorVersion);
    buf->putBitLong(m_maintVersion);
    buf->put3BitDouble(axisBasePt);
    buf->put3BitDouble(startPt);
    buf->put3BitDouble(axisVector);
    buf->putBitDouble(radius);
    buf->putBitDouble(turns);
    buf->putBitDouble(turnHeight);
    buf->putBit(handedness ? 1 : 0);
    buf->putRawChar8(static_cast<std::uint8_t>(constraintType));

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_MText::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 44;  // MTEXT class id — see dwgreader.cpp:1215
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000/R2004/R2010 MTEXT body — mirror of DRW_MText::parseDwg.
    buf->put3BitDouble(basePoint);          // insertion
    buf->put3BitDouble(extPoint);           // extrusion
    buf->put3BitDouble(secPoint);           // X-axis dir
    buf->putBitDouble(widthscale);          // rect width
    if (version > DRW::AC1018) {
        buf->putBitDouble(0.0);             // rect height, R2007+
    }
    buf->putBitDouble(height);              // text height
    buf->putBitShort(static_cast<std::uint16_t>(textgen));  // attachment
    buf->putBitShort(static_cast<std::uint16_t>(alignH));   // drawing dir
    buf->putBitDouble(0.0);                 // ext_ht (extents height; undocumented)
    buf->putBitDouble(0.0);                 // ext_wid (extents width; undocumented)
    // For AC1024: text goes to string buffer; for AC1015/AC1018: inline.
    (strBuf ? strBuf : buf)->putVariableText(version, text);
    // R2000+ extras:
    buf->putBitShort(linespacingStyle);     // linespacing style BS 73
    buf->putBitDouble(interlin);            // linespacing factor BD
    buf->putBit(0);                         // unknown bit
    if (version > DRW::AC1015) {            // R2004+: background flags BL
        buf->putBitLong(m_backgroundFlags);
        if ((m_backgroundFlags & 0x01) || (version >= DRW::AC1032 && (m_backgroundFlags & 0x10))) {
            buf->putBitDouble(m_backgroundScale);  // BitDouble (matches the read fix)
            buf->putCmColor(version, static_cast<std::uint16_t>(m_backgroundColor));
            buf->putBitLong(m_backgroundTransparency);
        }
    }
    if (version >= DRW::AC1032) {
        buf->putBit(m_r2018IsNotAnnotative ? 1 : 0);
        if (m_r2018IsNotAnnotative) {
            buf->putBitShort(m_r2018Version);
            buf->putBit(m_r2018DefaultFlag ? 1 : 0);
            buf->putBitLong(m_r2018Attachment);
            buf->put3BitDouble(m_r2018XAxisDir);
            buf->put3BitDouble(m_r2018InsertionPoint);
            buf->putBitDouble(m_r2018RectWidth);
            buf->putBitDouble(m_r2018RectHeight);
            buf->putBitDouble(m_r2018ExtentsHeight);
            buf->putBitDouble(m_r2018ExtentsWidth);
            if (m_r2018ColumnType > 2
                || m_r2018ColumnCount < 0
                || m_r2018ColumnCount > kMaxMTextColumnHeights
                || m_r2018ColumnHeights.size()
                    > static_cast<std::size_t>(kMaxMTextColumnHeights))
                return false;
            buf->putBitShort(m_r2018ColumnType);
            if (m_r2018ColumnType != 0) {
                std::int32_t columnCount = m_r2018ColumnCount;
                if (!m_r2018ColumnAutoHeight && m_r2018ColumnType == 2
                    && !m_r2018ColumnHeights.empty()) {
                    columnCount = static_cast<std::int32_t>(m_r2018ColumnHeights.size());
                }
                if (!m_r2018ColumnAutoHeight && m_r2018ColumnType == 2
                    && static_cast<std::size_t>(columnCount)
                        != m_r2018ColumnHeights.size())
                    return false;
                buf->putBitLong(columnCount);
                buf->putBitDouble(m_r2018ColumnWidth);
                buf->putBitDouble(m_r2018ColumnGutter);
                buf->putBit(m_r2018ColumnAutoHeight ? 1 : 0);
                buf->putBit(m_r2018ColumnFlowReversed ? 1 : 0);
                if (!m_r2018ColumnAutoHeight && m_r2018ColumnType == 2) {
                    for (std::int32_t i = 0; i < columnCount; ++i) {
                        const double columnHeight = static_cast<size_t>(i) < m_r2018ColumnHeights.size()
                            ? m_r2018ColumnHeights[static_cast<size_t>(i)]
                            : 0.0;
                        buf->putBitDouble(columnHeight);
                    }
                }
            }
        }
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // styleH — hard pointer to STYLE table record (default STANDARD).
    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    putHardPointerHandle(hb, (styleH.ref == 0) ? 0x13 : styleH.ref);
    if (version >= DRW::AC1032 && m_r2018IsNotAnnotative)
        putHardPointerHandle(hb, (m_r2018AppIdHandle == 0) ? 0x14 : m_r2018AppIdHandle);
    return buf->isGood() && hb->isGood();
}

bool DRW_Insert::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr)
        return false;
    const bool hasAttrib = !attribHandles.empty();
    if (!hasAttrib) {
        if (seqendH.ref != DRW::NoHandle)
            return false;
    } else {
        if (seqendH.ref == DRW::NoHandle)
            return false;
        if (version < DRW::AC1018) {
            if (attribHandles.size() != 2 ||
                (attribHandles.front().ref == DRW::NoHandle) !=
                    (attribHandles.back().ref == DRW::NoHandle)) {
                return false;
            }
        } else if (std::any_of(attribHandles.cbegin(), attribHandles.cend(),
                               [](const dwgHandle& handle) {
                                   return handle.ref == DRW::NoHandle;
                               })) {
            return false;
        }
    }
    if (attribHandles.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        return false;

    // 2b.6: emit MINSERT (oType 8) when a column/row grid is present;
    // otherwise a plain INSERT (oType 7). The reader keys the grid block off
    // oType==8 (parseDwg :3189).
    oType = isMInsert() ? 8 : 7;
    if (!encodeDwgCommon(version, buf)) return false;

    // INSERT body — mirror of DRW_Insert::parseDwg for R2000.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);

    // dataFlags: pick the most compact form based on actual scales.
    //   3 → all scales default to 1.0 (no emit)
    //   2 → uniform scale (xscale RD only; yscale=zscale=xscale)
    //   1 → xscale defaults to 1, yscale/zscale as DD against xscale
    //   0 → xscale RD; yscale/zscale as DD against xscale
    if (sameStoredDouble(xscale, 1.0) && sameStoredDouble(yscale, 1.0)
        && sameStoredDouble(zscale, 1.0)) {
        buf->put2Bits(3);
    } else if (sameStoredDouble(xscale, yscale) && sameStoredDouble(yscale, zscale)) {
        buf->put2Bits(2);
        buf->putRawDouble(xscale);
    } else if (sameStoredDouble(xscale, 1.0)) {
        // xscale is exactly 1.0 (parseDwg leaves it at its 1.0 default and
        // never reads it in this branch), so it can be omitted; y and z are
        // independent and go through the same DD-against-1.0 path parseDwg
        // reads them with.
        buf->put2Bits(1);
        buf->putDefaultDouble(1.0, yscale);
        buf->putDefaultDouble(1.0, zscale);
    } else {
        // Use dataFlags=0 (general case): RD x + DD y + DD z.
        buf->put2Bits(0);
        buf->putRawDouble(xscale);
        buf->putDefaultDouble(xscale, yscale);
        buf->putDefaultDouble(xscale, zscale);
    }

    buf->putBitDouble(angle);                // radians
    buf->putExtrusion(extPoint, /*b_R2000_style=*/false);
    buf->putBit(hasAttrib ? 1 : 0);
    if (hasAttrib && version > DRW::AC1015)
        buf->putBitLong(static_cast<std::int32_t>(attribHandles.size()));
    // hasAttrib==0 ⇒ the SINCE-R2004 num_owned BL is absent (parse :3184), so
    // the MINSERT grid (oType==8) follows the hasAttrib bit directly. Field
    // order mirrors parseDwg :3190-3193 (colcount BS, rowcount BS, colspace BD,
    // rowspace BD) and libreDWG dwg.spec num_cols/num_rows/col_spacing/row_spacing.
    if (oType == 8) {  // MINSERT grid
        buf->putBitShort(static_cast<std::uint16_t>(colcount));
        buf->putBitShort(static_cast<std::uint16_t>(rowcount));
        buf->putBitDouble(colspace);
        buf->putBitDouble(rowspace);
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // BLOCK_RECORD hard pointer.
    dwgHandle bhH;
    bhH.code = (blockRecH.ref == 0) ? 0 : 5;
    bhH.ref  = blockRecH.ref;
    bhH.size = 0;
    if (bhH.ref != 0) {
        std::uint32_t t = bhH.ref;
        while (t != 0) { t >>= 8; ++bhH.size; }
    }
    (handleBuf ? handleBuf : buf)->putHandle(bhH);

    if (hasAttrib) {
        dwgBufferW *hb = handleBuf ? handleBuf : buf;
        if (version < DRW::AC1018) {
            for (const dwgHandle& attribHandle : attribHandles)
                putDwgReference(hb, DRW::DwgSoftPointer, attribHandle.ref);
        } else {
            const std::uint8_t attributeCode =
                oType == 8 ? DRW::DwgSoftPointer : DRW::DwgHardOwnership;
            for (const dwgHandle& attribHandle : attribHandles)
                putDwgReference(hb, attributeCode, attribHandle.ref);
        }
        putDwgReference(hb, DRW::DwgHardOwnership, seqendH.ref);
    }
    return buf->isGood() && (handleBuf == nullptr || handleBuf->isGood());
}

bool DRW_3Dface::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 28;  // 3DFACE class id — see dwgreader.cpp:1237
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ 3DFACE body — mirror of parseDwg's z_is_zero / has_no_flag
    // optimization.  Reader checks `invisibleflag != NoEdge`; if NoEdge,
    // emit has_no_flag=1 to suppress the BS read.
    bool hasNoFlag = (invisibleflag == /*NoEdge*/0);
    bool zIsZero   = (basePoint.z == 0.0);
    buf->putBit(hasNoFlag ? 1 : 0);
    buf->putBit(zIsZero ? 1 : 0);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    if (!zIsZero) buf->putRawDouble(basePoint.z);
    buf->putDefaultDouble(basePoint.x, secPoint.x);
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putDefaultDouble(basePoint.z, secPoint.z);
    buf->putDefaultDouble(secPoint.x, thirdPoint.x);
    buf->putDefaultDouble(secPoint.y, thirdPoint.y);
    buf->putDefaultDouble(secPoint.z, thirdPoint.z);
    buf->putDefaultDouble(thirdPoint.x, fourPoint.x);
    buf->putDefaultDouble(thirdPoint.y, fourPoint.y);
    buf->putDefaultDouble(thirdPoint.z, fourPoint.z);
    if (!hasNoFlag) buf->putBitShort(static_cast<std::uint16_t>(invisibleflag));

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Solid::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 31;  // SOLID class id — see dwgreader.cpp:1305
    if (!encodeDwgCommon(version, buf)) return false;

    // Same body layout as TRACE (4 corners + extrusion).  Duplicated
    // here rather than delegating to DRW_Trace::encodeDwg because that
    // hardcodes oType=32.
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putBitDouble(basePoint.z);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putRawDouble(secPoint.x);
    buf->putRawDouble(secPoint.y);
    buf->putRawDouble(thirdPoint.x);
    buf->putRawDouble(thirdPoint.y);
    buf->putRawDouble(fourPoint.x);
    buf->putRawDouble(fourPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_LWPolyline::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    oType = 77;  // LWPOLYLINE class id — see dwgreader.cpp:1202
    if (!encodeDwgCommon(version, buf)) return false;

    // DRW_LWPolyline::flags carries DXF-side bits (1=closed, 128=plinegen).
    // DWG-side flags are different: they signal which optional fields are
    // present.  Per parseDwg, bit 9 (0x200) = closed, bit 8 (0x100) =
    // plinegen.  Build the DWG flags from the DXF flags plus the data.
    std::uint16_t dwgFlags = 0;
    if (flags & 1)   dwgFlags |= 0x200;   // closed
    if (flags & 128) dwgFlags |= 0x100;   // plinegen
    if (thickness != 0.0) dwgFlags |= 0x2;
    if (width     != 0.0) dwgFlags |= 0x4;
    if (elevation != 0.0) dwgFlags |= 0x8;
    bool defaultExt = (extPoint.x == 0.0 && extPoint.y == 0.0 && extPoint.z == 1.0);
    if (!defaultExt) dwgFlags |= 0x1;
    // Detect per-vertex bulge / width data.
    bool anyBulge = false;
    bool anyWidth = false;
    bool anyVertexId = false;
    for (const auto& v : vertlist) {
        if (v && v->bulge    != 0.0) anyBulge = true;
        if (v && (v->stawidth != 0.0 || v->endwidth != 0.0)) anyWidth = true;
        if (v && v->identifier != 0) anyVertexId = true;
    }
    if (anyBulge) dwgFlags |= 0x10;
    if (anyWidth) dwgFlags |= 0x20;
    if (version > DRW::AC1021 && anyVertexId) dwgFlags |= 0x400;

    buf->putBitShort(dwgFlags);
    if (dwgFlags & 0x4)  buf->putBitDouble(width);
    if (dwgFlags & 0x8)  buf->putBitDouble(elevation);
    if (dwgFlags & 0x2)  buf->putBitDouble(thickness);
    if (dwgFlags & 0x1)  buf->putExtrusion(extPoint, /*b_R2000_style=*/false);

    const std::int32_t numVerts = static_cast<std::int32_t>(vertlist.size());
    buf->putBitLong(numVerts);
    if (dwgFlags & 0x10) buf->putBitLong(numVerts);  // bulgesnum
    if (version > DRW::AC1021 && (dwgFlags & 0x400)) {
        buf->putBitLong(numVerts);                    // vertexIdCount
    }
    if (dwgFlags & 0x20) buf->putBitLong(numVerts);  // widthsnum

    if (numVerts > 0) {
        // First vertex as 2RD.  Subsequent vertices as 2DD relative to
        // the previous, with putDefaultDouble always emitting code 0b11
        // (full RD); the reader's getDefaultDouble returns the raw value.
        buf->putRawDouble(vertlist[0]->x);
        buf->putRawDouble(vertlist[0]->y);
        for (size_t i = 1; i < vertlist.size(); ++i) {
            buf->putDefaultDouble(vertlist[i-1]->x, vertlist[i]->x);
            buf->putDefaultDouble(vertlist[i-1]->y, vertlist[i]->y);
        }
        if (dwgFlags & 0x10) {
            for (const auto& v : vertlist)
                buf->putBitDouble(v->bulge);
        }
        if (version > DRW::AC1021 && (dwgFlags & 0x400)) {
            for (const auto& v : vertlist)
                buf->putBitLong(static_cast<std::int32_t>(v->identifier));
        }
        if (dwgFlags & 0x20) {
            for (const auto& v : vertlist) {
                buf->putBitDouble(v->stawidth);
                buf->putBitDouble(v->endwidth);
            }
        }
    }

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Block::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    // BLOCK = 4, ENDBLK = 5 per DWG spec.  isEnd controls which.
    oType = isEnd ? 5 : 4;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!isEnd) {
        (strBuf ? strBuf : buf)->putVariableText(version, name);
    }
    if (version > DRW::AC1018) {
        buf->putBit(0);  // unknown bit (R2007+: always 0 for our output)
    }
    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Text::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 1;  // TEXT class id — see dwgreader.cpp:1208
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ TEXT body — mirror of DRW_Text::parseDwg.  We emit
    // data_flags=0 so the reader sees every optional field rather than
    // substituting defaults — keeps the encoder simple, costs ~30 bytes
    // per TEXT versus the most compressed form.
    buf->putRawChar8(0);                              // data_flags=0
    buf->putRawDouble(basePoint.z);                   // elevation RD
    buf->putRawDouble(basePoint.x);                   // insertion 2RD
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.x, secPoint.x);   // alignment 2DD
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putRawDouble(oblique);                       // oblique angle
    // Angle: struct holds degrees; on-disk format is radians.  Reader
    // does `angle *= ARAD` (180/π) after read.  Inverse: divide here.
    buf->putRawDouble(angle / ARAD);
    buf->putRawDouble(height);                        // text height
    buf->putRawDouble(widthscale);                    // width factor
    (strBuf ? strBuf : buf)->putVariableText(version, text);  // text string
    buf->putBitShort(static_cast<std::uint16_t>(textgen));
    buf->putBitShort(static_cast<std::uint16_t>(alignH));
    buf->putBitShort(static_cast<std::uint16_t>(alignV));

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // styleH — hard pointer to STYLE table record.  Default points at
    // the STANDARD textstyle (handle 0x13) if caller hasn't set one.
    dwgHandle sH;
    std::uint32_t sref = (styleH.ref == 0) ? 0x13 : styleH.ref;
    sH.code = 5;  // hard pointer
    sH.ref  = sref;
    sH.size = 0;
    if (sref != 0) {
        std::uint32_t t = sref;
        while (t != 0) { t >>= 8; ++sH.size; }
    }
    (handleBuf ? handleBuf : buf)->putHandle(sH);
    return true;
}

bool DRW_Ellipse::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 35;  // ELLIPSE class id — see dwgreader.cpp:1117
    if (!encodeDwgCommon(version, buf)) return false;

    // Ellipse body — mirror of DRW_Ellipse::parseDwg.
    buf->put3BitDouble(basePoint);       // center
    buf->put3BitDouble(secPoint);        // major axis vector
    buf->put3BitDouble(extPoint);        // extrusion
    buf->putBitDouble(ratio);            // minor/major ratio
    buf->putBitDouble(staparam);         // start parameter
    buf->putBitDouble(endparam);         // end parameter

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Arc::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 17;  // ARC class id — see dwgreader.cpp:1093
    if (!encodeDwgCommon(version, buf)) return false;

    // Arc body — Circle body + 2 BD angles.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(radious);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putBitDouble(staangle);
    buf->putBitDouble(endangle);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Line::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedSecPoint;
    DRW_Coord parsedExtrusion;
    double parsedThickness = 0.0;
    if (version < DRW::AC1015) {
        if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedBasePoint)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedSecPoint))
            return fail();
    } else {
        if (!proxyEntityHasBits(bodyProbe, bodyEndBit, 1))
            return fail();
        const bool zIsZero = bodyProbe.getBit() != 0;
        if (!bodyProbe.isGood()
            || !readBoundedRawDouble(bodyProbe, bodyEndBit,
                                     parsedBasePoint.x)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit,
                                         parsedBasePoint.x,
                                         parsedSecPoint.x)
            || !readBoundedRawDouble(bodyProbe, bodyEndBit,
                                     parsedBasePoint.y)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit,
                                         parsedBasePoint.y,
                                         parsedSecPoint.y))
            return fail();
        if (zIsZero) {
            parsedBasePoint.z = 0.0;
            parsedSecPoint.z = 0.0;
        } else if (!readBoundedRawDouble(bodyProbe, bodyEndBit,
                                         parsedBasePoint.z)
                   || !readBoundedDefaultDouble(bodyProbe, bodyEndBit,
                                                parsedBasePoint.z,
                                                parsedSecPoint.z))
            return fail();
    }
    if (!readBoundedThickness(bodyProbe, bodyEndBit,
                              version > DRW::AC1014, parsedThickness)
        || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                 version > DRW::AC1014, parsedExtrusion)
        || !std::isfinite(parsedBasePoint.x)
        || !std::isfinite(parsedBasePoint.y)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedSecPoint.x)
        || !std::isfinite(parsedSecPoint.y)
        || !std::isfinite(parsedSecPoint.z)
        || !std::isfinite(parsedThickness)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedBasePoint;
    secPoint = parsedSecPoint;
    thickness = parsedThickness;
    extPoint = parsedExtrusion;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Ray::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedDirection;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedBasePoint)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedDirection)
        || !std::isfinite(parsedBasePoint.x)
        || !std::isfinite(parsedBasePoint.y)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedDirection.x)
        || !std::isfinite(parsedDirection.y)
        || !std::isfinite(parsedDirection.z))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedBasePoint;
    secPoint = parsedDirection;
    *sourceBuf = handleProbe;
    return true;
}

void DRW_Circle::applyExtrusion(){
    if (haveExtrusion) {
        //NOTE: Commenting these out causes the the arcs being tested to be located
        //on the other side of the y axis (all x dimensions are negated).
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
    }
}

bool DRW_Circle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        radious = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

void DRW_Circle::resetDwgState() {
    DRW_Point::resetDwgState();
    radious = 0.0;
}

bool DRW_Circle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    radious = 0.0;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedCenter;
    DRW_Coord parsedExtrusion;
    double parsedRadius = 0.0;
    double parsedThickness = 0.0;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedCenter)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedRadius)
        || !readBoundedThickness(bodyProbe, bodyEndBit,
                                  version > DRW::AC1014, parsedThickness)
        || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                 version > DRW::AC1014, parsedExtrusion)
        || !std::isfinite(parsedCenter.x)
        || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z)
        || !std::isfinite(parsedRadius)
        || !std::isfinite(parsedThickness)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedCenter;
    radious = parsedRadius;
    thickness = parsedThickness;
    extPoint = parsedExtrusion;
    *sourceBuf = handleProbe;
    return true;
}

void DRW_Arc::applyExtrusion(){
    DRW_Circle::applyExtrusion();

    if(haveExtrusion){
        // If the extrusion vector has a z value less than 0, the angles for the arc
        // have to be mirrored since DXF files use the right hand rule.
        // Note that the following code only handles the special case where there is a 2D
        // drawing with the z axis heading into the paper (or rather screen). An arbitrary
        // extrusion axis (with x and y values greater than 1/64) may still have issues.
        if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625 && extPoint.z < 0.0) {
            staangle=M_PI-staangle;
            endangle=M_PI-endangle;

            double temp = staangle;
            staangle=endangle;
            endangle=temp;
        }
    }
}

bool DRW_Arc::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 50:
        staangle = reader->getDouble()/ ARAD;
        break;
    case 51:
        endangle = reader->getDouble()/ ARAD;
        break;
    default:
        return DRW_Circle::parseCode(code, reader);
    }

    return true;
}

bool DRW_Arc::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Circle::resetDwgState();
    radious = 0.0;
    staangle = 0.0;
    endangle = 0.0;
    isccw = 1;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Circle::resetDwgState();
        staangle = 0.0;
        endangle = 0.0;
        isccw = 1;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedCenter;
    DRW_Coord parsedExtrusion;
    double parsedRadius = 0.0;
    double parsedThickness = 0.0;
    double parsedStartAngle = 0.0;
    double parsedEndAngle = 0.0;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedCenter)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedRadius)
        || !readBoundedThickness(bodyProbe, bodyEndBit,
                                  version > DRW::AC1014, parsedThickness)
        || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                 version > DRW::AC1014, parsedExtrusion)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedStartAngle)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedEndAngle)
        || !std::isfinite(parsedCenter.x)
        || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z)
        || !std::isfinite(parsedRadius)
        || !std::isfinite(parsedThickness)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z)
        || !std::isfinite(parsedStartAngle)
        || !std::isfinite(parsedEndAngle))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedCenter;
    radious = parsedRadius;
    thickness = parsedThickness;
    extPoint = parsedExtrusion;
    staangle = parsedStartAngle;
    endangle = parsedEndAngle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Ellipse::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        ratio = reader->getDouble();
        break;
    case 41:
        staparam = reader->getDouble();
        break;
    case 42:
        endparam = reader->getDouble();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

void DRW_Ellipse::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
        extrudePoint(extPoint, &secPoint);
        double intialparam = staparam;
        if (extPoint.z < 0.){
            staparam = M_PIx2 - endparam;
            endparam = M_PIx2 - intialparam;
        }
    }
}

//if ratio > 1 minor axis are greather than major axis, correct it
void DRW_Ellipse::correctAxis(){
    bool complete = false;
    if (staparam == endparam) {
        staparam = 0.0;
        endparam = M_PIx2; //2*M_PI;
        complete = true;
    }
    if (ratio > 1){
        if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10)
            complete = true;
        double incX = secPoint.x;
        secPoint.x = -(secPoint.y * ratio);
        secPoint.y = incX*ratio;
        ratio = 1/ratio;
        if (!complete){
            if (staparam < M_PI_2)
                staparam += M_PI *2;
            if (endparam < M_PI_2)
                endparam += M_PI *2;
            endparam -= M_PI_2;
            staparam -= M_PI_2;
        }
    }
}

bool DRW_Ellipse::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    ratio = 1.0;
    staparam = 0.0;
    endparam = 0.0;
    isccw = 1;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        ratio = 1.0;
        staparam = 0.0;
        endparam = 0.0;
        isccw = 1;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedCenter;
    DRW_Coord parsedAxis;
    DRW_Coord parsedExtrusion;
    double parsedRatio = 1.0;
    double parsedStart = 0.0;
    double parsedEnd = 0.0;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedCenter)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedAxis)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedExtrusion)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedRatio)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedStart)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedEnd)
        || !std::isfinite(parsedCenter.x)
        || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z)
        || !std::isfinite(parsedAxis.x)
        || !std::isfinite(parsedAxis.y)
        || !std::isfinite(parsedAxis.z)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z)
        || !std::isfinite(parsedRatio)
        || !std::isfinite(parsedStart)
        || !std::isfinite(parsedEnd))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = parsedCenter;
    secPoint = parsedAxis;
    extPoint = parsedExtrusion;
    ratio = parsedRatio;
    staparam = parsedStart;
    endparam = parsedEnd;
    *sourceBuf = handleProbe;
    return true;
}

//parts are the number of vertex to split polyline, default 128
void DRW_Ellipse::toPolyline(DRW_Polyline *pol, int parts){
    double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
    double cosCurr, sinCurr;
	radMajor = hypot(secPoint.x, secPoint.y);
    radMinor = radMajor*ratio;
    //calculate sin & cos of included angle
    incAngle = atan2(secPoint.y, secPoint.x);
    cosRot = cos(incAngle);
    sinRot = sin(incAngle);
    incAngle = M_PIx2 / parts;
    curAngle = staparam;
    int i = static_cast<int>(curAngle / incAngle);
    do {
        if (curAngle > endparam) {
            curAngle = endparam;
            i = parts+2;
        }
        cosCurr = cos(curAngle);
        sinCurr = sin(curAngle);
        double x = basePoint.x + (cosCurr*cosRot*radMajor) - (sinCurr*sinRot*radMinor);
        double y = basePoint.y + (cosCurr*sinRot*radMajor) + (sinCurr*cosRot*radMinor);
        pol->addVertex( DRW_Vertex(x, y, 0.0, 0.0));
        curAngle = (++i)*incAngle;
    } while (i<parts);
    if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10){
        pol->flags = 1;
    }
    pol->layer = this->layer;
    pol->lineType = this->lineType;
    pol->color = this->color;
    pol->lWeight = this->lWeight;
    pol->extPoint = this->extPoint;
}

void DRW_Trace::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
        extrudePoint(extPoint, &secPoint);
        extrudePoint(extPoint, &thirdPoint);
        extrudePoint(extPoint, &fourPoint);
    }
}

bool DRW_Trace::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 12:
        thirdPoint.x = reader->getDouble();
        break;
    case 22:
        thirdPoint.y = reader->getDouble();
        break;
    case 32:
        thirdPoint.z = reader->getDouble();
        break;
    case 13:
        fourPoint.x = reader->getDouble();
        break;
    case 23:
        fourPoint.y = reader->getDouble();
        break;
    case 33:
        fourPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

void DRW_Trace::resetDwgState() {
    DRW_Line::resetDwgState();
    thirdPoint = DRW_Coord{0.0, 0.0, 0.0};
    fourPoint = DRW_Coord{0.0, 0.0, 0.0};
}

bool DRW_Trace::parseDwg(DRW::Version version, dwgBuffer *buf,
                         std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    double parsedThickness = 0.0;
    double parsedElevation = 0.0;
    std::array<double, 8> coordinates{};
    DRW_Coord parsedExtrusion{0.0, 0.0, 1.0};
    if (!readBoundedThickness(bodyProbe, bodyEndBit,
                              version > DRW::AC1014, parsedThickness)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedElevation))
        return fail();
    for (double& coordinate : coordinates) {
        if (!readBoundedRawDouble(bodyProbe, bodyEndBit, coordinate)
            || !std::isfinite(coordinate))
            return fail();
    }
    if (!std::isfinite(parsedThickness)
        || !std::isfinite(parsedElevation)
        || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                 version > DRW::AC1014, parsedExtrusion)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    thickness = parsedThickness;
    basePoint = DRW_Coord{coordinates[0], coordinates[1], parsedElevation};
    secPoint = DRW_Coord{coordinates[2], coordinates[3], parsedElevation};
    thirdPoint = DRW_Coord{coordinates[4], coordinates[5], parsedElevation};
    fourPoint = DRW_Coord{coordinates[6], coordinates[7], parsedElevation};
    extPoint = parsedExtrusion;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Solid::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    DRW_DBG("\n***************************** parsing Solid *********************************************\n");
    return DRW_Trace::parseDwg(v, buf, bs);
}

bool DRW_3Dface::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        invisibleflag = reader->getInt32();
        break;
    default:
        return DRW_Trace::parseCode(code, reader);
    }

    return true;
}

bool DRW_3Dface::parseDwg(DRW::Version v, dwgBuffer *buf,
                          std::uint32_t bs) {
    DRW_Trace::resetDwgState();
    invisibleflag = 0;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Trace::resetDwgState();
        invisibleflag = 0;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(v, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::array<DRW_Coord, 4> points{};
    std::uint16_t parsedInvisibleFlag = 0;
    if (v < DRW::AC1015) {
        for (DRW_Coord& point : points) {
            if (!readBoundedBitDouble(bodyProbe, bodyEndBit, point.x)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit, point.y)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit, point.z))
                return fail();
        }
        if (!readBoundedBitShort(bodyProbe, bodyEndBit,
                                 parsedInvisibleFlag))
            return fail();
    } else {
        if (!proxyEntityHasBits(bodyProbe, bodyEndBit, 2))
            return fail();
        const bool hasNoFlag = bodyProbe.getBit() != 0;
        const bool zIsZero = bodyProbe.getBit() != 0;
        if (!bodyProbe.isGood()
            || !readBoundedRawDouble(bodyProbe, bodyEndBit, points[0].x)
            || !readBoundedRawDouble(bodyProbe, bodyEndBit, points[0].y))
            return fail();
        if (zIsZero) {
            points[0].z = 0.0;
        } else if (!readBoundedRawDouble(bodyProbe, bodyEndBit,
                                         points[0].z)) {
            return fail();
        }

        if (!readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[0].x,
                                     points[1].x)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[0].y,
                                         points[1].y)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[0].z,
                                         points[1].z)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[1].x,
                                         points[2].x)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[1].y,
                                         points[2].y)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[1].z,
                                         points[2].z)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[2].x,
                                         points[3].x)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[2].y,
                                         points[3].y)
            || !readBoundedDefaultDouble(bodyProbe, bodyEndBit, points[2].z,
                                         points[3].z))
            return fail();
        if (hasNoFlag) {
            parsedInvisibleFlag = NoEdge;
        } else if (!readBoundedBitShort(bodyProbe, bodyEndBit,
                                        parsedInvisibleFlag)) {
            return fail();
        }
    }

    for (const DRW_Coord& point : points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y)
            || !std::isfinite(point.z))
            return fail();
    }
    if (parsedInvisibleFlag > AllEdges)
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();

    basePoint = points[0];
    secPoint = points[1];
    thirdPoint = points[2];
    fourPoint = points[3];
    invisibleflag = static_cast<int>(parsedInvisibleFlag);
    *sourceBuf = handleProbe;
    return true;
}

void DRW_ModelerGeometry::resetDwgState() {
    DRW_Entity::reset();
    m_modelerVersion = 0;
    m_bodyBitSize = 0;
    m_objectSize = 0;
    m_isEmpty = false;
    m_hasModelerData = false;
    m_modelerDataUnknownBit = false;
    m_hasWireframe = false;
    m_historyHandle = 0;
    m_rawBytes.clear();
    m_payloadRanges.clear();
    m_wireframe = DRW_AcisBrep();
    m_wireframeDecoded = false;
}

bool DRW_ModelerGeometry::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = v > DRW::AC1018 ? &stringProbe : nullptr;
    bool ret = DRW_Entity::parseDwg(v, &probe, stringStream, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing modeler geometry ******************\n");

    std::uint64_t bodyEndBit = 0;
    if (!entityBodyDataEndBit(
            stringStream != nullptr ? *stringStream : probe,
            v, objSize, bodyEndBit))
        return fail();
    const std::uint64_t sourceEndBit = proxyEntityEndBit(*sourceBuf, 0);
    if ((v >= DRW::AC1015 && objSize == 0) || bodyEndBit > sourceEndBit
        || currentDwgBit(&probe) > bodyEndBit)
        return fail();
    bool parsedIsEmpty = false;
    bool parsedUnknownBit = false;
    std::uint16_t parsedModelerVersion = 0;
    if (!readBoundedBit(probe, bodyEndBit, parsedIsEmpty)
        || !readBoundedBit(probe, bodyEndBit, parsedUnknownBit))
        return fail();
    const bool parsedHasModelerData = !parsedIsEmpty;
    if (parsedHasModelerData
        && !readBoundedBitShort(probe, bodyEndBit, parsedModelerVersion))
        return fail();
    if (!probe.isGood() || (stringStream != nullptr && !stringStream->isGood()))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    // R13-R2004 modeler bodies contain opaque ACIS/SAT data between this
    // header and the common handles. Do not interpret that payload: jump to
    // the bounded object-data end exactly as the surface/proxy readers do.
    dwgBuffer handleProbe = sourceBuf->forkIndependent();
    if (v <= DRW::AC1018) {
        if (!handleProbe.setPosition(bodyEndBit >> 3))
            return fail();
        handleProbe.setBitPos(static_cast<std::uint8_t>(bodyEndBit & 7u));
        if (!handleProbe.isGood())
            return fail();
    }
    ret = DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();
    std::uint32_t parsedHistoryHandle = 0;
    if (eType == DRW::E3DSOLID && v > DRW::AC1018
        && proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle historyH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  historyH))
            return fail();
        parsedHistoryHandle = historyH.ref;
        DRW_DBG(" 3DSOLID history Handle: ");
        DRW_DBGHL(historyH.code, historyH.size, historyH.ref); DRW_DBG("\n");
    }

    if (handleProbe.size() > std::numeric_limits<std::uint32_t>::max())
        return fail();
    m_bodyBitSize = bs;
    m_objectSize = static_cast<std::uint32_t>(probe.size());
    m_isEmpty = parsedIsEmpty;
    m_hasModelerData = parsedHasModelerData;
    m_modelerDataUnknownBit = parsedUnknownBit;
    m_modelerVersion = parsedModelerVersion;
    m_historyHandle = parsedHistoryHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_ModelerGeometry::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 1:
    case 3:
        if (!appendTextBytesChecked(m_rawBytes, reader->getString(),
                                    dwgSafety::MaxBufferSize))
            return false;
        break;
    case 70: {
        const int value = reader->getInt32();
        if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
            return false;
        m_modelerVersion = static_cast<std::uint16_t>(value);
        break;
    }
    case 350:
    case 360:
        m_historyHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 310:
        {
            std::vector<std::uint8_t> decoded;
            if (!decodeHexBytes(reader->getString(), decoded))
                return false;
            if (!appendBytesChecked(m_rawBytes, decoded,
                                    dwgSafety::MaxBufferSize))
                return false;
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_Mesh::resetDwgState() {
    DRW_Entity::reset();
    version = 2;
    blendCrease = false;
    subdivisionLevel = 0;
    subdivVertices.clear();
    vertices.clear();
    faces.clear();
    edges.clear();
    creases.clear();
    unknown = 0;
    propertyOverrides.clear();
    m_dxfMeshSubclassSeen = false;
    m_dxfState = 0;
    m_dxfVertexCount = -1;
    m_dxfFaceItemsRemaining = -1;
    m_dxfEdgeValuesRemaining = -1;
    m_dxfCreaseValuesRemaining = -1;
    m_dxfPending = 0;
    m_dxfEdgeFrom = -1;
    m_dxfOverrideEntityCount = -1;
    m_dxfOverridePropertyCount = -1;
}

// DRW_Mesh::parseDwg — AcDbSubDMesh, field order shared by ACadSharp and
// dwgTs. DWG group 91 is the subdivision level, not a refined-vertex count;
// the following BL is the level-0 vertex count.
bool DRW_Mesh::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = v > DRW::AC1018 ? &stringProbe : nullptr;
    if (!DRW_Entity::parseDwg(v, &bodyProbe, stringStream, bs))
        return fail();
    buf = &bodyProbe;
    DRW_DBG("\n***************************** parsing MESH (AcDbSubDMesh) *****************\n");
    std::uint64_t bodyEndBit = 0;
    if (!entityBodyDataEndBit(
            stringStream != nullptr ? *stringStream : bodyProbe,
            v, objSize, bodyEndBit))
        return fail();

    auto readCount = [&](std::uint32_t minimumBitsPerItem,
                         std::uint32_t& count) {
        return readTableBodyCount(
            v, buf, static_cast<std::uint32_t>(bodyEndBit),
            DRW_Mesh::kMaxMeshItems, minimumBitsPerItem, count);
    };

    std::uint16_t parsedVersion = 0;
    bool parsedBlendCrease = false;
    std::int32_t parsedSubdivisionLevel = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedVersion)
        || !readBoundedBit(*buf, bodyEndBit, parsedBlendCrease)
        || !readBoundedBitLong(*buf, bodyEndBit, parsedSubdivisionLevel)
        || parsedSubdivisionLevel < 0) {
        return fail();
    }

    std::uint32_t nVert = 0; // BL vertex count (92)
    if (!readCount(6, nVert)) return fail(); // minimum 3BD width
    std::vector<DRW_Coord> parsedVertices;
    if (!DRW::reserve(parsedVertices, static_cast<int>(nVert)))
        return fail();
    std::vector<std::vector<std::int32_t>> parsedFaces;
    std::vector<std::pair<std::int32_t, std::int32_t>> parsedEdges;
    std::vector<double> parsedCreases;
    for (std::uint32_t i = 0; i < nVert; ++i) {
        DRW_Coord point;
        if (!readBoundedBitCoord(*buf, bodyEndBit, point))
            return fail();
        parsedVertices.push_back(point);
    }
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!buf->isGood()
        || (stringStream != nullptr && !stringStream->isGood())
        || !std::all_of(parsedVertices.begin(), parsedVertices.end(), finite))
        return fail();

    // faces (93) is a FLAT BL stream of length num_faces; each face is
    // [count, idx0, idx1, ...]. num_faces is the stream length, not the polygon
    // count — group on the fly.
    std::uint32_t faceItems = 0;                      // BL face-list size (93)
    if (!readCount(2, faceItems)) return fail();
    std::int32_t remaining = static_cast<std::int32_t>(faceItems);
    while (remaining > 0 && buf->isGood()) {
        std::int32_t cnt = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, cnt))
            return fail();
        --remaining;
        if (cnt < 0 || cnt > remaining)
            return fail();
        std::vector<std::int32_t> face;
        if (!DRW::reserve(face, cnt))
            return fail();
        for (std::int32_t j = 0; j < cnt; ++j) {
            std::int32_t index = 0;
            if (!readBoundedBitLong(*buf, bodyEndBit, index))
                return fail();
            if (index < 0 || static_cast<std::uint32_t>(index) >= nVert)
                return fail();
            face.push_back(index);
            --remaining;
        }
        if (!buf->isGood()) return fail();
        parsedFaces.push_back(std::move(face));
    }
    if (remaining != 0 || !buf->isGood()) return fail();

    std::uint32_t nEdges = 0;                         // BL edge count (94)
    if (!readCount(4, nEdges)) return fail(); // two minimum-width BL values
    if (!DRW::reserve(parsedEdges, static_cast<int>(nEdges)))
        return fail();
    for (std::uint32_t i = 0; i < nEdges; ++i) {
        std::int32_t from = 0;
        std::int32_t to = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, from)
            || !readBoundedBitLong(*buf, bodyEndBit, to))
            return fail();
        if (from < 0 || to < 0
            || static_cast<std::uint32_t>(from) >= nVert
            || static_cast<std::uint32_t>(to) >= nVert)
            return fail();
        parsedEdges.emplace_back(from, to);
    }
    if (!buf->isGood()) return fail();

    std::uint32_t nCrease = 0;                        // BL crease count (95)
    if (!readCount(2, nCrease)) return fail(); // minimum-width BD value
    if (!DRW::reserve(parsedCreases, static_cast<int>(nCrease)))
        return fail();
    for (std::uint32_t i = 0; i < nCrease; ++i) {
        double crease = 0.0;
        if (!readBoundedBitDouble(*buf, bodyEndBit, crease))
            return fail();
        parsedCreases.push_back(crease);
    }
    if (!buf->isGood()
        || !std::all_of(parsedCreases.begin(), parsedCreases.end(),
                        [](double value) { return std::isfinite(value); }))
        return fail();

    std::int32_t parsedUnknown = 0;
    if (!readBoundedBitLong(*buf, bodyEndBit, parsedUnknown))
        return fail(); // trailing unknown (BL)

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();
    vertices = std::move(parsedVertices);
    faces = std::move(parsedFaces);
    edges = std::move(parsedEdges);
    creases = std::move(parsedCreases);
    version = parsedVersion;
    blendCrease = parsedBlendCrease;
    subdivisionLevel = parsedSubdivisionLevel;
    unknown = parsedUnknown;
    *sourceBuf = handleProbe;
    return true;
}

// DRW_Mesh::parseCode — DXF read (codes 71/72/91/92/10·20·30/93/90/94/95/140).
// The 90 stream is shared by faces (after 93) and edges (after 94); m_dxfState
// sequences which one is being filled (mirrors DRW_Image::parseCode's stateful
// 91/14/24 WIPEOUT-vertex accumulation). After the crease stream, group 90
// starts the bounded property-override records described by AcDbSubDMesh.
bool DRW_Mesh::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    try {
    switch (code) {
    case 71: {
        const std::int32_t value = reader->getInt32();
        if (value < 0 || value > std::numeric_limits<std::int16_t>::max())
            return false;
        version = static_cast<std::uint16_t>(value);
        return true;
    }
    case 72: {
        const std::int32_t value = reader->getInt32();
        if (value != 0 && value != 1)
            return false;
        blendCrease = value != 0;
        return true;
    }
    case 91: {
        const std::int32_t value = reader->getInt32();
        if (m_dxfOverrideEntityCount >= 0) {
            if (m_dxfState != 96
                || (!propertyOverrides.empty() && m_dxfOverridePropertyCount != 0)
                || propertyOverrides.size() >= static_cast<std::size_t>(m_dxfOverrideEntityCount))
                return false;
            propertyOverrides.push_back({value, {}});
            m_dxfOverridePropertyCount = -1;
            return true;
        }
        if (value < 0)
            return false;
        subdivisionLevel = value;
        return true;
    }
    case 100:
        if (reader->getString() == "AcDbSubDMesh")
            m_dxfMeshSubclassSeen = true;
        return true;
    case 92: { // base-vertex count; later 92 values belong to overrides
        if (!m_dxfMeshSubclassSeen)
            return DRW_Entity::parseCode(code, reader);
        if (m_dxfVertexCount >= 0) {
            if (m_dxfOverrideEntityCount < 0 || m_dxfState != 96
                || propertyOverrides.empty() || m_dxfOverridePropertyCount >= 0)
                return false;
            const std::int32_t count = reader->getInt32();
            if (!isValidCount(count, 4))
                return false;
            if (!DRW::reserve(propertyOverrides.back().propertyTypes, count))
                return false;
            m_dxfOverridePropertyCount = count;
            return true;
        }
        const std::int32_t count = reader->getInt32();
        if (!isValidCount(count, DRW_Mesh::kMaxMeshItems))
            return false;
        if (!DRW::reserve(vertices, count))
            return false;
        m_dxfVertexCount = count;
        m_dxfState = 92;
        return true;
    }
    case 10:
        if (m_dxfState != 92 || m_dxfVertexCount < 0
            || vertices.size() >= static_cast<std::size_t>(m_dxfVertexCount))
            return false;
        {
            const double value = reader->getDouble();
            if (!std::isfinite(value))
                return false;
            vertices.emplace_back();
            vertices.back().x = value;
        }
        return true;
    case 20:
        if (m_dxfState != 92 || vertices.empty())
            return false;
        {
            const double value = reader->getDouble();
            if (!std::isfinite(value))
                return false;
            vertices.back().y = value;
        }
        return true;
    case 30:
        if (m_dxfState != 92 || vertices.empty())
            return false;
        {
            const double value = reader->getDouble();
            if (!std::isfinite(value))
                return false;
            vertices.back().z = value;
        }
        return true;
    case 93: { // face-list item count
        if (m_dxfVertexCount < 0
            || vertices.size() != static_cast<std::size_t>(m_dxfVertexCount))
            return false;
        const std::int32_t count = reader->getInt32();
        if (!isValidCount(count, DRW_Mesh::kMaxMeshItems))
            return false;
        m_dxfState = 93;
        m_dxfFaceItemsRemaining = count;
        m_dxfPending = 0;
        return true;
    }
    case 94: { // edge count; each edge consumes two code-90 values
        if (m_dxfFaceItemsRemaining != 0 || m_dxfPending != 0)
            return false;
        const std::int32_t count = reader->getInt32();
        if (!isValidCount(count, DRW_Mesh::kMaxMeshItems))
            return false;
        m_dxfState = 94;
        m_dxfEdgeValuesRemaining = count * 2;
        m_dxfEdgeFrom = -1;
        if (!DRW::reserve(edges, count))
            return false;
        return true;
    }
    case 90: {
        const std::int32_t val = reader->getInt32();
        if (m_dxfState == 95) {
            if (m_dxfCreaseValuesRemaining != 0)
                return false;
            if (m_dxfOverrideEntityCount >= 0)
                return false;
            if (!isValidCount(val, DRW_Mesh::kMaxMeshItems))
                return false;
            m_dxfOverrideEntityCount = val;
            m_dxfOverridePropertyCount = val == 0 ? 0 : -1;
            m_dxfState = 96;
        } else if (m_dxfState == 96) {
            if (m_dxfOverrideEntityCount < 0 || propertyOverrides.empty()
                || m_dxfOverridePropertyCount <= 0 || val < 0 || val > 3)
                return false;
            propertyOverrides.back().propertyTypes.push_back(val);
            --m_dxfOverridePropertyCount;
        } else if (m_dxfState == 93) {
            if (m_dxfFaceItemsRemaining <= 0)
                return false;
            if (m_dxfPending == 0) {
                if (val < 3 || val > m_dxfFaceItemsRemaining - 1)
                    return false;
                faces.emplace_back();
                m_dxfPending = val;
                --m_dxfFaceItemsRemaining;
            } else {
                if (val < 0 || val >= m_dxfVertexCount)
                    return false;
                if (!faces.empty())
                    faces.back().push_back(val);
                --m_dxfFaceItemsRemaining;
                --m_dxfPending;
            }
        } else if (m_dxfState == 94) {
            if (m_dxfEdgeValuesRemaining <= 0 || val < 0 || val >= m_dxfVertexCount)
                return false;
            --m_dxfEdgeValuesRemaining;
            if (m_dxfEdgeFrom < 0)
                m_dxfEdgeFrom = val;
            else {
                edges.emplace_back(m_dxfEdgeFrom, val);
                m_dxfEdgeFrom = -1;
            }
        } else {
            return false;
        }
        return true;
    }
    case 95: { // edge crease count
        if (m_dxfEdgeValuesRemaining != 0 || m_dxfEdgeFrom != -1)
            return false;
        const std::int32_t count = reader->getInt32();
        if (!isValidCount(count, DRW_Mesh::kMaxMeshItems))
            return false;
        m_dxfState = 95;
        m_dxfCreaseValuesRemaining = count;
        if (!DRW::reserve(creases, count))
            return false;
        return true;
    }
    case 140:
        if (m_dxfState != 95 || m_dxfCreaseValuesRemaining <= 0)
            return false;
        {
            const double value = reader->getDouble();
            if (!std::isfinite(value))
                return false;
            creases.push_back(value);
        }
        --m_dxfCreaseValuesRemaining;
        return true;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    } catch (...) {
        return false;
    }
}

bool DRW_Mesh::validateGeometry() const {
    if (subdivisionLevel < 0
        || vertices.size() > static_cast<std::size_t>(kMaxMeshItems)
        || edges.size() > static_cast<std::size_t>(kMaxMeshItems)
        || faces.size() > static_cast<std::size_t>(kMaxMeshItems)
        || creases.size() > static_cast<std::size_t>(kMaxMeshItems))
        return false;

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!std::all_of(vertices.cbegin(), vertices.cend(), finite)
        || !std::all_of(creases.cbegin(), creases.cend(),
                        [](double value) { return std::isfinite(value); }))
        return false;
    std::int32_t faceStreamCount = 0;
    for (const auto& face : faces) {
        if (face.size() < 3
            || face.size() > static_cast<std::size_t>(kMaxMeshItems - 1)
            || faceStreamCount > kMaxMeshItems
                - static_cast<std::int32_t>(face.size() + 1))
            return false;
        for (std::int32_t index : face) {
            if (index < 0
                || static_cast<std::size_t>(index) >= vertices.size())
                return false;
        }
        faceStreamCount += static_cast<std::int32_t>(face.size() + 1);
    }
    for (const auto& edge : edges) {
        if (edge.first < 0 || edge.second < 0
            || static_cast<std::size_t>(edge.first) >= vertices.size()
            || static_cast<std::size_t>(edge.second) >= vertices.size())
            return false;
    }
    return true;
}

bool DRW_Mesh::validateDxf() const {
    if (!m_dxfMeshSubclassSeen
        || m_dxfVertexCount < 0
        || vertices.size() != static_cast<std::size_t>(m_dxfVertexCount)
        || m_dxfFaceItemsRemaining != 0
        || m_dxfPending != 0
        || m_dxfEdgeValuesRemaining != 0
        || m_dxfEdgeFrom != -1
        || m_dxfCreaseValuesRemaining != 0
        || (m_dxfOverrideEntityCount >= 0
            && (m_dxfOverridePropertyCount < 0
                || m_dxfOverridePropertyCount != 0
                || propertyOverrides.size() != static_cast<std::size_t>(m_dxfOverrideEntityCount))))
        return false;
    for (const auto& overrideData : propertyOverrides) {
        if (overrideData.subEntityMarker < 0
            || overrideData.propertyTypes.size() > 4u)
            return false;
        if (!std::all_of(overrideData.propertyTypes.cbegin(),
                         overrideData.propertyTypes.cend(),
                         [](std::int32_t type) { return type >= 0 && type <= 3; }))
            return false;
    }
    return validateGeometry();
}

bool DRW_Mesh::validateDxfOutput() const {
    if (version > static_cast<std::uint16_t>(std::numeric_limits<std::int16_t>::max())
        || propertyOverrides.size() > static_cast<std::size_t>(kMaxMeshItems))
        return false;
    for (const auto& overrideData : propertyOverrides) {
        if (overrideData.subEntityMarker < 0
            || overrideData.propertyTypes.size() > 4u
            || !std::all_of(overrideData.propertyTypes.cbegin(),
                            overrideData.propertyTypes.cend(),
                            [](std::int32_t type) { return type >= 0 && type <= 3; }))
            return false;
    }
    return validateGeometry();
}

bool DRW_Mesh::encodeDwg(DRW::Version dwgVersion, dwgBufferW *buf, std::uint32_t bs,
                         dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (!validateGeometry())
        return false;

    std::int32_t faceStreamCount = 0;
    for (const auto& face : faces)
        faceStreamCount += static_cast<std::int32_t>(face.size() + 1);

    oType = kDwgClassNum;
    if (!encodeDwgCommon(dwgVersion, buf)) return false;

    buf->putBitShort(version);
    buf->putBit(blendCrease ? 1 : 0);

    buf->putBitLong(subdivisionLevel);

    buf->putBitLong(static_cast<std::int32_t>(vertices.size()));
    for (const DRW_Coord& vertex : vertices)
        buf->put3BitDouble(vertex);

    buf->putBitLong(faceStreamCount);
    for (const auto& face : faces) {
        buf->putBitLong(static_cast<std::int32_t>(face.size()));
        for (std::int32_t index : face)
            buf->putBitLong(index);
    }

    buf->putBitLong(static_cast<std::int32_t>(edges.size()));
    for (const auto& edge : edges) {
        buf->putBitLong(edge.first);
        buf->putBitLong(edge.second);
    }

    buf->putBitLong(static_cast<std::int32_t>(creases.size()));
    for (double crease : creases)
        buf->putBitDouble(crease);

    buf->putBitLong(unknown);

    return encodeDwgEntHandle(dwgVersion, buf, handleBuf);
}

bool DRW_Shape::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 2:
        m_styleName = reader->getUtf8String();
        break;
    case 10:
        m_insertionPoint.x = reader->getDouble();
        break;
    case 20:
        m_insertionPoint.y = reader->getDouble();
        break;
    case 30:
        m_insertionPoint.z = reader->getDouble();
        break;
    case 39:
        m_thickness = reader->getDouble();
        break;
    case 40:
        m_scale = reader->getDouble();
        break;
    case 41:
        m_widthFactor = reader->getDouble();
        break;
    case 50:
        m_rotation = reader->getDouble() / ARAD;
        break;
    case 51:
        m_oblique = reader->getDouble() / ARAD;
        break;
    case 210:
        m_extrusion.x = reader->getDouble();
        break;
    case 220:
        m_extrusion.y = reader->getDouble();
        break;
    case 230:
        m_extrusion.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_Shape::resetDwgState() {
    DRW_Entity::reset();
    m_insertionPoint = DRW_Coord{0.0, 0.0, 0.0};
    m_scale = 1.0;
    m_rotation = 0.0;
    m_widthFactor = 1.0;
    m_oblique = 0.0;
    m_thickness = 0.0;
    m_shapeIndex = 0;
    m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
    m_shapeFileHandle = 0;
    m_objectSize = 0;
    m_bodyBitSize = 0;
    m_rawBytes.clear();
}

bool DRW_Shape::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    m_bodyBitSize = bs;
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(v, &bodyProbe, bs))
        return fail();
    DRW_DBG("\n***************************** parsing SHAPE *****************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord insertionPoint;
    DRW_Coord extrusion;
    double scale = 0.0;
    double rotation = 0.0;
    double widthFactor = 0.0;
    double oblique = 0.0;
    double thickness = 0.0;
    std::uint16_t shapeIndex = 0;
    if (!readBounded3BitDouble(bodyProbe, bodyEndBit, insertionPoint)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, scale)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, rotation)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, widthFactor)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, oblique)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, thickness)
        || !readBoundedBitShort(bodyProbe, bodyEndBit, shapeIndex)
        || !readBounded3BitDouble(bodyProbe, bodyEndBit, extrusion))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!bodyProbe.isGood() || !finite(insertionPoint) || !finite(extrusion)
        || !std::isfinite(scale) || !std::isfinite(rotation)
        || !std::isfinite(widthFactor) || !std::isfinite(oblique)
        || !std::isfinite(thickness))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();

    // DwgObjectFrame removes the CRC trailer before exposing the body. The
    // SHAPEFILE hard pointer is therefore mandatory and must be parsed even
    // when only its exact handle bytes remain.
    dwgHandle shapeFileH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              shapeFileH)
        || shapeFileH.ref == 0)
        return fail();

    m_insertionPoint = insertionPoint;
    m_scale = scale;
    m_rotation = rotation;
    m_widthFactor = widthFactor;
    m_oblique = oblique;
    m_thickness = thickness;
    m_shapeIndex = shapeIndex;
    m_extrusion = extrusion;
    m_shapeFileHandle = shapeFileH.ref;
    DRW_DBG(" SHAPEFILE Handle: ");
    DRW_DBGHL(shapeFileH.code, shapeFileH.size, shapeFileH.ref);
    DRW_DBG("\n");
    *sourceBuf = handleProbe;
    return true;
}

// Phase 6.1: SHAPE encoder (fixed oType 33). Exact inverse of parseDwg above.
// Without this override a SHAPE would encode as a LINE (default DRW_Entity).
bool DRW_Shape::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                          dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
               && std::isfinite(point.z);
    };
    if (!finite(m_insertionPoint) || !finite(m_extrusion)
        || !std::isfinite(m_scale) || !std::isfinite(m_rotation)
        || !std::isfinite(m_widthFactor) || !std::isfinite(m_oblique)
        || !std::isfinite(m_thickness) || m_shapeFileHandle == 0) {
        return false;
    }
    oType = 33;  // SHAPE class id — see dwgreader.cpp case 33
    if (!encodeDwgCommon(version, buf)) return false;

    buf->put3BitDouble(m_insertionPoint);
    buf->putBitDouble(m_scale);
    buf->putBitDouble(m_rotation);
    buf->putBitDouble(m_widthFactor);
    buf->putBitDouble(m_oblique);
    buf->putBitDouble(m_thickness);
    buf->putBitShort(m_shapeIndex);
    buf->put3BitDouble(m_extrusion);

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // Trailing SHAPEFILE style hard pointer (code 5), byte-count-sized.
    dwgHandle sH;
    sH.code = 5;
    sH.ref  = m_shapeFileHandle;
    sH.size = 0;
    if (m_shapeFileHandle != 0) {
        std::uint32_t t = m_shapeFileHandle;
        while (t != 0) { t >>= 8; ++sH.size; }
    } else {
        sH.code = 0;  // null handle
    }
    (handleBuf ? handleBuf : buf)->putHandle(sH);
    return true;
}

bool DRW_Ole2Frame::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    const auto readUnsigned = [&](std::uint32_t maximum,
                                  std::uint32_t& target) {
        const int value = reader->getInt32();
        if (value < 0 || static_cast<std::uint32_t>(value) > maximum)
            return false;
        target = static_cast<std::uint32_t>(value);
        return true;
    };

    switch (code) {
    case 3:
        m_oleClient = reader->getUtf8String();
        break;
    case 10:
        m_pt1.x = reader->getDouble();
        break;
    case 20:
        m_pt1.y = reader->getDouble();
        break;
    case 30:
        m_pt1.z = reader->getDouble();
        break;
    case 11:
        m_pt2.x = reader->getDouble();
        break;
    case 21:
        m_pt2.y = reader->getDouble();
        break;
    case 31:
        m_pt2.z = reader->getDouble();
        break;
    case 70: {
        std::uint32_t value = 0;
        if (!readUnsigned(std::numeric_limits<std::uint16_t>::max(), value))
            return false;
        m_oleVersion = static_cast<std::uint16_t>(value);
        break;
    }
    case 71: {
        std::uint32_t value = 0;
        if (!readUnsigned(std::numeric_limits<std::uint16_t>::max(), value))
            return false;
        m_flags = static_cast<std::uint16_t>(value);
        break;
    }
    case 72: {
        std::uint32_t value = 0;
        if (!readUnsigned(std::numeric_limits<std::uint16_t>::max(), value))
            return false;
        m_mode = static_cast<std::uint16_t>(value);
        break;
    }
    case 73: {
        std::uint32_t value = 0;
        if (!readUnsigned(std::numeric_limits<std::uint8_t>::max(), value))
            return false;
        m_lockAspect = static_cast<std::uint8_t>(value);
        break;
    }
    case 90: {
        std::uint32_t value = 0;
        if (!readUnsigned(kMaxOlePayloadBytes, value))
            return false;
        m_declaredPayloadLength = value;
        m_dxfPayloadLengthSpecified = true;
        m_payloadPresent = value != 0;
        break;
    }
    case 310: {
        std::vector<std::uint8_t> bytes;
        if (!decodeHexBytes(reader->getString(), bytes))
            return false;
        if (!appendBytesChecked(m_payloadBytes, bytes,
                                kMaxOlePayloadBytes)) {
            m_payloadTooLarge = m_payloadBytes.size() > kMaxOlePayloadBytes
                || bytes.size() > kMaxOlePayloadBytes -
                                      std::min(m_payloadBytes.size(),
                                               static_cast<std::size_t>(
                                                   kMaxOlePayloadBytes));
            return false;
        }
        m_payloadPresent = true;
        break;
    }
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_Ole2Frame::resetDwgState() {
    DRW_Entity::reset();
    m_flags = 0;
    m_mode = 0;
    m_declaredPayloadLength = 0;
    m_payloadByteCount = 0;
    m_payloadStartBit = 0;
    m_payloadPresent = false;
    m_payloadTruncated = false;
    m_payloadTooLarge = false;
    m_hasR2000TrailingByte = false;
    m_r2000TrailingByte = 0;
    m_objectSize = 0;
    m_bodyBitSize = 0;
    m_payloadBytes.clear();
    m_pt1 = DRW_Coord{0.0, 0.0, 0.0};
    m_pt2 = DRW_Coord{0.0, 0.0, 0.0};
    m_lockAspect = 3;
    m_dxfPayloadLengthSpecified = false;
    m_oleVersion = 2;
    m_oleClient = "OLE";
    m_rawBytes.clear();
}

bool DRW_Ole2Frame::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    m_bodyBitSize = bs;
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(v, &bodyProbe, bs))
        return fail();
    DRW_DBG("\n***************************** parsing OLE2FRAME ************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint16_t parsedFlags = 0;
    std::uint16_t parsedMode = 0;
    std::int32_t declaredLength = 0;
    if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedFlags)
        || (v > DRW::AC1014
            && !readBoundedBitShort(bodyProbe, bodyEndBit, parsedMode))
        || !readBoundedBitLong(bodyProbe, bodyEndBit, declaredLength)
        || declaredLength < 0
        || static_cast<std::uint32_t>(declaredLength) > kMaxOlePayloadBytes)
        return fail();

    const std::uint64_t payloadStartBit = currentDwgBit(&bodyProbe);
    const auto bodyRemainingBits = [&](const dwgBuffer& cursor,
                                       std::uint64_t& remaining) {
        const std::uint64_t currentBit = currentDwgBit(&cursor);
        if (v > DRW::AC1014 && objSize != 0) {
            if (currentBit > bodyEndBit)
                return false;
            remaining = bodyEndBit - currentBit;
            return true;
        }
        const int bytes = cursor.numRemainingBytes();
        if (bytes < 0)
            return false;
        remaining = static_cast<std::uint64_t>(bytes) * 8u;
        return true;
    };

    std::uint64_t remainingBits = 0;
    if (!bodyRemainingBits(bodyProbe, remainingBits))
        return fail();
    const std::uint64_t trailingBits = v > DRW::AC1014 ? 8u : 0u;
    if (remainingBits < trailingBits
        || static_cast<std::uint64_t>(declaredLength)
               > (remainingBits - trailingBits) / 8u)
        return fail();

    std::vector<std::uint8_t> payload;
    if (!DRW::resize(payload, declaredLength))
        return fail();
    if (!readBoundedBytes(bodyProbe, bodyEndBit, payload.data(),
                          payload.size()))
        return fail();

    std::uint8_t trailingByte = 0;
    bool hasTrailingByte = false;
    if (v > DRW::AC1014) {
        if (!bodyRemainingBits(bodyProbe, remainingBits)
            || (remainingBits != 0 && remainingBits < 8))
            return fail();
        if (remainingBits >= 8) {
            if (!readBoundedRawChar8(bodyProbe, bodyEndBit, trailingByte))
                return fail();
            hasTrailingByte = true;
        }
    }

    // Decode the frame rectangle (DXF 10/11) from the OLE header. AutoCAD/ODA do
    // NOT store pt1/pt2 as DWG fields; they live in the first ~0x80 bytes of the
    // payload as raw little-endian doubles. Guarded so a non-finite/short
    // payload simply leaves both points at the origin.
    DRW_Coord point1;
    DRW_Coord point2;
    if (payload.size() >= 0x4a && payload[0] == 0x80) {
        auto rd = [&](std::size_t off) {
            double value = 0.0;
            std::memcpy(&value, payload.data() + off, sizeof(double));
            return value;
        };
        const DRW_Coord upperLeft(rd(0x02), rd(0x0a), rd(0x12));
        const DRW_Coord lowerRight(rd(0x32), rd(0x3a), rd(0x42));
        if (std::isfinite(upperLeft.x) && std::isfinite(upperLeft.y)
            && std::isfinite(upperLeft.z) && std::isfinite(lowerRight.x)
            && std::isfinite(lowerRight.y) && std::isfinite(lowerRight.z)) {
            point1 = upperLeft;
            point2 = lowerRight;
        }
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();

    m_flags = parsedFlags;
    m_mode = parsedMode;
    m_declaredPayloadLength = static_cast<std::uint32_t>(declaredLength);
    m_payloadStartBit = payloadStartBit;
    m_payloadPresent = declaredLength > 0;
    m_payloadByteCount = static_cast<std::uint32_t>(declaredLength);
    m_payloadBytes = std::move(payload);
    m_hasR2000TrailingByte = hasTrailingByte;
    m_r2000TrailingByte = trailingByte;
    m_pt1 = point1;
    m_pt2 = point2;
    *sourceBuf = handleProbe;
    return true;
}

// Phase 6.2: OLE2FRAME encoder (fixed oType 74). Inverse of parseDwg, emitting
// the captured opaque payload byte-for-byte. Without this override an OLE2FRAME
// would encode as a LINE.
bool DRW_Ole2Frame::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                              dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (m_payloadTooLarge || m_payloadTruncated
        || m_payloadBytes.size() > kMaxOlePayloadBytes
        || (m_payloadPresent
            && m_declaredPayloadLength != m_payloadBytes.size())) {
        return false;
    }
    oType = 74;  // OLE2FRAME class id — see dwgreader.cpp case 74
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBitShort(m_flags);
    if (version > DRW::AC1014)
        buf->putBitShort(m_mode);
    // Emit the actual captured length so the reader's data_size matches the
    // bytes that follow (avoids a declared-vs-actual mismatch on re-read).
    const std::uint32_t payloadLen = static_cast<std::uint32_t>(m_payloadBytes.size());
    buf->putBitLong(static_cast<std::int32_t>(payloadLen));
    if (payloadLen > 0)
        buf->putBytes(m_payloadBytes.data(), m_payloadBytes.size());
    // R2000+ Unknown RC (ODA §20.4.88): emitted UNCONDITIONALLY for version >
    // AC1014. parseDwg reads it whenever bytes remain before the handle stream
    // (which is always — handle data always follows), so gating the write on
    // m_hasR2000TrailingByte desynced a directly-constructed OLE2FRAME (the
    // default false): the parser consumed the first handle byte as this RC and
    // shifted the entity handle stream. Default m_r2000TrailingByte is 0, so
    // constructed entities align and round-tripped ones keep the captured byte.
    if (version > DRW::AC1014)
        buf->putRawChar8(m_r2000TrailingByte);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_OleFrame::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 70: {
        const int value = reader->getInt32();
        if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
            return false;
        m_flags = static_cast<std::uint16_t>(value);
        break;
    }
    case 90: {
        const int value = reader->getInt32();
        if (value < 0
            || static_cast<std::uint32_t>(value) > kMaxOlePayloadBytes) {
            m_payloadTooLarge = value >= 0;
            return false;
        }
        m_declaredPayloadLength = static_cast<std::uint32_t>(value);
        m_dxfPayloadLengthSpecified = true;
        m_payloadPresent = value != 0;
        break;
    }
    case 310: {
        std::vector<std::uint8_t> bytes;
        if (!decodeHexBytes(reader->getString(), bytes))
            return false;
        if (!appendBytesChecked(m_payloadBytes, bytes,
                                kMaxOlePayloadBytes)) {
            m_payloadTooLarge = m_payloadBytes.size() > kMaxOlePayloadBytes
                || bytes.size() > kMaxOlePayloadBytes -
                                      std::min(m_payloadBytes.size(),
                                               static_cast<std::size_t>(
                                                   kMaxOlePayloadBytes));
            return false;
        }
        m_payloadPresent = true;
        break;
    }
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_OleFrame::resetDwgState() {
    DRW_Entity::reset();
    m_flags = 0;
    m_mode = 0;
    m_declaredPayloadLength = 0;
    m_payloadByteCount = 0;
    m_payloadPresent = false;
    m_payloadTruncated = false;
    m_payloadTooLarge = false;
    m_objectSize = 0;
    m_bodyBitSize = 0;
    m_payloadBytes.clear();
    m_dxfPayloadLengthSpecified = false;
    m_rawBytes.clear();
}

bool DRW_OleFrame::parseDwg(DRW::Version version, dwgBuffer *buf,
                            std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    m_bodyBitSize = bs;
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs))
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint16_t parsedFlags = 0;
    std::uint16_t parsedMode = 0;
    std::int32_t declaredLength = 0;
    if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedFlags)
        || (version > DRW::AC1014
            && !readBoundedBitShort(bodyProbe, bodyEndBit, parsedMode))
        || !readBoundedBitLong(bodyProbe, bodyEndBit, declaredLength)
        || declaredLength < 0
        || static_cast<std::uint32_t>(declaredLength) > kMaxOlePayloadBytes)
        return fail();

    std::uint64_t bodyRemainingBits = 0;
    const std::uint64_t currentBit = currentDwgBit(&bodyProbe);
    if (version > DRW::AC1014 && objSize != 0) {
        if (currentBit > bodyEndBit)
            return fail();
        bodyRemainingBits = bodyEndBit - currentBit;
    } else {
        const int remainingBytes = bodyProbe.numRemainingBytes();
        if (remainingBytes < 0)
            return fail();
        bodyRemainingBits = static_cast<std::uint64_t>(remainingBytes) * 8u;
    }

    if (static_cast<std::uint64_t>(declaredLength) > bodyRemainingBits / 8u)
        return fail();
    std::vector<std::uint8_t> payload;
    if (!DRW::resize(payload, declaredLength))
        return fail();
    if (!readBoundedBytes(bodyProbe, bodyEndBit, payload.data(),
                          payload.size()))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    m_flags = parsedFlags;
    m_mode = parsedMode;
    m_declaredPayloadLength = static_cast<std::uint32_t>(declaredLength);
    m_payloadPresent = declaredLength != 0;
    m_payloadByteCount = static_cast<std::uint32_t>(declaredLength);
    m_payloadBytes = std::move(payload);
    *sourceBuf = handleProbe;
    return true;

}

bool DRW_OleFrame::encodeDwg(DRW::Version version, dwgBufferW *buf,
                             std::uint32_t bs, dwgBufferW *strBuf,
                             dwgBufferW *handleBuf) {
    (void)bs;
    (void)strBuf;
    if (buf == nullptr || m_payloadTooLarge || m_payloadTruncated
        || m_payloadBytes.size() > kMaxOlePayloadBytes
        || (m_payloadPresent
            && m_declaredPayloadLength != m_payloadBytes.size())) {
        return false;
    }

    oType = 43;
    if (!encodeDwgCommon(version, buf))
        return false;
    buf->putBitShort(m_flags);
    if (version > DRW::AC1014)
        buf->putBitShort(m_mode);
    const std::uint32_t payloadLength =
        static_cast<std::uint32_t>(m_payloadBytes.size());
    buf->putBitLong(static_cast<std::int32_t>(payloadLength));
    if (payloadLength != 0)
        buf->putBytes(m_payloadBytes.data(), m_payloadBytes.size());
    return encodeDwgEntHandle(version, buf, handleBuf);
}

void DRW_Light::resetDwgState() {
    DRW_Entity::reset();
    m_classVersion = 0;
    m_name.clear();
    m_type = 0;
    m_status = false;
    m_color = 0;
    m_plotGlyph = false;
    m_intensity = 0.0;
    m_position = DRW_Coord{0.0, 0.0, 0.0};
    m_target = DRW_Coord{0.0, 0.0, 0.0};
    m_attenuationType = 0;
    m_useAttenuationLimits = false;
    m_attenuationStartLimit = 0.0;
    m_attenuationEndLimit = 0.0;
    m_hotspotAngle = 0.0;
    m_falloffAngle = 0.0;
    m_castShadows = false;
    m_shadowType = 0;
    m_shadowMapSize = 0;
    m_shadowMapSoftness = 0;
    m_hasPhotometricData = false;
    m_hasWebFile = false;
    m_webFile.clear();
    m_physicalIntensityMethod = 0;
    m_physicalIntensity = 0.0;
    m_illuminanceDistance = 0.0;
    m_lampColorType = 0;
    m_lampColorTemperature = 0.0;
    m_lampColorPreset = 0;
    m_webRotation = {1.0, 0.0, 0.0};
    m_extendedLightShape = 0;
    m_extendedLightLength = 0.0;
    m_extendedLightWidth = 0.0;
    m_extendedLightRadius = 0.0;
}

bool DRW_Light::parseDwg(DRW::Version v, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = v > DRW::AC1018 ? &stringProbe : nullptr;
    if (!DRW_Entity::parseDwg(v, &bodyProbe, stringStream, bs)
        || !bodyProbe.isGood() || !stringProbe.isGood())
        return fail();
    DRW_DBG("\n***************************** parsing LIGHT *****************************\n");

    const std::uint64_t bodyDataEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyDataEndBit;
    if (stringStream != nullptr && v > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*stringStream, v, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }

    std::int32_t parsedClassVersion = 0;
    UTF8STRING parsedName;
    std::int32_t parsedType = 0;
    bool parsedStatus = false;
    std::uint32_t parsedColor = 0;
    bool parsedPlotGlyph = false;
    double parsedIntensity = 0.0;
    DRW_Coord parsedPosition;
    DRW_Coord parsedTarget;
    std::int32_t parsedAttenuationType = 0;
    bool parsedUseAttenuationLimits = false;
    double parsedAttenuationStartLimit = 0.0;
    double parsedAttenuationEndLimit = 0.0;
    double parsedHotspotAngle = 0.0;
    double parsedFalloffAngle = 0.0;
    bool parsedCastShadows = false;
    std::int32_t parsedShadowType = 0;
    std::uint16_t parsedShadowMapSize = 0;
    std::uint8_t parsedShadowMapSoftness = 0;
    if (!readBoundedBitLong(bodyProbe, bodyDataEndBit, parsedClassVersion)
        || (stringStream != nullptr
            ? !readBoundedVariableText(stringProbe, stringEndBit, v,
                                       parsedName)
            : !readBoundedVariableText(bodyProbe, bodyDataEndBit, v,
                                       parsedName))
        || !readBoundedBitLong(bodyProbe, bodyDataEndBit, parsedType)
        || !readBoundedBit(bodyProbe, bodyDataEndBit, parsedStatus)
        || !readBoundedCmColor(bodyProbe, stringStream, bodyDataEndBit, v,
                               parsedColor, nullptr, nullptr, nullptr,
                               nullptr, stringEndBit)
        || !readBoundedBit(bodyProbe, bodyDataEndBit, parsedPlotGlyph)
        || !readBoundedBitDouble(bodyProbe, bodyDataEndBit, parsedIntensity)
        || !readBoundedBitCoord(bodyProbe, bodyDataEndBit, parsedPosition)
        || !readBoundedBitCoord(bodyProbe, bodyDataEndBit, parsedTarget)
        || !readBoundedBitLong(bodyProbe, bodyDataEndBit,
                               parsedAttenuationType)
        || !readBoundedBit(bodyProbe, bodyDataEndBit,
                           parsedUseAttenuationLimits)
        || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                 parsedAttenuationStartLimit)
        || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                 parsedAttenuationEndLimit)
        || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                 parsedHotspotAngle)
        || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                 parsedFalloffAngle)
        || !readBoundedBit(bodyProbe, bodyDataEndBit, parsedCastShadows)
        || !readBoundedBitLong(bodyProbe, bodyDataEndBit, parsedShadowType)
        || !readBoundedBitShort(bodyProbe, bodyDataEndBit,
                                parsedShadowMapSize)
        || !readBoundedRawChar8(bodyProbe, bodyDataEndBit,
                                parsedShadowMapSoftness))
        return fail();

    bool hasPhotometricData = false;
    bool hasWebFile = false;
    UTF8STRING webFile;
    std::uint16_t physicalIntensityMethod = 0;
    double physicalIntensity = 0.0;
    double illuminanceDistance = 0.0;
    std::uint16_t lampColorType = 0;
    double lampColorTemperature = 0.0;
    std::uint16_t lampColorPreset = 0;
    DRW_Coord webRotation{1.0, 0.0, 0.0};
    std::uint16_t extendedLightShape = 0;
    double extendedLightLength = 0.0;
    double extendedLightWidth = 0.0;
    double extendedLightRadius = 0.0;
    if (v > DRW::AC1018
        && proxyEntityHasBits(bodyProbe, bodyDataEndBit, 1)) {
        if (!readBoundedBit(bodyProbe, bodyDataEndBit, hasPhotometricData))
            return fail();
        if (hasPhotometricData) {
            if (!readBoundedBit(bodyProbe, bodyDataEndBit, hasWebFile)
                || !readBoundedVariableText(stringProbe, stringEndBit, v,
                                             webFile)
                || !readBoundedBitShort(bodyProbe, bodyDataEndBit,
                                        physicalIntensityMethod)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         physicalIntensity)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         illuminanceDistance)
                || !readBoundedBitShort(bodyProbe, bodyDataEndBit,
                                        lampColorType)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         lampColorTemperature)
                || !readBoundedBitShort(bodyProbe, bodyDataEndBit,
                                        lampColorPreset)
                || !readBoundedBitCoord(bodyProbe, bodyDataEndBit,
                                        webRotation)
                || !readBoundedBitShort(bodyProbe, bodyDataEndBit,
                                        extendedLightShape)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         extendedLightLength)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         extendedLightWidth)
                || !readBoundedBitDouble(bodyProbe, bodyDataEndBit,
                                         extendedLightRadius))
                return fail();
        }
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!bodyProbe.isGood() || !stringProbe.isGood()
        || currentDwgBit(&bodyProbe) > bodyDataEndBit
        || (stringStream != nullptr && currentDwgBit(&stringProbe) > stringEndBit)
        || parsedClassVersion < 0 || parsedType < 0
        || parsedAttenuationType < 0 || parsedShadowType < 0
        || !finite(parsedPosition) || !finite(parsedTarget)
        || !finite(webRotation)
        || !std::isfinite(parsedIntensity)
        || !std::isfinite(parsedAttenuationStartLimit)
        || !std::isfinite(parsedAttenuationEndLimit)
        || !std::isfinite(parsedHotspotAngle)
        || !std::isfinite(parsedFalloffAngle)
        || !std::isfinite(physicalIntensity)
        || !std::isfinite(illuminanceDistance)
        || !std::isfinite(lampColorTemperature)
        || !std::isfinite(extendedLightLength)
        || !std::isfinite(extendedLightWidth)
        || !std::isfinite(extendedLightRadius))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();

    m_classVersion = static_cast<std::uint32_t>(parsedClassVersion);
    m_name = std::move(parsedName);
    m_type = static_cast<std::uint32_t>(parsedType);
    m_status = parsedStatus;
    m_color = parsedColor;
    m_plotGlyph = parsedPlotGlyph;
    m_intensity = parsedIntensity;
    m_position = parsedPosition;
    m_target = parsedTarget;
    m_attenuationType = static_cast<std::uint32_t>(parsedAttenuationType);
    m_useAttenuationLimits = parsedUseAttenuationLimits;
    m_attenuationStartLimit = parsedAttenuationStartLimit;
    m_attenuationEndLimit = parsedAttenuationEndLimit;
    m_hotspotAngle = parsedHotspotAngle;
    m_falloffAngle = parsedFalloffAngle;
    m_castShadows = parsedCastShadows;
    m_shadowType = static_cast<std::uint32_t>(parsedShadowType);
    m_shadowMapSize = parsedShadowMapSize;
    m_shadowMapSoftness = parsedShadowMapSoftness;
    m_hasPhotometricData = hasPhotometricData;
    m_hasWebFile = hasWebFile;
    m_webFile = std::move(webFile);
    m_physicalIntensityMethod = physicalIntensityMethod;
    m_physicalIntensity = physicalIntensity;
    m_illuminanceDistance = illuminanceDistance;
    m_lampColorType = lampColorType;
    m_lampColorTemperature = lampColorTemperature;
    m_lampColorPreset = lampColorPreset;
    m_webRotation = webRotation;
    m_extendedLightShape = extendedLightShape;
    m_extendedLightLength = extendedLightLength;
    m_extendedLightWidth = extendedLightWidth;
    m_extendedLightRadius = extendedLightRadius;
    DRW_DBG("LIGHT name: "); DRW_DBG(m_name.c_str()); DRW_DBG("\n");
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Camera::parseCode(int code,
                           const std::unique_ptr<dxfReader>& reader) {
    if (code == 340) {
        m_viewHandle = reader->getHandleString();
        return true;
    }
    return DRW_Entity::parseCode(code, reader);
}

void DRW_Camera::resetDwgState() {
    DRW_Entity::reset();
    m_viewHandle = 0;
}

bool DRW_Camera::parseDwg(DRW::Version v, dwgBuffer *buf,
                          std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr || v < DRW::AC1015)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwg(
            v, &bodyProbe, v > DRW::AC1018 ? &stringProbe : nullptr, bs)
        || !bodyProbe.isGood() || !stringProbe.isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();

    std::uint32_t viewHandle = 0;
    if (proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle viewH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false, viewH))
            return fail();
        viewHandle = viewH.ref;
    } else if (currentDwgBit(&handleProbe) != handleEndBit) {
        return fail();
    }

    m_viewHandle = viewHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Camera::encodeDwg(DRW::Version v, dwgBufferW *buf,
                           std::uint32_t bs, dwgBufferW *strBuf,
                           dwgBufferW *handleBuf) {
    (void)bs;
    if (v < DRW::AC1015 || buf == nullptr)
        return false;

    oType = kDwgClassNum;
    if (!encodeDwgCommon(v, buf, strBuf)
        || !encodeDwgEntHandle(v, buf, handleBuf)) {
        return false;
    }

    putHardPointerHandle(handleBuf ? handleBuf : buf, m_viewHandle);
    return true;
}

bool DRW_GeoPositionMarker::parseCode(
    int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 90:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, std::numeric_limits<int>::max(), value))
                return false;
            m_classVersion = static_cast<std::uint32_t>(value);
        }
        return true;
    case 10:
        m_position.x = reader->getDouble();
        return true;
    case 20:
        m_position.y = reader->getDouble();
        return true;
    case 30:
        m_position.z = reader->getDouble();
        return true;
    case 40:
        if (m_dxfDouble40Count++ == 0)
            m_radius = reader->getDouble();
        else if (m_dxfDouble40Count == 2)
            m_landingGap = reader->getDouble();
        else
            (void)reader->getDouble();
        return true;
    case 1:
        m_notes = reader->getUtf8String();
        return true;
    case 280:
        {
            const int value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint8_t>::max())
                return false;
            m_textAlignment = static_cast<std::uint8_t>(value);
        }
        return true;
    case 290:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 1, value))
                return false;
            if (m_dxfBool290Count++ == 0)
                m_mtextVisible = value != 0;
            else if (m_dxfBool290Count == 2)
                m_enableFrameText = value != 0;
        }
        return true;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
}

bool DRW_GeoPositionMarker::parseDxfVariant(const DRW_Variant& value) {
    switch (value.code()) {
    case 90:
        if (value.type() != DRW_Variant::INTEGER)
            return false;
        if (value.i_val() < 0)
            return false;
        m_classVersion = static_cast<std::uint32_t>(value.i_val());
        return true;
    case 10:
        if (value.type() != DRW_Variant::DOUBLE)
            return false;
        m_position.x = value.d_val();
        return true;
    case 20:
        if (value.type() != DRW_Variant::DOUBLE)
            return false;
        m_position.y = value.d_val();
        return true;
    case 30:
        if (value.type() != DRW_Variant::DOUBLE)
            return false;
        m_position.z = value.d_val();
        return true;
    case 40:
        if (value.type() != DRW_Variant::DOUBLE)
            return false;
        if (m_dxfDouble40Count++ == 0)
            m_radius = value.d_val();
        else if (m_dxfDouble40Count == 2)
            m_landingGap = value.d_val();
        return true;
    case 1:
        if (value.type() != DRW_Variant::STRING)
            return false;
        m_notes = value.c_str();
        return true;
    case 280:
        if (value.type() != DRW_Variant::INTEGER)
            return false;
        if (value.i_val() < 0
            || value.i_val() > std::numeric_limits<std::uint8_t>::max())
            return false;
        m_textAlignment = static_cast<std::uint8_t>(value.i_val());
        return true;
    case 290:
        if (value.type() != DRW_Variant::INTEGER)
            return false;
        if (value.i_val() < 0 || value.i_val() > 1)
            return false;
        if (m_dxfBool290Count++ == 0)
            m_mtextVisible = value.i_val() != 0;
        else if (m_dxfBool290Count == 2)
            m_enableFrameText = value.i_val() != 0;
        return true;
    default:
        return true; // Common and unknown groups remain raw-authoritative.
    }
}

void DRW_GeoPositionMarker::resetDwgState() {
    DRW_Entity::reset();
    m_classVersion = 0;
    m_position = DRW_Coord{0.0, 0.0, 0.0};
    m_radius = 0.0;
    m_notes.clear();
    m_landingGap = 0.0;
    m_mtextVisible = false;
    m_textAlignment = 0;
    m_enableFrameText = false;
    mtext.reset();
    m_dxfDouble40Count = 0;
    m_dxfBool290Count = 0;
}

bool DRW_GeoPositionMarker::parseDwg(DRW::Version v, dwgBuffer *buf,
                                     std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr || v < DRW::AC1027)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwg(v, &bodyProbe, &stringProbe, bs)
        || !bodyProbe.isGood() || !stringProbe.isGood())
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (v > DRW::AC1018
        && (!bodyProbe.getR2007StringStreamBounds(objSize, stringStartBit,
                                                  stringEndBit)
            || currentDwgBit(&stringProbe) > stringEndBit))
        return fail();

    std::int32_t parsedClassVersion = 0;
    DRW_Coord position;
    double radius = 0.0;
    UTF8STRING notes;
    double landingGap = 0.0;
    bool mtextVisible = false;
    std::uint8_t textAlignment = 0;
    bool enableFrameText = false;
    if (!readBoundedBitLong(bodyProbe, bodyEndBit, parsedClassVersion)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, position)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, radius)
        || !readBoundedVariableText(stringProbe, stringEndBit, v, notes)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, landingGap)
        || !readBoundedBit(bodyProbe, bodyEndBit, mtextVisible)
        || !readBoundedRawChar8(bodyProbe, bodyEndBit, textAlignment)
        || !readBoundedBit(bodyProbe, bodyEndBit, enableFrameText))
        return fail();

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!bodyProbe.isGood() || !stringProbe.isGood()
        || !finite(position) || !std::isfinite(radius)
        || !std::isfinite(landingGap))
        return fail();

    // AcDbMTextObjectEmbedded follows the marker body when framing is enabled.
    // Parse it on a bounded probe before the parent and embedded handle passes;
    // otherwise a truncated nested body can consume parent handle bytes.
    dwgBuffer bodyTailProbe = bodyProbe.forkIndependent();
    std::unique_ptr<DRW_MText> parsedMText;
    EmbeddedMTextHandleInfo embeddedHandles;
    if (enableFrameText) {
        if (objSize == 0
            || !(parsedMText = std::make_unique<DRW_MText>())
            || !parseEmbeddedMTextDwg(v, &bodyTailProbe, &stringProbe,
                                      *parsedMText, embeddedHandles,
                                      bodyEndBit, stringEndBit)
            || !bodyTailProbe.isGood() || !stringProbe.isGood())
            return fail();
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();

    dwgBuffer handleProbe = bodyTailProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();
    if (enableFrameText) {
        if (!consumeEmbeddedMTextHandles(v, &handleProbe, embeddedHandles,
                                         parsedMText.get(), handleEndBit))
            return fail();
    }

    m_classVersion = static_cast<std::uint32_t>(parsedClassVersion);
    m_position = position;
    m_radius = radius;
    m_notes = notes;
    m_landingGap = landingGap;
    m_mtextVisible = mtextVisible;
    m_textAlignment = textAlignment;
    m_enableFrameText = enableFrameText;
    mtext = std::move(parsedMText);
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Light::encodeDwg(DRW::Version v, dwgBufferW *buf, std::uint32_t bs,
                          dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (v < DRW::AC1021)
        return false;

    oType = kDwgClassNum;
    if (!encodeDwgCommon(v, buf, strBuf))
        return false;

    dwgBufferW *sb = strBuf ? strBuf : buf;
    buf->putBitLong(m_classVersion);
    sb->putVariableText(v, m_name);
    buf->putBitLong(m_type);
    buf->putBit(m_status ? 1 : 0);
    buf->putCmColor(v, static_cast<std::uint16_t>(m_color));
    buf->putBit(m_plotGlyph ? 1 : 0);
    buf->putBitDouble(m_intensity);
    buf->put3BitDouble(m_position);
    buf->put3BitDouble(m_target);
    buf->putBitLong(m_attenuationType);
    buf->putBit(m_useAttenuationLimits ? 1 : 0);
    buf->putBitDouble(m_attenuationStartLimit);
    buf->putBitDouble(m_attenuationEndLimit);
    buf->putBitDouble(m_hotspotAngle);
    buf->putBitDouble(m_falloffAngle);
    buf->putBit(m_castShadows ? 1 : 0);
    buf->putBitLong(m_shadowType);
    buf->putBitShort(m_shadowMapSize);
    buf->putRawChar8(m_shadowMapSoftness);

    buf->putBit(m_hasPhotometricData ? 1 : 0);
    if (m_hasPhotometricData) {
        buf->putBit(m_hasWebFile ? 1 : 0);
        sb->putVariableText(v, m_webFile);
        buf->putBitShort(m_physicalIntensityMethod);
        buf->putBitDouble(m_physicalIntensity);
        buf->putBitDouble(m_illuminanceDistance);
        buf->putBitShort(m_lampColorType);
        buf->putBitDouble(m_lampColorTemperature);
        buf->putBitShort(m_lampColorPreset);
        buf->put3BitDouble(m_webRotation);
        buf->putBitShort(m_extendedLightShape);
        buf->putBitDouble(m_extendedLightLength);
        buf->putBitDouble(m_extendedLightWidth);
        buf->putBitDouble(m_extendedLightRadius);
    }

    return encodeDwgEntHandle(v, buf, handleBuf);
}

bool DRW_SectionObject::parseCode(
    int code, const std::unique_ptr<dxfReader>& reader) {
    if (code != 100 && !m_dxfInBody)
        return DRW_Entity::parseCode(code, reader);
    switch (code) {
    case 100:
        m_dxfInBody = reader->getUtf8String() == "AcDbSection";
        return true;
    case 90:
        m_state = static_cast<std::uint32_t>(reader->getInt32());
        return true;
    case 91:
        m_flags = static_cast<std::uint32_t>(reader->getInt32());
        return true;
    case 1:
        m_name = reader->getUtf8String();
        return true;
    case 10:
        m_vertDir.x = reader->getDouble();
        return true;
    case 20:
        m_vertDir.y = reader->getDouble();
        return true;
    case 30:
        m_vertDir.z = reader->getDouble();
        return true;
    case 40:
        m_topHeight = reader->getDouble();
        return true;
    case 41:
        m_bottomHeight = reader->getDouble();
        return true;
    case 70:
        {
            const int value = reader->getInt32();
            if (value < 0 || value > 0xFFFF)
                return false;
            m_indicatorAlpha = static_cast<std::uint16_t>(value);
        }
        return true;
    case 62:
        m_indicatorColor = static_cast<std::uint32_t>(reader->getInt32());
        return true;
    case 92:
        {
        const int value = reader->getInt32();
        if (value < 0
            || static_cast<std::size_t>(value) > DRW_SectionObject::kMaxVertices)
            return false;
        m_verts.clear();
        if (!DRW::reserve(m_verts, value))
            return false;
        return true;
        }
    case 93:
        {
        const int value = reader->getInt32();
        if (value < 0
            || static_cast<std::size_t>(value) > DRW_SectionObject::kMaxVertices)
            return false;
        m_blVerts.clear();
        if (!DRW::reserve(m_blVerts, value))
            return false;
        return true;
        }
    case 11:
        if (m_verts.size() >= DRW_SectionObject::kMaxVertices)
            return false;
        m_verts.emplace_back();
        m_verts.back().x = reader->getDouble();
        return true;
    case 21:
        if (m_verts.empty())
            return false;
        m_verts.back().y = reader->getDouble();
        return true;
    case 31:
        if (m_verts.empty())
            return false;
        m_verts.back().z = reader->getDouble();
        return true;
    case 12:
        if (m_blVerts.size() >= DRW_SectionObject::kMaxVertices)
            return false;
        m_blVerts.emplace_back();
        m_blVerts.back().x = reader->getDouble();
        return true;
    case 22:
        if (m_blVerts.empty())
            return false;
        m_blVerts.back().y = reader->getDouble();
        return true;
    case 32:
        if (m_blVerts.empty())
            return false;
        m_blVerts.back().z = reader->getDouble();
        return true;
    case 360:
        m_sectionSettingsHandle = reader->getHandleString();
        return true;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
}

// DRW_SectionObject::parseDwg — SECTIONOBJECT / AcDbSection, field order per
// libreDWG dwg2.spec DWG_ENTITY(SECTIONOBJECT): BL state, BL flags, T name,
// 3BD vert_dir, BD top/bottom height, BS indicator_alpha, CMTC indicator_color,
// BL num_verts + verts, BL num_blverts + blverts; then the common entity handle
// data followed by the section_settings hard reference (H 5, 360).
void DRW_SectionObject::resetDwgState() {
    DRW_Entity::reset();
    m_state = 0;
    m_flags = 0;
    m_name.clear();
    m_vertDir = DRW_Coord{0.0, 0.0, 0.0};
    m_topHeight = 0.0;
    m_bottomHeight = 0.0;
    m_indicatorAlpha = 0;
    m_indicatorColor = 0;
    m_verts.clear();
    m_blVerts.clear();
    m_sectionSettingsHandle = 0;
    m_dxfInBody = false;
}

bool DRW_SectionObject::parseDwg(DRW::Version v, dwgBuffer *buf,
                                 std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    // R2007+ stores text in a separate stream. Independent cursors keep a
    // malformed field from consuming the source or a later handle stream.
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = v > DRW::AC1018 ? &stringProbe : &bodyProbe;
    if (!DRW_Entity::parseDwg(v, &bodyProbe, stringStream, bs)
        || !bodyProbe.isGood() || !stringStream->isGood())
        return fail();
    DRW_DBG("\n***************************** parsing SECTIONOBJECT *********************\n");

    // The common entity parser leaves bodyProbe at the first SECTIONOBJECT
    // field and stringStream at the detached string stream for R2007+.
    // Keep both cursors inside their declared data ranges while staging the
    // complete header before publishing it.
    const std::uint64_t bodyEndBit = tableBodyEndBit(v, stringStream, objSize);
    const std::uint64_t stringEndBit = v > DRW::AC1018
        ? objSize : bodyEndBit;
    std::int32_t stateValue = 0;
    std::int32_t flagsValue = 0;
    UTF8STRING name;
    DRW_Coord vertDir;
    double topHeight = 0.0;
    double bottomHeight = 0.0;
    std::uint16_t indicatorAlpha = 0;
    std::uint32_t indicatorColor = 0;
    if (!readBoundedBitLong(bodyProbe, bodyEndBit, stateValue)
        || !readBoundedBitLong(bodyProbe, bodyEndBit, flagsValue)
        || !readBoundedVariableText(*stringStream, stringEndBit, v, name)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, vertDir)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, topHeight)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, bottomHeight)
        || !readBoundedBitShort(bodyProbe, bodyEndBit, indicatorAlpha)
        || !readBoundedCmColor(bodyProbe, stringStream, bodyEndBit, v,
                               indicatorColor, nullptr, nullptr, nullptr,
                               nullptr, stringEndBit))
        return fail();

    std::uint32_t numVerts = 0;
    if (!readTableBodyCount(
            v, &bodyProbe, bodyEndBit,
            static_cast<std::uint32_t>(DRW_SectionObject::kMaxVertices), 6,
            numVerts)
        || !proxyEntityHasBits(
            bodyProbe, bodyEndBit, static_cast<std::uint64_t>(numVerts) * 6u))
        return fail();
    std::vector<DRW_Coord> verts;
    if (!DRW::reserve(verts, static_cast<int>(numVerts)))
        return fail();
    for (std::uint32_t i = 0; i < numVerts; ++i) {
        DRW_Coord vertex;
        if (!readBoundedBitCoord(bodyProbe, bodyEndBit, vertex))
            return fail();
        verts.push_back(vertex);
    }

    std::uint32_t numBl = 0;
    if (!readTableBodyCount(
            v, &bodyProbe, bodyEndBit,
            static_cast<std::uint32_t>(DRW_SectionObject::kMaxVertices), 6,
            numBl)
        || !proxyEntityHasBits(
            bodyProbe, bodyEndBit, static_cast<std::uint64_t>(numBl) * 6u))
        return fail();
    std::vector<DRW_Coord> blVerts;
    if (!DRW::reserve(blVerts, static_cast<int>(numBl)))
        return fail();
    for (std::uint32_t i = 0; i < numBl; ++i) {
        DRW_Coord vertex;
        if (!readBoundedBitCoord(bodyProbe, bodyEndBit, vertex))
            return fail();
        blVerts.push_back(vertex);
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!bodyProbe.isGood() || !stringStream->isGood()
        || currentDwgBit(&bodyProbe) > bodyEndBit
        || !finite(vertDir) || !std::isfinite(topHeight)
        || !std::isfinite(bottomHeight))
        return fail();
    if (!std::all_of(verts.begin(), verts.end(), finite)
        || !std::all_of(blVerts.begin(), blVerts.end(), finite))
        return fail();

    // The common entity handle stream starts at the object-data boundary for
    // R2007+ and follows the body inline in older versions. The section
    // settings reference is a hard pointer (H 5), after those common handles.
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();
    std::uint32_t sectionSettingsHandle = 0;
    if (proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle settingsHandle;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                                  settingsHandle))
            return fail();
        sectionSettingsHandle = settingsHandle.ref;
        DRW_DBG(" section_settings Handle: ");
        DRW_DBGHL(settingsHandle.code, settingsHandle.size,
                  settingsHandle.ref); DRW_DBG("\n");
    }
    if (!handleProbe.isGood())
        return fail();

    m_state = static_cast<std::uint32_t>(stateValue);
    m_flags = static_cast<std::uint32_t>(flagsValue);
    m_name = name;
    m_vertDir = vertDir;
    m_topHeight = topHeight;
    m_bottomHeight = bottomHeight;
    m_indicatorAlpha = indicatorAlpha;
    m_indicatorColor = indicatorColor;
    m_verts = std::move(verts);
    m_blVerts = std::move(blVerts);
    m_sectionSettingsHandle = sectionSettingsHandle;
    *sourceBuf = handleProbe;
    DRW_DBG("SECTIONOBJECT name: "); DRW_DBG(m_name.c_str());
    DRW_DBG(" verts: "); DRW_DBG(static_cast<int>(m_verts.size())); DRW_DBG("\n");
    return true;
}

bool DRW_SectionObject::encodeDwg(DRW::Version v, dwgBufferW *buf,
                                  std::uint32_t bs, dwgBufferW *strBuf,
                                  dwgBufferW *handleBuf) {
    (void)bs;
    if (v < DRW::AC1021 || buf == nullptr
        || m_verts.size() > DRW_SectionObject::kMaxVertices
        || m_blVerts.size() > DRW_SectionObject::kMaxVertices)
        return false;
    oType = kDwgClassNum;
    if (!encodeDwgCommon(v, buf, strBuf))
        return false;
    dwgBufferW *stringBuffer = strBuf != nullptr ? strBuf : buf;
    buf->putBitLong(static_cast<std::int32_t>(m_state));
    buf->putBitLong(static_cast<std::int32_t>(m_flags));
    stringBuffer->putVariableText(v, m_name);
    buf->put3BitDouble(m_vertDir);
    buf->putBitDouble(m_topHeight);
    buf->putBitDouble(m_bottomHeight);
    buf->putBitShort(m_indicatorAlpha);
    buf->putCmColor(v, static_cast<std::uint16_t>(m_indicatorColor));
    buf->putBitLong(static_cast<std::int32_t>(m_verts.size()));
    for (const DRW_Coord& point : m_verts)
        buf->put3BitDouble(point);
    buf->putBitLong(static_cast<std::int32_t>(m_blVerts.size()));
    for (const DRW_Coord& point : m_blVerts)
        buf->put3BitDouble(point);
    if (!encodeDwgEntHandle(v, buf, handleBuf))
        return false;
    putHardPointerHandle(handleBuf != nullptr ? handleBuf : buf,
                         m_sectionSettingsHandle);
    return true;
}

bool DRW_Tolerance::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 3:
        dimStyleName = reader->getUtf8String();
        break;
    case 10:
        insertionPoint.x = reader->getDouble();
        break;
    case 20:
        insertionPoint.y = reader->getDouble();
        break;
    case 30:
        insertionPoint.z = reader->getDouble();
        break;
    case 11:
        xAxisDirectionVector.x = reader->getDouble();
        break;
    case 21:
        xAxisDirectionVector.y = reader->getDouble();
        break;
    case 31:
        xAxisDirectionVector.z = reader->getDouble();
        break;
    case 210:
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_Tolerance::resetDwgState() {
    DRW_Entity::reset();
    text.clear();
    dimStyleName = "STANDARD";
    dimStyleH = dwgHandle{};
    insertionPoint = DRW_Coord{0.0, 0.0, 0.0};
    xAxisDirectionVector = DRW_Coord{0.0, 0.0, 0.0};
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
}

bool DRW_Tolerance::parseDwg(DRW::Version v, dwgBuffer *buf,
                             std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = v > DRW::AC1018 ? &stringProbe : &bodyProbe;
    if (!DRW_Entity::parseDwg(v, &bodyProbe, stringStream, bs)
        || !bodyProbe.isGood() || !stringStream->isGood())
        return fail();

    DRW_DBG("\n***************************** parsing tolerance *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    const std::uint64_t stringStartBit = currentDwgBit(stringStream);
    std::uint64_t stringEndBit = bodyEndBit;
    if (v > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*stringStream, v, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    auto read3BitDoubleChecked = [&](DRW_Coord& point) {
        return readBoundedBitDouble(bodyProbe, bodyEndBit, point.x)
            && readBoundedBitDouble(bodyProbe, bodyEndBit, point.y)
            && readBoundedBitDouble(bodyProbe, bodyEndBit, point.z);
    };

    if (v < DRW::AC1015) {
        std::uint16_t unknown = 0;
        double heightAtCreation = 0.0;
        double dimGapAtCreation = 0.0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, unknown)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                     heightAtCreation)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                     dimGapAtCreation)
            || !std::isfinite(heightAtCreation)
            || !std::isfinite(dimGapAtCreation))
            return fail();
    }

    DRW_Coord parsedInsertionPoint;
    DRW_Coord parsedXAxisDirectionVector;
    DRW_Coord parsedExtPoint;
    if (!read3BitDoubleChecked(parsedInsertionPoint)
        || !read3BitDoubleChecked(parsedXAxisDirectionVector)
        || !read3BitDoubleChecked(parsedExtPoint))
        return fail();
    UTF8STRING parsedText;
    if (v > DRW::AC1018 && stringStartBit >= stringEndBit) {
        parsedText.clear();
    } else if (!readBoundedVariableText(*stringStream, stringEndBit, v,
                                        parsedText)) {
        return fail();
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!bodyProbe.isGood() || !stringStream->isGood()
        || currentDwgBit(&bodyProbe) > bodyEndBit
        || currentDwgBit(stringStream) > stringEndBit
        || !finite(parsedInsertionPoint)
        || !finite(parsedXAxisDirectionVector)
        || !finite(parsedExtPoint))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, v, objSize, bs, handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(v, &handleProbe, true, handleEndBit)
        || !handleProbe.isGood())
        return fail();
    dwgHandle parsedDimStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                              parsedDimStyleH)
        || !handleProbe.isGood())
        return fail();

    insertionPoint = parsedInsertionPoint;
    xAxisDirectionVector = parsedXAxisDirectionVector;
    extPoint = parsedExtPoint;
    text = parsedText;
    dimStyleH = parsedDimStyleH;
    *sourceBuf = handleProbe;
    DRW_DBG("insertionPoint: ");
    DRW_DBGPT(insertionPoint.x, insertionPoint.y, insertionPoint.z);
    DRW_DBG("\ntolerance text: "); DRW_DBG(text.c_str()); DRW_DBG("\n");
    DRW_DBG("dim style Handle: ");
    DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref);
    DRW_DBG("\n");
    return true;
}

bool DRW_Tolerance::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                              dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 46;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    if (version < DRW::AC1015) {
        buf->putBitShort(0);
        buf->putBitDouble(0.0);
        buf->putBitDouble(0.0);
    }

    buf->put3BitDouble(insertionPoint);
    buf->put3BitDouble(xAxisDirectionVector);
    buf->put3BitDouble(extPoint);
    (strBuf ? strBuf : buf)->putVariableText(version, text);

    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;

    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    putHardPointerHandle(hb, (dimStyleH.ref == 0) ? 0x15 : dimStyleH.ref);
    return true;
}


bool DRW_Block::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        xrefPath = reader->getUtf8String();
        break;
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

void DRW_Block::resetDwgState() {
    const bool endBlock = isEnd;
    DRW_Point::resetDwgState();
    name = endBlock ? UTF8STRING() : UTF8STRING("*U0");
    flags = 0;
    insUnits = 0;
    xrefPath.clear();
    previewData.clear();
    isEnd = endBlock;
}

bool DRW_Block::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    if (buf == nullptr)
        return false;
    auto fail = [this, buf]() {
        buf->invalidate();
        resetDwgState();
        return false;
    };
    dwgBuffer bodyProbe = buf->forkIndependent();
    dwgBuffer sBuff = bodyProbe.forkIndependent();
    dwgBuffer *sBuf = &bodyProbe;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, &bodyProbe, sBuf, bs);
    if (!ret)
        return fail();

    // BLOCK/ENDBLK carry one entity-specific bit after the common data range;
    // the detached string stream begins at dwgDataEndBit and is independent.
    const std::uint64_t bodyEndBit = version > DRW::AC1018
        && dwgDataEndBit < std::numeric_limits<std::uint64_t>::max()
        ? dwgDataEndBit + 1 : dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018
        && (!bodyProbe.getR2007StringStreamBounds(objSize, stringStartBit,
                                                  stringEndBit)
            || currentDwgBit(sBuf) > stringEndBit))
        return fail();

    UTF8STRING parsedName;
    if (!isEnd){
        DRW_DBG("\n***************************** parsing block *********************************************\n");
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     parsedName))
            return fail();
        DRW_DBG("Block name: "); DRW_DBG(parsedName.c_str()); DRW_DBG("\n");
    } else {
        DRW_DBG("\n***************************** parsing end block *********************************************\n");
    }
    if (version > DRW::AC1018) {//2007+
        bool ignoredUnknown = false;
        if (!readBoundedBit(bodyProbe, bodyEndBit, ignoredUnknown))
            return fail();
        DRW_DBG("unknown bit: "); DRW_DBG(ignoredUnknown); DRW_DBG("\n");
    }
//    X handleAssoc;   //X
    if (!bodyProbe.isGood() || currentDwgBit(&bodyProbe) > bodyEndBit)
        return fail();
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*buf, version, objSize, bs, handleEndBit))
        return fail();
    ret = DRW_Entity::parseDwgEntHandle(version, &bodyProbe, true,
                                         handleEndBit);
    if (!ret || !bodyProbe.isGood())
        return fail();
    //    RS crc;   //RS */
    // R13-R2004 keep the handle stream inline and may consume the final
    // byte-alignment padding beyond the declared object-data bit count.
    // Only R2007+ has a separate string stream range to validate here.
    if (!bodyProbe.isGood() || !sBuf->isGood()
        || (version > DRW::AC1018 && currentDwgBit(sBuf) > stringEndBit))
        return fail();
    *buf = bodyProbe;
    if (!isEnd)
        name = std::move(parsedName);
    return true;
}

bool DRW_Insert::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 41:
        xscale = reader->getDouble();
        break;
    case 42:
        yscale = reader->getDouble();
        break;
    case 43:
        zscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        angle = angle/ARAD; //convert to radian
        break;
    case 70: {
        int value = 0;
        if (!readDxfIntInRange(reader, 0, kMaxMInsertCount, value))
            return false;
        colcount = value;
        break;
    }
    case 71: {
        int value = 0;
        if (!readDxfIntInRange(reader, 0, kMaxMInsertCount, value))
            return false;
        rowcount = value;
        break;
    }
    case 44:
        colspace = reader->getDouble();
        break;
    case 45:
        rowspace = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Table::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    auto ensureGrid = [this]() {
        if (m_dxfRowsExpected < 0 || m_dxfColumnsExpected < 0)
            return true;

        const std::uint32_t rows = static_cast<std::uint32_t>(m_dxfRowsExpected);
        const std::uint32_t columns = static_cast<std::uint32_t>(m_dxfColumnsExpected);
        if (rows > kMaxTableRows || columns > kMaxTableColumns
            || (columns != 0 && rows > kMaxTableCells / columns)) {
            return false;
        }

        if (m_content.m_columns.size() != columns) {
            m_content.m_columns.clear();
            if (!DRW::reserve(m_content.m_columns, static_cast<int>(columns)))
                return false;
            if (!DRW::resize(m_content.m_columns, static_cast<int>(columns)))
                return false;
            m_dxfColumnWidthsRead = 0;
        }
        if (m_content.m_rows.size() != rows) {
            m_content.m_rows.clear();
            if (!DRW::reserve(m_content.m_rows, static_cast<int>(rows)))
                return false;
            if (!DRW::resize(m_content.m_rows, static_cast<int>(rows)))
                return false;
            m_dxfRowHeightsRead = 0;
        }
        for (auto& row : m_content.m_rows) {
            if (!DRW::reserve(row.m_cells, static_cast<int>(columns)))
                return false;
            if (!DRW::resize(row.m_cells, static_cast<int>(columns)))
                return false;
        }

        m_hasSemanticContent = true;
        m_semanticContentComplete = true;
        return true;
    };

    auto currentCell = [this]() -> DRW_TableCell* {
        if (m_dxfCurrentCell < 0 || m_content.m_columns.empty()
            || m_content.m_rows.empty()) {
            return nullptr;
        }

        const std::size_t columns = m_content.m_columns.size();
        const std::size_t cell = static_cast<std::size_t>(m_dxfCurrentCell);
        const std::size_t row = cell / columns;
        const std::size_t column = cell % columns;
        if (row >= m_content.m_rows.size()
            || column >= m_content.m_rows[row].m_cells.size()) {
            return nullptr;
        }
        return &m_content.m_rows[row].m_cells[column];
    };

    auto currentContent = [&currentCell]() -> DRW_TableCellContent* {
        DRW_TableCell *cell = currentCell();
        if (cell == nullptr)
            return nullptr;
        if (cell->m_contents.empty() || cell->m_contents.back().m_type != 1) {
            DRW_TableCellContent content;
            content.m_type = 1;
            cell->m_contents.push_back(content);
        }
        return &cell->m_contents.back();
    };

    if (code == 100) {
        const std::string subclass = reader->getString();
        if (subclass == "AcDbBlockReference") {
            m_dxfSubclass = DxfSubclass::BlockReference;
        } else if (subclass == "AcDbTable") {
            m_dxfSubclass = DxfSubclass::Table;
        } else if (subclass == "AcDbEntity") {
            m_dxfSubclass = DxfSubclass::Entity;
        }
        return true;
    }

    if (m_dxfSubclass != DxfSubclass::Table)
        return DRW_Insert::parseCode(code, reader);

    switch (code) {
    case 342:
        m_tableStyleHandle = static_cast<std::uint32_t>(reader->getHandleString());
        m_content.m_tableStyleHandle = m_tableStyleHandle;
        break;
    case 343:
        reader->getHandleString();
        break;
    case 11:
        m_horizontalDirection.x = reader->getDouble();
        break;
    case 21:
        m_horizontalDirection.y = reader->getDouble();
        break;
    case 31:
        m_horizontalDirection.z = reader->getDouble();
        break;
    case 90:
        if (m_dxfInCellValue) {
            if (DRW_TableCellContent *content = currentContent())
                content->m_value.m_dataType = reader->getInt32();
            else
                reader->getInt32();
        } else {
            m_valueFlag = reader->getInt32();
        }
        break;
    case 91:
        if (m_dxfRowsExpected < 0 && !m_dxfInCellValue) {
            int value = 0;
            if (!readDxfIntInRange(reader, 0,
                                   static_cast<int>(kMaxTableRows), value))
                return false;
            m_dxfRowsExpected = value;
            if (!ensureGrid())
                return false;
        } else {
            reader->getInt32();
        }
        break;
    case 92:
        if (m_dxfColumnsExpected < 0 && !m_dxfInCellValue) {
            int value = 0;
            if (!readDxfIntInRange(reader, 0,
                                   static_cast<int>(kMaxTableColumns), value))
                return false;
            m_dxfColumnsExpected = value;
            if (!ensureGrid())
                return false;
        } else {
            reader->getInt32();
        }
        break;
    case 93:
    case 94:
    case 95:
    case 96:
    case 172:
    case 173:
    case 174:
    case 175:
    case 176:
    case 178:
        reader->getInt32();
        break;
    case 141:
        if (!ensureGrid())
            return false;
        if (m_dxfRowHeightsRead < m_content.m_rows.size())
            m_content.m_rows[m_dxfRowHeightsRead++].m_height = reader->getDouble();
        else
            reader->getDouble();
        break;
    case 142:
        if (!ensureGrid())
            return false;
        if (m_dxfColumnWidthsRead < m_content.m_columns.size())
            m_content.m_columns[m_dxfColumnWidthsRead++].m_width = reader->getDouble();
        else
            reader->getDouble();
        break;
    case 145:
        reader->getDouble();
        break;
    case 171:
        if (!ensureGrid())
            return false;
        if (!m_content.m_rows.empty() && !m_content.m_columns.empty()
            && m_dxfNextCell < m_content.m_rows.size() * m_content.m_columns.size()) {
            m_dxfCurrentCell = static_cast<int>(m_dxfNextCell++);
            if (DRW_TableCell *cell = currentCell())
                cell->m_flags = reader->getInt32();
            else
                reader->getInt32();
        } else {
            m_dxfCurrentCell = -1;
            reader->getInt32();
        }
        m_dxfInCellValue = false;
        break;
    case 301:
        m_dxfInCellValue = reader->getString() == "CELL_VALUE";
        if (m_dxfInCellValue)
            currentContent();
        break;
    case 1:
    case 302: {
        const UTF8STRING text = reader->getUtf8String();
        if (m_dxfInCellValue) {
            if (DRW_TableCellContent *content = currentContent()) {
                content->m_text = text;
                content->m_value.m_dataType = 4;
                content->m_value.m_value.addString(1, text);
            }
        }
        break;
    }
    case 300:
        if (m_dxfInCellValue) {
            if (DRW_TableCellContent *content = currentContent())
                content->m_value.m_valueString = reader->getUtf8String();
            else
                reader->getUtf8String();
        } else {
            reader->getUtf8String();
        }
        break;
    case 304:
        reader->getString();
        m_dxfInCellValue = false;
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

void DRW_Insert::resetDwgState() {
    DRW_Point::resetDwgState();
    name.clear();
    xscale = yscale = zscale = 1.0;
    angle = 0.0;
    colcount = rowcount = 1;
    colspace = rowspace = 0.0;
    attlist.clear();
    blockRecH = dwgHandle{};
    seqendH = dwgHandle{};
    attribHandles.clear();
}

bool DRW_Insert::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    if (!DRW_Entity::parseDwgCommon(version, buf, bs)
        || !bodyProbe.isGood())
        return fail();

    std::int32_t objCount = 0;
    std::vector<dwgHandle> parsedAttribHandles;
    DRW_DBG("\n************************** parsing insert/minsert *****************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtPoint;
    double parsedXScale = 1.0;
    double parsedYScale = 1.0;
    double parsedZScale = 1.0;
    double parsedAngle = 0.0;
    bool hasAttrib = false;

    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint))
        return fail();
    DRW_DBG("insertion point: ");
    DRW_DBGPT(parsedBasePoint.x, parsedBasePoint.y, parsedBasePoint.z);
    DRW_DBG("\n");
    if (version < DRW::AC1015) {//14-
        if (!readBoundedBitDouble(*buf, bodyEndBit, parsedXScale)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedYScale)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedZScale))
            return fail();
    } else {
        if (!proxyEntityHasBits(*buf, bodyEndBit, 2))
            return fail();
        const std::uint8_t dataFlags = buf->get2Bits();
        if (!buf->isGood())
            return fail();
        if (dataFlags == 3) {
            //none default value 1,1,1
        } else if (dataFlags == 1) { //x default value 1, y & z use DD(1)
            if (!readBoundedDefaultDouble(*buf, bodyEndBit, 1.0,
                                           parsedYScale)
                || !readBoundedDefaultDouble(*buf, bodyEndBit, 1.0,
                                              parsedZScale))
                return fail();
        } else if (dataFlags == 2) {
            if (!readBoundedRawDouble(*buf, bodyEndBit, parsedXScale))
                return fail();
            parsedYScale = parsedZScale = parsedXScale;
        } else { //dataFlags == 0
            if (!readBoundedRawDouble(*buf, bodyEndBit, parsedXScale)
                || !readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                              parsedYScale)
                || !readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                              parsedZScale))
                return fail();
        }
    }
    if (!readBoundedBitDouble(*buf, bodyEndBit, parsedAngle)
        || !readBoundedExtrusion(*buf, bodyEndBit, false, parsedExtPoint)
        || !readBoundedBit(*buf, bodyEndBit, hasAttrib))
        return fail();
    DRW_DBG("scale : ");
    DRW_DBGPT(parsedXScale, parsedYScale, parsedZScale);
    DRW_DBG(", angle: "); DRW_DBG(parsedAngle);
    DRW_DBG("\nextrusion: ");
    DRW_DBGPT(parsedExtPoint.x, parsedExtPoint.y, parsedExtPoint.z);

    DRW_DBG("   has Attrib: "); DRW_DBG(hasAttrib);

    if (hasAttrib && version > DRW::AC1015) {//2004+
        if (!readBoundedBitLong(*buf, bodyEndBit, objCount)
            || !dwgSafety::validOwnedObjectCount(
                objCount, buf->numRemainingBytes())) {
            DRW_DBG("\nWARNING: Invalid INSERT owned-attribute count\n");
            return fail();
        }
        DRW_DBG("   objCount: "); DRW_DBG(objCount); DRW_DBG("\n");
    }
    int parsedColCount = 1;
    int parsedRowCount = 1;
    double parsedColSpace = 0.0;
    double parsedRowSpace = 0.0;
    if (oType == 8) {//entity are minsert
        std::uint16_t colCount = 0;
        std::uint16_t rowCount = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, colCount)
            || !readBoundedBitShort(*buf, bodyEndBit, rowCount)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedColSpace)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedRowSpace))
            return fail();
        parsedColCount = static_cast<int>(colCount);
        parsedRowCount = static_cast<int>(rowCount);
    }
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedBasePoint) || !finite(parsedExtPoint)
        || !std::isfinite(parsedXScale) || !std::isfinite(parsedYScale)
        || !std::isfinite(parsedZScale) || !std::isfinite(parsedAngle)
        || !std::isfinite(parsedColSpace) || !std::isfinite(parsedRowSpace))
        return fail();
    DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit))
        return fail();
    dwgHandle parsedBlockRecH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              parsedBlockRecH))
        return fail();
    dwgHandle parsedSeqendH;
    DRW_DBG("BLOCK HEADER Handle: "); DRW_DBGHL(parsedBlockRecH.code, parsedBlockRecH.size, parsedBlockRecH.ref); DRW_DBG("\n");
    DRW_DBG("   Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");

    /*attribs follows*/
    if (hasAttrib) {
        const std::int32_t expectedCount =
            version < DRW::AC1018 ? 2 : objCount;
        if (!DRW::reserve(parsedAttribHandles, expectedCount))
            return fail();
        if (version < DRW::AC1018) {//2000-
            dwgHandle attH;
            if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                      attH))
                return fail();
            DRW_DBG("first attrib Handle: "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
            parsedAttribHandles.push_back(attH);
            if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                      attH))
                return fail();
            DRW_DBG("second attrib Handle: "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
            parsedAttribHandles.push_back(attH);
        } else {
            for (std::int32_t i=0; i < objCount; ++i){
                dwgHandle attH;
                if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0,
                                          false, attH))
                    return fail();
                DRW_DBG("attrib Handle #"); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
                parsedAttribHandles.push_back(attH);
            }
        }
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  parsedSeqendH))
            return fail();
        DRW_DBG("seqendH Handle: "); DRW_DBGHL(parsedSeqendH.code, parsedSeqendH.size, parsedSeqendH.ref); DRW_DBG("\n");
    }
    DRW_DBG("   Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");

    if (!handleProbe.isGood())
        return fail();
    *sourceBuf = handleProbe;
    basePoint = parsedBasePoint;
    xscale = parsedXScale;
    yscale = parsedYScale;
    zscale = parsedZScale;
    angle = parsedAngle;
    extPoint = parsedExtPoint;
    colcount = parsedColCount;
    rowcount = parsedRowCount;
    colspace = parsedColSpace;
    rowspace = parsedRowSpace;
    blockRecH = parsedBlockRecH;
    seqendH = parsedSeqendH;
    attribHandles = std::move(parsedAttribHandles);
    //    RS crc;   //RS */
    return true;
}

void DRW_Table::resetDwgState() {
    DRW_Insert::resetDwgState();
    name.clear();
    xscale = yscale = zscale = 1.0;
    angle = 0.0;
    colcount = rowcount = 1;
    colspace = rowspace = 0.0;
    m_valueFlag = 0;
    m_horizontalDirection = DRW_Coord{0.0, 0.0, 0.0};
    m_hasSemanticContent = false;
    m_semanticContentComplete = false;
    m_tableStyleHandle = 0;
    m_content = DRW_TableContent{};
    m_dxfSubclass = DxfSubclass::Entity;
    m_dxfRowsExpected = -1;
    m_dxfColumnsExpected = -1;
    m_dxfRowHeightsRead = 0;
    m_dxfColumnWidthsRead = 0;
    m_dxfNextCell = 0;
    m_dxfCurrentCell = -1;
    m_dxfInCellValue = false;
}

bool DRW_Table::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    auto fail = [this, buf]() {
        if (buf != nullptr)
            buf->invalidate();
        resetDwgState();
        return false;
    };
    if (buf == nullptr || version < DRW::AC1015)
        return fail();

    dwgBuffer *sourceBuf = buf;
    dwgBuffer bodyProbe = buf->forkIndependent();
    buf = &bodyProbe;
    auto commit = [&]() {
        *sourceBuf = bodyProbe;
        return true;
    };

    dwgBuffer sBuff = bodyProbe.forkIndependent();
    sBuff.setVariableTextByteLength(true);
    dwgBuffer *sBuf = &sBuff;
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();

    DRW_DBG("\n************************** parsing table *****************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    std::uint8_t dataFlags = 0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint)
        || !proxyEntityHasBits(*buf, bodyEndBit, 2))
        return fail();
    dataFlags = buf->get2Bits();
    if (!buf->isGood())
        return fail();

    double parsedXScale = 1.0;
    double parsedYScale = 1.0;
    double parsedZScale = 1.0;
    if (dataFlags == 3) {
        // default scale 1,1,1
    } else if (dataFlags == 1) {
        if (!readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                      parsedYScale)
            || !readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                         parsedZScale))
            return fail();
    } else if (dataFlags == 2) {
        if (!readBoundedRawDouble(*buf, bodyEndBit, parsedXScale))
            return fail();
        parsedYScale = parsedZScale = parsedXScale;
    } else {
        if (!readBoundedRawDouble(*buf, bodyEndBit, parsedXScale)
            || !readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                          parsedYScale)
            || !readBoundedDefaultDouble(*buf, bodyEndBit, parsedXScale,
                                         parsedZScale))
            return fail();
    }

    double parsedAngle = 0.0;
    DRW_Coord parsedExtPoint;
    if (!readBoundedBitDouble(*buf, bodyEndBit, parsedAngle)
        || !readBoundedExtrusion(*buf, bodyEndBit, false, parsedExtPoint))
        return fail();

    if (!sBuf->isGood())
        return fail();

    std::int32_t objCount = 0;
    bool hasAttrib = false;
    if (!readBoundedBit(*buf, bodyEndBit, hasAttrib))
        return fail();
    if (hasAttrib && version > DRW::AC1015) {
        if (!readBoundedBitLong(*buf, bodyEndBit, objCount)
            || !dwgSafety::validOwnedObjectCount(
                objCount, buf->numRemainingBytes())) {
            DRW_DBG("\nWARNING: Invalid TABLE owned-attribute count\n");
            return fail();
        }
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer hBuff = buf->forkIndependent();
    if (version <= DRW::AC1018) {
        // R2000/R2004: parseDwgEntHandle only re-seeks to the handle stream
        // for version > AC1018 (2007+ string area).  For the legacy versions
        // seek the snapshot to the handle-stream start (objSize is the
        // bit offset of the handle stream, RL field read in
        // DRW_Entity::parseDwg) or every handle below reads mid-DATA garbage.
        if (!hBuff.setPosition(objSize >> 3))
            return fail();
        hBuff.setBitPos(objSize & 7);
    }
    ret = DRW_Entity::parseDwgEntHandle(version, &hBuff, true,
                                        handleEndBit);
    if (!ret)
        return fail();
    dwgHandle parsedBlockRecH;
    if (!readBoundedDwgHandle(hBuff, handleEndBit, 0, false,
                              parsedBlockRecH))
        return fail();

    std::vector<dwgHandle> parsedAttribHandles;
    dwgHandle parsedSeqendH;

    if (hasAttrib) {
        if (!DRW::reserve(parsedAttribHandles, objCount))
            return fail();
        for (std::int32_t i = 0; i < objCount; ++i) {
            dwgHandle attribHandle;
            if (!readBoundedDwgHandle(hBuff, handleEndBit, 0, false,
                                      attribHandle))
                return fail();
            parsedAttribHandles.push_back(attribHandle);
        }
        if (!readBoundedDwgHandle(hBuff, handleEndBit, 0, false,
                                  parsedSeqendH))
            return fail();
    }

    if (!hBuff.isGood())
        return fail();

    blockRecH = parsedBlockRecH;
    seqendH = parsedSeqendH;
    attribHandles = std::move(parsedAttribHandles);
    basePoint = parsedBasePoint;
    xscale = parsedXScale;
    yscale = parsedYScale;
    zscale = parsedZScale;
    angle = parsedAngle;
    extPoint = parsedExtPoint;

    if (version >= DRW::AC1024) {
        std::uint8_t ignoredTableVersion = 0;
        if (!readBoundedRawChar8(*buf, bodyEndBit, ignoredTableVersion))
            return fail();
        dwgHandle parsedTableStyleHandle;
        if (!readBoundedDwgHandle(hBuff, handleEndBit, 0, false,
                                  parsedTableStyleHandle))
            return fail();
        const std::uint32_t tableStyleHandle = parsedTableStyleHandle.ref;
        std::int32_t ignoredContentFlag = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, ignoredContentFlag))
            return fail();
        if (version >= DRW::AC1027) {
            std::int32_t ignoredContentVersion = 0;
            if (!readBoundedBitLong(*buf, bodyEndBit, ignoredContentVersion))
                return fail();
        } else {
            bool ignoredContentBit = false;
            if (!readBoundedBit(*buf, bodyEndBit, ignoredContentBit))
                return fail();
        }

        if (!buf->isGood() || !hBuff.isGood())
            return fail();

        m_hasSemanticContent = true;
        // TABLECONTENT is an optional semantic tail. Decode it on independent
        // cursors so a newer/partially supported cell variant cannot poison the
        // already validated entity prefix and common handle stream.
        dwgBuffer contentBuf = buf->forkIndependent();
        dwgBuffer contentStringBuf = sBuf->forkIndependent();
        dwgBuffer contentHandleBuf = hBuff.forkIndependent();
        TableDwgBounds tableBounds;
        tableBounds.bodyEndBit = tableBodyEndBit(version, sBuf, objSize);
        if (version > DRW::AC1021) {
            std::uint64_t stringStartBit = 0;
            if (!contentStringBuf.getR2007StringStreamBounds(
                    objSize, stringStartBit, tableBounds.stringEndBit))
                return fail();
        } else {
            tableBounds.stringEndBit = proxyEntityEndBit(contentStringBuf, 0);
        }
        tableBounds.handleEndBit = handleEndBit;
        DRW_TableContent parsedContent;
        m_semanticContentComplete = parseTableContent(
            version, &contentBuf, &contentStringBuf, &contentHandleBuf,
            parsedContent, tableBounds);
        if (!m_semanticContentComplete || !contentBuf.isGood()
            || !contentStringBuf.isGood() || !contentHandleBuf.isGood()) {
            // The frame, entity prefix, and common handles remain valid. Keep
            // the typed table shell and allow the raw DWG frame to preserve
            // content that this reader does not yet model.
            m_semanticContentComplete = false;
            return commit();
        }
        *buf = contentBuf;
        *sBuf = contentStringBuf;
        hBuff = contentHandleBuf;
        m_content = std::move(parsedContent);
        if (m_content.m_tableStyleHandle != 0)
            m_tableStyleHandle = m_content.m_tableStyleHandle;
        else
            m_tableStyleHandle = tableStyleHandle;
        if (!buf->isGood() || !sBuf->isGood() || !hBuff.isGood())
            return fail();
        if (!m_semanticContentComplete) {
            DRW_DBG("TABLECONTENT parse incomplete; anonymous block insert kept\n");
            return commit();
        }

        std::uint16_t ignoredTableFlags = 0;
        DRW_Coord parsedHorizontalDirection;
        if (!readBoundedBitShort(*buf, bodyEndBit, ignoredTableFlags)
            || !readBoundedBitCoord(*buf, bodyEndBit,
                                    parsedHorizontalDirection))
            return fail();
        m_horizontalDirection = parsedHorizontalDirection;

        const std::uint64_t breakStartBit = currentDwgBit(buf);
        std::int32_t breakDataFlag = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, breakDataFlag))
            return fail();
        const bool hasBreakData = breakDataFlag != 0;
        if (hasBreakData) {
            std::int32_t ignoredBreakType = 0;
            double ignoredBreakDistance = 0.0;
            std::int32_t manualPositionCount = 0;
            if (!readBoundedBitLong(*buf, bodyEndBit, ignoredBreakType)
                || !readBoundedBitLong(*buf, bodyEndBit, ignoredBreakType)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         ignoredBreakDistance)
                || !readBoundedBitLong(*buf, bodyEndBit, ignoredBreakType)
                || !readBoundedBitLong(*buf, bodyEndBit, ignoredBreakType)
                || !readBoundedBitLong(*buf, bodyEndBit,
                                       manualPositionCount))
                return fail();
            if (manualPositionCount < 0
                || static_cast<std::uint32_t>(manualPositionCount)
                       > kMaxTableItems) {
                m_content.m_subrecordRanges.push_back(makeDwgSubrecordRange(
                    "table-break-data", breakStartBit, currentDwgBit(buf),
                    version, manualPositionCount, false));
                return commit();
            }
            for (std::int32_t i = 0; i < manualPositionCount; ++i) {
                DRW_Coord ignoredPosition;
                double ignoredRotation = 0.0;
                std::int32_t ignoredPositionType = 0;
                if (!readBoundedBitCoord(*buf, bodyEndBit,
                                         ignoredPosition)
                    || !readBoundedBitDouble(*buf, bodyEndBit,
                                             ignoredRotation)
                    || !readBoundedBitLong(*buf, bodyEndBit,
                                           ignoredPositionType))
                    return fail();
            }
            m_content.m_subrecordRanges.push_back(makeDwgSubrecordRange(
                "table-break-data", breakStartBit, currentDwgBit(buf),
                version, static_cast<std::uint32_t>(manualPositionCount),
                buf->isGood()));
        }

        const std::uint64_t rowRangeStartBit = currentDwgBit(buf);
        std::int32_t rowRangeCount = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, rowRangeCount))
            return fail();
        if (rowRangeCount >= 0
            && static_cast<std::uint32_t>(rowRangeCount) <= kMaxTableItems) {
            for (std::int32_t i = 0; i < rowRangeCount; ++i) {
                DRW_Coord ignoredRangePoint;
                std::int32_t ignoredRangeValue = 0;
                if (!readBoundedBitCoord(*buf, bodyEndBit,
                                         ignoredRangePoint)
                    || !readBoundedBitLong(*buf, bodyEndBit,
                                           ignoredRangeValue)
                    || !readBoundedBitLong(*buf, bodyEndBit,
                                           ignoredRangeValue))
                    return fail();
            }
            if (rowRangeCount != 0) {
                m_content.m_subrecordRanges.push_back(makeDwgSubrecordRange(
                    "table-row-ranges", rowRangeStartBit, currentDwgBit(buf),
                    version, static_cast<std::uint32_t>(rowRangeCount),
                    buf->isGood()));
            }
        } else {
            m_content.m_subrecordRanges.push_back(makeDwgSubrecordRange(
                "table-row-ranges", rowRangeStartBit, currentDwgBit(buf),
                version, rowRangeCount, false));
        }

        if (!buf->isGood() || !sBuf->isGood() || !hBuff.isGood())
            return fail();
        return commit();
    }

    std::uint16_t parsedValueFlag = 0;
    DRW_Coord parsedHorizontalDirection;
    std::int32_t parsedColumnCount = 0;
    std::int32_t parsedRowCount = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedValueFlag)
        || !readBoundedBitCoord(*buf, bodyEndBit,
                                parsedHorizontalDirection)
        || !readBoundedBitLong(*buf, bodyEndBit, parsedColumnCount)
        || !readBoundedBitLong(*buf, bodyEndBit, parsedRowCount)
        || parsedColumnCount < 0 || parsedRowCount < 0)
        return fail();
    const std::uint32_t columns = static_cast<std::uint32_t>(parsedColumnCount);
    const std::uint32_t rows = static_cast<std::uint32_t>(parsedRowCount);
    if (columns > kMaxTableColumns || rows > kMaxTableRows
        || (columns != 0 && rows > kMaxTableCells / columns)) {
        return fail();
    }

    m_hasSemanticContent = true;
    m_semanticContentComplete = false;
    m_content.m_columns.clear();
    m_content.m_rows.clear();
    if (!DRW::reserve(m_content.m_columns, static_cast<int>(columns))
        || !DRW::reserve(m_content.m_rows, static_cast<int>(rows)))
        return fail();
    for (std::uint32_t i = 0; i < columns; ++i) {
        DRW_TableColumn column;
        if (!readBoundedBitDouble(*buf, bodyEndBit, column.m_width))
            return fail();
        m_content.m_columns.push_back(column);
    }
    for (std::uint32_t i = 0; i < rows; ++i) {
        DRW_TableRow row;
        if (!readBoundedBitDouble(*buf, bodyEndBit, row.m_height))
            return fail();
        if (!DRW::reserve(row.m_cells, static_cast<int>(columns)))
            return fail();
        if (!DRW::resize(row.m_cells, static_cast<int>(columns)))
            return fail();
        m_content.m_rows.push_back(row);
    }
    dwgHandle parsedTableStyleHandle;
    if (!readBoundedDwgHandle(hBuff, handleEndBit, 0, false,
                              parsedTableStyleHandle))
        return fail();
    m_tableStyleHandle = parsedTableStyleHandle.ref;
    if (!hBuff.isGood())
        return fail();
    m_content.m_tableStyleHandle = m_tableStyleHandle;
    m_semanticContentComplete = true;
    // For <=AC1018 (R2000/R2004) there is no separate R2007+ string stream:
    // DRW_Entity::parseDwg only seeks sBuf when version > AC1018 (see the
    // `strBuf != NULL && version > DRW::AC1018` guard there), so legacy cell
    // text is inline in `buf`.  Passing the stale sBuf copy here would read
    // text from the wrong position and desync `buf`.  Pass nullptr so the
    // cell readers' `textBuf = strBuf ? strBuf : buf` falls back to the
    // inline `buf`.  R2007 (AC1021) keeps the separate sBuf stream.
    dwgBuffer *cellStrBuf = (version > DRW::AC1018) ? sBuf : nullptr;
    TableDwgBounds tableBounds;
    tableBounds.bodyEndBit = bodyEndBit;
    tableBounds.stringEndBit = cellStrBuf == nullptr
        ? bodyEndBit : proxyEntityEndBit(*cellStrBuf, 0);
    tableBounds.handleEndBit = handleEndBit;
    for (std::uint32_t row = 0; row < rows && m_semanticContentComplete; ++row) {
        for (std::uint32_t column = 0; column < columns; ++column) {
            if (!parseR2007TableCell(version, buf, cellStrBuf, &hBuff,
                                     m_content.m_rows[row].m_cells[column],
                                     &m_content.m_subrecordRanges,
                                     tableBounds)) {
                m_semanticContentComplete = false;
                break;
            }
        }
    }

    if (m_semanticContentComplete)
        m_semanticContentComplete = skipR2007TableOverrides(
            version, buf, cellStrBuf, &hBuff, &m_content.m_subrecordRanges,
            tableBounds);
    if (!m_semanticContentComplete)
        DRW_DBG("R2007 TABLE cell parse incomplete; anonymous block insert kept\n");

    if (!buf->isGood() || !sBuf->isGood() || !hBuff.isGood())
        return fail();
    m_valueFlag = parsedValueFlag;
    m_horizontalDirection = parsedHorizontalDirection;
    return commit();
}

bool DRW_TableContentObject::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    reset();
    const auto fail = [this, buf]() {
        if (buf != nullptr)
            buf->invalidate();
        reset();
        return false;
    };
    if (buf == nullptr || version <= DRW::AC1018)
        return fail();

    dwgBuffer sBuff = *buf;
    sBuff.setVariableTextByteLength(true);
    dwgBuffer *sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n************************** parsing table content object ************************\n");
    if (!ret)
        return fail();

    dwgBuffer bodyBuff = *buf;
    dwgBuffer *bodyBuf = &bodyBuff;

    dwgBuffer hBuff = *buf;
    seekTableObjectHandleStream(version, &hBuff, objSize);
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*buf, version, objSize, bs, handleEndBit))
        return fail();
    std::uint32_t parsedParentHandle = 0;
    std::vector<std::uint32_t> parsedReactors;
    std::uint32_t parsedXDictHandle = 0;
    if (!readTableObjectCommonHandles(&hBuff, handle, numReactors, xDictFlag,
                                      &parsedParentHandle, &parsedReactors,
                                      &parsedXDictHandle, handleEndBit))
        return fail();

    DRW_TableContent parsedContent;
    TableDwgBounds tableBounds;
    tableBounds.bodyEndBit = tableBodyEndBit(version, sBuf, objSize);
    tableBounds.stringEndBit = proxyEntityEndBit(*sBuf, 0);
    tableBounds.handleEndBit = handleEndBit;
    const bool parseComplete = parseTableContent(
        version, bodyBuf, sBuf, &hBuff, parsedContent, tableBounds);
    const std::uint64_t bodyBitOffset =
        bodyBuf->getPosition() * 8u + bodyBuf->getBitPos();
    if (!parseComplete || !bodyBuf->isGood() || !sBuf->isGood()
        || !hBuff.isGood() || bodyBitOffset > objSize)
        return fail();

    m_content = std::move(parsedContent);
    m_parseComplete = true;
    parentHandle = parsedParentHandle;
    reactorHandles = std::move(parsedReactors);
    xDictHandle = parsedXDictHandle;
    return true;
}

void DRW_LWPolyline::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        for (unsigned int i=0; i<vertlist.size(); i++) {
			auto& vert = vertlist.at(i);
            DRW_Coord v(vert->x, vert->y, elevation);
            extrudePoint(extPoint, &v);
            vert->x = v.x;
            vert->y = v.y;
        }
    }
}

bool DRW_LWPolyline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10: {
		if (vertlist.size() >= static_cast<std::size_t>(kMaxLWPolylineVertices)
            || (m_dxfVertexCountSeen
                && vertlist.size() >= static_cast<std::size_t>(vertexnum)))
            return false;
		vertex = std::make_shared<DRW_Vertex2D>();
        vertlist.push_back(vertex);
        vertex->x = reader->getDouble();
        break; }
    case 20:
		if(vertex)
            vertex->y = reader->getDouble();
        break;
    case 40:
		if(vertex)
            vertex->stawidth = reader->getDouble();
        break;
    case 41:
		if(vertex)
            vertex->endwidth = reader->getDouble();
        break;
    case 42:
		if(vertex)
            vertex->bulge = reader->getDouble();
        break;
    case 91:
        if (vertex)
            vertex->identifier = reader->getInt32();
        break;
    case 38:
        elevation = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 43:
        width = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 90:
        if (m_dxfVertexCountSeen)
            return false;
        vertexnum = reader->getInt32();
        if (!isValidCount(vertexnum, kMaxLWPolylineVertices))
            return false;
        m_dxfVertexCountSeen = true;
        return DRW::reserve(vertlist, vertexnum);
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_LWPolyline::validateDxf() const {
    return (!m_dxfVertexCountSeen
            || vertlist.size() == static_cast<std::size_t>(vertexnum))
        && validatePayloadFields();
}

bool DRW_LWPolyline::validatePayloadFields() const {
    const auto finite = [](double value) { return std::isfinite(value); };
    const auto finiteVertex = [&finite](const auto& vertex) {
        return vertex != nullptr && finite(vertex->x) && finite(vertex->y)
            && finite(vertex->stawidth) && finite(vertex->endwidth)
            && finite(vertex->bulge)
            && vertex->identifier >= std::numeric_limits<std::int32_t>::min()
            && vertex->identifier <= std::numeric_limits<std::int32_t>::max();
    };

    // LWPOLYLINE group 70 has only the closed and linetype-generation bits.
    return flags >= 0 && (flags & ~0x81) == 0
        && finite(width) && finite(elevation) && finite(thickness)
        && finite(extPoint.x) && finite(extPoint.y) && finite(extPoint.z)
        && vertlist.size() <= static_cast<std::size_t>(kMaxLWPolylineVertices)
        && vertlist.size()
               <= static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        && std::all_of(vertlist.cbegin(), vertlist.cend(), finiteVertex);
}

bool DRW_LWPolyline::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    const auto resetState = [this]() {
        DRW_Entity::reset();
        vertexnum = 0;
        flags = 0;
        width = 0.0;
        elevation = 0.0;
        thickness = 0.0;
        extPoint = DRW_Coord{0.0, 0.0, 1.0};
        vertlist.clear();
        vertex.reset();
        m_dxfVertexCountSeen = false;
    };
    resetState();
    dwgBuffer *sourceBuf = buf;
    const auto fail = [sourceBuf, resetState]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    bool ret = DRW_Entity::parseDwgCommon(version, buf, bs);
    if (!ret || !buf->isGood())
        return fail();
    DRW_DBG("\n***************************** parsing LWPolyline *******************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint16_t rawFlagsValue = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, rawFlagsValue))
        return fail();
    const int rawFlags = static_cast<int>(rawFlagsValue);
    double parsedWidth = 0.0;
    double parsedElevation = 0.0;
    double parsedThickness = 0.0;
    DRW_Coord parsedExtPoint{0.0, 0.0, 1.0};
    if ((rawFlags & 4)
        && !readBoundedBitDouble(*buf, bodyEndBit, parsedWidth))
        return fail();
    if ((rawFlags & 8)
        && !readBoundedBitDouble(*buf, bodyEndBit, parsedElevation))
        return fail();
    if ((rawFlags & 2)
        && !readBoundedBitDouble(*buf, bodyEndBit, parsedThickness))
        return fail();
    if ((rawFlags & 1)
        && !readBoundedExtrusion(*buf, bodyEndBit, false, parsedExtPoint))
        return fail();
    DRW_DBG("flags value: "); DRW_DBG(rawFlags);

    const std::uint32_t minimumVertexBits = version < DRW::AC1015 ? 128u : 4u;
    std::uint32_t parsedVertexCount = 0;
    if (!readTableBodyCount(version, buf, objSize,
                            kMaxLWPolylineVertices, minimumVertexBits,
                            parsedVertexCount)) {
        return fail();
    }

    std::uint64_t vertexBits = 0;
    if (parsedVertexCount > 0) {
        if (version < DRW::AC1015) {
            if (!dwgSafety::multiply(parsedVertexCount, 128, vertexBits))
                return fail();
        } else {
            std::uint64_t deltaBits = 0;
            if (!dwgSafety::multiply(parsedVertexCount - 1, 4, deltaBits)
                || !dwgSafety::add(128, deltaBits, vertexBits))
                return fail();
        }
    }
    if (!proxyEntityHasBits(*buf, bodyEndBit, vertexBits))
        return fail();

    std::vector<std::shared_ptr<DRW_Vertex2D>> parsedVertices;
    if (!DRW::reserve(parsedVertices, parsedVertexCount))
        return fail();

    std::uint32_t bulgesnum = 0;
    if (rawFlags & 16) {
        if (!readTableBodyCount(version, buf, objSize,
                                kMaxLWPolylineVertices, 2,
                                bulgesnum))
            return fail();
    }
    std::uint32_t vertexIdCount = 0;
    if (version > DRW::AC1021) {//2010+
        if (rawFlags & 1024) {
            if (!readTableBodyCount(version, buf, objSize,
                                    kMaxLWPolylineVertices, 2,
                                    vertexIdCount))
                return fail();
        }
    }
    std::uint32_t widthsnum = 0;
    if (rawFlags & 32) {
        if (!readTableBodyCount(version, buf, objSize,
                                kMaxLWPolylineVertices, 4,
                                widthsnum))
            return fail();
    }
    std::uint64_t bulgeBits = 0;
    std::uint64_t vertexIdBits = 0;
    std::uint64_t widthBits = 0;
    std::uint64_t optionalBits = 0;
    if (!dwgSafety::multiply(bulgesnum, 2, bulgeBits)
        || !dwgSafety::multiply(vertexIdCount, 2, vertexIdBits)
        || !dwgSafety::multiply(widthsnum, 4, widthBits)
        || !dwgSafety::add(bulgeBits, vertexIdBits, optionalBits)
        || !dwgSafety::add(optionalBits, widthBits, optionalBits)) {
        return fail();
    }
    if (!proxyEntityHasBits(*buf, bodyEndBit, optionalBits))
        return fail();
    DRW_DBG("\nvertex num: "); DRW_DBG(parsedVertexCount); DRW_DBG(" bulges num: "); DRW_DBG(bulgesnum);
    DRW_DBG(" vertexIdCount: "); DRW_DBG(vertexIdCount); DRW_DBG(" widths num: "); DRW_DBG(widthsnum);

    if (parsedVertexCount > 0) { //verify if is lwpol without vertex (empty)
        // add vertexes
		auto parsedVertex = std::make_shared<DRW_Vertex2D>();
		if (!readBoundedRawDouble(*buf, bodyEndBit, parsedVertex->x)
		    || !readBoundedRawDouble(*buf, bodyEndBit, parsedVertex->y))
		    return fail();
		parsedVertices.push_back(parsedVertex);
		auto pv = parsedVertex;
		for (std::uint32_t i = 1; i < parsedVertexCount; ++i){
			parsedVertex = std::make_shared<DRW_Vertex2D>();
			if (version < DRW::AC1015) {//14-
                if (!readBoundedRawDouble(*buf, bodyEndBit, parsedVertex->x)
                    || !readBoundedRawDouble(*buf, bodyEndBit, parsedVertex->y))
                    return fail();
            } else {
                if (!readBoundedDefaultDouble(*buf, bodyEndBit, pv->x,
                                               parsedVertex->x)
                    || !readBoundedDefaultDouble(*buf, bodyEndBit, pv->y,
                                                  parsedVertex->y))
                    return fail();
            }
			pv = parsedVertex;
			parsedVertices.push_back(parsedVertex);
        }
    }
    // The optional vectors are independent DWG fields, not per-point
    // conditionals. Consume them even when num_points is zero so the handle
    // stream remains aligned; values without a corresponding vertex are
    // intentionally discarded by the existing model.
    for (std::uint32_t i = 0; i < bulgesnum; i++){
		double bulge = 0.0;
		if (!readBoundedBitDouble(*buf, bodyEndBit, bulge))
		    return fail();
        if (i < parsedVertices.size())
            parsedVertices.at(i)->bulge = bulge;
    }
    //add vertexId
    if (version > DRW::AC1021) {//2010+
        for (std::uint32_t i = 0; i < vertexIdCount; i++){
			std::int32_t vertexId = 0;
			if (!readBoundedBitLong(*buf, bodyEndBit, vertexId))
			    return fail();
            if (i < parsedVertices.size())
                parsedVertices.at(i)->identifier = vertexId;
        }
    }
    //add widths
    for (std::uint32_t i = 0; i < widthsnum; i++){
		double staW = 0.0;
		double endW = 0.0;
		if (!readBoundedBitDouble(*buf, bodyEndBit, staW)
		    || !readBoundedBitDouble(*buf, bodyEndBit, endW))
		    return fail();
        if (i < parsedVertices.size()) {
            parsedVertices.at(i)->stawidth = staW;
            parsedVertices.at(i)->endwidth = endW;
        }
    }
    if (DRW_DBGGL == DRW_dbg::Level::Debug){
        DRW_DBG("\nVertex list: ");
        for (auto& pv: parsedVertices) {
            DRW_DBG("\n   x: "); DRW_DBG(pv->x); DRW_DBG(" y: "); DRW_DBG(pv->y); DRW_DBG(" bulge: "); DRW_DBG(pv->bulge);
            DRW_DBG(" stawidth: "); DRW_DBG(pv->stawidth); DRW_DBG(" endwidth: "); DRW_DBG(pv->endwidth);
            DRW_DBG(" identifier: "); DRW_DBG(pv->identifier);
        }
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedExtPoint) || !std::isfinite(parsedWidth)
        || !std::isfinite(parsedElevation) || !std::isfinite(parsedThickness)
        || !std::all_of(parsedVertices.begin(), parsedVertices.end(),
                         [](const std::shared_ptr<DRW_Vertex2D>& point) {
                             return point != nullptr
                                 && std::isfinite(point->x)
                                 && std::isfinite(point->y)
                                 && std::isfinite(point->bulge)
                                 && std::isfinite(point->stawidth)
                                 && std::isfinite(point->endwidth);
                         }))
        return fail();

    DRW_DBG("\n");
    /* Common Entity Handle Data */
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();

    // Translate DWG LWPLINE flag bits to DXF group 70 bits.
    int dxfFlags = 0;
    if (rawFlags & 512)
        dxfFlags |= 1;
    if (rawFlags & 256)
        dxfFlags |= 128;
    vertexnum = static_cast<std::int32_t>(parsedVertexCount);
    flags = dxfFlags;
    width = parsedWidth;
    elevation = parsedElevation;
    thickness = parsedThickness;
    extPoint = parsedExtPoint;
    vertlist = std::move(parsedVertices);
    *sourceBuf = handleProbe;
    DRW_DBG("end flags value: "); DRW_DBG(flags);
    /* CRC X --- */
    return true;
}


// ----------------------------------------------------------------------------
// DRW_MLine — multiline entity (ODA §19.4.78, fixed type 0x2F = 47).
// ----------------------------------------------------------------------------

bool DRW_MLine::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 2:
        styleName = reader->getString();
        break;
    case 340:
        styleHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 40:
        scale = reader->getDouble();
        break;
    case 70:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 2, value))
                return false;
            justification = static_cast<std::uint8_t>(value);
        }
        break;
    case 71:
        openClosed = reader->getInt32();
        break;
    case 72:
        if (m_dxfVertexCountSeen)
            return false;
        {
            const std::int32_t count = reader->getInt32();
            if (!isValidCount(count, kMaxDxfVertices))
                return false;
            numVerts = static_cast<std::uint16_t>(count);
            m_dxfVertexCountSeen = true;
        }
        break;
    case 73:
        if (m_dxfLineCountSeen)
            return false;
        {
            const std::int32_t count = reader->getInt32();
            if (!isValidCount(count, kMaxDxfLines))
                return false;
            numLines = static_cast<std::uint8_t>(count);
            m_dxfLineCountSeen = true;
        }
        break;
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 210:
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    // Per-vertex block: code 11 starts a new vertex; 12/13 follow.
    case 11:
        if (m_currentSegExpected >= 0 || m_currentAreaExpected >= 0)
            return false;
        ++m_currentVertexIdx;
        m_currentElementIdx = 0;
        if (m_currentVertexIdx >= kMaxDxfVertices
            || (m_dxfVertexCountSeen
                && m_currentVertexIdx >= static_cast<int>(numVerts)))
            return false;
        vertlist.emplace_back();
        vertlist[m_currentVertexIdx].position.x = reader->getDouble();
        break;
    case 21:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].position.y = reader->getDouble();
        break;
    case 31:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].position.z = reader->getDouble();
        break;
    case 12:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.x = reader->getDouble();
        break;
    case 22:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.y = reader->getDouble();
        break;
    case 32:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.z = reader->getDouble();
        break;
    case 13:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.x = reader->getDouble();
        break;
    case 23:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.y = reader->getDouble();
        break;
    case 33:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.z = reader->getDouble();
        break;
    // 74 = segment-param count for current element. Sets up the inner
    // vector and resets the running param count. 41 reads each param.
    // 75 = fill-param count; 42 reads each. After fills are consumed,
    // advance to the next element. AutoCAD emits 74/41*/75/42* per element.
    case 74:
        if (m_currentVertexIdx < 0
            || m_currentSegExpected >= 0 || m_currentAreaExpected >= 0)
            return false;
        {
            auto& v = vertlist[m_currentVertexIdx];
            if (m_currentElementIdx != static_cast<int>(v.segParms.size())
                || m_currentElementIdx >= kMaxDxfLines
                || (m_dxfLineCountSeen
                    && m_currentElementIdx >= static_cast<int>(numLines)))
                return false;
            const std::int32_t count = reader->getInt32();
            if (!isValidCount(count, kMaxDxfParameters))
                return false;
            v.segParms.emplace_back();
            v.areaFillParms.emplace_back();
            m_currentSegExpected = count;
            m_currentSegFillCount = 0;
        }
        break;
    case 41:
        if (m_currentVertexIdx < 0 || m_currentSegExpected < 0
            || m_currentElementIdx >= static_cast<int>(vertlist[m_currentVertexIdx].segParms.size()))
            return false;
        if (static_cast<std::int32_t>(
                vertlist[m_currentVertexIdx].segParms[m_currentElementIdx].size())
            >= m_currentSegExpected)
            return false;
        vertlist[m_currentVertexIdx].segParms[m_currentElementIdx]
            .push_back(reader->getDouble());
        break;
    case 75:
        if (m_currentVertexIdx < 0 || m_currentSegExpected < 0
            || m_currentAreaExpected >= 0
            || m_currentElementIdx >= static_cast<int>(vertlist[m_currentVertexIdx].segParms.size()))
            return false;
        if (static_cast<std::int32_t>(
                vertlist[m_currentVertexIdx].segParms[m_currentElementIdx].size())
            != m_currentSegExpected)
            return false;
        m_currentSegExpected = -1;
        m_currentSegFillCount = reader->getInt32();
        if (!isValidCount(m_currentSegFillCount, kMaxDxfParameters))
            return false;
        m_currentAreaExpected = m_currentSegFillCount;
        if (m_currentAreaExpected == 0) {
            m_currentAreaExpected = -1;
            ++m_currentElementIdx;
        }
        break;
    case 42:
        if (m_currentVertexIdx < 0 || m_currentAreaExpected < 0
            || m_currentElementIdx >= static_cast<int>(vertlist[m_currentVertexIdx].areaFillParms.size()))
            return false;
        if (static_cast<std::int32_t>(
                vertlist[m_currentVertexIdx].areaFillParms[m_currentElementIdx].size())
            >= m_currentAreaExpected)
            return false;
        vertlist[m_currentVertexIdx].areaFillParms[m_currentElementIdx]
            .push_back(reader->getDouble());
        if (static_cast<std::int32_t>(
                vertlist[m_currentVertexIdx].areaFillParms[m_currentElementIdx].size())
            == m_currentAreaExpected) {
            m_currentAreaExpected = -1;
            ++m_currentElementIdx;
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_MLine::validateDxf() const {
    if (m_currentSegExpected >= 0 || m_currentAreaExpected >= 0)
        return false;
    if (m_dxfVertexCountSeen && vertlist.size() != numVerts)
        return false;
    if (!m_dxfLineCountSeen)
        return true;
    for (const DRW_MLineVertex& vertex : vertlist) {
        if (vertex.segParms.size() != numLines
            || vertex.areaFillParms.size() != numLines)
            return false;
    }
    return validatePayloadFields();
}

bool DRW_MLine::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(basePoint) || !finite(extPoint) || !std::isfinite(scale)
        || openClosed < 0 || openClosed > 0xffff
        || numVerts > static_cast<std::uint16_t>(kMaxDxfVertices)
        || vertlist.size() > static_cast<std::size_t>(kMaxDxfVertices)
        || vertlist.size() != static_cast<std::size_t>(numVerts))
        return false;

    for (const DRW_MLineVertex& vertex : vertlist) {
        if (!finite(vertex.position) || !finite(vertex.vertexDir)
            || !finite(vertex.miterDir)
            || vertex.segParms.size() != numLines
            || vertex.areaFillParms.size() != numLines)
            return false;
        const auto validParameters = [](const auto& parameterLists) {
            return std::all_of(
                parameterLists.cbegin(), parameterLists.cend(),
                [](const std::vector<double>& values) {
                    return values.size() <= static_cast<std::size_t>(
                               DRW_MLine::kMaxDxfParameters)
                        && std::all_of(values.cbegin(), values.cend(),
                                       [](double value) {
                                           return std::isfinite(value);
                                       });
                });
        };
        if (!validParameters(vertex.segParms)
            || !validParameters(vertex.areaFillParms))
            return false;
    }
    return true;
}

void DRW_MLine::resetDwgState() {
    DRW_Entity::reset();
    scale = 1.0;
    justification = 0;
    basePoint = DRW_Coord{0.0, 0.0, 0.0};
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
    openClosed = 1;
    numLines = 0;
    numVerts = 0;
    styleName.clear();
    styleHandle = 0;
    vertlist.clear();
    m_currentVertexIdx = -1;
    m_currentElementIdx = 0;
    m_currentSegFillCount = 0;
    m_currentSegExpected = -1;
    m_currentAreaExpected = -1;
    m_dxfVertexCountSeen = false;
    m_dxfLineCountSeen = false;
}

bool DRW_MLine::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    if (!DRW_Entity::parseDwgCommon(version, buf, bs)) return fail();
    DRW_DBG("\n***************************** parsing MLINE *********************\n");
    // Per ODA §19.4.78 / libreDWG dwg_decode_MLINE:
    //   BD scale, RC justification, 3BD basePoint, BE extrusion,
    //   BS open/closed flag, RC num_lines, BS num_verts,
    //   then per-vertex: 3BD pos, 3BD vdir, 3BD mdir,
    //     per-line: BS num_segparms × BD parm, BS num_areafillparms × BD parm.
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    double parsedScale = 0.0;
    std::uint8_t parsedJustification = 0;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtPoint;
    std::uint16_t parsedOpenClosed = 0;
    std::uint8_t parsedNumLines = 0;
    std::uint16_t parsedNumVerts = 0;
    if (!readBoundedBitDouble(*buf, bodyEndBit, parsedScale)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedJustification)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint)
        || !readBoundedExtrusion(*buf, bodyEndBit, false, parsedExtPoint)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedOpenClosed)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedNumLines)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedNumVerts))
        return fail();
    if (!std::isfinite(parsedScale)
        || !std::isfinite(parsedBasePoint.x)
        || !std::isfinite(parsedBasePoint.y)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedExtPoint.x)
        || !std::isfinite(parsedExtPoint.y)
        || !std::isfinite(parsedExtPoint.z))
        return fail();
    DRW_DBG(" mline scale: "); DRW_DBG(parsedScale);
    DRW_DBG(" just: "); DRW_DBG(static_cast<int>(parsedJustification));
    DRW_DBG(" openClosed: "); DRW_DBG(parsedOpenClosed);
    DRW_DBG(" lines: "); DRW_DBG(static_cast<int>(parsedNumLines));
    DRW_DBG(" verts: "); DRW_DBG(parsedNumVerts); DRW_DBG("\n");
    if (!buf->isGood()
        || !isValidCount(static_cast<std::int32_t>(parsedNumVerts), kMaxDxfVertices)
        || !isValidCount(static_cast<std::int32_t>(parsedNumLines), kMaxDxfLines))
        return fail();

    // Each vertex always contains three minimum-width 3BD values and one
    // minimum-width BS for each segment/fill parameter count. Check this
    // before allocating the per-vertex/per-line vector grid.
    const std::uint64_t minimumBitsPerVertex =
        18u + static_cast<std::uint64_t>(parsedNumLines) * 4u;
    std::uint64_t minimumVertexBits = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(parsedNumVerts),
                             minimumBitsPerVertex, minimumVertexBits)
        || !proxyEntityHasBits(*buf, bodyEndBit, minimumVertexBits))
        return fail();

    // The per-vertex parameter lists are also capped by the DWG spec. Keep a
    // combined budget so a valid per-list maximum cannot multiply into an
    // unbounded number of vector allocations on hostile input.
    constexpr std::uint64_t kMaxDwgParameters = 1000000;
    std::uint64_t totalParameters = 0;
    std::vector<DRW_MLineVertex> parsedVertices;
    if (!DRW::reserve(parsedVertices, parsedNumVerts))
        return fail();
    for (int vi = 0; vi < parsedNumVerts; ++vi) {
        DRW_MLineVertex vtx;
        if (!readBoundedBitCoord(*buf, bodyEndBit, vtx.position)
            || !readBoundedBitCoord(*buf, bodyEndBit, vtx.vertexDir)
            || !readBoundedBitCoord(*buf, bodyEndBit, vtx.miterDir))
            return fail();
        if (!DRW::reserve(vtx.segParms, parsedNumLines)
            || !DRW::reserve(vtx.areaFillParms, parsedNumLines))
            return fail();
        if (!DRW::resize(vtx.segParms, parsedNumLines)
            || !DRW::resize(vtx.areaFillParms, parsedNumLines))
            return fail();
        for (int li = 0; li < parsedNumLines; ++li) {
            std::uint16_t nSeg = 0;
            if (!readBoundedBitShort(*buf, bodyEndBit, nSeg)
                || !proxyEntityHasBits(*buf, bodyEndBit,
                                       static_cast<std::uint64_t>(nSeg) * 2u))
                return fail();
            if (!buf->isGood()
                || !isValidCount(static_cast<std::int32_t>(nSeg),
                                 kMaxDxfParameters)
                || totalParameters > kMaxDwgParameters - nSeg)
                return fail();
            totalParameters += nSeg;
            if (!DRW::reserve(vtx.segParms[li], nSeg))
                return fail();
            for (int s = 0; s < nSeg; ++s) {
                double value = 0.0;
                if (!readBoundedBitDouble(*buf, bodyEndBit, value)
                    || !std::isfinite(value))
                    return fail();
                vtx.segParms[li].push_back(value);
            }
            std::uint16_t nFill = 0;
            if (!readBoundedBitShort(*buf, bodyEndBit, nFill)
                || !proxyEntityHasBits(*buf, bodyEndBit,
                                       static_cast<std::uint64_t>(nFill) * 2u))
                return fail();
            if (!buf->isGood()
                || !isValidCount(static_cast<std::int32_t>(nFill),
                                 kMaxDxfParameters)
                || totalParameters > kMaxDwgParameters - nFill)
                return fail();
            totalParameters += nFill;
            if (!DRW::reserve(vtx.areaFillParms[li], nFill))
                return fail();
            for (int f = 0; f < nFill; ++f) {
                double value = 0.0;
                if (!readBoundedBitDouble(*buf, bodyEndBit, value)
                    || !std::isfinite(value))
                    return fail();
                vtx.areaFillParms[li].push_back(value);
            }
        }
        const auto finite = [](const DRW_Coord& point) {
            return std::isfinite(point.x) && std::isfinite(point.y)
                && std::isfinite(point.z);
        };
        if (!finite(vtx.position) || !finite(vtx.vertexDir)
            || !finite(vtx.miterDir))
            return fail();
        parsedVertices.push_back(std::move(vtx));
    }
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit))
        return fail();
    // MLINE has one extra handle in the handle stream after the standard
    // entity handles: the MLINESTYLE reference. Read if available — some
    // older files (R14) store the style name inline instead.
    std::uint32_t parsedStyleHandle = 0;
    if (version > DRW::AC1014
        && proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle styleH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                  styleH))
            return fail();
        if (!handleProbe.isGood())
            return fail();
        parsedStyleHandle = styleH.ref;
        DRW_DBG(" MLINE style handle: ");
        DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");
    }
    if (!handleProbe.isGood())
        return fail();
    *sourceBuf = handleProbe;
    scale = parsedScale;
    justification = parsedJustification;
    basePoint = parsedBasePoint;
    extPoint = parsedExtPoint;
    openClosed = parsedOpenClosed;
    numLines = parsedNumLines;
    numVerts = parsedNumVerts;
    styleHandle = parsedStyleHandle;
    vertlist = std::move(parsedVertices);
    return true;
}


// ----------------------------------------------------------------------------
// DRW_Underlay — UNDERLAY entity (PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY).
// libreDWG UNDERLAYREFERENCE.spec field order:
//   extrusion (BE) -> position (3BD) -> angle (BD radians) -> scale (BD x 3)
//   -> flags (RC) -> contrast (RC) -> fade (RC) -> num_clip (BL)
//   -> clip_verts (2RD × num_clip).
// Handle stream after standard entity handles: definition_id (H).
// ----------------------------------------------------------------------------

bool DRW_Underlay::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 340:
        definitionHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 10: position.x = reader->getDouble(); break;
    case 20: position.y = reader->getDouble(); break;
    case 30: position.z = reader->getDouble(); break;
    case 41: scale.x = reader->getDouble(); break;
    case 42: scale.y = reader->getDouble(); break;
    case 43: scale.z = reader->getDouble(); break;
    case 50: rotation = reader->getDouble(); break;  // degrees in DXF
    case 210: extPoint.x = reader->getDouble(); break;
    case 220: extPoint.y = reader->getDouble(); break;
    case 230: extPoint.z = reader->getDouble(); break;
    case 280: {
        int value = 0;
        if (!readDxfIntInRange(reader, 0, 0xFF, value))
            return false;
        flags = static_cast<std::uint8_t>(value);
        break;
    }
    case 281: {
        int value = 0;
        if (!readDxfIntInRange(reader, 0, 100, value))
            return false;
        contrast = static_cast<std::uint8_t>(value);
        break;
    }
    case 282: {
        int value = 0;
        if (!readDxfIntInRange(reader, 0, 100, value))
            return false;
        fade = static_cast<std::uint8_t>(value);
        break;
    }
    case 11: {
        if (clipBoundary.size() >= kMaxClipVertices)
            return false;
        ++m_currentClipVertexIdx;
        if (m_currentClipVertexIdx >= static_cast<int>(clipBoundary.size())) {
            if (!DRW::resize(clipBoundary, m_currentClipVertexIdx + 1))
                return false;
        }
        clipBoundary[m_currentClipVertexIdx].x = reader->getDouble();
        break;
    }
    case 21:
        if (m_currentClipVertexIdx >= 0
            && m_currentClipVertexIdx < static_cast<int>(clipBoundary.size())) {
            clipBoundary[m_currentClipVertexIdx].y = reader->getDouble();
        }
        break;
    case 170: {
        const int count = reader->getInt32();
        if (!isValidCount(count, static_cast<std::int32_t>(kMaxClipVertices)))
            return false;
        m_inverseClipVertexCount = static_cast<std::uint32_t>(count);
        m_inverseClipCountSeen = true;
        if (!DRW::reserve(inverseClipBoundary, count))
            return false;
        break;
    }
    case 12: {
        if (inverseClipBoundary.size() >= kMaxClipVertices
            || (m_inverseClipCountSeen
                && inverseClipBoundary.size() >= m_inverseClipVertexCount))
            return false;
        ++m_currentInverseClipVertexIdx;
        if (m_currentInverseClipVertexIdx
            >= static_cast<int>(inverseClipBoundary.size())) {
            if (!DRW::resize(inverseClipBoundary,
                             m_currentInverseClipVertexIdx + 1))
                return false;
        }
        inverseClipBoundary[m_currentInverseClipVertexIdx].x = reader->getDouble();
        break;
    }
    case 22:
        if (m_currentInverseClipVertexIdx >= 0
            && m_currentInverseClipVertexIdx
                   < static_cast<int>(inverseClipBoundary.size())) {
            inverseClipBoundary[m_currentInverseClipVertexIdx].y = reader->getDouble();
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_Underlay::resetDwgState() {
    DRW_Entity::reset();
    // Entity carriers can be reused by callers. Clip payloads and their
    // parser counters belong to this record, not to the carrier lifetime.
    position = DRW_Coord{0.0, 0.0, 0.0};
    scale = DRW_Coord{1.0, 1.0, 1.0};
    rotation = 0.0;
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
    flags = 2;
    contrast = 100;
    fade = 0;
    clipBoundary.clear();
    inverseClipBoundary.clear();
    m_currentClipVertexIdx = -1;
    m_currentInverseClipVertexIdx = -1;
    m_inverseClipVertexCount = 0;
    m_inverseClipCountSeen = false;
    definitionHandle = 0;
}

bool DRW_Underlay::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto finiteClipPoint = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y);
    };
    return finite(position) && finite(scale) && finite(extPoint)
        && std::isfinite(rotation)
        && std::all_of(clipBoundary.cbegin(), clipBoundary.cend(),
                       finiteClipPoint)
        && std::all_of(inverseClipBoundary.cbegin(),
                       inverseClipBoundary.cend(), finiteClipPoint);
}

bool DRW_Underlay::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    if (!DRW_Entity::parseDwgCommon(version, buf, bs)) return fail();
    DRW_DBG("\n***************************** parsing UNDERLAY ***************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedExtPoint;
    DRW_Coord parsedPosition;
    DRW_Coord parsedScale;
    double parsedRotation = 0.0;
    std::uint8_t parsedFlags = 0;
    std::uint8_t parsedContrast = 0;
    std::uint8_t parsedFade = 0;
    if (!readBoundedExtrusion(*buf, bodyEndBit, false, parsedExtPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedPosition)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedRotation)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedScale.x)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedScale.y)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedScale.z)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedFlags)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedContrast)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedFade))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedExtPoint) || !finite(parsedPosition)
        || !finite(parsedScale) || !std::isfinite(parsedRotation))
        return fail();
    std::uint32_t nClip = 0;
    DRW_DBG(" UNDERLAY pos: "); DRW_DBG(parsedPosition.x); DRW_DBG(",");
    DRW_DBG(parsedPosition.y); DRW_DBG(" rot: "); DRW_DBG(parsedRotation);
    DRW_DBG(" flags: "); DRW_DBGH(parsedFlags); DRW_DBG("\n");
    if (!readTableBodyCount(version, buf, objSize, kMaxClipVertices, 128,
                            nClip)
        || !proxyEntityHasBits(*buf, bodyEndBit,
                               static_cast<std::uint64_t>(nClip) * 128u))
        return fail();
    std::vector<DRW_Coord> parsedClipBoundary;
    if (!DRW::reserve(parsedClipBoundary, static_cast<std::size_t>(nClip)))
        return fail();
    for (std::uint32_t i = 0; i < nClip; ++i) {
        DRW_Coord p;
        if (!readBoundedRawDouble(*buf, bodyEndBit, p.x)
            || !readBoundedRawDouble(*buf, bodyEndBit, p.y)
            || !std::isfinite(p.x) || !std::isfinite(p.y))
            return fail();
        p.z = 0.0;
        parsedClipBoundary.push_back(p);
    }
    std::vector<DRW_Coord> parsedInverseClipBoundary;
    if (version > DRW::AC1021 && (parsedFlags & 0x10) != 0) {
        std::uint16_t nInverse = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, nInverse)
            || nInverse > kMaxClipVertices
            || !proxyEntityHasBits(
                *buf, bodyEndBit,
                static_cast<std::uint64_t>(nInverse) * 128u)
            || !DRW::reserve(parsedInverseClipBoundary,
                             static_cast<std::size_t>(nInverse)))
            return fail();
        for (std::uint16_t i = 0; i < nInverse; ++i) {
            DRW_Coord p;
            if (!readBoundedRawDouble(*buf, bodyEndBit, p.x)
                || !readBoundedRawDouble(*buf, bodyEndBit, p.y)
                || !std::isfinite(p.x) || !std::isfinite(p.y))
                return fail();
            p.z = 0.0;
            parsedInverseClipBoundary.push_back(p);
        }
    }
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit))
        return fail();
    std::uint32_t parsedDefinitionHandle = 0;
    if (version > DRW::AC1014
        && proxyEntityHasBits(handleProbe, handleEndBit, 16)) {
        dwgHandle defH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                  defH))
            return fail();
        if (!handleProbe.isGood())
            return fail();
        parsedDefinitionHandle = defH.ref;
        DRW_DBG(" UNDERLAY definitionHandle: ");
        DRW_DBGHL(defH.code, defH.size, defH.ref); DRW_DBG("\n");
    }
    if (!handleProbe.isGood())
        return fail();
    *sourceBuf = handleProbe;
    extPoint = parsedExtPoint;
    position = parsedPosition;
    rotation = parsedRotation;
    scale = parsedScale;
    flags = parsedFlags;
    contrast = parsedContrast;
    fade = parsedFade;
    clipBoundary = std::move(parsedClipBoundary);
    inverseClipBoundary = std::move(parsedInverseClipBoundary);
    definitionHandle = parsedDefinitionHandle;
    return true;
}

bool DRW_Underlay::encodeDwg(DRW::Version version, dwgBufferW *buf,
                             std::uint32_t bs, dwgBufferW *strBuf,
                             dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (!validatePayloadFields()
        || clipBoundary.size() > kMaxClipVertices
        || inverseClipBoundary.size() > kMaxClipVertices
        || (version <= DRW::AC1021 && !inverseClipBoundary.empty()))
        return false;

    switch (kind) {
    case DGN:
        oType = kDwgClassNumDgn;
        break;
    case DWF:
        oType = kDwgClassNumDwf;
        break;
    case PDF:
    default:
        oType = kDwgClassNumPdf;
        break;
    }
    if (!encodeDwgCommon(version, buf))
        return false;

    const bool hasInverseClip = !inverseClipBoundary.empty() || (flags & 0x10) != 0;
    const std::uint8_t wireFlags =
        hasInverseClip ? static_cast<std::uint8_t>(flags | 0x10) : flags;

    buf->putExtrusion(extPoint, false);
    buf->put3BitDouble(position);
    buf->putBitDouble(rotation);
    buf->putBitDouble(scale.x);
    buf->putBitDouble(scale.y);
    buf->putBitDouble(scale.z);
    buf->putRawChar8(wireFlags);
    buf->putRawChar8(contrast);
    buf->putRawChar8(fade);
    buf->putBitLong(static_cast<std::int32_t>(clipBoundary.size()));
    for (const auto& vertex : clipBoundary) {
        buf->putRawDouble(vertex.x);
        buf->putRawDouble(vertex.y);
    }
    if (version > DRW::AC1021 && hasInverseClip) {
        buf->putBitShort(static_cast<std::uint16_t>(inverseClipBoundary.size()));
        for (const auto& vertex : inverseClipBoundary) {
            buf->putRawDouble(vertex.x);
            buf->putRawDouble(vertex.y);
        }
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;
    putNullableHardPointerHandle(handleBuf ? handleBuf : buf, definitionHandle);
    return true;
}


bool DRW_Text::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        widthscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 51:
        oblique = reader->getDouble();
        break;
    case 71:
        textgen = reader->getInt32();
        break;
    case 72:
        alignH = (HAlign)reader->getInt32();
        break;
    case 73:
        alignV = (VAlign)reader->getInt32();
        break;
    case 1:
        text = reader->getUtf8String();
        break;
    case 7:
        style = reader->getUtf8String();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

void DRW_Text::resetDwgState() {
    DRW_Entity::reset();
    basePoint = DRW_Coord{0.0, 0.0, 0.0};
    secPoint = DRW_Coord{0.0, 0.0, 0.0};
    thickness = 0.0;
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
    xAxisAngle = 0.0;
    height = 0.0;
    text.clear();
    angle = 0.0;
    widthscale = 1.0;
    oblique = 0.0;
    style = "STANDARD";
    textgen = 0;
    alignH = HLeft;
    alignV = VBaseLine;
    styleH = dwgHandle{};
}

bool DRW_Text::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing text *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;

 // DataFlags RC Used to determine presence of subsequent data, set to 0xFF for R14-
    std::uint8_t data_flags = 0x00;
    if (version > DRW::AC1014) {//2000+
        if (!readBoundedRawChar8(*buf, bodyEndBit, data_flags))
            return fail(); /* DataFlags RC Used to determine presence of subsequent data */
        DRW_DBG("data_flags: "); DRW_DBG(data_flags); DRW_DBG("\n");
        if ( !(data_flags & 0x01) ) { /* Elevation RD --- present if !(DataFlags & 0x01) */
            if (!readBoundedRawDouble(*buf, bodyEndBit, basePoint.z))
                return fail();
        }
    } else {//14-
        if (!readBoundedBitDouble(*buf, bodyEndBit, basePoint.z))
            return fail(); /* Elevation BD --- */
    }
    if (!readBoundedRawDouble(*buf, bodyEndBit, basePoint.x)
        || !readBoundedRawDouble(*buf, bodyEndBit, basePoint.y))
        return fail(); /* Insertion pt 2RD 10 */
    DRW_DBG("Insert point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x02) ) { /* Alignment pt 2DD 11 present if !(DataFlags & 0x02), use 10 & 20 values for 2 default values.*/
            if (!readBoundedDefaultDouble(*buf, bodyEndBit, basePoint.x,
                                          secPoint.x)
                || !readBoundedDefaultDouble(*buf, bodyEndBit, basePoint.y,
                                              secPoint.y))
                return fail();
        } else {
            secPoint = basePoint;
        }
    } else {//14-
        if (!readBoundedRawDouble(*buf, bodyEndBit, secPoint.x)
            || !readBoundedRawDouble(*buf, bodyEndBit, secPoint.y))
            return fail();  /* Alignment pt 2RD 11 */
    }
    secPoint.z = basePoint.z;
    DRW_DBG("Alignment: "); DRW_DBGPT(secPoint.x, secPoint.y, basePoint.z); DRW_DBG("\n");
    if (!readBoundedExtrusion(*buf, bodyEndBit, version > DRW::AC1014,
                              extPoint))
        return fail();
    DRW_DBG("Extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z); DRW_DBG("\n");
    if (!readBoundedThickness(*buf, bodyEndBit, version > DRW::AC1014,
                              thickness))
        return fail(); /* Thickness BD 39 */

    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x04) ) { /* Oblique ang RD 51 present if !(DataFlags & 0x04) */
            if (!readBoundedRawDouble(*buf, bodyEndBit, oblique))
                return fail();
        }
        if ( !(data_flags & 0x08) ) { /* Rotation ang RD 50 present if !(DataFlags & 0x08) */
            if (!readBoundedRawDouble(*buf, bodyEndBit, angle))
                return fail();
        }
        if (!readBoundedRawDouble(*buf, bodyEndBit, height))
            return fail(); /* Height RD 40 */
        if ( !(data_flags & 0x10) ) { /* Width factor RD 41 present if !(DataFlags & 0x10) */
            if (!readBoundedRawDouble(*buf, bodyEndBit, widthscale))
                return fail();
        }
    } else {//14-
        if (!readBoundedBitDouble(*buf, bodyEndBit, oblique)
            || !readBoundedBitDouble(*buf, bodyEndBit, angle)
            || !readBoundedBitDouble(*buf, bodyEndBit, height)
            || !readBoundedBitDouble(*buf, bodyEndBit, widthscale))
            return fail(); /* Oblique/rotation/height/width BD */
    }
    angle *= ARAD;
    if (!std::isfinite(basePoint.x) || !std::isfinite(basePoint.y)
        || !std::isfinite(basePoint.z) || !std::isfinite(secPoint.x)
        || !std::isfinite(secPoint.y) || !std::isfinite(secPoint.z)
        || !std::isfinite(extPoint.x) || !std::isfinite(extPoint.y)
        || !std::isfinite(extPoint.z) || !std::isfinite(thickness)
        || !std::isfinite(oblique) || !std::isfinite(angle)
        || !std::isfinite(height) || !std::isfinite(widthscale))
        return fail();
    DRW_DBG("thickness: "); DRW_DBG(thickness); DRW_DBG(", Oblique ang: "); DRW_DBG(oblique); DRW_DBG(", Width: ");
    DRW_DBG(widthscale); DRW_DBG(", Rotation: "); DRW_DBG(angle); DRW_DBG(", height: "); DRW_DBG(height); DRW_DBG("\n");
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    UTF8STRING parsedText;
    if (version > DRW::AC1018
        && currentDwgBit(sBuf) >= stringEndBit) {
        parsedText.clear();
    } else if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                        parsedText)) {
        return fail();
    }
    DRW_DBG("text string: "); DRW_DBG(parsedText.c_str());DRW_DBG("\n");
    //textgen, alignH, alignV always present in R14-, data_flags set in initialisation
    if ( !(data_flags & 0x20) ) { /* Generation BS 71 present if !(DataFlags & 0x20) */
        std::uint16_t parsedTextgen = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedTextgen))
            return fail();
        textgen = parsedTextgen;
        DRW_DBG("textgen: "); DRW_DBG(textgen);
    }
    if ( !(data_flags & 0x40) ) { /* Horiz align. BS 72 present if !(DataFlags & 0x40) */
        std::uint16_t parsedAlignH = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedAlignH))
            return fail();
        alignH = static_cast<HAlign>(parsedAlignH);
        DRW_DBG(", alignH: "); DRW_DBG(alignH);
    }
    if ( !(data_flags & 0x80) ) { /* Vert align. BS 73 present if !(DataFlags & 0x80) */
        std::uint16_t parsedAlignV = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedAlignV))
            return fail();
        alignV = static_cast<VAlign>(parsedAlignV);
        DRW_DBG(", alignV: "); DRW_DBG(alignV);
    }
    DRW_DBG("\n");

    if (!buf->isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    /* Common Entity Handle Data */
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();

    dwgHandle parsedStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              parsedStyleH)
        || !handleProbe.isGood() || !sBuf->isGood())
        return fail();

    *sourceBuf = handleProbe;
    text = std::move(parsedText);
    styleH = parsedStyleH;
    DRW_DBG("text style Handle: "); DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");
    /* CRC X --- */
    return true;
}

// ---------------------------------------------------------------------------
// RTEXT (RText, Express Tools) — read-only, mapped onto DRW_Text.
// ---------------------------------------------------------------------------
bool DRW_RText::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // RTEXT's DXF layout is a TEXT subset (1 text, 7 style, 10/20/30 insertion,
    // 40 height, 50 rotation deg, 210/220/230 extrusion) plus a flags long (70)
    // that plain TEXT does not carry.
    if (70 == code) {
        m_rTextFlags = reader->getInt32();
        return true;
    }
    return DRW_Text::parseCode(code, reader);
}

bool DRW_RText::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    m_rTextFlags = 0;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        m_rTextFlags = 0;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)   // 2007+ strings live in a separate stream
        sBuf = &sBuff;
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing rtext ********************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtrusion;
    double parsedAngle = 0.0;
    double parsedHeight = 0.0;
    std::uint16_t parsedFlags = 0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedExtrusion)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedAngle)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedHeight)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedFlags)
        || !std::isfinite(parsedBasePoint.x)
        || !std::isfinite(parsedBasePoint.y)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z)
        || !std::isfinite(parsedAngle)
        || !std::isfinite(parsedHeight))
        return fail();
    basePoint = parsedBasePoint;         // insertion 3BD
    secPoint = parsedBasePoint;          // no separate alignment point
    extPoint = parsedExtrusion;          // extrusion 3BD
    angle = parsedAngle * ARAD;          // rotation BD (radians) -> degrees
    height = parsedHeight;               // height BD
    m_rTextFlags = parsedFlags;          // flags BS
    if (!std::isfinite(angle))
        return fail();
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    UTF8STRING parsedText;
    if (version > DRW::AC1018
        && currentDwgBit(sBuf) >= stringEndBit) {
        parsedText.clear();
    } else if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                        parsedText)) {
        return fail();
    }
    DRW_DBG("rtext string: "); DRW_DBG(parsedText.c_str()); DRW_DBG("\n");

    if (!buf->isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();
    dwgHandle parsedStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              parsedStyleH)
        || !handleProbe.isGood() || !sBuf->isGood())
        return fail();

    *sourceBuf = handleProbe;
    text = std::move(parsedText);
    styleH = parsedStyleH;
    return true;
}

// ---------------------------------------------------------------------------
// ARCALIGNEDTEXT (AcDbArcAlignedText, Express Tools) — read-only, mapped onto
// DRW_Text as a 2D approximation (text at the arc mid-point, tangent baseline).
// ---------------------------------------------------------------------------
bool DRW_RText::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                          dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf)) return false;

    buf->put3BitDouble(basePoint);
    buf->put3BitDouble(extPoint);
    buf->putBitDouble(angle / ARAD);
    buf->putBitDouble(height);
    buf->putBitShort(bitShortFromInt(m_rTextFlags));
    (strBuf ? strBuf : buf)->putVariableText(version, text);

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putHardPointerHandle(handleBuf ? handleBuf : buf,
                         (styleH.ref == 0) ? 0x13 : styleH.ref);
    return true;
}

void DRW_ArcAlignedText::applyArcApproximation(){
    const double mid = 0.5 * (m_startAngle + m_endAngle);
    basePoint.x = m_center.x + m_radius * std::cos(mid);
    basePoint.y = m_center.y + m_radius * std::sin(mid);
    basePoint.z = m_center.z;
    secPoint = basePoint;
    // Baseline tangent to the arc at the mid-point; angle stored in degrees to
    // match DRW_Text (which the DWG path fills via `angle *= ARAD`).
    angle = (mid + M_PI_2) * ARAD;
    // Height from the text-size D2T string when parseable, else a fraction of
    // the radius so the approximation is at least visible.
    double h = 0.0;
    try { h = std::stod(m_textSize); } catch (...) { h = 0.0; }
    if (h > 0.0)
        height = h;
    else if (height <= 0.0)
        height = 0.1 * m_radius;
}

// Format a D2T (double-to-text) field the way the ARCALIGNEDTEXT model stores
// it: the DWG body carries these as text ("2.5", "1", "0"), while the DXF path
// reads them as doubles (group codes 41-46 fall in the double range, so the
// reader populates doubleData and leaves strData stale — getString() is unsafe
// here).  %g reproduces the same compact textual form.
static std::string arcAlignedD2T(double v){
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);
    return std::string(buf);
}

static std::string arcAlignedStringOrDefault(const UTF8STRING& value,
                                             const std::string& fallback) {
    return value.empty() ? fallback : value;
}

bool DRW_ArcAlignedText::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                   std::uint32_t bs, dwgBufferW *strBuf,
                                   dwgBufferW *handleBuf) {
    (void)bs;
    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf)) return false;

    dwgBufferW *sb = strBuf ? strBuf : buf;
    sb->putVariableText(version, arcAlignedStringOrDefault(
        m_textSize, arcAlignedD2T(height > 0.0 ? height : 0.0)));
    sb->putVariableText(version, arcAlignedStringOrDefault(
        m_xScale, arcAlignedD2T(widthscale > 0.0 ? widthscale : 1.0)));
    sb->putVariableText(version, arcAlignedStringOrDefault(m_charSpacing, "1"));
    sb->putVariableText(version, style.empty() ? "Standard" : style);
    sb->putVariableText(version, m_fontName);
    sb->putVariableText(version, m_bigFontName);
    sb->putVariableText(version, text);
    sb->putVariableText(version, arcAlignedStringOrDefault(m_offsetFromArc, "0"));
    sb->putVariableText(version, arcAlignedStringOrDefault(m_rightOffset, "0"));
    sb->putVariableText(version, arcAlignedStringOrDefault(m_leftOffset, "0"));

    buf->put3BitDouble(m_center);
    buf->putBitDouble(m_radius);
    buf->putBitDouble(m_startAngle);
    buf->putBitDouble(m_endAngle);
    buf->put3BitDouble(extPoint);
    buf->putBitLong(static_cast<std::int32_t>(m_rawColor));
    buf->putBitShort(bitShortFromInt(m_characterSet));
    buf->putBitShort(bitShortFromInt(m_pitchAndFamily));
    buf->putBitShort(bitShortFromInt(m_isShx));
    buf->putBitShort(bitShortFromInt(m_isBold));
    buf->putBitShort(bitShortFromInt(m_isItalic));
    buf->putBitShort(bitShortFromInt(m_isUnderlined));
    buf->putBitShort(bitShortFromInt(m_alignment));
    buf->putBitShort(bitShortFromInt(m_isReverse));
    buf->putBitShort(bitShortFromInt(m_wizardFlag));
    buf->putBitShort(bitShortFromInt(m_textPosition));
    buf->putBitShort(bitShortFromInt(m_textDirection));

    if (version <= DRW::AC1018)
        putNullableHardPointerHandle(buf, m_arcHandle);
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    if (version > DRW::AC1018)
        putNullableHardPointerHandle(handleBuf ? handleBuf : buf, m_arcHandle);
    return true;
}

bool DRW_ArcAlignedText::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // ARCALIGNEDTEXT repurposes several TEXT codes (2/10/40/41/50/51/70…), so it
    // must not delegate those to DRW_Text; unknown codes fall through to the
    // AcDbEntity common parser.  Angles (50/51) are DXF degrees -> radians.
    switch (code) {
    case 1:   text = reader->getUtf8String(); break;
    case 2:   m_fontName = reader->getUtf8String(); break;
    case 3:   m_bigFontName = reader->getUtf8String(); break;
    case 7:   style = reader->getUtf8String(); break;
    case 10:  m_center.x = reader->getDouble(); break;
    case 20:  m_center.y = reader->getDouble(); break;
    case 30:  m_center.z = reader->getDouble(); break;
    case 40:  m_radius = reader->getDouble(); break;
    case 41:  m_xScale = arcAlignedD2T(reader->getDouble()); break;
    case 42:  m_textSize = arcAlignedD2T(reader->getDouble()); break;
    case 43:  m_charSpacing = arcAlignedD2T(reader->getDouble()); break;
    case 44:  m_offsetFromArc = arcAlignedD2T(reader->getDouble()); break;
    case 45:  m_rightOffset = arcAlignedD2T(reader->getDouble()); break;
    case 46:  m_leftOffset = arcAlignedD2T(reader->getDouble()); break;
    case 50:  m_startAngle = reader->getDouble() / ARAD; break;
    case 51:  m_endAngle = reader->getDouble() / ARAD; break;
    case 70:  m_isReverse = reader->getInt32(); break;
    case 71:  m_textDirection = reader->getInt32(); break;
    case 72:  m_alignment = reader->getInt32(); break;
    case 73:  m_textPosition = reader->getInt32(); break;
    case 74:  m_isBold = reader->getInt32(); break;
    case 75:  m_isItalic = reader->getInt32(); break;
    case 76:  m_isUnderlined = reader->getInt32(); break;
    case 77:  m_characterSet = reader->getInt32(); break;
    case 78:  m_pitchAndFamily = reader->getInt32(); break;
    case 79:  m_isShx = reader->getInt32(); break;
    case 90:  m_rawColor = reader->getInt32(); break;
    case 210: extPoint.x = reader->getDouble(); break;
    case 220: extPoint.y = reader->getDouble(); break;
    case 230: extPoint.z = reader->getDouble(); break;
    case 280: m_wizardFlag = reader->getInt32(); break;
    default:  return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

void DRW_ArcAlignedText::resetDwgState() {
    DRW_Text::resetDwgState();
    m_center = DRW_Coord{0.0, 0.0, 0.0};
    m_radius = 0.0;
    m_startAngle = 0.0;
    m_endAngle = 0.0;
    m_textSize.clear();
    m_xScale.clear();
    m_charSpacing.clear();
    m_offsetFromArc.clear();
    m_rightOffset.clear();
    m_leftOffset.clear();
    m_fontName.clear();
    m_bigFontName.clear();
    m_rawColor = 0;
    m_characterSet = 0;
    m_pitchAndFamily = 0;
    m_isShx = 0;
    m_isBold = 0;
    m_isItalic = 0;
    m_isUnderlined = 0;
    m_alignment = 0;
    m_isReverse = 0;
    m_wizardFlag = 0;
    m_textPosition = 0;
    m_textDirection = 0;
    m_arcHandle = 0;
}

bool DRW_ArcAlignedText::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    // The arc reference is an optional trailer whose position depends on the
    // DWG version. Never carry it across a reused or truncated record.
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)   // 2007+ strings live in a separate stream
        sBuf = &sBuff;
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing arcalignedtext **********************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }

    UTF8STRING parsedTextSize;
    UTF8STRING parsedXScale;
    UTF8STRING parsedCharSpacing;
    UTF8STRING parsedStyle;
    UTF8STRING parsedFontName;
    UTF8STRING parsedBigFontName;
    UTF8STRING parsedText;
    UTF8STRING parsedOffsetFromArc;
    UTF8STRING parsedRightOffset;
    UTF8STRING parsedLeftOffset;
    if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                 parsedTextSize)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedXScale)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedCharSpacing)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedStyle)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedFontName)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedBigFontName)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedText)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedOffsetFromArc)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedRightOffset)
        || !readBoundedVariableText(*sBuf, stringEndBit, version,
                                    parsedLeftOffset))
        return fail();

    DRW_Coord parsedCenter;
    double parsedRadius = 0.0;
    double parsedStartAngle = 0.0;
    double parsedEndAngle = 0.0;
    DRW_Coord parsedExtPoint;
    std::int32_t parsedRawColor = 0;
    std::uint16_t parsedCharacterSet = 0;
    std::uint16_t parsedPitchAndFamily = 0;
    std::uint16_t parsedIsShx = 0;
    std::uint16_t parsedIsBold = 0;
    std::uint16_t parsedIsItalic = 0;
    std::uint16_t parsedIsUnderlined = 0;
    std::uint16_t parsedAlignment = 0;
    std::uint16_t parsedIsReverse = 0;
    std::uint16_t parsedWizardFlag = 0;
    std::uint16_t parsedTextPosition = 0;
    std::uint16_t parsedTextDirection = 0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedCenter)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedRadius)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedStartAngle)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedEndAngle)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedExtPoint)
        || !readBoundedBitLong(*buf, bodyEndBit, parsedRawColor)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedCharacterSet)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedPitchAndFamily)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedIsShx)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedIsBold)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedIsItalic)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedIsUnderlined)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedAlignment)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedIsReverse)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedWizardFlag)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedTextPosition)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedTextDirection))
        return fail();
    DRW_DBG("arcalignedtext string: "); DRW_DBG(parsedText.c_str());
    DRW_DBG("\n");

    if (!buf->isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit
        || !std::isfinite(parsedCenter.x) || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z) || !std::isfinite(parsedRadius)
        || parsedRadius < 0.0 || !std::isfinite(parsedStartAngle)
        || !std::isfinite(parsedEndAngle) || !std::isfinite(parsedExtPoint.x)
        || !std::isfinite(parsedExtPoint.y)
        || !std::isfinite(parsedExtPoint.z))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    // R2004- keeps the arc handle before the common handle stream; R2007+ after.
    dwgBuffer handleProbe = buf->forkIndependent();
    std::uint32_t parsedArcHandle = 0;
    if (version <= DRW::AC1018
        && proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle arcHandle;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  arcHandle))
            return fail();
        parsedArcHandle = arcHandle.ref;
    }

    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();
    if (version > DRW::AC1018
        && proxyEntityHasBits(handleProbe, handleEndBit, 8)) {
        dwgHandle arcHandle;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  arcHandle))
            return fail();
        parsedArcHandle = arcHandle.ref;
    }
    if (!handleProbe.isGood())
        return fail();

    *sourceBuf = handleProbe;
    m_textSize = std::move(parsedTextSize);
    m_xScale = std::move(parsedXScale);
    m_charSpacing = std::move(parsedCharSpacing);
    style = std::move(parsedStyle);
    m_fontName = std::move(parsedFontName);
    m_bigFontName = std::move(parsedBigFontName);
    text = std::move(parsedText);
    m_offsetFromArc = std::move(parsedOffsetFromArc);
    m_rightOffset = std::move(parsedRightOffset);
    m_leftOffset = std::move(parsedLeftOffset);
    m_center = parsedCenter;
    m_radius = parsedRadius;
    m_startAngle = parsedStartAngle;
    m_endAngle = parsedEndAngle;
    extPoint = parsedExtPoint;
    m_rawColor = parsedRawColor;
    m_characterSet = parsedCharacterSet;
    m_pitchAndFamily = parsedPitchAndFamily;
    m_isShx = parsedIsShx;
    m_isBold = parsedIsBold;
    m_isItalic = parsedIsItalic;
    m_isUnderlined = parsedIsUnderlined;
    m_alignment = parsedAlignment;
    m_isReverse = parsedIsReverse;
    m_wizardFlag = parsedWizardFlag;
    m_textPosition = parsedTextPosition;
    m_textDirection = parsedTextDirection;
    m_arcHandle = parsedArcHandle;
    applyArcApproximation();
    return true;
}

// Out-of-line special members: required because mtext is a unique_ptr<DRW_MText>
// declared with a forward-declared element type in the header.
DRW_Attrib::~DRW_Attrib() = default;
DRW_Attrib::DRW_Attrib(const DRW_Attrib& o)
    : DRW_Text(o), tag(o.tag), attribFlags(o.attribFlags),
      m_fieldLength(o.m_fieldLength),
      lockPosition(o.lockPosition), attVersion(o.attVersion),
      m_attributeType(o.m_attributeType),
      keepDuplicateRecords(o.keepDuplicateRecords),
      mtext(o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr) {}
DRW_Attrib& DRW_Attrib::operator=(const DRW_Attrib& o) {
    if (this != &o) {
        DRW_Text::operator=(o);
        tag = o.tag;
        attribFlags = o.attribFlags;
        m_fieldLength = o.m_fieldLength;
        lockPosition = o.lockPosition;
        attVersion = o.attVersion;
        m_attributeType = o.m_attributeType;
        keepDuplicateRecords = o.keepDuplicateRecords;
        mtext = o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr;
    }
    return *this;
}
DRW_Attrib::DRW_Attrib(DRW_Attrib&&) noexcept = default;
DRW_Attrib& DRW_Attrib::operator=(DRW_Attrib&&) noexcept = default;

DRW_GeoPositionMarker::~DRW_GeoPositionMarker() = default;
DRW_GeoPositionMarker::DRW_GeoPositionMarker(
    const DRW_GeoPositionMarker& o)
    : DRW_Entity(o), m_classVersion(o.m_classVersion),
      m_position(o.m_position), m_radius(o.m_radius), m_notes(o.m_notes),
      m_landingGap(o.m_landingGap), m_mtextVisible(o.m_mtextVisible),
      m_textAlignment(o.m_textAlignment),
      m_enableFrameText(o.m_enableFrameText),
      mtext(o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr),
      m_dxfDouble40Count(o.m_dxfDouble40Count),
      m_dxfBool290Count(o.m_dxfBool290Count) {}
DRW_GeoPositionMarker& DRW_GeoPositionMarker::operator=(
    const DRW_GeoPositionMarker& o) {
    if (this != &o) {
        DRW_Entity::operator=(o);
        m_classVersion = o.m_classVersion;
        m_position = o.m_position;
        m_radius = o.m_radius;
        m_notes = o.m_notes;
        m_landingGap = o.m_landingGap;
        m_mtextVisible = o.m_mtextVisible;
        m_textAlignment = o.m_textAlignment;
        m_enableFrameText = o.m_enableFrameText;
        mtext = o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr;
        m_dxfDouble40Count = o.m_dxfDouble40Count;
        m_dxfBool290Count = o.m_dxfBool290Count;
    }
    return *this;
}
DRW_GeoPositionMarker::DRW_GeoPositionMarker(
    DRW_GeoPositionMarker&&) noexcept = default;
DRW_GeoPositionMarker& DRW_GeoPositionMarker::operator=(
    DRW_GeoPositionMarker&&) noexcept = default;

namespace {
static bool parseEmbeddedMTextEntityMode(DRW::Version version, dwgBuffer *buf,
                                         std::uint64_t bodyEndBit,
                                         EmbeddedMTextHandleInfo& info,
                                         DRW_MText& mtext) {
    std::uint8_t entmode = 0;
    if (!readBounded2Bits(*buf, bodyEndBit, entmode))
        return false;
    info.m_ownerHandle = entmode == 0;
    if (!readBoundedBitLong(*buf, bodyEndBit, info.m_numReactors)
        || !dwgSafety::validReactorCount(info.m_numReactors))
        return false;
    if (version > DRW::AC1015) {
        bool xDictFlag = false;
        if (!readBoundedBit(*buf, bodyEndBit, xDictFlag))
            return false;
        info.m_xDictFlag = xDictFlag ? 1 : 0;
    }
    if (version > DRW::AC1024) {
        bool hasDataStorage = false;
        if (!readBoundedBit(*buf, bodyEndBit, hasDataStorage))
            return false;
        mtext.setHasDataStorageBinaryData(hasDataStorage);
    } else if (version < DRW::AC1018) {
        bool noLinks = false;
        if (!readBoundedBit(*buf, bodyEndBit, noLinks))
            return false; // nolinks / have-next-links
    }
    std::uint32_t color = 0;
    if (!readBoundedEnColor(*buf, bodyEndBit, version, color))
        return false;
    mtext.color = static_cast<int>(color);
    info.m_hasAcDbColorHandle = buf->lastEnColorHadDbColorRef;
    mtext.color24 = buf->lastEnColorRgb;
    if (!buf->lastEnColorName.empty()) {
        mtext.colorName = buf->lastEnColorBookName.empty()
            ? buf->lastEnColorName
            : (buf->lastEnColorBookName + "$" + buf->lastEnColorName);
    }
    if (buf->lastEnColorAlphaRaw != 0)
        mtext.transparency = static_cast<int>(buf->lastEnColorAlphaRaw);
    if (!readBoundedBitDouble(*buf, bodyEndBit, mtext.ltypeScale))
        return false;
    if (version > DRW::AC1014) {
        std::uint8_t ltFlags = 0;
        std::uint8_t plotFlags = 0;
        if (!readBounded2Bits(*buf, bodyEndBit, ltFlags)
            || !readBounded2Bits(*buf, bodyEndBit, plotFlags))
            return false;
        info.m_ltFlags = ltFlags;
        info.m_plotFlags = plotFlags;
        switch (info.m_ltFlags) {
        case 0:
            mtext.lineType = "BYLAYER";
            break;
        case 1:
            mtext.lineType = "BYBLOCK";
            break;
        case 2:
            mtext.lineType = "CONTINUOUS";
            break;
        default:
            mtext.lineType.clear();
            break;
        }
    }
    if (version > DRW::AC1018) {
        std::uint8_t materialFlag = 0;
        std::uint8_t shadowFlag = 0;
        if (!readBounded2Bits(*buf, bodyEndBit, materialFlag)
            || !readBoundedRawChar8(*buf, bodyEndBit, shadowFlag))
            return false;
        info.m_materialFlag = materialFlag;
        info.m_shadowFlag = shadowFlag;
        mtext.shadow = static_cast<DRW::ShadowMode>(info.m_shadowFlag & 0x3);
    }
    if (version > DRW::AC1021) {
        if (!readBoundedBit(*buf, bodyEndBit, info.m_hasFullVisualStyle)
            || !readBoundedBit(*buf, bodyEndBit, info.m_hasFaceVisualStyle)
            || !readBoundedBit(*buf, bodyEndBit, info.m_hasEdgeVisualStyle))
            return false;
    }
    std::uint16_t invisibleFlag = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, invisibleFlag))
        return false;
    mtext.visible = (invisibleFlag & 1) == 0;
    if (version > DRW::AC1014) {
        std::uint8_t lineWeight = 0;
        if (!readBoundedRawChar8(*buf, bodyEndBit, lineWeight))
            return false;
        mtext.lWeight = DRW_LW_Conv::dwgInt2lineWidth(lineWeight);
    }
    return buf->isGood() && currentDwgBit(buf) <= bodyEndBit;
}

static bool parseEmbeddedMTextDwgFields(
    DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf,
    DRW_MText& mtext, EmbeddedMTextHandleInfo& info,
    std::uint64_t bodyEndBit, std::uint64_t stringEndBit) {
    if (!parseEmbeddedMTextEntityMode(version, buf, bodyEndBit, info, mtext))
        return false;

    if (!readBoundedBitCoord(*buf, bodyEndBit, mtext.basePoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, mtext.extPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, mtext.secPoint))
        return false;
    mtext.angle = atan2(mtext.secPoint.y, mtext.secPoint.x) * ARAD;
    if (!readBoundedBitDouble(*buf, bodyEndBit, mtext.widthscale))
        return false;
    if (version > DRW::AC1018) {
        double ignoredRectangleHeight = 0.0;
        if (!readBoundedBitDouble(*buf, bodyEndBit,
                                  ignoredRectangleHeight))
            return false;
    }
    if (!readBoundedBitDouble(*buf, bodyEndBit, mtext.height))
        return false;
    std::uint16_t textgen = 0;
    std::uint16_t alignH = 0;
    double ignoredExtentsHeight = 0.0;
    double ignoredExtentsWidth = 0.0;
    if (!readBoundedBitShort(*buf, bodyEndBit, textgen)
        || !readBoundedBitShort(*buf, bodyEndBit, alignH)
        || !readBoundedBitDouble(*buf, bodyEndBit, ignoredExtentsHeight)
        || !readBoundedBitDouble(*buf, bodyEndBit, ignoredExtentsWidth))
        return false;
    mtext.textgen = textgen;
    mtext.alignH = static_cast<DRW_Text::HAlign>(alignH);
    if (!readBoundedVariableText(*sBuf, stringEndBit, version, mtext.text))
        return false;

    if (version > DRW::AC1014) {
        std::uint16_t ignoredLineSpacingStyle = 0;
        bool ignoredLineSpacingUnknown = false;
        if (!readBoundedBitShort(*buf, bodyEndBit,
                                 ignoredLineSpacingStyle)
            || !readBoundedBitDouble(*buf, bodyEndBit, mtext.interlin)
            || !readBoundedBit(*buf, bodyEndBit,
                               ignoredLineSpacingUnknown))
            return false;
    }
    if (version > DRW::AC1015) {
        if (!readBoundedBitLong(*buf, bodyEndBit, mtext.m_backgroundFlags))
            return false;
        if ((mtext.m_backgroundFlags & 0x01)
            || (version >= DRW::AC1032 && (mtext.m_backgroundFlags & 0x10))) {
            std::uint32_t backgroundColor = 0;
            if (!readBoundedBitDouble(*buf, bodyEndBit,
                                      mtext.m_backgroundScale)
                || !readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                                       backgroundColor, nullptr, nullptr,
                                       nullptr, nullptr, stringEndBit)
                || !readBoundedBitLong(*buf, bodyEndBit,
                                       mtext.m_backgroundTransparency))
                return false;
            mtext.m_backgroundColor = static_cast<int>(backgroundColor);
        }
    }

    if (version >= DRW::AC1032) {
        mtext.m_r2018ColumnHeights.clear();
        if (!readBoundedBit(*buf, bodyEndBit,
                            mtext.m_r2018IsNotAnnotative)
            || !readBoundedBit(*buf, bodyEndBit,
                               mtext.m_r2018ReallyLocked))
            return false;
        info.m_hasR2018AppIdHandle = mtext.m_r2018IsNotAnnotative;
    }

    std::uint16_t annotativeSize = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, annotativeSize))
        return false;
    if (annotativeSize > 0) {
        std::vector<std::uint8_t> annotativeData;
        if (!DRW::reserve(annotativeData, annotativeSize))
            return false;
        for (std::uint16_t i = 0; i < annotativeSize; ++i) {
            std::uint8_t value = 0;
            if (!readBoundedRawChar8(*buf, bodyEndBit, value))
                return false;
            annotativeData.push_back(value);
        }
        mtext.m_r2018AnnotativeData = std::move(annotativeData);
        info.m_hasAnnotativeAppHandle = true;
        if (!readBoundedBitShort(*buf, bodyEndBit,
                                 mtext.m_r2018AnnotativeUnknown))
            return false;
    }
    return buf->isGood() && sBuf->isGood()
        && currentDwgBit(buf) <= bodyEndBit
        && currentDwgBit(sBuf) <= stringEndBit;
}

static bool parseEmbeddedMTextDwg(
    DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf,
    DRW_MText& mtext, EmbeddedMTextHandleInfo& info,
    std::uint64_t bodyEndBit, std::uint64_t stringEndBit) {
    if (buf == nullptr || sBuf == nullptr || !buf->isGood()
        || !sBuf->isGood())
        return false;

    const std::uint64_t bodyStartBit = currentDwgBit(buf);
    const std::uint64_t stringStartBit = currentDwgBit(sBuf);
    const auto totalBits = [](const dwgBuffer& buffer) {
        std::uint64_t bits = 0;
        return dwgSafety::multiply(buffer.size(), 8, bits) ? bits : 0;
    };
    if (bodyEndBit == std::numeric_limits<std::uint64_t>::max())
        bodyEndBit = totalBits(*buf);
    if (stringEndBit == std::numeric_limits<std::uint64_t>::max())
        stringEndBit = totalBits(*sBuf);
    if (bodyStartBit > bodyEndBit || stringStartBit > stringEndBit)
        return false;

    dwgBuffer bodyProbe = buf->forkIndependent();
    std::unique_ptr<dwgBuffer> separateStringProbe;
    dwgBuffer *stringProbe = &bodyProbe;
    if (sBuf != buf) {
        separateStringProbe = std::make_unique<dwgBuffer>(
            sBuf->forkIndependent());
        stringProbe = separateStringProbe.get();
    }
    if (!parseEmbeddedMTextDwgFields(version, &bodyProbe, stringProbe,
                                     mtext, info, bodyEndBit,
                                     stringEndBit))
        return false;

    const std::uint64_t bodyConsumed = currentDwgBit(&bodyProbe);
    const std::uint64_t stringConsumed = currentDwgBit(stringProbe);
    if (bodyConsumed < bodyStartBit || stringConsumed < stringStartBit
        || bodyConsumed > bodyEndBit || stringConsumed > stringEndBit)
        return false;

    const auto seekBits = [](dwgBuffer& buffer, std::uint64_t bit) {
        if (!buffer.setPosition(bit >> 3))
            return false;
        buffer.setBitPos(static_cast<std::uint8_t>(bit & 7u));
        return buffer.isGood() && currentDwgBit(&buffer) == bit;
    };
    if (!seekBits(*buf, bodyConsumed))
        return false;
    if (sBuf != buf && !seekBits(*sBuf, stringConsumed))
        return false;
    return true;
}

static bool consumeEmbeddedMTextHandles(
    DRW::Version version, dwgBuffer *buf,
    const EmbeddedMTextHandleInfo& info, DRW_MText *mtext,
    std::uint64_t handleEndBit) {
    if (buf == nullptr || !buf->isGood())
        return false;

    if (handleEndBit == std::numeric_limits<std::uint64_t>::max()) {
        if (!dwgSafety::multiply(buf->size(), 8, handleEndBit))
            return false;
    }
    dwgBuffer probe = buf->forkIndependent();
    auto readHandle = [&](bool offset, dwgHandle& value) {
        return readBoundedDwgHandle(probe, handleEndBit, 0, offset, value);
    };

    dwgHandle owner;
    if (info.m_ownerHandle && !readHandle(false, owner))
        return false;

    std::vector<std::uint32_t> reactors;
    if (info.m_numReactors < 0
        || !dwgSafety::validReactorCount(info.m_numReactors))
        return false;
    if (!DRW::reserve(reactors, info.m_numReactors))
        return false;
    for (int i = 0; i < info.m_numReactors; ++i) {
        dwgHandle reactor;
        if (!readHandle(false, reactor))
            return false;
        if (reactor.ref != 0)
            reactors.push_back(reactor.ref);
    }

    dwgHandle xDict;
    const bool hasXDict = version <= DRW::AC1015 || info.m_xDictFlag != 1;
    if (hasXDict && !readHandle(false, xDict))
        return false;

    dwgHandle color;
    if (info.m_hasAcDbColorHandle && !readHandle(false, color))
        return false;

    dwgHandle layer;
    const bool hasLayer = version > DRW::AC1014;
    if (hasLayer && !readHandle(false, layer))
        return false;

    dwgHandle lineType;
    const bool hasLineType = version > DRW::AC1014 && info.m_ltFlags == 3;
    if (hasLineType && !readHandle(false, lineType))
        return false;

    dwgHandle material;
    const bool hasMaterial = version > DRW::AC1018 && info.m_materialFlag == 3;
    if (hasMaterial && !readHandle(false, material))
        return false;

    dwgHandle shadow;
    const bool hasShadow = version > DRW::AC1018 && info.m_shadowFlag == 3;
    if (hasShadow && !readHandle(false, shadow))
        return false;

    dwgHandle plotStyle;
    const bool hasPlotStyle = info.m_plotFlags == 3;
    if (hasPlotStyle && !readHandle(false, plotStyle))
        return false;

    dwgHandle fullVisualStyle;
    const bool hasFullVisualStyle = version > DRW::AC1021
        && info.m_hasFullVisualStyle;
    if (hasFullVisualStyle && !readHandle(false, fullVisualStyle))
        return false;

    dwgHandle faceVisualStyle;
    const bool hasFaceVisualStyle = version > DRW::AC1021
        && info.m_hasFaceVisualStyle;
    if (hasFaceVisualStyle && !readHandle(false, faceVisualStyle))
        return false;

    dwgHandle edgeVisualStyle;
    const bool hasEdgeVisualStyle = version > DRW::AC1021
        && info.m_hasEdgeVisualStyle;
    if (hasEdgeVisualStyle && !readHandle(false, edgeVisualStyle))
        return false;

    dwgHandle style;
    if (info.m_hasStyleHandle && !readHandle(false, style))
        return false;

    dwgHandle appId;
    if (info.m_hasR2018AppIdHandle && !readHandle(false, appId))
        return false;

    dwgHandle annotativeApp;
    if (info.m_hasAnnotativeAppHandle && !readHandle(false, annotativeApp))
        return false;

    if (mtext != nullptr) {
        if (info.m_ownerHandle)
            mtext->parentHandle = owner.ref;
        mtext->reactorHandles = std::move(reactors);
        if (hasXDict)
            mtext->xDictHandle = xDict.ref;
        if (info.m_hasAcDbColorHandle)
            mtext->setDwgAcDbColorHandle(true, color.ref);
        if (hasLayer)
            mtext->setDwgLayerHandle(layer.ref);
        if (hasLineType)
            mtext->setDwgLinetypeHandle(lineType.ref);
        if (hasMaterial)
            mtext->material = material.ref;
        if (hasShadow)
            mtext->shadowHandle = shadow.ref;
        if (hasPlotStyle)
            mtext->plotStyle = static_cast<int>(plotStyle.ref);
        if (hasFullVisualStyle)
            mtext->fullVisualStyleHandle = fullVisualStyle.ref;
        if (hasFaceVisualStyle)
            mtext->faceVisualStyleHandle = faceVisualStyle.ref;
        if (hasEdgeVisualStyle)
            mtext->edgeVisualStyleHandle = edgeVisualStyle.ref;
        if (info.m_hasStyleHandle)
            mtext->styleH = style;
        if (info.m_hasR2018AppIdHandle)
            mtext->m_r2018AppIdHandle = appId.ref;
        if (info.m_hasAnnotativeAppHandle)
            mtext->m_r2018AnnotativeAppHandle = annotativeApp.ref;
    }
    *buf = probe;
    return true;
}

static bool encodeEmbeddedMTextEntityMode(DRW::Version version, dwgBufferW *buf,
                                          const DRW_MText& mtext) {
    if (version < DRW::AC1032)
        return false;

    // Embedded MTEXT begins at AcDbEntity mode, not with an object type,
    // object size, own handle, EED, or graphics data.
    if (mtext.reactorHandles.size()
        > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        return false;
    buf->put2Bits(mtext.parentHandle == DRW::NoHandle ? 2 : 0);
    buf->putBitLong(static_cast<std::int32_t>(mtext.reactorHandles.size()));
    buf->putBit(mtext.xDictHandle == 0 ? 1 : 0);
    buf->putBit(mtext.hasDataStorageBinaryData() ? 1 : 0);
    UTF8STRING colorName = mtext.colorName;
    UTF8STRING bookName;
    const std::string::size_type separator = colorName.find('$');
    if (separator != std::string::npos) {
        bookName = colorName.substr(0, separator);
        colorName.erase(0, separator + 1);
    }
    buf->putEnColor(version, static_cast<std::uint16_t>(mtext.color),
                    mtext.color24, colorName, bookName,
                    static_cast<std::uint32_t>(mtext.transparency),
                    mtext.hasDwgAcDbColorHandle());
    buf->putBitDouble(mtext.ltypeScale);
    std::uint8_t ltFlags = 0;
    if (mtext.dwgLinetypeHandle() != 0 || mtext.lineType.empty())
        ltFlags = 3;
    else if (mtext.lineType == "BYBLOCK")
        ltFlags = 1;
    else if (mtext.lineType == "CONTINUOUS")
        ltFlags = 2;
    buf->put2Bits(ltFlags);
    buf->put2Bits(mtext.plotStyle == DRW::DefaultPlotStyle ? 0 : 3);
    buf->put2Bits(mtext.material == DRW::MaterialByLayer ? 0 : 3);
    buf->putRawChar8(static_cast<std::uint8_t>(mtext.shadow));
    buf->putBit(mtext.fullVisualStyleHandle != 0);
    buf->putBit(mtext.faceVisualStyleHandle != 0);
    buf->putBit(mtext.edgeVisualStyleHandle != 0);
    buf->putBitShort(mtext.visible ? 0 : 1);
    buf->putRawChar8(static_cast<std::uint8_t>(mtext.lWeight));
    return true;
}

static bool encodeEmbeddedMTextDwg(DRW::Version version, dwgBufferW *buf,
                                   dwgBufferW *strBuf, dwgBufferW *handleBuf,
                                   const DRW_MText& mtext) {
    if (!encodeEmbeddedMTextEntityMode(version, buf, mtext))
        return false;

    buf->put3BitDouble(mtext.basePoint);
    buf->put3BitDouble(mtext.extPoint);
    buf->put3BitDouble(mtext.secPoint);
    buf->putBitDouble(mtext.widthscale);
    buf->putBitDouble(mtext.m_r2018RectHeight);
    buf->putBitDouble(mtext.height);
    buf->putBitShort(static_cast<std::uint16_t>(mtext.textgen));
    buf->putBitShort(static_cast<std::uint16_t>(mtext.alignH));
    buf->putBitDouble(mtext.m_r2018ExtentsHeight);
    buf->putBitDouble(mtext.m_r2018ExtentsWidth);
    (strBuf ? strBuf : buf)->putVariableText(version, mtext.text);

    buf->putBitShort(0);                    // linespacing style
    buf->putBitDouble(mtext.interlin);
    buf->putBit(0);
    buf->putBitLong(mtext.m_backgroundFlags);
    if ((mtext.m_backgroundFlags & 0x01)
        || (mtext.m_backgroundFlags & 0x10)) {
        buf->putBitDouble(mtext.m_backgroundScale);  // BitDouble, not BitLong
        buf->putCmColor(version, static_cast<std::uint16_t>(mtext.m_backgroundColor));
        buf->putBitLong(mtext.m_backgroundTransparency);
    }

    buf->putBit(mtext.m_r2018IsNotAnnotative ? 1 : 0);
    buf->putBit(mtext.m_r2018ReallyLocked ? 1 : 0);

    if (mtext.m_r2018AnnotativeData.size() > std::numeric_limits<std::uint16_t>::max())
        return false;
    buf->putBitShort(static_cast<std::uint16_t>(mtext.m_r2018AnnotativeData.size()));
    for (std::uint8_t byte : mtext.m_r2018AnnotativeData)
        buf->putRawChar8(byte);
    if (!mtext.m_r2018AnnotativeData.empty()) {
        buf->putBitShort(mtext.m_r2018AnnotativeUnknown);
    }

    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    if (mtext.parentHandle != DRW::NoHandle)
        putHardPointerHandle(hb, mtext.parentHandle);
    for (const std::uint32_t reactor : mtext.reactorHandles)
        putHardPointerHandle(hb, reactor);
    if (mtext.xDictHandle != 0)
        putHardPointerHandle(hb, mtext.xDictHandle);
    if (mtext.hasDwgAcDbColorHandle())
        putHardPointerHandle(hb, mtext.dwgAcDbColorHandle());
    putHardPointerHandle(hb, mtext.dwgLayerHandle() == 0
                              ? 0x12 : mtext.dwgLayerHandle());
    if (mtext.dwgLinetypeHandle() != 0 || mtext.lineType.empty())
        putHardPointerHandle(hb, mtext.dwgLinetypeHandle());
    if (mtext.material != DRW::MaterialByLayer)
        putHardPointerHandle(hb, mtext.material);
    if (mtext.shadow == DRW::IgnoreShadows)
        putHardPointerHandle(hb, mtext.shadowHandle);
    if (mtext.plotStyle != DRW::DefaultPlotStyle)
        putHardPointerHandle(hb, static_cast<std::uint32_t>(mtext.plotStyle));
    if (mtext.fullVisualStyleHandle != 0)
        putHardPointerHandle(hb, mtext.fullVisualStyleHandle);
    if (mtext.faceVisualStyleHandle != 0)
        putHardPointerHandle(hb, mtext.faceVisualStyleHandle);
    if (mtext.edgeVisualStyleHandle != 0)
        putHardPointerHandle(hb, mtext.edgeVisualStyleHandle);
    putHardPointerHandle(hb, (mtext.styleH.ref == 0) ? 0x13 : mtext.styleH.ref);
    if (mtext.m_r2018IsNotAnnotative)
        putHardPointerHandle(hb, (mtext.m_r2018AppIdHandle == 0) ? 0x14 : mtext.m_r2018AppIdHandle);
    if (!mtext.m_r2018AnnotativeData.empty())
        putHardPointerHandle(hb, (mtext.m_r2018AnnotativeAppHandle == 0)
                                  ? 0x14 : mtext.m_r2018AnnotativeAppHandle);
    return true;
}
}

bool DRW_GeoPositionMarker::encodeDwg(DRW::Version v, dwgBufferW *buf,
                                      std::uint32_t bs, dwgBufferW *strBuf,
                                      dwgBufferW *handleBuf) {
    (void)bs;
    if (v < DRW::AC1027 || buf == nullptr)
        return false;
    // The embedded AcDbMTextObjectEmbedded wire form was introduced with
    // AC1032. Reject an AC1027 framed marker rather than emitting a body that
    // a reader will interpret as the parent handle stream.
    if (m_enableFrameText && (v < DRW::AC1032 || mtext == nullptr))
        return false;

    oType = kDwgType;
    if (!encodeDwgCommon(v, buf, strBuf))
        return false;

    dwgBufferW *stringBuffer = strBuf ? strBuf : buf;
    buf->putBitLong(m_classVersion);
    buf->put3BitDouble(m_position);
    buf->putBitDouble(m_radius);
    stringBuffer->putVariableText(v, m_notes);
    buf->putBitDouble(m_landingGap);
    buf->putBit(m_mtextVisible ? 1 : 0);
    buf->putRawChar8(m_textAlignment);
    buf->putBit(m_enableFrameText ? 1 : 0);

    dwgBufferW embeddedHandles;
    if (m_enableFrameText
        && !encodeEmbeddedMTextDwg(v, buf, strBuf, &embeddedHandles, *mtext))
        return false;
    if (!encodeDwgEntHandle(v, buf, handleBuf))
        return false;

    dwgBufferW *resolvedHandles = handleBuf ? handleBuf : buf;
    appendBitBuffer(resolvedHandles, embeddedHandles);
    return true;
}

bool DRW_Attrib::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // Multi-line ATTRIB (R2018+, ODA spec §20.4.4): an embedded MTEXT object
    // is introduced by the DXF subclass marker `100 / Embedded Object` (NOT
    // `AcDbMText`).  After the marker, the standard MTEXT group codes follow
    // (10/20/30 insertion, 11/21/31 X-axis, 40 height, 41 rect width, 71
    // attachment point, 72 drawing direction, 1 formatted text, etc.), then
    // the ATTRIB-specific tail (tag=2, prompt=3 for ATTDEF, flags=70,
    // lock-position=280) which we must NOT route into the embedded MText.
    if (code == 100) {
        const std::string sub = reader->getString();
        if (sub == "Embedded Object" && !mtext) {
            mtext = std::make_unique<DRW_MText>();
            if (attVersion == 0) attVersion = 1;
        }
        dxfInAttributeSubclass = sub == "AcDbAttribute"
            || sub == "AcDbAttributeDefinition";
        return true;
    }
    if (dxfInAttributeSubclass && !mtext) {
        if (code == 71) {
            const int value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint8_t>::max())
                return false;
            m_attributeType = static_cast<std::uint8_t>(value);
            return true;
        }
        if (code == 72) {
            (void)reader->getInt32();
            return true;
        }
    }
    // Inside the embedded MText scope, route MTEXT-owned codes to mtext but
    // keep ATTRIB-specific tail codes for ATTRIB / ATTDEF handling below.
    if (mtext) {
        switch (code) {
        case 2:    // tag (ATTRIB-specific; group 1 in MText is text body)
        case 3:    // prompt (ATTDEF-specific)
        case 70:   // ATTRIB flags
        case 280:  // ATTRIB lock-position
            break; // fall through to ATTRIB handling below
        default:
            return mtext->parseCode(code, reader);
        }
    }
    switch (code) {
    case 2:
        tag = reader->getUtf8String();
        break;
    case 70:
        attribFlags = reader->getInt32();
        break;
    case 73:
        // AcDbAttribute code 73 = field length (obsolete); NOT the vertical
        // alignment from AcDbText (which is code 73 in TEXT but 74 in ATTRIB).
        m_fieldLength = reader->getInt32();
        break;
    case 74:
        // AcDbAttribute vertical alignment (code 74); TEXT uses code 73 for
        // this but ATTRIB moves it here to free code 73 for field length.
        alignV = (VAlign)reader->getInt32();
        break;
    case 280:
        // Lock position flag (R2010+ DXF group code)
        lockPosition = reader->getInt32() != 0;
        break;
    default:
        return DRW_Text::parseCode(code, reader);
    }
    return true;
}

void DRW_Attrib::resetAttribDwgState() {
    resetDwgState();
    tag.clear();
    attribFlags = 0;
    m_fieldLength = 0;
    lockPosition = false;
    attVersion = 0;
    m_attributeType = 1;
    keepDuplicateRecords = 0;
    mtext.reset();
    dxfInAttributeSubclass = false;
}

bool DRW_Attrib::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetAttribDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetAttribDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = bodyProbe.forkIndependent();
    dwgBuffer *sBuf = &bodyProbe;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing attrib *********************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018
        && (!buf->getR2007StringStreamBounds(objSize, stringStartBit,
                                             stringEndBit)
            || currentDwgBit(sBuf) > stringEndBit))
        return fail();

    DwgTextBodyData parsedText;
    if (!readBoundedDwgTextBody(version, *buf, *sBuf, bodyEndBit,
                                stringStartBit, stringEndBit, parsedText))
        return fail();

    // R2010+ ATTRIB version follows the common TEXT data. R2018 adds the
    // attribute type immediately after it.
    std::uint8_t parsedAttVersion = 0;
    std::uint8_t parsedAttributeType = 1;
    if (version >= DRW::AC1024) {
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedAttVersion))
            return fail();
    }
    if (version >= DRW::AC1032) {
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedAttributeType))
            return fail();
    }

    bool hasEmbeddedMText = false;
    EmbeddedMTextHandleInfo embeddedMTextHandles;
    std::unique_ptr<DRW_MText> parsedMText;
    if (version >= DRW::AC1032
        && parsedAttributeType != 0 && parsedAttributeType != 1) {
        parsedMText = std::make_unique<DRW_MText>();
        if (!parseEmbeddedMTextDwg(version, buf, sBuf, *parsedMText,
                                   embeddedMTextHandles, bodyEndBit,
                                   stringEndBit)) {
            DRW_DBG("R2018 multi-line ATTRIB payload failed\n");
            return fail();
        }
        hasEmbeddedMText = true;
    }

    // ATTRIB-specific fields
    UTF8STRING parsedTag;
    if (version <= DRW::AC1018 || stringStartBit < stringEndBit) {
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     parsedTag))
            return fail();
    }

    std::uint16_t parsedFieldLength = 0;
    std::uint8_t parsedAttribFlags = 0;
    bool parsedLockPosition = false;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedFieldLength)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedAttribFlags))
        return fail();

    // lockPosition (DXF 280) appears since R2007 (AC1021) per ODA §20.4.x /
    // ACadSharp.  Read gate lowered AC1024->AC1021 so R2007/8/9 imports keep
    // it.  The writer emits the matching bit for AC1021 and later;
    // parseDwgEntHandle repositions to objSize for version>AC1018 before
    // consuming the handle stream.
    if (version >= DRW::AC1021) {
        if (!readBoundedBit(*buf, bodyEndBit, parsedLockPosition))
            return fail();
    }

    if (!buf->isGood() || !sBuf->isGood())
        return fail();

    /* Common Entity Handle Data */
    // The parent ATTRIB handle stream starts at objSize. Embedded MTEXT
    // handles follow it, so parse the parent first and consume the embedded
    // stream from the resulting position.
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();

    if (hasEmbeddedMText) {
        if (!consumeEmbeddedMTextHandles(version, &handleProbe,
                                            embeddedMTextHandles,
                                            parsedMText.get(), handleEndBit))
            return fail();
    }

    dwgHandle parsedStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false, parsedStyleH)
        || !handleProbe.isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    *sourceBuf = handleProbe;
    basePoint = parsedText.basePoint;
    secPoint = parsedText.secPoint;
    extPoint = parsedText.extPoint;
    thickness = parsedText.thickness;
    oblique = parsedText.oblique;
    angle = parsedText.angle;
    height = parsedText.height;
    widthscale = parsedText.widthscale;
    text = std::move(parsedText.text);
    textgen = parsedText.textgen;
    alignH = parsedText.alignH;
    alignV = parsedText.alignV;
    attVersion = parsedAttVersion;
    m_attributeType = parsedAttributeType;
    mtext = std::move(parsedMText);
    tag = std::move(parsedTag);
    m_fieldLength = parsedFieldLength;
    attribFlags = parsedAttribFlags;
    lockPosition = parsedLockPosition;
    styleH = parsedStyleH;
    DRW_DBG("text style Handle: "); DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");
    return true;
}

bool DRW_Attrib::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (version >= DRW::AC1024 && version < DRW::AC1032 && (attVersion != 0 || mtext))
        return false;
    const std::uint8_t attributeType = (m_attributeType == 0) ? 1 : m_attributeType;
    const bool hasEmbeddedMText = version >= DRW::AC1032 && attributeType != 1;
    if (hasEmbeddedMText && !mtext)
        return false;

    oType = 2;  // ATTRIB class id — see dwgreader.cpp:1148
    if (!encodeDwgCommon(version, buf)) return false;

    // TEXT-body section — mirrors DRW_Attrib::parseDwg.
    // data_flags=0: emit every optional field unconditionally (same
    // strategy as DRW_Text::encodeDwg — simpler encoder, ~30 bytes larger).
    buf->putRawChar8(0);                              // data_flags=0
    buf->putRawDouble(basePoint.z);                   // elevation RD
    buf->putRawDouble(basePoint.x);                   // insertion 2RD
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.x, secPoint.x);   // alignment 2DD
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putRawDouble(oblique);                       // oblique angle RD
    buf->putRawDouble(angle / ARAD);                  // angle in radians RD
    buf->putRawDouble(height);                        // text height RD
    buf->putRawDouble(widthscale);                    // width factor RD
    dwgBufferW *sb = strBuf ? strBuf : buf;
    sb->putVariableText(version, text);               // text string TV
    buf->putBitShort(static_cast<std::uint16_t>(textgen));  // generation flags BS
    buf->putBitShort(static_cast<std::uint16_t>(alignH));   // horiz align BS
    buf->putBitShort(static_cast<std::uint16_t>(alignV));   // vert align BS

    if (version >= DRW::AC1024) {
        buf->putRawChar8(hasEmbeddedMText && attVersion == 0 ? 1 : attVersion);
    }
    if (version >= DRW::AC1032) {
        buf->putRawChar8(attributeType);
    }

    dwgBufferW embeddedHandles;
    if (hasEmbeddedMText) {
        if (!encodeEmbeddedMTextDwg(version, buf, strBuf, &embeddedHandles, *mtext))
            return false;
    }

    // ATTRIB-specific tail
    sb->putVariableText(version, tag);                // tag TV
    buf->putBitShort(static_cast<std::uint16_t>(m_fieldLength)); // fieldLen BS
    buf->putRawChar8(attribFlags);                    // flags RC
    if (version >= DRW::AC1021) {
        buf->putBit(lockPosition ? 1 : 0);             // lock position B (R2007a+)
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgHandle sH;
    std::uint32_t sref = (styleH.ref == 0) ? 0x13 : styleH.ref;
    sH.code = 5;
    sH.ref  = sref;
    sH.size = 0;
    if (sref != 0) { std::uint32_t t = sref; while (t != 0) { t >>= 8; ++sH.size; } }
    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    appendBitBuffer(hb, embeddedHandles);
    hb->putHandle(sH);
    return buf->isGood() && sb->isGood() && hb->isGood();
}

bool DRW_Attdef::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        prompt = reader->getUtf8String();
        break;
    default:
        return DRW_Attrib::parseCode(code, reader);
    }
    return true;
}

bool DRW_Attdef::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    // ATTDEF mirrors ATTRIB layout but adds a prompt string after the tag.
    // Implementation duplicates ATTRIB::parseDwg in order to inject the
    // prompt read at the correct offset; refactor opportunity if a third
    // sibling appears.
    resetAttribDwgState();
    prompt.clear();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetAttribDwgState();
        prompt.clear();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = bodyProbe.forkIndependent();
    dwgBuffer *sBuf = &bodyProbe;
    if (version > DRW::AC1018) {
        sBuf = &sBuff;
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing attdef *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018
        && (!buf->getR2007StringStreamBounds(objSize, stringStartBit,
                                             stringEndBit)
            || currentDwgBit(sBuf) > stringEndBit))
        return fail();

    DwgTextBodyData parsedText;
    if (!readBoundedDwgTextBody(version, *buf, *sBuf, bodyEndBit,
                                stringStartBit, stringEndBit, parsedText))
        return fail();

    std::uint8_t parsedAttVersion = 0;
    std::uint8_t parsedAttributeType = 1;
    if (version >= DRW::AC1024) {
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedAttVersion))
            return fail();
    }
    if (version >= DRW::AC1032) {
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedAttributeType))
            return fail();
    }

    bool hasEmbeddedMText = false;
    EmbeddedMTextHandleInfo embeddedMTextHandles;
    std::unique_ptr<DRW_MText> parsedMText;
    if (version >= DRW::AC1032
        && parsedAttributeType != 0 && parsedAttributeType != 1) {
        parsedMText = std::make_unique<DRW_MText>();
        if (!parseEmbeddedMTextDwg(version, buf, sBuf, *parsedMText,
                                   embeddedMTextHandles, bodyEndBit,
                                   stringEndBit)) {
            DRW_DBG("R2018 multi-line ATTDEF payload failed\n");
            return fail();
        }
        hasEmbeddedMText = true;
    }

    UTF8STRING parsedTag;
    if (version <= DRW::AC1018 || stringStartBit < stringEndBit) {
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     parsedTag))
            return fail();
    }

    std::uint16_t parsedFieldLength = 0;
    std::uint8_t parsedAttribFlags = 0;
    bool parsedLockPosition = false;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedFieldLength)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedAttribFlags))
        return fail();

    // lockPosition (DXF 280) is a bit in R2007+ DWG data.
    if (version >= DRW::AC1021) {
        if (!readBoundedBit(*buf, bodyEndBit, parsedLockPosition))
            return fail();
    }

    // ATTDEF prompt follows attrib body
    UTF8STRING parsedPrompt;
    if (version <= DRW::AC1018 || stringStartBit < stringEndBit) {
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     parsedPrompt))
            return fail();
    }

    if (!buf->isGood() || !sBuf->isGood())
        return fail();

    // The parent ATTDEF handle stream precedes the embedded MTEXT handles.
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();

    if (hasEmbeddedMText) {
        if (!consumeEmbeddedMTextHandles(version, &handleProbe,
                                            embeddedMTextHandles,
                                            parsedMText.get(), handleEndBit))
            return fail();
    }

    dwgHandle parsedStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false, parsedStyleH)
        || !handleProbe.isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    *sourceBuf = handleProbe;
    basePoint = parsedText.basePoint;
    secPoint = parsedText.secPoint;
    extPoint = parsedText.extPoint;
    thickness = parsedText.thickness;
    oblique = parsedText.oblique;
    angle = parsedText.angle;
    height = parsedText.height;
    widthscale = parsedText.widthscale;
    text = std::move(parsedText.text);
    textgen = parsedText.textgen;
    alignH = parsedText.alignH;
    alignV = parsedText.alignV;
    attVersion = parsedAttVersion;
    m_attributeType = parsedAttributeType;
    mtext = std::move(parsedMText);
    tag = std::move(parsedTag);
    m_fieldLength = parsedFieldLength;
    attribFlags = parsedAttribFlags;
    lockPosition = parsedLockPosition;
    prompt = std::move(parsedPrompt);
    styleH = parsedStyleH;
    return true;
}

bool DRW_Attdef::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (version >= DRW::AC1024 && version < DRW::AC1032 && (attVersion != 0 || mtext))
        return false;
    const std::uint8_t attributeType = (m_attributeType == 0) ? 1 : m_attributeType;
    const bool hasEmbeddedMText = version >= DRW::AC1032 && attributeType != 1;
    if (hasEmbeddedMText && !mtext)
        return false;

    oType = 3;  // ATTDEF class id — see dwgreader.cpp:1185
    if (!encodeDwgCommon(version, buf)) return false;

    // TEXT-body section — identical layout to DRW_Attrib::encodeDwg.
    buf->putRawChar8(0);
    buf->putRawDouble(basePoint.z);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.x, secPoint.x);
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putRawDouble(oblique);
    buf->putRawDouble(angle / ARAD);
    buf->putRawDouble(height);
    buf->putRawDouble(widthscale);
    dwgBufferW *sb = strBuf ? strBuf : buf;
    sb->putVariableText(version, text);
    buf->putBitShort(static_cast<std::uint16_t>(textgen));
    buf->putBitShort(static_cast<std::uint16_t>(alignH));
    buf->putBitShort(static_cast<std::uint16_t>(alignV));

    if (version >= DRW::AC1024) {
        buf->putRawChar8(hasEmbeddedMText && attVersion == 0 ? 1 : attVersion);
    }
    if (version >= DRW::AC1032) {
        buf->putRawChar8(attributeType);
    }

    dwgBufferW embeddedHandles;
    if (hasEmbeddedMText) {
        if (!encodeEmbeddedMTextDwg(version, buf, strBuf, &embeddedHandles, *mtext))
            return false;
    }

    sb->putVariableText(version, tag);
    buf->putBitShort(static_cast<std::uint16_t>(m_fieldLength)); // fieldLen BS
    buf->putRawChar8(attribFlags);
    if (version >= DRW::AC1021) {
        buf->putBit(lockPosition ? 1 : 0);
    }

    // ATTDEF adds prompt between flags and handle stream
    sb->putVariableText(version, prompt);

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgHandle sH;
    std::uint32_t sref = (styleH.ref == 0) ? 0x13 : styleH.ref;
    sH.code = 5;
    sH.ref  = sref;
    sH.size = 0;
    if (sref != 0) { std::uint32_t t = sref; while (t != 0) { t >>= 8; ++sH.size; } }
    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    appendBitBuffer(hb, embeddedHandles);
    hb->putHandle(sH);
    return buf->isGood() && sb->isGood() && hb->isGood();
}

bool DRW_MText::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text += reader->getString();
        text = reader->toUtf8String(text);
        break;
    case 11:
        hasXAxisVec = true;
        return DRW_Text::parseCode(code, reader);
    case 3:
        text += reader->getString();
        break;
    case 44:
        interlin = reader->getDouble();
        break;
    case 50: // djm: per dxf docs, last of code 11 or code 50 prevails
        hasXAxisVec = false;
        angle = reader->getDouble();
        break;
    case 73: {
        const int value = reader->getInt32();
        if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
            return false;
        linespacingStyle = static_cast<std::uint16_t>(value);
        break;
    }
    case 42:
        m_r2018ExtentsWidth = reader->getDouble();
        break;
    case 43:
        m_r2018ExtentsHeight = reader->getDouble();
        break;
    case 45:
        m_backgroundScale = reader->getDouble();
        break;
    case 63:
        m_backgroundColor = reader->getInt32();
        break;
    case 90:
        m_backgroundFlags = reader->getInt32();
        break;
    case 421:
        m_backgroundColor = reader->getInt32();
        break;
    case 441:
        m_backgroundTransparency = reader->getInt32();
        break;
    default:
        return DRW_Text::parseCode(code, reader);
    }

    return true;
}

void DRW_MText::resetDwgState() {
    DRW_Text::resetDwgState();
    interlin = 1.0;
    linespacingStyle = 1;
    m_backgroundFlags = 0;
    m_backgroundScale = 0.0;
    m_backgroundColor = 0;
    m_backgroundTransparency = 0;
    hasXAxisVec = false;
    m_r2018IsNotAnnotative = false;
    m_r2018ReallyLocked = false;
    m_r2018Version = 0;
    m_r2018DefaultFlag = false;
    m_r2018AppIdHandle = 0;
    m_r2018Attachment = 0;
    m_r2018XAxisDir = {};
    m_r2018InsertionPoint = {};
    m_r2018RectWidth = 0.0;
    m_r2018RectHeight = 0.0;
    m_r2018ExtentsHeight = 0.0;
    m_r2018ExtentsWidth = 0.0;
    m_r2018ColumnType = 0;
    m_r2018ColumnCount = 0;
    m_r2018ColumnWidth = 0.0;
    m_r2018ColumnGutter = 0.0;
    m_r2018ColumnAutoHeight = false;
    m_r2018ColumnFlowReversed = false;
    m_r2018ColumnHeights.clear();
    m_r2018AnnotativeData.clear();
    m_r2018AnnotativeUnknown = 0;
    m_r2018AnnotativeAppHandle = 0;
    textgen = 1;
    alignV = static_cast<VAlign>(TopLeft);
}

bool DRW_MText::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    // A reader may reuse an entity carrier while walking a compound stream.
    // R2018 context data is version-gated, so clear it before parsing an
    // older MTEXT or stale column/annotative state can leak into the result.
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing mtext *********************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018) {
        if (!buf->getR2007StringStreamBounds(objSize, stringStartBit,
                                             stringEndBit)
            || currentDwgBit(sBuf) > stringEndBit)
            return fail();
    }

    const bool hasStringStream = stringStartBit < stringEndBit;
    const auto readText = [&](UTF8STRING& value) {
        if (version > DRW::AC1018 && !hasStringStream) {
            value.clear();
            return true;
        }
        const std::uint64_t endBit = version > DRW::AC1018
            ? stringEndBit : bodyEndBit;
        return readBoundedVariableText(*sBuf, endBit, version, value);
    };

    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtrusion;
    DRW_Coord parsedXAxisDirection;
    double parsedWidthScale = 0.0;
    double parsedRectangleHeight = 0.0;
    double parsedHeight = 0.0;
    std::uint16_t parsedAttachment = 1;
    std::uint16_t parsedDrawingDirection = 0;
    double ignoredExtentsHeight = 0.0;
    double ignoredExtentsWidth = 0.0;
    UTF8STRING parsedText;
    std::uint16_t parsedLineSpacingStyle = 1;
    double parsedInterlin = 1.0;
    std::int32_t parsedBackgroundFlags = 0;
    double parsedBackgroundScale = 0.0;
    std::uint32_t parsedBackgroundColor = 0;
    std::int32_t parsedBackgroundTransparency = 0;
    bool parsedIsNotAnnotative = false;
    bool parsedReallyLocked = false;
    std::uint16_t parsedR2018Version = 0;
    bool parsedDefaultFlag = false;
    std::int32_t parsedR2018Attachment = 0;
    DRW_Coord parsedR2018XAxisDirection;
    DRW_Coord parsedR2018InsertionPoint;
    double parsedR2018RectangleWidth = 0.0;
    double parsedR2018RectangleHeight = 0.0;
    double parsedR2018ExtentsHeight = 0.0;
    double parsedR2018ExtentsWidth = 0.0;
    std::uint16_t parsedColumnType = 0;
    std::int32_t parsedColumnCount = 0;
    double parsedColumnWidth = 0.0;
    double parsedColumnGutter = 0.0;
    bool parsedColumnAutoHeight = false;
    bool parsedColumnFlowReversed = false;
    std::vector<double> parsedColumnHeights;

    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedExtrusion)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedXAxisDirection)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedWidthScale)
        || (version > DRW::AC1018
            && !readBoundedBitDouble(*buf, bodyEndBit,
                                     parsedRectangleHeight))
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedHeight)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedAttachment)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedDrawingDirection)
        || !readBoundedBitDouble(*buf, bodyEndBit, ignoredExtentsHeight)
        || !readBoundedBitDouble(*buf, bodyEndBit, ignoredExtentsWidth)
        || !readText(parsedText))
        return fail();

    if (version > DRW::AC1014
        && (!readBoundedBitShort(*buf, bodyEndBit, parsedLineSpacingStyle)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedInterlin)))
        return fail();
    bool ignoredBit = false;
    if (version > DRW::AC1014 && !readBoundedBit(*buf, bodyEndBit,
                                                 ignoredBit))
        return fail();

    if (version > DRW::AC1015) {
        if (!readBoundedBitLong(*buf, bodyEndBit, parsedBackgroundFlags))
            return fail();
        const bool hasBackgroundDetails = (parsedBackgroundFlags & 0x01) != 0
            || (version >= DRW::AC1032
                && (parsedBackgroundFlags & 0x10) != 0);
        if (hasBackgroundDetails
            && (!readBoundedBitDouble(*buf, bodyEndBit,
                                      parsedBackgroundScale)
                || !readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                                       parsedBackgroundColor, nullptr,
                                       nullptr, nullptr, nullptr,
                                       stringEndBit)
                || !readBoundedBitLong(*buf, bodyEndBit,
                                       parsedBackgroundTransparency)))
            return fail();
    }

    bool hasR2018AppId = false;
    if (version >= DRW::AC1032) {
        if (!readBoundedBit(*buf, bodyEndBit, parsedIsNotAnnotative))
            return fail();
        if (parsedIsNotAnnotative) {
            hasR2018AppId = true;
            if (!readBoundedBitShort(*buf, bodyEndBit, parsedR2018Version)
                || !readBoundedBit(*buf, bodyEndBit, parsedDefaultFlag)
                || !readBoundedBitLong(*buf, bodyEndBit,
                                       parsedR2018Attachment)
                || !readBoundedBitCoord(*buf, bodyEndBit,
                                        parsedR2018XAxisDirection)
                || !readBoundedBitCoord(*buf, bodyEndBit,
                                        parsedR2018InsertionPoint)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         parsedR2018RectangleWidth)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         parsedR2018RectangleHeight)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         parsedR2018ExtentsHeight)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         parsedR2018ExtentsWidth)
                || !readBoundedBitShort(*buf, bodyEndBit, parsedColumnType)
                || parsedColumnType > 2)
                return fail();
            if (parsedColumnType != 0) {
                if (!readBoundedBitLong(*buf, bodyEndBit, parsedColumnCount)
                    || !isValidCount(parsedColumnCount,
                                     kMaxMTextColumnHeights)
                    || !readBoundedBitDouble(*buf, bodyEndBit,
                                             parsedColumnWidth)
                    || !readBoundedBitDouble(*buf, bodyEndBit,
                                             parsedColumnGutter)
                    || !readBoundedBit(*buf, bodyEndBit,
                                       parsedColumnAutoHeight)
                    || !readBoundedBit(*buf, bodyEndBit,
                                       parsedColumnFlowReversed))
                    return fail();
                if (!parsedColumnAutoHeight && parsedColumnType == 2
                    && parsedColumnCount > 0) {
                    if (!DRW::reserve(parsedColumnHeights,
                                      parsedColumnCount))
                        return fail();
                    for (std::int32_t i = 0; i < parsedColumnCount; ++i) {
                        double columnHeight = 0.0;
                        if (!readBoundedBitDouble(*buf, bodyEndBit,
                                                  columnHeight)
                            || !std::isfinite(columnHeight))
                            return fail();
                        parsedColumnHeights.push_back(columnHeight);
                    }
                }
            }
        }
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!buf->isGood() || !sBuf->isGood()
        || !finite(parsedBasePoint) || !finite(parsedExtrusion)
        || !finite(parsedXAxisDirection)
        || !std::isfinite(parsedWidthScale)
        || !std::isfinite(parsedRectangleHeight)
        || !std::isfinite(parsedHeight)
        || !std::isfinite(ignoredExtentsHeight)
        || !std::isfinite(ignoredExtentsWidth)
        || !std::isfinite(parsedInterlin)
        || !finite(parsedR2018XAxisDirection)
        || !finite(parsedR2018InsertionPoint)
        || !std::isfinite(parsedR2018RectangleWidth)
        || !std::isfinite(parsedR2018RectangleHeight)
        || !std::isfinite(parsedR2018ExtentsHeight)
        || !std::isfinite(parsedR2018ExtentsWidth))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit))
        return fail();

    dwgHandle parsedStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              parsedStyleH))
        return fail();
    dwgHandle parsedAppIdH;
    if (hasR2018AppId
        && !readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  parsedAppIdH))
        return fail();
    if (!handleProbe.isGood() || !sBuf->isGood())
        return fail();

    *sourceBuf = handleProbe;
    basePoint = parsedBasePoint;
    extPoint = parsedExtrusion;
    secPoint = parsedXAxisDirection;
    hasXAxisVec = true;
    widthscale = parsedWidthScale;
    height = parsedHeight;
    angle = atan2(secPoint.y, secPoint.x) * ARAD;
    textgen = parsedAttachment;
    alignH = static_cast<HAlign>(parsedDrawingDirection);
    text = std::move(parsedText);
    linespacingStyle = parsedLineSpacingStyle;
    interlin = parsedInterlin;
    m_backgroundFlags = parsedBackgroundFlags;
    m_backgroundScale = parsedBackgroundScale;
    m_backgroundColor = static_cast<int>(parsedBackgroundColor);
    m_backgroundTransparency = parsedBackgroundTransparency;
    m_r2018IsNotAnnotative = parsedIsNotAnnotative;
    m_r2018Version = parsedR2018Version;
    m_r2018DefaultFlag = parsedDefaultFlag;
    m_r2018AppIdHandle = hasR2018AppId ? parsedAppIdH.ref : 0;
    m_r2018Attachment = parsedR2018Attachment;
    m_r2018XAxisDir = parsedR2018XAxisDirection;
    m_r2018InsertionPoint = parsedR2018InsertionPoint;
    m_r2018RectWidth = parsedR2018RectangleWidth;
    m_r2018RectHeight = parsedR2018RectangleHeight;
    m_r2018ExtentsHeight = parsedR2018ExtentsHeight;
    m_r2018ExtentsWidth = parsedR2018ExtentsWidth;
    m_r2018ColumnType = parsedColumnType;
    m_r2018ColumnCount = parsedColumnCount;
    m_r2018ColumnWidth = parsedColumnWidth;
    m_r2018ColumnGutter = parsedColumnGutter;
    m_r2018ColumnAutoHeight = parsedColumnAutoHeight;
    m_r2018ColumnFlowReversed = parsedColumnFlowReversed;
    m_r2018ColumnHeights = std::move(parsedColumnHeights);
    styleH = parsedStyleH;
    DRW_DBG("text style Handle: "); DRW_DBG(styleH.code); DRW_DBG(".");
    DRW_DBG(styleH.size); DRW_DBG("."); DRW_DBG(styleH.ref); DRW_DBG("\n");
    return true;
}

void DRW_MText::updateAngle() {
    if (hasXAxisVec) {
        angle = atan2(secPoint.y, secPoint.x) * ARAD;
    }
}

bool DRW_Polyline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        defstawidth = reader->getDouble();
        break;
    case 41:
        defendwidth = reader->getDouble();
        break;
    case 71:
        vertexcount = reader->getInt32();
        break;
    case 72:
        facecount = reader->getInt32();
        break;
    case 73:
        smoothM = reader->getInt32();
        break;
    case 74:
        smoothN = reader->getInt32();
        break;
    case 75:
        curvetype = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

//0x0F polyline 2D bit 4(8) & 5(16) NOT set
//0x10 polyline 3D bit 4(8) set
//0x1D PFACE bit 5(16) set
void DRW_Polyline::resetDwgState() {
    DRW_Point::resetDwgState();
    flags = 0;
    defstawidth = 0.0;
    defendwidth = 0.0;
    vertexcount = 0;
    facecount = 0;
    smoothM = 0;
    smoothN = 0;
    curvetype = 0;
    vertlist.clear();
    hadlesList.clear();
    firstEH = 0;
    lastEH = 0;
    seqEndH = dwgHandle{};
}

bool DRW_Polyline::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto validBitShort = [](int value) {
        return value >= 0
            && value <= std::numeric_limits<std::uint16_t>::max();
    };
    return flags >= 0 && flags <= std::numeric_limits<std::uint8_t>::max()
        && validBitShort(curvetype) && validBitShort(vertexcount)
        && validBitShort(facecount) && validBitShort(smoothM)
        && validBitShort(smoothN) && finite(basePoint)
        && finite(extPoint) && std::isfinite(defstawidth)
        && std::isfinite(defendwidth) && std::isfinite(thickness)
        && vertlist.size() <= dwgSafety::MaxOwnedObjectCount
        && vertlist.size()
               <= static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        && std::all_of(vertlist.cbegin(), vertlist.cend(),
                       [](const auto& vertex) { return vertex != nullptr; });
}

bool DRW_Polyline::isDwgVertexCompatible(
    const DRW_Vertex& vertex) const noexcept {
    if ((flags & 64) != 0) {
        return vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Polyface
            || vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::PolyfaceFace;
    }
    if ((flags & 16) != 0)
        return vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Mesh;
    if ((flags & 8) != 0)
        return vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Vertex3D;
    return vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Vertex2D;
}

bool DRW_Polyline::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };

    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    DRW_DBG("\n***************************** parsing polyline *********************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    int parsedFlags = 0;
    double parsedDefStartWidth = 0.0;
    double parsedDefEndWidth = 0.0;
    int parsedVertexCount = 0;
    int parsedFaceCount = 0;
    int parsedSmoothM = 0;
    int parsedSmoothN = 0;
    int parsedCurveType = 0;
    double parsedThickness = 0.0;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedExtrusion;
    std::int32_t ooCount = 0;
    if (oType == 0x0F) { //pline 2D
        std::uint16_t value = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedFlags = value;
        DRW_DBG("flags value: "); DRW_DBG(parsedFlags);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedCurveType = value;
        if (!readBoundedBitDouble(bodyProbe, bodyEndBit, parsedDefStartWidth)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedDefEndWidth)
            || !readBoundedThickness(bodyProbe, bodyEndBit,
                                      version > DRW::AC1014, parsedThickness)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedBasePoint.z)
            || !readBoundedExtrusion(bodyProbe, bodyEndBit,
                                     version > DRW::AC1014, parsedExtrusion))
            return fail();
    } else if (oType == 0x10) { //pline 3D
        std::uint8_t tmpFlag = 0;
        if (!readBoundedRawChar8(bodyProbe, bodyEndBit, tmpFlag))
            return fail();
        DRW_DBG("flags 1 value: "); DRW_DBG(tmpFlag);
        if (tmpFlag & 1)
            parsedCurveType = 5;       // quadratic B-spline
        else if (tmpFlag & 2)
            parsedCurveType = 6;       // cubic B-spline
        if (tmpFlag & 3)
            parsedFlags |= 4;          // splined (bit 2); do NOT overwrite curvetype to 8
        if (!readBoundedRawChar8(bodyProbe, bodyEndBit, tmpFlag))
            return fail();
        if (tmpFlag & 1)
            parsedFlags |= 1;
        parsedFlags |= 8; //indicate 3DPOL
        DRW_DBG("flags 2 value: "); DRW_DBG(tmpFlag);
    } else if (oType == 0x1D) { //PFACE
        std::uint16_t vertexValue = 0;
        std::uint16_t faceValue = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, vertexValue)
            || !readBoundedBitShort(bodyProbe, bodyEndBit, faceValue))
            return fail();
        parsedFlags = 64;
        parsedVertexCount = vertexValue;
        parsedFaceCount = faceValue;
        DRW_DBG("vertex count: "); DRW_DBG(parsedVertexCount);
        DRW_DBG("face count: "); DRW_DBG(parsedFaceCount);
        DRW_DBG("flags value: "); DRW_DBG(parsedFlags);
    } else if (oType == 0x1E) { //POLYLINE_MESH per ODA spec sec 19.4.31
        std::uint16_t value = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedFlags = value | 16; //bit 4 = 3D polygon mesh
        DRW_DBG("flags value: "); DRW_DBG(parsedFlags);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedCurveType = value;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedVertexCount = value; //M-count
        DRW_DBG(" M count: "); DRW_DBG(parsedVertexCount);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedFaceCount = value; //N-count
        DRW_DBG(" N count: "); DRW_DBG(parsedFaceCount);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedSmoothM = value; //M smooth-surface density, DXF 73
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedSmoothN = value; //N smooth-surface density, DXF 74
        DRW_DBG(" M/N density: "); DRW_DBG(parsedSmoothM); DRW_DBG("/"); DRW_DBG(parsedSmoothN);
    } else {
        return fail();
    }
    if (version > DRW::AC1015){ //2004+
        if (!readBoundedBitLong(bodyProbe, bodyEndBit, ooCount)
            || !dwgSafety::validOwnedObjectCount(
                ooCount, bodyProbe.numRemainingBytes())) {
            DRW_DBG("\nWARNING: Invalid POLYLINE owned-object count\n");
            return fail();
        }
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();

    std::list<std::uint32_t> parsedHandles;
    std::uint32_t parsedFirstEH = 0;
    std::uint32_t parsedLastEH = 0;

    if (version < DRW::AC1018){ //2000-
        dwgHandle objectH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                  objectH))
            return fail();
        parsedFirstEH = objectH.ref;
        DRW_DBG(" first Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                  objectH))
            return fail();
        parsedLastEH = objectH.ref;
        DRW_DBG(" last Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
    } else {
        for (std::int32_t i = 0; i < ooCount; ++i){
                dwgHandle objectH;
                if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle,
                                          true, objectH))
                    return fail();
                try {
                    parsedHandles.push_back(objectH.ref);
                } catch (...) {
                    return fail();
                }
                DRW_DBG(" Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
                DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
        }
    }
    dwgHandle parsedSeqEndH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                              parsedSeqEndH))
        return fail();
    if (!handleProbe.isGood())
        return fail();
    *sourceBuf = handleProbe;
    flags = parsedFlags;
    defstawidth = parsedDefStartWidth;
    defendwidth = parsedDefEndWidth;
    vertexcount = parsedVertexCount;
    facecount = parsedFaceCount;
    smoothM = parsedSmoothM;
    smoothN = parsedSmoothN;
    curvetype = parsedCurveType;
    thickness = parsedThickness;
    basePoint = parsedBasePoint;
    extPoint = parsedExtrusion;
    firstEH = parsedFirstEH;
    lastEH = parsedLastEH;
    seqEndH = parsedSeqEndH;
    hadlesList = std::move(parsedHandles);
    DRW_DBG(" SEQEND Handle: "); DRW_DBGHL(seqEndH.code, seqEndH.size, seqEndH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Vertex::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        stawidth = reader->getDouble();
        break;
    case 41:
        endwidth = reader->getDouble();
        break;
    case 42:
        bulge = reader->getDouble();
        break;
    case 50:
        tgdir = reader->getDouble();
        break;
    case 71:
        vindex1 = reader->getInt32();
        break;
    case 72:
        vindex2 = reader->getInt32();
        break;
    case 73:
        vindex3 = reader->getInt32();
        break;
    case 74:
        vindex4 = reader->getInt32();
        break;
    case 91:
        identifier = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

//0x0A vertex 2D
//0x0B vertex 3D
//0x0C MESH
//0x0D PFACE
//0x0E PFACE FACE
void DRW_Vertex::resetDwgState() {
    DRW_Point::resetDwgState();
    stawidth = 0.0;
    endwidth = 0.0;
    bulge = 0.0;
    flags = 0;
    tgdir = 0.0;
    vindex1 = vindex2 = vindex3 = vindex4 = 0;
    identifier = 0;
    m_dwgSubtype = DwgSubtype::Auto;
}

bool DRW_Vertex::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto validFaceIndex = [](int value) {
        return value >= std::numeric_limits<std::int16_t>::min()
            && value <= std::numeric_limits<std::int16_t>::max();
    };
    const auto validInt32 = [](int value) {
        return value >= std::numeric_limits<std::int32_t>::min()
            && value <= std::numeric_limits<std::int32_t>::max();
    };
    return flags >= 0 && flags <= std::numeric_limits<std::uint8_t>::max()
        && validFaceIndex(vindex1) && validFaceIndex(vindex2)
        && validFaceIndex(vindex3) && validFaceIndex(vindex4)
        && validInt32(identifier) && finite(basePoint)
        && std::isfinite(stawidth) && std::isfinite(endwidth)
        && std::isfinite(bulge) && std::isfinite(tgdir);
}

bool DRW_Vertex::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs, double el){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };

    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &bodyProbe, bs)
        || !bodyProbe.isGood())
        return fail();
    DRW_DBG("\n***************************** parsing pline Vertex *********************************************\n");

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Vertex::DwgSubtype parsedSubtype = DRW_Vertex::DwgSubtype::Auto;
    std::uint8_t parsedFlags = 0;
    DRW_Coord parsedBasePoint;
    double parsedStartWidth = 0.0;
    double parsedEndWidth = 0.0;
    double parsedBulge = 0.0;
    double parsedTangentDirection = 0.0;
    int parsedIndex1 = 0;
    int parsedIndex2 = 0;
    int parsedIndex3 = 0;
    int parsedIndex4 = 0;
    int parsedIdentifier = 0;
    if (oType == 0x0A) { //pline 2D, needed example
        parsedSubtype = DwgSubtype::Vertex2D;
        if (!readBoundedRawChar8(bodyProbe, bodyEndBit, parsedFlags)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedBasePoint)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedStartWidth))
            return fail();
        parsedBasePoint.z = el;
        DRW_DBG("flags value: "); DRW_DBG(parsedFlags);
        DRW_DBG("basePoint: "); DRW_DBGPT(parsedBasePoint.x, parsedBasePoint.y, parsedBasePoint.z);
        if (parsedStartWidth < 0.0) {
            parsedEndWidth = parsedStartWidth = std::fabs(parsedStartWidth);
        } else if (!readBoundedBitDouble(bodyProbe, bodyEndBit,
                                         parsedEndWidth)) {
            return fail();
        }
        if (!readBoundedBitDouble(bodyProbe, bodyEndBit, parsedBulge))
            return fail();
        if (version > DRW::AC1021) { //2010+
            if (!readBoundedBitLong(bodyProbe, bodyEndBit, parsedIdentifier))
                return fail();
            DRW_DBG("Vertex ID: "); DRW_DBG(parsedIdentifier);
        }
        if (!readBoundedBitDouble(bodyProbe, bodyEndBit,
                                  parsedTangentDirection))
            return fail();
    } else if (oType == 0x0B || oType == 0x0C || oType == 0x0D) { //PFACE
        if (oType == 0x0B)
            parsedSubtype = DwgSubtype::Vertex3D;
        else if (oType == 0x0C)
            parsedSubtype = DwgSubtype::Mesh;
        else
            parsedSubtype = DwgSubtype::Polyface;
        if (!readBoundedRawChar8(bodyProbe, bodyEndBit, parsedFlags)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, parsedBasePoint))
            return fail();
        DRW_DBG("flags value: "); DRW_DBG(parsedFlags);
        DRW_DBG("basePoint: "); DRW_DBGPT(parsedBasePoint.x, parsedBasePoint.y, parsedBasePoint.z);
    } else if (oType == 0x0E) { //PFACE FACE
        parsedSubtype = DwgSubtype::PolyfaceFace;
        auto signedIndex = [](std::uint16_t value) {
            return value > 32767 ? value - 65536 : value;
        };
        std::uint16_t value = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedIndex1 = signedIndex(value);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedIndex2 = signedIndex(value);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedIndex3 = signedIndex(value);
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, value))
            return fail();
        parsedIndex4 = signedIndex(value);
    } else {
        return fail();
    }

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedBasePoint) || !std::isfinite(parsedStartWidth)
        || !std::isfinite(parsedEndWidth) || !std::isfinite(parsedBulge)
        || !std::isfinite(parsedTangentDirection)
        || !std::isfinite(el))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();
    //    RS crc;   //RS */
    *sourceBuf = handleProbe;
    m_dwgSubtype = parsedSubtype;
    flags = parsedFlags;
    basePoint = parsedBasePoint;
    stawidth = parsedStartWidth;
    endwidth = parsedEndWidth;
    bulge = parsedBulge;
    tgdir = parsedTangentDirection;
    vindex1 = parsedIndex1;
    vindex2 = parsedIndex2;
    vindex3 = parsedIndex3;
    vindex4 = parsedIndex4;
    identifier = parsedIdentifier;
    return true;
}

bool DRW_Hatch::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    try {
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        solid = reader->getInt32();
        break;
    case 71:
        associative = reader->getInt32();
        break;
    case 72: {      /*edge type*/
        if (ispol){ // polyline path: 72 is the has-bulge flag. Do NOT fold it
            // into pline->flags — bit 0 there is the *closed* flag (set by code
            // 73), and the per-vertex bulges arrive via code 42 regardless. Some
            // writers (e.g. ezdxf MPOLYGON) emit 73 before 72; the old code let
            // 72 clear the closed bit 73 had just set, leaving the boundary open
            // so RS_Hatch::validate() rejected the area.
            break;
        }
        if (spline && !validateCurrentSplineDxf())
            return false;
        if (!loop
            || loop->objlist.size() >= static_cast<std::size_t>(kMaxHatchItems)
            || (m_dxfLoopEdgeCountExpected >= 0
                && loop->objlist.size()
                    >= static_cast<std::size_t>(m_dxfLoopEdgeCountExpected)))
            return false;
        const int edgeType = reader->getInt32();
        if (edgeType == 1){ //line
            addLine();
        } else if (edgeType == 2){ //arc
            addArc();
        } else if (edgeType == 3){ //elliptic arc
            addEllipse();
        } else if (edgeType == 4){ //spline
            addSpline();
        } else {
            return false;
        }
        break;
    }
    case 10:
        // Spline edge: 10 is a control-point x-coord.
        if (spline) {
            if (spline->controllist.size() >= static_cast<std::size_t>(kMaxHatchItems)
                || (m_dxfSplineControlCountExpected >= 0
                    && spline->controllist.size()
                        >= static_cast<std::size_t>(m_dxfSplineControlCountExpected)))
                return false;
            spline->controllist.push_back(std::make_shared<DRW_Coord>(reader->getDouble(), 0.0, 0.0));
            break;
        }
        if (pt) pt->basePoint.x = reader->getDouble();
        else if (pline) {
            if (pline->vertlist.size() >= static_cast<std::size_t>(kMaxHatchItems)
                || (m_dxfPolylineVertexCountExpected >= 0
                    && pline->vertlist.size()
                        >= static_cast<std::size_t>(m_dxfPolylineVertexCountExpected)))
                return false;
            plvert = pline->addVertex();
            plvert->x = reader->getDouble();
        } else if (m_dxfSeedPointsExpected < 0) {
            return DRW_Point::parseCode(code, reader);
        } else {
            // After group 98 the boundary path is closed; seed-point
            // coords arrive as group-10/20 pairs.
            if (seedPoints.size() >= static_cast<std::size_t>(m_dxfSeedPointsExpected))
                return false;
            DRW_Coord seed;
            seed.x = reader->getDouble();
            seedPoints.push_back(seed);
        }
        break;
    case 20:
        if (spline && !spline->controllist.empty()) {
            spline->controllist.back()->y = reader->getDouble();
            break;
        }
        if (pt) pt->basePoint.y = reader->getDouble();
        else if (plvert) plvert ->y = reader->getDouble();
        else if (m_dxfSeedPointsExpected >= 0) {
            if (seedPoints.empty())
                return false;
            seedPoints.back().y = reader->getDouble();
        } else {
            return DRW_Point::parseCode(code, reader);
        }
        break;
    case 11:
        // Spline edge: 11 is a fit-point x-coord.
        if (spline) {
            if (spline->fitlist.size() >= static_cast<std::size_t>(kMaxHatchItems)
                || (m_dxfSplineFitCountExpected >= 0
                    && spline->fitlist.size()
                        >= static_cast<std::size_t>(m_dxfSplineFitCountExpected)))
                return false;
            spline->fitlist.push_back(std::make_shared<DRW_Coord>(reader->getDouble(), 0.0, 0.0));
            break;
        }
        if (line) line->secPoint.x = reader->getDouble();
        else if (ellipse) ellipse->secPoint.x = reader->getDouble();
        break;
    case 21:
        if (spline && !spline->fitlist.empty()) {
            spline->fitlist.back()->y = reader->getDouble();
            break;
        }
        if (line) line->secPoint.y = reader->getDouble();
        else if (ellipse) ellipse->secPoint.y = reader->getDouble();
        break;
    case 12:
        if (spline) { spline->tgStart.x = reader->getDouble(); break; }
        break;
    case 22:
        if (spline) { spline->tgStart.y = reader->getDouble(); break; }
        break;
    case 13:
        if (spline) { spline->tgEnd.x = reader->getDouble(); break; }
        break;
    case 23:
        if (spline) { spline->tgEnd.y = reader->getDouble(); break; }
        break;
    case 40:
        // Spline edge: 40 is a knot value (occurs nknots times).
        if (spline) {
            if (spline->knotslist.size() >= static_cast<std::size_t>(kMaxHatchItems)
                || (m_dxfSplineKnotCountExpected >= 0
                    && spline->knotslist.size()
                        >= static_cast<std::size_t>(m_dxfSplineKnotCountExpected)))
                return false;
            spline->knotslist.push_back(reader->getDouble());
            break;
        }
        if (arc) arc->radious = reader->getDouble();
        else if (ellipse) ellipse->ratio = reader->getDouble();
        break;
    case 41:
        scale = reader->getDouble();
        break;
    case 42:
        // Spline edge: 42 is a per-control-point weight.
        if (spline) {
            if (spline->weightlist.size() >= static_cast<std::size_t>(kMaxHatchItems)
                || (m_dxfSplineControlCountExpected >= 0
                    && spline->weightlist.size()
                        >= static_cast<std::size_t>(m_dxfSplineControlCountExpected)))
                return false;
            spline->weightlist.push_back(reader->getDouble());
            break;
        }
        if (plvert) plvert ->bulge = reader->getDouble();
        break;
    case 50:
        if (arc) arc->staangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->staparam = reader->getDouble()/ARAD;
        break;
    case 51:
        if (arc) arc->endangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->endparam = reader->getDouble()/ARAD;
        break;
    case 47:
        pixelSize = reader->getDouble();
        break;
    case 52:
        angle = reader->getDouble();
        break;
    case 53: // pattern line angle — starts a new PatternLine record
        if (!validateCurrentPatternLineDxf())
            return false;
        if (m_dxfPatternLineCountExpected >= 0
            && patternLines.size() >= static_cast<std::size_t>(m_dxfPatternLineCountExpected))
            return false;
        if (patternLines.size() >= static_cast<std::size_t>(kMaxHatchItems))
            return false;
        patternLines.push_back(PatternLine());
        patternLines.back().angle = reader->getDouble();
        m_dxfPatternDashCountExpected = -1;
        m_dxfPatternDashCountSeen = false;
        break;
    case 43:
        if (!patternLines.empty()) patternLines.back().baseX = reader->getDouble();
        break;
    case 44:
        if (!patternLines.empty()) patternLines.back().baseY = reader->getDouble();
        break;
    case 45:
        if (!patternLines.empty()) patternLines.back().offsetX = reader->getDouble();
        break;
    case 46:
        if (!patternLines.empty()) patternLines.back().offsetY = reader->getDouble();
        break;
    case 79: // dash count — the 49s that follow will accumulate
        if (patternLines.empty() || m_dxfPatternDashCountSeen)
            return false;
        m_dxfPatternDashCountExpected = reader->getInt32();
        if (!isValidCount(m_dxfPatternDashCountExpected, kMaxHatchItems))
            return false;
        m_dxfPatternDashCountSeen = true;
        return DRW::reserve(patternLines.back().dashList, m_dxfPatternDashCountExpected);
    case 49:
        if (patternLines.empty()
            || patternLines.back().dashList.size() >= static_cast<std::size_t>(kMaxHatchItems)
            || (m_dxfPatternDashCountExpected >= 0
                && patternLines.back().dashList.size()
                    >= static_cast<std::size_t>(m_dxfPatternDashCountExpected)))
            return false;
        patternLines.back().dashList.push_back(reader->getDouble());
        break;
    case 73:
        // Spline edge: 73 is the rational flag (1 = rational).
        if (spline) {
            if (reader->getInt32()) spline->flags |= 0x4;
            break;
        }
        if (arc) arc->isccw = reader->getInt32();
        // polyline path: 73 is the is-closed flag -> set bit 0 only, leaving the
        // rest of pline->flags untouched (order-independent vs code 72).
        else if (pline) pline->flags = (pline->flags & ~1) | (reader->getInt32() ? 1 : 0);
        break;
    case 74:
        // Spline edge: 74 is the periodic flag (1 = periodic/closed).
        if (spline) {
            if (reader->getInt32()) spline->flags |= 0x2;
        }
        break;
    case 94:
        // Spline edge degree.
        if (spline) {
            spline->degree = reader->getInt32();
            return isValidCount(spline->degree, kMaxSplineDegree)
                && spline->degree != 0;
        }
        break;
    case 95:
        // Spline edge number of knots.
        if (spline) {
            if (m_dxfSplineKnotCountExpected >= 0)
                return false;
            spline->nknots = reader->getInt32();
            if (!isValidCount(spline->nknots, kMaxHatchItems))
                return false;
            m_dxfSplineKnotCountExpected = spline->nknots;
            return DRW::reserve(spline->knotslist, spline->nknots);
        }
        break;
    case 96:
        // Spline edge number of control points.
        if (spline) {
            if (m_dxfSplineControlCountExpected >= 0)
                return false;
            spline->ncontrol = reader->getInt32();
            if (!isValidCount(spline->ncontrol, kMaxHatchItems))
                return false;
            m_dxfSplineControlCountExpected = spline->ncontrol;
            return DRW::reserve(spline->controllist, spline->ncontrol);
        }
        break;
    case 97:
        if (spline) {
            if (!m_splineNfitSet) {
                // First 97 in this spline edge = fit-point count (nfit).
                spline->nfit = reader->getInt32();
                if (!isValidCount(spline->nfit, kMaxHatchItems))
                    return false;
                if (m_dxfSplineFitCountExpected >= 0)
                    return false;
                m_dxfSplineFitCountExpected = spline->nfit;
                if (!DRW::reserve(spline->fitlist, spline->nfit))
                    return false;
                if (spline->nfit == 0) {
                    if (!validateCurrentSplineDxf())
                        return false;
                    // No fit points or tangents follow; safe to clear spline
                    // so the next code-97 (loop boundary count) is not
                    // misinterpreted as another nfit.
                    spline.reset();
                } else {
                    m_splineNfitSet = true;
                }
            } else {
                // Second 97 while spline is active = loop boundary handle count.
                if (!validateCurrentSplineDxf())
                    return false;
                spline.reset();
                m_splineNfitSet = false;
                m_boundaryHandleCount = reader->getInt32();
                if (!isValidCount(m_boundaryHandleCount, kMaxHatchItems))
                    return false;
                if (m_dxfBoundaryHandleCountSeen)
                    return false;
                m_dxfBoundaryHandleCountSeen = true;
                if (m_boundaryHandleCount > 0 && loop
                    && !DRW::reserve(loop->m_boundaryHandles, m_boundaryHandleCount))
                    return false;
            }
            break;
        }
        // No active spline: this is the loop boundary handle count.
        m_splineNfitSet = false;
        if (m_dxfBoundaryHandleCountSeen)
            return false;
        m_boundaryHandleCount = reader->getInt32();
        if (!isValidCount(m_boundaryHandleCount, kMaxHatchItems))
            return false;
        m_dxfBoundaryHandleCountSeen = true;
        if (m_boundaryHandleCount > 0 && loop
            && !DRW::reserve(loop->m_boundaryHandles, m_boundaryHandleCount))
            return false;
        break;
    case 330:
        if (m_boundaryHandleCount > 0 && loop) {
            // getHandleString() converts the hex string to int for us.
            loop->m_boundaryHandles.push_back(
                static_cast<std::uint32_t>(reader->getHandleString()));
            --m_boundaryHandleCount;
            break;
        }
        if (m_dxfBoundaryHandleCountSeen)
            return false;
        return DRW_Point::parseCode(code, reader);
    case 75:
        hstyle = reader->getInt32();
        break;
    case 76:
        hpattern = reader->getInt32();
        break;
    case 77:
        doubleflag = reader->getInt32();
        break;
    case 78:
        if (m_dxfPatternLineCountSeen)
            return false;
        deflines = reader->getInt32();
        if (!isValidCount(deflines, kMaxHatchItems))
            return false;
        m_dxfPatternLineCountExpected = deflines;
        m_dxfPatternLineCountSeen = true;
        if (patternLines.size() > static_cast<std::size_t>(deflines))
            return false;
        break;
    case 91:
        if (m_dxfLoopCountSeen)
            return false;
        loopsnum = reader->getInt32();
        if (!isValidCount(loopsnum, kMaxHatchItems))
            return false;
        m_dxfLoopCountExpected = loopsnum;
        m_dxfLoopCountSeen = true;
        return DRW::reserve( looplist, loopsnum);
    case 92: {
        if (!validateCurrentBoundaryPathDxf())
            return false;
        if (m_dxfLoopCountExpected >= 0
            && looplist.size() >= static_cast<std::size_t>(m_dxfLoopCountExpected))
            return false;
        clearEntities();
        m_dxfLoopEdgeCountExpected = -1;
        m_dxfLoopEdgeCountSeen = false;
        m_dxfPolylineVertexCountExpected = -1;
        m_dxfPolylineVertexCountSeen = false;
        m_dxfBoundaryHandleCountSeen = false;
        m_boundaryHandleCount = 0;
        loop = std::make_shared<DRW_HatchLoop>(reader->getInt32());
        looplist.push_back(loop);
        if (reader->getInt32() & 2) {
            ispol = true;
            clearEntities();
            pline = std::make_shared<DRW_LWPolyline>();
            loop->objlist.push_back(pline);
        } else ispol = false;
        break;
    }
    case 93:
        if (pline) {
            if (m_dxfPolylineVertexCountSeen)
                return false;
            pline->vertexnum = reader->getInt32();
            if (!isValidCount(pline->vertexnum, kMaxHatchItems))
                return false;
            m_dxfPolylineVertexCountExpected = pline->vertexnum;
            m_dxfPolylineVertexCountSeen = true;
        } else if (loop) {
            if (m_dxfLoopEdgeCountSeen)
                return false;
            loop->numedges = reader->getInt32();//aqui reserve
            if (!isValidCount(loop->numedges, kMaxHatchItems))
                return false;
            if (loop->objlist.size() > static_cast<std::size_t>(loop->numedges))
                return false;
            m_dxfLoopEdgeCountExpected = loop->numedges;
            m_dxfLoopEdgeCountSeen = true;
        }
        break;
    case 98: { // seed-point count; coords follow as group-10/20 pairs
        if (!validateCurrentBoundaryPathDxf() || !validateCurrentPatternLineDxf())
            return false;
        if (m_dxfSeedPointCountSeen)
            return false;
        clearEntities();
        const int count = reader->getInt32();
        if (!isValidCount(count, kMaxHatchItems)
            || !DRW::reserve(seedPoints, count))
            return false;
        m_dxfSeedPointsExpected = count;
        m_dxfSeedPointCountSeen = true;
        break;
    }
    case 450:
        isGradient = reader->getInt32();
        break;
    case 451:
        gradReserved = reader->getInt32();
        break;
    case 452:
        singleColor = reader->getInt32();
        break;
    case 453: {
        if (m_dxfGradientColorCountSeen)
            return false;
        const int n = reader->getInt32();
        if (!isValidCount(n, kMaxHatchItems)
            || !DRW::reserve(gradColors, n))
            return false;
        m_dxfGradientColorCountExpected = n;
        m_dxfGradientColorCountSeen = true;
        break;
    }
    case 460:
        gradAngle = reader->getDouble();
        break;
    case 461:
        gradShift = reader->getDouble();
        break;
    case 462:
        gradTint = reader->getDouble();
        break;
    case 463: {
        if (m_dxfGradientColorCountExpected >= 0
            && gradColors.size() >= static_cast<std::size_t>(m_dxfGradientColorCountExpected))
            return false;
        if (gradColors.size() >= static_cast<std::size_t>(kMaxHatchItems))
            return false;
        DRW_Hatch::GradientStop stop;
        stop.value = reader->getDouble();
        gradColors.push_back(stop);
        break;
    }
    case 421:
        if (!gradColors.empty())
            gradColors.back().rgb = reader->getInt32();
        break;
    case 63:
        if (!gradColors.empty())
            gradColors.back().aciColor = reader->getInt32();
        else
            return DRW_Point::parseCode(code, reader);
        break;
    case 431:
        if (!gradColors.empty())
            gradColors.back().colorMethod = reader->getInt32();
        break;
    case 432:
        if (!gradColors.empty())
            gradColors.back().colorName = reader->getUtf8String();
        break;
    case 433:
        if (!gradColors.empty())
            gradColors.back().colorBookName = reader->getUtf8String();
        break;
    case 470:
        gradName = reader->getUtf8String();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
    }
    catch (...) {
        return false;
    }
}

bool DRW_Hatch::validateCurrentSplineDxf() const {
    if (!spline)
        return true;
    if (m_dxfSplineKnotCountExpected >= 0
        && spline->knotslist.size()
            != static_cast<std::size_t>(m_dxfSplineKnotCountExpected))
        return false;
    if (m_dxfSplineControlCountExpected >= 0
        && spline->controllist.size()
            != static_cast<std::size_t>(m_dxfSplineControlCountExpected))
        return false;
    if (m_dxfSplineFitCountExpected >= 0
        && spline->fitlist.size()
            != static_cast<std::size_t>(m_dxfSplineFitCountExpected))
        return false;
    if (spline->flags & 0x4)
        return m_dxfSplineControlCountExpected < 0
            || spline->weightlist.size()
                == static_cast<std::size_t>(m_dxfSplineControlCountExpected);
    return spline->weightlist.empty();
}

bool DRW_Hatch::validateCurrentPatternLineDxf() const {
    if (patternLines.empty() || m_dxfPatternDashCountExpected < 0)
        return true;
    return patternLines.back().dashList.size()
        == static_cast<std::size_t>(m_dxfPatternDashCountExpected);
}

bool DRW_Hatch::validateCurrentBoundaryPathDxf() const {
    if (m_boundaryHandleCount != 0 || !validateCurrentSplineDxf())
        return false;
    if (!loop)
        return true;
    if (ispol) {
        std::shared_ptr<DRW_LWPolyline> polyline = pline;
        if (!polyline && loop->objlist.size() == 1)
            polyline = std::dynamic_pointer_cast<DRW_LWPolyline>(loop->objlist.front());
        if (!polyline)
            return false;
        return !m_dxfPolylineVertexCountSeen
            || polyline->vertlist.size()
                == static_cast<std::size_t>(m_dxfPolylineVertexCountExpected);
    }
    return !m_dxfLoopEdgeCountSeen
        || loop->objlist.size()
            == static_cast<std::size_t>(m_dxfLoopEdgeCountExpected);
}

bool DRW_Hatch::validateDxf() const {
    if (!validateCurrentBoundaryPathDxf()
        || !validateCurrentPatternLineDxf())
        return false;
    if (m_dxfLoopCountExpected >= 0
        && looplist.size() != static_cast<std::size_t>(m_dxfLoopCountExpected))
        return false;
    if (m_dxfPatternLineCountExpected >= 0
        && patternLines.size()
            != static_cast<std::size_t>(m_dxfPatternLineCountExpected))
        return false;
    if (m_dxfGradientColorCountExpected >= 0
        && gradColors.size()
            != static_cast<std::size_t>(m_dxfGradientColorCountExpected))
        return false;
    return m_dxfSeedPointsExpected < 0
        || seedPoints.size() == static_cast<std::size_t>(m_dxfSeedPointsExpected);
}

bool DRW_MPolygon::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // MPOLYGON shares HATCH's boundary/pattern/gradient codes, so delegate those
    // to DRW_Hatch::parseCode. It adds a trailer that plain HATCH never emits:
    //   63 / 421 / 430  fill color (ACI / RGB / book-name) — the filled area's
    //                   color, which may differ from the boundary outline color;
    //   11 / 21         boundary x-direction vector (no render impact; left to
    //                   the base, which ignores it outside an edge context);
    //   99              count of degenerate boundary paths.
    // 63/421 are also gradient sub-codes in HATCH, so only claim them here when no
    // gradient is being accumulated (gradColors empty) — otherwise defer to base.
    switch (code) {
    case 63:
        if (gradColors.empty()) { fillColorAci = reader->getInt32(); return true; }
        break;
    case 421:
        if (gradColors.empty()) { fillColorRgb = reader->getInt32(); return true; }
        break;
    case 430:
        fillColorName = reader->getUtf8String();
        return true;
    case 99:
        degenerateLoops = reader->getInt32();
        return true;
    default:
        break;
    }
    return DRW_Hatch::parseCode(code, reader);
}

// DRW_MPolygon::parseDwg — AcDbMPolygon DWG body.
// Layout mirrors HATCH except (per ACadSharp MPolygon / libreDWG dwg.spec):
//   * a leading BS `style` (DXF group 75) precedes the gradient block, and
//   * the trailer is a fill CMC + boundary x-direction (2RD) + degenerate-path
//     count (BL) instead of HATCH's pixel-size + seed points.
// The gradient/elevation/extrusion/name/solid/associative prologue and the whole
// boundary-loop body are identical, so they reuse DRW_Hatch::parseDwgBoundaryData.
// DWG runtime coverage uses testdata/mpolygon_solid.dwg, ODA-synthesized from
// the ezdxf-verified inline DXF in mpolygon_tests.cpp and confirmed with the
// dwg-parser oracle.
bool DRW_MPolygon::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    fillColorAci = 0;
    fillColorRgb = -1;
    fillColorName.clear();
    xDirX = 0.0;
    xDirY = 0.0;
    degenerateLoops = 0;
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &stringProbe : buf;
    std::uint32_t totalBoundItems = 0;
    bool havePixelSize = false;
    if (!DRW_Entity::parseDwg(
            version, buf, version > DRW::AC1018 ? &stringProbe : nullptr, bs)
        || !buf->isGood() || !sBuf->isGood())
        return fail();
    DRW_DBG("\n***************************** parsing mpolygon *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018) {
        stringStartBit = currentDwgBit(sBuf);
        if (version > DRW::AC1021) {
            std::uint64_t ignoredBodyEndBit = 0;
            if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                      ignoredBodyEndBit, &stringEndBit))
                return fail();
        }
    }

    // Leading BS style (group 75) — read once here and again after the loops
    // below (HATCH has only the latter); matches the reference parser, which
    // discards this first read.
    std::uint16_t parsedLeadingStyle = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedLeadingStyle))
        return fail();
    hstyle = parsedLeadingStyle;

    if (version > DRW::AC1015) { //2004+ gradient (same layout as HATCH)
        if (!readBoundedHatchGradient(
                *buf, sBuf, bodyEndBit, version, isGradient, gradReserved,
                gradAngle, gradShift, singleColor, gradTint, gradName,
                gradColors, stringEndBit))
            return fail();
    }
    if (!readBoundedBitDouble(*buf, bodyEndBit, basePoint.z)
        || !readBoundedBitCoord(*buf, bodyEndBit, extPoint)
        || !std::isfinite(basePoint.z)
        || !std::isfinite(extPoint.x)
        || !std::isfinite(extPoint.y)
        || !std::isfinite(extPoint.z))
        return fail();
    UTF8STRING parsedName;
    if (version > DRW::AC1018 && stringStartBit >= stringEndBit) {
        parsedName.clear();
    } else if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                        parsedName)) {
        return fail();
    }
    bool parsedSolid = false;
    bool parsedAssociative = false;
    if (!sBuf->isGood()
        || currentDwgBit(sBuf) > stringEndBit
        || !readBoundedBit(*buf, bodyEndBit, parsedSolid)
        || !readBoundedBit(*buf, bodyEndBit, parsedAssociative))
        return fail();
    solid = parsedSolid;
    associative = parsedAssociative;
    name = std::move(parsedName);

    if (!parseDwgBoundaryData(version, buf, totalBoundItems, havePixelSize))
        return fail();

    std::uint16_t parsedStyle = 0;
    std::uint16_t parsedPattern = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedStyle)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedPattern))
        return fail();
    hstyle = parsedStyle;
    hpattern = parsedPattern;
    if (!solid){
        double parsedAngle = 0.0;
        double parsedScale = 0.0;
        bool parsedDoubleFlag = false;
        if (!readBoundedBitDouble(*buf, bodyEndBit, parsedAngle)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedScale)
            || !readBoundedBit(*buf, bodyEndBit, parsedDoubleFlag)
            || !std::isfinite(parsedAngle)
            || !std::isfinite(parsedScale))
            return fail();
        angle = parsedAngle;
        scale = parsedScale;
        doubleflag = parsedDoubleFlag;
        std::uint32_t parsedDeflines = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 12,
                                parsedDeflines))
            return fail();
        if (!DRW::reserve(patternLines, parsedDeflines))
            return fail();
        deflines = static_cast<int>(parsedDeflines);
        for (std::uint32_t i = 0 ; i < parsedDeflines; ++i){
            PatternLine patternLine;
            if (!readBoundedBitDouble(*buf, bodyEndBit, patternLine.angle)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.baseX)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.baseY)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.offsetX)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.offsetY)
                || !std::isfinite(patternLine.angle)
                || !std::isfinite(patternLine.baseX)
                || !std::isfinite(patternLine.baseY)
                || !std::isfinite(patternLine.offsetX)
                || !std::isfinite(patternLine.offsetY))
                return fail();
            std::uint32_t numDashL = 0;
            if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 2,
                                    numDashL)
                || !proxyEntityHasBits(
                    *buf, bodyEndBit, static_cast<std::uint64_t>(numDashL) * 2u)
                || !DRW::reserve(patternLine.dashList, numDashL))
                return fail();
            for (std::uint32_t d = 0 ; d < numDashL; ++d) {
                double dash = 0.0;
                if (!readBoundedBitDouble(*buf, bodyEndBit, dash)
                    || !std::isfinite(dash))
                    return fail();
                patternLine.dashList.push_back(dash);
            }
            patternLines.push_back(std::move(patternLine));
        }
    }

    // MPOLYGON trailer (differs from HATCH): fill CMC + x-direction + degenerate
    // path count. No pixel size / seed points here.
    std::int32_t rgb = -1;
    UTF8STRING colName;
    std::uint32_t parsedFillColor = 0;
    if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                            parsedFillColor, &rgb, nullptr, &colName))
        return fail();
    fillColorAci = static_cast<int>(parsedFillColor);
    fillColorRgb = rgb;
    fillColorName = colName;
    DRW_Coord xdir;
    if (!readBounded2RawDouble(*buf, bodyEndBit, xdir)
        || !std::isfinite(xdir.x) || !std::isfinite(xdir.y))
        return fail();
    xDirX = xdir.x;
    xDirY = xdir.y;
    std::uint32_t parsedDegenerateLoops = 0;
    if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 0,
                            parsedDegenerateLoops))
        return fail();
    degenerateLoops = static_cast<std::int32_t>(parsedDegenerateLoops);

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();
    for (std::uint32_t i = 0 ; i < totalBoundItems; ++i) {
        dwgHandle boundaryHandle;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                                  boundaryHandle))
            return fail();
    }
    if (!handleProbe.isGood())
        return fail();

    *sourceBuf = handleProbe;
    return true;
}

// Shared DWG boundary-loop reader for HATCH and MPOLYGON (ODA §20.4.36).
// Reads the loop count and, per loop, the derived-boundary flag plus its edge
// list or polyline. Accumulates the running boundary-source-handle total and
// whether any loop is derived (needs a trailing pixel size). Extracted from
// DRW_Hatch::parseDwg so DRW_MPolygon::parseDwg reuses the identical body while
// supplying its own differing leading (BS style) and trailing (fill CMC +
// x-direction + degenerate count) field order.
bool validateDwgHatchData(const DRW_Hatch& hatch) {
    if (hatch.looplist.size() > static_cast<std::size_t>(kMaxHatchItems)
        || hatch.patternLines.size() > static_cast<std::size_t>(kMaxHatchItems)
        || hatch.gradColors.size() > static_cast<std::size_t>(kMaxHatchItems)
        || hatch.seedPoints.size() > static_cast<std::size_t>(kMaxHatchItems))
        return false;

    for (const auto& loop : hatch.looplist) {
        if (!loop
            || loop->objlist.size() > static_cast<std::size_t>(kMaxHatchItems)
            || loop->m_boundaryHandles.size() > static_cast<std::size_t>(kMaxHatchItems))
            return false;

        if ((loop->type & 2) != 0) {
            if (loop->objlist.size() != 1)
                return false;
            const auto* polyline =
                dynamic_cast<const DRW_LWPolyline*>(loop->objlist.front().get());
            if (!polyline || polyline->vertlist.size() > static_cast<std::size_t>(kMaxHatchItems))
                return false;
            for (const auto& vertex : polyline->vertlist) {
                if (!vertex)
                    return false;
            }
            continue;
        }

        for (const auto& entity : loop->objlist) {
            if (dynamic_cast<const DRW_Line*>(entity.get())
                || dynamic_cast<const DRW_Arc*>(entity.get())
                || dynamic_cast<const DRW_Ellipse*>(entity.get())) {
                continue;
            }
            const auto* spline = dynamic_cast<const DRW_Spline*>(entity.get());
            if (!spline
                || spline->knotslist.size() > static_cast<std::size_t>(kMaxHatchItems)
                || spline->controllist.size() > static_cast<std::size_t>(kMaxHatchItems)
                || spline->fitlist.size() > static_cast<std::size_t>(kMaxHatchItems))
                return false;
            for (const auto& point : spline->controllist) {
                if (!point)
                    return false;
            }
            for (const auto& point : spline->fitlist) {
                if (!point)
                    return false;
            }
        }
    }

    for (const auto& line : hatch.patternLines) {
        if (line.dashList.size() > static_cast<std::size_t>(kMaxHatchItems))
            return false;
    }
    return true;
}

bool DRW_MPolygon::encodeDwg(DRW::Version version, dwgBufferW *buf,
                             std::uint32_t bs, dwgBufferW *strBuf,
                             dwgBufferW *handleBuf) {
    (void)bs;
    if (!validateDwgHatchData(*this)
        || !isValidCount(degenerateLoops, kMaxHatchItems))
        return false;
    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    dwgBufferW *sb = strBuf ? strBuf : buf;

    // AcDbMPolygon has a leading style field before the HATCH-like gradient
    // prologue. The same style is emitted again after the boundary data.
    buf->putBitShort(static_cast<std::uint16_t>(hstyle));
    encodeDwgGradientData(version, buf, sb);

    buf->putBitDouble(basePoint.z);
    buf->put3BitDouble(extPoint);
    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>(solid));
    buf->putBit(static_cast<std::uint8_t>(associative));
    if (!encodeDwgBoundaryData(version, buf)) return false;

    buf->putBitShort(static_cast<std::uint16_t>(hstyle));
    buf->putBitShort(static_cast<std::uint16_t>(hpattern));

    if (!solid) {
        buf->putBitDouble(angle);
        buf->putBitDouble(scale);
        buf->putBit(static_cast<std::uint8_t>(doubleflag));
        buf->putBitShort(static_cast<std::uint16_t>(patternLines.size()));
        for (const PatternLine& pl : patternLines) {
            buf->putBitDouble(pl.angle);
            buf->putBitDouble(pl.baseX);
            buf->putBitDouble(pl.baseY);
            buf->putBitDouble(pl.offsetX);
            buf->putBitDouble(pl.offsetY);
            buf->putBitShort(static_cast<std::uint16_t>(pl.dashList.size()));
            for (double dash : pl.dashList)
                buf->putBitDouble(dash);
        }
    }

    buf->putCmColor(version,
                    static_cast<std::uint16_t>(fillColorAci),
                    fillColorRgb,
                    fillColorName,
                    {},
                    sb);
    buf->put2RawDouble(DRW_Coord{xDirX, xDirY, 0.0});
    buf->putBitLong(degenerateLoops);

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_Hatch::parseDwgBoundaryData(DRW::Version version, dwgBuffer *buf,
                                     std::uint32_t &totalBoundItems, bool &havePixelSize) {
    try {
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint32_t parsedLoopCount = 0;
    if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 4,
                            parsedLoopCount))
        return false;
    if (!DRW::reserve(looplist, parsedLoopCount))
        return false;
    loopsnum = static_cast<std::int32_t>(parsedLoopCount);
    DRW_DBG("solid: "); DRW_DBG(solid); DRW_DBG(" associative: "); DRW_DBG(associative);
    DRW_DBG(" loopsnum: "); DRW_DBG(loopsnum); DRW_DBG("\n");

    //read loops
    for (std::uint32_t i = 0 ; i < parsedLoopCount; ++i){
        std::int32_t loopType = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, loopType)
            || loopType < 0)
            return false;
        loop = std::make_shared<DRW_HatchLoop>(loopType);
        havePixelSize = havePixelSize || ((loop->type & 4) != 0);
        DRW_DBG(" loop["); DRW_DBG(i); DRW_DBG("] type: "); DRW_DBG(loop->type);
        if (!(loop->type & 2)){ //Not polyline
            std::uint32_t numPathSeg = 0;
            if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 8,
                                    numPathSeg))
                return false;
            if (!DRW::reserve(loop->objlist, numPathSeg))
                return false;
            DRW_DBG(" numPathSeg: "); DRW_DBG(numPathSeg); DRW_DBG("\n");
            for (std::uint32_t j = 0; j<numPathSeg;++j){
                std::uint8_t typePath = 0;
                if (!readBoundedRawChar8(*buf, bodyEndBit, typePath)
                    || typePath < 1 || typePath > 4)
                    return false;
                DRW_DBG("  seg["); DRW_DBG(j); DRW_DBG("] typePath: "); DRW_DBG(typePath); DRW_DBG("\n");
                if (typePath == 1){ //line
                    DRW_Coord basePoint;
                    DRW_Coord secPoint;
                    if (!readBounded2RawDouble(*buf, bodyEndBit, basePoint)
                        || !readBounded2RawDouble(*buf, bodyEndBit, secPoint))
                        return false;
                    addLine();
                    line->basePoint = basePoint;
                    line->secPoint = secPoint;
                } else if (typePath == 2){ //circle arc
                    DRW_Coord basePoint;
                    double radius = 0.0;
                    double startAngle = 0.0;
                    double endAngle = 0.0;
                    bool isCcw = false;
                    if (!readBounded2RawDouble(*buf, bodyEndBit, basePoint)
                        || !readBoundedBitDouble(*buf, bodyEndBit, radius)
                        || !readBoundedBitDouble(*buf, bodyEndBit, startAngle)
                        || !readBoundedBitDouble(*buf, bodyEndBit, endAngle)
                        || !readBoundedBit(*buf, bodyEndBit, isCcw)
                        || !std::isfinite(basePoint.x)
                        || !std::isfinite(basePoint.y)
                        || !std::isfinite(radius)
                        || !std::isfinite(startAngle)
                        || !std::isfinite(endAngle))
                        return false;
                    addArc();
                    arc->basePoint = basePoint;
                    arc->radious = radius;
                    arc->staangle = startAngle;
                    arc->endangle = endAngle;
                    arc->isccw = isCcw;
                } else if (typePath == 3){ //ellipse arc
                    DRW_Coord basePoint;
                    DRW_Coord secPoint;
                    double ratio = 0.0;
                    double startParam = 0.0;
                    double endParam = 0.0;
                    bool isCcw = false;
                    if (!readBounded2RawDouble(*buf, bodyEndBit, basePoint)
                        || !readBounded2RawDouble(*buf, bodyEndBit, secPoint)
                        || !readBoundedBitDouble(*buf, bodyEndBit, ratio)
                        || !readBoundedBitDouble(*buf, bodyEndBit, startParam)
                        || !readBoundedBitDouble(*buf, bodyEndBit, endParam)
                        || !readBoundedBit(*buf, bodyEndBit, isCcw)
                        || !std::isfinite(basePoint.x)
                        || !std::isfinite(basePoint.y)
                        || !std::isfinite(secPoint.x)
                        || !std::isfinite(secPoint.y)
                        || !std::isfinite(ratio)
                        || !std::isfinite(startParam)
                        || !std::isfinite(endParam))
                        return false;
                    addEllipse();
                    ellipse->basePoint = basePoint;
                    ellipse->secPoint = secPoint;
                    ellipse->ratio = ratio;
                    ellipse->staparam = startParam;
                    ellipse->endparam = endParam;
                    ellipse->isccw = isCcw;
                } else if (typePath == 4){ //spline
                    std::int32_t degree = 0;
                    bool isRational = false;
                    bool isPeriodic = false;
                    if (!readBoundedBitLong(*buf, bodyEndBit, degree)
                        || !isValidSplineDegree(degree)
                        || !readBoundedBit(*buf, bodyEndBit, isRational)
                        || !readBoundedBit(*buf, bodyEndBit, isPeriodic))
                        return false;
                    std::uint32_t knotCount = 0;
                    if (!readTableBodyCount(version, buf, objSize,
                                            kMaxHatchItems, 2, knotCount)) {
                        return false;
                    }
                    std::uint32_t controlCount = 0;
                    if (!readTableBodyCount(version, buf, objSize,
                                            kMaxHatchItems, 128, controlCount)) {
                        return false;
                    }
                    DRW_Spline parsedSpline;
                    parsedSpline.degree = degree;
                    parsedSpline.nknots = static_cast<std::int32_t>(knotCount);
                    parsedSpline.ncontrol = static_cast<std::int32_t>(controlCount);
                    parsedSpline.flags = (isRational ? 4 : 0)
                        | (isPeriodic ? 2 : 0);
                    if (!DRW::reserve(parsedSpline.knotslist, knotCount)
                        || !DRW::reserve(parsedSpline.controllist, controlCount))
                        return false;
                    for (std::uint32_t j = 0; j < knotCount;++j){
                        double knot = 0.0;
                        if (!readBoundedBitDouble(*buf, bodyEndBit, knot)
                            || !std::isfinite(knot))
                            return false;
                        parsedSpline.knotslist.push_back(knot);
                    }
                    for (std::uint32_t j = 0; j < controlCount;++j){
                        DRW_Coord control;
                        if (!readBounded2RawDouble(*buf, bodyEndBit, control)
                            || !std::isfinite(control.x)
                            || !std::isfinite(control.y))
                            return false;
                        if (isRational) {
                            if (!readBoundedBitDouble(*buf, bodyEndBit, control.z)
                                || !std::isfinite(control.z))
                                return false;
                        }
                        parsedSpline.controllist.push_back(
                            std::make_shared<DRW_Coord>(control));
                    }
                    if (version > DRW::AC1021) { //2010+
                        std::uint32_t fitCount = 0;
                        if (!readTableBodyCount(version, buf, objSize,
                                                kMaxHatchItems, 128, fitCount))
                            return false;
                        parsedSpline.nfit = static_cast<std::int32_t>(fitCount);
                        // Fit points AND the start/end tangents are present only
                        // when nfit > 0 (matches ACadSharp's `if (nfitPoints > 0)`).
                        // Reading the two tangents unconditionally on an nfit==0
                        // spline edge over-runs the entity body and fails the parse
                        // (e.g. svg/export_sample.dwg: a degree-3 non-rational
                        // spline boundary edge with 9 control points / 0 fit points).
                        if (parsedSpline.nfit > 0) {
                            if (!DRW::reserve(parsedSpline.fitlist, fitCount)) {
                                return false;
                            }
                            for (std::uint32_t j = 0; j < fitCount;++j){
                                DRW_Coord fit;
                                if (!readBounded2RawDouble(*buf, bodyEndBit, fit)
                                    || !std::isfinite(fit.x)
                                    || !std::isfinite(fit.y))
                                    return false;
                                parsedSpline.fitlist.push_back(
                                    std::make_shared<DRW_Coord>(fit));
                            }
                            if (!readBounded2RawDouble(*buf, bodyEndBit,
                                                       parsedSpline.tgStart)
                                || !readBounded2RawDouble(*buf, bodyEndBit,
                                                          parsedSpline.tgEnd)
                                || !std::isfinite(parsedSpline.tgStart.x)
                                || !std::isfinite(parsedSpline.tgStart.y)
                                || !std::isfinite(parsedSpline.tgEnd.x)
                                || !std::isfinite(parsedSpline.tgEnd.y))
                                return false;
                        }
                    }
                    addSpline();
                    *spline = std::move(parsedSpline);
                }
            }
        } else { //end not pline, start polyline
            if (!proxyEntityHasBits(*buf, bodyEndBit, 2))
                return false;
            bool asBulge = false;
            bool closed = false;
            if (!readBoundedBit(*buf, bodyEndBit, asBulge)
                || !readBoundedBit(*buf, bodyEndBit, closed))
                return false;
            std::uint32_t numVert = 0;
            if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems,
                                    128, numVert))
                return false;
            if (!DRW::reserve(loop->objlist, 1))
                return false;
            DRW_DBG(" asBulge: "); DRW_DBG(asBulge); DRW_DBG(" closed: "); DRW_DBG(closed);
            DRW_DBG(" numVert: "); DRW_DBG(numVert); DRW_DBG("\n");
            std::vector<DRW_Vertex2D> vertices;
            if (!DRW::reserve(vertices, numVert))
                return false;
            for (std::uint32_t j = 0; j<numVert;++j){
                DRW_Vertex2D v;
                if (!readBoundedRawDouble(*buf, bodyEndBit, v.x)
                    || !readBoundedRawDouble(*buf, bodyEndBit, v.y)
                    || !std::isfinite(v.x) || !std::isfinite(v.y))
                    return false;
                if (asBulge
                    && (!readBoundedBitDouble(*buf, bodyEndBit, v.bulge)
                        || !std::isfinite(v.bulge)))
                    return false;
                vertices.push_back(v);
            }
            pline = std::make_shared<DRW_LWPolyline>();
            pline->flags = closed;
            for (const DRW_Vertex2D& vertex : vertices)
                pline->addVertex(vertex);
            loop->objlist.push_back(pline);
        }//end polyline
        loop->update();
        std::uint32_t boundaryHandleCount = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 0,
                                boundaryHandleCount)
            || totalBoundItems > kMaxHatchItems - boundaryHandleCount)
            return false;
        looplist.push_back(loop);
        totalBoundItems += boundaryHandleCount;
        DRW_DBG(" totalBoundItems: "); DRW_DBG(totalBoundItems);
    } //end read loops
    return buf->isGood();
    } catch (...) {
        return false;
    }
}

bool DRW_Hatch::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &stringProbe : buf;
    std::uint32_t totalBoundItems = 0;
    bool havePixelSize = false;

    bool ret = DRW_Entity::parseDwg(
        version, buf, version > DRW::AC1018 ? &stringProbe : nullptr, bs);
    if (!ret || !buf->isGood() || !sBuf->isGood())
        return fail();
    DRW_DBG("\n***************************** parsing hatch *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018) {
        stringStartBit = currentDwgBit(sBuf);
        if (version > DRW::AC1021) {
            std::uint64_t ignoredBodyEndBit = 0;
            if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                      ignoredBodyEndBit, &stringEndBit))
                return fail();
        }
    }

    if (version > DRW::AC1015) { //2004+
        if (!readBoundedHatchGradient(
                *buf, sBuf, bodyEndBit, version, isGradient, gradReserved,
                gradAngle, gradShift, singleColor, gradTint, gradName,
                gradColors, stringEndBit))
            return fail();
        DRW_DBG("\ngradient name: "); DRW_DBG(gradName.c_str()); DRW_DBG("\n");
    }
    DRW_Coord parsedBasePoint = basePoint;
    DRW_Coord parsedExtPoint;
    UTF8STRING parsedName;
    bool parsedSolid = false;
    bool parsedAssociative = false;
    if (!readBoundedBitDouble(*buf, bodyEndBit, parsedBasePoint.z)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedExtPoint)
        || !std::isfinite(parsedBasePoint.z)
        || !std::isfinite(parsedExtPoint.x)
        || !std::isfinite(parsedExtPoint.y)
        || !std::isfinite(parsedExtPoint.z))
        return fail();
    DRW_DBG("base point: "); DRW_DBGPT(parsedBasePoint.x, parsedBasePoint.y,
                                        parsedBasePoint.z);
    DRW_DBG("\nextrusion: "); DRW_DBGPT(parsedExtPoint.x, parsedExtPoint.y,
                                          parsedExtPoint.z);
    if (version > DRW::AC1018 && stringStartBit >= stringEndBit) {
        parsedName.clear();
    } else if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                        parsedName)) {
        return fail();
    }
    if (!sBuf->isGood()
        || currentDwgBit(sBuf) > stringEndBit
        || !readBoundedBit(*buf, bodyEndBit, parsedSolid)
        || !readBoundedBit(*buf, bodyEndBit, parsedAssociative))
        return fail();
    basePoint = parsedBasePoint;
    extPoint = parsedExtPoint;
    name = std::move(parsedName);
    solid = parsedSolid;
    associative = parsedAssociative;
    DRW_DBG("\nhatch pattern name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    if (!parseDwgBoundaryData(version, buf, totalBoundItems, havePixelSize))
        return fail();

    std::uint16_t parsedStyle = 0;
    std::uint16_t parsedPattern = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedStyle)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedPattern))
        return fail();
    hstyle = parsedStyle;
    hpattern = parsedPattern;
    DRW_DBG("\nhatch style: "); DRW_DBG(hstyle); DRW_DBG(" pattern type"); DRW_DBG(hpattern);
    if (!solid){
        double parsedAngle = 0.0;
        double parsedScale = 0.0;
        bool parsedDoubleFlag = false;
        if (!readBoundedBitDouble(*buf, bodyEndBit, parsedAngle)
            || !readBoundedBitDouble(*buf, bodyEndBit, parsedScale)
            || !readBoundedBit(*buf, bodyEndBit, parsedDoubleFlag)
            || !std::isfinite(parsedAngle)
            || !std::isfinite(parsedScale))
            return fail();
        angle = parsedAngle;
        scale = parsedScale;
        doubleflag = parsedDoubleFlag;
        std::uint32_t parsedDeflines = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 12,
                                parsedDeflines))
            return fail();
        deflines = static_cast<int>(parsedDeflines);
        for (std::uint32_t i = 0 ; i < parsedDeflines; ++i){
            PatternLine patternLine;
            if (!readBoundedBitDouble(*buf, bodyEndBit, patternLine.angle)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.baseX)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.baseY)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.offsetX)
                || !readBoundedBitDouble(*buf, bodyEndBit, patternLine.offsetY)
                || !std::isfinite(patternLine.angle)
                || !std::isfinite(patternLine.baseX)
                || !std::isfinite(patternLine.baseY)
                || !std::isfinite(patternLine.offsetX)
                || !std::isfinite(patternLine.offsetY))
                return fail();
            std::uint32_t numDashL = 0;
            if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 2,
                                    numDashL)
                || !proxyEntityHasBits(
                    *buf, bodyEndBit, static_cast<std::uint64_t>(numDashL) * 2u)
                || !DRW::reserve(patternLine.dashList, numDashL))
                return fail();
            DRW_DBG("\ndef line: "); DRW_DBG(patternLine.angle); DRW_DBG(",");
            DRW_DBG(patternLine.baseX); DRW_DBG(","); DRW_DBG(patternLine.baseY);
            DRW_DBG(","); DRW_DBG(patternLine.offsetX); DRW_DBG(",");
            DRW_DBG(patternLine.offsetY);
            for (std::uint32_t i = 0 ; i < numDashL; ++i){
                double lengthL = 0.0;
                if (!readBoundedBitDouble(*buf, bodyEndBit, lengthL)
                    || !std::isfinite(lengthL))
                    return fail();
                patternLine.dashList.push_back(lengthL);
                DRW_DBG(","); DRW_DBG(lengthL);
            }
            patternLines.push_back(std::move(patternLine));
        }//end deflines
    } //end not solid

    if (havePixelSize){
        if (!readBoundedBitDouble(*buf, bodyEndBit, pixelSize)
            || !std::isfinite(pixelSize))
            return fail();
        DRW_DBG("\npixel size: "); DRW_DBG(pixelSize);
    }
    std::uint32_t numSeedPoints = 0;
    if (!readTableBodyCount(version, buf, objSize, kMaxHatchItems, 128,
                            numSeedPoints)
        || !proxyEntityHasBits(
            *buf, bodyEndBit, static_cast<std::uint64_t>(numSeedPoints) * 128u)
        || !DRW::reserve(seedPoints, numSeedPoints))
        return fail();
    DRW_DBG("\nnum Seed Points  "); DRW_DBG(numSeedPoints);
    for (std::uint32_t i = 0 ; i < numSeedPoints; ++i){
        DRW_Coord seedPt;
        if (!readBounded2RawDouble(*buf, bodyEndBit, seedPt)
            || !std::isfinite(seedPt.x)
            || !std::isfinite(seedPt.y))
            return fail();
        DRW_DBG("\n  "); DRW_DBG(seedPt.x); DRW_DBG(","); DRW_DBG(seedPt.y);
        seedPoints.push_back(seedPt);
    }

    DRW_DBG("\n");
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");

    for (std::uint32_t i = 0 ; i < totalBoundItems; ++i){
        dwgHandle biH;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                                  biH))
            return fail();
        DRW_DBG("Boundary Items Handle: "); DRW_DBGHL(biH.code, biH.size, biH.ref);
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    if (!handleProbe.isGood())
        return fail();
    *sourceBuf = handleProbe;
    return true;
}

void DRW_Hatch::encodeDwgGradientData(DRW::Version version, dwgBufferW *buf,
                                      dwgBufferW *strBuf) const {
    if (version <= DRW::AC1015)
        return;

    dwgBufferW *sb = strBuf ? strBuf : buf;
    buf->putBitLong(isGradient);
    buf->putBitLong(gradReserved);
    buf->putBitDouble(gradAngle);
    buf->putBitDouble(gradShift);
    buf->putBitLong(singleColor);
    buf->putBitDouble(gradTint);
    buf->putBitLong(static_cast<std::int32_t>(gradColors.size()));
    for (const GradientStop& stop : gradColors) {
        buf->putBitDouble(stop.value);
        buf->putBitShort(static_cast<std::uint16_t>(stop.aciColor));
        buf->putBitLong(static_cast<std::uint32_t>(stop.rgb));
        buf->putRawChar8(0);
    }
    sb->putVariableText(version, gradName);
}

bool DRW_Hatch::encodeDwgBoundaryData(DRW::Version version, dwgBufferW *buf,
                                      bool includePixelSize) const {
    buf->putBitLong(static_cast<std::int32_t>(looplist.size()));

    for (const auto& lp : looplist) {
        buf->putBitLong(includePixelSize ? lp->type : lp->type & ~4);

        if (!(lp->type & 2)) {
            buf->putBitLong(static_cast<std::int32_t>(lp->objlist.size()));
            for (const auto& seg : lp->objlist) {
                if (const auto* ln = dynamic_cast<const DRW_Line*>(seg.get())) {
                    buf->putRawChar8(1);  // line
                    buf->put2RawDouble(ln->basePoint);
                    buf->put2RawDouble(ln->secPoint);
                } else if (const auto* arc = dynamic_cast<const DRW_Arc*>(seg.get())) {
                    buf->putRawChar8(2);  // circular arc
                    buf->put2RawDouble(arc->basePoint);
                    buf->putBitDouble(arc->radious);
                    buf->putBitDouble(arc->staangle);
                    buf->putBitDouble(arc->endangle);
                    buf->putBit(static_cast<std::uint8_t>(arc->isccw));
                } else if (const auto* el = dynamic_cast<const DRW_Ellipse*>(seg.get())) {
                    buf->putRawChar8(3);  // ellipse arc
                    buf->put2RawDouble(el->basePoint);
                    buf->put2RawDouble(el->secPoint);
                    buf->putBitDouble(el->ratio);
                    buf->putBitDouble(el->staparam);
                    buf->putBitDouble(el->endparam);
                    buf->putBit(static_cast<std::uint8_t>(el->isccw));
                } else if (const auto* sp = dynamic_cast<const DRW_Spline*>(seg.get())) {
                    buf->putRawChar8(4);  // spline
                    buf->putBitLong(sp->degree);
                    bool isRational = (sp->flags & 4) != 0;
                    bool isPeriodic = (sp->flags & 2) != 0;
                    buf->putBit(static_cast<std::uint8_t>(isRational));
                    buf->putBit(static_cast<std::uint8_t>(isPeriodic));
                    buf->putBitLong(static_cast<std::int32_t>(sp->knotslist.size()));
                    buf->putBitLong(static_cast<std::int32_t>(sp->controllist.size()));
                    for (double k : sp->knotslist)
                        buf->putBitDouble(k);
                    for (const auto& cp : sp->controllist) {
                        DRW_Coord c2{cp->x, cp->y, 0.0};
                        buf->put2RawDouble(c2);
                        if (isRational)
                            buf->putBitDouble(cp->z);
                    }
                    if (version > DRW::AC1021) {
                        buf->putBitLong(static_cast<std::int32_t>(sp->fitlist.size()));
                        for (const auto& fp : sp->fitlist) {
                            DRW_Coord f2{fp->x, fp->y, 0.0};
                            buf->put2RawDouble(f2);
                        }
                        buf->put2RawDouble(sp->tgStart);
                        buf->put2RawDouble(sp->tgEnd);
                    }
                } else {
                    return false;
                }
            }
        } else {
            const DRW_LWPolyline* pl = nullptr;
            if (!lp->objlist.empty())
                pl = dynamic_cast<const DRW_LWPolyline*>(lp->objlist[0].get());
            if (!pl)
                return false;

            bool asBulge = false;
            for (const auto& v : pl->vertlist)
                if (v->bulge != 0.0) { asBulge = true; break; }

            buf->putBit(static_cast<std::uint8_t>(asBulge));
            buf->putBit(static_cast<std::uint8_t>(pl->flags & 1));
            buf->putBitLong(static_cast<std::int32_t>(pl->vertlist.size()));
            for (const auto& v : pl->vertlist) {
                buf->putRawDouble(v->x);
                buf->putRawDouble(v->y);
                if (asBulge)
                    buf->putBitDouble(v->bulge);
            }
        }

        buf->putBitLong(0);  // numBoundHandles for this loop (0 = non-associative)
    }

    return true;
}

bool DRW_Hatch::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (!validateDwgHatchData(*this))
        return false;
    oType = 78;  // HATCH class id — see dwgreader.cpp:1380
    if (!encodeDwgCommon(version, buf)) return false;

    dwgBufferW *sb = strBuf ? strBuf : buf;
    encodeDwgGradientData(version, buf, sb);

    buf->putBitDouble(basePoint.z);            // BD: elevation
    buf->put3BitDouble(extPoint);              // 3BD: extrusion (NOT BE-style for HATCH)
    sb->putVariableText(version, name);        // TV: hatch pattern name
    buf->putBit(static_cast<std::uint8_t>(solid));
    buf->putBit(static_cast<std::uint8_t>(associative));
    if (!encodeDwgBoundaryData(version, buf, true)) return false;

    buf->putBitShort(static_cast<std::uint16_t>(hstyle));
    buf->putBitShort(static_cast<std::uint16_t>(hpattern));

    if (!solid) {
        buf->putBitDouble(angle);
        buf->putBitDouble(scale);
        buf->putBit(static_cast<std::uint8_t>(doubleflag));
        buf->putBitShort(static_cast<std::uint16_t>(patternLines.size()));
        for (const PatternLine& patternLine : patternLines) {
            buf->putBitDouble(patternLine.angle);
            buf->putBitDouble(patternLine.baseX);
            buf->putBitDouble(patternLine.baseY);
            buf->putBitDouble(patternLine.offsetX);
            buf->putBitDouble(patternLine.offsetY);
            buf->putBitShort(static_cast<std::uint16_t>(patternLine.dashList.size()));
            for (double dash : patternLine.dashList)
                buf->putBitDouble(dash);
        }
    }

    bool havePixelSize = false;
    for (const auto& lp : looplist)
        havePixelSize = havePixelSize || (lp && ((lp->type & 4) != 0));
    if (havePixelSize)
        buf->putBitDouble(pixelSize);

    buf->putBitLong(static_cast<std::int32_t>(seedPoints.size()));
    for (const auto& sp : seedPoints) {
        buf->putRawDouble(sp.x);
        buf->putRawDouble(sp.y);
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    return true;
}

bool DRW_Spline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 210:
        normalVec.x = reader->getDouble();
        break;
    case 220:
        normalVec.y = reader->getDouble();
        break;
    case 230:
        normalVec.z = reader->getDouble();
        break;
    case 12:
        tgStart.x = reader->getDouble();
        break;
    case 22:
        tgStart.y = reader->getDouble();
        break;
    case 32:
        tgStart.z = reader->getDouble();
        break;
    case 13:
        tgEnd.x = reader->getDouble();
        break;
    case 23:
        tgEnd.y = reader->getDouble();
        break;
    case 33:
        tgEnd.z = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 71:
        degree = reader->getInt32();
        if (!isValidCount(degree, kMaxDxfDegree) || degree == 0)
            return false;
        m_dxfDegreeSeen = true;
        break;
    case 72:
        if (m_dxfKnotCountSeen)
            return false;
        nknots = reader->getInt32();
        if (!isValidCount(nknots, kMaxDxfItems))
            return false;
        m_dxfKnotCountSeen = true;
        break;
    case 73:
        if (m_dxfControlCountSeen)
            return false;
        ncontrol = reader->getInt32();
        if (!isValidCount(ncontrol, kMaxDxfItems))
            return false;
        m_dxfControlCountSeen = true;
        break;
    case 74:
        if (m_dxfFitCountSeen)
            return false;
        nfit = reader->getInt32();
        if (!isValidCount(nfit, kMaxDxfItems))
            return false;
        m_dxfFitCountSeen = true;
        break;
    case 42:
        tolknot = reader->getDouble();
        break;
    case 43:
        tolcontrol = reader->getDouble();
        break;
    case 44:
        tolfit = reader->getDouble();
        break;
    case 10: {
		if (controllist.size() >= static_cast<std::size_t>(kMaxDxfItems)
            || (m_dxfControlCountSeen
                && controllist.size() >= static_cast<std::size_t>(ncontrol)))
            return false;
        controlpoint = std::make_shared<DRW_Coord>();
        controllist.push_back(controlpoint);
        controlpoint->x = reader->getDouble();
        break; }
    case 20:
        if(controlpoint)
            controlpoint->y = reader->getDouble();
        break;
    case 30:
        if(controlpoint)
            controlpoint->z = reader->getDouble();
        break;
    case 11: {
		if (fitlist.size() >= static_cast<std::size_t>(kMaxDxfItems)
            || (m_dxfFitCountSeen
                && fitlist.size() >= static_cast<std::size_t>(nfit)))
            return false;
        fitpoint = std::make_shared<DRW_Coord>();
        fitlist.push_back(fitpoint);
        fitpoint->x = reader->getDouble();
        break; }
    case 21:
        if(fitpoint)
            fitpoint->y = reader->getDouble();
        break;
    case 31:
        if(fitpoint)
            fitpoint->z = reader->getDouble();
        break;
    case 40:
        if (knotslist.size() >= static_cast<std::size_t>(kMaxDxfItems)
            || (m_dxfKnotCountSeen
                && knotslist.size() >= static_cast<std::size_t>(nknots)))
            return false;
        knotslist.push_back(reader->getDouble());
        break;
    case 41:
        if (weightlist.size() >= static_cast<std::size_t>(kMaxDxfItems)
            || (m_dxfControlCountSeen
                && weightlist.size() >= static_cast<std::size_t>(ncontrol)))
            return false;
        weightlist.push_back(reader->getDouble());
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Spline::validateDxf() const {
    if (m_dxfKnotCountSeen
        && knotslist.size() != static_cast<std::size_t>(nknots))
        return false;
    if (m_dxfControlCountSeen
        && controllist.size() != static_cast<std::size_t>(ncontrol))
        return false;
    if (m_dxfFitCountSeen
        && fitlist.size() != static_cast<std::size_t>(nfit))
        return false;
    return validatePayloadFields(/*allowMixedLists=*/true);
}

bool DRW_Spline::validatePayloadFields(bool allowMixedLists) const {
    const auto finite = [](double value) { return std::isfinite(value); };
    const auto finiteCoord = [&finite](const DRW_Coord& point) {
        return finite(point.x) && finite(point.y) && finite(point.z);
    };
    const auto validCount = [](std::size_t count) {
        return count <= static_cast<std::size_t>(kMaxSplineItems)
            && count <= static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max());
    };
    const auto validPointList = [&finiteCoord, &validCount](const auto& points) {
        return validCount(points.size())
            && std::all_of(points.cbegin(), points.cend(),
                           [&finiteCoord](const auto& point) {
                               return point != nullptr && finiteCoord(*point);
                           });
    };

    if (!isValidSplineDegree(degree)
        || flags < 0
        || (static_cast<std::uint32_t>(flags) & ~0x1Fu) != 0u
        || !finiteCoord(normalVec) || !finiteCoord(tgStart)
        || !finiteCoord(tgEnd) || !finite(tolknot)
        || !finite(tolcontrol) || !finite(tolfit)
        || !validCount(knotslist.size())
        || !std::all_of(knotslist.cbegin(), knotslist.cend(), finite)
        || !validCount(weightlist.size())
        || !std::all_of(weightlist.cbegin(), weightlist.cend(), finite)
        || !validPointList(controllist) || !validPointList(fitlist)
        || weightlist.size() > controllist.size())
        return false;

    // The DWG body has disjoint control-point and fit-point scenarios.  When
    // both lists are present, the encoder selects control points and would
    // silently discard the fit points, so reject that lossy representation.
    if (!allowMixedLists && !fitlist.empty() && !controllist.empty())
        return false;
    if (!fitlist.empty() && !controllist.empty())
        return isValidControlSplineLayout(
                   degree, static_cast<std::int32_t>(knotslist.size()),
                   static_cast<std::int32_t>(controllist.size()))
            && isValidFitSplineLayout(
                   degree, static_cast<std::int32_t>(fitlist.size()));
    if (!fitlist.empty())
        return isValidFitSplineLayout(
            degree, static_cast<std::int32_t>(fitlist.size()));
    return isValidControlSplineLayout(
        degree, static_cast<std::int32_t>(knotslist.size()),
        static_cast<std::int32_t>(controllist.size()));
}

bool DRW_Helix::validateDxf() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    return DRW_Spline::validateDxf()
        && finite(axisBasePt) && finite(startPt) && finite(axisVector)
        && std::isfinite(radius) && std::isfinite(turns)
        && std::isfinite(turnHeight);
}

bool DRW_Helix::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    if (code == 100) {
        const std::string subclass = reader->getString();
        m_parsingHelixSubclass = (subclass == "AcDbHelix");
        return true;
    }

    if (!m_parsingHelixSubclass)
        return DRW_Spline::parseCode(code, reader);

    switch (code) {
    case 90:
        m_majorVersion = reader->getInt32();
        break;
    case 91:
        m_maintVersion = reader->getInt32();
        break;
    case 10:
        axisBasePt.x = reader->getDouble();
        break;
    case 20:
        axisBasePt.y = reader->getDouble();
        break;
    case 30:
        axisBasePt.z = reader->getDouble();
        break;
    case 11:
        startPt.x = reader->getDouble();
        break;
    case 21:
        startPt.y = reader->getDouble();
        break;
    case 31:
        startPt.z = reader->getDouble();
        break;
    case 12:
        axisVector.x = reader->getDouble();
        break;
    case 22:
        axisVector.y = reader->getDouble();
        break;
    case 32:
        axisVector.z = reader->getDouble();
        break;
    case 40:
        radius = reader->getDouble();
        break;
    case 41:
        turns = reader->getDouble();
        break;
    case 42:
        turnHeight = reader->getDouble();
        break;
    case 290:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 1, value))
                return false;
            handedness = value != 0;
        }
        break;
    case 280:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 0xFF, value))
                return false;
            constraintType = static_cast<std::uint8_t>(value);
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

void DRW_Spline::resetDwgState() {
    DRW_Entity::reset();
    normalVec = DRW_Coord{0.0, 0.0, 0.0};
    tgStart = DRW_Coord{0.0, 0.0, 0.0};
    tgEnd = DRW_Coord{0.0, 0.0, 0.0};
    flags = 0;
    degree = 0;
    m_scenario = 0;
    m_splineFlags1 = 0;
    m_knotParam = 15;
    nknots = 0;
    ncontrol = 0;
    nfit = 0;
    tolknot = 0.0000001;
    tolcontrol = 0.0000001;
    tolfit = 0.0000001;
    knotslist.clear();
    weightlist.clear();
    controllist.clear();
    fitlist.clear();
    controlpoint.reset();
    fitpoint.reset();
    m_dxfDegreeSeen = false;
    m_dxfKnotCountSeen = false;
    m_dxfControlCountSeen = false;
    m_dxfFitCountSeen = false;
}

bool DRW_Spline::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return fail();
    if (!parseDwgSplineBody(version, buf))
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    /* Common Entity Handle Data */
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                         handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();
//    RS crc;   //RS */
    *sourceBuf = handleProbe;
    return true;
}

// Spline body decode: the scenario/degree/knots/ctrl/fit section, WITHOUT
// the leading DRW_Entity::parseDwg(common) or the trailing parseDwgEntHandle.
// Factored out so DRW_Helix can reuse the identical spline payload before its
// AcDbHelix trailer (Phase 8a-1).
bool DRW_Spline::parseDwgSplineBody(DRW::Version version, dwgBuffer *buf){
    try {
    DRW_DBG("\n***************************** parsing spline *********************************************\n");
    if (buf == nullptr || !buf->isGood())
        return false;
    std::uint8_t weight = 0; // RLZ ??? flags, weight, code 70, bit 4 (16)
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t controlBits = 0;

    std::int32_t scenario = 0;
    if (!readBoundedBitLong(*buf, bodyEndBit, scenario))
        return false;
    m_scenario = scenario;
    DRW_DBG("scenario: "); DRW_DBG(scenario);
    if (version > DRW::AC1024) {
        std::int32_t splFlag1 = 0;
        std::int32_t knotParam = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, splFlag1)
            || !readBoundedBitLong(*buf, bodyEndBit, knotParam))
            return false;
        m_splineFlags1 = splFlag1;
        m_knotParam = knotParam;
        if (knotParam == kSplineKnotParamCustom || !(splFlag1 & kSplineFlagUseKnotParameter)) {
            scenario = 1;
        } else if (splFlag1 & kSplineFlagMethodFitPoints) {
            scenario = 2;
        }
        m_scenario = scenario;
        DRW_DBG(" 2013 splFlag1: "); DRW_DBG(splFlag1);
        DRW_DBG(" 2013 knotParam: "); DRW_DBG(knotParam);
//        DRW_DBG("unk bit: "); DRW_DBG(buf->getBit());
    }
    if (!readBoundedBitLong(*buf, bodyEndBit, degree))
        return false;
    DRW_DBG(" degree: "); DRW_DBG(degree); DRW_DBG("\n");
    if (!isValidSplineDegree(degree)) {
        DRW_DBG("\ndwg Spline, invalid degree "); DRW_DBG(degree); DRW_DBG("\n");
        return false;
    }
    if (scenario == 2) {
        flags = 8;//scenario 2 = not rational & planar
        if (m_splineFlags1 & kSplineFlagClosed)
            flags |= 1;
        if (!readBoundedBitDouble(*buf, bodyEndBit, tolfit)
            || !readBoundedBitCoord(*buf, bodyEndBit, tgStart)
            || !readBoundedBitCoord(*buf, bodyEndBit, tgEnd))
            return false;
        DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(" tolfit: "); DRW_DBG(tolfit);
        DRW_DBG(" Start Tangent: "); DRW_DBGPT(tgStart.x, tgStart.y, tgStart.z);
        DRW_DBG("\nEnd Tangent: "); DRW_DBGPT(tgEnd.x, tgEnd.y, tgEnd.z);
        std::uint32_t parsedFitCount = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxSplineItems, 6,
                                parsedFitCount)) {
            return false;
        }
        nfit = static_cast<std::int32_t>(parsedFitCount);
        if (!isValidFitSplineLayout(degree, nfit)) {
            DRW_DBG("\ndwg Spline, invalid fit layout degree/count: ");
            DRW_DBG(degree); DRW_DBG("/"); DRW_DBG(nfit); DRW_DBG("\n");
            return false;
        }
        DRW_DBG("\nnumber of fit points: "); DRW_DBG(nfit);
    } else if (scenario == 1) {
        flags = 8;//scenario 1 = rational & planar
        bool rational = false;
        bool closed = false;
        bool periodic = false;
        if (!readBoundedBit(*buf, bodyEndBit, rational)
            || !readBoundedBit(*buf, bodyEndBit, closed)
            || !readBoundedBit(*buf, bodyEndBit, periodic)
            || !readBoundedBitDouble(*buf, bodyEndBit, tolknot)
            || !readBoundedBitDouble(*buf, bodyEndBit, tolcontrol))
            return false;
        flags |= rational ? 4 : 0;
        flags |= closed ? 1 : 0;
        flags |= periodic ? 2 : 0;
        DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(" knot tolerance: "); DRW_DBG(tolknot);
        DRW_DBG(" control point tolerance: "); DRW_DBG(tolcontrol);
        std::uint32_t parsedKnotCount = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxSplineItems, 2,
                                parsedKnotCount)) {
            return false;
        }
        nknots = static_cast<std::int32_t>(parsedKnotCount);
        std::uint32_t parsedControlCount = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxSplineItems, 6,
                                parsedControlCount)) {
            return false;
        }
        ncontrol = static_cast<std::int32_t>(parsedControlCount);
        if (!isValidControlSplineLayout(degree, nknots, ncontrol)) {
            DRW_DBG("\ndwg Spline, invalid control layout degree/knots/control: ");
            DRW_DBG(degree); DRW_DBG("/"); DRW_DBG(nknots); DRW_DBG("/");
            DRW_DBG(ncontrol); DRW_DBG("\n");
            return false;
        }
        bool hasWeights = false;
        if (!readBoundedBit(*buf, bodyEndBit, hasWeights))
            return false;
        weight = hasWeights ? 1 : 0; // flags bit 4: weights present (code 70)
        if (weight) flags |= 0x10;
        if (!dwgSafety::multiply(static_cast<std::uint64_t>(ncontrol),
                                 weight ? 8u : 6u, controlBits))
            return false;
        if (!proxyEntityHasBits(*buf, bodyEndBit, controlBits)) {
            return false;
        }
        DRW_DBG("\nnum of knots: "); DRW_DBG(nknots); DRW_DBG(" num of control pt: ");
        DRW_DBG(ncontrol); DRW_DBG(" weight bit: "); DRW_DBG(weight);
    } else {
        DRW_DBG("\ndwg Spline, unknown scenario "); DRW_DBG(scenario);
        DRW_DBG(" (expected 1 or 2)\n");
        return false; //RLZ: from doc only 1 or 2 are ok ?
    }

    std::vector<double> parsedKnots;
    std::vector<double> parsedWeights;
    std::vector<std::shared_ptr<DRW_Coord>> parsedControls;
    std::vector<std::shared_ptr<DRW_Coord>> parsedFits;
    std::uint64_t knotBits = 0;
    std::uint64_t fitBits = 0;
    std::uint64_t requiredBits = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(nknots), 2,
                             knotBits)
        || !dwgSafety::multiply(static_cast<std::uint64_t>(nfit), 6,
                                fitBits)
        || !dwgSafety::add(knotBits, controlBits, requiredBits)
        || !dwgSafety::add(requiredBits, fitBits, requiredBits)
        || !proxyEntityHasBits(*buf, bodyEndBit, requiredBits)) {
        return false;
    }
    if (!DRW::reserve(parsedKnots, nknots)) {
        return false;
    }
    for (std::int32_t i= 0; i<nknots; ++i){
        double knot = 0.0;
        if (!readBoundedBitDouble(*buf, bodyEndBit, knot)
            || !std::isfinite(knot))
            return false;
        parsedKnots.push_back(knot);
    }
    if (!DRW::reserve(parsedControls, ncontrol)) {
        return false;
    }
    if (weight && !DRW::reserve(parsedWeights, ncontrol)) {
        return false;
    }
    for (std::int32_t i= 0; i<ncontrol; ++i){
        DRW_Coord control;
        if (!readBoundedBitCoord(*buf, bodyEndBit, control)
            || !std::isfinite(control.x)
            || !std::isfinite(control.y)
            || !std::isfinite(control.z))
            return false;
        parsedControls.push_back(std::make_shared<DRW_Coord>(control));
        if (weight) {
            //per-control-point weight; required for hyperbola/parabola
            //conic detection in consumers (e.g. LibreCAD addSpline)
            double w = 0.0; //RLZ Warning: D (BD or RD)
            if (!readBoundedBitDouble(*buf, bodyEndBit, w)
                || !std::isfinite(w))
                return false;
            parsedWeights.push_back(w);
            DRW_DBG("\n w: "); DRW_DBG(w);
        }
    }
    if (!DRW::reserve(parsedFits, nfit)) {
        return false;
    }
    for (std::int32_t i= 0; i<nfit; ++i) {
        DRW_Coord fit;
        if (!readBoundedBitCoord(*buf, bodyEndBit, fit)
            || !std::isfinite(fit.x)
            || !std::isfinite(fit.y)
            || !std::isfinite(fit.z))
            return false;
        parsedFits.push_back(std::make_shared<DRW_Coord>(fit));
    }

    if (DRW_DBGGL == DRW_dbg::Level::Debug) {
        DRW_DBG("\nknots list: ");
        for (auto const& v: parsedKnots) {
            DRW_DBG("\n"); DRW_DBG(v);
        }
        DRW_DBG("\ncontrol point list: ");
        for (auto const& v: parsedControls) {
            DRW_DBG("\n"); DRW_DBGPT(v->x, v->y, v->z);
        }
        DRW_DBG("\nfit point list: ");
        for (auto const& v: parsedFits) {
            DRW_DBG("\n"); DRW_DBGPT(v->x, v->y, v->z);
        }
    }

    if (!buf->isGood())
        return false;
    knotslist = std::move(parsedKnots);
    weightlist = std::move(parsedWeights);
    controllist = std::move(parsedControls);
    fitlist = std::move(parsedFits);
    return true;
    } catch (...) {
        return false;
    }
}

// AcDbHelix trailer order (libreDWG dwg2.spec:2493-2503):
//   major_version BL, maint_version BL, axis_base_pt 3BD, start_pt 3BD,
//   axis_vector 3BD, radius BD, turns BD, turn_height BD, handedness B,
//   constraint_type RC.
void DRW_Helix::resetDwgState() {
    DRW_Spline::resetDwgState();
    m_majorVersion = 0;
    m_maintVersion = 0;
    axisBasePt = DRW_Coord{0.0, 0.0, 0.0};
    startPt = DRW_Coord{0.0, 0.0, 0.0};
    axisVector = DRW_Coord{0.0, 0.0, 0.0};
    radius = 0.0;
    turns = 0.0;
    turnHeight = 0.0;
    handedness = false;
    constraintType = 0;
    m_parsingHelixSubclass = false;
}

bool DRW_Helix::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing helix *********************************************\n");
    if (!parseDwgSplineBody(version, buf))
        return fail();

    // AcDbHelix trailer (see field order above).
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::int32_t parsedMajorVersion = 0;
    std::int32_t parsedMaintVersion = 0;
    DRW_Coord parsedAxisBasePt;
    DRW_Coord parsedStartPt;
    DRW_Coord parsedAxisVector;
    double parsedRadius = 0.0;
    double parsedTurns = 0.0;
    double parsedTurnHeight = 0.0;
    bool parsedHandedness = false;
    std::uint8_t parsedConstraintType = 0;
    if (!readBoundedBitLong(*buf, bodyEndBit, parsedMajorVersion)
        || !readBoundedBitLong(*buf, bodyEndBit, parsedMaintVersion)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedAxisBasePt)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedStartPt)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedAxisVector)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedRadius)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedTurns)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedTurnHeight)
        || !readBoundedBit(*buf, bodyEndBit, parsedHandedness)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedConstraintType)
        || !std::isfinite(parsedAxisBasePt.x)
        || !std::isfinite(parsedAxisBasePt.y)
        || !std::isfinite(parsedAxisBasePt.z)
        || !std::isfinite(parsedStartPt.x)
        || !std::isfinite(parsedStartPt.y)
        || !std::isfinite(parsedStartPt.z)
        || !std::isfinite(parsedAxisVector.x)
        || !std::isfinite(parsedAxisVector.y)
        || !std::isfinite(parsedAxisVector.z)
        || !std::isfinite(parsedRadius)
        || !std::isfinite(parsedTurns)
        || !std::isfinite(parsedTurnHeight))
        return fail();
    DRW_DBG("\nhelix radius: "); DRW_DBG(parsedRadius); DRW_DBG(" turns: "); DRW_DBG(parsedTurns);

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    /* Common Entity Handle Data */
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                         handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();
    //    RS crc;   //RS */
    m_majorVersion = parsedMajorVersion;
    m_maintVersion = parsedMaintVersion;
    axisBasePt = parsedAxisBasePt;
    startPt = parsedStartPt;
    axisVector = parsedAxisVector;
    radius = parsedRadius;
    turns = parsedTurns;
    turnHeight = parsedTurnHeight;
    handedness = parsedHandedness;
    constraintType = parsedConstraintType;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Image::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 12:
        vVector.x = reader->getDouble();
        break;
    case 22:
        vVector.y = reader->getDouble();
        break;
    case 32:
        vVector.z = reader->getDouble();
        break;
    case 13:
        sizeu = reader->getDouble();
        break;
    case 23:
        sizev = reader->getDouble();
        break;
    case 70:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 0xFFFF, value))
                return false;
            m_displayProps = value;
        }
        break;
    case 90: {
        const std::int32_t value = reader->getInt32();
        if (value < 0 || value > kMaxClassVersion)
            return false;
        m_classVersion = value;
        break;
    }
    case 340:
        ref = reader->getHandleString();
        break;
    case 360:
        m_imageDefReactorHandle = reader->getHandleString();
        break;
    case 280:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 1, value))
                return false;
            clip = value;
        }
        break;
    case 281:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 100, value))
                return false;
            brightness = value;
        }
        break;
    case 282:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 100, value))
                return false;
            contrast = value;
        }
        break;
    case 283:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 100, value))
                return false;
            fade = value;
        }
        break;
    case 71:
        {
            int value = 0;
            if (!readDxfIntInRange(reader, 0, 2, value))
                return false;
            m_clipBoundaryType = value;
        }
        break;
    case 91:
        // The declared count is a structural invariant: reject negative or
        // implausibly large values before reserve() can allocate unboundedly.
        {
        const std::int32_t count = reader->getInt32();
        if (count < 0 || count > static_cast<std::int32_t>(DRW_Image::kMaxClipVertices))
            return false;
        clipPath.clear();
        if (!DRW::reserve(clipPath, count))
            return false;
        m_declaredClipVertexCount = count;
        m_clipPathHasOpenVertex = false;
        }
        break;
    case 14:
        // WIPEOUT polygon vertex x — start a new vertex.  Group 24 (y) follows.
        if (m_clipPathHasOpenVertex)
            return false;
        clipPath.emplace_back(reader->getDouble(), 0.0);
        m_clipPathHasOpenVertex = true;
        break;
    case 24:
        // WIPEOUT polygon vertex y — complete the most recently started vertex.
        if (!m_clipPathHasOpenVertex || clipPath.empty())
            return false;
        clipPath.back().y = reader->getDouble();
        m_clipPathHasOpenVertex = false;
        break;
    case 290:
        // R2010+ Clip mode (IMAGE/WIPEOUT, ODA spec §20.4.80):
        // 0 = mask outside the polygon, 1 = mask inside.
        clipMode = reader->getBool();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

bool DRW_Image::hasValidClipBoundary() const {
    if (m_clipPathHasOpenVertex
        || (m_declaredClipVertexCount >= 0
            && static_cast<std::size_t>(m_declaredClipVertexCount) != clipPath.size())) {
        return false;
    }
    switch (m_clipBoundaryType) {
    case 0:
        return clipPath.empty();
    case 1:
        return clipPath.size() == 2;
    case 2:
        return clipPath.size() >= 3;
    default:
        return false;
    }
}

bool DRW_Image::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    return m_classVersion >= 0 && m_classVersion <= kMaxClassVersion
        && m_displayProps >= 0 && m_displayProps <= 0xffff
        && clip >= 0 && clip <= 1
        && brightness >= 0 && brightness <= 0xff
        && contrast >= 0 && contrast <= 0xff
        && fade >= 0 && fade <= 0xff
        && std::isfinite(sizeu) && std::isfinite(sizev)
        && finite(basePoint) && finite(secPoint) && finite(vVector)
        && clipPath.size() <= kMaxClipVertices
        && std::all_of(clipPath.cbegin(), clipPath.cend(),
                       [](const DRW_Coord& point) {
                           return std::isfinite(point.x)
                               && std::isfinite(point.y);
                       });
}

void DRW_Image::resetDwgState() {
    DRW_Line::resetDwgState();
    m_classVersion = 0;
    ref = 0;
    m_imageDefReactorHandle = 0;
    m_displayProps = 0;
    vVector = DRW_Coord{0.0, 0.0, 0.0};
    sizeu = 0.0;
    sizev = 0.0;
    dz = 0.0;
    clip = 0;
    brightness = 50;
    contrast = 50;
    fade = 0;
    m_clipBoundaryType = 0;
    clipPath.clear();
    clipMode = false;
    m_declaredClipVertexCount = -1;
    m_clipPathHasOpenVertex = false;
}

bool DRW_Image::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing image *********************************************\n");

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::int32_t parsedClassVersion = 0;
    DRW_Coord parsedBasePoint;
    DRW_Coord parsedSecPoint;
    DRW_Coord parsedVVector;
    double parsedSizeU = 0.0;
    double parsedSizeV = 0.0;
    std::uint16_t parsedDisplayProps = 0;
    bool parsedClip = false;
    std::uint8_t parsedBrightness = 0;
    std::uint8_t parsedContrast = 0;
    std::uint8_t parsedFade = 0;
    bool parsedClipMode = false;
    if (!readBoundedBitLong(*buf, bodyEndBit, parsedClassVersion)
        || parsedClassVersion < 0 || parsedClassVersion > kMaxClassVersion
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedBasePoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedSecPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedVVector)
        || !readBoundedRawDouble(*buf, bodyEndBit, parsedSizeU)
        || !readBoundedRawDouble(*buf, bodyEndBit, parsedSizeV)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedDisplayProps)
        || !readBoundedBit(*buf, bodyEndBit, parsedClip)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedBrightness)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedContrast)
        || !readBoundedRawChar8(*buf, bodyEndBit, parsedFade)
        || (version > DRW::AC1021
            && !readBoundedBit(*buf, bodyEndBit, parsedClipMode))
        || !finite(parsedBasePoint) || !finite(parsedSecPoint)
        || !finite(parsedVVector) || !std::isfinite(parsedSizeU)
        || !std::isfinite(parsedSizeV))
        return fail();
    DRW_DBG("class Version: "); DRW_DBG(parsedClassVersion);
    DRW_DBG("\nbase point: "); DRW_DBGPT(parsedBasePoint.x, parsedBasePoint.y, parsedBasePoint.z);
    DRW_DBG("\nU vector: "); DRW_DBGPT(parsedSecPoint.x, parsedSecPoint.y, parsedSecPoint.z);
    DRW_DBG("\nV vector: "); DRW_DBGPT(parsedVVector.x, parsedVVector.y, parsedVVector.z);
    DRW_DBG("\nsize U: "); DRW_DBG(parsedSizeU); DRW_DBG("\nsize V: "); DRW_DBG(parsedSizeV);
    DRW_DBG("\ndisplay props: "); DRW_DBG(parsedDisplayProps);

    std::uint16_t parsedClipBoundaryType = 0;
    if (!readBoundedBitShort(*buf, bodyEndBit, parsedClipBoundaryType))
        return fail();
    std::vector<DRW_Coord> parsedClipPath;
    std::int32_t parsedDeclaredClipVertexCount = -1;
    if (parsedClipBoundaryType == 0) {
        // No clip boundary payload.
    } else if (parsedClipBoundaryType == 1){
        // Rectangles are encoded as exactly two opposite corners. Keep that
        // canonical payload intact; rendering expands it independently.
        DRW_Coord ll;
        DRW_Coord ur;
        if (!readBoundedRawDouble(*buf, bodyEndBit, ll.x)
            || !readBoundedRawDouble(*buf, bodyEndBit, ll.y)
            || !readBoundedRawDouble(*buf, bodyEndBit, ur.x)
            || !readBoundedRawDouble(*buf, bodyEndBit, ur.y)
            || !finite(ll) || !finite(ur))
            return fail();
        parsedClipPath.push_back(ll);
        parsedClipPath.push_back(ur);
        parsedDeclaredClipVertexCount = 2;
    } else if (parsedClipBoundaryType == 2) {
        std::uint32_t numVerts = 0;
        if (!readTableBodyCount(version, buf, objSize,
                                static_cast<std::uint32_t>(kMaxClipVertices),
                                128, numVerts))
            return fail();
        std::uint64_t vertexBits = 0;
        if (!dwgSafety::multiply(static_cast<std::uint64_t>(numVerts), 128,
                                 vertexBits)
            || !proxyEntityHasBits(*buf, bodyEndBit, vertexBits)
            || !DRW::reserve(parsedClipPath, numVerts))
            return fail();
        for (std::uint32_t i = 0; i < numVerts; ++i) {
            DRW_Coord point;
            if (!readBoundedRawDouble(*buf, bodyEndBit, point.x)
                || !readBoundedRawDouble(*buf, bodyEndBit, point.y)
                || !finite(point))
                return fail();
            parsedClipPath.push_back(point);
        }
        parsedDeclaredClipVertexCount = static_cast<std::int32_t>(numVerts);
    } else {
        DRW_DBG("unsupported image clip type: "); DRW_DBG(parsedClipBoundaryType); DRW_DBG("\n");
        return fail();
    }

    if ((parsedClipBoundaryType == 0 && !parsedClipPath.empty())
        || (parsedClipBoundaryType == 2 && parsedClipPath.size() < 3))
        return fail();

    if (!buf->isGood())
        return fail();
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                        handleEndBit);
    if (!ret)
        return fail();
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");

    if (!proxyEntityHasBits(handleProbe, handleEndBit, 8))
        return fail();
    dwgHandle biH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false, biH))
        return fail();
    DRW_DBG("ImageDef Handle: "); DRW_DBGHL(biH.code, biH.size, biH.ref);
    if (!proxyEntityHasBits(handleProbe, handleEndBit, 8))
        return fail();
    dwgHandle reactorH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                              reactorH))
        return fail();
    DRW_DBG("ImageDefReactor Handle: "); DRW_DBGHL(reactorH.code, reactorH.size, reactorH.ref);
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
    if (!handleProbe.isGood() || !sBuf->isGood())
        return fail();
    m_classVersion = parsedClassVersion;
    basePoint = parsedBasePoint;
    secPoint = parsedSecPoint;
    vVector = parsedVVector;
    sizeu = parsedSizeU;
    sizev = parsedSizeV;
    m_displayProps = parsedDisplayProps;
    clip = parsedClip ? 1 : 0;
    brightness = parsedBrightness;
    contrast = parsedContrast;
    fade = parsedFade;
    clipMode = parsedClipMode;
    m_clipBoundaryType = parsedClipBoundaryType;
    clipPath = std::move(parsedClipPath);
    m_declaredClipVertexCount = parsedDeclaredClipVertexCount;
    m_clipPathHasOpenVertex = false;
    *sourceBuf = handleProbe;
    ref = biH.ref;
    m_imageDefReactorHandle = reactorH.ref;
    return true;
}

// DRW_Image::encodeDwg — inverse of DRW_Image::parseDwg above (libreDWG
// dwg.spec:5533-5563).  Body field order: BL class_version, 3 x 3BD
// (base/uvec/vvec), 2 x RD (sizeu/sizev), BS display_props, B clip,
// 3 x RC (brightness/contrast/fade), [R2010+ B clip_mode], BS
// clip_boundary_type + verts.  Both handles (imagedef code 5 + reactor
// code 3) are emitted UNCONDITIONALLY at the END of the handle stream,
// matching parseDwg's order — NOT the spec's interleaved mid-stream slots.
bool DRW_Image::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                          dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (!validatePayloadFields()) {
        DRW_DBG("IMAGE has an invalid scalar or clip field\n");
        return false;
    }
    // Callers sometimes populate clipPath without setting m_clipBoundaryType
    // (DXF import historically stored only the vertices). Infer a coherent type
    // so encode does not reject a well-formed polygon/rectangle path.
    if (m_clipBoundaryType == 0 && !clipPath.empty()) {
        if (clipPath.size() == 2)
            m_clipBoundaryType = 1;
        else if (clipPath.size() >= 3)
            m_clipBoundaryType = 2;
    }
    if (!hasValidClipBoundary()) {
        DRW_DBG("IMAGE has invalid clip boundary\n");
        return false;
    }
    oType = 101;  // IMAGE class id — see dwgreader.cpp case 101
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBitLong(m_classVersion);
    buf->putBitDouble(basePoint.x); buf->putBitDouble(basePoint.y); buf->putBitDouble(basePoint.z);
    buf->putBitDouble(secPoint.x);  buf->putBitDouble(secPoint.y);  buf->putBitDouble(secPoint.z);  // uvec
    buf->putBitDouble(vVector.x);   buf->putBitDouble(vVector.y);   buf->putBitDouble(vVector.z);
    buf->putRawDouble(sizeu);
    buf->putRawDouble(sizev);
    buf->putBitShort(static_cast<std::uint16_t>(m_displayProps));
    buf->putBit(static_cast<std::uint8_t>(clip & 1));
    buf->putRawChar8(static_cast<std::uint8_t>(brightness));
    buf->putRawChar8(static_cast<std::uint8_t>(contrast));
    buf->putRawChar8(static_cast<std::uint8_t>(fade));
    if (version > DRW::AC1021) {  // 2010+ clip mode
        buf->putBit(clipMode ? 1 : 0);
    }
    if (m_clipBoundaryType == 0) {
        buf->putBitShort(0);  // clip_boundary_type 0 = none
    } else if (m_clipBoundaryType == 1) {
        buf->putBitShort(1);
        buf->put2RawDouble(clipPath[0]);
        buf->put2RawDouble(clipPath[1]);
    } else {
        buf->putBitShort(2);
        buf->putBitLong(static_cast<std::int32_t>(clipPath.size()));
        for (std::size_t i = 0; i < clipPath.size(); ++i)
            buf->put2RawDouble(clipPath[i]);
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // Emit both trailing handles UNCONDITIONALLY in parseDwg order:
    // imagedef (hard pointer, code 5) then imagedefreactor (hard owner, code 3).
    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    auto makeHandle = [](std::uint8_t code, std::uint32_t r) {
        dwgHandle h;
        h.code = (r == 0) ? 0 : code;
        h.ref  = r;
        h.size = 0;
        if (r != 0) { std::uint32_t t = r; while (t != 0) { t >>= 8; ++h.size; } }
        return h;
    };
    hb->putHandle(makeHandle(5, ref));                       // imagedef (340)
    hb->putHandle(makeHandle(3, m_imageDefReactorHandle));   // imagedefreactor (360)
    return true;
}

bool DRW_Wipeout::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    return DRW_Image::parseCode(code, reader);
}

bool DRW_Wipeout::hasValidBoundary() const {
    return m_clipBoundaryType != 0 && hasValidClipBoundary();
}

bool DRW_Wipeout::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    if (buf == nullptr)
        return false;

    // Image parsing is transactional; keep it on a second cursor because a
    // valid image payload is still invalid for WIPEOUT when it has no clip
    // boundary. The wrapper must preserve the same failure contract.
    dwgBuffer probe = buf->forkIndependent();
    if (!DRW_Image::parseDwg(version, &probe, bs)) {
        buf->invalidate();
        resetDwgState();
        return false;
    }
    if (!hasValidBoundary()) {
        buf->invalidate();
        resetDwgState();
        return false;
    }
    *buf = probe;
    return true;
}

bool DRW_Wipeout::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                             dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (!validatePayloadFields()) {
        DRW_DBG("WIPEOUT has an invalid scalar or clip field\n");
        return false;
    }
    if (clipPath.size() > DRW_Image::kMaxClipVertices) {
        DRW_DBG("WIPEOUT clip vertices exceed DWG limit\n");
        return false;
    }
    if (!hasValidBoundary()) {
        DRW_DBG("WIPEOUT has invalid clip boundary\n");
        return false;
    }
    // WIPEOUT is fixed DWG entity type 1109.  It is not a custom class and
    // must not depend on a file-local CLASSES ordinal.
    oType = dwgType::WIPEOUT;
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBitLong(0);
    buf->putBitDouble(basePoint.x); buf->putBitDouble(basePoint.y); buf->putBitDouble(basePoint.z);
    buf->putBitDouble(secPoint.x);  buf->putBitDouble(secPoint.y);  buf->putBitDouble(secPoint.z);
    buf->putBitDouble(vVector.x);   buf->putBitDouble(vVector.y);   buf->putBitDouble(vVector.z);
    buf->putRawDouble(sizeu);
    buf->putRawDouble(sizev);
    buf->putBitShort(static_cast<std::uint16_t>(m_displayProps));
    buf->putBit(static_cast<std::uint8_t>(clip & 1));
    buf->putRawChar8(static_cast<std::uint8_t>(brightness));
    buf->putRawChar8(static_cast<std::uint8_t>(contrast));
    buf->putRawChar8(static_cast<std::uint8_t>(fade));
    if (version > DRW::AC1021) {
        buf->putBit(clipMode ? 1 : 0);
    }
    if (m_clipBoundaryType == 1) {
        buf->putBitShort(1);
        buf->put2RawDouble(clipPath[0]);
        buf->put2RawDouble(clipPath[1]);
    } else {
        buf->putBitShort(2);
        buf->putBitLong(static_cast<std::int32_t>(clipPath.size()));
        for (std::size_t i = 0; i < clipPath.size(); ++i)
            buf->put2RawDouble(clipPath[i]);
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    auto makeHandle = [](std::uint8_t code, std::uint32_t r) {
        dwgHandle h;
        h.code = (r == 0) ? 0 : code;
        h.ref  = r;
        h.size = 0;
        if (r != 0) { std::uint32_t t = r; while (t != 0) { t >>= 8; ++h.size; } }
        return h;
    };
    hb->putHandle(makeHandle(5, ref));
    hb->putHandle(makeHandle(3, m_imageDefReactorHandle));
    return true;
}

bool DRW_NavisworksModel::parseCode(
    int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        const std::string subclass = reader->getString();
        if (subclass == "AcDbNavisworksModel")
            m_dxfInBody = true;
        return true;
    }
    if (!m_dxfInBody)
        return DRW_Entity::parseCode(code, reader);

    switch (code) {
    case 70: {
        const std::int32_t value = reader->getInt32();
        if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
            return false;
        flags = static_cast<std::uint16_t>(value);
        m_dxfSawFlags = true;
        return true;
    }
    case 340:
        definitionHandle = reader->getHandleString();
        m_dxfSawDefinition = true;
        return true;
    case 40:
        if (m_dxfTransformCount < kTransformSize) {
            transform[m_dxfTransformCount++] = reader->getDouble();
            return true;
        }
        if (m_dxfSawUnitFactor)
            return false;
        unitFactor = reader->getDouble();
        m_dxfSawUnitFactor = true;
        return true;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
}

bool DRW_NavisworksModel::finalizeDxf() {
    return m_dxfTransformCount == kTransformSize && m_dxfSawUnitFactor
        && std::isfinite(unitFactor)
        && std::all_of(transform.begin(), transform.end(),
                       [](double value) { return std::isfinite(value); });
}

void DRW_NavisworksModel::resetDwgState() {
    DRW_Entity::reset();
    flags = 0;
    definitionHandle = 0;
    transform = {1.0, 0.0, 0.0, 0.0,
                 0.0, 1.0, 0.0, 0.0,
                 0.0, 0.0, 1.0, 0.0,
                 0.0, 0.0, 0.0, 1.0};
    unitFactor = 1.0;
}

bool DRW_NavisworksModel::parseDwg(DRW::Version version, dwgBuffer *buf,
                                   std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();
    if (version < DRW::AC1015)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringBuffer = bodyProbe.forkIndependent();
    dwgBuffer *stringBuf = version > DRW::AC1018 ? &stringBuffer : &bodyProbe;
    if (!DRW_Entity::parseDwg(version, &bodyProbe, stringBuf, bs))
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    // BS is an unsigned bit-short at the codec boundary.  The flags are a
    // mask, so values with bit 15 set must survive instead of being rejected
    // through an intermediate signed conversion.
    std::uint16_t parsedFlags = 0;
    if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedFlags))
        return fail();

    std::uint32_t parsedDefinitionHandle = 0;
    if (version <= DRW::AC1018) {
        dwgHandle definition;
        if (!readBoundedDwgHandle(bodyProbe, handleEndBit, 0, false,
                                  definition))
            return fail();
        parsedDefinitionHandle = definition.ref;
    }

    std::array<double, 16> parsedTransform{};
    for (double& value : parsedTransform) {
        if (!readBoundedBitDouble(bodyProbe, bodyEndBit, value))
            return fail();
    }
    double parsedUnitFactor = 0.0;
    if (!readBoundedBitDouble(bodyProbe, bodyEndBit, parsedUnitFactor))
        return fail();
    if (!std::all_of(parsedTransform.begin(), parsedTransform.end(),
                    [](double value) { return std::isfinite(value); })
        || !std::isfinite(parsedUnitFactor)) {
        return fail();
    }

    if (!DRW_Entity::parseDwgEntHandle(version, &bodyProbe, true,
                                       handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (version > DRW::AC1018) {
        dwgHandle definition;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  definition))
            return fail();
        parsedDefinitionHandle = definition.ref;
    }
    if (!handleProbe.isGood() || !stringBuf->isGood())
        return fail();
    flags = parsedFlags;
    transform = parsedTransform;
    unitFactor = parsedUnitFactor;
    definitionHandle = parsedDefinitionHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_NavisworksModel::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                    std::uint32_t bs, dwgBufferW *strBuf,
                                    dwgBufferW *handleBuf) {
    (void)bs;
    if (version < DRW::AC1015
        || !std::isfinite(unitFactor)
        || !std::all_of(transform.begin(), transform.end(),
                        [](double value) { return std::isfinite(value); }))
        return false;

    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    buf->putBitShort(flags);
    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    auto makeHandle = [](std::uint8_t code, std::uint32_t ref) {
        dwgHandle handle;
        handle.code = ref == 0 ? 0 : code;
        handle.ref = ref;
        handle.size = 0;
        for (std::uint32_t value = ref; value != 0; value >>= 8)
            ++handle.size;
        return handle;
    };

    // AC1015/AC1018 place this entity-specific handle before the matrix in
    // the legacy data stream. AC1021+ places it after the common handles.
    if (version <= DRW::AC1018)
        buf->putHandle(makeHandle(2, definitionHandle));
    for (double value : transform)
        buf->putBitDouble(value);
    buf->putBitDouble(unitFactor);

    if (!DRW_Entity::encodeDwgEntHandle(version, buf, handleBuf))
        return false;
    if (version > DRW::AC1018)
        hb->putHandle(makeHandle(2, definitionHandle));
    return true;
}

bool DRW_PointCloud::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        m_dxfInBody = reader->getString() == "AcDbPointCloud";
        return true;
    }
    if (!m_dxfInBody)
        return DRW_Entity::parseCode(code, reader);
    switch (code) {
    case 70:
        m_dxfSawClassVersion = true;
        classVersion = reader->getInt32();
        break;
    case 10: origin.x = reader->getDouble(); break;
    case 20: origin.y = reader->getDouble(); break;
    case 30: origin.z = reader->getDouble(); break;
    case 1: savedFilename = reader->getUtf8String(); break;
    case 90:
        if (m_dxfSawSourceCount)
            return false;
        m_dxfSawSourceCount = true;
        sourceFileCount = reader->getInt32();
        if (!isValidCount(sourceFileCount,
                          static_cast<std::int32_t>(kMaxItems))) {
            return false;
        }
        sourceFiles.clear();
        if (!DRW::reserve(sourceFiles, sourceFileCount))
            return false;
        break;
    case 2:
        if (sourceFiles.size() >= static_cast<std::size_t>(sourceFileCount))
            return false;
        sourceFiles.push_back(reader->getUtf8String());
        break;
    case 3: ucsName = reader->getUtf8String(); break;
    case 11: extentsMin.x = reader->getDouble(); break;
    case 21: extentsMin.y = reader->getDouble(); break;
    case 31: extentsMin.z = reader->getDouble(); break;
    case 12: extentsMax.x = reader->getDouble(); break;
    case 22: extentsMax.y = reader->getDouble(); break;
    case 32: extentsMax.z = reader->getDouble(); break;
    case 92: {
        if (m_dxfSawPointCount)
            return false;
        m_dxfSawPointCount = true;
        const std::int32_t count = reader->getInt32();
        if (count < 0)
            return false;
        pointCount = static_cast<std::uint64_t>(count);
        break;
    }
    case 13: ucsOrigin.x = reader->getDouble(); break;
    case 23: ucsOrigin.y = reader->getDouble(); break;
    case 33: ucsOrigin.z = reader->getDouble(); break;
    case 210: ucsXDirection.x = reader->getDouble(); break;
    case 220: ucsXDirection.y = reader->getDouble(); break;
    case 230: ucsXDirection.z = reader->getDouble(); break;
    case 211: ucsYDirection.x = reader->getDouble(); break;
    case 221: ucsYDirection.y = reader->getDouble(); break;
    case 231: ucsYDirection.z = reader->getDouble(); break;
    case 212: ucsZDirection.x = reader->getDouble(); break;
    case 222: ucsZDirection.y = reader->getDouble(); break;
    case 232: ucsZDirection.z = reader->getDouble(); break;
    case 330:
        if (!m_dxfInBody)
            return DRW_Entity::parseCode(code, reader);
        definitionHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 360: reactorHandle = static_cast<std::uint32_t>(reader->getHandleString()); break;
    case 71: intensityScheme = reader->getInt32(); break;
    case 40: intensityStyle.minIntensity = reader->getDouble(); break;
    case 41: intensityStyle.maxIntensity = reader->getDouble(); break;
    case 42: intensityStyle.lowThreshold = reader->getDouble(); break;
    case 43: intensityStyle.highThreshold = reader->getDouble(); break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_PointCloud::finalizeDxf() {
    return m_dxfSawClassVersion && m_dxfSawSourceCount && m_dxfSawPointCount
        && isValidCount(sourceFileCount,
                        static_cast<std::int32_t>(kMaxItems))
        && sourceFiles.size() == static_cast<std::size_t>(sourceFileCount);
}

void DRW_PointCloud::resetDwgState() {
    DRW_Entity::reset();
    classVersion = 0;
    origin = DRW_Coord{0.0, 0.0, 0.0};
    savedFilename.clear();
    sourceFileCount = 0;
    sourceFiles.clear();
    extentsMin = DRW_Coord{0.0, 0.0, 0.0};
    extentsMax = DRW_Coord{0.0, 0.0, 0.0};
    pointCount = 0;
    ucsName.clear();
    ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
    ucsXDirection = DRW_Coord{1.0, 0.0, 0.0};
    ucsYDirection = DRW_Coord{0.0, 1.0, 0.0};
    ucsZDirection = DRW_Coord{0.0, 0.0, 1.0};
    definitionHandle = 0;
    reactorHandle = 0;
    showIntensity = false;
    intensityScheme = 0;
    intensityStyle = DRW_PointCloudIntensityStyle{};
    showClipping = false;
    clippingCount = 0;
    clippings.clear();
    m_dxfInBody = false;
    m_dxfSawClassVersion = false;
    m_dxfSawSourceCount = false;
    m_dxfSawPointCount = false;
}

bool DRW_PointCloud::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr || version <= DRW::AC1018)
        return fail();

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwg(version, &bodyProbe, &stringProbe, bs))
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(stringProbe, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    std::uint16_t parsedClassVersion = 0;
    if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedClassVersion)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, origin))
        return fail();
    classVersion = parsedClassVersion;
    if (!finite(origin))
        return fail();
    if (!readBoundedVariableText(stringProbe, stringEndBit, version,
                                 savedFilename))
        return fail();
    std::uint32_t declaredSourceFiles = 0;
    if (!readTableBodyCount(
            version, &bodyProbe, objSize,
            static_cast<std::uint32_t>(kMaxItems), 2,
            declaredSourceFiles)) {
        return fail();
    }
    sourceFileCount = static_cast<int>(declaredSourceFiles);
    sourceFiles.clear();

    if (sourceFileCount == 0) {
        if (!readBoundedBitCoord(bodyProbe, bodyEndBit, extentsMin)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, extentsMax)
            || !readBoundedRawLong64(bodyProbe, bodyEndBit, pointCount))
            return fail();
        if (!finite(extentsMin) || !finite(extentsMax))
            return fail();
        if (!readBoundedVariableText(stringProbe, stringEndBit, version,
                                     ucsName))
            return fail();
        if (!readBoundedBitCoord(bodyProbe, bodyEndBit, ucsOrigin)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsXDirection)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsYDirection)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsZDirection))
            return fail();
        if (!finite(ucsOrigin) || !finite(ucsXDirection)
            || !finite(ucsYDirection) || !finite(ucsZDirection)) {
            return fail();
        }

        if (version > DRW::AC1024) {
            std::uint16_t parsedIntensityScheme = 0;
            if (!readBoundedBit(bodyProbe, bodyEndBit, showIntensity)
                || !readBoundedBitShort(bodyProbe, bodyEndBit,
                                        parsedIntensityScheme)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                         intensityStyle.minIntensity)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                         intensityStyle.maxIntensity)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                         intensityStyle.lowThreshold)
                || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                         intensityStyle.highThreshold))
                return fail();
            intensityScheme = static_cast<int>(parsedIntensityScheme);
            if (!std::isfinite(intensityStyle.minIntensity)
                || !std::isfinite(intensityStyle.maxIntensity)
                || !std::isfinite(intensityStyle.lowThreshold)
                || !std::isfinite(intensityStyle.highThreshold)) {
                return fail();
            }
            if (!readBoundedBit(bodyProbe, bodyEndBit, showClipping))
                return fail();

            std::uint32_t declaredClippings = 0;
            if (!readTableBodyCount(
                    version, &bodyProbe, objSize,
                    static_cast<std::uint32_t>(kMaxItems), 2,
                    declaredClippings)) {
                return fail();
            }
            clippingCount = static_cast<int>(declaredClippings);
            clippings.clear();
            if (!DRW::reserve(clippings, clippingCount))
                return fail();
            for (std::int32_t i = 0; i < clippingCount; ++i) {
                DRW_PointCloudClipping clipping;
                if (!readBoundedBit(bodyProbe, bodyEndBit,
                                    clipping.isInverted))
                    return fail();
                std::uint16_t clippingType = 0;
                if (!readBoundedBitShort(bodyProbe, bodyEndBit, clippingType))
                    return fail();
                clipping.type = clippingType;
                if (clipping.type < 1 || clipping.type > 3)
                    return fail();
                std::uint32_t vertexCount = 2;
                if (clipping.type == 3
                    && !readTableBodyCount(
                        version, &bodyProbe, objSize,
                        static_cast<std::uint32_t>(kMaxItems), 128,
                        vertexCount)) {
                    return fail();
                }
                clipping.vertexCount = static_cast<int>(vertexCount);
                if (!DRW::reserve(clipping.vertices, clipping.vertexCount))
                    return fail();
                for (std::int32_t j = 0; j < clipping.vertexCount; ++j) {
                    DRW_Coord vertex;
                    if (!readBounded2RawDouble(bodyProbe, bodyEndBit, vertex))
                        return fail();
                    clipping.vertices.push_back(vertex);
                }
                if (clipping.type == 1) {
                    if (!readBoundedBitDouble(bodyProbe, bodyEndBit,
                                              clipping.zMin)
                        || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                                 clipping.zMax))
                        return fail();
                }
                if (!std::all_of(
                        clipping.vertices.begin(), clipping.vertices.end(),
                        finite)
                    || !std::isfinite(clipping.zMin)
                    || !std::isfinite(clipping.zMax)) {
                    return fail();
                }
                clippings.push_back(std::move(clipping));
            }
        } else {
            showIntensity = false;
            intensityScheme = 0;
            showClipping = false;
            clippingCount = 0;
            clippings.clear();
        }
    }

    if (!DRW::reserve(sourceFiles, sourceFileCount))
        return fail();
    for (std::int32_t i = 0; i < sourceFileCount; ++i) {
        UTF8STRING sourceFile;
        if (!readBoundedVariableText(stringProbe, stringEndBit, version,
                                     sourceFile))
            return fail();
        sourceFiles.push_back(std::move(sourceFile));
    }

    if (!bodyProbe.isGood() || !stringProbe.isGood()
        || currentDwgBit(&bodyProbe) > bodyEndBit
        || currentDwgBit(&stringProbe) > stringEndBit)
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();
    std::uint32_t parsedDefinitionHandle = 0;
    std::uint32_t parsedReactorHandle = 0;
    if (version > DRW::AC1024 && sourceFileCount == 0) {
        dwgHandle definition;
        dwgHandle reactor;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  definition)
            || !readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                     reactor))
            return fail();
        parsedDefinitionHandle = definition.ref;
        parsedReactorHandle = reactor.ref;
    }
    if (!handleProbe.isGood() || !stringProbe.isGood())
        return fail();
    definitionHandle = parsedDefinitionHandle;
    reactorHandle = parsedReactorHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_PointCloud::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                                dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (version <= DRW::AC1018
        || classVersion > std::numeric_limits<std::uint16_t>::max()
        || sourceFileCount < 0
        || sourceFileCount > static_cast<int>(kMaxItems)
        || sourceFiles.size() != static_cast<std::size_t>(sourceFileCount)
        || clippingCount < 0
        || (sourceFileCount == 0 && clippings.size() > kMaxItems)
        || (sourceFileCount != 0
            && (clippingCount != 0 || !clippings.empty()
                || showIntensity || showClipping))
        || intensityScheme < 0
        || intensityScheme > std::numeric_limits<std::uint16_t>::max()
        || clippings.size() != static_cast<std::size_t>(clippingCount)) {
        return false;
    }
    if (!finite(origin) || !finite(extentsMin) || !finite(extentsMax)
        || !finite(ucsOrigin) || !finite(ucsXDirection)
        || !finite(ucsYDirection) || !finite(ucsZDirection)
        || !std::isfinite(intensityStyle.minIntensity)
        || !std::isfinite(intensityStyle.maxIntensity)
        || !std::isfinite(intensityStyle.lowThreshold)
        || !std::isfinite(intensityStyle.highThreshold)) {
        return false;
    }
    for (const DRW_PointCloudClipping& clipping : clippings) {
        if (clipping.type < 1 || clipping.type > 3
            || clipping.vertexCount < 0
            || clipping.vertexCount > static_cast<int>(kMaxItems)
            || clipping.vertices.size() != static_cast<std::size_t>(clipping.vertexCount)
            || (clipping.type != 3 && clipping.vertexCount != 2)
            || !std::isfinite(clipping.zMin)
            || !std::isfinite(clipping.zMax)) {
            return false;
        }
        for (const DRW_Coord& vertex : clipping.vertices) {
            if (!finite(vertex))
                return false;
        }
    }

    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;
    dwgBufferW *sBuf = strBuf ? strBuf : buf;
    buf->putBitShort(static_cast<std::uint16_t>(classVersion));
    buf->put3BitDouble(origin);
    sBuf->putVariableText(version, savedFilename);
    buf->putBitLong(sourceFileCount);
    if (sourceFileCount == 0) {
        buf->put3BitDouble(extentsMin);
        buf->put3BitDouble(extentsMax);
        buf->putRawLong64(pointCount);
        sBuf->putVariableText(version, ucsName);
        buf->put3BitDouble(ucsOrigin);
        buf->put3BitDouble(ucsXDirection);
        buf->put3BitDouble(ucsYDirection);
        buf->put3BitDouble(ucsZDirection);

        if (version > DRW::AC1024) {
            buf->putBit(showIntensity ? 1 : 0);
            buf->putBitShort(static_cast<std::uint16_t>(intensityScheme));
            buf->putBitDouble(intensityStyle.minIntensity);
            buf->putBitDouble(intensityStyle.maxIntensity);
            buf->putBitDouble(intensityStyle.lowThreshold);
            buf->putBitDouble(intensityStyle.highThreshold);
            buf->putBit(showClipping ? 1 : 0);
            buf->putBitLong(clippingCount);
            for (const DRW_PointCloudClipping& clipping : clippings) {
                buf->putBit(clipping.isInverted ? 1 : 0);
                buf->putBitShort(static_cast<std::uint16_t>(clipping.type));
                if (clipping.type == 3)
                    buf->putBitLong(clipping.vertexCount);
                for (const DRW_Coord& vertex : clipping.vertices)
                    buf->put2RawDouble(vertex);
                if (clipping.type == 1) {
                    buf->putBitDouble(clipping.zMin);
                    buf->putBitDouble(clipping.zMax);
                }
            }
        }
    }
    for (const UTF8STRING& sourceFile : sourceFiles)
        sBuf->putVariableText(version, sourceFile);

    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;
    if (version > DRW::AC1024 && sourceFileCount == 0) {
        dwgBufferW *hBuf = handleBuf ? handleBuf : buf;
        auto putHandle = [hBuf](std::uint8_t code, std::uint32_t ref) {
            dwgHandle handle;
            handle.code = ref == 0 ? 0 : code;
            handle.ref = ref;
            handle.size = 0;
            for (std::uint32_t value = ref; value != 0; value >>= 8)
                ++handle.size;
            hBuf->putHandle(handle);
        };
        putHandle(5, definitionHandle);
        putHandle(3, reactorHandle);
    }
    return true;
}

bool DRW_PointCloudEx::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        m_dxfInBody = reader->getString() == "AcDbPointCloud";
        return true;
    }
    if (!m_dxfInBody)
        return DRW_Entity::parseCode(code, reader);
    switch (code) {
    case 70:
        if (m_dxfSeen70++ != 0) return false;
        classVersion = reader->getInt32();
        break;
    case 10: extentsMin.x = reader->getDouble(); break;
    case 20: extentsMin.y = reader->getDouble(); break;
    case 30: extentsMin.z = reader->getDouble(); break;
    case 11: extentsMax.x = reader->getDouble(); break;
    case 21: extentsMax.y = reader->getDouble(); break;
    case 31: extentsMax.z = reader->getDouble(); break;
    case 12: ucsOrigin.x = reader->getDouble(); break;
    case 22: ucsOrigin.y = reader->getDouble(); break;
    case 32: ucsOrigin.z = reader->getDouble(); break;
    case 210: ucsXDirection.x = reader->getDouble(); break;
    case 220: ucsXDirection.y = reader->getDouble(); break;
    case 230: ucsXDirection.z = reader->getDouble(); break;
    case 211: ucsYDirection.x = reader->getDouble(); break;
    case 221: ucsYDirection.y = reader->getDouble(); break;
    case 231: ucsYDirection.z = reader->getDouble(); break;
    case 212: ucsZDirection.x = reader->getDouble(); break;
    case 222: ucsZDirection.y = reader->getDouble(); break;
    case 232: ucsZDirection.z = reader->getDouble(); break;
    case 290:
        if (m_dxfHasCropping) {
            if (m_dxfSeen290 % 2 == 0)
                m_dxfCurrentCropping.isInside = reader->getBool();
            else
                m_dxfCurrentCropping.isInverted = reader->getBool();
            ++m_dxfSeen290;
        } else {
            if (m_dxfSeen290++ != 0) return false;
            isLocked = reader->getBool();
        }
        break;
    case 330:
        if (!m_dxfInBody)
            return DRW_Entity::parseCode(code, reader);
        definitionHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 360:
        reactorHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 1:
        switch (m_dxfTextCount++) {
        case 0: name = reader->getUtf8String(); break;
        case 1: intensityColorScheme = reader->getUtf8String(); break;
        case 2: currentColorScheme = reader->getUtf8String(); break;
        case 3: classificationColorScheme = reader->getUtf8String(); break;
        default: return false;
        }
        break;
    case 291: showIntensity = reader->getBool(); break;
    case 295: showCropping = reader->getBool(); break;
    case 71:
        if (m_dxfSeen71++ == 0)
            stylizationType = reader->getInt32();
        else if (m_dxfSeen71 == 2)
            intensityOutOfRangeBehavior = reader->getInt32();
        else
            return false;
        break;
    case 72: elevationOutOfRangeBehavior = reader->getInt32(); break;
    case 40:
        if (m_dxfSeen40++ == 0)
            elevationMin = reader->getDouble();
        else if (m_dxfSeen40 == 2)
            elevationMax = reader->getDouble();
        else
            return false;
        break;
    case 41: elevationMax = reader->getDouble(); break;
    case 90:
        if (m_dxfSeen90++ != 0) return false;
        intensityMin = reader->getInt32();
        break;
    case 91:
        if (m_dxfSeen91++ != 0) return false;
        intensityMax = reader->getInt32();
        break;
    case 292: elevationApplyToFixedRange = reader->getBool(); break;
    case 293: intensityAsGradient = reader->getBool(); break;
    case 294: elevationAsGradient = reader->getBool(); break;
    case 92: {
        if (m_dxfSawCroppingCount) return false;
        const std::int32_t count = reader->getInt32();
        if (!isValidCount(count, static_cast<std::int32_t>(kMaxItems)))
            return false;
        croppingCount = count;
        m_dxfSawCroppingCount = true;
        break;
    }
    case 93:
        if (m_dxfHasCropping) {
            if (m_dxfSeen93++ != 0) return false;
            m_dxfCurrentCropping.pointCount = reader->getInt32();
            if (!isValidCount(m_dxfCurrentCropping.pointCount,
                              static_cast<std::int32_t>(kMaxItems))) {
                return false;
            }
            m_dxfCurrentCropping.points.clear();
            if (!DRW::reserve(m_dxfCurrentCropping.points,
                              m_dxfCurrentCropping.pointCount))
                return false;
        } else if (m_dxfSawCroppingCount && croppingCount == 0) {
            if (m_dxfSeen93++ == 0)
                unknownInt0 = reader->getInt32();
            else if (m_dxfSeen93 == 2)
                unknownInt1 = reader->getInt32();
            else
                return false;
        } else {
            return false;
        }
        break;
    case 280:
        if (!m_dxfSawCroppingCount || croppingCount == 0
            || !finishDxfCropping()) {
            return false;
        }
        m_dxfCurrentCropping = DRW_PointCloudExCropping{};
        m_dxfCurrentCropping.type = reader->getInt32();
        if (m_dxfCurrentCropping.type < 0
            || m_dxfCurrentCropping.type > std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        m_dxfHasCropping = true;
        m_dxfSeen290 = 0;
        m_dxfSeen93 = 0;
        m_dxfDirectionComponent = 0;
        m_dxfCropPlaneComponent = 0;
        m_dxfCropPointOpen = false;
        break;
    case 13:
    case 23:
    case 33:
        if (!m_dxfHasCropping) {
            if (code == 13) ucsOrigin.x = reader->getDouble();
            else if (code == 23) ucsOrigin.y = reader->getDouble();
            else ucsOrigin.z = reader->getDouble();
        } else if (m_dxfCropPlaneComponent < 3) {
            if (code == 13 && m_dxfCropPlaneComponent != 0) return false;
            if (code == 23 && m_dxfCropPlaneComponent != 1) return false;
            if (code == 33 && m_dxfCropPlaneComponent != 2) return false;
            const double value = reader->getDouble();
            if (code == 13) m_dxfCurrentCropping.cropPlane.x = value;
            else if (code == 23) m_dxfCurrentCropping.cropPlane.y = value;
            else m_dxfCurrentCropping.cropPlane.z = value;
            ++m_dxfCropPlaneComponent;
        } else {
            if (code == 13) {
                if (m_dxfCropPointOpen) return false;
                m_dxfCropPoint = DRW_Coord{};
                m_dxfCropPointOpen = true;
            } else if (!m_dxfCropPointOpen) {
                return false;
            }
            const double value = reader->getDouble();
            if (code == 13) m_dxfCropPoint.x = value;
            else if (code == 23) m_dxfCropPoint.y = value;
            else m_dxfCropPoint.z = value;
            if (code == 33) {
                m_dxfCurrentCropping.points.push_back(m_dxfCropPoint);
                m_dxfCropPointOpen = false;
            }
        }
        break;
    case 213:
    case 223:
    case 233: {
        if (!m_dxfHasCropping || m_dxfDirectionComponent >= 6)
            return false;
        const double value = reader->getDouble();
        DRW_Coord *direction = m_dxfDirectionComponent < 3
            ? &m_dxfCurrentCropping.cropXDirection
            : &m_dxfCurrentCropping.cropYDirection;
        const int component = static_cast<int>(m_dxfDirectionComponent % 3);
        if ((component == 0 && code != 213)
            || (component == 1 && code != 223)
            || (component == 2 && code != 233)) {
            return false;
        }
        if (component == 0) direction->x = value;
        else if (component == 1) direction->y = value;
        else direction->z = value;
        ++m_dxfDirectionComponent;
        break;
    }
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_PointCloudEx::finishDxfCropping() {
    if (!m_dxfHasCropping)
        return true;
    if (m_dxfSeen290 != 2 || m_dxfDirectionComponent != 6
        || m_dxfCropPlaneComponent != 3 || m_dxfCropPointOpen
        || m_dxfSeen93 != 1
        || m_dxfCurrentCropping.points.size()
               != static_cast<std::size_t>(m_dxfCurrentCropping.pointCount)) {
        return false;
    }
    croppings.push_back(m_dxfCurrentCropping);
    m_dxfHasCropping = false;
    return true;
}

bool DRW_PointCloudEx::finalizeDxf() {
    if (!finishDxfCropping())
        return false;
    if (m_dxfSeen70 != 1 || !m_dxfSawCroppingCount)
        return false;
    if (croppingCount == 0)
        return m_dxfSeen93 == 2 && croppings.empty();
    return croppings.size() == static_cast<std::size_t>(croppingCount);
}

void DRW_PointCloudEx::resetDwgState() {
    DRW_Entity::reset();
    classVersion = 0;
    extentsMin = DRW_Coord{0.0, 0.0, 0.0};
    extentsMax = DRW_Coord{0.0, 0.0, 0.0};
    ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
    ucsXDirection = DRW_Coord{1.0, 0.0, 0.0};
    ucsYDirection = DRW_Coord{0.0, 1.0, 0.0};
    ucsZDirection = DRW_Coord{0.0, 0.0, 1.0};
    isLocked = false;
    definitionHandle = 0;
    reactorHandle = 0;
    name.clear();
    showIntensity = false;
    showCropping = false;
    croppingCount = 0;
    unknownInt0 = 0;
    unknownInt1 = 0;
    stylizationType = 0;
    intensityColorScheme.clear();
    currentColorScheme.clear();
    classificationColorScheme.clear();
    elevationMin = 0.0;
    elevationMax = 0.0;
    intensityMin = 0.0;
    intensityMax = 0.0;
    intensityOutOfRangeBehavior = 0;
    elevationOutOfRangeBehavior = 0;
    elevationApplyToFixedRange = false;
    intensityAsGradient = false;
    elevationAsGradient = false;
    croppings.clear();
    m_dxfCurrentCropping = DRW_PointCloudExCropping{};
    m_dxfTextCount = 0;
    m_dxfSeen70 = 0;
    m_dxfSeen71 = 0;
    m_dxfSeen90 = 0;
    m_dxfSeen91 = 0;
    m_dxfSeen93 = 0;
    m_dxfSeen290 = 0;
    m_dxfSeen40 = 0;
    m_dxfDirectionComponent = 0;
    m_dxfCropPlaneComponent = 0;
    m_dxfSawCroppingCount = false;
    m_dxfInBody = false;
    m_dxfHasCropping = false;
    m_dxfCropPointOpen = false;
    m_dxfCropPoint = DRW_Coord{0.0, 0.0, 0.0};
}

bool DRW_PointCloudEx::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr || version <= DRW::AC1024)
        return fail();

    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    if (!DRW_Entity::parseDwg(version, &bodyProbe, &stringProbe, bs))
        return fail();

    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(stringProbe, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    std::uint16_t parsedClassVersion = 0;
    if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedClassVersion)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, extentsMin)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, extentsMax)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsOrigin)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsXDirection)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsYDirection)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, ucsZDirection))
        return fail();
    classVersion = parsedClassVersion;
    if (!finite(extentsMin) || !finite(extentsMax) || !finite(ucsOrigin)
        || !finite(ucsXDirection) || !finite(ucsYDirection)
        || !finite(ucsZDirection)) {
        return fail();
    }
    if (!readBoundedBit(bodyProbe, bodyEndBit, isLocked))
        return fail();
    if (!readBoundedVariableText(stringProbe, stringEndBit, version, name))
        return fail();
    if (!readBoundedBit(bodyProbe, bodyEndBit, showIntensity)
        || !readBoundedBit(bodyProbe, bodyEndBit, showCropping))
        return fail();
    std::uint32_t declaredCroppings = 0;
    if (!readTableBodyCount(
            version, &bodyProbe, objSize,
            static_cast<std::uint32_t>(kMaxItems), 24,
            declaredCroppings)) {
        return fail();
    }
    croppingCount = static_cast<int>(declaredCroppings);
    croppings.clear();
    if (croppingCount == 0) {
        std::uint16_t parsedStylizationType = 0;
        if (!readBoundedBitLong(bodyProbe, bodyEndBit, unknownInt0)
            || !readBoundedBitLong(bodyProbe, bodyEndBit, unknownInt1)
            || !readBoundedBitShort(bodyProbe, bodyEndBit,
                                    parsedStylizationType))
            return fail();
        stylizationType = parsedStylizationType;
        if (!readBoundedVariableText(stringProbe, stringEndBit, version,
                                     intensityColorScheme)
            || !readBoundedVariableText(stringProbe, stringEndBit, version,
                                        currentColorScheme)
            || !readBoundedVariableText(stringProbe, stringEndBit, version,
                                        classificationColorScheme))
            return fail();
        std::int32_t parsedIntensityMin = 0;
        std::int32_t parsedIntensityMax = 0;
        if (!readBoundedBitDouble(bodyProbe, bodyEndBit, elevationMin)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit, elevationMax)
            || !readBoundedBitLong(bodyProbe, bodyEndBit, parsedIntensityMin)
            || !readBoundedBitLong(bodyProbe, bodyEndBit, parsedIntensityMax))
            return fail();
        intensityMin = static_cast<double>(parsedIntensityMin);
        intensityMax = static_cast<double>(parsedIntensityMax);
        if (!std::isfinite(elevationMin) || !std::isfinite(elevationMax))
            return fail();
        std::uint16_t parsedIntensityBehavior = 0;
        std::uint16_t parsedElevationBehavior = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit,
                                 parsedIntensityBehavior)
            || !readBoundedBitShort(bodyProbe, bodyEndBit,
                                    parsedElevationBehavior)
            || !readBoundedBit(bodyProbe, bodyEndBit,
                               elevationApplyToFixedRange)
            || !readBoundedBit(bodyProbe, bodyEndBit, intensityAsGradient)
            || !readBoundedBit(bodyProbe, bodyEndBit, elevationAsGradient))
            return fail();
        intensityOutOfRangeBehavior = parsedIntensityBehavior;
        elevationOutOfRangeBehavior = parsedElevationBehavior;
    } else {
        unknownInt0 = 0;
        unknownInt1 = 0;
    }
    if (!bodyProbe.isGood() || !stringProbe.isGood()
        || currentDwgBit(&bodyProbe) > bodyEndBit
        || currentDwgBit(&stringProbe) > stringEndBit)
        return fail();

    if (!DRW::reserve(croppings, croppingCount))
        return fail();
    for (std::uint32_t i = 0; i < declaredCroppings; ++i) {
        DRW_PointCloudExCropping cropping;
        std::uint16_t croppingType = 0;
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, croppingType)
            || !readBoundedBit(bodyProbe, bodyEndBit, cropping.isInside)
            || !readBoundedBit(bodyProbe, bodyEndBit, cropping.isInverted)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit,
                                     cropping.cropPlane)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit,
                                     cropping.cropXDirection)
            || !readBoundedBitCoord(bodyProbe, bodyEndBit,
                                     cropping.cropYDirection)) {
            return fail();
        }
        cropping.type = static_cast<int>(croppingType);
        if (!finite(cropping.cropPlane) || !finite(cropping.cropXDirection)
            || !finite(cropping.cropYDirection)) {
            return fail();
        }
        std::uint32_t declaredPoints = 0;
        if (!readTableBodyCount(
                version, &bodyProbe, objSize,
                static_cast<std::uint32_t>(kMaxItems),
                6, declaredPoints)) {
            return fail();
        }
        std::uint64_t pointBits = 0;
        if (!dwgSafety::multiply(declaredPoints, 6, pointBits)
            || !proxyEntityHasBits(bodyProbe, bodyEndBit, pointBits)) {
            return fail();
        }
        cropping.pointCount = static_cast<int>(declaredPoints);
        if (!DRW::reserve(cropping.points, cropping.pointCount))
            return fail();
        for (std::uint32_t j = 0; j < declaredPoints; ++j) {
            DRW_Coord point;
            if (!readBoundedBitCoord(bodyProbe, bodyEndBit, point))
                return fail();
            cropping.points.push_back(point);
        }
        if (!std::all_of(cropping.points.begin(), cropping.points.end(), finite))
            return fail();
        croppings.push_back(std::move(cropping));
    }

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();
    dwgHandle definition;
    dwgHandle reactor;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                              definition)
        || !readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                 reactor))
        return fail();
    const std::uint32_t parsedDefinitionHandle = definition.ref;
    const std::uint32_t parsedReactorHandle = reactor.ref;
    if (!handleProbe.isGood() || !stringProbe.isGood())
        return fail();
    definitionHandle = parsedDefinitionHandle;
    reactorHandle = parsedReactorHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_PointCloudEx::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                                  dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto fitsUint16 = [](int value) {
        return value >= 0
            && value <= std::numeric_limits<std::uint16_t>::max();
    };
    if (version <= DRW::AC1024
        || classVersion > std::numeric_limits<std::uint16_t>::max()
        || croppingCount < 0
        || croppingCount > static_cast<int>(kMaxItems)
        || croppings.size() != static_cast<std::size_t>(croppingCount)) {
        return false;
    }
    for (const DRW_PointCloudExCropping& cropping : croppings) {
        if (cropping.pointCount < 0
            || cropping.pointCount > static_cast<int>(kMaxItems)
            || cropping.points.size() != static_cast<std::size_t>(cropping.pointCount)) {
            return false;
        }
        if (!fitsUint16(cropping.type) || !finite(cropping.cropPlane)
            || !finite(cropping.cropXDirection)
            || !finite(cropping.cropYDirection)) {
            return false;
        }
        for (const DRW_Coord& point : cropping.points) {
            if (!finite(point))
                return false;
        }
    }
    if (!finite(extentsMin) || !finite(extentsMax) || !finite(ucsOrigin)
        || !finite(ucsXDirection) || !finite(ucsYDirection)
        || !finite(ucsZDirection) || !std::isfinite(elevationMin)
        || !std::isfinite(elevationMax)
        || !fitsUint16(stylizationType)
        || !fitsUint16(intensityOutOfRangeBehavior)
        || !fitsUint16(elevationOutOfRangeBehavior)) {
        return false;
    }
    const auto integral32 = [](double value) {
        return std::isfinite(value)
            && value >= static_cast<double>(std::numeric_limits<std::int32_t>::min())
            && value <= static_cast<double>(std::numeric_limits<std::int32_t>::max())
            && std::floor(value) == value;
    };
    if (croppingCount == 0
        && (!integral32(intensityMin) || !integral32(intensityMax))) {
        return false;
    }

    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;
    dwgBufferW *sBuf = strBuf ? strBuf : buf;
    buf->putBitShort(static_cast<std::uint16_t>(classVersion));
    buf->put3BitDouble(extentsMin);
    buf->put3BitDouble(extentsMax);
    buf->put3BitDouble(ucsOrigin);
    buf->put3BitDouble(ucsXDirection);
    buf->put3BitDouble(ucsYDirection);
    buf->put3BitDouble(ucsZDirection);
    buf->putBit(isLocked ? 1 : 0);
    sBuf->putVariableText(version, name);
    buf->putBit(showIntensity ? 1 : 0);
    buf->putBit(showCropping ? 1 : 0);
    buf->putBitLong(croppingCount);
    if (croppingCount == 0) {
        buf->putBitLong(unknownInt0);
        buf->putBitLong(unknownInt1);
        buf->putBitShort(static_cast<std::uint16_t>(stylizationType));
        sBuf->putVariableText(version, intensityColorScheme);
        sBuf->putVariableText(version, currentColorScheme);
        sBuf->putVariableText(version, classificationColorScheme);
        buf->putBitDouble(elevationMin);
        buf->putBitDouble(elevationMax);
        buf->putBitLong(static_cast<std::int32_t>(intensityMin));
        buf->putBitLong(static_cast<std::int32_t>(intensityMax));
        buf->putBitShort(static_cast<std::uint16_t>(intensityOutOfRangeBehavior));
        buf->putBitShort(static_cast<std::uint16_t>(elevationOutOfRangeBehavior));
        buf->putBit(elevationApplyToFixedRange ? 1 : 0);
        buf->putBit(intensityAsGradient ? 1 : 0);
        buf->putBit(elevationAsGradient ? 1 : 0);
    }
    for (const DRW_PointCloudExCropping& cropping : croppings) {
        buf->putBitShort(static_cast<std::uint16_t>(cropping.type));
        buf->putBit(cropping.isInside ? 1 : 0);
        buf->putBit(cropping.isInverted ? 1 : 0);
        buf->put3BitDouble(cropping.cropPlane);
        buf->put3BitDouble(cropping.cropXDirection);
        buf->put3BitDouble(cropping.cropYDirection);
        buf->putBitLong(cropping.pointCount);
        for (const DRW_Coord& point : cropping.points)
            buf->put3BitDouble(point);
    }
    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;

    dwgBufferW *hBuf = handleBuf ? handleBuf : buf;
    auto putHandle = [hBuf](std::uint8_t code, std::uint32_t ref) {
        dwgHandle handle;
        handle.code = ref == 0 ? 0 : code;
        handle.ref = ref;
        handle.size = 0;
        for (std::uint32_t value = ref; value != 0; value >>= 8)
            ++handle.size;
        hBuf->putHandle(handle);
    };
    putHandle(5, definitionHandle);
    putHandle(3, reactorHandle);
    return true;
}

bool DRW_Surface::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 1:
    case 3:
        // SURFACE ACIS payloads use text groups in the AcDbModelerGeometry
        // subclass. Keep the bytes verbatim; group 3 is a continuation of
        // the same payload and must not be interpreted as a surface field.
        if (!appendTextBytesChecked(rawAcisData, reader->getString(),
                                    dwgSafety::MaxBufferSize))
            return false;
        break;
    case 70:
        modelerFormatVersion = reader->getInt32();
        break;
    case 71:
        uIsolines = reader->getInt32();
        break;
    case 72:
        vIsolines = reader->getInt32();
        break;
    case 310:
        {
            std::vector<std::uint8_t> decoded;
            if (!decodeHexBytes(reader->getString(), decoded))
                return false;
            if (!appendBytesChecked(rawAcisData, decoded,
                                    dwgSafety::MaxBufferSize))
                return false;
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_ExtrudedSurface::parseCode(
        int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 100: {
        const std::string marker = reader->getString();
        if (marker == "AcDbExtrudedSurface")
            m_dxfInSubtype = true;
        else if (marker == "AcDbModelerGeometry"
                 || marker == "AcDbSurface")
            m_dxfInSubtype = false;
        break;
    }
    case 90:
        if (m_dxfClassIdSeen)
            return false;
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0)
                return false;
            classId = static_cast<std::uint32_t>(value);
            m_dxfClassIdSeen = true;
        }
        break;
    case 10:
        sweepVector.x = reader->getDouble();
        break;
    case 20:
        sweepVector.y = reader->getDouble();
        break;
    case 30:
        sweepVector.z = reader->getDouble();
        break;
    case 11:
        referenceVector.x = reader->getDouble();
        break;
    case 21:
        referenceVector.y = reader->getDouble();
        break;
    case 31:
        referenceVector.z = reader->getDouble();
        break;
    case 40:
        if (m_dxfExtrudedTransformCount >= extrudedTransform.size())
            return false;
        extrudedTransform[m_dxfExtrudedTransformCount++] = reader->getDouble();
        break;
    case 41:
        // Code 41 is not part of the AcDbExtrudedSurface mapping. Keep an
        // unexpected value out of the common entity state.
        return false;
    case 42:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        draftAngle = reader->getDouble();
        break;
    case 43:
        draftStartDistance = reader->getDouble();
        break;
    case 44:
        draftEndDistance = reader->getDouble();
        break;
    case 45:
        twistAngle = reader->getDouble();
        break;
    case 46:
        if (m_dxfSweepTransformCount >= sweepEntityTransform.size())
            return false;
        sweepEntityTransform[m_dxfSweepTransformCount++] = reader->getDouble();
        break;
    case 47:
        if (m_dxfPathTransformCount >= pathEntityTransform.size())
            return false;
        pathEntityTransform[m_dxfPathTransformCount++] = reader->getDouble();
        break;
    case 48:
        scaleFactor = reader->getDouble();
        break;
    case 49:
        alignAngle = reader->getDouble();
        break;
    case 70:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
                return false;
            sweepAlignmentFlags = value;
        }
        break;
    case 71:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
                return false;
            pathFlags = value;
        }
        break;
    case 290:
        solid = reader->getInt32() != 0;
        break;
    case 292:
        alignStart = reader->getInt32() != 0;
        break;
    case 293:
        bank = reader->getInt32() != 0;
        break;
    case 294:
        basePointSet = reader->getInt32() != 0;
        break;
    case 295:
        sweepEntityTransformComputed = reader->getInt32() != 0;
        break;
    case 296:
        pathEntityTransformComputed = reader->getInt32() != 0;
        break;
    default:
        return DRW_Surface::parseCode(code, reader);
    }
    return true;
}

bool DRW_ExtrudedSurface::finalizeDxf() const {
    if (!m_dxfClassIdSeen)
        return true;
    return m_dxfInSubtype
        && m_dxfExtrudedTransformCount == extrudedTransform.size()
        && m_dxfSweepTransformCount == sweepEntityTransform.size()
        && m_dxfPathTransformCount == pathEntityTransform.size();
}

bool DRW_SweptSurface::parseCode(
        int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 100: {
        const std::string marker = reader->getString();
        if (marker == "AcDbSweptSurface")
            m_dxfInSubtype = true;
        else if (marker == "AcDbModelerGeometry"
                 || marker == "AcDbSurface")
            m_dxfInSubtype = false;
        break;
    }
    case 90:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        if (m_dxfSweepEntityIdSeen)
            return false;
        m_dxfTypedFieldSeen = true;
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0)
                return false;
            sweepEntityId = static_cast<std::uint32_t>(value);
            m_dxfSweepEntityIdSeen = true;
        }
        break;
    case 91:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        if (m_dxfPathEntityIdSeen)
            return false;
        m_dxfTypedFieldSeen = true;
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0)
                return false;
            pathEntityId = static_cast<std::uint32_t>(value);
            m_dxfPathEntityIdSeen = true;
        }
        break;
    case 310: {
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        std::vector<std::uint8_t> decoded;
        if (!decodeHexBytes(reader->getString(), decoded))
            return false;
        auto& destination = m_dxfPathEntityIdSeen ? pathData : sweepData;
        if (!appendBytesChecked(destination, decoded, kMaxSweepDataSize))
            return false;
        break;
    }
    case 11:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        referenceVector.x = reader->getDouble();
        break;
    case 21:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        referenceVector.y = reader->getDouble();
        break;
    case 31:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        referenceVector.z = reader->getDouble();
        break;
    case 40:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        if (m_dxfSweepTransformCount >= sweepEntityTransformed.size())
            return false;
        sweepEntityTransformed[m_dxfSweepTransformCount++] = reader->getDouble();
        break;
    case 41:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        if (m_dxfPathTransformCount >= pathEntityTransformed.size())
            return false;
        pathEntityTransformed[m_dxfPathTransformCount++] = reader->getDouble();
        break;
    case 42:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        draftAngle = reader->getDouble();
        break;
    case 43:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        draftStartDistance = reader->getDouble();
        break;
    case 44:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        draftEndDistance = reader->getDouble();
        break;
    case 45:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        twistAngle = reader->getDouble();
        break;
    case 46:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        if (m_dxfSweepEntityTransformedCount >= sweepEntityTransform.size())
            return false;
        sweepEntityTransform[m_dxfSweepEntityTransformedCount++] =
            reader->getDouble();
        break;
    case 47:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        if (m_dxfPathEntityTransformedCount >= pathEntityTransform.size())
            return false;
        pathEntityTransform[m_dxfPathEntityTransformedCount++] =
            reader->getDouble();
        break;
    case 48:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        scaleFactor = reader->getDouble();
        break;
    case 49:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        alignAngle = reader->getDouble();
        break;
    case 70:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
                return false;
            sweepAlignmentFlags = value;
        }
        break;
    case 71:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
                return false;
            pathFlags = value;
        }
        break;
    case 290:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        solid = reader->getInt32() != 0;
        break;
    case 292:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        alignStart = reader->getInt32() != 0;
        break;
    case 293:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        bank = reader->getInt32() != 0;
        break;
    case 294:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        basePointSet = reader->getInt32() != 0;
        break;
    case 295:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        sweepEntityTransformComputed = reader->getInt32() != 0;
        break;
    case 296:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        pathEntityTransformComputed = reader->getInt32() != 0;
        break;
    default:
        return DRW_Surface::parseCode(code, reader);
    }
    return true;
}

bool DRW_SweptSurface::finalizeDxf() const {
    if (!m_dxfTypedFieldSeen)
        return true;
    return m_dxfInSubtype && m_dxfSweepEntityIdSeen
        && m_dxfPathEntityIdSeen
        && m_dxfSweepTransformCount == sweepEntityTransformed.size()
        && m_dxfPathTransformCount == pathEntityTransformed.size()
        && m_dxfSweepEntityTransformedCount == sweepEntityTransform.size()
        && m_dxfPathEntityTransformedCount == pathEntityTransform.size();
}

bool DRW_LoftedSurface::parseCode(
        int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 100: {
        const std::string marker = reader->getString();
        if (marker == "AcDbLoftedSurface")
            m_dxfInSubtype = true;
        else if (marker == "AcDbModelerGeometry"
                 || marker == "AcDbSurface")
            m_dxfInSubtype = false;
        break;
    }
    case 40:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        if (m_dxfTransformCount >= loftEntityTransform.size())
            return false;
        loftEntityTransform[m_dxfTransformCount++] = reader->getDouble();
        break;
    case 70:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        planeNormalLoftingType = reader->getInt32();
        break;
    case 41:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        startDraftAngle = reader->getDouble();
        break;
    case 42:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        endDraftAngle = reader->getDouble();
        break;
    case 43:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        startDraftMagnitude = reader->getDouble();
        break;
    case 44:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        endDraftMagnitude = reader->getDouble();
        break;
    case 90: {
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        if (m_dxfReferenceTokenCount >= kMaxReferenceTokenCount)
            return false;
        const std::int32_t value = reader->getInt32();
        if (value < 0 || static_cast<std::size_t>(value)
                              > kMaxReferenceTokenCount)
            return false;
        m_dxfTypedFieldSeen = true;
        dxfReferenceData.emplace_back(90, value);
        ++m_dxfReferenceTokenCount;
        break;
    }
    case 310: {
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        std::vector<std::uint8_t> decoded;
        if (!decodeHexBytes(reader->getString(), decoded))
            return false;
        if (m_dxfReferenceTokenCount >= kMaxReferenceTokenCount
            || decoded.size() > kMaxReferenceDataSize
            || m_dxfReferenceDataSize
                   > kMaxReferenceDataSize - decoded.size())
            return false;
        m_dxfTypedFieldSeen = true;
        dxfReferenceData.emplace_back(310, std::move(decoded));
        m_dxfReferenceDataSize += dxfReferenceData.back().binary()->size();
        ++m_dxfReferenceTokenCount;
        break;
    }
    case 290:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        arcLengthParameterization = reader->getInt32() != 0;
        break;
    case 291:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        noTwist = reader->getInt32() != 0;
        break;
    case 292:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        alignDirection = reader->getInt32() != 0;
        break;
    case 293:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        simpleSurfaces = reader->getInt32() != 0;
        break;
    case 294:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        closedSurfaces = reader->getInt32() != 0;
        break;
    case 295:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        solid = reader->getInt32() != 0;
        break;
    case 296:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        ruledSurface = reader->getInt32() != 0;
        break;
    case 297:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        m_dxfTypedFieldSeen = true;
        virtualGuide = reader->getInt32() != 0;
        break;
    case 5:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        if (!reader->isValidHandleString())
            return false;
        m_dxfTypedFieldSeen = true;
        pathCurveHandle = reader->getHandleString();
        break;
    default:
        return DRW_Surface::parseCode(code, reader);
    }
    return true;
}

bool DRW_LoftedSurface::finalizeDxf() const {
    if (!m_dxfTypedFieldSeen)
        return true;
    return m_dxfInSubtype
        && m_dxfTransformCount == loftEntityTransform.size()
        && std::all_of(loftEntityTransform.begin(), loftEntityTransform.end(),
                       [](double value) { return std::isfinite(value); })
        && std::isfinite(startDraftAngle)
        && std::isfinite(endDraftAngle)
        && std::isfinite(startDraftMagnitude)
        && std::isfinite(endDraftMagnitude);
}

bool DRW_NurbsSurface::parseCode(
        int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 100: {
        const std::string marker = reader->getString();
        if (marker == "AcDbNurbSurface")
            m_dxfInSubtype = true;
        else if (marker == "AcDbModelerGeometry"
                 || marker == "AcDbSurface")
            m_dxfInSubtype = false;
        break;
    }
    case 170:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        {
            const std::int32_t value = reader->getInt32();
            if (value < 0 || value > std::numeric_limits<std::uint16_t>::max())
                return false;
            short170 = static_cast<std::uint16_t>(value);
            m_dxfTypedFieldSeen = true;
        }
        break;
    case 290:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        cvHullDisplay = reader->getInt32() != 0;
        m_dxfTypedFieldSeen = true;
        break;
    case 10:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec1.x = reader->getDouble();
        m_dxfCoordinateMask[0] |= 1;
        m_dxfTypedFieldSeen = true;
        break;
    case 20:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec1.y = reader->getDouble();
        m_dxfCoordinateMask[0] |= 2;
        m_dxfTypedFieldSeen = true;
        break;
    case 30:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec1.z = reader->getDouble();
        m_dxfCoordinateMask[0] |= 4;
        m_dxfTypedFieldSeen = true;
        break;
    case 11:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec1.x = reader->getDouble();
        m_dxfCoordinateMask[1] |= 1;
        m_dxfTypedFieldSeen = true;
        break;
    case 21:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec1.y = reader->getDouble();
        m_dxfCoordinateMask[1] |= 2;
        m_dxfTypedFieldSeen = true;
        break;
    case 31:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec1.z = reader->getDouble();
        m_dxfCoordinateMask[1] |= 4;
        m_dxfTypedFieldSeen = true;
        break;
    case 12:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec2.x = reader->getDouble();
        m_dxfCoordinateMask[2] |= 1;
        m_dxfTypedFieldSeen = true;
        break;
    case 22:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec2.y = reader->getDouble();
        m_dxfCoordinateMask[2] |= 2;
        m_dxfTypedFieldSeen = true;
        break;
    case 32:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        uvec2.z = reader->getDouble();
        m_dxfCoordinateMask[2] |= 4;
        m_dxfTypedFieldSeen = true;
        break;
    case 13:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec2.x = reader->getDouble();
        m_dxfCoordinateMask[3] |= 1;
        m_dxfTypedFieldSeen = true;
        break;
    case 23:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec2.y = reader->getDouble();
        m_dxfCoordinateMask[3] |= 2;
        m_dxfTypedFieldSeen = true;
        break;
    case 33:
        if (!m_dxfInSubtype)
            return DRW_Surface::parseCode(code, reader);
        vvec2.z = reader->getDouble();
        m_dxfCoordinateMask[3] |= 4;
        m_dxfTypedFieldSeen = true;
        break;
    default:
        return DRW_Surface::parseCode(code, reader);
    }
    return true;
}

bool DRW_NurbsSurface::finalizeDxf() const {
    if (!m_dxfTypedFieldSeen)
        return true;
    const bool coordinatesSeen = std::any_of(
        m_dxfCoordinateMask.begin(), m_dxfCoordinateMask.end(),
        [](std::uint8_t mask) { return mask != 0; });
    if (!m_dxfInSubtype
        || (coordinatesSeen && std::any_of(
                m_dxfCoordinateMask.begin(), m_dxfCoordinateMask.end(),
                [](std::uint8_t mask) { return mask != 7; }))) {
        return false;
    }
    return std::all_of(m_dxfCoordinateMask.begin(), m_dxfCoordinateMask.end(),
                       [](std::uint8_t mask) { return mask == 0 || mask == 7; })
        && std::isfinite(uvec1.x) && std::isfinite(uvec1.y)
        && std::isfinite(uvec1.z) && std::isfinite(vvec1.x)
        && std::isfinite(vvec1.y) && std::isfinite(vvec1.z)
        && std::isfinite(uvec2.x) && std::isfinite(uvec2.y)
        && std::isfinite(uvec2.z) && std::isfinite(vvec2.x)
        && std::isfinite(vvec2.y) && std::isfinite(vvec2.z);
}

bool DRW_RevolvedSurface::parseCode(
        int code, const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 90:
        if (!m_dxfClassIdSeen) {
            const std::int32_t value = reader->getInt32();
            if (value < 0)
                return false;
            classId = static_cast<std::uint32_t>(value);
            m_dxfClassIdSeen = true;
        } else {
            const std::int32_t value = reader->getInt32();
            if (value < 0)
                return false;
            id = static_cast<std::uint32_t>(value);
        }
        break;
    case 10:
        axisPoint.x = reader->getDouble();
        break;
    case 20:
        axisPoint.y = reader->getDouble();
        break;
    case 30:
        axisPoint.z = reader->getDouble();
        break;
    case 11:
        axisVector.x = reader->getDouble();
        break;
    case 21:
        axisVector.y = reader->getDouble();
        break;
    case 31:
        axisVector.z = reader->getDouble();
        break;
    case 40:
        revolveAngle = reader->getDouble();
        break;
    case 41:
        startAngle = reader->getDouble();
        break;
    case 42:
        if (m_dxfTransformCount >= transform.size())
            return false;
        transform[m_dxfTransformCount++] = reader->getDouble();
        break;
    case 43:
        draftAngle = reader->getDouble();
        break;
    case 44:
        draftStartDistance = reader->getDouble();
        break;
    case 45:
        draftEndDistance = reader->getDouble();
        break;
    case 46:
        twistAngle = reader->getDouble();
        break;
    case 290:
        solid = reader->getInt32() != 0;
        break;
    case 291:
        closeToAxis = reader->getInt32() != 0;
        break;
    default:
        return DRW_Surface::parseCode(code, reader);
    }
    return true;
}

void DRW_Surface::resetDwgState() {
    DRW_Entity::reset();
    uIsolines = 0;
    vIsolines = 0;
    modelerFormatVersion = 0;
    acisEmpty = false;
    acisVersion = 0;
    rawAcisData.clear();
    hasRawDwgBody = false;
    rawDwgBodyBitSize = 0;
    rawDwgBodyVersion = DRW::UNKNOWNV;
    crossSectionHandles.clear();
    guideCurveHandles.clear();
    dwgClassNum = 0;
    dwgPayloadDecoded = false;
    m_wireframe = DRW_AcisBrep();
    m_wireframeDecoded = false;

    const auto identity = [] {
        return std::array<double, DRW_ExtrudedSurface::kTransformSize>{
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0};
    };
    switch (eType) {
    case DRW::EXTRUDEDSURFACE: {
        auto *surface = static_cast<DRW_ExtrudedSurface *>(this);
        surface->classId = 0;
        surface->sweepVector = DRW_Coord{0.0, 0.0, 0.0};
        surface->extrudedTransform = identity();
        surface->sweepEntityTransform = identity();
        surface->pathEntityTransform = identity();
        surface->draftAngle = 0.0;
        surface->draftStartDistance = 0.0;
        surface->draftEndDistance = 0.0;
        surface->twistAngle = 0.0;
        surface->scaleFactor = 0.0;
        surface->alignAngle = 0.0;
        surface->solid = false;
        surface->sweepAlignmentFlags = 0;
        surface->pathFlags = 0;
        surface->alignStart = false;
        surface->bank = false;
        surface->basePointSet = false;
        surface->sweepEntityTransformComputed = false;
        surface->pathEntityTransformComputed = false;
        surface->referenceVector = DRW_Coord{0.0, 0.0, 0.0};
        break;
    }
    case DRW::REVOLVEDSURFACE: {
        auto *surface = static_cast<DRW_RevolvedSurface *>(this);
        surface->classId = 0;
        surface->id = 0;
        surface->axisPoint = DRW_Coord{0.0, 0.0, 0.0};
        surface->axisVector = DRW_Coord{0.0, 0.0, 0.0};
        surface->revolveAngle = 0.0;
        surface->startAngle = 0.0;
        surface->transform = identity();
        surface->draftAngle = 0.0;
        surface->draftStartDistance = 0.0;
        surface->draftEndDistance = 0.0;
        surface->twistAngle = 0.0;
        surface->solid = false;
        surface->closeToAxis = false;
        break;
    }
    case DRW::SWEPTSURFACE: {
        auto *surface = static_cast<DRW_SweptSurface *>(this);
        surface->classVersion = 0;
        surface->sweepEntityId = 0;
        surface->sweepData.clear();
        surface->pathEntityId = 0;
        surface->pathData.clear();
        surface->sweepEntityTransform = identity();
        surface->pathEntityTransform = identity();
        surface->draftAngle = 0.0;
        surface->draftStartDistance = 0.0;
        surface->draftEndDistance = 0.0;
        surface->twistAngle = 0.0;
        surface->scaleFactor = 1.0;
        surface->alignAngle = 0.0;
        surface->sweepEntityTransformed = identity();
        surface->pathEntityTransformed = identity();
        surface->solid = false;
        surface->sweepAlignmentFlags = 0;
        surface->pathFlags = 0;
        surface->alignStart = false;
        surface->bank = false;
        surface->basePointSet = false;
        surface->sweepEntityTransformComputed = false;
        surface->pathEntityTransformComputed = false;
        surface->referenceVector = DRW_Coord{0.0, 0.0, 0.0};
        break;
    }
    case DRW::LOFTEDSURFACE: {
        auto *surface = static_cast<DRW_LoftedSurface *>(this);
        surface->loftEntityTransform = identity();
        surface->planeNormalLoftingType = 0;
        surface->startDraftAngle = 0.0;
        surface->endDraftAngle = 0.0;
        surface->startDraftMagnitude = 0.0;
        surface->endDraftMagnitude = 0.0;
        surface->arcLengthParameterization = false;
        surface->noTwist = true;
        surface->alignDirection = true;
        surface->simpleSurfaces = true;
        surface->closedSurfaces = false;
        surface->solid = false;
        surface->ruledSurface = false;
        surface->virtualGuide = false;
        surface->numCrossSections = 0;
        surface->numGuideCurves = 0;
        surface->dxfReferenceData.clear();
        surface->pathCurveHandle = 0;
        break;
    }
    case DRW::NURBSURFACE: {
        auto *surface = static_cast<DRW_NurbsSurface *>(this);
        surface->short170 = 0;
        surface->cvHullDisplay = false;
        surface->uvec1 = DRW_Coord{0.0, 0.0, 0.0};
        surface->vvec1 = DRW_Coord{0.0, 0.0, 0.0};
        surface->uvec2 = DRW_Coord{0.0, 0.0, 0.0};
        surface->vvec2 = DRW_Coord{0.0, 0.0, 0.0};
        break;
    }
    default:
        break;
    }
}

bool DRW_Surface::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    dwgBuffer stringProbe = sourceBuf->forkIndependent();
    dwgBuffer *stringStream = version > DRW::AC1018 ? &stringProbe : nullptr;
    if (!DRW_Entity::parseDwg(version, &bodyProbe, stringStream, bs))
        return fail();
    buf = &bodyProbe;
    const std::uint64_t bodyStartBit = currentDwgBit(buf);

    // Surface bodies are opaque here, but their bounded bytes still carry the
    // ACIS payload and must be available for rendering and metadata replay.
    std::uint64_t bodyEndBit = bodyStartBit;
    if (!entityBodyDataEndBit(
            stringStream != nullptr ? *stringStream : bodyProbe,
            version, objSize, bodyEndBit))
        return fail();
    if (bodyEndBit < bodyStartBit)
        return fail();
    if (bodyEndBit > bodyStartBit
        && bodyEndBit - bodyStartBit > std::numeric_limits<std::uint32_t>::max())
        return fail();
    hasRawDwgBody = bodyEndBit > bodyStartBit;
    rawDwgBodyBitSize = static_cast<std::uint32_t>(
        bodyEndBit >= bodyStartBit ? bodyEndBit - bodyStartBit : 0);
    rawDwgBodyVersion = version;
    dwgClassNum = oType >= 0 ? static_cast<std::uint16_t>(oType) : 0;
    dwgPayloadDecoded = false;
    crossSectionHandles.clear();
    guideCurveHandles.clear();
    if (bodyEndBit > bodyStartBit
        && !copyDwgBitRange(*buf, bodyStartBit, bodyEndBit, rawAcisData)) {
        return fail();
    }

    // When has_ds_data is set, the bounded body is an inline DataStorage/SAB
    // payload. Keep it opaque, matching the cross-project reader contract;
    // interpreting its bytes as subtype fields can fabricate counts and make
    // a valid surface fail while reading a nonexistent handle tail.
    if (bodyEndBit > bodyStartBit && !hasDsData) {
        SurfaceDwgPayloadInfo payload;
        if (readSurfaceDwgPayload(
                rawAcisData, bodyEndBit - bodyStartBit, version, eType,
                surfaceHasModelerFormatVersion(eType),
                surfaceModelerFormatVersionLimit(eType), payload)) {
            acisEmpty = payload.acisEmpty;
            acisVersion = payload.acisVersion;
            modelerFormatVersion = payload.modelerFormatVersion;
            uIsolines = payload.uIsolines;
            vIsolines = payload.vIsolines;
            if (payload.extrudedFieldsValid
                && eType == DRW::EXTRUDEDSURFACE) {
                auto *extruded = static_cast<DRW_ExtrudedSurface *>(this);
                extruded->sweepVector = payload.extrudedSweepVector;
                extruded->extrudedTransform = payload.extrudedTransform;
                extruded->sweepEntityTransform =
                    payload.extrudedSweepEntityTransform;
                extruded->pathEntityTransform =
                    payload.extrudedPathEntityTransform;
                extruded->draftAngle = payload.extrudedDraftAngle;
                extruded->draftStartDistance =
                    payload.extrudedDraftStartDistance;
                extruded->draftEndDistance = payload.extrudedDraftEndDistance;
                extruded->twistAngle = payload.extrudedTwistAngle;
                extruded->scaleFactor = payload.extrudedScaleFactor;
                extruded->alignAngle = payload.extrudedAlignAngle;
                extruded->solid = payload.extrudedSolid;
                extruded->sweepAlignmentFlags =
                    payload.extrudedSweepAlignmentFlags;
                extruded->pathFlags = payload.extrudedPathFlags;
                extruded->alignStart = payload.extrudedAlignStart;
                extruded->bank = payload.extrudedBank;
                extruded->basePointSet = payload.extrudedBasePointSet;
                extruded->sweepEntityTransformComputed =
                    payload.extrudedSweepEntityTransformComputed;
                extruded->pathEntityTransformComputed =
                    payload.extrudedPathEntityTransformComputed;
                extruded->referenceVector = payload.extrudedReferenceVector;
            }
            if (payload.sweptFieldsValid && eType == DRW::SWEPTSURFACE) {
                auto *swept = static_cast<DRW_SweptSurface *>(this);
                swept->classVersion = payload.sweptClassVersion;
                swept->sweepEntityId = payload.sweptEntityId;
                swept->sweepData = payload.sweptData;
                swept->pathEntityId = payload.sweptPathEntityId;
                swept->pathData = payload.sweptPathData;
                swept->sweepEntityTransform = payload.sweptEntityTransform;
                swept->pathEntityTransform = payload.sweptPathEntityTransform;
                swept->draftAngle = payload.sweptDraftAngle;
                swept->draftStartDistance = payload.sweptDraftStartDistance;
                swept->draftEndDistance = payload.sweptDraftEndDistance;
                swept->twistAngle = payload.sweptTwistAngle;
                swept->scaleFactor = payload.sweptScaleFactor;
                swept->alignAngle = payload.sweptAlignAngle;
                swept->solid = payload.sweptSolid;
                swept->sweepAlignmentFlags = payload.sweptAlignmentFlags;
                swept->pathFlags = payload.sweptPathFlags;
                swept->alignStart = payload.sweptAlignStart;
                swept->bank = payload.sweptBank;
                swept->basePointSet = payload.sweptBasePointSet;
                swept->sweepEntityTransformComputed =
                    payload.sweptEntityTransformComputed;
                swept->pathEntityTransformComputed =
                    payload.sweptPathEntityTransformComputed;
                swept->referenceVector = payload.sweptReferenceVector;
            }
            if (payload.loftedFieldsValid && eType == DRW::LOFTEDSURFACE) {
                auto *lofted = static_cast<DRW_LoftedSurface *>(this);
                lofted->loftEntityTransform = payload.loftedEntityTransform;
                lofted->planeNormalLoftingType =
                    payload.loftedPlaneNormalLoftingType;
                lofted->startDraftAngle = payload.loftedStartDraftAngle;
                lofted->endDraftAngle = payload.loftedEndDraftAngle;
                lofted->startDraftMagnitude =
                    payload.loftedStartDraftMagnitude;
                lofted->endDraftMagnitude = payload.loftedEndDraftMagnitude;
                lofted->arcLengthParameterization =
                    payload.loftedArcLengthParameterization;
                lofted->noTwist = payload.loftedNoTwist;
                lofted->alignDirection = payload.loftedAlignDirection;
                lofted->simpleSurfaces = payload.loftedSimpleSurfaces;
                lofted->closedSurfaces = payload.loftedClosedSurfaces;
                lofted->solid = payload.loftedSolid;
                lofted->ruledSurface = payload.loftedRuledSurface;
                lofted->virtualGuide = payload.loftedVirtualGuide;
                lofted->numCrossSections = payload.loftedNumCrossSections;
                lofted->numGuideCurves = payload.loftedNumGuideCurves;
            }
            if (payload.nurbsFieldsValid && eType == DRW::NURBSURFACE) {
                auto *nurbs = static_cast<DRW_NurbsSurface *>(this);
                nurbs->short170 = payload.nurbsShort170;
                nurbs->cvHullDisplay = payload.nurbsCvHullDisplay;
                nurbs->uvec1 = payload.nurbsUvec1;
                nurbs->vvec1 = payload.nurbsVvec1;
                nurbs->uvec2 = payload.nurbsUvec2;
                nurbs->vvec2 = payload.nurbsVvec2;
            }
            dwgPayloadDecoded = true;
            if (payload.revolvedFieldsValid
                && eType == DRW::REVOLVEDSURFACE) {
                auto *revolved = static_cast<DRW_RevolvedSurface *>(this);
                revolved->classId = payload.revolvedClassId;
                revolved->id = payload.revolvedId;
                revolved->axisPoint = payload.revolvedAxisPoint;
                revolved->axisVector = payload.revolvedAxisVector;
                revolved->revolveAngle = payload.revolvedAngle;
                revolved->startAngle = payload.revolvedStartAngle;
                revolved->transform = payload.revolvedTransform;
                revolved->draftAngle = payload.revolvedDraftAngle;
                revolved->draftStartDistance = payload.revolvedDraftStartDistance;
                revolved->draftEndDistance = payload.revolvedDraftEndDistance;
                revolved->twistAngle = payload.revolvedTwistAngle;
                revolved->solid = payload.revolvedSolid;
                revolved->closeToAxis = payload.revolvedCloseToAxis;
            }
        }
    }

    // R13-R2004 keep the handle stream inline; seek to the documented body
    // boundary before the common handle parser. R2007+ performs that seek in
    // parseDwgEntHandle() itself.
    dwgBuffer handleProbe = bodyProbe.forkIndependent();
    if (version <= DRW::AC1018 && bodyEndBit > bodyStartBit) {
        if (!handleProbe.setPosition(bodyEndBit >> 3))
            return fail();
        handleProbe.setBitPos(static_cast<std::uint8_t>(bodyEndBit & 7u));
        if (!handleProbe.isGood())
            return fail();
    }
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    if (!DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                       handleEndBit)
        || !handleProbe.isGood())
        return fail();
    std::vector<std::uint32_t> parsedCrossSectionHandles;
    std::vector<std::uint32_t> parsedGuideCurveHandles;
    std::uint32_t parsedPathCurveHandle = 0;
    const auto readHandleList = [&](std::int32_t count,
                                    std::vector<std::uint32_t>& values) {
        if (count < 0
            || count > static_cast<std::int32_t>(
                DRW_LoftedSurface::kMaxReferenceTokenCount)
            || !DRW::reserve(values, count))
            return false;
        for (std::int32_t i = 0; i < count; ++i) {
            dwgHandle value;
            if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                      value))
                return false;
            values.push_back(value.ref);
        }
        return true;
    };
    if (eType == DRW::LOFTEDSURFACE && dwgPayloadDecoded) {
        auto *lofted = static_cast<DRW_LoftedSurface *>(this);
        if (!readHandleList(lofted->numCrossSections,
                            parsedCrossSectionHandles)
            || !readHandleList(lofted->numGuideCurves,
                               parsedGuideCurveHandles))
            return fail();
        dwgHandle pathCurve;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, true,
                                  pathCurve))
            return fail();
        parsedPathCurveHandle = pathCurve.ref;
    }
    if (!handleProbe.isGood())
        return fail();
    crossSectionHandles = std::move(parsedCrossSectionHandles);
    guideCurveHandles = std::move(parsedGuideCurveHandles);
    if (eType == DRW::LOFTEDSURFACE)
        static_cast<DRW_LoftedSurface *>(this)->pathCurveHandle =
            parsedPathCurveHandle;
    *sourceBuf = handleProbe;
    return true;
}

bool DRW_Surface::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                             dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (buf == nullptr || version < DRW::AC1021 || dwgClassNum == 0)
        return false;

    const auto validBitShort = [](int value) {
        return value >= 0
            && value <= std::numeric_limits<std::uint16_t>::max();
    };
    const auto validCount = [](std::int32_t value, std::size_t limit) {
        return value >= 0 && static_cast<std::size_t>(value) <= limit;
    };
    const auto validTypedPayload = [&]() {
        if (!acisEmpty && !rawAcisData.empty())
            return false;
        if (surfaceHasModelerFormatVersion(eType)
            && (!validBitShort(modelerFormatVersion)
                || (eType == DRW::LOFTEDSURFACE
                    && modelerFormatVersion
                        > surfaceModelerFormatVersionLimit(eType))))
            return false;
        if (!validBitShort(uIsolines) || !validBitShort(vIsolines))
            return false;

        switch (eType) {
        case DRW::EXTRUDEDSURFACE: {
            const auto *surface = static_cast<const DRW_ExtrudedSurface *>(this);
            return std::isfinite(surface->draftAngle)
                && std::isfinite(surface->draftStartDistance)
                && std::isfinite(surface->draftEndDistance)
                && std::isfinite(surface->twistAngle)
                && std::isfinite(surface->scaleFactor)
                && std::isfinite(surface->alignAngle)
                && finiteSurfaceCoord(surface->referenceVector)
                && finiteSurfaceCoord(surface->sweepVector)
                && validBitShort(surface->sweepAlignmentFlags)
                && validBitShort(surface->pathFlags)
                && finiteSurfaceMatrix(surface->extrudedTransform)
                && finiteSurfaceMatrix(surface->sweepEntityTransform)
                && finiteSurfaceMatrix(surface->pathEntityTransform);
        }
        case DRW::LOFTEDSURFACE: {
            const auto *surface = static_cast<const DRW_LoftedSurface *>(this);
            return std::isfinite(surface->startDraftAngle)
                && std::isfinite(surface->endDraftAngle)
                && std::isfinite(surface->startDraftMagnitude)
                && std::isfinite(surface->endDraftMagnitude)
                && validCount(surface->numCrossSections,
                              DRW_LoftedSurface::kMaxReferenceTokenCount)
                && validCount(surface->numGuideCurves,
                              DRW_LoftedSurface::kMaxReferenceTokenCount)
                && surface->crossSectionHandles.size()
                    == static_cast<std::size_t>(surface->numCrossSections)
                && surface->guideCurveHandles.size()
                    == static_cast<std::size_t>(surface->numGuideCurves)
                && finiteSurfaceMatrix(surface->loftEntityTransform);
        }
        case DRW::REVOLVEDSURFACE: {
            const auto *surface = static_cast<const DRW_RevolvedSurface *>(this);
            return surface->classId <= std::numeric_limits<std::int32_t>::max()
                && surface->id <= std::numeric_limits<std::int32_t>::max()
                && finiteSurfaceCoord(surface->axisPoint)
                && finiteSurfaceCoord(surface->axisVector)
                && std::isfinite(surface->revolveAngle)
                && std::isfinite(surface->startAngle)
                && std::isfinite(surface->draftAngle)
                && std::isfinite(surface->draftStartDistance)
                && std::isfinite(surface->draftEndDistance)
                && std::isfinite(surface->twistAngle)
                && finiteSurfaceMatrix(surface->transform);
        }
        case DRW::SWEPTSURFACE: {
            const auto *surface = static_cast<const DRW_SweptSurface *>(this);
            return surface->classVersion <= 10
                && surface->sweepEntityId
                    <= std::numeric_limits<std::int32_t>::max()
                && surface->pathEntityId
                    <= std::numeric_limits<std::int32_t>::max()
                && surface->sweepData.size()
                    <= DRW_SweptSurface::kMaxSweepDataSize
                && surface->pathData.size()
                    <= DRW_SweptSurface::kMaxSweepDataSize
                && std::isfinite(surface->draftAngle)
                && std::isfinite(surface->draftStartDistance)
                && std::isfinite(surface->draftEndDistance)
                && std::isfinite(surface->twistAngle)
                && std::isfinite(surface->scaleFactor)
                && std::isfinite(surface->alignAngle)
                && finiteSurfaceCoord(surface->referenceVector)
                && validBitShort(surface->sweepAlignmentFlags)
                && validBitShort(surface->pathFlags)
                && finiteSurfaceMatrix(surface->sweepEntityTransform)
                && finiteSurfaceMatrix(surface->pathEntityTransform);
        }
        case DRW::NURBSURFACE:
            if (version < DRW::AC1027)
                return true;
            {
                const auto *surface = static_cast<const DRW_NurbsSurface *>(this);
                return finiteSurfaceCoord(surface->uvec1)
                    && finiteSurfaceCoord(surface->vvec1)
                    && finiteSurfaceCoord(surface->uvec2)
                    && finiteSurfaceCoord(surface->vvec2);
            }
        case DRW::PLANESURFACE:
            return true;
        default:
            return false;
        }
    };

    if (hasRawDwgBody && rawDwgBodyVersion == version) {
        if (rawDwgBodyBitSize == 0
            || rawDwgBodyBitSize > surfaceRawBitCapacity(rawAcisData))
            return false;
    } else if (!validTypedPayload()) {
        return false;
    }
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    if (hasRawDwgBody && rawDwgBodyVersion == version) {
        if (!putSurfaceRawBits(buf, rawAcisData, rawDwgBodyBitSize))
            return false;
    } else {
        // DXF ACIS/SAB bytes are not the bit-packed DWG ACTION_3DSOLID body.
        // Do not place them directly into a DWG object. Native ACIS emission
        // remains a separate conversion step; empty modeler bodies are valid
        // and allow metadata-only surfaces to round-trip safely.
        if (!acisEmpty && !rawAcisData.empty())
            return false;
        buf->putBit(1); // ACTION_3DSOLID: no ACIS body

        const bool hasModelerFormat = surfaceHasModelerFormatVersion(eType);
        if (hasModelerFormat) {
            if (modelerFormatVersion < 0
                || modelerFormatVersion > 0xFFFF
                || (eType == DRW::LOFTEDSURFACE && modelerFormatVersion > 3))
                return false;
            buf->putBitShort(static_cast<std::uint16_t>(modelerFormatVersion));
        }
        if (uIsolines < 0 || uIsolines > 0xFFFF
            || vIsolines < 0 || vIsolines > 0xFFFF)
            return false;
        buf->putBitShort(static_cast<std::uint16_t>(uIsolines));
        buf->putBitShort(static_cast<std::uint16_t>(vIsolines));

        auto putCoord = [&](const DRW_Coord& value) {
            if (!finiteSurfaceCoord(value))
                return false;
            buf->put3BitDouble(value);
            return true;
        };
        auto putBool = [&](bool value) { buf->putBit(value ? 1 : 0); };
        auto putCount = [&](std::int32_t value, std::size_t limit) {
            return value >= 0 && static_cast<std::size_t>(value) <= limit;
        };

        switch (eType) {
        case DRW::EXTRUDEDSURFACE: {
            const auto *surface = static_cast<const DRW_ExtrudedSurface *>(this);
            if (!std::isfinite(surface->draftAngle)
                || !std::isfinite(surface->draftStartDistance)
                || !std::isfinite(surface->draftEndDistance)
                || !std::isfinite(surface->twistAngle)
                || !std::isfinite(surface->scaleFactor)
                || !std::isfinite(surface->alignAngle)
                || !finiteSurfaceCoord(surface->referenceVector)
                || !finiteSurfaceCoord(surface->sweepVector)
                || surface->sweepAlignmentFlags < 0
                || surface->sweepAlignmentFlags > 0xFFFF
                || surface->pathFlags < 0 || surface->pathFlags > 0xFFFF
                || !finiteSurfaceMatrix(surface->sweepEntityTransform)
                || !finiteSurfaceMatrix(surface->pathEntityTransform))
                return false;
            buf->putBitDouble(surface->draftAngle);
            buf->putBitDouble(surface->draftStartDistance);
            buf->putBitDouble(surface->draftEndDistance);
            buf->putBitDouble(surface->twistAngle);
            buf->putBitDouble(surface->scaleFactor);
            buf->putBitDouble(surface->alignAngle);
            // The matrices were validated above; emit them in their specified
            // position after SweepOptions' scalar fields.
            for (double value : surface->sweepEntityTransform)
                buf->putBitDouble(value);
            for (double value : surface->pathEntityTransform)
                buf->putBitDouble(value);
            putBool(surface->solid);
            buf->putBitShort(static_cast<std::uint16_t>(surface->sweepAlignmentFlags));
            buf->putBitShort(static_cast<std::uint16_t>(surface->pathFlags));
            putBool(surface->alignStart);
            putBool(surface->bank);
            putBool(surface->basePointSet);
            putBool(surface->sweepEntityTransformComputed);
            putBool(surface->pathEntityTransformComputed);
            if (!finiteSurfaceCoord(surface->referenceVector)
                || !finiteSurfaceCoord(surface->sweepVector)
                || !finiteSurfaceMatrix(surface->extrudedTransform))
                return false;
            putCoord(surface->referenceVector);
            putCoord(surface->sweepVector);
            for (double value : surface->extrudedTransform)
                buf->putBitDouble(value);
            break;
        }
        case DRW::LOFTEDSURFACE: {
            const auto *surface = static_cast<const DRW_LoftedSurface *>(this);
            if (!std::isfinite(surface->startDraftAngle)
                || !std::isfinite(surface->endDraftAngle)
                || !std::isfinite(surface->startDraftMagnitude)
                || !std::isfinite(surface->endDraftMagnitude)
                || !putCount(surface->numCrossSections,
                             DRW_LoftedSurface::kMaxReferenceTokenCount)
                || !putCount(surface->numGuideCurves,
                             DRW_LoftedSurface::kMaxReferenceTokenCount)
                || !finiteSurfaceMatrix(surface->loftEntityTransform))
                return false;
            for (double value : surface->loftEntityTransform)
                buf->putBitDouble(value);
            buf->putBitLong(surface->planeNormalLoftingType);
            buf->putBitDouble(surface->startDraftAngle);
            buf->putBitDouble(surface->endDraftAngle);
            buf->putBitDouble(surface->startDraftMagnitude);
            buf->putBitDouble(surface->endDraftMagnitude);
            putBool(surface->arcLengthParameterization);
            putBool(surface->noTwist);
            putBool(surface->alignDirection);
            putBool(surface->simpleSurfaces);
            putBool(surface->closedSurfaces);
            putBool(surface->solid);
            putBool(surface->ruledSurface);
            putBool(surface->virtualGuide);
            buf->putBitShort(static_cast<std::uint16_t>(surface->numCrossSections));
            buf->putBitShort(static_cast<std::uint16_t>(surface->numGuideCurves));
            break;
        }
        case DRW::REVOLVEDSURFACE: {
            const auto *surface = static_cast<const DRW_RevolvedSurface *>(this);
            if (surface->classId > std::numeric_limits<std::int32_t>::max()
                || surface->id > std::numeric_limits<std::int32_t>::max()
                || !finiteSurfaceCoord(surface->axisPoint)
                || !finiteSurfaceCoord(surface->axisVector)
                || !std::isfinite(surface->revolveAngle)
                || !std::isfinite(surface->startAngle)
                || !std::isfinite(surface->draftAngle)
                || !std::isfinite(surface->draftStartDistance)
                || !std::isfinite(surface->draftEndDistance)
                || !std::isfinite(surface->twistAngle)
                || !finiteSurfaceMatrix(surface->transform))
                return false;
            buf->putBitLong(static_cast<std::int32_t>(surface->classId));
            buf->putBitLong(static_cast<std::int32_t>(surface->id));
            buf->put3BitDouble(surface->axisPoint);
            buf->put3BitDouble(surface->axisVector);
            buf->putBitDouble(surface->revolveAngle);
            buf->putBitDouble(surface->startAngle);
            for (double value : surface->transform)
                buf->putBitDouble(value);
            buf->putBitDouble(surface->draftAngle);
            buf->putBitDouble(surface->draftStartDistance);
            buf->putBitDouble(surface->draftEndDistance);
            buf->putBitDouble(surface->twistAngle);
            putBool(surface->solid);
            putBool(surface->closeToAxis);
            break;
        }
        case DRW::SWEPTSURFACE: {
            const auto *surface = static_cast<const DRW_SweptSurface *>(this);
            if (surface->classVersion > 10
                || surface->sweepEntityId > std::numeric_limits<std::int32_t>::max()
                || surface->pathEntityId > std::numeric_limits<std::int32_t>::max()
                || surface->sweepData.size() > DRW_SweptSurface::kMaxSweepDataSize
                || surface->pathData.size() > DRW_SweptSurface::kMaxSweepDataSize
                || !std::isfinite(surface->draftAngle)
                || !std::isfinite(surface->draftStartDistance)
                || !std::isfinite(surface->draftEndDistance)
                || !std::isfinite(surface->twistAngle)
                || !std::isfinite(surface->scaleFactor)
                || !std::isfinite(surface->alignAngle)
                || !finiteSurfaceCoord(surface->referenceVector)
                || surface->sweepAlignmentFlags < 0
                || surface->sweepAlignmentFlags > 0xFFFF
                || surface->pathFlags < 0 || surface->pathFlags > 0xFFFF
                || !finiteSurfaceMatrix(surface->sweepEntityTransform)
                || !finiteSurfaceMatrix(surface->pathEntityTransform))
                return false;
            buf->putBitLong(static_cast<std::int32_t>(surface->classVersion));
            buf->putBitLong(static_cast<std::int32_t>(surface->sweepEntityId));
            buf->putBitLong(static_cast<std::int32_t>(surface->sweepData.size()));
            if (!surface->sweepData.empty())
                buf->putBytes(surface->sweepData.data(), surface->sweepData.size());
            buf->putBitLong(static_cast<std::int32_t>(surface->pathEntityId));
            buf->putBitLong(static_cast<std::int32_t>(surface->pathData.size()));
            if (!surface->pathData.empty())
                buf->putBytes(surface->pathData.data(), surface->pathData.size());
            buf->putBitDouble(surface->draftAngle);
            buf->putBitDouble(surface->draftStartDistance);
            buf->putBitDouble(surface->draftEndDistance);
            buf->putBitDouble(surface->twistAngle);
            buf->putBitDouble(surface->scaleFactor);
            buf->putBitDouble(surface->alignAngle);
            for (double value : surface->sweepEntityTransform)
                buf->putBitDouble(value);
            for (double value : surface->pathEntityTransform)
                buf->putBitDouble(value);
            putBool(surface->solid);
            buf->putBitShort(static_cast<std::uint16_t>(surface->sweepAlignmentFlags));
            buf->putBitShort(static_cast<std::uint16_t>(surface->pathFlags));
            putBool(surface->alignStart);
            putBool(surface->bank);
            putBool(surface->basePointSet);
            putBool(surface->sweepEntityTransformComputed);
            putBool(surface->pathEntityTransformComputed);
            if (!finiteSurfaceCoord(surface->referenceVector))
                return false;
            putCoord(surface->referenceVector);
            break;
        }
        case DRW::NURBSURFACE: {
            if (version < DRW::AC1027)
                break;
            const auto *surface = static_cast<const DRW_NurbsSurface *>(this);
            if (!finiteSurfaceCoord(surface->uvec1)
                || !finiteSurfaceCoord(surface->vvec1)
                || !finiteSurfaceCoord(surface->uvec2)
                || !finiteSurfaceCoord(surface->vvec2))
                return false;
            buf->putBitShort(surface->short170);
            putBool(surface->cvHullDisplay);
            buf->put3BitDouble(surface->uvec1);
            buf->put3BitDouble(surface->vvec1);
            buf->put3BitDouble(surface->uvec2);
            buf->put3BitDouble(surface->vvec2);
            break;
        }
        case DRW::PLANESURFACE:
            break;
        default:
            return false;
        }
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;
    if (eType == DRW::LOFTEDSURFACE) {
        dwgBufferW *hb = handleBuf != nullptr ? handleBuf : buf;
        for (std::uint32_t ref : crossSectionHandles)
            putSurfaceHandle(hb, ref);
        for (std::uint32_t ref : guideCurveHandles)
            putSurfaceHandle(hb, ref);
        putSurfaceHandle(hb,
                         static_cast<const DRW_LoftedSurface *>(this)
                             ->pathCurveHandle);
    }
    return true;
}

bool DRW_Dimension::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 2:
        name = reader->getString();
        break;
    case 3:
        style = reader->getUtf8String();
        break;
    case 70:
        type = reader->getInt32();
        break;
    case 71:
        align = reader->getInt32();
        break;
    case 72:
        linesty = reader->getInt32();
        break;
    case 10:
        defPoint.x = reader->getDouble();
        break;
    case 20:
        defPoint.y = reader->getDouble();
        break;
    case 30:
        defPoint.z = reader->getDouble();
        break;
    case 11:
        textPoint.x = reader->getDouble();
        break;
    case 21:
        textPoint.y = reader->getDouble();
        break;
    case 31:
        textPoint.z = reader->getDouble();
        break;
    case 12:
        clonePoint.x = reader->getDouble();
        break;
    case 22:
        clonePoint.y = reader->getDouble();
        break;
    case 32:
        clonePoint.z = reader->getDouble();
        break;
    case 13:
        def1.x = reader->getDouble();
        break;
    case 23:
        def1.y = reader->getDouble();
        break;
    case 33:
        def1.z = reader->getDouble();
        break;
    case 14:
        def2.x = reader->getDouble();
        break;
    case 24:
        def2.y = reader->getDouble();
        break;
    case 34:
        def2.z = reader->getDouble();
        break;
    case 15:
        circlePoint.x = reader->getDouble();
        break;
    case 25:
        circlePoint.y = reader->getDouble();
        break;
    case 35:
        circlePoint.z = reader->getDouble();
        break;
    case 16:
        arcPoint.x = reader->getDouble();
        break;
    case 26:
        arcPoint.y = reader->getDouble();
        break;
    case 36:
        arcPoint.z = reader->getDouble();
        break;
    case 41:
        linefactor = reader->getDouble();
        break;
    case 53:
        rot = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 52:
        oblique = reader->getDouble();
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 51:
        hdir = reader->getDouble();
        break;
    case 42:
        measureValue = reader->getDouble();
        break;
    case 74:
        flipArrow1 = reader->getInt32() != 0;
        break;
    case 75:
        flipArrow2 = reader->getInt32() != 0;
        break;
    case 76:
        genTol = reader->getInt32() != 0;
        break;
    case 77:
        limGen = reader->getInt32() != 0;
        break;
    case 43:
        tolPlus = reader->getDouble();
        break;
    case 44:
        tolMinus = reader->getDouble();
        break;
    case 45:
        tolScale = reader->getDouble();
        break;
    case 78:
        tolDecimals = reader->getInt32();
        break;
    case 79:
        tolAlign = reader->getInt32();
        break;
    case 80:
        tolZero = reader->getInt32();
        break;
    case 81:
        altTolDecimals = reader->getInt32();
        break;
    case 82:
        altZero = reader->getInt32();
        break;
    case 83:
        altTolZero = reader->getInt32();
        break;
    case 84:
        textMove = reader->getInt32();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Dimension::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs)
{
    DRW_UNUSED( version);
    DRW_UNUSED( buf);
    DRW_UNUSED( bs);

    DRW_DBG("DRW_Dimension::parseDwg(): base class implemntation should never be called direct!\n");

    return false;
}

void DRW_Dimension::resetDwgState() {
    DRW_Entity::reset();
    type = 0;
    name.clear();
    defPoint = DRW_Coord{0.0, 0.0, 0.0};
    textPoint = DRW_Coord{0.0, 0.0, 0.0};
    text.clear();
    style = "STANDARD";
    align = 5;
    linesty = 1;
    linefactor = 1.0;
    rot = 0.0;
    extPoint = DRW_Coord{0.0, 0.0, 1.0};
    hdir = 0.0;
    clonePoint = DRW_Coord{0.0, 0.0, 0.0};
    def1 = DRW_Coord{0.0, 0.0, 0.0};
    def2 = DRW_Coord{0.0, 0.0, 0.0};
    angle = 0.0;
    oblique = 0.0;
    circlePoint = DRW_Coord{0.0, 0.0, 0.0};
    arcPoint = DRW_Coord{0.0, 0.0, 0.0};
    length = 0.0;
    measureValue = 0.0;
    flipArrow1 = false;
    flipArrow2 = false;
    genTol = false;
    limGen = false;
    tolPlus = 0.0;
    tolMinus = 0.0;
    tolScale = 0.0;
    tolDecimals = 0;
    tolAlign = 0;
    tolZero = 0;
    altTolDecimals = 0;
    altZero = 0;
    altTolZero = 0;
    textMove = 0;
    dimStyleH = dwgHandle{};
    blockH = dwgHandle{};
}

bool DRW_Dimension::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, std::uint32_t bs /*= 0*/) {
    (void)sBuf;
    resetDwgState();
    if (buf == nullptr)
        return false;

    dwgBuffer sBuff = *buf;
    dwgBuffer *commonStringBuffer = buf;
    if (version > DRW::AC1018) {//2007+
        commonStringBuffer = &sBuff; //separate buffer for strings
    }

    if (!DRW_Entity::parseDwg(version, buf, commonStringBuffer, bs))
        return false;

    DRW_DBG("\n***************************** parsing dimension *********************************************");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    dwgBuffer bodyProbe = buf->forkIndependent();
    dwgBuffer stringProbe = sBuff;
    dwgBuffer *textBuffer = &bodyProbe;
    if (version > DRW::AC1018)
        textBuffer = &stringProbe;
    std::uint64_t stringStartBit = bodyEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1018) {
        stringStartBit = currentDwgBit(textBuffer);
        if (version > DRW::AC1021) {
            std::uint64_t ignoredBodyEndBit = 0;
            if (!entityBodyDataEndBit(*textBuffer, version, objSize,
                                      ignoredBodyEndBit, &stringEndBit))
                return false;
        }
    }

    std::uint8_t parsedDimVersion = 0;
    if (version > DRW::AC1021) { //2010+
        if (!readBoundedRawChar8(bodyProbe, bodyEndBit, parsedDimVersion))
            return false;
        DRW_DBG("\ndimVersion: "); DRW_DBG(parsedDimVersion);
    }

    DRW_Coord parsedExtrusion;
    DRW_Coord parsedTextPoint;
    DRW_Coord ignoredInsertionPoint;
    double parsedTextPointZ = 0.0;
    double ignoredInsertionRotation = 0.0;
    std::uint8_t parsedType = 0;
    if (!readBoundedBitCoord(bodyProbe, bodyEndBit, parsedExtrusion)
        || !readBoundedRawDouble(bodyProbe, bodyEndBit, parsedTextPoint.x)
        || !readBoundedRawDouble(bodyProbe, bodyEndBit, parsedTextPoint.y)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedTextPointZ)
        || !readBoundedRawChar8(bodyProbe, bodyEndBit, parsedType)) {
        return false;
    }
    parsedTextPoint.z = parsedTextPointZ;

    // ODA §20.4.22: Extrusion is plain 3BD (NOT BE) — confirmed by libreDWG dwg_spec_shared.h
    DRW_DBG("\nextPoint: "); DRW_DBGPT(parsedExtrusion.x, parsedExtrusion.y,
                                       parsedExtrusion.z);
    DRW_DBG("\ntextPoint: "); DRW_DBGPT(parsedTextPoint.x, parsedTextPoint.y,
                                        parsedTextPoint.z);
    DRW_DBG("\ntype (70) read: "); DRW_DBG(parsedType);
    const std::uint8_t parsedDimensionType =
        static_cast<std::uint8_t>((parsedType & 1) ? parsedType & 0x7F
                                                    : parsedType | 0x80);
    int parsedTypeValue = (parsedDimensionType & 2) ? parsedDimensionType | 0x20
                                                    : parsedDimensionType & 0xDF;
    DRW_DBG(" type (70) set: "); DRW_DBG(parsedTypeValue);
    //clear last 3 bits to set integer dim type
    parsedTypeValue &= 0xF8;
    UTF8STRING parsedText;
    if (version > DRW::AC1018 && stringStartBit >= stringEndBit) {
        parsedText.clear();
    } else if (!readBoundedVariableText(*textBuffer, stringEndBit, version,
                                        parsedText)) {
        return false;
    }
    DRW_DBG("\nforced dim text: "); DRW_DBG(parsedText.c_str());
    double parsedRot = 0.0;
    double parsedHDir = 0.0;
    if (!readBoundedBitDouble(bodyProbe, bodyEndBit, parsedRot)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedHDir)
        || !readBoundedBitCoord(bodyProbe, bodyEndBit, ignoredInsertionPoint)
        || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                 ignoredInsertionRotation)) {
        return false;
    }
    DRW_DBG("\ninspoint: "); DRW_DBGPT(ignoredInsertionPoint.x,
                                       ignoredInsertionPoint.y,
                                       ignoredInsertionPoint.z);
    DRW_DBG(" insRot_code54: "); DRW_DBG(ignoredInsertionRotation);

    std::uint16_t parsedAlign = 5;
    std::uint16_t parsedLineStyle = 1;
    double parsedLineFactor = 1.0;
    double parsedMeasureValue = 0.0;
    bool parsedFlipArrow1 = false;
    bool parsedFlipArrow2 = false;
    if (version > DRW::AC1014) { //2000+
        if (!readBoundedBitShort(bodyProbe, bodyEndBit, parsedAlign)
            || !readBoundedBitShort(bodyProbe, bodyEndBit, parsedLineStyle)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit, parsedLineFactor)
            || !readBoundedBitDouble(bodyProbe, bodyEndBit,
                                     parsedMeasureValue))
            return false;
        DRW_DBG("\n  actMeas_code42: "); DRW_DBG(parsedMeasureValue);
        if (version > DRW::AC1018) { //2007+
            bool ignoredBit = false;
            if (!readBoundedBit(bodyProbe, bodyEndBit, ignoredBit)
                || !readBoundedBit(bodyProbe, bodyEndBit, parsedFlipArrow1)
                || !readBoundedBit(bodyProbe, bodyEndBit, parsedFlipArrow2))
                return false;
            DRW_DBG("\n2007, unk, flip1, flip2: "); DRW_DBG(ignoredBit);
            DRW_DBG(parsedFlipArrow1); DRW_DBG(parsedFlipArrow2);
        }
    }
    DRW_Coord parsedClonePoint;
    if (!readBoundedRawDouble(bodyProbe, bodyEndBit, parsedClonePoint.x)
        || !readBoundedRawDouble(bodyProbe, bodyEndBit, parsedClonePoint.y))
        return false;
    parsedClonePoint.z = 0.0;
    if (!bodyProbe.isGood() || !textBuffer->isGood()
        || currentDwgBit(&bodyProbe) > bodyEndBit
        || currentDwgBit(textBuffer) > stringEndBit
        || !std::isfinite(parsedExtrusion.x)
        || !std::isfinite(parsedExtrusion.y)
        || !std::isfinite(parsedExtrusion.z)
        || !std::isfinite(parsedTextPoint.x)
        || !std::isfinite(parsedTextPoint.y)
        || !std::isfinite(parsedTextPoint.z)
        || !std::isfinite(ignoredInsertionPoint.x)
        || !std::isfinite(ignoredInsertionPoint.y)
        || !std::isfinite(ignoredInsertionPoint.z)
        || !std::isfinite(parsedRot)
        || !std::isfinite(parsedHDir)
        || !std::isfinite(ignoredInsertionRotation)
        || !std::isfinite(parsedLineFactor)
        || !std::isfinite(parsedMeasureValue)
        || !std::isfinite(parsedClonePoint.x)
        || !std::isfinite(parsedClonePoint.y))
        return false;

    extPoint = parsedExtrusion;
    textPoint = parsedTextPoint;
    type = parsedTypeValue;
    text = parsedText;
    rot = parsedRot;
    hdir = parsedHDir;
    align = parsedAlign;
    linesty = parsedLineStyle;
    linefactor = parsedLineFactor;
    measureValue = parsedMeasureValue;
    flipArrow1 = parsedFlipArrow1;
    flipArrow2 = parsedFlipArrow2;
    clonePoint = parsedClonePoint;
    *buf = bodyProbe;
    return true;
}

bool DRW_Dimension::parseDwgDimensionHandles(DRW::Version version,
                                             dwgBuffer *buf,
                                             std::uint64_t handleEndBit) {
    if (buf == nullptr)
        return false;
    dwgBuffer probe = buf->forkIndependent();
    if (!parseDwgEntHandle(version, &probe, true, handleEndBit))
        return false;
    dwgHandle parsedDimStyle;
    dwgHandle parsedBlock;
    if (!readBoundedDwgHandle(probe, handleEndBit, 0, false, parsedDimStyle)
        || !readBoundedDwgHandle(probe, handleEndBit, 0, false, parsedBlock))
        return false;
    dimStyleH = parsedDimStyle;
    blockH = parsedBlock;
    *buf = probe;
    return true;
}

bool DRW_DimAligned::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    if (oType == 0x15)
        DRW_DBG("\n***************************** parsing dim linear *********************************************\n");
    else
        DRW_DBG("\n***************************** parsing dim aligned *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDef1;
    DRW_Coord parsedDef2;
    DRW_Coord parsedDimPoint;
    double parsedOblique = 0.0;
    double parsedAngle = 0.0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDef1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef2)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDimPoint)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedOblique))
        return fail();
    if (oType == 0x15
        && !readBoundedBitDouble(*buf, bodyEndBit, parsedAngle))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!std::isfinite(parsedOblique) || !std::isfinite(parsedAngle)
        || !finite(parsedDef1) || !finite(parsedDef2)
        || !finite(parsedDimPoint)) {
        return fail();
    }
    setPt3(parsedDef1); //def1
    setPt4(parsedDef2);
    setDefPoint(parsedDimPoint);
    setOb52(parsedOblique * ARAD);  // radians → degrees
    if (oType == 0x15)
        setAn50(parsedAngle * ARAD);
    else
        type |= 1;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAligned::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    *sourceBuf = probe;
    return true;
 }

bool DRW_DimRadial::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    DRW_DBG("\n***************************** parsing dim radial *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDefPoint;
    DRW_Coord parsedCenter;
    double parsedLength = 0.0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedCenter)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedLength)
        || !std::isfinite(parsedDefPoint.x)
        || !std::isfinite(parsedDefPoint.y)
        || !std::isfinite(parsedDefPoint.z)
        || !std::isfinite(parsedCenter.x)
        || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z)
        || !std::isfinite(parsedLength))
        return fail();
    setDefPoint(parsedDefPoint); //code 10
    DRW_DBG("defPoint: "); DRW_DBGPT(parsedDefPoint.x, parsedDefPoint.y,
                                      parsedDefPoint.z);
    setPt5(parsedCenter); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(parsedCenter.x, parsedCenter.y,
                                           parsedCenter.z);
    setRa40(parsedLength); //leader length code 40
    DRW_DBG("\nleader length: "); DRW_DBG(getRa40());
    type |= 4;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimRadial::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    *sourceBuf = probe;
    return true;
}

// DRW_DimLargeRadial (AcDbRadialDimensionLarge, LARGE_RADIAL_DIMENSION).
// DXF group-code parser: the AcDbRadialDimensionLarge subclass overloads codes
// 13/14/15/40 (chord / override center / jog point / jog angle), so gate them on
// the subclass marker (like DRW_DimArc). The chord point is stored as the radial
// diameter point so the existing addDimRadial consumer renders center→chord.
bool DRW_DimLargeRadial::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    if (code == 100) {
        std::string s = reader->getString();
        if (s == "AcDbRadialDimensionLarge") {
            m_largeRadialSubclassSeen = true;
            return true;
        }
        return DRW_Dimension::parseCode(code, reader);
    }
    if (m_largeRadialSubclassSeen) {
        DRW_Coord chord;
        switch (code) {
        case 13: chord = getPt5(); chord.x = reader->getDouble(); setPt5(chord); return true;
        case 23: chord = getPt5(); chord.y = reader->getDouble(); setPt5(chord); return true;
        case 33: chord = getPt5(); chord.z = reader->getDouble(); setPt5(chord); return true;
        case 14: overrideCenterPoint.x = reader->getDouble(); return true;
        case 24: overrideCenterPoint.y = reader->getDouble(); return true;
        case 34: overrideCenterPoint.z = reader->getDouble(); return true;
        case 15: jogPoint.x = reader->getDouble(); return true;
        case 25: jogPoint.y = reader->getDouble(); return true;
        case 35: jogPoint.z = reader->getDouble(); return true;
        case 40: jogAngle = reader->getDouble(); return true;
        default: break;
        }
    }
    return DRW_Dimension::parseCode(code, reader);
}

// DRW_DimLargeRadial DWG body: five subclass reads then the dim-style and
// anon-block handles.  The three subclass points are ordered
//   definition point, JOG point, jog angle, CHORD point, OVERRIDDEN center
// so that the decoded fields match the DXF group codes (chord=13, override=14,
// jog=15) and libdxfrw's own DXF parseCode.  The read-only reference parser
// (parseLargeRadialDimension) labels the 2nd/4th/5th reads chord/override/jog,
// i.e. a cyclic rotation of the point roles; that is inconsistent with the DXF
// semantics and with an ODA File Converter DXF↔DWG round-trip (which preserves
// codes 13/14/15 exactly).  Verified by large_radial_dim_dwg_tests.cpp against
// an ODA-synthesized fixture, cross-checked with the dwg-parser's DXF read.
// Only the field labels change vs. the reference parser — the read sizes/order
// (3BD, 3BD, BD, 3BD, 3BD) are identical, so buffer alignment is unchanged.
bool DRW_DimLargeRadial::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    overrideCenterPoint = DRW_Coord{0.0, 0.0, 0.0};
    jogPoint = DRW_Coord{0.0, 0.0, 0.0};
    jogAngle = 0.0;
    m_largeRadialSubclassSeen = false;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        overrideCenterPoint = DRW_Coord{0.0, 0.0, 0.0};
        jogPoint = DRW_Coord{0.0, 0.0, 0.0};
        jogAngle = 0.0;
        m_largeRadialSubclassSeen = false;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDefPoint;
    DRW_Coord parsedJogPoint;
    DRW_Coord parsedChordPoint;
    DRW_Coord parsedOverrideCenter;
    double parsedJogAngle = 0.0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedJogPoint)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedJogAngle)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedChordPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedOverrideCenter)
        || !std::isfinite(parsedDefPoint.x)
        || !std::isfinite(parsedDefPoint.y)
        || !std::isfinite(parsedDefPoint.z)
        || !std::isfinite(parsedJogPoint.x)
        || !std::isfinite(parsedJogPoint.y)
        || !std::isfinite(parsedJogPoint.z)
        || !std::isfinite(parsedChordPoint.x)
        || !std::isfinite(parsedChordPoint.y)
        || !std::isfinite(parsedChordPoint.z)
        || !std::isfinite(parsedOverrideCenter.x)
        || !std::isfinite(parsedOverrideCenter.y)
        || !std::isfinite(parsedOverrideCenter.z)
        || !std::isfinite(parsedJogAngle))
        return fail();
    setDefPoint(parsedDefPoint);          // definition point (code 10)
    jogPoint = parsedJogPoint;            // jog vertex (code 15)
    jogAngle = parsedJogAngle;            // jog transverse angle (code 40)
    setPt5(parsedChordPoint);             // chord point (code 13)
    overrideCenterPoint = parsedOverrideCenter; // overridden center (code 14)
    type |= 4;                                  // radial dimension type bit
    if (!buf->isGood())
        return fail();
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        return fail();
    }
    *sourceBuf = probe;
    return true;
}

bool DRW_DimDiametric::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    DRW_DBG("\n***************************** parsing dim diametric *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedCenter;
    DRW_Coord parsedDefPoint;
    double parsedLength = 0.0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedCenter)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedLength)
        || !std::isfinite(parsedCenter.x)
        || !std::isfinite(parsedCenter.y)
        || !std::isfinite(parsedCenter.z)
        || !std::isfinite(parsedDefPoint.x)
        || !std::isfinite(parsedDefPoint.y)
        || !std::isfinite(parsedDefPoint.z)
        || !std::isfinite(parsedLength))
        return fail();
    setPt5(parsedCenter); //center pt  code 15
    DRW_DBG("center point: "); DRW_DBGPT(parsedCenter.x, parsedCenter.y,
                                          parsedCenter.z);
    setDefPoint(parsedDefPoint); //code 10
    DRW_DBG("\ndefPoint: "); DRW_DBGPT(parsedDefPoint.x, parsedDefPoint.y,
                                       parsedDefPoint.z);
    setRa40(parsedLength); //leader length code 40
    DRW_DBG("\nleader length: "); DRW_DBG(getRa40());
    type |= 3;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimDiametric::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    *sourceBuf = probe;
    return true;
}

bool DRW_DimAngular::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    DRW_DBG("\n***************************** parsing dim angular *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedArcPoint;
    DRW_Coord parsedDef1;
    DRW_Coord parsedDef2;
    DRW_Coord parsedCenter;
    DRW_Coord parsedDefPoint;
    if (!readBoundedRawDouble(*buf, bodyEndBit, parsedArcPoint.x)
        || !readBoundedRawDouble(*buf, bodyEndBit, parsedArcPoint.y)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef2)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedCenter)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint))
        return fail();
    parsedArcPoint.z = 0.0;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedArcPoint) || !finite(parsedDef1) || !finite(parsedDef2)
        || !finite(parsedCenter) || !finite(parsedDefPoint))
        return fail();
    setPt6(parsedArcPoint); //code 16
    DRW_DBG("arc Point: "); DRW_DBGPT(parsedArcPoint.x, parsedArcPoint.y,
                                       parsedArcPoint.z);
    setPt3(parsedDef1); //def1  code 13
    DRW_DBG("\ndef1: "); DRW_DBGPT(parsedDef1.x, parsedDef1.y, parsedDef1.z);
    setPt4(parsedDef2); //def2  code 14
    DRW_DBG("\ndef2: "); DRW_DBGPT(parsedDef2.x, parsedDef2.y, parsedDef2.z);
    setPt5(parsedCenter); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(parsedCenter.x, parsedCenter.y,
                                          parsedCenter.z);
    setDefPoint(parsedDefPoint); //code 10
    DRW_DBG("\ndefPoint: "); DRW_DBGPT(parsedDefPoint.x, parsedDefPoint.y,
                                       parsedDefPoint.z);
    type |= 0x02;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAngular::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    *sourceBuf = probe;
    return true;
}

bool DRW_DimAngular3p::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    DRW_DBG("\n***************************** parsing dim angular3p *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDefPoint;
    DRW_Coord parsedDef1;
    DRW_Coord parsedDef2;
    DRW_Coord parsedCenter;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef2)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedCenter))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedDefPoint) || !finite(parsedDef1) || !finite(parsedDef2)
        || !finite(parsedCenter))
        return fail();
    setDefPoint(parsedDefPoint); //code 10
    DRW_DBG("defPoint: "); DRW_DBGPT(parsedDefPoint.x, parsedDefPoint.y,
                                     parsedDefPoint.z);
    setPt3(parsedDef1); //def1  code 13
    DRW_DBG("\ndef1: "); DRW_DBGPT(parsedDef1.x, parsedDef1.y, parsedDef1.z);
    setPt4(parsedDef2); //def2  code 14
    DRW_DBG("\ndef2: "); DRW_DBGPT(parsedDef2.x, parsedDef2.y, parsedDef2.z);
    setPt5(parsedCenter); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(parsedCenter.x, parsedCenter.y,
                                          parsedCenter.z);
    type |= 0x05;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");
    if (!buf->isGood())
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAngular3p::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    *sourceBuf = probe;
    return true;
}

bool DRW_DimOrdinate::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    DRW_Dimension::resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;

    DRW_DBG("\n***************************** parsing dim ordinate *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDefPoint;
    DRW_Coord parsedDef1;
    DRW_Coord parsedDef2;
    std::uint8_t type2 = 0;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef2)
        || !readBoundedRawChar8(*buf, bodyEndBit, type2))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedDefPoint) || !finite(parsedDef1) || !finite(parsedDef2))
        return fail();
    setDefPoint(parsedDefPoint);
    DRW_DBG("defPoint: "); DRW_DBGPT(parsedDefPoint.x, parsedDefPoint.y,
                                     parsedDefPoint.z);
    setPt3(parsedDef1); //def1
    DRW_DBG("\ndef1: "); DRW_DBGPT(parsedDef1.x, parsedDef1.y, parsedDef1.z);
    setPt4(parsedDef2);
    DRW_DBG("\ndef2: "); DRW_DBGPT(parsedDef2.x, parsedDef2.y, parsedDef2.z);
    DRW_DBG("type2 (70) read: "); DRW_DBG(type2);
    // 0B.1: x-vs-y ordinate flag is DXF group-70 bit 6 (0x40), matching the
    // filter (rs_filterdxfrw.cpp `type & 64`) and the DWG parseCode path.
    // (Previously set bit 7/0x80, which the filter never checks.) The clear
    // mask 0xBF already clears 0x40. The DIMENSION base type byte (bit 7) is
    // a separate field — see :6141/:6409/:6660, NOT touched here.
    int parsedType = type;
    parsedType = (type2 & 1) ? parsedType | 0x40 : parsedType & 0xBF;
    DRW_DBG(" type (70) set: "); DRW_DBG(parsedType);
    parsedType |= 6;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(parsedType); DRW_DBG("\n");

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAligned::parseDwg()\n");
        return fail();
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    if (!buf->isGood())
        return fail();
    type = parsedType;
    *sourceBuf = probe;
    return true;
}

// ----------------------------------------------------------------------------
// DRW_Dimension shared base encoder (R2000 / AC1015)
// ----------------------------------------------------------------------------
bool DRW_Dimension::encodeDwgDimBase(DRW::Version version, dwgBufferW *buf,
                                     dwgBufferW *strBuf) const {
    // ODA §20.4.22: version RC present for R2010+ (mirrors parseDwg read at version > AC1021)
    if (version > DRW::AC1021)
        buf->putRawChar8(0);
    // R2007+: the dim text below is routed to strBuf (the separate string
    // stream) via the (strBuf ? strBuf : buf) selector in putVariableText.
    buf->put3BitDouble(extPoint);        // 3BD per ODA §20.4.22 (NOT BE, NO padding bits)
    buf->putRawDouble(textPoint.x);
    buf->putRawDouble(textPoint.y);
    buf->putBitDouble(textPoint.z);
    // Reverse the parseDwg type-byte transformation:
    //   parseDwg bit0=0 → type bit7 set;  bit0=1 → type bit7 clear
    //   parseDwg bit1=1 → type bit5 set;  bit1=0 → type bit5 clear
    std::uint8_t rawByte = static_cast<std::uint8_t>(
        ((type & 0x80) ? 0 : 1) | ((type & 0x20) ? 2 : 0));
    buf->putRawChar8(rawByte);
    (strBuf ? strBuf : buf)->putVariableText(version, text);
    buf->putBitDouble(rot);
    buf->putBitDouble(hdir);
    // ins_scale (3BD) of the dimension's anonymous block — not stored by the
    // reader, but ODA/libreDWG default it to (1,1,1) (dwg.spec FIELD_3BD_1), not
    // (0,0,0). A zero scale is degenerate for ODA consumers. (write-review #46)
    const DRW_Coord insScale{1.0, 1.0, 1.0};
    buf->put3BitDouble(insScale);
    buf->putBitDouble(0.0);         // ins_rotation (code 54) — default 0, not stored
    // R2000 (version > AC1014): alignment, spacing, line factor, measure
    buf->putBitShort(static_cast<std::uint16_t>(align));
    buf->putBitShort(static_cast<std::uint16_t>(linesty));
    buf->putBitDouble(linefactor);
    buf->putBitDouble(measureValue);
    if (version > DRW::AC1018) {
        buf->putBit(0); // unknown R2007+ bit
        buf->putBit(flipArrow1 ? 1 : 0);
        buf->putBit(flipArrow2 ? 1 : 0);
    }
    buf->putRawDouble(clonePoint.x);
    buf->putRawDouble(clonePoint.y);
    return true;
}

// Helper: emit dimStyleH (defaults to STANDARD=0x15) and blockH.
static void putDimHandles(dwgBufferW *buf, const dwgHandle& dimStyleH, const dwgHandle& blockH,
                          dwgBufferW *hBuf = nullptr) {
    dwgBufferW *hb = hBuf ? hBuf : buf;
    dwgHandle dsH;
    dsH.code = 5;
    dsH.ref  = (dimStyleH.ref == 0) ? 0x15 : dimStyleH.ref;
    dsH.size = 0;
    if (dsH.ref != 0) { std::uint32_t t = dsH.ref; while (t != 0) { t >>= 8; ++dsH.size; } }
    hb->putHandle(dsH);

    dwgHandle bhH;
    bhH.code = (blockH.ref == 0) ? 0 : 5;
    bhH.ref  = blockH.ref;
    bhH.size = 0;
    if (bhH.ref != 0) { std::uint32_t t = bhH.ref; while (t != 0) { t >>= 8; ++bhH.size; } }
    hb->putHandle(bhH);
}

// ----------------------------------------------------------------------------
// DRW_DimAligned::encodeDwg  (oType=22)
// ----------------------------------------------------------------------------
bool DRW_DimAligned::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 22;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getPt3());      // def1
    buf->put3BitDouble(getPt4());      // def2
    buf->put3BitDouble(getDefPoint()); // defPoint
    buf->putBitDouble(getOb52() / ARAD);  // oblique: degrees → radians
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimLinear::encodeDwg  (oType=21)
// ----------------------------------------------------------------------------
bool DRW_DimLinear::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 21;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getPt3());      // def1
    buf->put3BitDouble(getPt4());      // def2
    buf->put3BitDouble(getDefPoint()); // defPoint
    buf->putBitDouble(getOb52() / ARAD);  // oblique: degrees → radians
    buf->putBitDouble(getAn50() / ARAD);  // rotation angle: degrees → radians
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimRadial::encodeDwg  (oType=25)
// ----------------------------------------------------------------------------
bool DRW_DimRadial::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 25;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getDefPoint());  // center point (code 10)
    buf->put3BitDouble(getPt5());       // diameter point (code 15)
    buf->putBitDouble(getRa40());       // leader length (code 40)
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimLargeRadial::encodeDwg  (oType=519, custom AcDbRadialDimensionLarge)
// ----------------------------------------------------------------------------
bool DRW_DimLargeRadial::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                   std::uint32_t bs, dwgBufferW *strBuf,
                                   dwgBufferW *handleBuf) {
    (void)bs;
    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getCenterPoint());       // definition point (code 10)
    buf->put3BitDouble(jogPoint);               // jog vertex (code 15)
    buf->putBitDouble(jogAngle);                // jog transverse angle (code 40)
    buf->put3BitDouble(getChordPoint());        // chord point (code 13)
    buf->put3BitDouble(overrideCenterPoint);    // overridden center (code 14)
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimDiametric::encodeDwg  (oType=26)
// ----------------------------------------------------------------------------
bool DRW_DimDiametric::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 26;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getPt5());       // first diameter point (code 15) — matches parseDwg order
    buf->put3BitDouble(getDefPoint());  // opposite point (code 10)
    buf->putBitDouble(getRa40());       // leader length (code 40)
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimAngular::encodeDwg  (oType=24, 2-line angular)
// ----------------------------------------------------------------------------
bool DRW_DimAngular::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 24;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    // arcPoint is 2RD (not 3BD) in parseDwg — only x and y
    buf->putRawDouble(getPt6().x);
    buf->putRawDouble(getPt6().y);
    buf->put3BitDouble(getPt3());       // def1 (line 1 start)
    buf->put3BitDouble(getPt4());       // def2 (line 1 end)
    buf->put3BitDouble(getPt5());       // circlePoint (center)
    buf->put3BitDouble(getDefPoint());  // defPoint
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimAngular3p::encodeDwg  (oType=23, 3-point angular)
// ----------------------------------------------------------------------------
bool DRW_DimAngular3p::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 23;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getDefPoint());  // defPoint (code 10)
    buf->put3BitDouble(getPt3());       // def1 (code 13)
    buf->put3BitDouble(getPt4());       // def2 (code 14)
    buf->put3BitDouble(getPt5());       // circlePoint / vertex (code 15)
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimArc::parseCode  (DXF group-code parser)
// ----------------------------------------------------------------------------
bool DRW_DimArc::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        std::string s = reader->getString();
        if (s == "AcDbArcDimension") {
            m_arcSubclassSeen = true;
            return true;
        }
        // Fall through for AcDbEntity / AcDbDimension so base classes see them
        return DRW_Dimension::parseCode(code, reader);
    }
    if (m_arcSubclassSeen) {
        switch (code) {
        case 40: arcStartAngle = reader->getDouble();         return true;
        case 41: arcEndAngle   = reader->getDouble();         return true;
        case 70: arcSymbol     = reader->getInt32();          return true;
        case 71: isPartial     = reader->getInt32() != 0;    return true;
        }
    }
    switch (code) {
    case 17: leaderPt2.x = reader->getDouble(); return true;
    case 27: leaderPt2.y = reader->getDouble(); return true;
    case 37: leaderPt2.z = reader->getDouble(); return true;
    }
    return DRW_Dimension::parseCode(code, reader);
}

// ----------------------------------------------------------------------------
// DRW_DimArc::parseDwg  (ODA DWG spec §20.4.19)
// ----------------------------------------------------------------------------
bool DRW_DimArc::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    DRW_Dimension::resetDwgState();
    leaderPt2 = DRW_Coord{0.0, 0.0, 0.0};
    arcStartAngle = 0.0;
    arcEndAngle = 0.0;
    arcSymbol = 0;
    isPartial = false;
    hasLeader = false;
    m_arcSubclassSeen = false;
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        DRW_Dimension::resetDwgState();
        leaderPt2 = DRW_Coord{0.0, 0.0, 0.0};
        arcStartAngle = 0.0;
        arcEndAngle = 0.0;
        arcSymbol = 0;
        isPartial = false;
        hasLeader = false;
        m_arcSubclassSeen = false;
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer probe = sourceBuf->forkIndependent();
    if (!DRW_Dimension::parseDwg(version, &probe, nullptr, bs))
        return fail();
    buf = &probe;
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    DRW_Coord parsedDefPoint;
    DRW_Coord parsedDef1;
    DRW_Coord parsedDef2;
    DRW_Coord parsedCenter;
    DRW_Coord parsedLeaderPoint1;
    DRW_Coord parsedLeaderPoint2;
    bool parsedPartial = false;
    double parsedStartAngle = 0.0;
    double parsedEndAngle = 0.0;
    bool parsedHasLeader = false;
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedDefPoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedDef2)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedCenter)
        || !readBoundedBit(*buf, bodyEndBit, parsedPartial)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedStartAngle)
        || !readBoundedBitDouble(*buf, bodyEndBit, parsedEndAngle)
        || !readBoundedBit(*buf, bodyEndBit, parsedHasLeader)
        // ODA §20.4.19: leader points are unconditional in the stream.
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedLeaderPoint1)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedLeaderPoint2))
        return fail();
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(parsedDefPoint) || !finite(parsedDef1) || !finite(parsedDef2)
        || !finite(parsedCenter) || !finite(parsedLeaderPoint1)
        || !finite(parsedLeaderPoint2) || !std::isfinite(parsedStartAngle)
        || !std::isfinite(parsedEndAngle))
        return fail();
    setDefPoint(parsedDefPoint);
    setPt3(parsedDef1);
    setPt4(parsedDef2);
    setPt5(parsedCenter);
    setPt6(parsedLeaderPoint1);
    leaderPt2 = parsedLeaderPoint2;
    isPartial = parsedPartial;
    arcStartAngle = parsedStartAngle;
    arcEndAngle = parsedEndAngle;
    hasLeader = parsedHasLeader;
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs, handleEndBit)
        || !parseDwgDimensionHandles(version, buf, handleEndBit))
        return fail();
    *sourceBuf = probe;
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimArc::encodeDwg  (oType=500 — dynamic class, classNum from writeDwgClasses)
// ----------------------------------------------------------------------------
bool DRW_DimArc::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                             dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = DRW_DimArc::kDwgClassNum;  // assigned in writeDwgClasses; reader resolves via classesmap
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getDefPoint());   // arc dim-line arc point (code 10)
    buf->put3BitDouble(getPt3());        // extension line 1 (code 13)
    buf->put3BitDouble(getPt4());        // extension line 2 (code 14)
    buf->put3BitDouble(getPt5());        // arc center (code 15)
    buf->putBit(isPartial ? 1 : 0);
    buf->putBitDouble(arcStartAngle);
    buf->putBitDouble(arcEndAngle);
    buf->putBit(hasLeader ? 1 : 0);
    // ODA §20.4.19: leader points are UNCONDITIONAL — always written; default to ext-line pts
    DRW_Coord lp1 = hasLeader ? getPt6()    : getPt3();
    DRW_Coord lp2 = hasLeader ? leaderPt2   : getPt4();
    buf->put3BitDouble(lp1);             // leader point 1 (code 16)
    buf->put3BitDouble(lp2);             // leader point 2 (code 17)
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

// ----------------------------------------------------------------------------
// DRW_DimOrdinate::encodeDwg  (oType=20)
// ----------------------------------------------------------------------------
bool DRW_DimOrdinate::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs, dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    oType = 20;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!encodeDwgDimBase(version, buf, strBuf)) return false;
    buf->put3BitDouble(getDefPoint());  // origin/definition point (code 10)
    buf->put3BitDouble(getPt3());       // feature location point (code 13)
    buf->put3BitDouble(getPt4());       // leader end point (code 14)
    // type2 byte encodes the x-vs-y ordinate flag (bit 6 / 0x40 of type, per
    // 0B.1) — keeps the DWG byte round-trip self-consistent with the parse
    // side while making the filter's `type & 64` check fire.
    std::uint8_t type2byte = (type & 0x40) ? 1 : 0;
    buf->putRawChar8(type2byte);
    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;
    putDimHandles(buf, dimStyleH, blockH, handleBuf);
    return true;
}

bool DRW_Leader::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        style = reader->getUtf8String();
        break;
    case 71:
        arrow = reader->getInt32();
        break;
    case 72:
        leadertype = reader->getInt32();
        break;
    case 73:
        flag = reader->getInt32();
        break;
    case 74:
        hookline = reader->getInt32();
        break;
    case 75:
        hookflag = reader->getInt32();
        break;
    case 76:
        vertnum = reader->getInt32();
        hasVertexCount = true;
        break;
    case 77:
        coloruse = reader->getInt32();
        break;
    case 40:
        textheight = reader->getDouble();
        break;
    case 41:
        textwidth = reader->getDouble();
        break;
    case 10:
        vertexpoint= std::make_shared<DRW_Coord>();
        vertexlist.push_back(vertexpoint);
        vertexpoint->x = reader->getDouble();
        break;
    case 20:
        if(vertexpoint)
            vertexpoint->y = reader->getDouble();
        break;
    case 30:
        if(vertexpoint)
            vertexpoint->z = reader->getDouble();
        break;
    case 340:
        annotHandle = reader->getHandleString();
        break;
    case 210:
        extrusionPoint.x = reader->getDouble();
        break;
    case 220:
        extrusionPoint.y = reader->getDouble();
        break;
    case 230:
        extrusionPoint.z = reader->getDouble();
        break;
    case 211:
        horizdir.x = reader->getDouble();
        break;
    case 221:
        horizdir.y = reader->getDouble();
        break;
    case 231:
        horizdir.z = reader->getDouble();
        break;
    case 212:
        offsetblock.x = reader->getDouble();
        break;
    case 222:
        offsetblock.y = reader->getDouble();
        break;
    case 232:
        offsetblock.z = reader->getDouble();
        break;
    case 213:
        offsettext.x = reader->getDouble();
        break;
    case 223:
        offsettext.y = reader->getDouble();
        break;
    case 233:
        offsettext.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Leader::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if ((arrow != 0 && arrow != 1) || (hookline != 0 && hookline != 1)
        || (hookflag != 0 && hookflag != 1) || leadertype < 0
        || leadertype > 1 || flag < 0 || flag > 3
        || coloruse < 0
        || coloruse > static_cast<int>(std::numeric_limits<std::uint16_t>::max())
        || vertexlist.size() > static_cast<std::size_t>(kMaxLeaderVertices)
        || !std::isfinite(textheight) || !std::isfinite(textwidth)
        || !finite(origin) || !finite(extrusionPoint) || !finite(horizdir)
        || !finite(offsetblock) || !finite(offsettext)) {
        return false;
    }
    return std::all_of(vertexlist.begin(), vertexlist.end(),
                       [&finite](const std::shared_ptr<DRW_Coord>& point) {
                           return point != nullptr && finite(*point);
                       });
}

bool DRW_Leader::validateDxf() const {
    if (!validatePayloadFields())
        return false;
    if (!hasVertexCount)
        return true;
    return vertnum >= 0
        && vertnum <= kMaxLeaderVertices
        && static_cast<std::size_t>(vertnum) == vertexlist.size();
}

void DRW_Leader::resetDwgState() {
    DRW_Entity::reset();
    style.clear();
    arrow = 1;
    leadertype = 0;
    flag = 3;
    hookline = 1;
    hookflag = 1;
    textheight = 1.0;
    textwidth = 1.0;
    vertnum = 0;
    coloruse = 7;
    annotHandle = 0;
    origin = DRW_Coord{0.0, 0.0, 0.0};
    extrusionPoint = DRW_Coord{0.0, 0.0, 1.0};
    horizdir = DRW_Coord{1.0, 0.0, 0.0};
    offsetblock = DRW_Coord{0.0, 0.0, 0.0};
    offsettext = DRW_Coord{0.0, 0.0, 0.0};
    vertexlist.clear();
    vertexpoint.reset();
    hasVertexCount = false;
    dimStyleH = dwgHandle{};
    AnnotH = dwgHandle{};
}

bool DRW_Leader::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret || !buf->isGood() || !sBuf->isGood())
        return fail();
    DRW_DBG("\n***************************** parsing leader *********************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    bool ignoredBit = false;
    std::uint16_t ignoredShort = 0;
    std::uint16_t parsedAnnotationType = 0;
    std::uint16_t parsedLeaderType = 0;
    if (!readBoundedBit(*buf, bodyEndBit, ignoredBit)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedAnnotationType)
        || !readBoundedBitShort(*buf, bodyEndBit, parsedLeaderType))
        return fail();
    if (parsedAnnotationType > 3 || parsedLeaderType > 1)
        return fail();
    DRW_DBG(" Path type "); DRW_DBG(parsedLeaderType);
    std::uint32_t parsedPointCount = 0;
    if (!readTableBodyCount(version, buf, objSize, kMaxLeaderVertices, 6,
                            parsedPointCount))
        return fail();
    const std::int32_t nPt = static_cast<std::int32_t>(parsedPointCount);
    DRW_DBG(" Num pts "); DRW_DBG(nPt);
    std::uint64_t pointBits = 0;
    if (!dwgSafety::multiply(parsedPointCount, 6, pointBits)
        || !proxyEntityHasBits(*buf, bodyEndBit, pointBits))
        return fail();

    // add vertexes
    std::vector<std::shared_ptr<DRW_Coord>> parsedVertices;
    if (!DRW::reserve(parsedVertices, nPt))
        return fail();
    for (std::uint32_t i = 0; i < parsedPointCount; i++){
        DRW_Coord vertex;
        if (!readBoundedBitCoord(*buf, bodyEndBit, vertex))
            return fail();
        parsedVertices.push_back(std::make_shared<DRW_Coord>(vertex));
        DRW_DBG("\nvertex "); DRW_DBGPT(vertex.x, vertex.y, vertex.z);
    }
    DRW_Coord parsedOrigin;
    DRW_Coord parsedExtrusion;
    DRW_Coord parsedHorizdir;
    DRW_Coord parsedOffsetblock;
    DRW_Coord parsedOffsettext{0.0, 0.0, 0.0};
    if (!readBoundedBitCoord(*buf, bodyEndBit, parsedOrigin)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedExtrusion)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedHorizdir)
        || !readBoundedBitCoord(*buf, bodyEndBit, parsedOffsetblock))
        return fail();
    DRW_DBG("\norigin "); DRW_DBGPT(parsedOrigin.x, parsedOrigin.y, parsedOrigin.z);
    // ODA §20.4.47: Extrusion is plain 3DPOINT (3BD), not BE — confirmed by libreDWG dwg.spec:3439
    if (leaderHasEndpointProjection(version)) {
        if (!readBoundedBitCoord(*buf, bodyEndBit, parsedOffsettext))
            return fail();
        DRW_DBG("\nEndptproj "); DRW_DBGPT(parsedOffsettext.x, parsedOffsettext.y, parsedOffsettext.z);
    }
    if (version < DRW::AC1015) { //R14 -
        double ignoredDouble = 0.0;
        if (!readBoundedBitDouble(*buf, bodyEndBit, ignoredDouble))
            return fail();
    }
    double parsedTextheight = 0.0;
    double parsedTextwidth = 0.0;
    bool parsedHookline = false;
    bool parsedArrow = false;
    std::uint16_t parsedColoruse = 7;
    if ((leaderHasTextBox(version)
         && (!readBoundedBitDouble(*buf, bodyEndBit, parsedTextheight)
             || !readBoundedBitDouble(*buf, bodyEndBit, parsedTextwidth)))
        || !readBoundedBit(*buf, bodyEndBit, parsedHookline)
        || !readBoundedBit(*buf, bodyEndBit, parsedArrow))
        return fail();
    DRW_DBG("\ntextheight "); DRW_DBG(parsedTextheight); DRW_DBG(" textwidth "); DRW_DBG(parsedTextwidth);
    DRW_DBG(" hookline "); DRW_DBG(parsedHookline); DRW_DBG(" arrow flag "); DRW_DBG(parsedArrow);

    // ODA §20.4.47 places arrowhead type immediately after the hook and
    // arrow flags in every DWG version.  R2000+ keeps the field even though
    // the remaining legacy arrowhead data is absent.
    if (!readBoundedBitShort(*buf, bodyEndBit, ignoredShort))
        return fail();
    if (version < DRW::AC1015) { //R14 -
        double ignoredDimSize = 0.0;
        if (!readBoundedBitDouble(*buf, bodyEndBit, ignoredDimSize)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit)
            || !readBoundedBitShort(*buf, bodyEndBit, ignoredShort)
            || !readBoundedBitShort(*buf, bodyEndBit, parsedColoruse)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit))
            return fail();
    } else { //R2000+
        if (!readBoundedBit(*buf, bodyEndBit, ignoredBit)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit))
            return fail();
    }
    DRW_DBG("\n");
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();

    dwgBuffer handleProbe = buf->forkIndependent();
    ret = DRW_Entity::parseDwgEntHandle(version, &handleProbe, true,
                                         handleEndBit);
    if (!ret || !handleProbe.isGood())
        return fail();
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
    dwgHandle parsedAnnotH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                              parsedAnnotH))
        return fail();
    dwgHandle parsedDimStyleH;
    if (!readBoundedDwgHandle(handleProbe, handleEndBit, handle, false,
                              parsedDimStyleH))
        return fail(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("annot block Handle: "); DRW_DBGHL(parsedAnnotH.code, parsedAnnotH.size, parsedAnnotH.ref); DRW_DBG("\n");
    DRW_DBG("dim style Handle: "); DRW_DBGHL(parsedDimStyleH.code, parsedDimStyleH.size, parsedDimStyleH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(handleProbe.numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!handleProbe.isGood() || !sBuf->isGood()
        || !finite(parsedOrigin) || !finite(parsedExtrusion)
        || !finite(parsedHorizdir) || !finite(parsedOffsetblock)
        || !finite(parsedOffsettext) || !std::isfinite(parsedTextheight)
        || !std::isfinite(parsedTextwidth)
        || !std::all_of(parsedVertices.begin(), parsedVertices.end(),
                         [](const std::shared_ptr<DRW_Coord>& point) {
                             return point != nullptr
                                 && std::isfinite(point->x)
                                 && std::isfinite(point->y)
                                 && std::isfinite(point->z);
                         }))
        return fail();
    *sourceBuf = handleProbe;
    flag = static_cast<int>(parsedAnnotationType);
    leadertype = parsedLeaderType;
    vertnum = nPt;
    origin = parsedOrigin;
    extrusionPoint = parsedExtrusion;
    horizdir = parsedHorizdir;
    offsetblock = parsedOffsetblock;
    offsettext = parsedOffsettext;
    textheight = parsedTextheight;
    textwidth = parsedTextwidth;
    hookline = parsedHookline;
    arrow = parsedArrow;
    coloruse = static_cast<int>(parsedColoruse);
    AnnotH = parsedAnnotH;
    annotHandle = AnnotH.ref;
    dimStyleH = parsedDimStyleH;
    vertexlist = std::move(parsedVertices);
    return true;
}

// DXF CONTEXT_DATA{} nested-block state machine (§20.4.86).  The nested blocks
// open with 300 "CONTEXT_DATA{" / 302 "LEADER{" / 304 "LEADER_LINE{" and close
// with the distinct codes 301 / 303 / 305, so the open block is tracked with a
// single state int (no stack needed).  The numeric group codes are overloaded
// by block — e.g. 40 is the overall scale in CONTEXT, the landing distance in
// LEADER and the arrow size in LEADER_LINE; 10/20/30 are the content base point,
// the connection point and a polyline vertex respectively — so they are routed
// per state into `context`.  Returns true when the code belongs to the context
// block (consumed); false at entity level so parseCode handles it.
bool DRW_MLeader::parseDxfContextCode(int code, const std::unique_ptr<dxfReader>& reader){
    try {
        switch (code) {                              // block open/close markers
        case 300:
            if (m_dxfCtxState != 0 || reader->getString() != "CONTEXT_DATA{")
                return false;
            m_dxfCtxState = 1;
            return true;
        case 301:
            if (m_dxfCtxState != 1 || reader->getString() != "}")
                return false;
            m_dxfCtxState = 0;
            return true;
        case 302:
            if (m_dxfCtxState != 1 || reader->getString() != "LEADER{"
                || context.roots.size() >= static_cast<std::size_t>(kMaxMLeaderItems))
                return false;
            context.roots.emplace_back();
            m_dxfCtxState = 2;
            return true;
        case 303:
            if (m_dxfCtxState != 2 || reader->getString() != "}")
                return false;
            m_dxfCtxState = 1;
            return true;
        case 304:
            if (m_dxfCtxState == 1) {
                context.textLabel = reader->getUtf8String();
                return true;
            }
            if (m_dxfCtxState != 2 || reader->getString() != "LEADER_LINE{"
                || context.roots.empty()
                || context.roots.back().leaderLines.size()
                       >= static_cast<std::size_t>(kMaxMLeaderItems))
                return false;
            context.roots.back().leaderLines.emplace_back();
            m_dxfCtxState = 3;
            return true;
        case 305:
            if (m_dxfCtxState != 3 || reader->getString() != "}")
                return false;
            m_dxfCtxState = 2;
            return true;
        default: break;
        }

        if (m_dxfCtxState == 0)
            return false;                            // entity level — parseCode handles it

        if (m_dxfCtxState == 3) {                    // LEADER_LINE{}: a polyline + overrides
            DRW_MLeaderRoot* root = context.roots.empty() ? nullptr : &context.roots.back();
            DRW_MLeaderLeaderLine* line =
                (root && !root->leaderLines.empty()) ? &root->leaderLines.back() : nullptr;
            if (line) switch (code) {
                case 10:
                    if (line->points.size() >= static_cast<std::size_t>(kMaxLeaderVertices))
                        return false;
                    line->points.emplace_back(reader->getDouble(), 0.0, 0.0);
                    return true;
                case 20: if (!line->points.empty()) line->points.back().y = reader->getDouble(); return true;
                case 30: if (!line->points.empty()) line->points.back().z = reader->getDouble(); return true;
                case 40: line->arrowSize = reader->getDouble(); return true;
                case 90: line->segmentIndex = reader->getInt32(); return true;
                case 91: line->leaderLineIndex = reader->getInt32(); return true;
                case 92: line->color = reader->getInt32(); return true;
                case 93: line->overrideFlags = reader->getInt32(); return true;
                case 170: line->leaderType = reader->getInt32(); return true;
                case 171: line->lineWeight = reader->getInt32(); return true;
                default: break;
            }
            return true;                             // swallow other line codes
        }

        if (m_dxfCtxState == 2) {                     // LEADER{}: one root attachment
            DRW_MLeaderRoot* root = context.roots.empty() ? nullptr : &context.roots.back();
            if (root) switch (code) {
                case 290: root->isContentValid = (reader->getInt32() != 0); return true;
                case 291: root->unknown291 = (reader->getInt32() != 0); return true;
                case 10: root->connectionPoint.x = reader->getDouble(); return true;
                case 20: root->connectionPoint.y = reader->getDouble(); return true;
                case 30: root->connectionPoint.z = reader->getDouble(); return true;
                case 11: root->direction.x = reader->getDouble(); return true;
                case 21: root->direction.y = reader->getDouble(); return true;
                case 31: root->direction.z = reader->getDouble(); return true;
                case 90: root->leaderIndex = reader->getInt32(); return true;
                case 40: root->landingDistance = reader->getDouble(); return true;
                case 271: root->attachmentDirection = reader->getInt32(); return true;
                default: break;
            }
            return true;                             // swallow other leader codes
        }

        switch (code) {                              // m_dxfCtxState == 1: CONTEXT_DATA{}
        case 40: context.overallScale = reader->getDouble(); return true;
        case 10: context.contentBasePoint.x = reader->getDouble(); return true;
        case 20: context.contentBasePoint.y = reader->getDouble(); return true;
        case 30: context.contentBasePoint.z = reader->getDouble(); return true;
        case 41: context.textHeight = reader->getDouble(); return true;
        case 140: context.arrowHeadSize = reader->getDouble(); return true;
        case 145: context.landingGap = reader->getDouble(); return true;
        case 174: context.styleLeftAttach = reader->getInt32(); return true;
        case 175: context.styleRightAttach = reader->getInt32(); return true;
        case 176: context.textAlignType = reader->getInt32(); return true;
        case 177: context.attachmentType = reader->getInt32(); return true;
        case 290: context.hasTextContents = (reader->getInt32() != 0); return true;
        /* text-content branch */
        case 11: context.textNormal.x = reader->getDouble(); return true;
        case 21: context.textNormal.y = reader->getDouble(); return true;
        case 31: context.textNormal.z = reader->getDouble(); return true;
        case 12: context.textLocation.x = reader->getDouble(); return true;
        case 22: context.textLocation.y = reader->getDouble(); return true;
        case 32: context.textLocation.z = reader->getDouble(); return true;
        case 13: context.textDirection.x = reader->getDouble(); return true;
        case 23: context.textDirection.y = reader->getDouble(); return true;
        case 33: context.textDirection.z = reader->getDouble(); return true;
        case 42: context.textRotation = reader->getDouble(); return true;
        case 43: context.boundaryWidth = reader->getDouble(); return true;
        case 44: context.boundaryHeight = reader->getDouble(); return true;
        case 45: context.lineSpacingFactor = reader->getDouble(); return true;
        case 170: context.lineSpacingStyle = reader->getInt32(); return true;
        case 90: context.textColor = reader->getInt32(); return true;
        case 171: context.alignment = reader->getInt32(); return true;
        case 172: context.flowDirection = reader->getInt32(); return true;
        case 91: context.bgFillColor = reader->getInt32(); return true;
        case 141: context.bgScaleFactor = reader->getDouble(); return true;
        case 92: context.bgTransparency = reader->getInt32(); return true;
        case 291: context.bgFillEnabled = (reader->getInt32() != 0); return true;
        case 292: context.bgMaskFillOn = (reader->getInt32() != 0); return true;
        case 173: context.columnType = reader->getInt32(); return true;
        case 293: context.textHeightAuto = (reader->getInt32() != 0); return true;
        case 142: context.columnWidth = reader->getDouble(); return true;
        case 143: context.columnGutter = reader->getDouble(); return true;
        case 294: context.columnFlowReversed = (reader->getInt32() != 0); return true;
        case 144:
            if (context.columnSizes.size() >= static_cast<std::size_t>(kMaxMLeaderItems))
                return false;
            context.columnSizes.push_back(reader->getDouble());
            return true;
        case 295: context.wordBreak = (reader->getInt32() != 0); return true;
        /* block-content branch */
        case 296: context.hasContentsBlock = (reader->getInt32() != 0); return true;
        case 14: context.blockNormal.x = reader->getDouble(); return true;
        case 24: context.blockNormal.y = reader->getDouble(); return true;
        case 34: context.blockNormal.z = reader->getDouble(); return true;
        case 15: context.blockLocation.x = reader->getDouble(); return true;
        case 25: context.blockLocation.y = reader->getDouble(); return true;
        case 35: context.blockLocation.z = reader->getDouble(); return true;
        case 16: context.blockScale.x = reader->getDouble(); return true;
        case 26: context.blockScale.y = reader->getDouble(); return true;
        case 36: context.blockScale.z = reader->getDouble(); return true;
        case 46: context.blockRotation = reader->getDouble(); return true;
        case 93: context.blockColor = reader->getInt32(); return true;
        /* common tail */
        case 110: context.basePoint.x = reader->getDouble(); return true;
        case 120: context.basePoint.y = reader->getDouble(); return true;
        case 130: context.basePoint.z = reader->getDouble(); return true;
        case 111: context.baseDirection.x = reader->getDouble(); return true;
        case 121: context.baseDirection.y = reader->getDouble(); return true;
        case 131: context.baseDirection.z = reader->getDouble(); return true;
        case 112: context.baseVertical.x = reader->getDouble(); return true;
        case 122: context.baseVertical.y = reader->getDouble(); return true;
        case 132: context.baseVertical.z = reader->getDouble(); return true;
        case 297: context.isNormalReversed = (reader->getInt32() != 0); return true;
        case 272: context.styleBottomAttach = reader->getInt32(); return true;
        case 273: context.styleTopAttach = reader->getInt32(); return true;
        default: return true;                        // swallow any other context code
        }
    } catch (...) {
        return false;
    }
}

bool DRW_MLeader::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // The embedded CONTEXT_DATA{} block (§20.4.86) is routed by the nested-block
    // state machine; the remaining (entity-level) fields are read below and
    // mirror the DWG body parser.
    if (parseDxfContextCode(code, reader))
        return true;
    // A failed nested parse must not fall through to the permissive base
    // parser: that would silently discard malformed controls or over-limit
    // collections and still publish the entity.
    if (m_dxfCtxState != 0 || (code >= 300 && code <= 305))
        return false;
    switch (code) {
    case 170: leaderType = reader->getInt32(); break;
    case 171: leaderLineWeight = reader->getInt32(); break;
    case 172: styleContentType = reader->getInt32(); break;
    case 173: styleLeftAttach = reader->getInt32(); break;
    case 95:  styleRightAttach = reader->getInt32(); break;
    case 174: styleTextAngleType = reader->getInt32(); break;
    case 175: unknown175 = reader->getInt32(); break;
    case 176: styleAttachmentType = reader->getInt32(); break;
    case 178: ipeAlign = reader->getInt32(); break;
    case 179: justification = reader->getInt32(); break;
    case 271: attachmentDirection = reader->getInt32(); break;
    case 272: styleBottomAttach = reader->getInt32(); break;
    case 273: styleTopAttach = reader->getInt32(); break;
    case 90:  overrideFlags = reader->getInt32(); break;
    case 91:  leaderColor = reader->getInt32(); break;
    case 92:  styleTextColor = reader->getInt32(); break;
    case 93:  styleBlockColor = reader->getInt32(); break;
    case 41:  landingDistance = reader->getDouble(); break;
    case 42:  defaultArrowHeadSize = reader->getDouble(); break;
    case 43:  styleBlockRotation = reader->getDouble(); break;
    case 45:  scaleFactor = reader->getDouble(); break;
    case 290: landingEnabled = (reader->getInt32() != 0); break;
    case 291: doglegEnabled = (reader->getInt32() != 0); break;
    case 292: styleTextFrameEnabled = (reader->getInt32() != 0); break;
    case 293: isAnnotative = (reader->getInt32() != 0); break;
    case 294: isTextDirectionNegative = (reader->getInt32() != 0); break;
    case 295: leaderExtendedToText = (reader->getInt32() != 0); break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

// Helper: parse one AcDbMLeaderObjectContextData::LeaderRoot entry (§20.4.86).
//
// Each root has: connection point + direction, optional break pairs, leader
// index, landing distance, then a count-and-list of leader lines.  Lines
// themselves carry: point list, break-info pairs, and (R2010+) per-line
// style overrides.  The handles inside (line-type / arrow per leader line)
// are deferred to the entity-level handle stream and not stored here.
static bool parseMLeaderRoot(DRW::Version version, dwgBuffer *buf,
                             std::uint32_t objectSize,
                             std::uint64_t bodyEndBit,
                             DRW_MLeaderRoot& root) {
    // Layout per libreDWG dwg2.spec:1316-1366 (Dwg_LEADER_Node + Dwg_LEADER_Line).
    // The two 3BD coords at the head of the node are conditional on the
    // preceding B flags; reading them unconditionally drifts the bit stream
    // when either flag is 0.
    bool hasLastPt = false;
    bool hasDogleg = false;
    if (!readBoundedBit(*buf, bodyEndBit, hasLastPt)
        || !readBoundedBit(*buf, bodyEndBit, hasDogleg))
        return false;   // 290 has_lastleaderlinepoint, 291 has_dogleg
    root.isContentValid = hasLastPt;
    root.unknown291     = hasDogleg;
    if (hasLastPt) {
        if (!readBoundedBitCoord(*buf, bodyEndBit, root.connectionPoint))
            return false;
    }
    if (hasDogleg) {
        if (!readBoundedBitCoord(*buf, bodyEndBit, root.direction))
            return false;
    }

    std::uint32_t nBreaks = 0;
    if (!readTableBodyCount(version, buf, objectSize, kMaxMLeaderItems, 12,
                            nBreaks))
        return false;
    if (!DRW::reserve(root.breaks, static_cast<int>(nBreaks)))
        return false;
    for (std::uint32_t i = 0; i < nBreaks; ++i) {
        DRW_Coord a;
        DRW_Coord b;
        if (!readBoundedBitCoord(*buf, bodyEndBit, a)
            || !readBoundedBitCoord(*buf, bodyEndBit, b))
            return false;
        root.breaks.emplace_back(a, b);
    }

    if (!readBoundedBitLong(*buf, bodyEndBit, root.leaderIndex)
        || !readBoundedBitDouble(*buf, bodyEndBit, root.landingDistance))
        return false;

    std::uint32_t nLines = 0;
    if (!readTableBodyCount(version, buf, objectSize, kMaxMLeaderItems, 6,
                            nLines))
        return false;
    if (!DRW::reserve(root.leaderLines, static_cast<int>(nLines)))
        return false;
    for (std::uint32_t i = 0; i < nLines; ++i) {
        DRW_MLeaderLeaderLine line;
        // Per libreDWG: BL num_points, points, BL num_breaks, breaks, BL line_index.
        // The previous 5-BL layout (brkInfoCount + segmentIndex + nPairs +
        // pairs + leaderLineIndex) inserted two spurious BL reads, drifting
        // every subsequent entity-level field (overallScale, contentType, …).
        std::uint32_t nPts = 0;
        if (!readTableBodyCount(version, buf, objectSize, kMaxMLeaderItems, 6,
                                nPts))
            return false;
        if (!DRW::reserve(line.points, static_cast<int>(nPts)))
            return false;
        for (std::uint32_t j = 0; j < nPts; ++j) {
            DRW_Coord point;
            if (!readBoundedBitCoord(*buf, bodyEndBit, point))
                return false;
            line.points.push_back(point);
        }
        std::uint32_t nLineBreaks = 0;
        if (!readTableBodyCount(version, buf, objectSize, kMaxMLeaderItems,
                                12, nLineBreaks))
            return false;
        for (std::uint32_t j = 0; j < nLineBreaks; ++j) {
            DRW_Coord a;
            DRW_Coord b;
            if (!readBoundedBitCoord(*buf, bodyEndBit, a)
                || !readBoundedBitCoord(*buf, bodyEndBit, b))
                return false;
            line.breaks.emplace_back(a, b);
        }
        if (!readBoundedBitLong(*buf, bodyEndBit, line.leaderLineIndex))
            return false;

        // R2010+ per-line override block.  The spec marks this block "R2010"
        // (§20.4.86 page 215); the override flags BL 93 says which fields
        // were overridden.  The handle fields (340 line-type, 341 arrow) are
        // deferred to the trailing handle stream.
        if (version >= DRW::AC1024) {
            if (!readBoundedBitShort(*buf, bodyEndBit, line.leaderType))
                return false;
            std::uint32_t parsedColor = 0;
            if (!readBoundedCmColor(*buf, nullptr, bodyEndBit, version,
                                    parsedColor))
                return false;
            line.color = static_cast<int>(parsedColor);
            // line type handle 340 — read from handles section later
            if (!readBoundedBitLong(*buf, bodyEndBit, line.lineWeight)
                || !readBoundedBitDouble(*buf, bodyEndBit, line.arrowSize)
                || !readBoundedBitLong(*buf, bodyEndBit,
                                       line.overrideFlags))
                return false;
            // arrow handle 341 — handles section
        }
        root.leaderLines.push_back(std::move(line));
    }

    if (version >= DRW::AC1024) {
        if (!readBoundedBitShort(*buf, bodyEndBit, root.attachmentDirection))
            return false;
    }

    return buf->isGood();
}

// Helper: parse the AcDbMLeaderObjectContextData (§20.4.86) payload, the
// large embedded block at the start of the MLEADER body that carries the
// leader geometry plus either text or block content.
static bool parseMLeaderAnnotContext(DRW::Version version, dwgBuffer *buf,
                                     dwgBuffer *sBuf,
                                     std::uint32_t objectSize,
                                     std::uint64_t bodyEndBit,
                                     std::uint64_t stringEndBit,
                                     DRW_MLeaderAnnotContext& ctx) {
    // NOTE: when AcDbMLeaderObjectContextData is embedded INSIDE the MLEADER
    // entity body (rather than serialized as a standalone object), the
    // AcDbObjectContextData base preamble (BS version, B has-file-ext-dict,
    // B default-flag) does NOT appear in the bit stream — those fields are
    // standalone-object metadata.  The embedded AnnotContext starts directly
    // with the leader-roots count.  Its standalone-object scale handle is
    // likewise absent from this embedded payload.

    // Number of leader roots.
    std::int32_t nRoots = 0;
    if (!readBoundedBitLong(*buf, bodyEndBit, nRoots))
        return false;
    if (nRoots == 0) {
        if (!proxyEntityHasBits(*buf, bodyEndBit, 7))
            return false;
        bool rootCountBits[7] = {};
        for (bool& rootCountBit : rootCountBits) {
            if (!readBoundedBit(*buf, bodyEndBit, rootCountBit))
                return false;
        }
        nRoots = rootCountBits[5] ? 2 : 1;
    }
    if (!isValidCount(nRoots, kMaxMLeaderItems)) return false;
    std::uint64_t minimumRootBits = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(nRoots), 10,
                             minimumRootBits)
        || !proxyEntityHasBits(*buf, bodyEndBit, minimumRootBits)) {
        return false;
    }
    ctx.roots.clear();
    if (!DRW::reserve(ctx.roots, nRoots))
        return false;
    for (std::int32_t i = 0; i < nRoots; ++i) {
        DRW_MLeaderRoot root;
        if (!parseMLeaderRoot(version, buf, objectSize, bodyEndBit, root))
            return false;
        ctx.roots.push_back(std::move(root));
    }

    // Common content fields.
    if (!readBoundedBitDouble(*buf, bodyEndBit, ctx.overallScale)
        || !readBoundedBitCoord(*buf, bodyEndBit, ctx.contentBasePoint)
        || !readBoundedBitDouble(*buf, bodyEndBit, ctx.textHeight)
        || !readBoundedBitDouble(*buf, bodyEndBit, ctx.arrowHeadSize)
        || !readBoundedBitDouble(*buf, bodyEndBit, ctx.landingGap)
        || !readBoundedBitShort(*buf, bodyEndBit, ctx.styleLeftAttach)
        || !readBoundedBitShort(*buf, bodyEndBit, ctx.styleRightAttach)
        || !readBoundedBitShort(*buf, bodyEndBit, ctx.textAlignType)
        || !readBoundedBitShort(*buf, bodyEndBit, ctx.attachmentType)
        || !readBoundedBit(*buf, bodyEndBit, ctx.hasTextContents))
        return false;

    if (ctx.hasTextContents) {
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     ctx.textLabel))
            return false;
        if (!readBoundedBitCoord(*buf, bodyEndBit, ctx.textNormal))
            return false;
        // text style handle 340 — handles section
        if (!readBoundedBitCoord(*buf, bodyEndBit, ctx.textLocation)
            || !readBoundedBitCoord(*buf, bodyEndBit, ctx.textDirection)
            || !readBoundedBitDouble(*buf, bodyEndBit, ctx.textRotation)
            || !readBoundedBitDouble(*buf, bodyEndBit, ctx.boundaryWidth)
            || !readBoundedBitDouble(*buf, bodyEndBit, ctx.boundaryHeight)
            || !readBoundedBitDouble(*buf, bodyEndBit,
                                     ctx.lineSpacingFactor)
            || !readBoundedBitShort(*buf, bodyEndBit, ctx.lineSpacingStyle))
            return false;
        std::uint32_t parsedTextColor = 0;
        if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                                parsedTextColor, nullptr, nullptr, nullptr,
                                nullptr, stringEndBit))
            return false;
        ctx.textColor = static_cast<int>(parsedTextColor);
        if (!readBoundedBitShort(*buf, bodyEndBit, ctx.alignment)
            || !readBoundedBitShort(*buf, bodyEndBit, ctx.flowDirection))
            return false;
        std::uint32_t parsedBgFillColor = 0;
        if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                                parsedBgFillColor, nullptr, nullptr, nullptr,
                                nullptr, stringEndBit))
            return false;
        ctx.bgFillColor = static_cast<int>(parsedBgFillColor);
        if (!readBoundedBitDouble(*buf, bodyEndBit, ctx.bgScaleFactor)
            || !readBoundedBitLong(*buf, bodyEndBit, ctx.bgTransparency)
            || !readBoundedBit(*buf, bodyEndBit, ctx.bgFillEnabled)
            || !readBoundedBit(*buf, bodyEndBit, ctx.bgMaskFillOn)
            || !readBoundedBitShort(*buf, bodyEndBit, ctx.columnType)
            || !readBoundedBit(*buf, bodyEndBit, ctx.textHeightAuto)
            || !readBoundedBitDouble(*buf, bodyEndBit, ctx.columnWidth)
            || !readBoundedBitDouble(*buf, bodyEndBit, ctx.columnGutter)
            || !readBoundedBit(*buf, bodyEndBit, ctx.columnFlowReversed))
            return false;
        std::uint32_t nColSizes = 0;
        if (!readTableBodyCount(version, buf, objectSize, kMaxMLeaderItems, 2,
                                nColSizes))
            return false;
        if (!DRW::reserve(ctx.columnSizes, static_cast<int>(nColSizes)))
            return false;
        for (std::uint32_t i = 0; i < nColSizes; ++i) {
            double columnSize = 0.0;
            if (!readBoundedBitDouble(*buf, bodyEndBit, columnSize))
                return false;
            ctx.columnSizes.push_back(columnSize);
        }
        bool ignoredBit = false;
        if (!readBoundedBit(*buf, bodyEndBit, ctx.wordBreak)
            || !readBoundedBit(*buf, bodyEndBit, ignoredBit))
            return false;  // unknown trailing bit
    } else {
        if (!readBoundedBit(*buf, bodyEndBit, ctx.hasContentsBlock))
            return false;
        if (ctx.hasContentsBlock) {
            // BlockTableRecord handle 341 — deferred
            if (!readBoundedBitCoord(*buf, bodyEndBit, ctx.blockNormal)
                || !readBoundedBitCoord(*buf, bodyEndBit, ctx.blockLocation)
                || !readBoundedBitCoord(*buf, bodyEndBit, ctx.blockScale)
                || !readBoundedBitDouble(*buf, bodyEndBit,
                                         ctx.blockRotation))
                return false;
            std::uint32_t parsedBlockColor = 0;
            if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version,
                                    parsedBlockColor, nullptr, nullptr, nullptr,
                                    nullptr, stringEndBit))
                return false;
            ctx.blockColor = static_cast<int>(parsedBlockColor);
            for (size_t i = 0; i < 16; ++i) {
                if (!readBoundedBitDouble(*buf, bodyEndBit,
                                          ctx.blockTransform[i]))
                    return false;
            }
        }
    }

    // Common tail.
    if (!readBoundedBitCoord(*buf, bodyEndBit, ctx.basePoint)
        || !readBoundedBitCoord(*buf, bodyEndBit, ctx.baseDirection)
        || !readBoundedBitCoord(*buf, bodyEndBit, ctx.baseVertical)
        || !readBoundedBit(*buf, bodyEndBit, ctx.isNormalReversed))
        return false;

    if (version >= DRW::AC1024) {
        if (!readBoundedBitShort(*buf, bodyEndBit, ctx.styleTopAttach)
            || !readBoundedBitShort(*buf, bodyEndBit, ctx.styleBottomAttach))
            return false;
    }

    return buf->isGood() && sBuf->isGood();
}

static bool encodeMLeaderRoot(DRW::Version version, dwgBufferW *buf,
                              const DRW_MLeaderRoot& root) {
    if (root.breaks.size() > DRW_MLeader::kMaxLeaderPoints
        || root.leaderLines.size() > DRW_MLeader::kMaxLeaderLines)
        return false;

    buf->putBit(root.isContentValid ? 1 : 0);
    buf->putBit(root.unknown291 ? 1 : 0);
    if (root.isContentValid)
        buf->put3BitDouble(root.connectionPoint);
    if (root.unknown291)
        buf->put3BitDouble(root.direction);

    buf->putBitLong(static_cast<std::int32_t>(root.breaks.size()));
    for (const auto& brk : root.breaks) {
        buf->put3BitDouble(brk.first);
        buf->put3BitDouble(brk.second);
    }

    buf->putBitLong(root.leaderIndex);
    buf->putBitDouble(root.landingDistance);

    buf->putBitLong(static_cast<std::int32_t>(root.leaderLines.size()));
    for (const DRW_MLeaderLeaderLine& line : root.leaderLines) {
        if (line.points.size() > DRW_MLeader::kMaxLeaderPoints
            || line.breaks.size() > DRW_MLeader::kMaxLeaderPoints)
            return false;
        buf->putBitLong(static_cast<std::int32_t>(line.points.size()));
        for (const DRW_Coord& point : line.points)
            buf->put3BitDouble(point);

        buf->putBitLong(static_cast<std::int32_t>(line.breaks.size()));
        for (const auto& brk : line.breaks) {
            buf->put3BitDouble(brk.first);
            buf->put3BitDouble(brk.second);
        }
        buf->putBitLong(line.leaderLineIndex);

        if (version >= DRW::AC1024) {
            buf->putBitShort(line.leaderType);
            buf->putCmColor(version, static_cast<std::uint16_t>(line.color));
            buf->putBitLong(line.lineWeight);
            buf->putBitDouble(line.arrowSize);
            buf->putBitLong(line.overrideFlags);
        }
    }

    if (version >= DRW::AC1024)
        buf->putBitShort(root.attachmentDirection);

    return true;
}

static bool encodeMLeaderAnnotContext(DRW::Version version, dwgBufferW *buf,
                                      dwgBufferW *strBuf,
                                      const DRW_MLeaderAnnotContext& ctx) {
    if (ctx.roots.size() > DRW_MLeader::kMaxRoots
        || ctx.columnSizes.size() > DRW_MLeader::kMaxLeaderPoints)
        return false;
    if (ctx.hasContentsBlock)
        return false;

    buf->putBitLong(static_cast<std::int32_t>(ctx.roots.size()));
    for (const DRW_MLeaderRoot& root : ctx.roots) {
        if (!encodeMLeaderRoot(version, buf, root))
            return false;
    }

    buf->putBitDouble(ctx.overallScale);
    buf->put3BitDouble(ctx.contentBasePoint);
    buf->putBitDouble(ctx.textHeight);
    buf->putBitDouble(ctx.arrowHeadSize);
    buf->putBitDouble(ctx.landingGap);
    buf->putBitShort(ctx.styleLeftAttach);
    buf->putBitShort(ctx.styleRightAttach);
    buf->putBitShort(ctx.textAlignType);
    buf->putBitShort(ctx.attachmentType);
    buf->putBit(ctx.hasTextContents ? 1 : 0);

    if (ctx.hasTextContents) {
        (strBuf ? strBuf : buf)->putVariableText(version, ctx.textLabel);
        buf->put3BitDouble(ctx.textNormal);
        buf->put3BitDouble(ctx.textLocation);
        buf->put3BitDouble(ctx.textDirection);
        buf->putBitDouble(ctx.textRotation);
        buf->putBitDouble(ctx.boundaryWidth);
        buf->putBitDouble(ctx.boundaryHeight);
        buf->putBitDouble(ctx.lineSpacingFactor);
        buf->putBitShort(ctx.lineSpacingStyle);
        buf->putCmColor(version, static_cast<std::uint16_t>(ctx.textColor));
        buf->putBitShort(ctx.alignment);
        buf->putBitShort(ctx.flowDirection);
        buf->putCmColor(version, static_cast<std::uint16_t>(ctx.bgFillColor));
        buf->putBitDouble(ctx.bgScaleFactor);
        buf->putBitLong(ctx.bgTransparency);
        buf->putBit(ctx.bgFillEnabled ? 1 : 0);
        buf->putBit(ctx.bgMaskFillOn ? 1 : 0);
        buf->putBitShort(ctx.columnType);
        buf->putBit(ctx.textHeightAuto ? 1 : 0);
        buf->putBitDouble(ctx.columnWidth);
        buf->putBitDouble(ctx.columnGutter);
        buf->putBit(ctx.columnFlowReversed ? 1 : 0);
        buf->putBitLong(static_cast<std::int32_t>(ctx.columnSizes.size()));
        for (double columnSize : ctx.columnSizes)
            buf->putBitDouble(columnSize);
        buf->putBit(ctx.wordBreak ? 1 : 0);
        buf->putBit(0);
    } else {
        buf->putBit(0); // hasContentsBlock
    }

    buf->put3BitDouble(ctx.basePoint);
    buf->put3BitDouble(ctx.baseDirection);
    buf->put3BitDouble(ctx.baseVertical);
    buf->putBit(ctx.isNormalReversed ? 1 : 0);

    if (version >= DRW::AC1024) {
        buf->putBitShort(ctx.styleTopAttach);
        buf->putBitShort(ctx.styleBottomAttach);
    }

    return true;
}

void DRW_MLeader::resetDwgState() {
    DRW_Entity::reset();
    context = DRW_MLeaderAnnotContext{};
    classVersion = 2;
    styleHandle = dwgHandle{};
    overrideFlags = 0;
    leaderType = 1;
    leaderColor = 0;
    leaderLineTypeHandle = dwgHandle{};
    leaderLineWeight = 0;
    landingEnabled = true;
    doglegEnabled = true;
    landingDistance = 0.0;
    arrowHeadHandle = dwgHandle{};
    defaultArrowHeadSize = 0.0;
    styleContentType = 2;
    styleTextStyleHandle = dwgHandle{};
    styleLeftAttach = 0;
    styleRightAttach = 0;
    styleTextAngleType = 0;
    unknown175 = 0;
    styleTextColor = 0;
    styleTextFrameEnabled = false;
    styleBlockHandle = dwgHandle{};
    styleBlockColor = 0;
    styleBlockScale = DRW_Coord{1.0, 1.0, 1.0};
    styleBlockRotation = 0.0;
    styleAttachmentType = 0;
    isAnnotative = false;
    arrowHeads.clear();
    blockLabels.clear();
    isTextDirectionNegative = false;
    ipeAlign = 0;
    justification = 0;
    scaleFactor = 1.0;
    attachmentDirection = 0;
    styleTopAttach = 0;
    styleBottomAttach = 0;
    leaderExtendedToText = false;
    m_dxfCtxState = 0;
}

bool DRW_MLeader::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;

    dwgBuffer sBuff = buf->forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret) return fail();
    DRW_DBG("\n***************************** parsing MLEADER ***************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }

    // R2010b+ class version (BS, default 2; <=R2004 was 1). libreDWG
    // dwg2.spec:1303-1306. Absent in R2007 streams; reading it would drift.
    if (version >= DRW::AC1024) {
        if (!readBoundedBitShort(*buf, bodyEndBit, classVersion))
            return fail();
        if (classVersion > 10) {
            DRW_DBG("\nMLEADER: implausible classVersion=");
            DRW_DBG(static_cast<int>(classVersion));
            DRW_DBG(", aborting body\n");
            return fail();
        }
    }

    // Phase 4 — embedded AcDbMLeaderObjectContextData / MLeaderAnnotContext.
    if (!parseMLeaderAnnotContext(version, buf, sBuf, objSize, bodyEndBit,
                                  stringEndBit, context)) {
        DRW_DBG("\nMLEADER: AnnotContext parse failed\n");
        return fail();
    }

    // Phase 3 — entity-level fields per §20.4.48 (after the AnnotContext).
    // Many handle slots are deferred to the trailing handle stream and not
    // stored here yet (resolution comes in Phase 7).
    if (!readBoundedBitLong(*buf, bodyEndBit, overrideFlags)
        || !readBoundedBitShort(*buf, bodyEndBit, leaderType))
        return fail();
    std::uint32_t parsedColor = 0;
    if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version, parsedColor,
                            nullptr, nullptr, nullptr, nullptr,
                            stringEndBit))
        return fail();
    leaderColor = static_cast<int>(parsedColor);
    // leader line type handle 341 — handle stream
    if (!readBoundedBitLong(*buf, bodyEndBit, leaderLineWeight)
        || !readBoundedBit(*buf, bodyEndBit, landingEnabled)
        || !readBoundedBit(*buf, bodyEndBit, doglegEnabled)
        || !readBoundedBitDouble(*buf, bodyEndBit, landingDistance))
        return fail();
    // arrow head handle 342 — handle stream
    if (!readBoundedBitDouble(*buf, bodyEndBit, defaultArrowHeadSize)
        || !readBoundedBitShort(*buf, bodyEndBit, styleContentType))
        return fail();
    // text style handle 343 — handle stream
    if (!readBoundedBitShort(*buf, bodyEndBit, styleLeftAttach)
        || !readBoundedBitShort(*buf, bodyEndBit, styleRightAttach)
        || !readBoundedBitShort(*buf, bodyEndBit, styleTextAngleType)
        || !readBoundedBitShort(*buf, bodyEndBit, unknown175))
        return fail();
    if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version, parsedColor,
                            nullptr, nullptr, nullptr, nullptr,
                            stringEndBit))
        return fail();
    styleTextColor = static_cast<int>(parsedColor);
    if (!readBoundedBit(*buf, bodyEndBit, styleTextFrameEnabled))
        return fail();
    // style block handle 344 — handle stream (optional)
    if (!readBoundedCmColor(*buf, sBuf, bodyEndBit, version, parsedColor,
                            nullptr, nullptr, nullptr, nullptr,
                            stringEndBit))
        return fail();
    styleBlockColor = static_cast<int>(parsedColor);
    if (!readBoundedBitCoord(*buf, bodyEndBit, styleBlockScale)
        || !readBoundedBitDouble(*buf, bodyEndBit, styleBlockRotation)
        || !readBoundedBitShort(*buf, bodyEndBit, styleAttachmentType)
        || !readBoundedBit(*buf, bodyEndBit, isAnnotative))
        return fail();

    // R2007 arrays (pre-R2010 only): per spec §20.4.48.  Bounds-check the
    // counts; a misaligned bit stream would produce huge nonsense values.
    if (version < DRW::AC1024) {
        std::uint32_t nArrows = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxMLeaderItems, 1,
                                nArrows))
            return fail();
        if (!DRW::reserve(arrowHeads, static_cast<int>(nArrows)))
            return fail();
        for (std::uint32_t i = 0; i < nArrows; ++i) {
            ArrowHeadEntry e;
            if (!readBoundedBit(*buf, bodyEndBit, e.isDefault))
                return fail();
            arrowHeads.push_back(e);
        }
        std::uint32_t nLabels = 0;
        if (!readTableBodyCount(version, buf, objSize, kMaxMLeaderItems, 4,
                                nLabels))
            return fail();
        if (!DRW::reserve(blockLabels, static_cast<int>(nLabels)))
            return fail();
        for (std::uint32_t i = 0; i < nLabels; ++i) {
            BlockLabelEntry e;
            if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                         e.labelText))
                return fail();
            if (!readBoundedBitShort(*buf, bodyEndBit, e.uiIndex)
                || !readBoundedBitDouble(*buf, bodyEndBit, e.width))
                return fail();
            blockLabels.push_back(std::move(e));
        }
        if (!readBoundedBit(*buf, bodyEndBit, isTextDirectionNegative)
            || !readBoundedBitShort(*buf, bodyEndBit, ipeAlign)
            || !readBoundedBitShort(*buf, bodyEndBit, justification)
            || !readBoundedBitDouble(*buf, bodyEndBit, scaleFactor))
            return fail();
    } else {  // R2010+
        if (!readBoundedBitShort(*buf, bodyEndBit, attachmentDirection)
            || !readBoundedBitShort(*buf, bodyEndBit, styleTopAttach)
            || !readBoundedBitShort(*buf, bodyEndBit, styleBottomAttach))
            return fail();
    }
    if (version >= DRW::AC1027) {  // R2013+
        if (!readBoundedBit(*buf, bodyEndBit, leaderExtendedToText))
            return fail();
    }

    if (!buf->isGood() || !sBuf->isGood()
        || currentDwgBit(buf) > bodyEndBit
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    // Common entity handles first (owner/reactors/xdic/layer/ltype/...) —
    // entity-specific handles follow in declared order from libreDWG
    // dwg2.spec.  Read order in the trailing handle stream:
    //   1. (R2010b+ only) per-leader-line ltype + arrow handles, in the
    //      same iteration order as the body block.
    //   2. AnnotContext content handle (text_style 340 if hasTextContents,
    //      else block_table 341 if hasContentsBlock).
    //   3. mleaderstyle (340), line_ltype (341), arrow_handle (342),
    //      text_style (343), block_style (344) entity-level handles.
    //   4. (R14-R2007 only) per-arrowhead + per-blocklabel handles.
    ret = DRW_Entity::parseDwgEntHandle(version, buf, true, handleEndBit);
    if (!ret)
        return fail();

    auto safeHandle = [&](dwgHandle& slot, const char* tag) {
        if (!readBoundedDwgHandle(*buf, handleEndBit, 0, false, slot))
            return false;
        DRW_DBG(" "); DRW_DBG(tag); DRW_DBG(": ");
        DRW_DBGHL(slot.code, slot.size, slot.ref); DRW_DBG("\n");
        return true;
    };

    // 1. R2010b+ per-line handles, in body iteration order.
    if (version >= DRW::AC1024) {
        for (auto& root : context.roots) {
            for (auto& line : root.leaderLines) {
                if (!safeHandle(line.lineTypeHandle, "line.ltype")) return fail();
                if (!safeHandle(line.arrowHandle,    "line.arrow")) return fail();
            }
        }
    }

    // 2. AnnotContext content handle.
    if (context.hasTextContents) {
        if (!safeHandle(context.textStyleHandle, "ctx.text_style")) return fail();
    } else if (context.hasContentsBlock) {
        if (!safeHandle(context.blockTableRecordHandle, "ctx.block_table")) return fail();
    }

    // 3. Entity-level handles.
    if (!safeHandle(styleHandle,           "mleaderstyle")) return fail();
    if (!safeHandle(leaderLineTypeHandle,  "line_ltype"))   return fail();
    if (!safeHandle(arrowHeadHandle,       "arrow_handle")) return fail();
    if (!safeHandle(styleTextStyleHandle,  "text_style"))   return fail();
    if (!safeHandle(styleBlockHandle,      "block_style"))  return fail();

    // 4. R14-R2007 per-arrowhead + per-blocklabel handles (counts came
    //    from the body-side arrays read earlier).
    if (version < DRW::AC1024) {
        for (auto& a  : arrowHeads)
            if (!safeHandle(a.handle,        "arrowheads.handle"))      return fail();
        for (auto& bl : blockLabels)
            if (!safeHandle(bl.attDefHandle, "blocklabels.attdef"))     return fail();
    }

    const int rb = buf->numRemainingBytes();
    DRW_DBG("\nMLEADER tail rb="); DRW_DBG(rb); DRW_DBG("\n");
    if (rb > 4) {
        DRW_DBG("MLEADER: handle-stream tail "); DRW_DBG(rb);
        DRW_DBG(" bytes unconsumed (handle ");
        DRW_DBGH(handle); DRW_DBG(") — review tail handle list\n");
    }

    if (!buf->isGood() || !sBuf->isGood())
        return fail();
    *sourceBuf = *buf;
    return true;
}

bool DRW_MLeader::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                            dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (version < DRW::AC1024)
        return false;

    oType = kDwgClassNum;
    if (!encodeDwgCommon(version, buf, strBuf))
        return false;

    buf->putBitShort(classVersion == 0 ? 2 : classVersion);
    if (!encodeMLeaderAnnotContext(version, buf, strBuf, context))
        return false;

    buf->putBitLong(overrideFlags);
    buf->putBitShort(leaderType);
    buf->putCmColor(version, static_cast<std::uint16_t>(leaderColor));
    buf->putBitLong(leaderLineWeight);
    buf->putBit(landingEnabled ? 1 : 0);
    buf->putBit(doglegEnabled ? 1 : 0);
    buf->putBitDouble(landingDistance);
    buf->putBitDouble(defaultArrowHeadSize);
    buf->putBitShort(styleContentType);
    buf->putBitShort(styleLeftAttach);
    buf->putBitShort(styleRightAttach);
    buf->putBitShort(styleTextAngleType);
    buf->putBitShort(unknown175);
    buf->putCmColor(version, static_cast<std::uint16_t>(styleTextColor));
    buf->putBit(styleTextFrameEnabled ? 1 : 0);
    buf->putCmColor(version, static_cast<std::uint16_t>(styleBlockColor));
    buf->put3BitDouble(styleBlockScale);
    buf->putBitDouble(styleBlockRotation);
    buf->putBitShort(styleAttachmentType);
    buf->putBit(isAnnotative ? 1 : 0);
    buf->putBitShort(attachmentDirection);
    buf->putBitShort(styleTopAttach);
    buf->putBitShort(styleBottomAttach);
    if (version >= DRW::AC1027)
        buf->putBit(leaderExtendedToText ? 1 : 0);

    if (!encodeDwgEntHandle(version, buf, handleBuf))
        return false;

    dwgBufferW *hb = handleBuf ? handleBuf : buf;
    for (const DRW_MLeaderRoot& root : context.roots) {
        for (const DRW_MLeaderLeaderLine& line : root.leaderLines) {
            putHardPointerHandle(hb, line.lineTypeHandle.ref);
            putHardPointerHandle(hb, line.arrowHandle.ref);
        }
    }

    if (context.hasTextContents) {
        putHardPointerHandle(hb, context.textStyleHandle.ref);
    } else if (context.hasContentsBlock) {
        putHardPointerHandle(hb, context.blockTableRecordHandle.ref);
    }

    putHardPointerHandle(hb, styleHandle.ref);
    putHardPointerHandle(hb, leaderLineTypeHandle.ref);
    putHardPointerHandle(hb, arrowHeadHandle.ref);
    putHardPointerHandle(hb, styleTextStyleHandle.ref);
    putHardPointerHandle(hb, styleBlockHandle.ref);

    return true;
}

bool DRW_Viewport::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        pswidth = reader->getDouble();
        break;
    case 41:
        psheight = reader->getDouble();
        break;
    case 68:
        vpstatus = reader->getInt32();
        break;
    case 69:
        vpID = reader->getInt32();
        break;
    case 12:
        centerPX = reader->getDouble();
        break;
    case 22:
        centerPY = reader->getDouble();
        break;
    case 13:
        snapPX = reader->getDouble();
        break;
    case 23:
        snapPY = reader->getDouble();
        break;
    case 14:
        snapSpPX = reader->getDouble();
        break;
    case 24:
        snapSpPY = reader->getDouble();
        break;
    case 15:
        gridSpX = reader->getDouble();
        break;
    case 25:
        gridSpY = reader->getDouble();
        break;
    case 16:
        viewDir.x = reader->getDouble();
        break;
    case 26:
        viewDir.y = reader->getDouble();
        break;
    case 36:
        viewDir.z = reader->getDouble();
        break;
    case 17:
        viewTarget.x = reader->getDouble();
        break;
    case 27:
        viewTarget.y = reader->getDouble();
        break;
    case 37:
        viewTarget.z = reader->getDouble();
        break;
    case 42:
        viewLength = reader->getDouble();
        break;
    case 43:
        frontClip = reader->getDouble();
        break;
    case 44:
        backClip = reader->getDouble();
        break;
    case 45:
        viewHeight = reader->getDouble();
        break;
    case 50:
        snapAngle = reader->getDouble();
        break;
    case 51:
        twistAngle = reader->getDouble();
        break;
    case 46:
        circleZoom = reader->getInt32();
        break;
    case 61:
        majorGridLines = reader->getInt32();
        break;
    case 63:
        ambientColor = static_cast<std::uint32_t>(reader->getInt32());
        break;
    case 72:
        circleZoom = reader->getInt32();
        break;
    case 90:
        statusFlags = reader->getInt32();
        break;
    case 1:
        styleSheet = reader->getUtf8String();
        break;
    case 281:
        renderMode = reader->getInt32();
        break;
    case 71:
        ucsPerViewport = reader->getInt32() != 0;
        break;
    case 74:
        ucsAtOrigin = reader->getInt32() != 0;
        break;
    case 110:
        ucsOrigin.x = reader->getDouble();
        break;
    case 120:
        ucsOrigin.y = reader->getDouble();
        break;
    case 130:
        ucsOrigin.z = reader->getDouble();
        break;
    case 111:
        ucsXAxis.x = reader->getDouble();
        break;
    case 121:
        ucsXAxis.y = reader->getDouble();
        break;
    case 131:
        ucsXAxis.z = reader->getDouble();
        break;
    case 112:
        ucsYAxis.x = reader->getDouble();
        break;
    case 122:
        ucsYAxis.y = reader->getDouble();
        break;
    case 132:
        ucsYAxis.z = reader->getDouble();
        break;
    case 146:
        ucsElevation = reader->getDouble();
        break;
    case 76:
        break;
    case 79:
        ucsOrthographicType = reader->getInt32();
        break;
    case 148:
        shadePlotMode = reader->getInt32();
        break;
    case 170:
        shadePlotMode = reader->getInt32();
        break;
    case 292:
        useDefaultLighting = reader->getInt32() != 0;
        break;
    case 282:
        defaultLightingType = reader->getInt32();
        break;
    case 451:
        brightness = reader->getDouble();
        break;
    case 452:
        contrast = reader->getDouble();
        break;
    case 141:
        brightness = reader->getDouble();
        break;
    case 142:
        contrast = reader->getDouble();
        break;
    case 421:
        ambientColorRgb = reader->getInt32();
        break;
    case 431:
        ambientColorName = reader->getUtf8String();
        break;
    case 331:
        frozenLayerHandles.push_back(
            static_cast<std::uint32_t>(reader->getHandleString()));
        break;
    case 332:
        backgroundHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 333:
        shadePlotHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 340:
        clipBoundaryHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 345:
        namedUcsHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 346:
        baseUcsHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 361:
        m_sunHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 348:
        visualStyleHandle = static_cast<std::uint32_t>(reader->getHandleString());
        fullVisualStyleHandle = visualStyleHandle;
        break;
    case 349:
        shadePlotHandle = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Viewport::validatePayloadFields() const {
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto fitsSignedShort = [](int value) {
        return value >= static_cast<int>(std::numeric_limits<std::int16_t>::min())
            && value <= static_cast<int>(std::numeric_limits<std::int16_t>::max());
    };
    const auto fitsByte = [](int value) {
        return value >= 0
            && value <= static_cast<int>(std::numeric_limits<std::uint8_t>::max());
    };
    const auto fitsBitShort = [](int value) {
        return value >= 0
            && value <= static_cast<int>(std::numeric_limits<std::uint16_t>::max());
    };
    const bool integralCircleZoom = std::isfinite(circleZoom)
        && std::trunc(circleZoom) == circleZoom;
    return finite(basePoint) && finite(viewTarget) && finite(viewDir)
        && finite(ucsOrigin) && finite(ucsXAxis) && finite(ucsYAxis)
        && std::isfinite(pswidth) && std::isfinite(psheight)
        && std::isfinite(centerPX) && std::isfinite(centerPY)
        && std::isfinite(snapPX) && std::isfinite(snapPY)
        && std::isfinite(snapSpPX) && std::isfinite(snapSpPY)
        && std::isfinite(gridSpX) && std::isfinite(gridSpY)
        && std::isfinite(viewLength) && std::isfinite(frontClip)
        && std::isfinite(backClip) && std::isfinite(viewHeight)
        && std::isfinite(snapAngle) && std::isfinite(twistAngle)
        && std::isfinite(ucsElevation) && std::isfinite(brightness)
        && std::isfinite(contrast) && fitsSignedShort(vpstatus)
        && fitsSignedShort(vpID) && integralCircleZoom
        && circleZoom >= 0.0
        && circleZoom <= static_cast<double>(std::numeric_limits<std::uint16_t>::max())
        && fitsBitShort(majorGridLines) && fitsByte(renderMode)
        && fitsBitShort(ucsOrthographicType) && fitsBitShort(shadePlotMode)
        && fitsByte(defaultLightingType)
        && ambientColor <= std::numeric_limits<std::uint16_t>::max()
        && ambientColorRgb >= -1 && ambientColorRgb <= 0xFFFFFF
        && frozenLayerHandles.size() <= kMaxViewportFrozenLayers;
}

bool DRW_Viewport::validateDxf() const {
    return validatePayloadFields();
}
//ex 22 dec 34
void DRW_Viewport::resetDwgState() {
    DRW_Point::resetDwgState();
    pswidth = 205.0;
    psheight = 156.0;
    vpstatus = 0;
    vpID = 0;
    centerPX = 128.5;
    centerPY = 97.5;
    snapPX = snapPY = 0.0;
    snapSpPX = snapSpPY = 10.0;
    gridSpX = gridSpY = 10.0;
    viewDir = DRW_Coord{0.0, 0.0, 1.0};
    viewTarget = DRW_Coord{0.0, 0.0, 0.0};
    viewLength = 50.0;
    frontClip = backClip = 0.0;
    viewHeight = 1.0;
    snapAngle = twistAngle = 0.0;
    circleZoom = 100.0;
    majorGridLines = 0;
    statusFlags = 0;
    styleSheet.clear();
    renderMode = 0;
    ucsAtOrigin = false;
    ucsPerViewport = false;
    ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
    ucsXAxis = DRW_Coord{1.0, 0.0, 0.0};
    ucsYAxis = DRW_Coord{0.0, 1.0, 0.0};
    ucsElevation = 0.0;
    ucsOrthographicType = 0;
    shadePlotMode = 0;
    useDefaultLighting = true;
    defaultLightingType = 1;
    brightness = contrast = 0.0;
    ambientColor = 250;
    ambientColorRgb = -1;
    ambientColorMethod = 0;
    ambientColorName.clear();
    vpHeaderHandle = 0;
    clipBoundaryHandle = 0;
    namedUcsHandle = 0;
    baseUcsHandle = 0;
    backgroundHandle = 0;
    visualStyleHandle = 0;
    shadePlotHandle = 0;
    m_sunHandle = 0;
    frozenLayerHandles.clear();
    frozenLyCount = 0;
}

bool DRW_Viewport::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    resetDwgState();
    dwgBuffer *sourceBuf = buf;
    auto fail = [this, sourceBuf]() {
        if (sourceBuf != nullptr)
            sourceBuf->invalidate();
        resetDwgState();
        return false;
    };
    if (sourceBuf == nullptr)
        return fail();

    dwgBuffer bodyProbe = sourceBuf->forkIndependent();
    buf = &bodyProbe;
    dwgBuffer sBuff = bodyProbe.forkIndependent();
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return fail();
    DRW_DBG("\n***************************** parsing viewport *****************************************\n");
    const std::uint64_t bodyEndBit = dwgDataEndBit;
    std::uint64_t stringEndBit = bodyEndBit;
    if (version > DRW::AC1021) {
        std::uint64_t ignoredBodyEndBit = 0;
        if (!entityBodyDataEndBit(*sBuf, version, objSize,
                                  ignoredBodyEndBit, &stringEndBit))
            return fail();
    }
    if (!readBoundedBitCoord(*buf, bodyEndBit, basePoint)
        || !readBoundedBitDouble(*buf, bodyEndBit, pswidth)
        || !readBoundedBitDouble(*buf, bodyEndBit, psheight))
        return fail();
    DRW_DBG("center "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    DRW_DBG("\nWidth: "); DRW_DBG(pswidth); DRW_DBG(", Height: "); DRW_DBG(psheight); DRW_DBG("\n");
    //RLZ TODO: complete in dxf
    if (version > DRW::AC1014) {//2000+
        if (!readBoundedBitCoord(*buf, bodyEndBit, viewTarget)
            || !readBoundedBitCoord(*buf, bodyEndBit, viewDir)
            || !readBoundedBitDouble(*buf, bodyEndBit, twistAngle)
            || !readBoundedBitDouble(*buf, bodyEndBit, viewHeight)
            || !readBoundedBitDouble(*buf, bodyEndBit, viewLength)
            || !readBoundedBitDouble(*buf, bodyEndBit, frontClip)
            || !readBoundedBitDouble(*buf, bodyEndBit, backClip)
            || !readBoundedBitDouble(*buf, bodyEndBit, snapAngle)
            || !readBoundedRawDouble(*buf, bodyEndBit, centerPX)
            || !readBoundedRawDouble(*buf, bodyEndBit, centerPY)
            || !readBoundedRawDouble(*buf, bodyEndBit, snapPX)
            || !readBoundedRawDouble(*buf, bodyEndBit, snapPY)
            || !readBoundedRawDouble(*buf, bodyEndBit, snapSpPX)
            || !readBoundedRawDouble(*buf, bodyEndBit, snapSpPY)
            || !readBoundedRawDouble(*buf, bodyEndBit, gridSpX)
            || !readBoundedRawDouble(*buf, bodyEndBit, gridSpY))
            return fail();
        DRW_DBG("view Target "); DRW_DBGPT(viewTarget.x, viewTarget.y, viewTarget.z);
        DRW_DBG("\nview direction "); DRW_DBGPT(viewDir.x, viewDir.y, viewDir.z);
        DRW_DBG("\nView twist Angle: "); DRW_DBG(twistAngle);
        DRW_DBG("\nview Height: "); DRW_DBG(viewHeight);
        DRW_DBG(" Lens Length: "); DRW_DBG(viewLength);
        DRW_DBG("\nfront Clip Z: "); DRW_DBG(frontClip);
        DRW_DBG(" back Clip Z: "); DRW_DBG(backClip);
        DRW_DBG("\n snap Angle: "); DRW_DBG(snapAngle);
        DRW_DBG("\nview center X: "); DRW_DBG(centerPX); DRW_DBG(", Y: "); DRW_DBG(centerPX);
        DRW_DBG("\nSnap base point X: "); DRW_DBG(snapPX); DRW_DBG(", Y: "); DRW_DBG(snapPY);
        DRW_DBG("\nSnap spacing X: "); DRW_DBG(snapSpPX); DRW_DBG(", Y: "); DRW_DBG(snapSpPY);
        std::uint16_t parsedCircleZoom = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedCircleZoom))
            return fail();
        circleZoom = parsedCircleZoom;
    }
    if (version > DRW::AC1018) {//2007+
        std::uint16_t parsedMajorGridLines = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedMajorGridLines))
            return fail();
        majorGridLines = parsedMajorGridLines;
    }
    if (version > DRW::AC1014) {//2000+
        std::int32_t parsedFrozenLyCount = 0;
        if (!readBoundedBitLong(*buf, bodyEndBit, parsedFrozenLyCount))
            return fail();
        if (!isValidCount(parsedFrozenLyCount, kMaxViewportFrozenLayers))
            return fail();
        frozenLyCount = static_cast<std::uint32_t>(parsedFrozenLyCount);
        DRW_DBG("Frozen Layer count?: "); DRW_DBG(frozenLyCount); DRW_DBG("\n");
        if (!readBoundedBitLong(*buf, bodyEndBit, statusFlags))
            return fail();
        if (!readBoundedVariableText(*sBuf, stringEndBit, version,
                                     styleSheet))
            return fail();
        std::uint8_t parsedRenderMode = 0;
        if (!readBoundedRawChar8(*buf, bodyEndBit, parsedRenderMode))
            return fail();
        renderMode = parsedRenderMode;
        // ODA order is UCS-at-origin (74), then UCS-per-viewport (71).
        if (!readBoundedBit(*buf, bodyEndBit, ucsAtOrigin)
            || !readBoundedBit(*buf, bodyEndBit, ucsPerViewport)
            || !readBoundedBitCoord(*buf, bodyEndBit, ucsOrigin)
            || !readBoundedBitCoord(*buf, bodyEndBit, ucsXAxis)
            || !readBoundedBitCoord(*buf, bodyEndBit, ucsYAxis)
            || !readBoundedBitDouble(*buf, bodyEndBit, ucsElevation))
            return fail();
        std::uint16_t parsedUcsOrthographicType = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit,
                                 parsedUcsOrthographicType))
            return fail();
        ucsOrthographicType = parsedUcsOrthographicType;
    }
    if (version > DRW::AC1015) {//2004+
        std::uint16_t parsedShadePlotMode = 0;
        if (!readBoundedBitShort(*buf, bodyEndBit, parsedShadePlotMode))
            return fail();
        shadePlotMode = parsedShadePlotMode;
    }
    if (version > DRW::AC1018) {//2007+
        std::uint8_t parsedDefaultLightingType = 0;
        if (!readBoundedBit(*buf, bodyEndBit, useDefaultLighting)
            || !readBoundedRawChar8(*buf, bodyEndBit,
                                     parsedDefaultLightingType)
            || !readBoundedBitDouble(*buf, bodyEndBit, brightness)
            || !readBoundedBitDouble(*buf, bodyEndBit, contrast))
            return fail();
        defaultLightingType = parsedDefaultLightingType;
        // ODA §20.4.38: ambient color is CMC, not ENC — confirmed by libreDWG dwg.spec:2512
        bool hasRgbColor = false;
        int parsedAmbientColorRgb = -1;
        UTF8STRING parsedAmbientColorName;
        UTF8STRING parsedAmbientColorBookName;
        dwgBuffer colorProbe = buf->forkIndependent();
        dwgBuffer colorStringProbe = sBuf->forkIndependent();
        const std::uint32_t parsedAmbientColor = colorProbe.getCmColor(
            version, &parsedAmbientColorRgb, &colorStringProbe,
            &parsedAmbientColorName, &parsedAmbientColorBookName,
            &hasRgbColor);
        if (!colorProbe.isGood() || !colorStringProbe.isGood()
            || currentDwgBit(&colorProbe) > bodyEndBit
            || currentDwgBit(&colorStringProbe) > stringEndBit)
            return fail();
        *buf = colorProbe;
        *sBuf = colorStringProbe;
        ambientColor = parsedAmbientColor;
        ambientColorRgb = parsedAmbientColorRgb;
        ambientColorName = parsedAmbientColorBookName.empty()
            ? std::move(parsedAmbientColorName)
            : (parsedAmbientColorBookName + "$"
               + parsedAmbientColorName);
        if (!hasRgbColor)
            ambientColorRgb = -1;
    }
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (!finite(basePoint) || !finite(viewTarget) || !finite(viewDir)
        || !finite(ucsOrigin) || !finite(ucsXAxis) || !finite(ucsYAxis)
        || !std::isfinite(pswidth) || !std::isfinite(psheight)
        || !std::isfinite(centerPX) || !std::isfinite(centerPY)
        || !std::isfinite(snapPX) || !std::isfinite(snapPY)
        || !std::isfinite(snapSpPX) || !std::isfinite(snapSpPY)
        || !std::isfinite(gridSpX) || !std::isfinite(gridSpY)
        || !std::isfinite(viewLength) || !std::isfinite(frontClip)
        || !std::isfinite(backClip) || !std::isfinite(viewHeight)
        || !std::isfinite(snapAngle) || !std::isfinite(twistAngle)
        || !std::isfinite(ucsElevation) || !std::isfinite(brightness)
        || !std::isfinite(contrast))
        return fail();
    if (!validatePayloadFields())
        return fail();
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*sourceBuf, version, objSize, bs,
                               handleEndBit))
        return fail();
    ret = DRW_Entity::parseDwgEntHandle(version, buf, true, handleEndBit);
    if (!ret || !buf->isGood() || !sBuf->isGood())
        return fail();

    if (!buf->isGood() || !sBuf->isGood()
        || currentDwgBit(sBuf) > stringEndBit)
        return fail();

    dwgBuffer handleProbe = buf->forkIndependent();
    auto readHandleRef = [&handleProbe, handleEndBit](std::uint32_t& target) {
        dwgHandle parsed;
        if (!readBoundedDwgHandle(handleProbe, handleEndBit, 0, false,
                                  parsed)) {
            return false;
        }
        target = parsed.ref;
        return true;
    };
    if (version < DRW::AC1015) {//R13 & R14 only
        if (!readHandleRef(vpHeaderHandle))
            return fail();
    }
    if (version > DRW::AC1014) {//2000+
        if (!DRW::reserve(frozenLayerHandles, static_cast<int>(frozenLyCount)))
            return fail();
        for (std::uint32_t i = 0; i < frozenLyCount; ++i) {
            std::uint32_t ref = 0;
            if (!readHandleRef(ref))
                return fail();
            frozenLayerHandles.push_back(ref);
        }
        if (!readHandleRef(clipBoundaryHandle))
            return fail();
        if (version == DRW::AC1015) {//2000 only
            if (!readHandleRef(vpHeaderHandle))
                return fail();
        }
        if (!readHandleRef(namedUcsHandle) || !readHandleRef(baseUcsHandle))
            return fail();
    }
    if (version > DRW::AC1018) {//2007+
        if (!readHandleRef(backgroundHandle)
            || !readHandleRef(visualStyleHandle)
            || !readHandleRef(shadePlotHandle))
            return fail();
    }
    if (version > DRW::AC1018 && !readHandleRef(m_sunHandle))
        return fail();
    if (!handleProbe.isGood() || !sBuf->isGood())
        return fail();
    *sourceBuf = handleProbe;
    return true;
}

// ---------------------------------------------------------------------------
// Write helpers shared by the new encoders below.
// ---------------------------------------------------------------------------

namespace {
// ODA handle reference codes: 3 hard owner, 4 soft pointer, 5 hard pointer.
static void putTypedHandle(dwgBufferW *hb, std::uint32_t ref,
                           std::uint8_t code) {
    dwgHandle h;
    h.code = ref == 0 ? 0 : code;
    h.ref  = ref;
    h.ref64 = ref;
    h.size = 0;
    if (h.ref != 0) {
        std::uint32_t t = h.ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    hb->putHandle(h);
}

static void putAbsHandle(dwgBufferW *hb, std::uint32_t ref) {
    putTypedHandle(hb, ref, 5);
}
} // namespace

// ---------------------------------------------------------------------------
// DRW_MLine::encodeDwg — OT=47 (AC1015/AC1018/AC1024)
// ---------------------------------------------------------------------------

bool DRW_MLine::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                           dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (!validatePayloadFields())
        return false;
    oType = 47;
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBitDouble(scale);
    buf->putRawChar8(justification);
    buf->put3BitDouble(basePoint);
    buf->putExtrusion(extPoint, false);  // false = pre-2000 style (getExtrusion(false) in parser)
    buf->putBitShort(static_cast<std::uint16_t>(openClosed));
    buf->putRawChar8(numLines);
    buf->putBitShort(numVerts);

    for (const auto& vtx : vertlist) {
        buf->put3BitDouble(vtx.position);
        buf->put3BitDouble(vtx.vertexDir);
        buf->put3BitDouble(vtx.miterDir);
        for (int li = 0; li < static_cast<int>(numLines); ++li) {
            const auto& segs  = (li < static_cast<int>(vtx.segParms.size()))
                                    ? vtx.segParms[li]      : std::vector<double>{};
            const auto& fills = (li < static_cast<int>(vtx.areaFillParms.size()))
                                    ? vtx.areaFillParms[li] : std::vector<double>{};
            buf->putBitShort(static_cast<std::uint16_t>(segs.size()));
            for (double s : segs)  buf->putBitDouble(s);
            buf->putBitShort(static_cast<std::uint16_t>(fills.size()));
            for (double f : fills) buf->putBitDouble(f);
        }
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    // MLINE style handle — extra handle after standard entity handles.
    if (version > DRW::AC1014) {
        dwgBufferW *hb = (handleBuf != nullptr) ? handleBuf : buf;
        putAbsHandle(hb, styleHandle);
    }
    return true;
}

// ---------------------------------------------------------------------------
// DRW_Vertex::encodeDwg — OT varies by flags
// ---------------------------------------------------------------------------

bool DRW_Vertex::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                             dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    switch (m_dwgSubtype) {
    case DwgSubtype::Vertex2D:     oType = 0x0A; break;
    case DwgSubtype::Vertex3D:     oType = 0x0B; break;
    case DwgSubtype::Mesh:         oType = 0x0C; break;
    case DwgSubtype::Polyface:     oType = 0x0D; break;
    case DwgSubtype::PolyfaceFace: oType = 0x0E; break;
    case DwgSubtype::Auto:
        if ((flags & 64) != 0)
            oType = 0x0D;  // VERTEX_PFACE coordinate vertex
        else if ((flags & 128) != 0)
            oType = 0x0E;  // VERTEX_PFACE_FACE
        else if ((flags & 16) != 0)
            oType = 0x0C;  // VERTEX_MESH
        else if ((flags & 32) != 0 || (flags & 8) != 0)
            oType = 0x0B;  // VERTEX_3D
        else
            oType = 0x0A;  // VERTEX_2D
        break;
    }

    if (!encodeDwgCommon(version, buf)) return false;

    if (oType == 0x0A) {
        buf->putRawChar8(static_cast<std::uint8_t>(flags));
        buf->put3BitDouble(basePoint);
        buf->putBitDouble(stawidth);
        buf->putBitDouble(endwidth);
        buf->putBitDouble(bulge);
        if (version > DRW::AC1021)
            buf->putBitLong(static_cast<std::int32_t>(identifier));
        buf->putBitDouble(tgdir);
    } else if (oType == 0x0B || oType == 0x0C || oType == 0x0D) {
        buf->putRawChar8(static_cast<std::uint8_t>(flags));
        buf->put3BitDouble(basePoint);
    } else {  // 0x0E pface face
        buf->putBitShort(static_cast<std::uint16_t>(vindex1));
        buf->putBitShort(static_cast<std::uint16_t>(vindex2));
        buf->putBitShort(static_cast<std::uint16_t>(vindex3));
        buf->putBitShort(static_cast<std::uint16_t>(vindex4));
    }

    return encodeDwgEntHandle(version, buf, handleBuf);
}

bool DRW_SeqEnd::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    DRW_Entity::reset();
    if (buf == nullptr)
        return false;

    auto fail = [this, buf]() {
        buf->invalidate();
        DRW_Entity::reset();
        return false;
    };
    dwgBuffer probe = buf->forkIndependent();
    if (!DRW_Entity::parseDwgCommon(version, &probe, bs)
        || !probe.isGood())
        return fail();
    std::uint64_t handleEndBit = 0;
    if (!dwgHandleStreamEndBit(*buf, version, objSize, bs, handleEndBit)
        || !DRW_Entity::parseDwgEntHandle(version, &probe, true,
                                           handleEndBit)
        || !probe.isGood())
        return fail();

    *buf = probe;
    return true;
}

bool DRW_SeqEnd::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                           dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    oType = 0x06;
    if (!encodeDwgCommon(version, buf))
        return false;
    return encodeDwgEntHandle(version, buf, handleBuf);
}

// ---------------------------------------------------------------------------
// DRW_Polyline::encodeDwg — OT varies by flags; vertex handles emitted here.
// ---------------------------------------------------------------------------

bool DRW_Polyline::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                               dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    // Determine object type from stored flags (mirror of parseDwg dispatch).
    if (flags & 64)       oType = 0x1D;  // POLYLINE_PFACE
    else if (flags & 16)  oType = 0x1E;  // POLYLINE_MESH
    else if (flags & 8)   oType = 0x10;  // POLYLINE_3D
    else                  oType = 0x0F;  // POLYLINE_2D

    if (!encodeDwgCommon(version, buf)) return false;

    if (oType == 0x0F) {
        buf->putBitShort(static_cast<std::uint16_t>(flags));
        buf->putBitShort(static_cast<std::uint16_t>(curvetype));
        buf->putBitDouble(defstawidth);
        buf->putBitDouble(defendwidth);
        buf->putThickness(thickness, version > DRW::AC1014);
        buf->putBitDouble(basePoint.z);
        buf->putExtrusion(extPoint, version > DRW::AC1014);
    } else if (oType == 0x10) {
        // curvetype → 2 RC flag bytes (mirror of parser decode)
        std::uint8_t rc1 = 0;
        if      (curvetype == 5) rc1 = 1;
        else if (curvetype == 6) rc1 = 2;
        else if (curvetype == 8) rc1 = 3;
        buf->putRawChar8(rc1);
        buf->putRawChar8(static_cast<std::uint8_t>(flags & 1));  // bit 0 = closed
    } else if (oType == 0x1D) {
        buf->putBitShort(static_cast<std::uint16_t>(vertexcount));
        buf->putBitShort(static_cast<std::uint16_t>(facecount));
    } else {  // 0x1E MESH
        buf->putBitShort(static_cast<std::uint16_t>(flags & ~16));  // strip reader-added bit 4
        buf->putBitShort(static_cast<std::uint16_t>(curvetype));
        buf->putBitShort(static_cast<std::uint16_t>(vertexcount));  // M count
        buf->putBitShort(static_cast<std::uint16_t>(facecount));    // N count
        buf->putBitShort(static_cast<std::uint16_t>(smoothM));      // mDensity, DXF 73
        buf->putBitShort(static_cast<std::uint16_t>(smoothN));      // nDensity, DXF 74
    }

    // AC2004+ (>AC1015): emit vertex count before the handle section.
    std::int32_t ooCount = static_cast<std::int32_t>(vertlist.size());
    if (version > DRW::AC1015)
        buf->putBitLong(ooCount);

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgBufferW *hb = (handleBuf != nullptr) ? handleBuf : buf;

    if (version < DRW::AC1018) {
        // R2000-: first/last vertex handles (absolute hard pointers).
        putAbsHandle(hb, vertlist.empty() ? 0u : vertlist.front()->handle);
        putAbsHandle(hb, vertlist.empty() ? 0u : vertlist.back()->handle);
    } else {
        // R2004+: one handle per vertex.
        for (const auto& v : vertlist)
            putAbsHandle(hb, v ? v->handle : 0u);
    }
    putAbsHandle(hb, seqEndH.ref);

    return true;
}

// ---------------------------------------------------------------------------
// DRW_Leader::encodeDwg — OT=45 (AC1015/AC1018/AC1024)
// ---------------------------------------------------------------------------

bool DRW_Leader::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                             dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs; (void)strBuf;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    oType = 45;
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBit(0);                                              // unknown bit
    buf->putBitShort(static_cast<std::uint16_t>(flag));          // annotation type
    buf->putBitShort(static_cast<std::int16_t>(leadertype));           // pathType (DXF code 72)
    buf->putBitLong(static_cast<std::int32_t>(vertexlist.size()));
    for (const auto& vp : vertexlist)
        buf->put3BitDouble(*vp);
    buf->put3BitDouble(origin);                                 // leader plane origin
    // ODA §20.4.47: Extrusion is plain 3DPOINT (3BD), not BE — matches parseDwg.
    buf->put3BitDouble(extrusionPoint);

    buf->put3BitDouble(horizdir);
    buf->put3BitDouble(offsetblock);

    if (leaderHasEndpointProjection(version))
        buf->put3BitDouble(offsettext);                         // endpoint projection

    if (version < DRW::AC1015)
        buf->putBitDouble(0.0);                                  // dimgap (pre-R2000)

    if (leaderHasTextBox(version)) {
        buf->putBitDouble(textheight);
        buf->putBitDouble(textwidth);
    }

    buf->putBit(static_cast<std::uint8_t>(hookline));
    buf->putBit(static_cast<std::uint8_t>(arrow));

    buf->putBitShort(0);        // arrowHeadType (present in every DWG version)
    if (version < DRW::AC1015) {
        buf->putBitDouble(0.0); // dimasz
        buf->putBit(0);         // unk
        buf->putBit(0);         // unk
        buf->putBitShort(0);    // unk short
        buf->putBitShort(static_cast<std::uint16_t>(coloruse)); // byBlock color
        buf->putBit(0);         // unk
        buf->putBit(0);         // unk
    } else {
        buf->putBit(0);         // unk
        buf->putBit(0);         // unk
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgBufferW *hb = (handleBuf != nullptr) ? handleBuf : buf;
    putAbsHandle(hb, 0);       // AnnotH — null (no annotation entity)
    putAbsHandle(hb, 0x15);    // dimStyleH — hard ptr to STANDARD (handle 0x15)

    return buf->isGood() && hb->isGood();
}

// ---------------------------------------------------------------------------
// DRW_Viewport::encodeDwg — OT=34 (AC1015/AC1018/AC1024)
// ---------------------------------------------------------------------------

bool DRW_Viewport::encodeDwg(DRW::Version version, dwgBufferW *buf, std::uint32_t bs,
                               dwgBufferW *strBuf, dwgBufferW *handleBuf) {
    (void)bs;
    if (buf == nullptr || !validatePayloadFields())
        return false;
    oType = 34;
    // Use strBuf for TV strings in AC1024; for AC1015/AC1018 strings go inline.
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    if (!encodeDwgCommon(version, buf)) return false;

    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(pswidth);
    buf->putBitDouble(psheight);

    if (version > DRW::AC1014) {
        buf->putBitDouble(viewTarget.x);
        buf->putBitDouble(viewTarget.y);
        buf->putBitDouble(viewTarget.z);
        buf->putBitDouble(viewDir.x);
        buf->putBitDouble(viewDir.y);
        buf->putBitDouble(viewDir.z);
        buf->putBitDouble(twistAngle);
        buf->putBitDouble(viewHeight);
        buf->putBitDouble(viewLength);    // lens length
        buf->putBitDouble(frontClip);
        buf->putBitDouble(backClip);
        buf->putBitDouble(snapAngle);
        buf->putRawDouble(centerPX);
        buf->putRawDouble(centerPY);
        buf->putRawDouble(snapPX);
        buf->putRawDouble(snapPY);
        buf->putRawDouble(snapSpPX);
        buf->putRawDouble(snapSpPY);
        buf->putRawDouble(gridSpX);
        buf->putRawDouble(gridSpY);
        buf->putBitShort(static_cast<std::uint16_t>(circleZoom));
    }

    if (version > DRW::AC1018)
        buf->putBitShort(static_cast<std::uint16_t>(majorGridLines));

    if (version > DRW::AC1014) {
        buf->putBitLong(static_cast<std::int32_t>(frozenLayerHandles.size()));
        buf->putBitLong(statusFlags);
        sb->putVariableText(version, styleSheet);
        buf->putRawChar8(static_cast<std::uint8_t>(renderMode));
        buf->putBit(ucsAtOrigin ? 1 : 0);
        buf->putBit(ucsPerViewport ? 1 : 0);
        buf->put3BitDouble(ucsOrigin);
        buf->put3BitDouble(ucsXAxis);
        buf->put3BitDouble(ucsYAxis);
        buf->putBitDouble(ucsElevation);
        buf->putBitShort(static_cast<std::uint16_t>(ucsOrthographicType));
    }

    if (version > DRW::AC1015)
        buf->putBitShort(static_cast<std::uint16_t>(shadePlotMode));

    if (version > DRW::AC1018) {
        buf->putBit(useDefaultLighting ? 1 : 0);
        buf->putRawChar8(defaultLightingType);
        buf->putBitDouble(brightness);
        buf->putBitDouble(contrast);
        if (ambientColorRgb >= 0) {
            buf->putCmColor(version, static_cast<std::uint16_t>(ambientColor),
                            ambientColorRgb, ambientColorName, "", strBuf);
        } else {
            buf->putCmColor(version, static_cast<std::uint16_t>(ambientColor));
        }
    }

    if (!encodeDwgEntHandle(version, buf, handleBuf)) return false;

    dwgBufferW *hb = (handleBuf != nullptr) ? handleBuf : buf;

    if (version < DRW::AC1015) {
        putTypedHandle(hb, vpHeaderHandle, 5);
    }
    if (version > DRW::AC1014) {
        const std::uint8_t frozenCode = version == DRW::AC1015 ? 5 : 4;
        for (const std::uint32_t ref : frozenLayerHandles)
            putTypedHandle(hb, ref, frozenCode);
        putTypedHandle(hb, clipBoundaryHandle, 4);
        if (version == DRW::AC1015)
            putTypedHandle(hb, vpHeaderHandle, 5);
        putTypedHandle(hb, namedUcsHandle, 5);
        putTypedHandle(hb, baseUcsHandle, 5);
    }
    if (version > DRW::AC1018) {
        putTypedHandle(hb, backgroundHandle, 4);
        putTypedHandle(hb, visualStyleHandle, 5);
        putTypedHandle(hb, shadePlotHandle, 4);
        putTypedHandle(hb, m_sunHandle, 3);
    }

    return buf->isGood() && sb->isGood() && hb->isGood();
}
