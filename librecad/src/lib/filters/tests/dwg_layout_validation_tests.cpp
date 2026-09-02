/****************************************************************************
**
**  This file is part of the LibreCAD project, a 2D CAD program
**
**  Copyright (C) 2026 LibreCAD (librecad.org)
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
******************************************************************************/

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "drw_entities.h"
#include "intern/dwg_dxf_output_transaction.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriterlayoutvalidation.h"

namespace {

using dwgWriterLayoutValidation::R2004Snapshot;
using dwgWriterLayoutValidation::R2007Section;
using dwgWriterLayoutValidation::R2007Snapshot;
using dwgWriterLayoutValidation::Snapshot;
using dwgWriterLayoutValidation::ValidationCategory;

enum class Mutation {
    None,
    R2004Topology,
    R2004Checksum,
    R2004Map,
    R2004DataSectionMapTruncated,
    R2004DataSectionMapTrailingByte,
    R2004DataSectionMapCount,
    R2004DataSectionMapMissingPage,
    R2004DataSectionMapDuplicatePage,
    R2004DataSectionMapOffset,
    R2004DataSectionMapNonAdvancingOffset,
    R2004SectionPageMapTruncated,
    R2004SectionPageMapTrailingByte,
    R2004SectionPageMapDuplicatePage,
    R2004SectionPageMapSize,
    R2007Topology,
    R2007Checksum,
    R2007ReedSolomon,
    R2007Map,
    R2007PageMapTruncated,
    R2007PageMapTrailingByte,
    R2007PageMapDuplicatePage,
    R2007PageMapSize,
    R2007SectionMapTruncated,
    R2007SectionMapTrailingByte,
    R2007SectionMapNameLength,
    R2007SectionMapPageCount,
    R2007SectionMapDuplicatePage,
    R2007DataPageDecompression,
    R2007DataPageTopology,
    R2007DataPageInvalidEncoding,
    R2007DataPageReedSolomon,
    R2007DataPageChecksum,
    R2007DataPageCrc
};

Mutation g_mutation = Mutation::None;
ValidationCategory g_observedCategory = ValidationCategory::None;

void putU32(std::vector<std::uint8_t>& bytes, std::size_t offset,
            std::uint32_t value) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        return;
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    bytes[offset + 2] = static_cast<std::uint8_t>(value >> 16);
    bytes[offset + 3] = static_cast<std::uint8_t>(value >> 24);
}

std::uint32_t getU32(const std::vector<std::uint8_t>& bytes,
                     std::size_t offset) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 4)
        return 0;
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

void putU64(std::vector<std::uint8_t>& bytes, std::size_t offset,
            std::uint64_t value) noexcept {
    putU32(bytes, offset, static_cast<std::uint32_t>(value));
    putU32(bytes, offset + 4, static_cast<std::uint32_t>(value >> 32));
}

std::uint64_t getU64(const std::vector<std::uint8_t>& bytes,
                     std::size_t offset) noexcept {
    return static_cast<std::uint64_t>(getU32(bytes, offset))
        | (static_cast<std::uint64_t>(getU32(bytes, offset + 4)) << 32);
}

bool r2004DsmPageOffset(const R2004Snapshot& state, std::size_t ordinal,
                        std::size_t& result) noexcept {
    constexpr std::size_t headerSize = 20;
    constexpr std::size_t descriptorSize = 96;
    constexpr std::size_t pageSize = 16;
    std::size_t offset = headerSize;
    std::size_t current = 0;
    for (const auto& section : state.sections) {
        if (offset > state.dataSectionMapData.size()
            || state.dataSectionMapData.size() - offset < descriptorSize)
            return false;
        const std::size_t pagesOffset = offset + descriptorSize;
        for (std::size_t page = 0; page < section.pages.size(); ++page) {
            if (pagesOffset > state.dataSectionMapData.size()
                || page >= (state.dataSectionMapData.size() - pagesOffset)
                              / pageSize)
                return false;
            if (current == ordinal) {
                result = pagesOffset + page * pageSize;
                return true;
            }
            ++current;
        }
        if (section.pages.size()
                > (state.dataSectionMapData.size() - pagesOffset) / pageSize)
            return false;
        offset = pagesOffset + section.pages.size() * pageSize;
    }
    return false;
}

