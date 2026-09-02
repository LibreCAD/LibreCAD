/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "drw_datastorage.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "intern/drw_reserve.h"

namespace {

using namespace DRW_DataStorageConst;

constexpr char kAcisBinaryFilePrefix[] = "ACIS BinaryFile";

bool findAcisBinaryFileMarker(const std::vector<std::uint8_t>& payload,
                              std::uint32_t& offset) {
    constexpr std::size_t markerLength = sizeof(kAcisBinaryFilePrefix) - 1u;
    if (payload.size() < markerLength)
        return false;
    for (std::size_t i = 0; i + markerLength <= payload.size(); ++i) {
        bool matches = true;
        for (std::size_t j = 0; j < markerLength; ++j) {
            if (payload[i + j]
                != static_cast<std::uint8_t>(kAcisBinaryFilePrefix[j])) {
                matches = false;
                break;
            }
        }
        if (matches) {
            offset = static_cast<std::uint32_t>(i);
            return true;
        }
    }
    return false;
}

class ByteReader {
public:
    ByteReader(const std::uint8_t* data, std::size_t size)
        : m_data(data)
        , m_size(size) {}

    std::size_t length() const { return m_size; }

    bool has(std::uint64_t offset, std::uint64_t size) const {
        if (m_data == nullptr)
            return false;
        if (offset > m_size)
            return false;
        return size <= (m_size - static_cast<std::size_t>(offset));
    }

    std::uint16_t u16(std::uint64_t offset) const {
        const auto i = static_cast<std::size_t>(offset);
        return static_cast<std::uint16_t>(m_data[i])
            | (static_cast<std::uint16_t>(m_data[i + 1]) << 8);
    }

    std::uint32_t u32(std::uint64_t offset) const {
        const auto i = static_cast<std::size_t>(offset);
        return static_cast<std::uint32_t>(m_data[i])
            | (static_cast<std::uint32_t>(m_data[i + 1]) << 8)
            | (static_cast<std::uint32_t>(m_data[i + 2]) << 16)
            | (static_cast<std::uint32_t>(m_data[i + 3]) << 24);
    }

    std::int32_t i32(std::uint64_t offset) const {
        return static_cast<std::int32_t>(u32(offset));
    }

    std::uint64_t u64(std::uint64_t offset) const {
        return static_cast<std::uint64_t>(u32(offset))
            | (static_cast<std::uint64_t>(u32(offset + 4)) << 32);
    }

    UTF8STRING ascii(std::uint64_t offset, std::uint64_t size) const {
        if (!has(offset, size))
            return {};
        UTF8STRING value;
        value.reserve(static_cast<std::size_t>(std::min<std::uint64_t>(size, 16)));
        const auto start = static_cast<std::size_t>(offset);
        for (std::uint64_t i = 0; i < size; ++i) {
            const std::uint8_t ch = m_data[start + static_cast<std::size_t>(i)];
            if (ch == 0)
                break;
            value.push_back(static_cast<char>(ch));
        }
        return value;
    }

    bool asciiZ(std::uint64_t offset, std::uint64_t limit,
                UTF8STRING& value) const {
        value.clear();
        if (m_data == nullptr || offset > limit || offset > m_size)
            return false;
        const std::size_t begin = static_cast<std::size_t>(offset);
        const std::size_t end = std::min<std::size_t>(
            m_size, static_cast<std::size_t>(limit));
        value.reserve(std::min<std::size_t>(end - begin, 16u));
        for (std::size_t i = begin; i < end; ++i) {
            const std::uint8_t ch = m_data[i];
            if (ch == 0)
                return true;
            value.push_back(static_cast<char>(ch));
        }
        value.clear();
        return false;
    }

    std::vector<std::uint8_t> bytes(std::uint64_t offset, std::uint64_t size) const {
        if (!has(offset, size))
            return {};
        const auto first = m_data + static_cast<std::size_t>(offset);
        try {
            return std::vector<std::uint8_t>(
                first, first + static_cast<std::size_t>(size));
        } catch (...) {
            return {};
        }
    }

private:
    const std::uint8_t* m_data;
    std::size_t m_size;
};

void pushDiag(DRW_DataStorageSection& section,
              std::string code,
              std::string message,
              std::uint64_t handle = 0,
              bool hasHandle = false,
              std::uint64_t offset = 0,
              bool hasOffset = false) {
    const auto kind = [&code]() {
        // These diagnostics describe tolerated producer variations or
        // revision history that do not invalidate the section bounds. All
        // unclassified diagnostics remain structural so replay fails closed.
        if (code == "datastorage-duplicate-record-handle")
            return DRW_DataStorageDiagnostic::Kind::Informational;
        if (code == "datastorage-segment-signature-invalid"
            || code == "datastorage-segment-index-mismatch"
            || code == "datastorage-segment-header-size-mismatch"
            || code == "datastorage-segment-size-not-aligned")
            return DRW_DataStorageDiagnostic::Kind::SupportedOpaque;
        return DRW_DataStorageDiagnostic::Kind::StructuralInvalid;
    }();
    DRW_DataStorageDiagnostic d;
    d.code = std::move(code);
    d.message = std::move(message);
    d.handle = handle;
    d.hasHandle = hasHandle;
    d.offset = offset;
    d.hasOffset = hasOffset;
    d.kind = kind;
    section.diagnostics.push_back(std::move(d));
    if (kind == DRW_DataStorageDiagnostic::Kind::StructuralInvalid) {
        section.structurallyValid = false;
        section.replayAllowed = false;
    }
}

UTF8STRING handleKey(std::uint64_t value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%llX",
                  static_cast<unsigned long long>(value));
    return UTF8STRING(buf);
}

bool checkedAdd(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& result) {
    if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs)
        return false;
    result = lhs + rhs;
    return true;
}

bool checkedMultiply(std::uint64_t lhs, std::uint64_t rhs,
                     std::uint64_t& result) {
    if (lhs != 0
        && rhs > std::numeric_limits<std::uint64_t>::max() / lhs)
        return false;
    result = lhs * rhs;
    return true;
}

bool isRangeWithin(std::uint64_t offset, std::uint64_t size,
                   std::uint64_t limit) {
    return offset <= limit && size <= limit - offset;
}

bool roundUpTo16(std::uint64_t n, std::uint64_t& result) {
    if (n > std::numeric_limits<std::uint64_t>::max() - 15u)
        return false;
    result = (n + 15u) & ~std::uint64_t{15};
    return true;
}

std::string toLower(const UTF8STRING& name) {
    std::string out;
    out.reserve(name.size());
    for (unsigned char ch : name)
        out.push_back(static_cast<char>(std::tolower(ch)));
    return out;
}

DRW_DataStorageSegmentKind classifyKind(const UTF8STRING& name) {
    const std::string n = toLower(name);
    if (n == "segidx")
        return DRW_DataStorageSegmentKind::SegmentIndex;
    if (n == "datidx" || n == "dindex")
        return DRW_DataStorageSegmentKind::DataIndex;
    if (n == "_data_" || n == "data")
        return DRW_DataStorageSegmentKind::Data;
    if (n == "schidx")
        return DRW_DataStorageSegmentKind::SchemaIndex;
    if (n == "schdat")
        return DRW_DataStorageSegmentKind::SchemaData;
    if (n == "search")
        return DRW_DataStorageSegmentKind::Search;
    if (n.rfind("blob", 0) == 0)
        return DRW_DataStorageSegmentKind::Blob;
    if (n == "prvsav")
        return DRW_DataStorageSegmentKind::PreviousSave;
    if (n == "freesp")
        return DRW_DataStorageSegmentKind::FreeSpace;
    if (n == "schema")
        return DRW_DataStorageSegmentKind::Schema;
    return DRW_DataStorageSegmentKind::Unknown;
}

bool isDataSegment(const DRW_DataStorageSegment& segment) {
    return segment.kind == DRW_DataStorageSegmentKind::Data
        || segment.kind == DRW_DataStorageSegmentKind::Blob;
}

std::uint32_t capCount(std::uint32_t declared,
                       std::uint64_t remainingBytes,
                       std::uint32_t entrySize,
                       DRW_DataStorageSection& section,
                       const char* what) {
    if (entrySize == 0)
        return 0;
    const std::uint64_t byRemaining = remainingBytes / entrySize;
    std::uint32_t capped = declared;
    if (byRemaining < capped)
        capped = static_cast<std::uint32_t>(byRemaining);
    if (capped > HARD_MAX_ENTRIES)
        capped = HARD_MAX_ENTRIES;
    if (capped < declared) {
        pushDiag(section,
                 "datastorage-count-capped",
                 std::string("DataStorage ") + what + " count capped from "
                     + std::to_string(declared) + " to " + std::to_string(capped));
    }
    return capped;
}

