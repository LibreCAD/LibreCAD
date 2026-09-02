/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgwriterlayoutvalidation.h"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "dwgsafety.h"
#include "dwgutil.h"

namespace dwgWriterLayoutValidation {
namespace {

constexpr std::uint64_t kR2004PageAlignment = 0x20;
constexpr std::size_t kR2004FileHeaderSize = 0x100;
constexpr std::uint64_t kR2007FileHeaderOffset = 0x80;
constexpr std::size_t kR2007FileHeaderPageSize = 0x400;
constexpr std::size_t kR2007FileHeaderDataSize = 0x110;
constexpr std::size_t kR2007EncodedHeaderSize = 0x2FD;
constexpr std::uint64_t kR2007PageAlignment = 0x20;
constexpr std::uint64_t kR2007CrcSeed = 0;
constexpr std::uint32_t kR2007EmptySectionMaxSize = 0xf800;

ValidationResult fail(ValidationCategory category) {
    return {category};
}

ValidationResult ok() {
    return {};
}

bool checkedSpan(const std::vector<std::uint8_t>& bytes,
                std::uint64_t offset, std::uint64_t size,
                const std::uint8_t*& span) {
    if (offset > bytes.size() || size > bytes.size() - offset)
        return false;
    span = bytes.data() + static_cast<std::size_t>(offset);
    return true;
}

std::uint32_t getRL(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

void putRL(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16));
    bytes.push_back(static_cast<std::uint8_t>(value >> 24));
}

void putRLL(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    putRL(bytes, static_cast<std::uint32_t>(value));
    putRL(bytes, static_cast<std::uint32_t>(value >> 32));
}

void appendUtf16Name(std::vector<std::uint8_t>& bytes,
                     const std::string& name) {
    for (const unsigned char character : name) {
        bytes.push_back(character);
        bytes.push_back(0);
    }
}

std::vector<std::uint8_t> buildR2007PageMap(
        const std::vector<R2007Page>& pages) {
    std::vector<std::uint8_t> bytes;
    std::uint64_t byteCount = 0;
    if (!dwgSafety::multiply(pages.size(), 16, byteCount)
        || byteCount > std::numeric_limits<std::size_t>::max())
        return {};
    try {
        bytes.reserve(static_cast<std::size_t>(byteCount));
        for (const R2007Page& page : pages) {
            putRLL(bytes, page.bytes.size());
            putRLL(bytes, page.id);
        }
    } catch (...) {
        return {};
    }
    return bytes;
}

std::vector<std::uint8_t> buildR2007SectionMap(
        const std::vector<R2007Section>& sections) {
    std::vector<std::uint8_t> bytes;
    try {
        for (const R2007Section& section : sections) {
            std::uint64_t nameByteLength = 0;
            if (!dwgSafety::multiply(section.name.size(), 2,
                                     nameByteLength))
                return {};
            putRLL(bytes, section.size);
            putRLL(bytes, section.maxSize);
            putRLL(bytes, section.encrypted);
            putRLL(bytes, section.hashCode);
            putRLL(bytes, nameByteLength);
            putRLL(bytes, 0);
            putRLL(bytes, section.encoding);
            putRLL(bytes, section.pages.size());
            appendUtf16Name(bytes, section.name);
            for (const R2007SectionPage& page : section.pages) {
                putRLL(bytes, page.startOffset);
                putRLL(bytes, page.pageSize);
                putRLL(bytes, page.pageId);
                putRLL(bytes, page.uncompressedSize);
                putRLL(bytes, page.compressedSize);
                putRLL(bytes, page.checksum);
                putRLL(bytes, page.crc);
            }
        }

        // R2007 terminates the map with one unnamed empty descriptor.
        putRLL(bytes, 0);
        putRLL(bytes, kR2007EmptySectionMaxSize);
        putRLL(bytes, 0);
        putRLL(bytes, 0);
        putRLL(bytes, 0);
        putRLL(bytes, 0);
        putRLL(bytes, 4);
        putRLL(bytes, 0);
    } catch (...) {
        return {};
    }
    return bytes;
}

class ByteCursor {
public:
    explicit ByteCursor(const std::vector<std::uint8_t>& bytes) noexcept
        : m_bytes(bytes) {}

    bool readU32(std::uint32_t& value) noexcept {
        const std::uint8_t* data = nullptr;
        if (!readBytes(sizeof(value), data))
            return false;
        value = getRL(data);
        return true;
    }

    bool readU64(std::uint64_t& value) noexcept {
        std::uint32_t low = 0;
        std::uint32_t high = 0;
        if (!readU32(low) || !readU32(high))
            return false;
        value = static_cast<std::uint64_t>(low)
            | (static_cast<std::uint64_t>(high) << 32);
        return true;
    }

    bool readBytes(std::size_t size, const std::uint8_t*& data) noexcept {
        if (size > m_bytes.size() - m_offset)
            return false;
        data = m_bytes.data() + m_offset;
        m_offset += size;
        return true;
    }

    bool atEnd() const noexcept {
        return m_offset == m_bytes.size();
    }

private:
    const std::vector<std::uint8_t>& m_bytes;
    std::size_t m_offset {0};
};

bool matchesR2004Name(ByteCursor& cursor, const std::string& expected) {
    if (expected.size() >= 64)
        return false;
    const std::uint8_t* bytes = nullptr;
    if (!cursor.readBytes(64, bytes))
        return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (bytes[index]
            != static_cast<std::uint8_t>(expected[index]))
            return false;
    }
    for (std::size_t index = expected.size(); index < 64; ++index) {
        if (bytes[index] != 0)
            return false;
    }
    return true;
}

bool matchesR2007Name(ByteCursor& cursor, std::uint64_t byteLength,
                      const std::string& expected) {
    std::uint64_t expectedByteLength = 0;
    if (!dwgSafety::multiply(expected.size(), 2, expectedByteLength)
        || byteLength != expectedByteLength
        || byteLength > std::numeric_limits<std::size_t>::max())
        return false;
    const std::uint8_t* bytes = nullptr;
    if (!cursor.readBytes(static_cast<std::size_t>(byteLength), bytes))
        return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (bytes[index * 2]
                != static_cast<std::uint8_t>(expected[index])
            || bytes[index * 2 + 1] != 0)
            return false;
    }
    return true;
}