bool firstR2007DataPage(R2007Snapshot& state,
                        R2007Section*& sectionResult,
                        dwgWriterLayoutValidation::R2007SectionPage*& pageResult,
                        dwgWriterLayoutValidation::R2007Page*& physicalResult) noexcept {
    for (auto& section : state.sections) {
        for (auto& page : section.pages) {
            for (auto& physical : state.pages) {
                if (physical.id == page.pageId) {
                    sectionResult = &section;
                    pageResult = &page;
                    physicalResult = &physical;
                    return true;
                }
            }
        }
    }
    return false;
}

bool r2007SectionPageOffset(const R2007Snapshot& state, std::size_t ordinal,
                            std::size_t& result) noexcept {
    constexpr std::size_t sectionDescriptorSize = 64;
    constexpr std::size_t pageDescriptorSize = 56;
    std::size_t offset = 0;
    std::size_t current = 0;
    for (const auto& section : state.sections) {
        if (offset > state.sectionMapData.size()
            || state.sectionMapData.size() - offset < sectionDescriptorSize)
            return false;
        const std::uint64_t nameSize = getU64(state.sectionMapData, offset + 32);
        const std::size_t afterDescriptor = offset + sectionDescriptorSize;
        if (nameSize > state.sectionMapData.size() - afterDescriptor)
            return false;
        const std::size_t pagesOffset = afterDescriptor
            + static_cast<std::size_t>(nameSize);
        if (section.pages.size()
                > (state.sectionMapData.size() - pagesOffset)
                      / pageDescriptorSize)
            return false;
        for (std::size_t page = 0; page < section.pages.size(); ++page) {
            if (current == ordinal) {
                result = pagesOffset + page * pageDescriptorSize;
                return true;
            }
            ++current;
        }
        offset = pagesOffset + section.pages.size() * pageDescriptorSize;
    }
    return false;
}