void readSegmentIndex(const ByteReader& r,
                      std::uint32_t offset,
                      std::uint32_t count,
                      DRW_DataStorageSection& section) {
    std::uint64_t entriesBase = 0;
    if (!checkedAdd(static_cast<std::uint64_t>(offset), SEGMENT_HEADER_SIZE,
                    entriesBase)) {
        pushDiag(section, "datastorage-segment-index-offset-overflow",
                 "DataStorage segment index entry offset overflows",
                 0, false, offset, true);
        return;
    }

    // A valid segidx entry describes the extent of the segment-index
    // segment itself. Use it to prevent a corrupt count from consuming the
    // following data-index or payload segments. Keep the section-wide bound
    // when the optional self-reference is absent so older files remain
    // readable.
    std::uint64_t readableEnd = r.length();
    if (r.has(entriesBase, SEGMENT_INDEX_ENTRY_SIZE)) {
        const std::uint64_t indexedOffset = r.u64(entriesBase);
        const std::uint32_t indexedSize = r.u32(entriesBase + 8);
        if (indexedOffset == offset) {
            std::uint64_t indexedEnd = 0;
            if (indexedSize < SEGMENT_HEADER_SIZE
                || !checkedAdd(indexedOffset, indexedSize, indexedEnd)
                || !isRangeWithin(indexedOffset, indexedSize, r.length())) {
                pushDiag(section, "datastorage-segment-index-size-invalid",
                         "DataStorage segment index self-reference is invalid",
                         0, false, entriesBase, true);
                readableEnd = entriesBase;
            } else {
                readableEnd = std::min(readableEnd, indexedEnd);
            }
        }
    }

    const std::uint64_t remaining =
        entriesBase < readableEnd ? (readableEnd - entriesBase) : 0;
    const std::uint32_t maxCount =
        capCount(count, remaining, SEGMENT_INDEX_ENTRY_SIZE, section,
                 "segmentIndexEntryCount");

    if (!DRW::reserve(section.segments, static_cast<int>(maxCount))) {
        pushDiag(section, "datastorage-segment-index-reserve-failed",
                 "DataStorage segment index reserve failed");
        return;
    }

    std::unordered_map<std::int32_t, std::uint32_t> seenHeaderIndexes;
    for (std::uint32_t i = 0; i < maxCount; ++i) {
        std::uint64_t entryDelta = 0;
        std::uint64_t entryOffset = 0;
        if (!checkedMultiply(static_cast<std::uint64_t>(i),
                             SEGMENT_INDEX_ENTRY_SIZE, entryDelta)
            || !checkedAdd(entriesBase, entryDelta, entryOffset)
            || !isRangeWithin(entryOffset, SEGMENT_INDEX_ENTRY_SIZE,
                              readableEnd)
            || !r.has(entryOffset, SEGMENT_INDEX_ENTRY_SIZE)) {
            pushDiag(section, "datastorage-segment-index-truncated",
                     "DataStorage segment index entry " + std::to_string(i)
                         + " is outside the segment bounds",
                     0, false, entryOffset, true);
            break;
        }

        DRW_DataStorageSegment segment;
        segment.index = i;
        segment.offset = r.u64(entryOffset);
        segment.size = r.u32(entryOffset + 8);
        const bool payloadOffsetValid = checkedAdd(
            segment.offset, SEGMENT_HEADER_SIZE, segment.payloadOffset);
        if (!payloadOffsetValid)
            segment.payloadOffset = 0;
        segment.payloadByteLength =
            segment.size >= SEGMENT_HEADER_SIZE
                ? segment.size - SEGMENT_HEADER_SIZE
                : 0;

        if (segment.offset != 0 && segment.size != 0
            && segment.size % SEGMENT_SIZE_ALIGNMENT != 0) {
            pushDiag(section, "datastorage-segment-size-not-aligned",
                     "DataStorage segment " + std::to_string(i)
                         + " size is not aligned to the specification boundary",
                     0, false, segment.offset, true);
        }

        if (segment.offset != 0 && segment.size < SEGMENT_HEADER_SIZE) {
            pushDiag(section, "datastorage-segment-size-too-small",
                     "DataStorage segment " + std::to_string(i)
                         + " is smaller than its header",
                     0, false, segment.offset, true);
            section.segments.push_back(std::move(segment));
            continue;
        }

        std::uint64_t segmentEndOffset = 0;
        if (!checkedAdd(segment.offset, segment.size, segmentEndOffset)
            || !isRangeWithin(segment.offset, segment.size, r.length())) {
            pushDiag(section, "datastorage-segment-size-out-of-bounds",
                     "DataStorage segment " + std::to_string(i)
                         + " extends beyond the section bounds",
                     0, false, segment.offset, true);
        }

        if (segment.offset == 0) {
            // Empty placeholder slot — keep dense array, skip header.
            section.segments.push_back(std::move(segment));
            continue;
        }

        if (r.has(segment.offset, SEGMENT_HEADER_SIZE)) {
            const std::int32_t headerSegmentIndex = r.i32(segment.offset + 8);
            segment.signature = r.u16(segment.offset);
            segment.name = r.ascii(segment.offset + 2, 6);
            segment.segmentIndex = headerSegmentIndex;
            segment.headerSegmentSize = r.u32(segment.offset + 16);
            segment.hasHeaderSegmentSize = true;
            if (segment.signature != SEGMENT_SIGNATURE) {
                pushDiag(section, "datastorage-segment-signature-invalid",
                         "DataStorage segment " + std::to_string(i)
                             + " has an invalid signature",
                         0, false, segment.offset, true);
            }
            if (headerSegmentIndex != static_cast<std::int32_t>(i)) {
                pushDiag(section, "datastorage-segment-index-mismatch",
                         "DataStorage segment " + std::to_string(i)
                             + " header index does not match its index entry",
                         0, false, segment.offset + 8u, true);
            }
            if (segment.headerSegmentSize != 0
                && segment.headerSegmentSize != segment.size) {
                pushDiag(section, "datastorage-segment-header-size-mismatch",
                         "DataStorage segment " + std::to_string(i)
                             + " header size does not match its index entry",
                         0, false, segment.offset + 16, true);
            }
            segment.revision = r.u32(segment.offset + 24);
            segment.hasRevision = true;
            segment.systemDataAlignmentOffset = r.u32(segment.offset + 32);
            segment.objectDataAlignmentOffset = r.u32(segment.offset + 36);
            const bool identityValid = segment.signature == SEGMENT_SIGNATURE
                && headerSegmentIndex == static_cast<std::int32_t>(i);
            if (identityValid) {
                segment.kind = classifyKind(segment.name);
                segment.isKnownKind =
                    segment.kind != DRW_DataStorageSegmentKind::Unknown;
            }

            const auto previous = seenHeaderIndexes.find(headerSegmentIndex);
            if (previous != seenHeaderIndexes.end()) {
                pushDiag(section, "datastorage-duplicate-segment-index",
                         "DataStorage segment header index "
                             + std::to_string(headerSegmentIndex)
                             + " is used by segments "
                             + std::to_string(previous->second) + " and "
                             + std::to_string(i),
                         0, false, segment.offset, true);
            } else {
                seenHeaderIndexes.emplace(headerSegmentIndex, i);
            }
        } else {
            pushDiag(section, "datastorage-segment-header-missing",
                     "DataStorage segment " + std::to_string(i)
                         + " header is outside the section bounds",
                     0, false, segment.offset, true);
        }

        section.segments.push_back(std::move(segment));
    }
}

bool isZeroPlaceholderDataIndexEntry(
    const DRW_DataStorageIndexEntry& entry,
    const std::vector<DRW_DataStorageSegment>& segments) {
    if (entry.segmentIndex != 0 || entry.localOffset != 0 || entry.schemaIndex != 0)
        return false;
    return segments.empty() || !isDataSegment(segments.front());
}