ValidationResult validateR2004SectionPageMap(
        const R2004Snapshot& snapshot) {
    ByteCursor cursor(snapshot.sectionPageMapData);
    std::unordered_set<std::uint32_t> pageIds;
    pageIds.reserve(snapshot.receipt.pages.size());
    for (const R2004PhysicalPage& page : snapshot.receipt.pages) {
        std::uint32_t pageId = 0;
        std::uint32_t pageSize = 0;
        if (!cursor.readU32(pageId) || !cursor.readU32(pageSize))
            return fail(ValidationCategory::Map);
        if (!pageIds.insert(pageId).second)
            return fail(ValidationCategory::Duplicate);
        if (pageId != page.pageId || pageSize != page.size)
            return fail(ValidationCategory::Map);
    }
    return cursor.atEnd() ? ok() : fail(ValidationCategory::Map);
}

ValidationResult validateR2004DataSectionMap(
        const R2004Snapshot& snapshot) {
    ByteCursor cursor(snapshot.dataSectionMapData);
    std::unordered_map<std::uint32_t, const R2004PhysicalPage*> dataPages;
    dataPages.reserve(snapshot.dataPages.size());
    for (const R2004PhysicalPage& page : snapshot.receipt.pages) {
        if (page.kind != R2004PhysicalPage::Kind::Data)
            continue;
        if (page.pageId == 0
            || !dataPages.emplace(page.pageId, &page).second)
            return fail(ValidationCategory::Topology);
    }
    if (dataPages.size() != snapshot.dataPages.size())
        return fail(ValidationCategory::Topology);

    std::uint32_t sectionCount = 0;
    std::uint32_t compression = 0;
    std::uint32_t maxPageSize = 0;
    std::uint32_t unknown = 0;
    std::uint32_t repeatedSectionCount = 0;
    if (!cursor.readU32(sectionCount) || !cursor.readU32(compression)
        || !cursor.readU32(maxPageSize) || !cursor.readU32(unknown)
        || !cursor.readU32(repeatedSectionCount)
        || sectionCount != snapshot.sections.size()
        || compression != 2 || maxPageSize != 0x7400 || unknown != 0
        || repeatedSectionCount != sectionCount)
        return fail(ValidationCategory::Map);

    std::unordered_set<std::uint32_t> sectionIds;
    std::unordered_set<std::uint32_t> referencedPageIds;
    sectionIds.reserve(snapshot.sections.size());
    referencedPageIds.reserve(dataPages.size());
    for (const R2004Section& section : snapshot.sections) {
        std::uint64_t dataSize = 0;
        std::uint32_t pageCount = 0;
        std::uint32_t descriptorMaxPageSize = 0;
        std::uint32_t descriptorUnknown = 0;
        std::uint32_t compressionType = 0;
        std::uint32_t sectionId = 0;
        std::uint32_t encrypted = 0;
        if (!cursor.readU64(dataSize) || !cursor.readU32(pageCount)
            || !cursor.readU32(descriptorMaxPageSize)
            || !cursor.readU32(descriptorUnknown)
            || !cursor.readU32(compressionType)
            || !cursor.readU32(sectionId) || !cursor.readU32(encrypted)
            || !matchesR2004Name(cursor, section.name))
            return fail(ValidationCategory::Map);
        if (!sectionIds.insert(sectionId).second)
            return fail(ValidationCategory::Duplicate);
        if (dataSize != section.dataSize || pageCount != section.pages.size()
            || descriptorMaxPageSize != section.maxPageSize
            || descriptorUnknown != 0
            || compressionType != section.compressionType
            || sectionId != section.sectionId || encrypted != section.encrypted)
            return fail(ValidationCategory::Map);

        for (const R2004Section::Page& sectionPage : section.pages) {
            std::uint32_t pageId = 0;
            std::uint32_t dataPageSize = 0;
            std::uint64_t startOffset = 0;
            if (!cursor.readU32(pageId) || !cursor.readU32(dataPageSize)
                || !cursor.readU64(startOffset))
                return fail(ValidationCategory::Map);
            const auto pageIt = dataPages.find(pageId);
            if (pageId == 0 || pageIt == dataPages.end())
                return fail(ValidationCategory::Map);
            if (!referencedPageIds.insert(pageId).second)
                return fail(ValidationCategory::Duplicate);
            const R2004PhysicalPage& page = *pageIt->second;
            if (page.kind != R2004PhysicalPage::Kind::Data
                || page.pageId != pageId
                || sectionPage.pageNum != pageId
                || sectionPage.dataSize != dataPageSize
                || sectionPage.startOffset != startOffset
                || dataPageSize != page.payloadSize)
                return fail(ValidationCategory::Map);
        }
    }
    if (!cursor.atEnd() || referencedPageIds.size() != dataPages.size())
        return fail(ValidationCategory::Map);
    return ok();
}

ValidationResult validateR2007PageMap(const R2007Snapshot& snapshot) {
    ByteCursor cursor(snapshot.pageMapData);
    std::unordered_set<std::uint64_t> pageIds;
    pageIds.reserve(snapshot.pages.size());
    for (const R2007Page& page : snapshot.pages) {
        std::uint64_t pageSize = 0;
        std::uint64_t pageId = 0;
        if (!cursor.readU64(pageSize) || !cursor.readU64(pageId))
            return fail(ValidationCategory::Map);
        if (!pageIds.insert(pageId).second)
            return fail(ValidationCategory::Duplicate);
        if (pageId != page.id || pageSize != page.bytes.size())
            return fail(ValidationCategory::Map);
    }
    return cursor.atEnd() ? ok() : fail(ValidationCategory::Map);
}