void mutateSnapshot(Snapshot& snapshot) noexcept {
    std::visit([](auto& state) {
        using State = std::decay_t<decltype(state)>;
        if constexpr (std::is_same_v<State, R2004Snapshot>) {
            if (g_mutation == Mutation::R2004Topology) {
                if (!state.receipt.pages.empty())
                    state.receipt.pages.front().pageId = 0;
            } else if (g_mutation == Mutation::R2004Checksum) {
                if (!state.receipt.pages.empty()
                    && !state.dataPages.empty()) {
                    const auto& page = state.receipt.pages.front();
                    const std::uint64_t offset =
                        page.address + 28;
                    if (offset < state.assembled.size()
                        && state.dataPages.front().size() > 28) {
                        state.assembled[static_cast<std::size_t>(offset)] ^= 1;
                        state.dataPages.front()[28] ^= 1;
                    }
                }
            } else if (g_mutation == Mutation::R2004Map) {
                if (!state.sections.empty())
                    state.sections.front().name += "-map";
            } else if (g_mutation == Mutation::R2004DataSectionMapTruncated) {
                if (!state.dataSectionMapData.empty())
                    state.dataSectionMapData.pop_back();
            } else if (g_mutation == Mutation::R2004DataSectionMapTrailingByte) {
                state.dataSectionMapData.push_back(0);
            } else if (g_mutation == Mutation::R2004DataSectionMapCount) {
                putU32(state.dataSectionMapData, 0,
                       getU32(state.dataSectionMapData, 0) + 1);
            } else if (g_mutation == Mutation::R2004DataSectionMapMissingPage) {
                std::size_t offset = 0;
                if (r2004DsmPageOffset(state, 0, offset))
                    putU32(state.dataSectionMapData, offset, 0);
            } else if (g_mutation == Mutation::R2004DataSectionMapDuplicatePage) {
                std::size_t firstOffset = 0;
                std::size_t secondOffset = 0;
                if (r2004DsmPageOffset(state, 0, firstOffset)
                    && r2004DsmPageOffset(state, 1, secondOffset)) {
                    putU32(state.dataSectionMapData, secondOffset,
                           getU32(state.dataSectionMapData, firstOffset));
                }
            } else if (g_mutation == Mutation::R2004DataSectionMapOffset) {
                std::size_t offset = 0;
                if (r2004DsmPageOffset(state, 0, offset)) {
                    putU64(state.dataSectionMapData, offset + 8,
                           getU64(state.dataSectionMapData, offset + 8) + 1);
                }
            } else if (g_mutation == Mutation::R2004DataSectionMapNonAdvancingOffset) {
                std::size_t ordinal = 0;
                for (const auto& section : state.sections) {
                    if (section.pages.size() >= 2) {
                        std::size_t firstOffset = 0;
                        std::size_t secondOffset = 0;
                        if (r2004DsmPageOffset(state, ordinal, firstOffset)
                            && r2004DsmPageOffset(state, ordinal + 1,
                                                  secondOffset)) {
                            putU64(state.dataSectionMapData, secondOffset + 8,
                                   getU64(state.dataSectionMapData,
                                          firstOffset + 8));
                        }
                        break;
                    }
                    ordinal += section.pages.size();
                }
            } else if (g_mutation == Mutation::R2004SectionPageMapTruncated) {
                if (!state.sectionPageMapData.empty())
                    state.sectionPageMapData.pop_back();
            } else if (g_mutation == Mutation::R2004SectionPageMapTrailingByte) {
                state.sectionPageMapData.push_back(0);
            } else if (g_mutation == Mutation::R2004SectionPageMapDuplicatePage) {
                if (state.sectionPageMapData.size() >= 16) {
                    putU32(state.sectionPageMapData, 8,
                           getU32(state.sectionPageMapData, 0));
                }
            } else if (g_mutation == Mutation::R2004SectionPageMapSize) {
                if (state.sectionPageMapData.size() >= 8) {
                    putU32(state.sectionPageMapData, 4,
                           getU32(state.sectionPageMapData, 4) + 1);
                }
            }
        } else if constexpr (std::is_same_v<State, R2007Snapshot>) {
            if (g_mutation == Mutation::R2007Topology) {
                if (!state.pages.empty())
                    state.pages.front().id = 0;
            } else if (g_mutation == Mutation::R2007Checksum) {
                state.pageMap.compressedCrc ^= 1;
            } else if (g_mutation == Mutation::R2007ReedSolomon) {
                if (!state.fileHeaderData.empty())
                    state.fileHeaderData.front() ^= 1;
            } else if (g_mutation == Mutation::R2007Map) {
                if (!state.sections.empty())
                    state.sections.front().hashCode ^= 1;
            } else if (g_mutation == Mutation::R2007PageMapTruncated) {
                if (!state.pageMapData.empty())
                    state.pageMapData.pop_back();
            } else if (g_mutation == Mutation::R2007PageMapTrailingByte) {
                state.pageMapData.push_back(0);
            } else if (g_mutation == Mutation::R2007PageMapDuplicatePage) {
                if (state.pageMapData.size() >= 32) {
                    putU64(state.pageMapData, 24,
                           getU64(state.pageMapData, 8));
                }
            } else if (g_mutation == Mutation::R2007PageMapSize) {
                if (state.pageMapData.size() >= 8) {
                    putU64(state.pageMapData, 0,
                           getU64(state.pageMapData, 0) + 1);
                }
            } else if (g_mutation == Mutation::R2007SectionMapTruncated) {
                if (!state.sectionMapData.empty())
                    state.sectionMapData.pop_back();
            } else if (g_mutation == Mutation::R2007SectionMapTrailingByte) {
                state.sectionMapData.push_back(0);
            } else if (g_mutation == Mutation::R2007SectionMapNameLength) {
                if (state.sectionMapData.size() >= 40) {
                    putU64(state.sectionMapData, 32,
                           getU64(state.sectionMapData, 32) + 2);
                }
            } else if (g_mutation == Mutation::R2007SectionMapPageCount) {
                if (state.sectionMapData.size() >= 64) {
                    putU64(state.sectionMapData, 56,
                           getU64(state.sectionMapData, 56) + 1);
                }
            } else if (g_mutation == Mutation::R2007SectionMapDuplicatePage) {
                std::size_t firstOffset = 0;
                std::size_t secondOffset = 0;
                if (r2007SectionPageOffset(state, 0, firstOffset)
                    && r2007SectionPageOffset(state, 1, secondOffset)) {
                    putU64(state.sectionMapData, secondOffset + 16,
                           getU64(state.sectionMapData, firstOffset + 16));
                }
            } else if (g_mutation == Mutation::R2007DataPageDecompression) {
                for (std::size_t index = 2;
                     index + 2 < state.pages.size(); ++index) {
                    if (state.receipt.pages[index].kind
                            != dwgWriterLayoutValidation::R2007PhysicalPage::Kind::Data
                        || state.pages[index].bytes.size() < 5)
                        continue;
                    for (std::size_t byte = 0; byte < 5; ++byte) {
                        state.pages[index].bytes[byte] ^= 0xff;
                        const std::uint64_t offset =
                            state.receipt.pages[index].address + byte;
                        if (offset < state.assembled.size()) {
                            state.assembled[static_cast<std::size_t>(offset)]
                                ^= 0xff;
                        }
                    }
                    break;
                }
            } else if (g_mutation == Mutation::R2007DataPageTopology
                       || g_mutation == Mutation::R2007DataPageInvalidEncoding
                       || g_mutation == Mutation::R2007DataPageReedSolomon
                       || g_mutation == Mutation::R2007DataPageChecksum
                       || g_mutation == Mutation::R2007DataPageCrc) {
                R2007Section* section = nullptr;
                dwgWriterLayoutValidation::R2007SectionPage* page = nullptr;
                dwgWriterLayoutValidation::R2007Page* physical = nullptr;
                if (!firstR2007DataPage(state, section, page, physical))
                    return;
                if (g_mutation == Mutation::R2007DataPageTopology) {
                    page->compressedSize = page->pageSize + 1;
                } else if (g_mutation == Mutation::R2007DataPageInvalidEncoding) {
                    section->encoding = 2;
                } else if (g_mutation == Mutation::R2007DataPageReedSolomon) {
                    const std::uint64_t alignedSize =
                        (page->compressedSize + 7) & ~std::uint64_t{7};
                    const std::uint64_t blockCount =
                        (alignedSize + 250) / 251;
                    for (std::size_t byte = 0; byte < 10; ++byte) {
                        const std::uint64_t index = byte * blockCount;
                        if (index >= physical->bytes.size())
                            break;
                        physical->bytes[static_cast<std::size_t>(index)] ^= 0xff;
                    }
                } else if (g_mutation == Mutation::R2007DataPageChecksum) {
                    page->checksum ^= 1;
                } else {
                    page->crc ^= 1;
                }
            }
        }
    }, snapshot);
}

