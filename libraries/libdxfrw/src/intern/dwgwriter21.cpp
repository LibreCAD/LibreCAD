/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 or later.                                               **
**                                                                           **
**  This library is distributed in the hope that it will be useful,          **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of           **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       **
**  General Public License for more details.                                  **
**                                                                           **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgwriter21.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "drw_reserve.h"
#include "dwgsafety.h"
#include "dwgutil.h"
#include "dwgwriterlayoutvalidation.h"

namespace {

constexpr std::uint64_t kR2007DataStart = 0x480;
constexpr std::uint64_t kR2007FileHeaderOffset = 0x80;
constexpr std::size_t kR2007FileHeaderPageSize =
    static_cast<std::size_t>(kR2007DataStart - kR2007FileHeaderOffset);
constexpr std::size_t kR2007FileHeaderCheckDataOffset = 0x3D8;
constexpr std::size_t kR2007FileHeaderCheckDataSize = 0x28;
constexpr std::uint32_t kR2007PageAlignment = 0x20;
constexpr std::uint32_t kR2007DataPageCapacity = 0x7400;
// The empty section is present in the section map but has no data pages.
// R2007 files use the file-header page-size sentinel for this descriptor.
constexpr std::uint32_t kR2007EmptySectionMaxSize = 0xf800;
constexpr std::uint32_t kR2007AppInfoHistoryPageCapacity = 0x1000;
// R2007 section-map page size from the ODA specification, section 5.3.
constexpr std::uint32_t kR2007AppInfoPageCapacity = 0x300;
constexpr std::uint32_t kR2007FileHeaderDataSize = 0x110;
constexpr std::uint32_t kR2007EncodedHeaderSize = 0x2FD;
constexpr std::uint64_t kR2007CrcSeed = 0;
constexpr std::uint64_t kR2007RandomSeed = 0;

constexpr std::array<std::string_view, 17> kR2007SectionMapOrder = {
    "AcDb:Security",
    "AcDb:FileDepList",
    "AcDb:AcDsPrototype_1b",
    "AcDb:VBAProject",
    "AcDb:AppInfoHistory",
    "AcDb:AppInfo",
    "AcDb:Preview",
    "AcDb:SummaryInfo",
    "AcDb:RevHistory",
    "AcDb:AcDbObjects",
    "AcDb:ObjFreeSpace",
    "AcDb:Template",
    "AcDb:Handles",
    "AcDb:Classes",
    "AcDb:AuxHeader",
    "AcDb:Header",
    "AcDb:Signature",
};

constexpr std::array<std::string_view, 17> kR2007StreamOrder = {
    "AcDb:SummaryInfo",
    "AcDb:Preview",
    "AcDb:VBAProject",
    "AcDb:AppInfo",
    "AcDb:AppInfoHistory",
    "AcDb:FileDepList",
    "AcDb:AcDsPrototype_1b",
    "AcDb:RevHistory",
    "AcDb:Security",
    "AcDb:AcDbObjects",
    "AcDb:ObjFreeSpace",
    "AcDb:Template",
    "AcDb:Handles",
    "AcDb:Classes",
    "AcDb:AuxHeader",
    "AcDb:Header",
    "AcDb:Signature",
};

template <std::size_t N>
std::size_t sectionOrderRank(
        std::string_view name,
        const std::array<std::string_view, N>& order) {
    const auto it = std::find(order.cbegin(), order.cend(), name);
    return it == order.cend()
        ? order.size()
        : static_cast<std::size_t>(it - order.cbegin());
}

std::uint32_t dataPageCapacityFor(const char* name) {
    // R2007 uses the normal 0x7400 page size for structural sections.  The
    // optional metadata sections have smaller documented limits.
    if (std::strcmp(name, "AcDb:AppInfoHistory") == 0)
        return kR2007AppInfoHistoryPageCapacity;
    if (std::strcmp(name, "AcDb:AppInfo") == 0)
        return kR2007AppInfoPageCapacity;
    return kR2007DataPageCapacity;
}

void putRLL(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i)
        bytes.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
}

void patchRLL(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
}

void putRLL(std::uint8_t* bytes, std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i)
        bytes[i] = static_cast<std::uint8_t>(value >> (i * 8));
}

std::uint64_t rotateR2007CheckValue(std::uint64_t value,
                                    std::uint64_t control) {
    const unsigned int shift = static_cast<unsigned int>(control & 0x1fU);
    return shift == 0 ? value : (value << shift) | (value >> (64u - shift));
}

class R2007CrcRandom {
public:
    explicit R2007CrcRandom(std::uint64_t seed) {
        m_values[0] = static_cast<std::uint32_t>(seed) * 0x343fdu + 0x269ec3u;
        m_values[1] = static_cast<std::uint32_t>(seed >> 32) * 0x343fdu
            + 0x269ec3u;
        std::uint32_t value = m_values[1];
        for (std::size_t i = 2; i < m_values.size(); ++i) {
            value = ((value >> 30) ^ value) * 0x6c078965u
                + static_cast<std::uint32_t>(i);
            m_values[i] = value;
        }
        for (std::size_t i = 0; i < m_padding.size(); ++i)
            m_padding[i] = m_values[i];
        m_index = m_padding.size();
    }

    std::uint64_t next() {
        m_index += 2;
        if (m_index >= m_values.size())
            m_index = 0;
        return static_cast<std::uint64_t>(m_values[m_index])
            | (static_cast<std::uint64_t>(m_values[m_index + 1]) << 32);
    }

    std::uint64_t encode(std::uint64_t value) {
        const std::uint64_t random = next();
        std::uint32_t lo = static_cast<std::uint32_t>(random & 0xdf7df7dfU);
        std::uint32_t hi = static_cast<std::uint32_t>(
            (random >> 32) & 0xf7df7df7U);
        if ((value & 0x200U) != 0) lo |= 0x20U;
        if ((value & 0x100U) != 0) lo |= 0x800U;
        if ((value & 0x80U) != 0) lo |= 0x20000U;
        if ((value & 0x40U) != 0) lo |= 0x800000U;
        if ((value & 0x20U) != 0) lo |= 0x20000000U;
        if ((value & 0x10U) != 0) hi |= 0x8U;
        if ((value & 0x8U) != 0) hi |= 0x200U;
        if ((value & 0x4U) != 0) hi |= 0x8000U;
        if ((value & 0x2U) != 0) hi |= 0x200000U;
        if ((value & 0x1U) != 0) hi |= 0x8000000U;
        return static_cast<std::uint64_t>(lo)
            | (static_cast<std::uint64_t>(hi) << 32);
    }

    void fillPadding(std::uint8_t* bytes, std::size_t size) const {
        for (std::size_t i = 0; i < size; ++i) {
            const std::uint32_t value = m_padding[(i / 4u) % m_padding.size()];
            bytes[i] = static_cast<std::uint8_t>(value >> ((i % 4u) * 8u));
        }
    }

private:
    std::array<std::uint32_t, 0x270> m_values{};
    std::array<std::uint32_t, 0x80> m_padding{};
    std::size_t m_index {0};
};