ValidationResult validateR2007SectionMap(const R2007Snapshot& snapshot) {
    ByteCursor cursor(snapshot.sectionMapData);
    std::unordered_set<std::uint32_t> pageIds;
    for (const R2007Section& section : snapshot.sections) {
        std::uint64_t dataSize = 0;
        std::uint64_t maxSize = 0;
        std::uint64_t encrypted = 0;
        std::uint64_t hashCode = 0;
        std::uint64_t nameSize = 0;
        std::uint64_t unknown = 0;
        std::uint64_t encoding = 0;
        std::uint64_t pageCount = 0;
        if (!cursor.readU64(dataSize) || !cursor.readU64(maxSize)
            || !cursor.readU64(encrypted) || !cursor.readU64(hashCode)
            || !cursor.readU64(nameSize) || !cursor.readU64(unknown)
            || !cursor.readU64(encoding) || !cursor.readU64(pageCount)
            || !matchesR2007Name(cursor, nameSize, section.name))
            return fail(ValidationCategory::Map);
        if (dataSize != section.size || maxSize != section.maxSize
            || encrypted != section.encrypted || hashCode != section.hashCode
            || unknown != 0 || encoding != section.encoding
            || pageCount != section.pages.size())
            return fail(ValidationCategory::Map);

        for (const R2007SectionPage& page : section.pages) {
            std::uint64_t startOffset = 0;
            std::uint64_t pageSize = 0;
            std::uint64_t pageId = 0;
            std::uint64_t uncompressedSize = 0;
            std::uint64_t compressedSize = 0;
            std::uint64_t checksum = 0;
            std::uint64_t crc = 0;
            if (!cursor.readU64(startOffset) || !cursor.readU64(pageSize)
                || !cursor.readU64(pageId)
                || !cursor.readU64(uncompressedSize)
                || !cursor.readU64(compressedSize)
                || !cursor.readU64(checksum) || !cursor.readU64(crc))
                return fail(ValidationCategory::Map);
            if (pageId > std::numeric_limits<std::uint32_t>::max()
                || !pageIds.insert(static_cast<std::uint32_t>(pageId)).second)
                return fail(ValidationCategory::Duplicate);
            if (startOffset != page.startOffset || pageSize != page.pageSize
                || pageId != page.pageId
                || uncompressedSize != page.uncompressedSize
                || compressedSize != page.compressedSize
                || checksum != page.checksum || crc != page.crc)
                return fail(ValidationCategory::Map);
        }
    }

    std::uint64_t dataSize = 0;
    std::uint64_t maxSize = 0;
    std::uint64_t encrypted = 0;
    std::uint64_t hashCode = 0;
    std::uint64_t nameSize = 0;
    std::uint64_t unknown = 0;
    std::uint64_t encoding = 0;
    std::uint64_t pageCount = 0;
    if (!cursor.readU64(dataSize) || !cursor.readU64(maxSize)
        || !cursor.readU64(encrypted) || !cursor.readU64(hashCode)
        || !cursor.readU64(nameSize) || !cursor.readU64(unknown)
        || !cursor.readU64(encoding) || !cursor.readU64(pageCount)
        || dataSize != 0 || maxSize != kR2007EmptySectionMaxSize
        || encrypted != 0 || hashCode != 0 || nameSize != 0 || unknown != 0
        || encoding != 4 || pageCount != 0)
        return fail(ValidationCategory::Map);
    return cursor.atEnd() ? ok() : fail(ValidationCategory::Map);
}

ValidationResult validateR2007DataPage(const R2007Section& section,
                                       const R2007SectionPage& sectionPage,
                                       const R2007Page& physicalPage) {
    std::uint64_t alignedCompressedSize = 0;
    std::uint64_t blockCountNumerator = 0;
    if (sectionPage.pageSize == 0
        || sectionPage.compressedSize == 0
        || sectionPage.uncompressedSize == 0
        || sectionPage.compressedSize > sectionPage.pageSize
        || sectionPage.uncompressedSize > sectionPage.pageSize
        || (section.encoding != 1 && section.encoding != 4)
        || !dwgSafety::alignUp8(sectionPage.compressedSize,
                                alignedCompressedSize)
        || !dwgSafety::add(alignedCompressedSize, 250,
                           blockCountNumerator))
        return fail(ValidationCategory::Topology);
    const std::uint64_t blockCount = blockCountNumerator / 251;
    std::uint64_t encodedSize = 0;
    std::uint64_t decodedSize = 0;
    if (blockCount == 0
        || blockCount > std::numeric_limits<std::uint32_t>::max()
        || !dwgSafety::multiply(blockCount, 255, encodedSize)
        || !dwgSafety::multiply(blockCount, 251, decodedSize)
        || encodedSize > physicalPage.bytes.size()
        || sectionPage.compressedSize > decodedSize
        || encodedSize > std::numeric_limits<std::size_t>::max()
        || decodedSize > std::numeric_limits<std::size_t>::max()
        || sectionPage.uncompressedSize
               > std::numeric_limits<std::size_t>::max())
        return fail(ValidationCategory::Topology);
    try {
        std::vector<std::uint8_t> encoded(
            physicalPage.bytes.cbegin(), physicalPage.bytes.cbegin()
                + static_cast<std::size_t>(encodedSize));
        std::vector<std::uint8_t> decodedRs(
            static_cast<std::size_t>(decodedSize), 0);
        const bool decoded = section.encoding == 1
            ? dwgRSCodec::decode251(
                encoded.data(), decodedRs.data(),
                static_cast<std::uint32_t>(blockCount))
            : section.encoding == 4
                ? dwgRSCodec::decode251I(
                    encoded.data(), decodedRs.data(),
                    static_cast<std::uint32_t>(blockCount))
                : false;
        if (!decoded)
            return fail(ValidationCategory::ReedSolomon);
        std::vector<std::uint8_t> logical(
            static_cast<std::size_t>(sectionPage.uncompressedSize), 0);
        if (sectionPage.compressedSize == sectionPage.uncompressedSize) {
            std::copy_n(decodedRs.cbegin(), logical.size(), logical.begin());
        } else {
            dwgCompressor compressor;
            if (!compressor.decompress21(
                    decodedRs.data(), logical.data(),
                    sectionPage.compressedSize,
                    sectionPage.uncompressedSize)
                || compressor.decompressedBytes()
                       != sectionPage.uncompressedSize)
                return fail(ValidationCategory::Decompression);
        }
        if (dwgUtil::checksum21(0, logical.data(), logical.size())
                != sectionPage.checksum
            || dwgUtil::crc64Mirrored(
                   dwgUtil::updateSeed1(kR2007CrcSeed,
                                        sectionPage.compressedSize),
                   decodedRs.data(), sectionPage.compressedSize)
                   != sectionPage.crc)
            return fail(ValidationCategory::Checksum);
    } catch (...) {
        return fail(ValidationCategory::Topology);
    }
    return ok();
}

