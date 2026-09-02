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

#ifndef DRW_DATASTORAGE_H
#define DRW_DATASTORAGE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "drw_base.h"

//! Constants for AcDb:AcDsPrototype_1b (DataStorage) binary layout.
//! Ported from dwg-parser DwgDataStorageReader (TS + C++).
namespace DRW_DataStorageConst {
constexpr std::uint32_t HEADER_SIZE = 56;
constexpr std::uint32_t SEGMENT_INDEX_ENTRY_SIZE = 12;
constexpr std::uint32_t SEGMENT_HEADER_SIZE = 48;
constexpr std::uint16_t SEGMENT_SIGNATURE = 0xd5acu;
constexpr std::uint32_t SEGMENT_SIZE_ALIGNMENT = 0x40u;
constexpr std::uint32_t DATA_INDEX_ENTRY_SIZE = 12;
constexpr std::uint32_t DATA_RECORD_HEADER_SIZE = 20;
//! Marker used instead of a byte count for a multi-segment blob reference.
constexpr std::uint32_t DATA_BLOB_REFERENCE_MARKER = 0xbb106bb1u;
constexpr std::uint32_t DATA_BLOB_REFERENCE_FIXED_SIZE = 32;
constexpr std::uint32_t DATA_BLOB_PAGE_ENTRY_SIZE = 8;
constexpr std::uint32_t DATA_BLOB_PAGE_HEADER_SIZE = 32;
constexpr std::uint32_t SCHEMA_INDEX_ENTRY_SIZE = 12;
constexpr std::uint32_t SCHEMA_PROPERTY_TYPE_COUNT = 16;
constexpr std::uint32_t SCHEMA_PROPERTY_NO_NAME = 0xffffffffu;
//! Hard ceiling on declared index counts (malicious/absurd inputs).
constexpr std::uint32_t HARD_MAX_ENTRIES = 1000000u;
//! Retain record payload bytes only when the full section is within this size.
constexpr std::size_t PAYLOAD_BLOB_SECTION_CAP = 8u * 1024u * 1024u;
} // namespace DRW_DataStorageConst

enum class DRW_DataStorageSegmentKind {
    Unknown = 0,
    SegmentIndex,
    DataIndex,
    Data,
    SchemaIndex,
    SchemaData,
    Search,
    Blob,
    PreviousSave,
    FreeSpace,
    Schema
};

struct DRW_DataStorageDiagnostic {
    enum class Kind {
        Informational,
        SupportedOpaque,
        StructuralInvalid
    };

    std::string code;
    std::string message;
    std::uint64_t handle = 0;
    std::uint64_t offset = 0;
    bool hasHandle = false;
    bool hasOffset = false;
    Kind kind = Kind::StructuralInvalid;
};

struct DRW_DataStorageSegment {
    std::uint32_t index = 0;
    std::uint64_t offset = 0;
    std::uint32_t size = 0;
    std::uint64_t payloadOffset = 0;
    std::uint32_t payloadByteLength = 0;
    std::uint16_t signature = 0;
    UTF8STRING name;
    std::int32_t segmentIndex = -1;
    //! Segment size copied from the segment header when available.
    std::uint32_t headerSegmentSize = 0;
    bool hasHeaderSegmentSize = false;
    std::uint32_t revision = 0;
    bool hasRevision = false;
    std::uint32_t systemDataAlignmentOffset = 0;
    std::uint32_t objectDataAlignmentOffset = 0;
    DRW_DataStorageSegmentKind kind = DRW_DataStorageSegmentKind::Unknown;
    bool isKnownKind = false;
};

struct DRW_DataStorageIndexEntry {
    std::uint32_t segmentIndex = 0;
    std::uint32_t localOffset = 0;
    std::uint32_t schemaIndex = 0;
};

//! One reference from the DataStorage schema index to schema data.
struct DRW_DataStorageSchemaIndexEntry {
    std::uint32_t index = 0;
    std::uint32_t segmentIndex = 0;
    std::uint32_t localOffset = 0;
};

//! Header for an opaque schema-data property that is not a schema.
struct DRW_DataStorageSchemaUnknownProperty {
    std::uint32_t index = 0;
    std::uint32_t segmentIndex = 0;
    std::uint32_t localOffset = 0;
    std::uint32_t dataSize = 0;
    std::uint32_t flags = 0;
};