void readDataIndex(const ByteReader& r, DRW_DataStorageSection& section) {
    if (section.dataIndexSegmentIndex < 0)
        return;
    if (static_cast<std::size_t>(section.dataIndexSegmentIndex)
        >= section.segments.size()) {
        pushDiag(section, "datastorage-data-index-missing",
                 "DataStorage data-index segment "
                     + std::to_string(section.dataIndexSegmentIndex)
                     + " is not present");
        return;
    }

    const DRW_DataStorageSegment& segment =
        section.segments[static_cast<std::size_t>(section.dataIndexSegmentIndex)];
    std::uint64_t segmentEndOffset = 0;
    std::uint64_t offset = 0;
    if (!checkedAdd(segment.offset, segment.size, segmentEndOffset)
        || !checkedAdd(segment.offset, SEGMENT_HEADER_SIZE, offset)
        || !isRangeWithin(offset, 8, segmentEndOffset)
        || !r.has(offset, 8)) {
        pushDiag(section, "datastorage-data-index-truncated",
                 "DataStorage data-index header is outside the section bounds",
                 0, false, offset, true);
        return;
    }

    const std::int32_t rawCount = r.i32(offset);
    if (rawCount <= 0)
        return;

    std::uint64_t entriesBase = 0;
    if (!checkedAdd(offset, 8, entriesBase)) {
        pushDiag(section, "datastorage-data-index-truncated",
                 "DataStorage data-index entries offset overflows",
                 0, false, offset, true);
        return;
    }
    const std::uint64_t readableEnd =
        std::min<std::uint64_t>(segmentEndOffset, r.length());
    const std::uint64_t remaining =
        entriesBase < readableEnd ? (readableEnd - entriesBase) : 0;
    const std::uint32_t maxCount =
        capCount(static_cast<std::uint32_t>(rawCount), remaining,
                 DATA_INDEX_ENTRY_SIZE, section, "dataIndexEntryCount");

    if (!DRW::reserve(section.dataIndexEntries, static_cast<int>(maxCount))) {
        pushDiag(section, "datastorage-data-index-reserve-failed",
                 "DataStorage data index reserve failed");
        return;
    }

    std::unordered_map<std::string, std::uint32_t> seen;
    for (std::uint32_t i = 0; i < maxCount; ++i) {
        std::uint64_t entryDelta = 0;
        std::uint64_t entryOffset = 0;
        if (!checkedMultiply(i, DATA_INDEX_ENTRY_SIZE, entryDelta)
            || !checkedAdd(entriesBase, entryDelta, entryOffset)
            || !isRangeWithin(entryOffset, DATA_INDEX_ENTRY_SIZE,
                              segmentEndOffset)
            || !r.has(entryOffset, DATA_INDEX_ENTRY_SIZE)) {
            pushDiag(section, "datastorage-data-index-entry-truncated",
                     "DataStorage data-index entry " + std::to_string(i)
                         + " is outside the section bounds",
                     0, false, entryOffset, true);
            break;
        }

        DRW_DataStorageIndexEntry entry;
        entry.segmentIndex = r.u32(entryOffset);
        entry.localOffset = r.u32(entryOffset + 4);
        entry.schemaIndex = r.u32(entryOffset + 8);

        const std::string key = std::to_string(entry.segmentIndex) + ":"
            + std::to_string(entry.localOffset);
        const auto previous = seen.find(key);
        if (previous != seen.end()) {
            if (isZeroPlaceholderDataIndexEntry(entry, section.segments))
                continue;
            pushDiag(section, "datastorage-duplicate-data-index-entry",
                     "DataStorage data-index entries "
                         + std::to_string(previous->second) + " and "
                         + std::to_string(i) + " point to the same offset",
                     0, false, entryOffset, true);
            continue;
        }
        seen.emplace(key, i);
        section.dataIndexEntries.push_back(entry);
    }
}

bool getSegmentBounds(const ByteReader& r,
                      const DRW_DataStorageSegment& segment,
                      std::uint64_t& payloadStart,
                      std::uint64_t& segmentEnd) {
    if (segment.offset == 0 || segment.size < SEGMENT_HEADER_SIZE
        || !checkedAdd(segment.offset, SEGMENT_HEADER_SIZE, payloadStart)
        || !checkedAdd(segment.offset, segment.size, segmentEnd)) {
        return false;
    }
    segmentEnd = std::min<std::uint64_t>(segmentEnd, r.length());
    return payloadStart <= segmentEnd && r.has(payloadStart, 0);
}

bool getSystemDataBounds(const ByteReader& r,
                         const DRW_DataStorageSegment& segment,
                         std::uint64_t& payloadStart,
                         std::uint64_t& systemDataEnd,
                         std::uint64_t& segmentEnd,
                         DRW_DataStorageSection& section) {
    if (!getSegmentBounds(r, segment, payloadStart, segmentEnd)) {
        pushDiag(section, "datastorage-schema-segment-bounds-invalid",
                 "DataStorage schema segment bounds are invalid", 0, false,
                 segment.offset, true);
        return false;
    }

    systemDataEnd = segmentEnd;
    if (segment.systemDataAlignmentOffset == 0)
        return true;

    std::uint64_t alignmentBytes = 0;
    std::uint64_t alignedOffset = 0;
    if (!checkedMultiply(segment.systemDataAlignmentOffset, 16u,
                         alignmentBytes)
        || !checkedAdd(segment.offset, alignmentBytes, alignedOffset)
        || alignedOffset < payloadStart || alignedOffset > segmentEnd) {
        pushDiag(section, "datastorage-system-data-alignment-invalid",
                 "DataStorage system-data alignment is outside its segment",
                 0, false, segment.offset, true);
        return false;
    }
    systemDataEnd = alignedOffset;
    return true;
}

void readSchemaIndex(const ByteReader& r, DRW_DataStorageSection& section) {
    if (section.schemaIndexSegmentIndex < 0
        || static_cast<std::size_t>(section.schemaIndexSegmentIndex)
               >= section.segments.size()) {
        if (section.schemaIndexSegmentIndex >= 0) {
            pushDiag(section, "datastorage-schema-index-missing",
                     "DataStorage schema-index segment "
                         + std::to_string(section.schemaIndexSegmentIndex)
                         + " is not present");
        }
        return;
    }

    const DRW_DataStorageSegment& segment = section.segments[
        static_cast<std::size_t>(section.schemaIndexSegmentIndex)];
    if (segment.kind != DRW_DataStorageSegmentKind::SchemaIndex) {
        pushDiag(section, "datastorage-schema-index-kind-invalid",
                 "DataStorage schema-index reference does not name a schema-index segment",
                 0, false, segment.offset, true);
        return;
    }

    std::uint64_t payloadStart = 0;
    std::uint64_t systemDataEnd = 0;
    std::uint64_t segmentEnd = 0;
    if (!getSystemDataBounds(r, segment, payloadStart, systemDataEnd,
                             segmentEnd, section)
        || !isRangeWithin(payloadStart, 8, systemDataEnd)
        || !r.has(payloadStart, 8)) {
        pushDiag(section, "datastorage-schema-index-truncated",
                 "DataStorage schema-index header is outside its segment",
                 0, false, payloadStart, true);
        return;
    }

    section.schemaIndexUnknownPropertyCount = r.u32(payloadStart);
    section.schemaIndexUnknown1 = r.u32(payloadStart + 4);

    std::uint64_t entriesBase = 0;
    if (!checkedAdd(payloadStart, 8, entriesBase)) {
        pushDiag(section, "datastorage-schema-index-offset-overflow",
                 "DataStorage schema-index entry offset overflows", 0, false,
                 payloadStart, true);
        return;
    }
    const std::uint32_t entryCount = capCount(
        section.schemaIndexUnknownPropertyCount,
        entriesBase < systemDataEnd ? systemDataEnd - entriesBase : 0,
        SCHEMA_INDEX_ENTRY_SIZE, section, "schemaIndexEntryCount");
    if (!DRW::reserve(section.schemaIndexEntries,
                      static_cast<int>(entryCount))) {
        pushDiag(section, "datastorage-schema-index-reserve-failed",
                 "DataStorage schema-index entry reserve failed");
        return;
    }

    for (std::uint32_t i = 0; i < entryCount; ++i) {
        std::uint64_t delta = 0;
        std::uint64_t entryOffset = 0;
        if (!checkedMultiply(i, SCHEMA_INDEX_ENTRY_SIZE, delta)
            || !checkedAdd(entriesBase, delta, entryOffset)
            || !isRangeWithin(entryOffset, SCHEMA_INDEX_ENTRY_SIZE,
                              systemDataEnd)
            || !r.has(entryOffset, SCHEMA_INDEX_ENTRY_SIZE)) {
            pushDiag(section, "datastorage-schema-index-entry-truncated",
                     "DataStorage schema-index entry is outside its segment",
                     0, false, entryOffset, true);
            break;
        }
        DRW_DataStorageSchemaIndexEntry entry;
        entry.index = r.u32(entryOffset);
        entry.segmentIndex = r.u32(entryOffset + 4);
        entry.localOffset = r.u32(entryOffset + 8);
        section.schemaIndexEntries.push_back(entry);
    }

    std::uint64_t afterEntries = 0;
    std::uint64_t firstTableBytes = 0;
    if (!checkedMultiply(entryCount, SCHEMA_INDEX_ENTRY_SIZE,
                         firstTableBytes)
        || !checkedAdd(entriesBase, firstTableBytes, afterEntries)
        || !isRangeWithin(afterEntries, 16, systemDataEnd)
        || !r.has(afterEntries, 16)) {
        pushDiag(section, "datastorage-schema-index-header-truncated",
                 "DataStorage schema-index property-entry header is outside its segment",
                 0, false, afterEntries, true);
        return;
    }

    section.schemaIndexTag = r.u64(afterEntries);
    section.schemaIndexPropertyEntryCount = r.u32(afterEntries + 8);
    section.schemaIndexUnknown2 = r.u32(afterEntries + 12);

    std::uint64_t propertyEntriesBase = 0;
    if (!checkedAdd(afterEntries, 16, propertyEntriesBase)) {
        pushDiag(section, "datastorage-schema-index-offset-overflow",
                 "DataStorage schema property-entry offset overflows", 0,
                 false, afterEntries, true);
        return;
    }
    const std::uint32_t propertyEntryCount = capCount(
        section.schemaIndexPropertyEntryCount,
        propertyEntriesBase < systemDataEnd
            ? systemDataEnd - propertyEntriesBase : 0,
        SCHEMA_INDEX_ENTRY_SIZE, section, "schemaPropertyEntryCount");
    if (!DRW::reserve(section.schemaPropertyEntries,
                      static_cast<int>(propertyEntryCount))) {
        pushDiag(section, "datastorage-schema-property-index-reserve-failed",
                 "DataStorage schema property-entry reserve failed");
        return;
    }

    for (std::uint32_t i = 0; i < propertyEntryCount; ++i) {
        std::uint64_t delta = 0;
        std::uint64_t entryOffset = 0;
        if (!checkedMultiply(i, SCHEMA_INDEX_ENTRY_SIZE, delta)
            || !checkedAdd(propertyEntriesBase, delta, entryOffset)
            || !isRangeWithin(entryOffset, SCHEMA_INDEX_ENTRY_SIZE,
                              systemDataEnd)
            || !r.has(entryOffset, SCHEMA_INDEX_ENTRY_SIZE)) {
            pushDiag(section, "datastorage-schema-property-index-truncated",
                     "DataStorage schema property entry is outside its segment",
                     0, false, entryOffset, true);
            break;
        }
        DRW_DataStorageSchemaIndexEntry entry;
        // The schema-index property-entry table uses the DWG order
        // segment-index, local-offset, index (the first table is different).
        entry.segmentIndex = r.u32(entryOffset);
        entry.localOffset = r.u32(entryOffset + 4);
        entry.index = r.u32(entryOffset + 8);
        section.schemaPropertyEntries.push_back(entry);
    }
}

