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

#ifndef DWGWRITERLAYOUTVALIDATION_H
#define DWGWRITERLAYOUTVALIDATION_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "../drw_base.h"

namespace dwgWriterLayoutValidation {

enum class ValidationCategory : std::uint8_t {
    None,
    Header,
    Span,
    Topology,
    Map,
    Checksum,
    ReedSolomon,
    Decompression,
    Duplicate
};

struct ValidationResult {
    ValidationCategory category {ValidationCategory::None};

    explicit operator bool() const noexcept {
        return category == ValidationCategory::None;
    }
};

struct R2004Section {
    std::uint32_t sectionId = 0;
    std::uint64_t dataSize = 0;
    std::uint32_t maxPageSize = 0;
    std::uint32_t compressionType = 1;
    std::uint32_t encrypted = 0;
    std::string name;

    struct Page {
        std::uint32_t pageNum = 0;
        std::uint32_t dataSize = 0;
        std::uint64_t startOffset = 0;
    };

    std::vector<std::uint8_t> data;
    std::vector<Page> pages;
};

struct R2004PhysicalPage {
    enum class Kind : std::uint8_t {
        Data,
        DataSectionMap,
        SectionPageMap
    };

    std::uint32_t pageId {0};
    std::uint64_t address {0};
    std::uint64_t size {0};
    Kind kind {Kind::Data};
    std::uint32_t sectionId {0};
    std::string sectionName;
    std::uint64_t logicalOffset {0};
    std::uint64_t headerOffset {0};
    std::uint64_t headerSize {0};
    std::uint64_t payloadOffset {0};
    std::uint64_t payloadSize {0};
};

struct R2004LayoutReceipt {
    DRW::Version version {DRW::AC1018};
    std::uint64_t dataStart {0};
    std::uint64_t declaredFileSize {0};
    std::uint64_t actualFileSize {0};
    std::size_t fileHeaderSize {0};
    std::vector<R2004PhysicalPage> pages;
};

struct R2004Snapshot {
    R2004LayoutReceipt receipt;
    std::vector<R2004Section> sections;
    std::vector<std::vector<std::uint8_t>> dataPages;
    std::vector<std::uint8_t> dataSectionMapPage;
    std::vector<std::uint8_t> sectionPageMapPage;
    std::vector<std::uint8_t> dataSectionMapData;
    std::vector<std::uint8_t> sectionPageMapData;
    std::vector<std::uint32_t> pageSizes;
    std::vector<std::uint8_t> fileHeader;
    std::vector<std::uint8_t> assembled;
};

struct R2007SystemPage {
    std::vector<std::uint8_t> bytes;
    std::uint64_t compressedSize {0};
    std::uint64_t uncompressedSize {0};
    std::uint64_t compressedCrc {0};
    std::uint64_t uncompressedCrc {0};
    std::uint64_t correctionFactor {0};
};

struct R2007Page {
    std::uint32_t id = 0;
    std::vector<std::uint8_t> bytes;
};

struct R2007SectionPage {
    std::uint64_t startOffset = 0;
    std::uint64_t pageSize = 0;
    std::uint64_t uncompressedSize = 0;
    std::uint64_t compressedSize = 0;
    std::uint32_t pageId = 0;
    std::uint64_t checksum = 0;
    std::uint64_t crc = 0;
};

struct R2007Section {
    std::string name;
    std::uint64_t size = 0;
    std::uint64_t maxSize = 0;
    std::uint64_t encoding = 4;
    std::uint64_t encrypted = 0;
    std::uint64_t hashCode = 0;
    std::vector<R2007SectionPage> pages;
};

struct R2007PhysicalPage {
    enum class Kind : std::uint8_t {
        PageMap,
        Data,
        SectionMap
    };

    std::uint32_t pageId {0};
    std::uint64_t address {0};
    std::uint64_t size {0};
    Kind kind {Kind::Data};
    std::string sectionName;
    std::uint64_t logicalOffset {0};
    std::uint64_t encodedSize {0};
};

struct R2007LayoutReceipt {
    DRW::Version version {DRW::AC1021};
    std::size_t metadataSize {0};
    std::size_t fileHeaderPageSize {0};
    std::uint64_t dataStart {0};
    std::uint64_t leadingHeaderAddress {0};
    std::uint64_t trailingHeaderAddress {0};
    std::uint64_t pageMap2Offset {0};
    std::uint32_t pageMap1Id {0};
    std::uint32_t pageMap2Id {0};
    std::uint32_t sectionMapId {0};
    std::uint32_t sectionMap2Id {0};
    std::uint64_t pageCount {0};
    std::uint64_t maxPageId {0};
    std::uint64_t sectionCount {0};
    std::uint64_t declaredFileSize {0};
    std::uint64_t actualFileSize {0};
    std::vector<R2007PhysicalPage> pages;
};

struct R2007Snapshot {
    R2007LayoutReceipt receipt;
    std::vector<R2007Section> sections;
    std::vector<R2007Page> pages;
    std::vector<std::uint8_t> pageMapData;
    std::vector<std::uint8_t> sectionMapData;
    R2007SystemPage pageMap;
    R2007SystemPage sectionMap;
    std::vector<std::uint8_t> metadata;
    std::vector<std::uint8_t> fileHeaderPage;
    std::vector<std::uint8_t> fileHeaderData;
    std::vector<std::uint8_t> assembled;
};

using Snapshot = std::variant<R2004Snapshot, R2007Snapshot>;

ValidationResult validate(const R2004Snapshot& snapshot);
ValidationResult validate(const R2007Snapshot& snapshot);

#ifdef DWG_LAYOUT_VALIDATION_TESTS
using TestSnapshotObserver = void (*)(Snapshot& snapshot);

ValidationResult validateR2007DataPageForTest(
    const R2007Section& section, const R2007SectionPage& sectionPage,
    const R2007Page& physicalPage);

bool reserveTestMutationCapacity(Snapshot& snapshot) noexcept;
void setTestSnapshotObserver(TestSnapshotObserver observer) noexcept;
void notifyTestSnapshot(Snapshot& snapshot) noexcept;
#endif

} // namespace dwgWriterLayoutValidation

#endif // DWGWRITERLAYOUTVALIDATION_H