//! A typed schema property. Values remain byte arrays because the DWG
//! specification defines the scalar type but not a LibreCAD consumer model.
struct DRW_DataStorageSchemaProperty {
    std::uint32_t flags = 0;
    std::uint32_t nameIndex = DRW_DataStorageConst::SCHEMA_PROPERTY_NO_NAME;
    std::uint32_t type = 0;
    std::uint32_t customTypeSize = 0;
    std::uint32_t typeSize = 0;
    std::uint32_t unknown1 = 0;
    std::uint32_t unknown2 = 0;
    std::uint16_t valueCount = 0;
    UTF8STRING name;
    std::vector<std::vector<std::uint8_t>> values;
};

//! One schema from the schdat segment, indexed by the schidx table.
struct DRW_DataStorageSchema {
    std::uint32_t index = 0;
    std::uint32_t segmentIndex = 0;
    std::uint32_t localOffset = 0;
    std::vector<std::uint64_t> indexes;
    std::vector<DRW_DataStorageSchemaProperty> properties;
};

//! One page reference in an AC1027+ DataStorage blob record.
struct DRW_DataStorageBlobPage {
    std::uint32_t segmentIndex = 0;
    std::uint32_t byteLength = 0;
};

struct DRW_DataStorageRecord {
    std::uint64_t handle = 0;
    UTF8STRING handleKey;
    //! False only when the numeric value is unsafe for IEEE-754 consumers;
    //! it does not make the DWG record structurally invalid.
    bool isHandleSafe = true;
    std::uint32_t segmentIndex = 0;
    std::uint32_t localOffset = 0;
    std::uint32_t schemaIndex = 0;
    std::uint32_t entrySize = 0;
    std::uint32_t recordLocalOffset = 0;
    std::uint64_t recordOffset = 0;
    std::uint64_t dataOffset = 0;
    std::uint32_t dataByteLength = 0;
    //! True when the record uses the specification's multi-segment blob form.
    bool isBlobReference = false;
    std::uint64_t blobTotalByteLength = 0;
    std::uint32_t blobPageCount = 0;
    std::uint32_t blobRecordByteLength = 0;
    std::uint32_t blobPageByteSize = 0;
    std::uint32_t blobLastPageByteSize = 0;
    std::uint32_t blobUnknown1 = 0;
    std::uint32_t blobUnknown2 = 0;
    std::vector<DRW_DataStorageBlobPage> blobPages;
    //! True when all referenced blob pages passed validation and were copied.
    bool blobPayloadRetained = false;
    //! Optional payload copy (empty when section exceeds blob retention cap).
    std::vector<std::uint8_t> payload;
    //! ACIS BinaryFile marker in the retained payload, if present.
    bool hasPayloadMarker = false;
    std::uint32_t payloadMarkerOffset = 0;
    std::uint32_t payloadMarkerLength = 0;
    UTF8STRING payloadMarkerSection;
};