void readSchemaNames(const ByteReader& r,
                     DRW_DataStorageSection& section,
                     const DRW_DataStorageSegment& segment,
                     std::uint64_t namesOffset,
                     std::uint64_t segmentEnd) {
    if (namesOffset == segmentEnd)
        return;
    if (!isRangeWithin(namesOffset, 4, segmentEnd)
        || !r.has(namesOffset, 4)) {
        pushDiag(section, "datastorage-schema-names-truncated",
                 "DataStorage schema property-name count is outside its segment",
                 0, false, namesOffset, true);
        return;
    }

    const std::uint32_t declaredCount = r.u32(namesOffset);
    section.schemaPropertyNameCount = declaredCount;
    std::uint64_t namesBase = 0;
    if (!checkedAdd(namesOffset, 4, namesBase)) {
        pushDiag(section, "datastorage-schema-names-offset-overflow",
                 "DataStorage schema property-name offset overflows", 0,
                 false, namesOffset, true);
        return;
    }
    const std::uint32_t maxCount = capCount(
        declaredCount,
        namesBase < segmentEnd ? segmentEnd - namesBase : 0, 1, section,
        "schemaPropertyNameCount");
    if (!DRW::reserve(section.schemaPropertyNames,
                      static_cast<int>(maxCount))) {
        pushDiag(section, "datastorage-schema-names-reserve-failed",
                 "DataStorage schema property-name reserve failed");
        return;
    }

    std::uint64_t cursor = namesBase;
    for (std::uint32_t i = 0; i < maxCount; ++i) {
        UTF8STRING name;
        if (!r.asciiZ(cursor, segmentEnd, name)) {
            pushDiag(section, "datastorage-schema-name-unterminated",
                     "DataStorage schema property name is not NUL terminated",
                     0, false, cursor, true);
            break;
        }
        section.schemaPropertyNames.push_back(std::move(name));
        std::uint64_t next = 0;
        if (!checkedAdd(cursor,
                        static_cast<std::uint64_t>(
                            section.schemaPropertyNames.back().size())
                            + 1u,
                        next)
            || next > segmentEnd) {
            pushDiag(section, "datastorage-schema-name-offset-overflow",
                     "DataStorage schema property-name offset overflows",
                     0, false, cursor, true);
            break;
        }
        cursor = next;
    }
}

void readSchemaUnknownProperties(const ByteReader& r,
                                 DRW_DataStorageSection& section,
                                 std::uint64_t payloadStart,
                                 std::uint64_t firstSchemaOffset,
                                 std::uint64_t systemDataEnd,
                                 std::uint32_t segmentIndex) {
    std::uint64_t unknownEnd = systemDataEnd;
    if (firstSchemaOffset != std::numeric_limits<std::uint64_t>::max()) {
        if (firstSchemaOffset
            > systemDataEnd - std::min(payloadStart, systemDataEnd)) {
            pushDiag(section, "datastorage-schema-offset-out-of-bounds",
                     "DataStorage schema offset is outside its segment",
                     0, false, payloadStart, true);
            return;
        }
        unknownEnd = std::min(
            systemDataEnd, payloadStart + firstSchemaOffset);
    }
    if (unknownEnd < payloadStart)
        return;

    // The second schidx table identifies the opaque property headers by
    // segment and local offset. Prefer it over a scan so multiple schema-data
    // segments retain their identity and padding cannot become a property.
    bool usedReferences = false;
    std::unordered_set<std::uint32_t> seenOffsets;
    for (const DRW_DataStorageSchemaIndexEntry& reference
         : section.schemaPropertyEntries) {
        if (reference.segmentIndex != segmentIndex)
            continue;
        usedReferences = true;
        std::uint64_t offset = 0;
        if (!checkedAdd(payloadStart, reference.localOffset, offset)
            || !isRangeWithin(offset, 8, unknownEnd)
            || !r.has(offset, 8)) {
            pushDiag(section, "datastorage-schema-unknown-property-offset-invalid",
                     "DataStorage schema unknown-property reference is outside its segment",
                     0, false, offset, true);
            continue;
        }
        if (!seenOffsets.emplace(reference.localOffset).second) {
            pushDiag(section, "datastorage-duplicate-schema-unknown-property",
                     "DataStorage schema unknown-property reference is duplicated",
                     0, false, offset, true);
            continue;
        }
        DRW_DataStorageSchemaUnknownProperty property;
        property.index = reference.index;
        property.segmentIndex = segmentIndex;
        property.localOffset = reference.localOffset;
        property.dataSize = r.u32(offset);
        property.flags = r.u32(offset + 4);
        section.schemaUnknownProperties.push_back(property);
    }
    if (usedReferences) {
        section.schemaUnknownPropertyCount = static_cast<std::uint32_t>(
            section.schemaUnknownProperties.size());
        return;
    }

    // Older files may not populate the second table. Their contiguous
    // pre-schema headers remain readable through this bounded fallback.
    const std::uint64_t bytes = unknownEnd - payloadStart;
    if (bytes == 0)
        return;
    if (bytes % 8u != 0) {
        pushDiag(section, "datastorage-schema-unknown-property-truncated",
                 "DataStorage schema unknown-property area is not record aligned",
                 0, false, unknownEnd, true);
    }
    const std::uint64_t count64 = bytes / 8u;
    const std::uint32_t count = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        count64, HARD_MAX_ENTRIES));
    if (count64 > HARD_MAX_ENTRIES) {
        pushDiag(section, "datastorage-count-capped",
                 "DataStorage schema unknown-property count was capped");
    }
    if (!DRW::reserve(section.schemaUnknownProperties,
                      static_cast<int>(count))) {
        pushDiag(section, "datastorage-schema-unknown-property-reserve-failed",
                 "DataStorage schema unknown-property reserve failed");
        return;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint64_t delta = 0;
        std::uint64_t offset = 0;
        if (!checkedMultiply(i, 8u, delta)
            || !checkedAdd(payloadStart, delta, offset)
            || !isRangeWithin(offset, 8, unknownEnd)
            || !r.has(offset, 8)) {
            break;
        }
        DRW_DataStorageSchemaUnknownProperty property;
        property.index = i;
        property.segmentIndex = segmentIndex;
        property.localOffset = static_cast<std::uint32_t>(offset
                                                           - payloadStart);
        property.dataSize = r.u32(offset);
        property.flags = r.u32(offset + 4);
        if (property.dataSize == 0u && property.flags == 0u)
            break;
        section.schemaUnknownProperties.push_back(property);
    }
    section.schemaUnknownPropertyCount = static_cast<std::uint32_t>(
        section.schemaUnknownProperties.size());
}