void validateObservedSnapshot(Snapshot& snapshot) noexcept {
    mutateSnapshot(snapshot);
    if (g_mutation == Mutation::R2007DataPageTopology
        || g_mutation == Mutation::R2007DataPageInvalidEncoding
        || g_mutation == Mutation::R2007DataPageReedSolomon
        || g_mutation == Mutation::R2007DataPageChecksum
        || g_mutation == Mutation::R2007DataPageCrc) {
        auto* state = std::get_if<R2007Snapshot>(&snapshot);
        R2007Section* section = nullptr;
        dwgWriterLayoutValidation::R2007SectionPage* page = nullptr;
        dwgWriterLayoutValidation::R2007Page* physical = nullptr;
        if (state == nullptr
            || !firstR2007DataPage(*state, section, page, physical)) {
            g_observedCategory = ValidationCategory::Header;
            return;
        }
        g_observedCategory =
            dwgWriterLayoutValidation::validateR2007DataPageForTest(
                *section, *page, *physical).category;
        return;
    }
    g_observedCategory = std::visit(
        [](const auto& state) {
            return dwgWriterLayoutValidation::validate(state).category;
        }, snapshot);
}

template <typename Writer>
bool writeMinimalDocument(DwgDxfOutputTransaction& output) {
    DRW_Header header;
    Writer writer(&output.stream(), &header);
    if (!writer.prepareDwgClassManifest()
        || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader()
        || !writer.writeDwgClasses()
        || !writer.writeDwgObjects())
        return false;

    DRW_Line line;
    line.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    line.secPoint = DRW_Coord{10.0, 10.0, 0.0};
    if (!writer.encodeEntity(&line)
        || !writer.finalizeHeaderHandseed()
        || !writer.writeDwgHandles()
        || !writer.writeSecondHeader())
        return false;
    return writer.finalize();
}