ValidationResult validateR2004(const R2004Snapshot& snapshot) {
    try {
        const R2004LayoutReceipt& receipt = snapshot.receipt;
        if (receipt.version < DRW::AC1018
            || receipt.version > DRW::AC1032
            || receipt.fileHeaderSize != kR2004FileHeaderSize
            || snapshot.fileHeader.size() != receipt.fileHeaderSize
            || receipt.dataStart != receipt.fileHeaderSize
            || snapshot.assembled.size() != receipt.actualFileSize
            || receipt.pages.size() != snapshot.pageSizes.size()
            || receipt.pages.size() != snapshot.dataPages.size() + 2
            || snapshot.pageSizes.size()
                   > std::numeric_limits<std::uint32_t>::max()
            || snapshot.dataSectionMapPage.empty()
            || snapshot.sectionPageMapPage.empty())
            return fail(ValidationCategory::Header);

        const std::uint8_t* span = nullptr;
        if (!checkedSpan(snapshot.assembled, 0, snapshot.fileHeader.size(),
                         span)
            || !std::equal(snapshot.fileHeader.cbegin(),
                           snapshot.fileHeader.cend(), span))
            return fail(ValidationCategory::Span);

        std::uint64_t address = receipt.dataStart;
        for (std::size_t index = 0; index < receipt.pages.size(); ++index) {
            const R2004PhysicalPage& page = receipt.pages[index];
            const std::uint64_t expectedSize = index < snapshot.dataPages.size()
                ? snapshot.dataPages[index].size()
                : index == snapshot.dataPages.size()
                    ? snapshot.dataSectionMapPage.size()
                    : snapshot.sectionPageMapPage.size();
            if (page.pageId != index + 1
                || page.address != address
                || page.size != expectedSize
                || page.size > std::numeric_limits<std::uint32_t>::max()
                || snapshot.pageSizes[index] != page.size
                || page.address % kR2004PageAlignment != 0)
                return fail(ValidationCategory::Topology);
            if (!checkedSpan(snapshot.assembled, page.address, page.size,
                             span))
                return fail(ValidationCategory::Span);
            if (index < snapshot.dataPages.size()) {
                if (page.kind != R2004PhysicalPage::Kind::Data
                    || page.size != snapshot.dataPages[index].size()
                    || page.size < 32
                    || page.headerOffset != page.address
                    || page.headerSize != 32
                    || page.payloadOffset != page.address + 32
                    || page.payloadSize != page.size - 32
                    || !std::equal(snapshot.dataPages[index].cbegin(),
                                   snapshot.dataPages[index].cend(), span))
                    return fail(ValidationCategory::Topology);
            } else if (index == snapshot.dataPages.size()) {
                if (page.kind != R2004PhysicalPage::Kind::DataSectionMap
                    || page.size != snapshot.dataSectionMapPage.size()
                    || page.headerOffset != page.address
                    || page.headerSize != 20
                    || page.payloadOffset != page.address + 20
                    || page.payloadSize != page.size - 20
                    || !std::equal(snapshot.dataSectionMapPage.cbegin(),
                                   snapshot.dataSectionMapPage.cend(), span))
                    return fail(ValidationCategory::Topology);
            } else if (page.kind != R2004PhysicalPage::Kind::SectionPageMap
                       || page.size != snapshot.sectionPageMapPage.size()
                       || page.headerOffset != page.address
                       || page.headerSize != 20
                       || page.payloadOffset != page.address + 20
                       || page.payloadSize != page.size - 20
                       || !std::equal(snapshot.sectionPageMapPage.cbegin(),
                                      snapshot.sectionPageMapPage.cend(), span))
                return fail(ValidationCategory::Topology);
            if (!dwgSafety::add(address, page.size, address))
                return fail(ValidationCategory::Span);
        }
        if (address != receipt.actualFileSize
            || receipt.actualFileSize != receipt.declaredFileSize)
            return fail(ValidationCategory::Header);

        std::vector<bool> seen(snapshot.dataPages.size() + 1, false);
        for (const R2004Section& section : snapshot.sections) {
            if (section.name.empty() || section.maxPageSize == 0
                || section.dataSize != section.data.size())
                return fail(ValidationCategory::Topology);
            std::uint64_t pageCountNumerator = 0;
            if (!dwgSafety::add(
                    section.dataSize,
                    static_cast<std::uint64_t>(section.maxPageSize) - 1,
                    pageCountNumerator))
                return fail(ValidationCategory::Topology);
            const std::uint64_t expectedPageCount = std::max<std::uint64_t>(
                1, pageCountNumerator / section.maxPageSize);
            if (section.pages.size() != expectedPageCount)
                return fail(ValidationCategory::Topology);
            for (std::size_t index = 0; index < section.pages.size();
                 ++index) {
                const R2004Section::Page& sectionPage = section.pages[index];
                std::uint64_t expectedStartOffset = 0;
                if (!dwgSafety::multiply(
                        static_cast<std::uint64_t>(index),
                        section.maxPageSize, expectedStartOffset)
                    || expectedStartOffset > section.dataSize)
                    return fail(ValidationCategory::Topology);
                const std::uint64_t remainingSize =
                    section.dataSize - expectedStartOffset;
                const std::uint64_t logicalSize = std::min<std::uint64_t>(
                    section.maxPageSize, remainingSize);
                if (sectionPage.pageNum == 0
                    || sectionPage.pageNum > snapshot.dataPages.size()
                    || sectionPage.startOffset != expectedStartOffset
                    || seen[sectionPage.pageNum])
                    return fail(ValidationCategory::Topology);
                const R2004PhysicalPage& page = snapshot.receipt.pages[
                    sectionPage.pageNum - 1];
                const std::uint8_t* payload = nullptr;
                if (page.kind != R2004PhysicalPage::Kind::Data
                    || page.sectionId != section.sectionId
                    || page.logicalOffset != expectedStartOffset
                    || sectionPage.dataSize != page.payloadSize
                    || page.payloadSize != section.maxPageSize
                    || !checkedSpan(snapshot.assembled, page.payloadOffset,
                                    page.payloadSize, payload))
                    return fail(ValidationCategory::Topology);
                const std::size_t logicalSizeAsSizeT =
                    static_cast<std::size_t>(logicalSize);
                const std::size_t startOffsetAsSizeT =
                    static_cast<std::size_t>(expectedStartOffset);
                const std::size_t payloadSizeAsSizeT =
                    static_cast<std::size_t>(page.payloadSize);
                if (!std::equal(section.data.cbegin() + startOffsetAsSizeT,
                                section.data.cbegin() + startOffsetAsSizeT
                                    + logicalSizeAsSizeT,
                                payload)
                    || !std::all_of(payload + logicalSizeAsSizeT,
                                    payload + payloadSizeAsSizeT,
                                    [](std::uint8_t value) {
                                        return value == 0;
                                    }))
                    return fail(ValidationCategory::Topology);
                seen[sectionPage.pageNum] = true;
            }
        }
        if (std::any_of(seen.cbegin() + 1, seen.cend(),
                        [](bool referenced) { return !referenced; }))
            return fail(ValidationCategory::Topology);

        for (std::size_t index = 0; index < snapshot.dataPages.size();
             ++index) {
            const R2004PhysicalPage& page = receipt.pages[index];
            if (!checkedSpan(snapshot.assembled, page.address, page.size,
                             span))
                return fail(ValidationCategory::Span);
            std::array<std::uint8_t, 32> header{};
            std::copy(span, span + header.size(), header.begin());
            dwgCompressor::decrypt18Hdr(header.data(), header.size(),
                                        page.address);
            if (getRL(header.data()) != 0x4163043b
                || getRL(header.data() + 4) != page.sectionId
                || getRL(header.data() + 8) != page.payloadSize
                || getRL(header.data() + 12) != page.payloadSize
                || getRL(header.data() + 16) != page.logicalOffset)
                return fail(ValidationCategory::Topology);
            const std::uint32_t storedDataChecksum =
                getRL(header.data() + 28);
            const auto dataChecksum = dwgUtil::checksum18(
                0, span + header.size(), page.payloadSize);
            if (storedDataChecksum != dataChecksum)
                return fail(ValidationCategory::Checksum);
            const std::uint32_t storedHeaderChecksum =
                getRL(header.data() + 24);
            std::fill(header.begin() + 24, header.begin() + 28, 0);
            if (dwgUtil::checksum18(dataChecksum, header.data(),
                                    header.size()) != storedHeaderChecksum)
                return fail(ValidationCategory::Checksum);
        }

        const auto validateSystemPage = [&](const R2004PhysicalPage& pageReceipt,
                                            const std::vector<std::uint8_t>& page,
                                            std::uint32_t expectedType,
                                            const std::vector<std::uint8_t>& data) {
            if (page.size() != pageReceipt.size
                || !checkedSpan(snapshot.assembled, pageReceipt.address,
                                 pageReceipt.size, span)
                || pageReceipt.size < 20
                || pageReceipt.headerSize != 20
                || pageReceipt.payloadSize != pageReceipt.size - 20
                || pageReceipt.payloadSize < 1
                || !std::equal(page.cbegin(), page.cend(), span))
                return fail(ValidationCategory::Topology);
            if (getRL(span) != expectedType
                || getRL(span + 4) != data.size()
                || getRL(span + 12) != 2)
                return fail(ValidationCategory::Topology);
            const std::uint64_t compressedSize = getRL(span + 8);
            if (compressedSize == 0 || compressedSize > pageReceipt.payloadSize)
                return fail(ValidationCategory::Topology);
            std::array<std::uint8_t, 20> header{};
            std::copy(span, span + header.size(), header.begin());
            const std::uint32_t storedChecksum = getRL(header.data() + 16);
            std::fill(header.begin() + 16, header.end(), 0);
            const auto headerChecksum = dwgUtil::checksum18(
                0, header.data(), header.size());
            const auto calculated = dwgUtil::checksum18(
                headerChecksum, span + header.size(), compressedSize);
            if (calculated != storedChecksum)
                return fail(ValidationCategory::Checksum);
            std::vector<std::uint8_t> decoded(data.size());
            dwgCompressor compressor;
            if (!compressor.decompress18(
                    span + header.size(), decoded.data(), compressedSize,
                    data.size())
                || compressor.decompressedBytes() != data.size()
                || decoded != data)
                return fail(ValidationCategory::Decompression);
            return ok();
        };

        const ValidationResult dataDescriptorResult =
            validateR2004DataSectionMap(snapshot);
        if (!dataDescriptorResult)
            return dataDescriptorResult;
        const ValidationResult sectionDescriptorResult =
            validateR2004SectionPageMap(snapshot);
        if (!sectionDescriptorResult)
            return sectionDescriptorResult;

        const ValidationResult dataMapResult = validateSystemPage(
            receipt.pages[snapshot.dataPages.size()],
            snapshot.dataSectionMapPage, 0x4163003b,
            snapshot.dataSectionMapData);
        if (!dataMapResult)
            return dataMapResult;
        const ValidationResult sectionMapResult = validateSystemPage(
            receipt.pages[snapshot.dataPages.size() + 1],
            snapshot.sectionPageMapPage, 0x41630e3b,
            snapshot.sectionPageMapData);
        if (!sectionMapResult)
            return sectionMapResult;
        return ok();
    } catch (...) {
        return fail(ValidationCategory::Topology);
    }
}