bool readSchema(const ByteReader& r,
                DRW_DataStorageSection& section,
                const DRW_DataStorageSchemaIndexEntry& reference,
                std::uint64_t payloadStart,
                std::uint64_t schemaEnd,
                DRW_DataStorageSchema& schema) {
    std::uint64_t schemaOffset = 0;
    if (!checkedAdd(payloadStart, reference.localOffset, schemaOffset)
        || schemaOffset > schemaEnd) {
        pushDiag(section, "datastorage-schema-offset-out-of-bounds",
                 "DataStorage schema offset is outside its segment", 0, false,
                 schemaOffset, true);
        return false;
    }

    schema.index = reference.index;
    schema.segmentIndex = reference.segmentIndex;
    schema.localOffset = reference.localOffset;
    std::uint64_t cursor = schemaOffset;
    if (!isRangeWithin(cursor, 2, schemaEnd) || !r.has(cursor, 2)) {
        pushDiag(section, "datastorage-schema-truncated",
                 "DataStorage schema index count is outside its segment", 0,
                 false, cursor, true);
        return false;
    }
    const std::uint16_t indexCount = r.u16(cursor);
    cursor += 2;
    std::uint64_t indexBytes = 0;
    if (!checkedMultiply(indexCount, 8u, indexBytes)
        || !isRangeWithin(cursor, indexBytes, schemaEnd)
        || !r.has(cursor, indexBytes)) {
        pushDiag(section, "datastorage-schema-index-values-truncated",
                 "DataStorage schema indexes are outside their segment", 0,
                 false, cursor, true);
        return false;
    }
    if (!DRW::reserve(schema.indexes, static_cast<int>(indexCount))) {
        pushDiag(section, "datastorage-schema-index-values-reserve-failed",
                 "DataStorage schema index-value reserve failed");
        return false;
    }
    for (std::uint16_t i = 0; i < indexCount; ++i) {
        schema.indexes.push_back(r.u64(cursor));
        cursor += 8;
    }

    if (!isRangeWithin(cursor, 2, schemaEnd) || !r.has(cursor, 2)) {
        pushDiag(section, "datastorage-schema-truncated",
                 "DataStorage schema property count is outside its segment", 0,
                 false, cursor, true);
        return false;
    }
    const std::uint16_t propertyCount = r.u16(cursor);
    cursor += 2;
    if (!DRW::reserve(schema.properties,
                      static_cast<int>(propertyCount))) {
        pushDiag(section, "datastorage-schema-properties-reserve-failed",
                 "DataStorage schema property reserve failed");
        return false;
    }

    constexpr std::uint32_t typeSizes[SCHEMA_PROPERTY_TYPE_COUNT] = {
        0, 0, 2, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, 0, 0, 0};
    for (std::uint16_t i = 0; i < propertyCount; ++i) {
        if (!isRangeWithin(cursor, 8, schemaEnd)
            || !r.has(cursor, 8)) {
            pushDiag(section, "datastorage-schema-property-truncated",
                     "DataStorage schema property header is outside its segment",
                     0, false, cursor, true);
            return false;
        }
        DRW_DataStorageSchemaProperty property;
        property.flags = r.u32(cursor);
        property.nameIndex = r.u32(cursor + 4);
        cursor += 8;
        if (property.flags != 0 && property.flags != 1
            && property.flags != 2 && property.flags != 8) {
            pushDiag(section, "datastorage-schema-property-flags-invalid",
                     "DataStorage schema property flags are invalid", 0, false,
                     cursor - 8, true);
            return false;
        }

        if ((property.flags & 2u) == 0) {
            if (!isRangeWithin(cursor, 4, schemaEnd) || !r.has(cursor, 4)) {
                pushDiag(section, "datastorage-schema-property-truncated",
                         "DataStorage schema property type is outside its segment",
                         0, false, cursor, true);
                return false;
            }
            property.type = r.u32(cursor);
            cursor += 4;
            if (property.type >= SCHEMA_PROPERTY_TYPE_COUNT) {
                pushDiag(section, "datastorage-schema-property-type-invalid",
                         "DataStorage schema property type is outside 0..15",
                         0, false, cursor - 4, true);
                return false;
            }
            if (property.type == 0xeu) {
                if (!isRangeWithin(cursor, 4, schemaEnd)
                    || !r.has(cursor, 4)) {
                    pushDiag(section, "datastorage-schema-property-truncated",
                             "DataStorage custom schema property size is outside its segment",
                             0, false, cursor, true);
                    return false;
                }
                property.customTypeSize = r.u32(cursor);
                property.typeSize = property.customTypeSize;
                cursor += 4;
            } else {
                property.typeSize = typeSizes[property.type];
            }
        }

        if (property.flags == 1u || property.flags == 8u) {
            if (!isRangeWithin(cursor, 4, schemaEnd) || !r.has(cursor, 4)) {
                pushDiag(section, "datastorage-schema-property-truncated",
                         "DataStorage schema property unknown field is outside its segment",
                         0, false, cursor, true);
                return false;
            }
            if (property.flags == 1u)
                property.unknown1 = r.u32(cursor);
            else
                property.unknown2 = r.u32(cursor);
            cursor += 4;
        }

        if (!isRangeWithin(cursor, 2, schemaEnd) || !r.has(cursor, 2)) {
            pushDiag(section, "datastorage-schema-property-truncated",
                     "DataStorage schema property value count is outside its segment",
                     0, false, cursor, true);
            return false;
        }
        property.valueCount = r.u16(cursor);
        cursor += 2;
        std::uint64_t valueBytes = 0;
        if (!checkedMultiply(property.valueCount, property.typeSize,
                             valueBytes)
            || !isRangeWithin(cursor, valueBytes, schemaEnd)
            || !r.has(cursor, valueBytes)) {
            pushDiag(section, "datastorage-schema-property-values-truncated",
                     "DataStorage schema property values are outside their segment",
                     0, false, cursor, true);
            return false;
        }
        if (property.typeSize != 0
            && !DRW::reserve(property.values,
                             static_cast<int>(property.valueCount))) {
            pushDiag(section, "datastorage-schema-property-values-reserve-failed",
                     "DataStorage schema property-value reserve failed");
            return false;
        }
        for (std::uint16_t value = 0; value < property.valueCount; ++value) {
            std::vector<std::uint8_t> bytes = r.bytes(cursor,
                                                      property.typeSize);
            if (bytes.size() != property.typeSize) {
                pushDiag(section, "datastorage-schema-property-values-truncated",
                         "DataStorage schema property value copy failed", 0,
                         false, cursor, true);
                return false;
            }
            property.values.push_back(std::move(bytes));
            cursor += property.typeSize;
        }

        if (property.nameIndex < section.schemaPropertyNames.size()) {
            property.name = section.schemaPropertyNames[property.nameIndex];
        } else if (property.nameIndex != SCHEMA_PROPERTY_NO_NAME) {
            pushDiag(section, "datastorage-schema-name-index-invalid",
                     "DataStorage schema property name index is outside the name table",
                     0, false, cursor, true);
        }
        schema.properties.push_back(std::move(property));
    }
    // Zero-filled reserved space is common after the opaque schdat property
    // area. Do not expose it as a valid empty schema merely because its first
    // two UInt16 fields happen to decode as zero.
    if (schema.indexes.empty() && schema.properties.empty())
        return false;
    return true;
}