bool writeLargeR2004Document(DwgDxfOutputTransaction& output) {
    DRW_Header header;
    dwgWriter18 writer(&output.stream(), &header);
    DRW_RawDwgSection stressSection;
    stressSection.m_name = "AcDb:ValidatorStress";
    stressSection.m_version = DRW::AC1018;
    stressSection.m_encoding = 1;
    stressSection.m_maxSize = 0x7400;
    stressSection.m_data.assign(0x7401, 0xa5);
    if (!writer.addRawDwgSection(stressSection)
        || !writer.prepareDwgClassManifest()
        || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader()
        || !writer.writeDwgClasses()
        || !writer.writeDwgObjects())
        return false;

    DRW_Line line;
    line.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    line.secPoint = DRW_Coord{10.0, 10.0, 0.0};
    return writer.encodeEntity(&line)
        && writer.finalizeHeaderHandseed()
        && writer.writeDwgHandles()
        && writer.writeSecondHeader()
        && writer.finalize();
}

template <typename Writer>
ValidationCategory runMutation(
        Mutation mutation,
        bool (*write)(DwgDxfOutputTransaction&) = writeMinimalDocument<Writer>) {
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / "libdxfrw_layout_validation_test.dwg";
    std::filesystem::remove(path);
    DwgDxfOutputTransaction output(path.string(), std::ios::binary);
    if (!output.open())
        return ValidationCategory::Header;

    g_mutation = mutation;
    g_observedCategory = ValidationCategory::None;
    dwgWriterLayoutValidation::setTestSnapshotObserver(
        &validateObservedSnapshot);
    const bool result = write(output);
    dwgWriterLayoutValidation::setTestSnapshotObserver(nullptr);
    g_mutation = Mutation::None;
    output.abort();
    std::filesystem::remove(path);
    if (result)
        return ValidationCategory::None;
    return g_observedCategory;
}

} // namespace

TEST_CASE("DWG layout validators accept writer snapshots",
          "[dwg-write][layout][validation]") {
    CHECK(runMutation<dwgWriter18>(Mutation::None)
          == ValidationCategory::None);
    CHECK(runMutation<dwgWriter21>(Mutation::None)
          == ValidationCategory::None);
}

TEST_CASE("R2004 layout validation rejects topology, checksum, and semantic map mutations",
          "[dwg-write][layout][r2004]") {
    CHECK(runMutation<dwgWriter18>(Mutation::R2004Topology)
          == ValidationCategory::Topology);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004Checksum)
          == ValidationCategory::Checksum);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004Map)
          == ValidationCategory::Map);
}

TEST_CASE("R2004 layout validation rejects direct map byte mutations",
          "[dwg-write][layout][r2004][map]") {
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapTruncated)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapTrailingByte)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapCount)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapMissingPage)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapDuplicatePage)
          == ValidationCategory::Duplicate);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004DataSectionMapOffset)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(
              Mutation::R2004DataSectionMapNonAdvancingOffset,
              &writeLargeR2004Document)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004SectionPageMapTruncated)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004SectionPageMapTrailingByte)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004SectionPageMapDuplicatePage)
          == ValidationCategory::Duplicate);
    CHECK(runMutation<dwgWriter18>(Mutation::R2004SectionPageMapSize)
          == ValidationCategory::Map);
}