struct R2007HeaderProtection {
    std::uint64_t pagesMapCrcSeed {0};
    std::uint64_t sectionsMapCrcSeed {0};
    std::uint64_t crcSeedEncoded {0};
    std::uint64_t sequenceFirst {0};
    std::array<std::uint8_t, kR2007FileHeaderCheckDataSize> checkData{};
    R2007CrcRandom random {kR2007RandomSeed};
};

R2007HeaderProtection buildR2007HeaderProtection() {
    R2007HeaderProtection protection;
    protection.sectionsMapCrcSeed = protection.random.encode(kR2007CrcSeed);
    protection.pagesMapCrcSeed = protection.random.encode(kR2007CrcSeed);
    const std::uint64_t random1 = protection.random.next();
    const std::uint64_t random2 = protection.random.next();
    const std::uint64_t checkCrcSeed = protection.random.encode(kR2007CrcSeed);
    protection.crcSeedEncoded = protection.random.encode(kR2007CrcSeed);
    protection.sequenceFirst = protection.random.next();

    const std::uint64_t normal0 = rotateR2007CheckValue(random1, random2);
    const std::uint64_t normal1 = rotateR2007CheckValue(normal0, normal0);
    const std::uint64_t normal2 = rotateR2007CheckValue(random2, normal1);
    const std::uint64_t normal3 = rotateR2007CheckValue(normal2, normal2);
    const std::uint64_t normal4 = rotateR2007CheckValue(random1, normal3);
    const std::uint64_t normal5 = rotateR2007CheckValue(normal4, normal4);
    const std::uint64_t normal6 = rotateR2007CheckValue(normal5, normal5);
    const std::uint64_t normal7 = rotateR2007CheckValue(normal6, normal6);
    const std::array<std::uint64_t, 8> normalValues = {
        normal0, normal1, normal2, normal3,
        normal4, normal5, normal6, normal7};
    std::array<std::uint8_t, 64> normalBuffer{};
    for (std::size_t i = 0; i < normalValues.size(); ++i)
        putRLL(normalBuffer.data() + i * 8, normalValues[i]);
    const std::uint64_t normalCrc = dwgUtil::crc64Normal(
        ~random2, normalBuffer.data(), normalBuffer.size());

    const std::uint64_t mirrored0 = rotateR2007CheckValue(random1, random2);
    const std::uint64_t mirrored1 = rotateR2007CheckValue(normalCrc, mirrored0);
    const std::uint64_t mirrored2 = rotateR2007CheckValue(random2, mirrored1);
    const std::uint64_t mirrored3 = rotateR2007CheckValue(normalCrc, mirrored2);
    const std::uint64_t mirrored4 = rotateR2007CheckValue(random1, mirrored3);
    const std::uint64_t mirrored5 = rotateR2007CheckValue(normalCrc, mirrored4);
    const std::uint64_t mirrored6 = rotateR2007CheckValue(random2, mirrored5);
    const std::uint64_t mirrored7 = rotateR2007CheckValue(mirrored6, mirrored6);
    const std::array<std::uint64_t, 8> mirroredValues = {
        mirrored0, mirrored1, mirrored2, mirrored3,
        mirrored4, mirrored5, mirrored6, mirrored7};
    std::array<std::uint8_t, 64> mirroredBuffer{};
    for (std::size_t i = 0; i < mirroredValues.size(); ++i)
        putRLL(mirroredBuffer.data() + i * 8, mirroredValues[i]);
    const std::uint64_t mirroredCrc = dwgUtil::crc64Mirrored(
        ~random1, mirroredBuffer.data(), mirroredBuffer.size());

    putRLL(protection.checkData.data(), random1);
    putRLL(protection.checkData.data() + 8, random2);
    putRLL(protection.checkData.data() + 16, checkCrcSeed);
    putRLL(protection.checkData.data() + 24, normalCrc);
    putRLL(protection.checkData.data() + 32, mirroredCrc);
    return protection;
}

using R2007SystemPage = dwgWriterLayoutValidation::R2007SystemPage;
using Page = dwgWriterLayoutValidation::R2007Page;
using SectionPage = dwgWriterLayoutValidation::R2007SectionPage;
using Section = dwgWriterLayoutValidation::R2007Section;
using R2007PhysicalPageReceipt =
    dwgWriterLayoutValidation::R2007PhysicalPage;
using R2007LayoutReceipt = dwgWriterLayoutValidation::R2007LayoutReceipt;

void alignBytes(std::vector<std::uint8_t>& bytes) {
    const std::size_t remainder = bytes.size() % kR2007PageAlignment;
    if (remainder != 0)
        bytes.resize(bytes.size() + kR2007PageAlignment - remainder, 0);
}

bool encodeSystemPage(const std::vector<std::uint8_t>& payload,
                      R2007SystemPage& page) {
    if (payload.empty())
        return false;
    std::uint64_t alignedSize = 0;
    std::uint64_t repeatedMinimum = 0;
    std::uint64_t minimumBlocksNumerator = 0;
    if (!dwgSafety::alignUp8(payload.size(), alignedSize)
        || !dwgSafety::multiply(alignedSize, 2, repeatedMinimum)
        || !dwgSafety::add(repeatedMinimum, 238, minimumBlocksNumerator))
        return false;
    const std::uint64_t minimumBlocks = minimumBlocksNumerator / 239;
    std::uint64_t minimumPageSize = 0;
    if (minimumBlocks == 0
        || !dwgSafety::multiply(minimumBlocks, 255, minimumPageSize))
        return false;
    minimumPageSize = std::max<std::uint64_t>(minimumPageSize, 0x400);
    std::uint64_t pageSizeNumerator = 0;
    if (!dwgSafety::add(minimumPageSize, kR2007PageAlignment - 1,
                        pageSizeNumerator))
        return false;
    const std::uint64_t pageSize = pageSizeNumerator
        & ~static_cast<std::uint64_t>(kR2007PageAlignment - 1);
    const std::uint64_t maxBlockCount = pageSize / 255;
    std::uint64_t maxRepeatedDataSize = 0;
    if (maxBlockCount == 0
        || !dwgSafety::multiply(maxBlockCount, 239, maxRepeatedDataSize))
        return false;
    const std::uint64_t correctionFactor = maxRepeatedDataSize / alignedSize;
    std::uint64_t repeatedDataSize = 0;
    std::uint64_t blockCountNumerator = 0;
    if (correctionFactor < 2
        || !dwgSafety::multiply(alignedSize, correctionFactor, repeatedDataSize)
        || !dwgSafety::add(repeatedDataSize, 238, blockCountNumerator))
        return false;
    const std::uint64_t blockCount = blockCountNumerator / 239;
    std::uint64_t encodedSize = 0;
    if (blockCount == 0
        || blockCount > std::numeric_limits<std::uint32_t>::max()
        || !dwgSafety::multiply(blockCount, 255, encodedSize)
        || encodedSize > pageSize
        || alignedSize > std::numeric_limits<std::size_t>::max()
        || repeatedDataSize > std::numeric_limits<std::size_t>::max()
        || pageSize > std::numeric_limits<std::size_t>::max())
        return false;
    try {
        std::vector<std::uint8_t> padded(static_cast<std::size_t>(alignedSize), 0);
        std::copy(payload.cbegin(), payload.cend(), padded.begin());
        std::vector<std::uint8_t> repeated(static_cast<std::size_t>(repeatedDataSize));
        for (std::uint64_t offset = 0; offset < repeatedDataSize; offset += alignedSize) {
            std::copy(padded.cbegin(), padded.cend(),
                      repeated.begin() + static_cast<std::ptrdiff_t>(offset));
        }
        std::uint64_t rsInputSize = 0;
        if (!dwgSafety::multiply(blockCount, 239, rsInputSize)
            || rsInputSize > std::numeric_limits<std::size_t>::max())
            return false;
        std::vector<std::uint8_t> rsInput(static_cast<std::size_t>(rsInputSize), 0);
        std::copy(repeated.cbegin(), repeated.cend(), rsInput.begin());
        page.bytes.assign(static_cast<std::size_t>(pageSize), 0);
        if (!dwgRSCodec::encode239I(
                rsInput.data(), page.bytes.data(),
                static_cast<std::uint32_t>(blockCount)))
            return false;
        R2007CrcRandom padding(kR2007RandomSeed);
        padding.fillPadding(page.bytes.data() + encodedSize,
                            page.bytes.size() - static_cast<std::size_t>(encodedSize));
        page.compressedSize = payload.size();
        page.uncompressedSize = payload.size();
        page.correctionFactor = correctionFactor;
        page.compressedCrc = dwgUtil::crc64Mirrored(
            dwgUtil::updateSeed1(kR2007CrcSeed, page.compressedSize),
            payload.data(), payload.size());
        page.uncompressedCrc = page.compressedCrc;
        return true;
    } catch (...) {
        return false;
    }
}