void readSchemaData(const ByteReader& r, DRW_DataStorageSection& section) {
    if (section.schemaIndexEntries.empty())
        return;

    std::vector<std::uint32_t> schemaSegmentIndexes;
    std::unordered_set<std::uint32_t> seenSegments;
    for (const DRW_DataStorageSchemaIndexEntry& reference
         : section.schemaIndexEntries) {
        if (static_cast<std::size_t>(reference.segmentIndex)
            >= section.segments.size()) {
            pushDiag(section, "datastorage-schema-segment-missing",
                     "DataStorage schema reference names a missing segment",
                     0, false, reference.localOffset, true);
            continue;
        }
        const DRW_DataStorageSegment& segment =
            section.segments[reference.segmentIndex];
        if (segment.kind != DRW_DataStorageSegmentKind::SchemaData) {
            pushDiag(section, "datastorage-schema-segment-kind-invalid",
                     "DataStorage schema reference does not name a schema-data segment",
                     0, false, segment.offset, true);
            continue;
        }
        if (seenSegments.emplace(reference.segmentIndex).second)
            schemaSegmentIndexes.push_back(reference.segmentIndex);
    }

    for (const std::uint32_t segmentIndex : schemaSegmentIndexes) {
        const DRW_DataStorageSegment& segment = section.segments[segmentIndex];
        std::uint64_t payloadStart = 0;
        std::uint64_t namesStart = 0;
        std::uint64_t segmentEnd = 0;
        if (!getSystemDataBounds(r, segment, payloadStart, namesStart,
                                 segmentEnd, section)) {
            continue;
        }

        const std::uint64_t namesOffset = segment.systemDataAlignmentOffset
            == 0 ? segmentEnd : namesStart;
        readSchemaNames(r, section, segment, namesOffset, segmentEnd);

        std::uint64_t firstSchemaOffset =
            std::numeric_limits<std::uint64_t>::max();
        for (const DRW_DataStorageSchemaIndexEntry& reference
             : section.schemaIndexEntries) {
            if (reference.segmentIndex == segmentIndex)
                firstSchemaOffset = std::min<std::uint64_t>(
                    firstSchemaOffset, reference.localOffset);
        }
        readSchemaUnknownProperties(r, section, payloadStart,
                                    firstSchemaOffset, namesStart,
                                    segmentIndex);

        std::unordered_set<std::uint64_t> seenSchemas;
        for (const DRW_DataStorageSchemaIndexEntry& reference
             : section.schemaIndexEntries) {
            if (reference.segmentIndex != segmentIndex)
                continue;
            const std::uint64_t key =
                (static_cast<std::uint64_t>(reference.index) << 32)
                | reference.localOffset;
            if (!seenSchemas.emplace(key).second)
                continue;
            DRW_DataStorageSchema schema;
            if (readSchema(r, section, reference, payloadStart, namesStart,
                           schema)) {
                section.schemas.push_back(std::move(schema));
            }
        }
    }
    section.schemaCount = static_cast<std::uint32_t>(section.schemas.size());
}

std::uint32_t countPackedRecordHeaders(const ByteReader& r,
                                       std::uint64_t segmentDataOffset,
                                       std::uint64_t segmentEndOffset) {
    std::uint32_t n = 0;
    for (;;) {
        std::uint64_t headerOffset = 0;
        std::uint64_t slot = 0;
        if (!checkedMultiply(n, DATA_RECORD_HEADER_SIZE, headerOffset)
            || !checkedAdd(segmentDataOffset, headerOffset, slot)
            || !isRangeWithin(slot, DATA_RECORD_HEADER_SIZE,
                              segmentEndOffset))
            break;
        if (!r.has(slot, 4) || r.u32(slot) != DATA_RECORD_HEADER_SIZE)
            break;
        ++n;
        if (n > HARD_MAX_ENTRIES)
            break;
    }
    return n;
}