TEST_CASE("R2007 layout validation rejects topology, checksum, RS, and map mutations",
          "[dwg-write][layout][r2007]") {
    CHECK(runMutation<dwgWriter21>(Mutation::R2007Topology)
          == ValidationCategory::Topology);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007Checksum)
          == ValidationCategory::Checksum);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007ReedSolomon)
          == ValidationCategory::ReedSolomon);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007Map)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageDecompression)
          == ValidationCategory::Decompression);
}

TEST_CASE("R2007 layout validation rejects direct map byte mutations",
          "[dwg-write][layout][r2007][map]") {
    CHECK(runMutation<dwgWriter21>(Mutation::R2007PageMapTruncated)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007PageMapTrailingByte)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007PageMapDuplicatePage)
          == ValidationCategory::Duplicate);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007PageMapSize)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007SectionMapTruncated)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007SectionMapTrailingByte)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007SectionMapNameLength)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007SectionMapPageCount)
          == ValidationCategory::Map);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007SectionMapDuplicatePage)
          == ValidationCategory::Duplicate);
}

TEST_CASE("R2007 ordinary data-page validation preserves failure categories",
          "[dwg-write][layout][r2007][data]") {
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageTopology)
          == ValidationCategory::Topology);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageInvalidEncoding)
          == ValidationCategory::Topology);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageReedSolomon)
          == ValidationCategory::ReedSolomon);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageChecksum)
          == ValidationCategory::Checksum);
    CHECK(runMutation<dwgWriter21>(Mutation::R2007DataPageCrc)
          == ValidationCategory::Checksum);
}

TEST_CASE("Rejected DWG layout leaves the existing destination unchanged",
          "[dwg-write][layout][transaction]") {
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / "libdxfrw_layout_validation_existing.dwg";
    std::filesystem::remove(path);
    {
        std::ofstream existing(path, std::ios::binary | std::ios::trunc);
        REQUIRE(existing.good());
        existing << "existing DWG content";
    }

    const auto rejectWrite = [&](Mutation mutation,
                                 ValidationCategory expectedCategory,
                                 auto&& write) {
        DwgDxfOutputTransaction output(path.string(), std::ios::binary);
        REQUIRE(output.open());
        g_mutation = mutation;
        g_observedCategory = ValidationCategory::None;
        dwgWriterLayoutValidation::setTestSnapshotObserver(
            &validateObservedSnapshot);
        CHECK_FALSE(write(output));
        dwgWriterLayoutValidation::setTestSnapshotObserver(nullptr);
        g_mutation = Mutation::None;
        output.abort();
        CHECK(g_observedCategory == expectedCategory);
    };
    rejectWrite(Mutation::R2004DataSectionMapTruncated,
                ValidationCategory::Map,
                [](DwgDxfOutputTransaction& output) {
                    return writeMinimalDocument<dwgWriter18>(output);
                });
    rejectWrite(Mutation::R2004Checksum, ValidationCategory::Checksum,
                [](DwgDxfOutputTransaction& output) {
                    return writeMinimalDocument<dwgWriter18>(output);
                });
    rejectWrite(Mutation::R2007Topology, ValidationCategory::Topology,
                [](DwgDxfOutputTransaction& output) {
                    return writeMinimalDocument<dwgWriter21>(output);
                });
    rejectWrite(Mutation::R2007DataPageDecompression,
                ValidationCategory::Decompression,
                [](DwgDxfOutputTransaction& output) {
                    return writeMinimalDocument<dwgWriter21>(output);
                });

    std::ifstream preserved(path, std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(preserved)),
                              std::istreambuf_iterator<char>());
    CHECK(content == "existing DWG content");

    const std::string temporaryPrefix = path.filename().string()
        + ".libdxfrw-";
    bool temporaryFound = false;
    for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
        const std::string entryName = entry.path().filename().string();
        if (entryName.size() >= temporaryPrefix.size()
            && entryName.compare(0, temporaryPrefix.size(), temporaryPrefix) == 0) {
            temporaryFound = true;
            break;
        }
    }
    CHECK_FALSE(temporaryFound);
    std::filesystem::remove(path);
}