bool encodeDataPage(const std::uint8_t* data, std::size_t size,
                    std::uint64_t encoding,
                    std::vector<std::uint8_t>& page) {
    if (data == nullptr || size == 0)
        return false;
    std::uint64_t alignedSize = 0;
    if (!dwgSafety::alignUp8(size, alignedSize))
        return false;
    std::uint64_t blockCountNumerator = 0;
    if (!dwgSafety::add(alignedSize, 250, blockCountNumerator))
        return false;
    const std::uint64_t blockCount = blockCountNumerator / 251;
    std::uint64_t dataSize = 0;
    std::uint64_t pageSize = 0;
    if (blockCount == 0
        || blockCount > std::numeric_limits<std::uint32_t>::max() / 255u
        || !dwgSafety::multiply(blockCount, 251, dataSize)
        || !dwgSafety::multiply(blockCount, 255, pageSize)
        || dataSize > std::numeric_limits<std::size_t>::max()
        || pageSize > std::numeric_limits<std::size_t>::max())
        return false;
    try {
        std::vector<std::uint8_t> padded(static_cast<std::size_t>(dataSize), 0);
        std::copy_n(data, size, padded.begin());
        page.resize(static_cast<std::size_t>(pageSize));
        bool encoded = false;
        if (encoding == 1) {
            encoded = dwgRSCodec::encode251(
                padded.data(), page.data(), static_cast<std::uint32_t>(blockCount));
        } else if (encoding == 4) {
            encoded = dwgRSCodec::encode251I(
                padded.data(), page.data(), static_cast<std::uint32_t>(blockCount));
        }
        if (!encoded)
            return false;
        alignBytes(page);
        return true;
    } catch (...) {
        return false;
    }
}

void appendLiteralBlock(std::vector<std::uint8_t>& compressed,
                        const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0 || size > 32)
        return;
    const auto* order = dwgCompressor::literalOrder21(
        static_cast<std::uint32_t>(size));
    if (order == nullptr)
        return;
    const std::size_t start = compressed.size();
    compressed.resize(start + size);
    for (std::size_t i = 0; i < size; ++i)
        compressed[start + order[i]] = data[i];
}

void appendLiteralRun(std::vector<std::uint8_t>& compressed,
                      const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size < 8
        || size > std::numeric_limits<std::uint32_t>::max())
        return;
    if (size <= 22) {
        compressed.push_back(static_cast<std::uint8_t>(size - 8));
    } else {
        compressed.push_back(0x0f);
        std::uint64_t remaining = size - 23;
        if (remaining < 0xffu) {
            compressed.push_back(static_cast<std::uint8_t>(remaining));
        } else {
            compressed.push_back(0xff);
            remaining -= 0xffu;
            while (remaining >= 0xffffu) {
                compressed.push_back(0xff);
                compressed.push_back(0xff);
                remaining -= 0xffffu;
            }
            compressed.push_back(static_cast<std::uint8_t>(remaining & 0xffu));
            compressed.push_back(static_cast<std::uint8_t>(remaining >> 8));
        }
    }
    while (size != 0) {
        const std::size_t blockSize = std::min<std::size_t>(size, 32);
        appendLiteralBlock(compressed, data, blockSize);
        data += blockSize;
        size -= blockSize;
    }
}

std::uint32_t findCompressionMatch(const std::uint8_t* data, std::size_t size,
                                   std::size_t position,
                                   std::array<std::int32_t, 0x8000>& hashTable,
                                   std::uint32_t& distance) {
    if (position + 3 >= size)
        return 0;

    const std::uint32_t v1 = static_cast<std::uint32_t>(data[position + 3]) << 6;
    const std::uint32_t v2 = v1 ^ data[position + 2];
    const std::uint32_t v3 = (v2 << 5) ^ data[position + 1];
    const std::uint32_t v4 = (v3 << 5) ^ data[position];
    int index = static_cast<int>((v4 + (v4 >> 5)) & 0x7fffu);
    std::int32_t previous = hashTable[static_cast<std::size_t>(index)];
    std::uint32_t candidateDistance = 0;
    if (previous >= 0)
        candidateDistance = static_cast<std::uint32_t>(position)
            - static_cast<std::uint32_t>(previous);

    if (previous >= 0 && candidateDistance <= 0x200u) {
        if (candidateDistance > 0x400u
            && data[position + 3]
                != data[static_cast<std::size_t>(previous) + 3]) {
            index = (index & 0x7ff) ^ 0x401f;
            previous = hashTable[static_cast<std::size_t>(index)];
            if (previous >= 0)
                candidateDistance = static_cast<std::uint32_t>(position)
                    - static_cast<std::uint32_t>(previous);
            if (previous < 0 || candidateDistance > 0x200u
                || (candidateDistance > 0x400u
                    && data[position + 3]
                        != data[static_cast<std::size_t>(previous) + 3])) {
                hashTable[static_cast<std::size_t>(index)] =
                    static_cast<std::int32_t>(position);
                return 0;
            }
        }

        const auto previousPosition = static_cast<std::size_t>(previous);
        if (data[position] == data[previousPosition]
            && data[position + 1] == data[previousPosition + 1]
            && data[position + 2] == data[previousPosition + 2]) {
            std::size_t candidate = position + 3;
            std::size_t source = previousPosition + 3;
            std::uint32_t length = 3;
            while (candidate < size && length < 14
                   && data[candidate] == data[source]) {
                ++candidate;
                ++source;
                ++length;
            }
            hashTable[static_cast<std::size_t>(index)] =
                static_cast<std::int32_t>(position);
            distance = candidateDistance;
            return length;
        }
    }

    hashTable[static_cast<std::size_t>(index)] =
        static_cast<std::int32_t>(position);
    return 0;
}