void readDataRecords(const ByteReader& r,
                     DRW_DataStorageSection& section,
                     bool retainPayloads) {
    std::unordered_map<std::uint64_t, std::uint64_t> dataRegionBaseByOffset;
    const std::uint32_t maxRecords =
        capCount(static_cast<std::uint32_t>(section.dataIndexEntries.size()),
                 r.length(), DATA_RECORD_HEADER_SIZE, section, "records");

    if (!DRW::reserve(section.records, static_cast<int>(maxRecords))) {
        pushDiag(section, "datastorage-records-reserve-failed",
                 "DataStorage records reserve failed");
        return;
    }

    std::uint32_t produced = 0;
    for (const DRW_DataStorageIndexEntry& entry : section.dataIndexEntries) {
        if (produced >= maxRecords)
            break;
        if (static_cast<std::size_t>(entry.segmentIndex) >= section.segments.size()) {
            pushDiag(section, "datastorage-record-segment-missing",
                     "DataStorage record references missing segment "
                         + std::to_string(entry.segmentIndex));
            continue;
        }
        const DRW_DataStorageSegment& segment =
            section.segments[entry.segmentIndex];
        if (!isDataSegment(segment))
            continue;

        std::uint64_t segmentDataOffset = 0;
        std::uint64_t segmentEndOffset = 0;
        std::uint64_t recordHeaderOffset = 0;
        if (!checkedAdd(segment.offset, SEGMENT_HEADER_SIZE,
                        segmentDataOffset)
            || !checkedAdd(segment.offset, segment.size, segmentEndOffset)
            || !checkedAdd(segmentDataOffset, entry.localOffset,
                           recordHeaderOffset)
            || !isRangeWithin(recordHeaderOffset, DATA_RECORD_HEADER_SIZE,
                              segmentEndOffset)
            || !r.has(recordHeaderOffset, DATA_RECORD_HEADER_SIZE)) {
            pushDiag(section, "datastorage-record-header-truncated",
                     "DataStorage record header at local offset "
                         + std::to_string(entry.localOffset)
                         + " is outside the referenced segment",
                     0, false, recordHeaderOffset, true);
            continue;
        }

        const std::uint32_t entrySize = r.u32(recordHeaderOffset);
        const std::uint64_t handle = r.u64(recordHeaderOffset + 8);
        const std::uint32_t recordLocalOffset = r.u32(recordHeaderOffset + 16);

        std::uint64_t dataRegionBase = 0;
        const auto cached = dataRegionBaseByOffset.find(segment.offset);
        if (cached != dataRegionBaseByOffset.end()) {
            dataRegionBase = cached->second;
        } else {
            const std::uint32_t headerCount = countPackedRecordHeaders(
                r, segmentDataOffset, segmentEndOffset);
            std::uint64_t headerBytes = 0;
            std::uint64_t unalignedDataRegionBase = 0;
            if (!checkedMultiply(headerCount, DATA_RECORD_HEADER_SIZE,
                                 headerBytes)
                || !checkedAdd(segmentDataOffset, headerBytes,
                               unalignedDataRegionBase)
                || !roundUpTo16(unalignedDataRegionBase, dataRegionBase)) {
                pushDiag(section, "datastorage-record-offset-overflow",
                         "DataStorage record data-region offset overflows",
                         0, false, segment.offset, true);
                continue;
            }
            dataRegionBaseByOffset.emplace(segment.offset, dataRegionBase);
        }

        std::uint64_t recordOffset = 0;
        if (entrySize < DATA_RECORD_HEADER_SIZE
            || !checkedAdd(dataRegionBase, recordLocalOffset, recordOffset)
            || !isRangeWithin(recordOffset, 4, segmentEndOffset)
            || !r.has(recordOffset, 4)) {
            pushDiag(section, "datastorage-record-entry-invalid",
                     "DataStorage record entry for handle " + handleKey(handle)
                         + " has an invalid header or payload offset",
                     handle, true, recordHeaderOffset, true);
            continue;
        }

        const std::uint32_t dataSize = r.u32(recordOffset);
        std::uint64_t dataOffset = 0;
        if (!checkedAdd(recordOffset, 4, dataOffset)) {
            pushDiag(section, "datastorage-record-offset-overflow",
                     "DataStorage record payload offset overflows", handle,
                     true, recordOffset, true);
            continue;
        }

        DRW_DataStorageRecord record;
        record.handle = handle;
        record.handleKey = handleKey(handle);
        record.isHandleSafe = handle <= 9007199254740991ull;
        record.segmentIndex = entry.segmentIndex;
        record.localOffset = entry.localOffset;
        record.schemaIndex = entry.schemaIndex;
        record.entrySize = entrySize;
        record.recordLocalOffset = recordLocalOffset;
        record.recordOffset = recordOffset;
        record.dataOffset = dataOffset;
        if (dataSize == DATA_BLOB_REFERENCE_MARKER) {
            // The marker is not a 0xbb106bb1-byte payload. It introduces a
            // fixed-size blob descriptor followed by page references; the
            // Actual bytes live in blob01 segments. Keep exact section replay
            // authoritative, while exposing a validated payload when it fits
            // the normal retention cap.
            if (!isRangeWithin(dataOffset, DATA_BLOB_REFERENCE_FIXED_SIZE,
                               segmentEndOffset)
                || !r.has(dataOffset, DATA_BLOB_REFERENCE_FIXED_SIZE)) {
                pushDiag(section, "datastorage-blob-reference-truncated",
                         "DataStorage blob reference header for handle "
                             + handleKey(handle) + " is outside its segment",
                         handle, true, dataOffset, true);
                continue;
            }

            const std::uint64_t totalByteLength = r.u64(dataOffset);
            const std::uint32_t pageCount = r.u32(dataOffset + 8);
            const std::uint32_t blobRecordByteLength = r.u32(dataOffset + 12);
            const std::uint32_t pageByteSize = r.u32(dataOffset + 16);
            const std::uint32_t lastPageByteSize = r.u32(dataOffset + 20);
            const std::uint32_t unknown1 = r.u32(dataOffset + 24);
            const std::uint32_t unknown2 = r.u32(dataOffset + 28);

            std::uint64_t pageEntriesSize = 0;
            std::uint64_t descriptorSize = 0;
            if (pageCount > HARD_MAX_ENTRIES
                || !checkedMultiply(pageCount, DATA_BLOB_PAGE_ENTRY_SIZE,
                                    pageEntriesSize)
                || !checkedAdd(DATA_BLOB_REFERENCE_FIXED_SIZE, pageEntriesSize,
                               descriptorSize)
                || !isRangeWithin(dataOffset, descriptorSize,
                                  segmentEndOffset)
                || !r.has(dataOffset, descriptorSize)
                || (blobRecordByteLength != 0
                    && blobRecordByteLength < descriptorSize)) {
                pushDiag(section, "datastorage-blob-reference-invalid",
                         "DataStorage blob reference for handle "
                             + handleKey(handle) + " has invalid bounds",
                         handle, true, dataOffset, true);
                continue;
            }

            record.isBlobReference = true;
            record.blobTotalByteLength = totalByteLength;
            record.blobPageCount = pageCount;
            record.blobRecordByteLength = blobRecordByteLength;
            record.blobPageByteSize = pageByteSize;
            record.blobLastPageByteSize = lastPageByteSize;
            record.blobUnknown1 = unknown1;
            record.blobUnknown2 = unknown2;
            if (totalByteLength
                <= static_cast<std::uint64_t>(
                    std::numeric_limits<std::uint32_t>::max())) {
                record.dataByteLength =
                    static_cast<std::uint32_t>(totalByteLength);
            }
            if (!DRW::reserve(record.blobPages, static_cast<int>(pageCount))) {
                pushDiag(section, "datastorage-blob-pages-reserve-failed",
                         "DataStorage blob page reference reserve failed",
                         handle, true, dataOffset, true);
                continue;
            }
            bool pageReferenceValid = true;
            std::uint64_t assembledByteLength = 0;
            std::vector<std::uint64_t> pageDataOffsets;
            if (!DRW::reserve(pageDataOffsets, static_cast<int>(pageCount))) {
                pushDiag(section, "datastorage-blob-pages-reserve-failed",
                         "DataStorage blob page offset reserve failed",
                         handle, true, dataOffset, true);
                continue;
            }
            for (std::uint32_t page = 0; page < pageCount; ++page) {
                const std::uint64_t pageOffset = dataOffset
                    + DATA_BLOB_REFERENCE_FIXED_SIZE
                    + static_cast<std::uint64_t>(page)
                        * DATA_BLOB_PAGE_ENTRY_SIZE;
                const std::uint32_t segmentIndex = r.u32(pageOffset);
                const std::uint32_t byteLength = r.u32(pageOffset + 4);
                if (static_cast<std::size_t>(segmentIndex)
                        >= section.segments.size()
                    || section.segments[segmentIndex].kind
                           != DRW_DataStorageSegmentKind::Blob) {
                    pageReferenceValid = false;
                    break;
                }
                record.blobPages.push_back({segmentIndex, byteLength});

                const DRW_DataStorageSegment& pageSegment =
                    section.segments[segmentIndex];
                std::uint64_t pageSegmentEnd = 0;
                std::uint64_t pageHeaderOffset = 0;
                if (!checkedAdd(pageSegment.offset, pageSegment.size,
                                pageSegmentEnd)
                    || !checkedAdd(pageSegment.offset, SEGMENT_HEADER_SIZE,
                                   pageHeaderOffset)
                    || !isRangeWithin(pageHeaderOffset,
                                      DATA_BLOB_PAGE_HEADER_SIZE,
                                      pageSegmentEnd)
                    || !r.has(pageHeaderOffset,
                             DATA_BLOB_PAGE_HEADER_SIZE)) {
                    pageReferenceValid = false;
                    break;
                }

                const std::uint64_t pageTotalByteLength =
                    r.u64(pageHeaderOffset);
                const std::uint64_t pageStartOffset =
                    r.u64(pageHeaderOffset + 8);
                const std::int32_t pageIndex =
                    r.i32(pageHeaderOffset + 16);
                const std::int32_t pageCountInHeader =
                    r.i32(pageHeaderOffset + 20);
                const std::uint64_t pageDataByteLength =
                    r.u64(pageHeaderOffset + 24);
                const std::uint32_t expectedPageByteLength =
                    page + 1u == pageCount ? lastPageByteSize : pageByteSize;
                std::uint64_t pageDataOffset = 0;
                if (pageTotalByteLength != totalByteLength
                    || pageIndex != static_cast<std::int32_t>(page)
                    || pageCountInHeader
                           != static_cast<std::int32_t>(pageCount)
                    || pageStartOffset != assembledByteLength
                    || pageDataByteLength != byteLength
                    || byteLength != expectedPageByteLength
                    || pageDataByteLength
                           > std::numeric_limits<std::uint32_t>::max()
                    || !checkedAdd(pageHeaderOffset,
                                   DATA_BLOB_PAGE_HEADER_SIZE,
                                   pageDataOffset)
                    || !isRangeWithin(pageDataOffset, pageDataByteLength,
                                      pageSegmentEnd)
                    || !r.has(pageDataOffset, pageDataByteLength)
                    || !checkedAdd(assembledByteLength, pageDataByteLength,
                                   assembledByteLength)) {
                    pageReferenceValid = false;
                    break;
                }
                pageDataOffsets.push_back(pageDataOffset);
            }
            if (!pageReferenceValid
                || assembledByteLength != totalByteLength) {
                pushDiag(section, "datastorage-blob-page-segment-invalid",
                         "DataStorage blob reference for handle "
                             + handleKey(handle)
                             + " has invalid page metadata",
                         handle, true, dataOffset, true);
                continue;
            }

            if (retainPayloads
                && totalByteLength <= PAYLOAD_BLOB_SECTION_CAP
                && totalByteLength
                       <= static_cast<std::uint64_t>(
                           std::numeric_limits<std::size_t>::max())) {
                if (!DRW::reserve(record.payload, static_cast<int>(
                                                     totalByteLength))) {
                    pushDiag(section, "datastorage-payload-reserve-failed",
                             "DataStorage blob payload retention failed",
                             handle, true, dataOffset, true);
                } else {
                    bool payloadValid = true;
                    try {
                        for (std::size_t pageIndex = 0;
                             pageIndex < pageDataOffsets.size(); ++pageIndex) {
                            const std::uint64_t pageDataOffset =
                                pageDataOffsets[pageIndex];
                            const std::uint64_t pageLength =
                                record.blobPages[pageIndex].byteLength;
                            const std::vector<std::uint8_t> bytes =
                                r.bytes(pageDataOffset, pageLength);
                            if (pageLength != 0 && bytes.empty()) {
                                payloadValid = false;
                                break;
                            }
                            record.payload.insert(record.payload.end(),
                                                  bytes.begin(), bytes.end());
                        }
                        if (payloadValid) {
                            record.blobPayloadRetained = true;
                        } else {
                            record.payload.clear();
                            pushDiag(section,
                                     "datastorage-payload-retain-failed",
                                     "DataStorage blob payload retention failed",
                                     handle, true, dataOffset, true);
                        }
                    } catch (const std::exception&) {
                        record.payload.clear();
                        pushDiag(section, "datastorage-payload-retain-failed",
                                 "DataStorage blob payload retention failed",
                                 handle, true, dataOffset, true);
                    } catch (...) {
                        record.payload.clear();
                        pushDiag(section, "datastorage-payload-retain-failed",
                                 "DataStorage blob payload retention failed",
                                 handle, true, dataOffset, true);
                    }
                }
            }
        } else {
            if (!r.has(dataOffset, dataSize)) {
                pushDiag(section, "datastorage-record-payload-truncated",
                         "DataStorage payload for handle " + handleKey(handle)
                             + " is outside the section bounds",
                         handle, true, dataOffset, true);
                continue;
            }
            if (!isRangeWithin(dataOffset, dataSize, segmentEndOffset)) {
                pushDiag(section, "datastorage-record-payload-out-of-segment",
                         "DataStorage payload for handle " + handleKey(handle)
                             + " extends beyond its segment bounds",
                         handle, true, dataOffset, true);
                continue;
            }
            record.dataByteLength = dataSize;
            if (retainPayloads) {
                record.payload = r.bytes(dataOffset, dataSize);
                if (dataSize != 0 && record.payload.empty()) {
                    pushDiag(section, "datastorage-payload-retain-failed",
                             "DataStorage payload retention failed",
                             handle, true, dataOffset, true);
                    continue;
                }
            }
        }
        if (record.payload.empty() == false) {
            std::uint32_t markerOffset = 0;
            if (findAcisBinaryFileMarker(record.payload, markerOffset)) {
                record.hasPayloadMarker = true;
                record.payloadMarkerOffset = markerOffset;
                record.payloadMarkerLength =
                    static_cast<std::uint32_t>(sizeof(kAcisBinaryFilePrefix) - 1u);
                record.payloadMarkerSection = "datastore";
            }
        }

        section.records.push_back(std::move(record));
        ++produced;
    }
}