ValidationResult validateR2007(const R2007Snapshot& snapshot) {
    try {
        const R2007LayoutReceipt& receipt = snapshot.receipt;
        std::uint64_t expectedDataStart = 0;
        if (!dwgSafety::add(receipt.metadataSize, receipt.fileHeaderPageSize,
                            expectedDataStart)
            || receipt.metadataSize != kR2007FileHeaderOffset
            || receipt.version < DRW::AC1021
            || receipt.version > DRW::AC1032
            || receipt.fileHeaderPageSize != kR2007FileHeaderPageSize
            || receipt.dataStart != expectedDataStart
            || snapshot.metadata.size() != receipt.metadataSize
            || snapshot.fileHeaderPage.size() != receipt.fileHeaderPageSize
            || snapshot.assembled.size() != receipt.actualFileSize
            || receipt.pages.size() != snapshot.pages.size()
            || receipt.pages.size() < 4
            || receipt.pageCount != snapshot.pages.size()
            || receipt.sectionCount != snapshot.sections.size() + 1u
            || receipt.pageMap1Id != 1
            || receipt.pageMap2Id != 2
            || receipt.pages.size() > std::numeric_limits<std::uint32_t>::max())
            return fail(ValidationCategory::Header);

        const std::uint8_t* span = nullptr;
        if (!checkedSpan(snapshot.assembled, 0, snapshot.metadata.size(), span)
            || !std::equal(snapshot.metadata.cbegin(), snapshot.metadata.cend(),
                           span)
            || !checkedSpan(snapshot.assembled, receipt.leadingHeaderAddress,
                             snapshot.fileHeaderPage.size(), span)
            || !std::equal(snapshot.fileHeaderPage.cbegin(),
                           snapshot.fileHeaderPage.cend(), span)
            || !checkedSpan(snapshot.assembled, receipt.trailingHeaderAddress,
                             snapshot.fileHeaderPage.size(), span)
            || !std::equal(snapshot.fileHeaderPage.cbegin(),
                           snapshot.fileHeaderPage.cend(), span))
            return fail(ValidationCategory::Span);

        if (snapshot.fileHeaderData.size() != kR2007FileHeaderDataSize
            || snapshot.fileHeaderPage.size() < kR2007EncodedHeaderSize)
            return fail(ValidationCategory::Header);
        std::vector<std::uint8_t> encodedHeader(
            snapshot.fileHeaderPage.cbegin(),
            snapshot.fileHeaderPage.cbegin() + kR2007EncodedHeaderSize);
        std::vector<std::uint8_t> decodedHeader(3u * 239u, 0);
        if (!dwgRSCodec::decode239I(encodedHeader.data(), decodedHeader.data(), 3)
            || !std::equal(snapshot.fileHeaderData.cbegin(),
                           snapshot.fileHeaderData.cend(),
                           decodedHeader.cbegin() + 32))
            return fail(ValidationCategory::ReedSolomon);

        std::uint64_t address = receipt.dataStart;
        std::unordered_map<std::uint32_t, std::size_t> pageIndexes;
        pageIndexes.reserve(snapshot.pages.size());
        for (std::size_t index = 0; index < snapshot.pages.size(); ++index) {
            const R2007PhysicalPage& receiptPage = receipt.pages[index];
            const R2007Page& page = snapshot.pages[index];
            if (receiptPage.pageId == 0
                || !pageIndexes.emplace(receiptPage.pageId, index).second
                || page.id != receiptPage.pageId
                || receiptPage.address != address
                || receiptPage.size != page.bytes.size()
                || receiptPage.size == 0
                || receiptPage.size % kR2007PageAlignment != 0)
                return fail(ValidationCategory::Topology);
            if (!checkedSpan(snapshot.assembled, receiptPage.address,
                             receiptPage.size, span)
                || !std::equal(page.bytes.cbegin(), page.bytes.cend(), span))
                return fail(ValidationCategory::Span);
            if (!dwgSafety::add(address, receiptPage.size, address))
                return fail(ValidationCategory::Span);
        }

        if (receipt.pages[1].address < receipt.dataStart)
            return fail(ValidationCategory::Topology);
        const std::uint64_t pageMap2Offset =
            receipt.pages[1].address - receipt.dataStart;
        std::uint64_t expectedFileSize = 0;
        if (receipt.leadingHeaderAddress != receipt.metadataSize
            || receipt.trailingHeaderAddress != address
            || !dwgSafety::add(address, receipt.fileHeaderPageSize,
                               expectedFileSize)
            || receipt.actualFileSize != expectedFileSize
            || receipt.declaredFileSize != receipt.actualFileSize
            || receipt.maxPageId != snapshot.pages.back().id
            || receipt.sectionMapId
                   != snapshot.pages[snapshot.pages.size() - 2].id
            || receipt.sectionMap2Id != snapshot.pages.back().id
            || receipt.pageMap2Offset != pageMap2Offset)
            return fail(ValidationCategory::Header);

        if (snapshot.pages[0].bytes != snapshot.pages[1].bytes
            || snapshot.pages[snapshot.pages.size() - 2].bytes
                   != snapshot.pages.back().bytes
            || snapshot.pageMapData.empty()
            || snapshot.sectionMapData.empty())
            return fail(ValidationCategory::Topology);
        const ValidationResult pageMapDescriptorResult =
            validateR2007PageMap(snapshot);
        if (!pageMapDescriptorResult)
            return pageMapDescriptorResult;
        const ValidationResult sectionMapDescriptorResult =
            validateR2007SectionMap(snapshot);
        if (!sectionMapDescriptorResult)
            return sectionMapDescriptorResult;
        if (snapshot.pageMap.uncompressedSize != snapshot.pageMapData.size()
            || snapshot.pageMap.compressedSize != snapshot.pageMapData.size()
            || snapshot.sectionMap.uncompressedSize
                   != snapshot.sectionMapData.size()
            || snapshot.sectionMap.compressedSize
                   != snapshot.sectionMapData.size())
            return fail(ValidationCategory::Topology);
        if (snapshot.pageMapData != buildR2007PageMap(snapshot.pages)
            || snapshot.sectionMapData
                   != buildR2007SectionMap(snapshot.sections))
            return fail(ValidationCategory::Topology);

        const auto validSystemPage = [](const R2007SystemPage& page,
                                        const std::vector<std::uint8_t>& payload,
                                        const std::vector<std::uint8_t>& physical) {
            std::uint64_t alignedSize = 0;
            std::uint64_t repeatedSize = 0;
            std::uint64_t blockCountNumerator = 0;
            if (payload.empty()
                || page.compressedSize != payload.size()
                || page.uncompressedSize != payload.size()
                || !dwgSafety::alignUp8(page.compressedSize, alignedSize)
                || !dwgSafety::multiply(alignedSize, page.correctionFactor,
                                        repeatedSize)
                || !dwgSafety::add(repeatedSize, 238, blockCountNumerator))
                return false;
            const std::uint64_t blockCount = blockCountNumerator / 239;
            std::uint64_t encodedSize = 0;
            if (blockCount == 0
                || blockCount > std::numeric_limits<std::uint32_t>::max()
                || !dwgSafety::multiply(blockCount, 255, encodedSize))
                return false;
            const std::uint64_t minimumPageSize = std::max<std::uint64_t>(
                encodedSize, 0x400);
            std::uint64_t alignedPageSize = 0;
            if (!dwgSafety::add(minimumPageSize, kR2007PageAlignment - 1,
                                alignedPageSize))
                return false;
            alignedPageSize &= ~static_cast<std::uint64_t>(
                kR2007PageAlignment - 1);
            const auto crc = dwgUtil::crc64Mirrored(
                dwgUtil::updateSeed1(kR2007CrcSeed, payload.size()),
                payload.data(), payload.size());
            return page.correctionFactor >= 2
                && physical.size() == alignedPageSize
                && page.compressedCrc == crc
                && page.uncompressedCrc == crc;
        };
        if (!validSystemPage(snapshot.pageMap, snapshot.pageMapData,
                             snapshot.pages[0].bytes)
            || !validSystemPage(snapshot.sectionMap, snapshot.sectionMapData,
                                snapshot.pages[snapshot.pages.size() - 2].bytes))
            return fail(ValidationCategory::Checksum);

        const auto decodeSystemPage = [&](const R2007PhysicalPage& pageReceipt,
                                          const R2007SystemPage& systemPage,
                                          const std::vector<std::uint8_t>& payload) {
            std::uint64_t alignedSize = 0;
            std::uint64_t repeatedSize = 0;
            std::uint64_t blockCountNumerator = 0;
            if (!dwgSafety::alignUp8(systemPage.compressedSize, alignedSize)
                || !dwgSafety::multiply(alignedSize,
                                        systemPage.correctionFactor,
                                        repeatedSize)
                || !dwgSafety::add(repeatedSize, 238, blockCountNumerator))
                return fail(ValidationCategory::Topology);
            const std::uint64_t blockCount = blockCountNumerator / 239;
            std::uint64_t encodedSize = 0;
            std::uint64_t decodedSize = 0;
            if (blockCount == 0
                || !dwgSafety::multiply(blockCount, 255, encodedSize)
                || !dwgSafety::multiply(blockCount, 239, decodedSize)
                || encodedSize > pageReceipt.size
                || decodedSize < payload.size()
                || encodedSize > std::numeric_limits<std::size_t>::max()
                || decodedSize > std::numeric_limits<std::size_t>::max())
                return fail(ValidationCategory::Topology);
            if (!checkedSpan(snapshot.assembled, pageReceipt.address,
                             pageReceipt.size, span))
                return fail(ValidationCategory::Span);
            try {
                std::vector<std::uint8_t> encoded(
                    span, span + static_cast<std::size_t>(encodedSize));
                std::vector<std::uint8_t> decoded(
                    static_cast<std::size_t>(decodedSize), 0);
                if (!dwgRSCodec::decode239I(
                        encoded.data(), decoded.data(),
                        static_cast<std::uint32_t>(blockCount))
                    || !std::equal(payload.cbegin(), payload.cend(),
                                   decoded.cbegin()))
                    return fail(ValidationCategory::ReedSolomon);
            } catch (...) {
                return fail(ValidationCategory::ReedSolomon);
            }
            return ok();
        };
        if (receipt.pages[0].kind != R2007PhysicalPage::Kind::PageMap
            || receipt.pages[1].kind != R2007PhysicalPage::Kind::PageMap
            || receipt.pages[receipt.pages.size() - 2].kind
                   != R2007PhysicalPage::Kind::SectionMap
            || receipt.pages.back().kind
                   != R2007PhysicalPage::Kind::SectionMap)
            return fail(ValidationCategory::Topology);
        const ValidationResult pageMapResult = decodeSystemPage(
            receipt.pages[0], snapshot.pageMap, snapshot.pageMapData);
        if (!pageMapResult)
            return pageMapResult;
        const ValidationResult pageMap2Result = decodeSystemPage(
            receipt.pages[1], snapshot.pageMap, snapshot.pageMapData);
        if (!pageMap2Result)
            return pageMap2Result;
        const ValidationResult sectionMapResult = decodeSystemPage(
            receipt.pages[receipt.pages.size() - 2], snapshot.sectionMap,
            snapshot.sectionMapData);
        if (!sectionMapResult)
            return sectionMapResult;
        const ValidationResult sectionMap2Result = decodeSystemPage(
            receipt.pages.back(), snapshot.sectionMap,
            snapshot.sectionMapData);
        if (!sectionMap2Result)
            return sectionMap2Result;

        const std::size_t dataPageBegin = 2;
        const std::size_t dataPageEnd = snapshot.pages.size() - 2;
        if (dataPageBegin >= dataPageEnd)
            return fail(ValidationCategory::Topology);
        std::vector<bool> referenced(dataPageEnd - dataPageBegin, false);
        std::vector<std::string> sectionNames;
        sectionNames.reserve(snapshot.sections.size());
        for (const R2007Section& section : snapshot.sections) {
            if (section.name.empty() || section.maxSize == 0
                || section.maxSize > dwgSafety::MaxBufferSize
                || section.maxSize > dwgSafety::MaxPageCap
                || (section.encoding != 1 && section.encoding != 4)
                || section.encrypted != 0
                || std::find(sectionNames.cbegin(), sectionNames.cend(),
                             section.name) != sectionNames.cend())
                return fail(ValidationCategory::Topology);
            sectionNames.push_back(section.name);

            std::uint64_t pageCountNumerator = 0;
            if (!dwgSafety::add(section.size, section.maxSize - 1,
                                pageCountNumerator))
                return fail(ValidationCategory::Topology);
            const std::uint64_t expectedPageCount = section.size == 0
                ? 0 : pageCountNumerator / section.maxSize;
            if (section.pages.size() != expectedPageCount)
                return fail(ValidationCategory::Topology);
            for (std::size_t pageIndex = 0; pageIndex < section.pages.size();
                 ++pageIndex) {
                const R2007SectionPage& sectionPage = section.pages[pageIndex];
                std::uint64_t expectedStartOffset = 0;
                if (!dwgSafety::multiply(static_cast<std::uint64_t>(pageIndex),
                                         section.maxSize, expectedStartOffset)
                    || expectedStartOffset > section.size)
                    return fail(ValidationCategory::Topology);
                const std::uint64_t remainingSize =
                    section.size - expectedStartOffset;
                const std::uint64_t expectedUncompressedSize =
                    std::min(section.maxSize, remainingSize);
                const auto pageIt = pageIndexes.find(sectionPage.pageId);
                if (pageIt == pageIndexes.end()
                    || pageIt->second < dataPageBegin
                    || pageIt->second >= dataPageEnd
                    || sectionPage.startOffset != expectedStartOffset
                    || sectionPage.pageSize != section.maxSize
                    || sectionPage.uncompressedSize
                           != expectedUncompressedSize
                    || sectionPage.compressedSize == 0
                    || sectionPage.compressedSize > sectionPage.pageSize)
                    return fail(ValidationCategory::Topology);
                const std::size_t pageIndexInStream =
                    pageIt->second - dataPageBegin;
                if (pageIndexInStream >= referenced.size()
                    || referenced[pageIndexInStream])
                    return fail(ValidationCategory::Duplicate);
                const R2007PhysicalPage& receiptPage = snapshot.receipt.pages[
                    dataPageBegin + pageIndexInStream];
                if (receiptPage.kind != R2007PhysicalPage::Kind::Data
                    || receiptPage.sectionName != section.name
                    || receiptPage.logicalOffset != sectionPage.startOffset
                    || receiptPage.encodedSize != sectionPage.compressedSize)
                    return fail(ValidationCategory::Topology);
                const R2007Page& physicalPage = snapshot.pages[
                    dataPageBegin + pageIndexInStream];
                std::uint64_t alignedCompressedSize = 0;
                std::uint64_t blockCountNumerator = 0;
                std::uint64_t expectedPhysicalSize = 0;
                if (!dwgSafety::alignUp8(sectionPage.compressedSize,
                                         alignedCompressedSize)
                    || !dwgSafety::add(alignedCompressedSize, 250,
                                       blockCountNumerator)
                    || !dwgSafety::multiply(blockCountNumerator / 251, 255,
                                            expectedPhysicalSize)
                    || !dwgSafety::add(expectedPhysicalSize,
                                       kR2007PageAlignment - 1,
                                       expectedPhysicalSize))
                    return fail(ValidationCategory::Topology);
                expectedPhysicalSize &= ~static_cast<std::uint64_t>(
                    kR2007PageAlignment - 1);
                if (sectionPage.compressedSize > physicalPage.bytes.size()
                    || sectionPage.uncompressedSize > sectionPage.pageSize
                    || expectedPhysicalSize != physicalPage.bytes.size())
                    return fail(ValidationCategory::Topology);
                const ValidationResult dataPageResult =
                    validateR2007DataPage(section, sectionPage, physicalPage);
                if (!dataPageResult)
                    return dataPageResult;
                referenced[pageIndexInStream] = true;
            }
            if (!section.pages.empty()) {
                const R2007SectionPage& lastPage = section.pages.back();
                std::uint64_t sectionEnd = 0;
                if (!dwgSafety::add(lastPage.startOffset,
                                    lastPage.uncompressedSize, sectionEnd)
                    || sectionEnd != section.size)
                    return fail(ValidationCategory::Topology);
            }
        }
        return std::all_of(referenced.cbegin(), referenced.cend(),
                           [](bool value) { return value; })
            ? ok() : fail(ValidationCategory::Topology);
    } catch (...) {
        return fail(ValidationCategory::Topology);
    }
}