void appendCompressionMatch(std::vector<std::uint8_t>& compressed,
                            std::uint32_t distance, std::uint32_t length,
                            std::uint32_t trailingLiterals) {
    const std::uint32_t value = distance - 1u;
    const std::uint32_t first = (length << 4) | (value & 0x0fu);
    const std::uint32_t second = ((value >> 1) & 0xf8u)
        | (trailingLiterals & 7u);
    compressed.push_back(static_cast<std::uint8_t>(first));
    compressed.push_back(static_cast<std::uint8_t>(second));
}

std::vector<std::uint8_t> compressDataPage(const std::uint8_t* data,
                                            std::size_t size) {
    if (data == nullptr || size <= 0x18u
        || size > std::numeric_limits<std::uint32_t>::max())
        return {};

    std::array<std::int32_t, 0x8000> hashTable;
    hashTable.fill(-1);
    try {
        std::vector<std::uint8_t> compressed;
        compressed.reserve(size);

        struct Match {
            std::size_t position;
            std::uint32_t length;
            std::uint32_t distance;
        };
        std::vector<Match> matches;
        std::size_t position = 0;
        while (position + 3 < size) {
            std::uint32_t distance = 0;
            const std::uint32_t length = findCompressionMatch(
                data, size, position, hashTable, distance);
            if (length < 3 || position < 8) {
                ++position;
                continue;
            }
            matches.push_back({position, length, distance});
            position += length;
        }

        if (matches.empty())
            return {};
        appendLiteralRun(compressed, data, matches.front().position);
        for (std::size_t i = 0; i < matches.size(); ++i) {
            const std::size_t matchEnd = matches[i].position + matches[i].length;
            const std::size_t nextPosition = i + 1 < matches.size()
                ? matches[i + 1].position : size;
            const std::size_t literals = nextPosition - matchEnd;
            const std::uint32_t trailing = literals < 8
                ? static_cast<std::uint32_t>(literals) : 0;
            appendCompressionMatch(compressed, matches[i].distance,
                                   matches[i].length, trailing);
            if (literals != 0) {
                if (literals < 8)
                    appendLiteralBlock(compressed, data + matchEnd, literals);
                else
                    appendLiteralRun(compressed, data + matchEnd, literals);
            }
        }

        if (compressed.size() >= size)
            return {};

        // The LZ stream has several compact literal/match forms.  Validate the
        // generated stream with the reader codec before putting it on disk; a
        // codec edge case must degrade to a stored page, never to a corrupt DWG.
        std::vector<std::uint8_t> roundTrip(size, 0);
        dwgCompressor validator;
        if (!validator.decompress21(compressed.data(), roundTrip.data(),
                                    compressed.size(), size)
            || !std::equal(roundTrip.cbegin(), roundTrip.cend(), data))
            return {};
        return compressed;
    } catch (...) {
        return {};
    }
}

// R2007 section descriptors carry the fixed hash codes assigned by the DWG
// format.  Opaque sections have no public hash metadata, so retain zero for
// those sections rather than inventing a value that cannot be validated.
std::uint64_t sectionHashCode(std::string_view name) {
    if (name == "AcDb:Security")
        return 0x4a0204eau;
    if (name == "AcDb:FileDepList")
        return 0x6c4205cau;
    if (name == "AcDb:VBAProject")
        return 0x586e0544u;
    if (name == "AcDb:AppInfo")
        return 0x3fa0043eu;
    if (name == "AcDb:Preview")
        return 0x40aa0473u;
    if (name == "AcDb:SummaryInfo")
        return 0x717a060fu;
    if (name == "AcDb:RevHistory")
        return 0x60a205b3u;
    if (name == "AcDb:AcDbObjects")
        return 0x674c05a9u;
    if (name == "AcDb:ObjFreeSpace")
        return 0x77e2061fu;
    if (name == "AcDb:Template")
        return 0x4a1404ceu;
    if (name == "AcDb:Handles")
        return 0x3f6e0450u;
    if (name == "AcDb:Classes")
        return 0x3f54045fu;
    if (name == "AcDb:AuxHeader")
        return 0x54f0050au;
    if (name == "AcDb:Header")
        return 0x32b803d9u;
    return 0;
}

template <std::size_t N>
void sortSections(std::vector<Section>& sections,
                  const std::array<std::string_view, N>& order) {
    std::stable_sort(sections.begin(), sections.end(),
                     [&order](const Section& lhs, const Section& rhs) {
                         return sectionOrderRank(lhs.name, order)
                             < sectionOrderRank(rhs.name, order);
                     });
}

bool reorderPhysicalPagesForStream(
        const std::vector<Section>& streamSections,
        std::vector<Page>& pages) {
    std::unordered_map<std::uint32_t, Page> pagesById;
    pagesById.reserve(pages.size());
    for (Page& page : pages) {
        if (!pagesById.emplace(page.id, std::move(page)).second)
            return false;
    }

    std::vector<Page> ordered;
    ordered.reserve(pagesById.size());
    for (const Section& section : streamSections) {
        for (const SectionPage& sectionPage : section.pages) {
            const auto it = pagesById.find(sectionPage.pageId);
            if (it == pagesById.end())
                return false;
            ordered.push_back(std::move(it->second));
            pagesById.erase(it);
        }
    }
    if (!pagesById.empty())
        return false;
    pages = std::move(ordered);
    return true;
}

void appendUtf16Name(std::vector<std::uint8_t>& bytes,
                     const std::string& name) {
    for (unsigned char c : name) {
        bytes.push_back(c);
        bytes.push_back(0);
    }
}

std::vector<std::uint8_t> buildSectionMap(const std::vector<Section>& sections) {
    std::vector<std::uint8_t> bytes;
    try {
        for (const Section& section : sections) {
            std::uint64_t nameByteLength = 0;
            if (!dwgSafety::multiply(section.name.size(), 2,
                                     nameByteLength))
                return {};
            putRLL(bytes, section.size);
            putRLL(bytes, section.maxSize);
            putRLL(bytes, section.encrypted);
            putRLL(bytes, section.hashCode);
            putRLL(bytes, nameByteLength);
            putRLL(bytes, 0); // unknown
            putRLL(bytes, section.encoding);
            putRLL(bytes, section.pages.size());
            appendUtf16Name(bytes, section.name);
            for (const SectionPage& page : section.pages) {
                putRLL(bytes, page.startOffset);
                putRLL(bytes, page.pageSize);
                putRLL(bytes, page.pageId);
                putRLL(bytes, page.uncompressedSize);
                putRLL(bytes, page.compressedSize);
                putRLL(bytes, page.checksum);
                putRLL(bytes, page.crc);
            }
        }

        // SectionsAmount is the number of real descriptors plus this required
        // empty-section descriptor. It is also the map terminator.
        putRLL(bytes, 0); // data size
        putRLL(bytes, kR2007EmptySectionMaxSize);
        putRLL(bytes, 0); // encryption
        putRLL(bytes, 0); // hash code
        putRLL(bytes, 0); // section-name length
        putRLL(bytes, 0); // unknown
        putRLL(bytes, 4); // R2007 section encoding
        putRLL(bytes, 0); // no data pages
    } catch (...) {
        return {};
    }
    return bytes;
}