bool preferredRecord(const DRW_DataStorageSection& section,
                     std::size_t candidateIndex,
                     std::size_t currentIndex) {
    const DRW_DataStorageRecord& candidate = section.records[candidateIndex];
    const DRW_DataStorageRecord& current = section.records[currentIndex];
    const DRW_DataStorageSegment& candidateSegment =
        section.segments[candidate.segmentIndex];
    const DRW_DataStorageSegment& currentSegment =
        section.segments[current.segmentIndex];

    if (candidateSegment.hasRevision != currentSegment.hasRevision)
        return candidateSegment.hasRevision;
    if (candidateSegment.revision != currentSegment.revision)
        return candidateSegment.revision > currentSegment.revision;
    if (candidate.dataOffset != current.dataOffset)
        return candidate.dataOffset > current.dataOffset;
    return candidate.segmentIndex > current.segmentIndex;
}

bool buildRecordIndex(DRW_DataStorageSection& section) {
    try {
    section.recordIndexByHandle.clear();
    section.recordIndexByHandleKey.clear();
    section.recordIndexesByHandle.clear();
    section.recordIndexesByHandleKey.clear();
    section.duplicateRecordHandleKeys.clear();
    if (section.records.size()
            > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(section.recordIndexByHandle,
                         static_cast<int>(section.records.size()))
        || !DRW::reserve(section.recordIndexByHandleKey,
                         static_cast<int>(section.records.size()))
        || !DRW::reserve(section.recordIndexesByHandle,
                         static_cast<int>(section.records.size()))
        || !DRW::reserve(section.recordIndexesByHandleKey,
                         static_cast<int>(section.records.size()))) {
        return false;
    }
    std::unordered_set<UTF8STRING> diagnosedDuplicates;
    if (!DRW::reserve(diagnosedDuplicates,
                      static_cast<int>(section.records.size())))
        return false;
    for (std::size_t i = 0; i < section.records.size(); ++i) {
        const std::uint64_t handle = section.records[i].handle;
        const UTF8STRING key = section.records[i].handleKey.empty()
            ? handleKey(handle) : section.records[i].handleKey;
        section.recordIndexesByHandle[handle].push_back(i);
        section.recordIndexesByHandleKey[key].push_back(i);
        const auto [numericIt, numericInserted] =
            section.recordIndexByHandle.emplace(handle, i);
        const auto [keyIt, keyInserted] =
            section.recordIndexByHandleKey.emplace(key, i);
        if (numericInserted && keyInserted)
            continue;

        if (!numericInserted && preferredRecord(section, i, numericIt->second))
            numericIt->second = i;
        if (!keyInserted && preferredRecord(section, i, keyIt->second))
            keyIt->second = i;

        if (diagnosedDuplicates.emplace(key).second) {
            section.duplicateRecordHandleKeys.push_back(key);
            const std::size_t selected = section.recordIndexByHandleKey.at(key);
            pushDiag(section,
                     "datastorage-duplicate-record-handle",
                     "DataStorage contains multiple records for handle "
                         + key + " (" + std::to_string(
                             section.recordIndexesByHandleKey.at(key).size())
                         + " records); selected record "
                         + std::to_string(selected),
                     handle, true,
                     section.records[selected].recordOffset,
                     true);
        }
    }
    return true;
    } catch (...) {
        section.recordIndexByHandle.clear();
        section.recordIndexByHandleKey.clear();
        section.recordIndexesByHandle.clear();
        section.recordIndexesByHandleKey.clear();
        section.duplicateRecordHandleKeys.clear();
        return false;
    }
}

} // namespace

DRW_DataStorageSection DRW_parseDataStorage(const std::uint8_t* data,
                                            std::size_t size,
                                            DRW::Version version) {
    DRW_DataStorageSection section;
    section.m_version = version;
    section.sectionByteLength = size;

    if (data == nullptr || size < HEADER_SIZE) {
        section.parseFailed = true;
        pushDiag(section, "datastorage-section-too-small",
                 "DataStorage section is too small");
        return section;
    }

    ByteReader r(data, size);
    section.signature = r.u32(0);
    section.headerSize = r.i32(4);
    section.version = r.u32(12);
    section.revision = r.u32(20);
    section.segmentIndexOffset = r.u32(24);
    section.segmentIndexEntryCount = r.u32(32);
    section.schemaIndexSegmentIndex = r.i32(36);
    section.dataIndexSegmentIndex = r.i32(40);
    section.searchSegmentIndex = r.i32(44);
    section.previousSaveIndex = r.i32(48);
    section.fileSize = r.u32(52);

    // ODA documents these as RL fields but does not assign stable values to
    // either one. Traversal starts from the fixed, bounded 56-byte header and
    // the explicit segment-index offset, so treating producer-specific values
    // as a structural failure would reject valid DataStorage payloads.
    if (static_cast<std::size_t>(section.fileSize) != size) {
        pushDiag(section, "datastorage-file-size-mismatch",
                 "DataStorage header file size " + std::to_string(section.fileSize)
                     + " does not match section byte length "
                     + std::to_string(size));
    }
    if (section.segmentIndexOffset < HEADER_SIZE
        || section.segmentIndexOffset > size) {
        pushDiag(section, "datastorage-segment-index-offset-invalid",
                 "DataStorage segment index offset is outside the section",
                 0, false, section.segmentIndexOffset, true);
    }
    const auto validSegmentIndex = [&](std::int32_t index) {
        return index == -1
            || (index >= 0
                && static_cast<std::uint32_t>(index)
                       < section.segmentIndexEntryCount);
    };
    if (!validSegmentIndex(section.schemaIndexSegmentIndex)
        || !validSegmentIndex(section.dataIndexSegmentIndex)
        || !validSegmentIndex(section.searchSegmentIndex)
        || !validSegmentIndex(section.previousSaveIndex)) {
        pushDiag(section, "datastorage-segment-index-reference-invalid",
                 "DataStorage header references a segment outside its index",
                 0, false, 36, true);
    }

    section.payloadsRetained = size <= PAYLOAD_BLOB_SECTION_CAP;

    readSegmentIndex(r, section.segmentIndexOffset,
                     section.segmentIndexEntryCount, section);
    readSchemaIndex(r, section);
    readSchemaData(r, section);
    readDataIndex(r, section);
    readDataRecords(r, section, section.payloadsRetained);
    if (!buildRecordIndex(section))
        section.parseFailed = true;
    return section;
}