#ifdef DWG_LAYOUT_VALIDATION_TESTS
TestSnapshotObserver g_testSnapshotObserver = nullptr;
#endif

} // namespace

ValidationResult validate(const R2004Snapshot& snapshot) {
    return validateR2004(snapshot);
}

ValidationResult validate(const R2007Snapshot& snapshot) {
    return validateR2007(snapshot);
}

#ifdef DWG_LAYOUT_VALIDATION_TESTS
ValidationResult validateR2007DataPageForTest(
        const R2007Section& section, const R2007SectionPage& sectionPage,
        const R2007Page& physicalPage) {
    return validateR2007DataPage(section, sectionPage, physicalPage);
}

bool reserveTestMutationCapacity(Snapshot& snapshot) noexcept {
    try {
        return std::visit([](auto& state) {
            const auto reserveOne = [](std::vector<std::uint8_t>& bytes) {
                if (bytes.size() == std::numeric_limits<std::size_t>::max())
                    return false;
                bytes.reserve(bytes.size() + 1);
                return true;
            };
            using State = std::decay_t<decltype(state)>;
            if constexpr (std::is_same_v<State, R2004Snapshot>) {
                return reserveOne(state.dataSectionMapData)
                    && reserveOne(state.sectionPageMapData);
            } else {
                return reserveOne(state.pageMapData)
                    && reserveOne(state.sectionMapData);
            }
        }, snapshot);
    } catch (...) {
        return false;
    }
}

void setTestSnapshotObserver(TestSnapshotObserver observer) noexcept {
    g_testSnapshotObserver = observer;
}

void notifyTestSnapshot(Snapshot& snapshot) noexcept {
    if (g_testSnapshotObserver != nullptr)
        g_testSnapshotObserver(snapshot);
}
#endif

} // namespace dwgWriterLayoutValidation