std::vector<std::uint8_t> buildPageMap(const std::vector<Page>& pages) {
    std::vector<std::uint8_t> bytes;
    std::uint64_t byteCount = 0;
    if (!dwgSafety::multiply(pages.size(), 16, byteCount)
        || byteCount > std::numeric_limits<std::size_t>::max())
        return {};
    try {
        bytes.reserve(static_cast<std::size_t>(byteCount));
        for (const Page& page : pages) {
            putRLL(bytes, page.bytes.size());
            putRLL(bytes, page.id);
        }
    } catch (...) {
        return {};
    }
    return bytes;
}

std::vector<std::uint8_t> emptyRevHistory() {
    std::vector<std::uint8_t> bytes(16, 0);
    bytes[8] = 1; // one empty history entry
    return bytes;
}

std::vector<std::uint8_t> emptyObjFreeSpace() {
    std::vector<std::uint8_t> bytes(53, 0);
    bytes[20] = 4; // numnums
    patchRLL(bytes, 21, 0x32);
    patchRLL(bytes, 29, 0x64);
    patchRLL(bytes, 37, 0x200);
    patchRLL(bytes, 45, 0xffffffffu);
    return bytes;
}

std::vector<std::uint8_t> emptyAuxHeader() {
    std::vector<std::uint8_t> bytes(123, 0);
    bytes[0] = 0xff;
    bytes[1] = 0x77;
    bytes[2] = 0x01;
    bytes[3] = 0x21; // AC1021
    bytes[5] = 0xff;
    return bytes;
}

std::vector<std::uint8_t> buildFileHeader(std::uint64_t fileSize,
                                          std::uint64_t pageMap2Offset,
                                          std::uint32_t pageMap2Id,
                                          std::uint32_t pageMapId,
                                          const R2007SystemPage& pageMap,
                                          std::uint64_t pageCount,
                                          std::uint64_t maxPageId,
                                          std::uint32_t sectionMap2Id,
                                          std::uint32_t sectionMapId,
                                          const R2007SystemPage& sectionMap,
                                          std::uint64_t sectionCount,
                                          const R2007HeaderProtection& protection) {
    std::vector<std::uint8_t> header(kR2007FileHeaderDataSize, 0);
    patchRLL(header, 0, kR2007FileHeaderDataSize);
    patchRLL(header, 8, fileSize);
    patchRLL(header, 16, pageMap.compressedCrc);
    patchRLL(header, 24, pageMap.correctionFactor);
    patchRLL(header, 32, protection.pagesMapCrcSeed);
    patchRLL(header, 40, pageMap2Offset);
    patchRLL(header, 48, pageMap2Id);
    patchRLL(header, 56, 0); // page map 1 is at the data-section start
    patchRLL(header, 64, pageMapId);
    patchRLL(header, 72, 0); // no second header
    patchRLL(header, 80, pageMap.compressedSize);
    patchRLL(header, 88, pageMap.uncompressedSize);
    patchRLL(header, 96, pageCount);
    patchRLL(header, 104, maxPageId);
    patchRLL(header, 112, 0x20);
    patchRLL(header, 120, 0x40);
    patchRLL(header, 128, pageMap.uncompressedCrc);
    patchRLL(header, 160, sectionCount + 1u);
    patchRLL(header, 168, sectionMap.uncompressedCrc);
    patchRLL(header, 176, sectionMap.compressedSize);
    patchRLL(header, 184, sectionMap2Id);
    patchRLL(header, 192, sectionMapId);
    patchRLL(header, 200, sectionMap.uncompressedSize);
    patchRLL(header, 208, sectionMap.compressedCrc);
    patchRLL(header, 216, sectionMap.correctionFactor);
    patchRLL(header, 224, protection.sectionsMapCrcSeed);
    patchRLL(header, 232, 0x60100);
    patchRLL(header, 240, kR2007CrcSeed);
    patchRLL(header, 248, protection.crcSeedEncoded);
    patchRLL(header, 256, kR2007RandomSeed);

    const auto crc = dwgUtil::crc64Normal(
        dwgUtil::updateSeed2(0, header.size()), header.data(), header.size());
    patchRLL(header, 264, crc);
    return header;
}

std::vector<std::uint8_t> buildEncodedFileHeader(
        std::uint64_t fileSize, std::uint64_t pageMap2Offset,
        std::uint32_t pageMap2Id, std::uint32_t pageMapId,
        const R2007SystemPage& pageMap, std::uint64_t pageCount,
        std::uint64_t maxPageId,
        std::uint32_t sectionMap2Id,
        std::uint32_t sectionMapId, const R2007SystemPage& sectionMap,
        std::uint64_t sectionCount,
        const R2007HeaderProtection& protection) {
    const auto data = buildFileHeader(
        fileSize, pageMap2Offset, pageMap2Id, pageMapId, pageMap,
        pageCount, maxPageId, sectionMap2Id, sectionMapId, sectionMap,
        sectionCount, protection);
    std::vector<std::uint8_t> decoded(3u * 239u, 0);
    const std::uint64_t sequenceSecond = rotateR2007CheckValue(
        protection.sequenceFirst, protection.sequenceFirst);
    std::array<std::uint8_t, 16> sequence{};
    putRLL(sequence.data(), protection.sequenceFirst);
    putRLL(sequence.data() + 8, sequenceSecond);
    patchRLL(decoded, 0, dwgUtil::crc64Normal(
        dwgUtil::updateSeed1(kR2007CrcSeed, sequence.size()),
        sequence.data(), sequence.size()));
    patchRLL(decoded, 8, protection.sequenceFirst);
    patchRLL(decoded, 16, dwgUtil::crc64Normal(
        dwgUtil::updateSeed1(kR2007CrcSeed, data.size()),
        data.data(), data.size()));
    const std::uint32_t dataSize = static_cast<std::uint32_t>(
        -static_cast<std::int32_t>(data.size()));
    for (unsigned int i = 0; i < 4; ++i)
        decoded[24 + i] = static_cast<std::uint8_t>(dataSize >> (i * 8));
    std::copy(data.cbegin(), data.cend(), decoded.begin() + 32);
    protection.random.fillPadding(decoded.data() + 32 + data.size(),
                                  decoded.size() - 32 - data.size());
    std::vector<std::uint8_t> encoded(kR2007EncodedHeaderSize, 0);
    if (!dwgRSCodec::encode239I(decoded.data(), encoded.data(), 3))
        return {};
    return encoded;
}