//! Typed index of AcDb:AcDsPrototype_1b with optional object linkage.
struct DRW_DataStorageSection {
    UTF8STRING m_name = "AcDb:AcDsPrototype_1b";
    DRW::Version m_version = DRW::UNKNOWNV;
    std::uint32_t signature = 0;
    std::int32_t headerSize = 0;
    std::uint32_t version = 0;
    std::uint32_t revision = 0;
    std::uint32_t segmentIndexOffset = 0;
    std::uint32_t segmentIndexEntryCount = 0;
    std::int32_t schemaIndexSegmentIndex = -1;
    std::int32_t dataIndexSegmentIndex = -1;
    std::int32_t searchSegmentIndex = -1;
    std::int32_t previousSaveIndex = -1;
    std::uint32_t fileSize = 0;
    std::size_t sectionByteLength = 0;
    bool payloadsRetained = false;
    std::vector<DRW_DataStorageSegment> segments;
    std::vector<DRW_DataStorageIndexEntry> dataIndexEntries;
    //! The first and second reference tables from the schidx segment.
    std::uint32_t schemaIndexUnknownPropertyCount = 0;
    std::uint32_t schemaIndexUnknown1 = 0;
    std::uint64_t schemaIndexTag = 0;
    std::uint32_t schemaIndexPropertyEntryCount = 0;
    std::uint32_t schemaIndexUnknown2 = 0;
    std::vector<DRW_DataStorageSchemaIndexEntry> schemaIndexEntries;
    std::vector<DRW_DataStorageSchemaIndexEntry> schemaPropertyEntries;
    //! Unknown-property headers and schemas decoded from schdat.
    std::uint32_t schemaUnknownPropertyCount = 0;
    std::uint32_t schemaCount = 0;
    std::uint32_t schemaPropertyNameCount = 0;
    std::vector<DRW_DataStorageSchemaUnknownProperty>
        schemaUnknownProperties;
    std::vector<DRW_DataStorageSchema> schemas;
    std::vector<UTF8STRING> schemaPropertyNames;
    std::vector<DRW_DataStorageRecord> records;
    //! Preferred record for each referenced object handle. Duplicate records
    //! remain in `records`; this index selects the newest segment revision,
    //! then the latest payload offset, deterministically.
    std::unordered_map<std::uint64_t, std::size_t> recordIndexByHandle;
    //! The same preferred-record index keyed by the canonical uppercase
    //! hexadecimal handle text. This preserves lookup identity when a
    //! consumer cannot safely narrow a DWG handle to a machine integer.
    std::unordered_map<UTF8STRING, std::size_t> recordIndexByHandleKey;
    //! All source-order record indexes grouped by numeric handle. The
    //! preferred maps above are compatibility lookups; these maps preserve
    //! duplicate records for revision-history and audit consumers, matching
    //! dwg-parser's recordsByHandleAll contract.
    std::unordered_map<std::uint64_t, std::vector<std::size_t>>
        recordIndexesByHandle;
    //! All source-order record indexes grouped by canonical handle text.
    std::unordered_map<UTF8STRING, std::vector<std::size_t>>
        recordIndexesByHandleKey;
    //! Canonical keys for which more than one record was retained.
    std::vector<UTF8STRING> duplicateRecordHandleKeys;
    //! Number of records whose handle was not referenced by a parsed
    //! AC1027+ modeler/surface entity. Populated after object traversal.
    std::size_t orphanRecordCount = 0;
    std::vector<DRW_DataStorageDiagnostic> diagnostics;
    //! True when the input was shorter than HEADER_SIZE or otherwise unusable.
    bool parseFailed = false;
    //! False when a diagnostic proves the typed index is unsafe as evidence
    //! for opaque section replay.
    bool structurallyValid = true;
    //! Tolerant parsing may retain bounded producer-variant data. Explicitly
    //! tolerated SupportedOpaque diagnostics keep replay enabled; structural
    //! diagnostics turn it off.
    bool replayAllowed = true;

    bool hasStructuralDiagnostics() const noexcept {
        return !structurallyValid;
    }

    //! Resolve the preferred payload record for an object handle. The
    //! returned pointer is valid while this section is not modified.
    const DRW_DataStorageRecord* findRecordByHandle(
        std::uint64_t handle) const noexcept {
        const auto it = recordIndexByHandle.find(handle);
        if (it == recordIndexByHandle.end() || it->second >= records.size())
            return nullptr;
        return &records[it->second];
    }

    //! Resolve the preferred payload record by canonical handle text.
    const DRW_DataStorageRecord* findRecordByHandleKey(
        const UTF8STRING& handleKey) const noexcept {
        const auto it = recordIndexByHandleKey.find(handleKey);
        if (it == recordIndexByHandleKey.end() || it->second >= records.size())
            return nullptr;
        return &records[it->second];
    }
};

//! Parse a raw AcDb:AcDsPrototype_1b section buffer into a typed index.
//! Never throws. Malicious counts are capped; short reads produce diagnostics.
DRW_DataStorageSection DRW_parseDataStorage(
    const std::uint8_t* data,
    std::size_t size,
    DRW::Version version = DRW::UNKNOWNV);

inline DRW_DataStorageSection DRW_parseDataStorage(
    const std::vector<std::uint8_t>& data,
    DRW::Version version = DRW::UNKNOWNV) {
    return DRW_parseDataStorage(data.data(), data.size(), version);
}

#endif // DRW_DATASTORAGE_H