std::vector<std::uint8_t> buildFileHeaderPage(
        std::uint64_t fileSize, std::uint64_t pageMap2Offset,
        std::uint32_t pageMap2Id, std::uint32_t pageMapId,
        const R2007SystemPage& pageMap, std::uint64_t pageCount,
        std::uint64_t maxPageId,
        std::uint32_t sectionMap2Id, std::uint32_t sectionMapId,
        const R2007SystemPage& sectionMap, std::uint64_t sectionCount,
        const R2007HeaderProtection& protection) {
    const auto encoded = buildEncodedFileHeader(
        fileSize, pageMap2Offset, pageMap2Id, pageMapId, pageMap,
        pageCount, maxPageId, sectionMap2Id, sectionMapId, sectionMap,
        sectionCount, protection);
    if (encoded.size() != kR2007EncodedHeaderSize)
        return {};

    std::vector<std::uint8_t> page(kR2007FileHeaderPageSize, 0);
    std::copy(encoded.cbegin(), encoded.cend(), page.begin());
    protection.random.fillPadding(
        page.data() + kR2007EncodedHeaderSize,
        kR2007FileHeaderCheckDataOffset - kR2007EncodedHeaderSize);
    std::copy(protection.checkData.cbegin(), protection.checkData.cend(),
              page.begin() + static_cast<std::ptrdiff_t>(
                  kR2007FileHeaderCheckDataOffset));
    return page;
}

} // namespace

void dwgWriter21::finishObject() {
    // AC1021 keeps the R2007 data, string, and handle streams inside one
    // object frame. The object-size RL marks the end of the data/string
    // stream; handles follow it and there is no AC1024 UMC prefix.
    if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                    [this](const auto& entry) {
                        return entry.first == m_currentHandle;
                    })) {
        m_writeError = true;
        return;
    }
    if (!m_buf.isGood() || !m_objectBody.isGood() || !m_objectStrings.isGood()
        || !m_objectHandles.isGood()) {
        m_writeError = true;
        return;
    }
    m_objectBody.alignToByte();
    if (m_objectStrings.data().size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectBody.size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectHandles.size() > std::numeric_limits<std::uint32_t>::max()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const std::uint64_t mergedStringBaseBit = m_objectBody.bitCount();
    if (!appendR2007StringStream(m_objectBody, m_objectStrings, false)) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }

    m_objectBody.alignToByte();
    m_objectHandles.alignToByte();
    const std::uint32_t dataBytes =
        static_cast<std::uint32_t>(m_objectBody.size());
    const std::uint32_t handleBytes =
        static_cast<std::uint32_t>(m_objectHandles.size());
    const std::uint64_t totalBytes64 =
        static_cast<std::uint64_t>(dataBytes) + handleBytes;
    if (dataBytes > std::numeric_limits<std::uint32_t>::max() / 8u
        || totalBytes64 > std::numeric_limits<std::uint32_t>::max()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }

    std::uint32_t bitCount = dataBytes * 8u;
    if (!m_objectBody.data().empty()) {
        const std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
        const std::size_t rlBitOffset =
            (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        m_objectBody.patchRawLong32AtBit(rlBitOffset, bitCount);
    }

    const std::uint32_t frameStart =
        static_cast<std::uint32_t>(m_buf.size());
    m_buf.putModularShort(static_cast<std::int32_t>(totalBytes64));
    if (!m_buf.isGood()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const std::size_t bodyStart = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), dataBytes);
    if (handleBytes != 0)
        m_buf.putBytes(m_objectHandles.data().data(), handleBytes);
    const std::uint16_t crc = m_buf.crc16(
        0xC0C1, frameStart, bodyStart + static_cast<std::size_t>(totalBytes64));
    m_buf.putRawShort16(crc);
    if (!captureLastDwgObjectHandleOccurrences(mergedStringBaseBit, 0, true,
                                               false)) {
        m_buf.truncate(frameStart);
        m_writeError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    m_objectMap.emplace_back(m_currentHandle, frameStart);
    markDwgClassInstanceEmitted(m_currentHandle);
    m_currentHandle = 0;
    m_objectStrings.reset();
    m_objectHandles.reset();
}

bool dwgWriter21::finalize() {
    if (m_writeError || m_stream == nullptr || !m_stream->good())
        return false;

    const auto& raw = m_buf.data();
    const auto headerOffset = m_sectionOffsets.find(recno::HEADER);
    const auto headerSize = m_sectionSizes.find(recno::HEADER);
    const auto classesOffset = m_sectionOffsets.find(recno::CLASSES);
    const auto classesSize = m_sectionSizes.find(recno::CLASSES);
    const auto handlesOffset = m_sectionOffsets.find(recno::HANDLES);
    const auto handlesSize = m_sectionSizes.find(recno::HANDLES);
    if (headerOffset == m_sectionOffsets.end()
        || headerSize == m_sectionSizes.end()
        || classesOffset == m_sectionOffsets.end()
        || classesSize == m_sectionSizes.end()
        || handlesOffset == m_sectionOffsets.end()
        || handlesSize == m_sectionSizes.end())
        return false;

    const auto sectionRangeIsValid = [&raw](std::uint32_t offset,
                                             std::uint32_t size) {
        return static_cast<std::uint64_t>(offset) <= raw.size()
            && static_cast<std::uint64_t>(size)
                   <= raw.size() - static_cast<std::uint64_t>(offset);
    };
    if (!sectionRangeIsValid(headerOffset->second, headerSize->second)
        || !sectionRangeIsValid(classesOffset->second, classesSize->second)
        || !sectionRangeIsValid(handlesOffset->second, handlesSize->second))
        return false;

    std::uint64_t objectsOffset = 0;
    if (!dwgSafety::add(classesOffset->second, classesSize->second,
                        objectsOffset)
        || objectsOffset > handlesOffset->second
        || objectsOffset > raw.size())
        return false;
    const std::uint64_t objectsSize64 =
        static_cast<std::uint64_t>(handlesOffset->second) - objectsOffset;
    if (objectsSize64 > std::numeric_limits<std::uint32_t>::max())
        return false;
    const auto objectsSize = static_cast<std::uint32_t>(objectsSize64);

    std::vector<Section> sections;
    std::vector<Page> dataPages;
    std::uint32_t nextPageId = 3;

    auto appendSection = [&](const char* name, const std::uint8_t* data,
                             std::uint32_t size, std::uint64_t encoding,
                             std::uint64_t encrypted,
                             std::uint64_t maxSizeOverride) {
        if (name == nullptr || *name == '\0')
            return false;
        if ((encoding != 1 && encoding != 4) || encrypted != 0)
            return false;
        const std::string_view sectionName{name};
        if (std::any_of(sections.cbegin(), sections.cend(),
                        [sectionName](const Section& section) {
                            return section.name == sectionName;
                        }))
            return false;
        Section section;
        section.name = name;
        section.size = size;
        section.encoding = encoding;
        section.encrypted = encrypted;
        section.hashCode = sectionHashCode(sectionName);
        section.maxSize = maxSizeOverride != 0
            ? maxSizeOverride : dataPageCapacityFor(name);
        if (section.maxSize == 0
            || section.maxSize > dwgSafety::MaxBufferSize
            || section.maxSize > dwgSafety::MaxPageCap)
            return false;
        if (size == 0) {
            sections.push_back(std::move(section));
            return true;
        }
        if (data == nullptr)
            return false;
        std::uint64_t pageCountNumerator = 0;
        if (!dwgSafety::add(static_cast<std::uint64_t>(size),
                            section.maxSize - 1, pageCountNumerator))
            return false;
        const std::uint64_t pageCount = pageCountNumerator / section.maxSize;
        for (std::uint64_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
            std::uint64_t start = 0;
            if (!dwgSafety::multiply(pageIndex, section.maxSize, start))
                return false;
            const std::size_t chunk = static_cast<std::size_t>(std::min<std::uint64_t>(
                section.maxSize, static_cast<std::uint64_t>(size) - start));
            const auto compressed = compressDataPage(data + start, chunk);
            const bool isCompressed = !compressed.empty();
            const std::uint8_t* pageData = isCompressed
                ? compressed.data() : data + start;
            const std::size_t pageDataSize = isCompressed
                ? compressed.size() : chunk;
            Page physical;
            if (nextPageId == std::numeric_limits<std::uint32_t>::max())
                return false;
            physical.id = nextPageId++;
            // Stored pages still use the R2007 RS(255,251) physical envelope;
            // only the section-map cSize/uSize equality distinguishes them
            // from LZ-compressed pages.
            if (!encodeDataPage(pageData, pageDataSize, section.encoding,
                                physical.bytes))
                return false;
            const std::uint64_t pageCrc = dwgUtil::crc64Mirrored(
                dwgUtil::updateSeed1(0, pageDataSize), pageData, pageDataSize);
            const std::uint64_t pageChecksum = dwgUtil::checksum21(
                0, data + start, chunk);
            section.pages.push_back({start, section.maxSize, chunk,
                                     pageDataSize, physical.id, pageChecksum,
                                     pageCrc});
            dataPages.push_back(std::move(physical));
        }
        sections.push_back(std::move(section));
        return true;
    };

    const auto appInfo = buildAppInfoContent(DRW::AC1021);
    const auto revHistory = emptyRevHistory();
    const auto objFreeSpace = emptyObjFreeSpace();
    const auto auxHeader = emptyAuxHeader();
    const std::array<std::uint8_t, 1> appInfoHistory{0};
    const std::array<std::uint8_t, 6> templateData{1, 0, 0, 0, 0, 0};
    if (!appendSection("AcDb:Header", raw.data() + headerOffset->second,
                       headerSize->second, 4, 0, 0)
        || !appendSection("AcDb:Classes", raw.data() + classesOffset->second,
                          classesSize->second, 4, 0, 0)
        || !appendSection("AcDb:AcDbObjects", raw.data() + objectsOffset,
                          objectsSize, 4, 0, 0))
        return false;

    for (const DRW_RawDwgSection& section : m_rawDwgSections) {
        if (section.m_data.size() > std::numeric_limits<std::uint32_t>::max())
            return false;
        // R2004 readers preserve their compression marker (2) in the shared
        // raw-section carrier. R2007+ section maps instead require the
        // interleaved Reed-Solomon encoding marker (4).
        const std::uint64_t encoding = section.m_encoding == 2
            ? 4 : section.m_encoding;
        if (!appendSection(section.m_name.c_str(),
                           section.m_data.empty() ? nullptr : section.m_data.data(),
                           static_cast<std::uint32_t>(section.m_data.size()),
                           encoding, section.m_encrypted,
                           section.m_maxSize))
            return false;
    }

    if (!appendSection("AcDb:AppInfo", appInfo.data(),
                          static_cast<std::uint32_t>(appInfo.size()), 4, 0, 0)
        || !appendSection("AcDb:AppInfoHistory", appInfoHistory.data(),
                          static_cast<std::uint32_t>(appInfoHistory.size()), 4, 0, 0)
        || !appendSection("AcDb:AuxHeader", auxHeader.data(),
                          static_cast<std::uint32_t>(auxHeader.size()), 4, 0, 0)
        || !appendSection("AcDb:RevHistory", revHistory.data(),
                          static_cast<std::uint32_t>(revHistory.size()), 4, 0, 0)
        || !appendSection("AcDb:ObjFreeSpace", objFreeSpace.data(),
                          static_cast<std::uint32_t>(objFreeSpace.size()), 4, 0, 0)
        || !appendSection("AcDb:Template", templateData.data(),
                          static_cast<std::uint32_t>(templateData.size()), 4, 0, 0)
        || !appendSection("AcDb:Handles", raw.data() + handlesOffset->second,
                          handlesSize->second, 4, 0, 0))
        return false;

    // The section map and the physical data stream have different normative
    // orders. Keep page IDs stable while publishing descriptors in map order
    // and placing their physical pages in stream order.
    std::vector<Section> streamSections = sections;
    sortSections(sections, kR2007SectionMapOrder);
    sortSections(streamSections, kR2007StreamOrder);
    if (!reorderPhysicalPagesForStream(streamSections, dataPages))
        return false;

    const auto sectionMapData = buildSectionMap(sections);
    if (sectionMapData.empty())
        return false;
    R2007SystemPage sectionMapPage1;
    R2007SystemPage sectionMapPage2;
    if (!encodeSystemPage(sectionMapData, sectionMapPage1)
        || !encodeSystemPage(sectionMapData, sectionMapPage2))
        return false;
    if (nextPageId > std::numeric_limits<std::uint32_t>::max() - 2u)
        return false;
    const std::uint32_t sectionMapId = nextPageId++;
    const std::uint32_t sectionMap2Id = nextPageId++;
    dataPages.push_back({sectionMapId, std::move(sectionMapPage1.bytes)});
    dataPages.push_back({sectionMap2Id, std::move(sectionMapPage2.bytes)});

    std::vector<Page> pages;
    pages.reserve(dataPages.size() + 2);
    std::vector<std::uint8_t> pageMapData;
    // Page-map page sizes are independent of their contents once the full
    // page count is known, so build the two identical map pages in one pass.
    const std::size_t totalPageCount = dataPages.size() + 2;
    std::uint64_t pageMapDataSize = 0;
    if (!dwgSafety::multiply(static_cast<std::uint64_t>(totalPageCount), 16,
                             pageMapDataSize)
        || pageMapDataSize > std::numeric_limits<std::size_t>::max())
        return false;
    pageMapData.resize(static_cast<std::size_t>(pageMapDataSize), 0);
    R2007SystemPage pageMapPage1;
    R2007SystemPage pageMapPage2;
    if (!encodeSystemPage(pageMapData, pageMapPage1)
        || !encodeSystemPage(pageMapData, pageMapPage2))
        return false;
    const std::uint32_t pageMap1Id = 1;
    const std::uint32_t pageMap2Id = 2;
    pages.push_back({pageMap1Id, std::move(pageMapPage1.bytes)});
    pages.push_back({pageMap2Id, std::move(pageMapPage2.bytes)});
    for (Page& page : dataPages)
        pages.push_back(std::move(page));

    // Fill the page maps after all physical sizes and IDs are known, then
    // rebuild both encoded copies with their actual contents.
    pageMapData = buildPageMap(pages);
    if (!encodeSystemPage(pageMapData, pageMapPage1)
        || !encodeSystemPage(pageMapData, pageMapPage2))
        return false;
    pages[0].bytes = std::move(pageMapPage1.bytes);
    pages[1].bytes = std::move(pageMapPage2.bytes);
    const std::uint64_t pageMap2Offset = pages[0].bytes.size();

    std::uint64_t fileSize = kR2007DataStart;
    for (const Page& page : pages) {
        if (!dwgSafety::add(fileSize, page.bytes.size(), fileSize))
            return false;
    }
    if (!dwgSafety::add(fileSize, kR2007FileHeaderPageSize, fileSize))
        return false;
    const R2007HeaderProtection protection = buildR2007HeaderProtection();
    const auto fileHeaderPage = buildFileHeaderPage(
        fileSize, pageMap2Offset, pageMap2Id, pageMap1Id, pageMapPage1,
        pages.size(), pages.back().id, sectionMap2Id, sectionMapId,
        sectionMapPage1, sections.size(), protection);
    if (fileHeaderPage.size() != kR2007FileHeaderPageSize)
        return false;
    const auto fileHeaderData = buildFileHeader(
        fileSize, pageMap2Offset, pageMap2Id, pageMap1Id, pageMapPage1,
        pages.size(), pages.back().id, sectionMap2Id, sectionMapId,
        sectionMapPage1, sections.size(), protection);
    if (fileHeaderData.size() != kR2007FileHeaderDataSize)
        return false;

    std::vector<std::uint8_t> metadata(kR2007FileHeaderOffset, 0);
    std::memcpy(metadata.data(), "AC1021", 6);
    metadata[0x0C] = 3;
    metadata[0x11] = 0x21;
    metadata[0x12] = 0xFF;
    const std::uint16_t codePage = fileCodePageId();
    metadata[0x13] = static_cast<std::uint8_t>(codePage);
    metadata[0x14] = static_cast<std::uint8_t>(codePage >> 8);
    R2007LayoutReceipt receipt;
    receipt.version = m_version;
    receipt.metadataSize = metadata.size();
    receipt.fileHeaderPageSize = fileHeaderPage.size();
    receipt.dataStart = kR2007DataStart;
    receipt.leadingHeaderAddress = receipt.metadataSize;
    receipt.pageMap2Offset = pageMap2Offset;
    receipt.pageMap1Id = pageMap1Id;
    receipt.pageMap2Id = pageMap2Id;
    receipt.sectionMapId = sectionMapId;
    receipt.sectionMap2Id = sectionMap2Id;
    receipt.pageCount = pages.size();
    receipt.maxPageId = pages.back().id;
    receipt.sectionCount = sections.size() + 1u;
    receipt.declaredFileSize = fileSize;
    receipt.actualFileSize = kR2007DataStart;
    if (pages.size() > static_cast<std::size_t>(
                           std::numeric_limits<int>::max())
        || !DRW::reserve(receipt.pages, static_cast<int>(pages.size())))
        return false;
    std::unordered_map<std::uint32_t, SectionPage> pageMetadata;
    pageMetadata.reserve(dataPages.size());
    for (const Section& section : streamSections) {
        for (const SectionPage& sectionPage : section.pages) {
            if (!pageMetadata.emplace(sectionPage.pageId, sectionPage).second)
                return false;
        }
    }
    std::uint64_t receiptAddress = receipt.dataStart;
    for (const Page& page : pages) {
        R2007PhysicalPageReceipt::Kind kind =
            R2007PhysicalPageReceipt::Kind::Data;
        std::string sectionName;
        std::uint64_t logicalOffset = 0;
        std::uint64_t encodedSize = 0;
        if (page.id == pageMap1Id || page.id == pageMap2Id) {
            kind = R2007PhysicalPageReceipt::Kind::PageMap;
        } else if (page.id == sectionMapId || page.id == sectionMap2Id) {
            kind = R2007PhysicalPageReceipt::Kind::SectionMap;
        } else {
            const auto metadataIt = pageMetadata.find(page.id);
            if (metadataIt == pageMetadata.end())
                return false;
            const auto sectionIt = std::find_if(
                streamSections.cbegin(), streamSections.cend(),
                [&metadataIt](const Section& section) {
                    return std::any_of(
                        section.pages.cbegin(), section.pages.cend(),
                        [&metadataIt](const SectionPage& sectionPage) {
                            return sectionPage.pageId == metadataIt->first;
                        });
                });
            if (sectionIt == streamSections.cend())
                return false;
            sectionName = sectionIt->name;
            logicalOffset = metadataIt->second.startOffset;
            encodedSize = metadataIt->second.compressedSize;
        }
        receipt.pages.push_back({page.id, receiptAddress, page.bytes.size(),
                                 kind, sectionName, logicalOffset, encodedSize});
        if (!dwgSafety::add(receiptAddress, page.bytes.size(), receiptAddress))
            return false;
    }
    receipt.trailingHeaderAddress = receiptAddress;
    if (!dwgSafety::add(receiptAddress, receipt.fileHeaderPageSize,
                        receipt.actualFileSize))
        return false;
    if (receipt.actualFileSize > std::numeric_limits<std::size_t>::max())
        return false;
    std::vector<std::uint8_t> assembled;
    try {
        assembled.reserve(static_cast<std::size_t>(receipt.actualFileSize));
        assembled.insert(assembled.end(), metadata.cbegin(), metadata.cend());
        assembled.insert(assembled.end(), fileHeaderPage.cbegin(),
                         fileHeaderPage.cend());
        for (const Page& page : pages)
            assembled.insert(assembled.end(), page.bytes.cbegin(),
                             page.bytes.cend());
        assembled.insert(assembled.end(), fileHeaderPage.cbegin(),
                         fileHeaderPage.cend());
    } catch (...) {
        return false;
    }
    if (assembled.size() != receipt.actualFileSize)
        return false;
    dwgWriterLayoutValidation::R2007Snapshot validationSnapshot;
    validationSnapshot.receipt = std::move(receipt);
    validationSnapshot.sections = std::move(sections);
    validationSnapshot.pages = std::move(pages);
    validationSnapshot.pageMapData = std::move(pageMapData);
    validationSnapshot.sectionMapData = std::move(sectionMapData);
    validationSnapshot.pageMap = std::move(pageMapPage1);
    validationSnapshot.sectionMap = std::move(sectionMapPage1);
    validationSnapshot.metadata = std::move(metadata);
    validationSnapshot.fileHeaderPage = std::move(fileHeaderPage);
    validationSnapshot.fileHeaderData = std::move(fileHeaderData);
    validationSnapshot.assembled = std::move(assembled);
    dwgWriterLayoutValidation::Snapshot validationVariant{
        std::move(validationSnapshot)};
#ifdef DWG_LAYOUT_VALIDATION_TESTS
    if (!dwgWriterLayoutValidation::reserveTestMutationCapacity(
            validationVariant))
        return false;
    dwgWriterLayoutValidation::notifyTestSnapshot(validationVariant);
#endif
    const auto* validated = std::get_if<
        dwgWriterLayoutValidation::R2007Snapshot>(&validationVariant);
    if (validated == nullptr
        || !dwgWriterLayoutValidation::validate(*validated))
        return false;
    m_stream->write(reinterpret_cast<const char*>(validated->assembled.data()),
                    static_cast<std::streamsize>(validated->assembled.size()));
    return m_stream->good();
}
