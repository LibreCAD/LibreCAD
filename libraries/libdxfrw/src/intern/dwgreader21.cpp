/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "dwgreader21.h"
#include "dwgsafety.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"

namespace {

// AC1021's fixed file-header envelope. These are wire-format dimensions, not
// resource limits; allocation limits remain centralized in dwgSafety.
constexpr std::uint64_t kR2007FileHeaderOffset = 0x80;
constexpr std::size_t kR2007RawFileHeaderSize = 0x2FD;
constexpr std::size_t kR2007DecodedFileHeaderSize = 0x2CD;
constexpr std::size_t kR2007FileHeaderDataSize = 0x110;
constexpr std::size_t kR2007FileHeaderPageSize = 0x400;
constexpr std::uint64_t kR2007PageMapBaseOffset = 0x480;

} // namespace

bool dwgReader21::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    DRW_DBG("dwgReader21::readFileHeader()\n");
    DRW_DBG("dwgReader21::parsing metadata\n");
    if (! fileBuf->setPosition(11))
        return false;
    maintenanceVersion = fileBuf->getRawChar8();
    DRW_DBG("maintenance version= "); DRW_DBGH(maintenanceVersion);
    DRW_DBG("\nbyte at 0x0C= "); DRW_DBG(fileBuf->getRawChar8());
    previewImagePos = fileBuf->getRawLong32();
    DRW_DBG("previewImagePos (seekerImageData) = "); DRW_DBG(previewImagePos);
    DRW_DBG("\n\napp writer version= "); DRW_DBGH(fileBuf->getRawChar8());
    appMaintenanceVersion = fileBuf->getRawChar8(); // byte 0x12 — hSize gate
    DRW_DBG("\napp writer maintenance version= "); DRW_DBGH(appMaintenanceVersion);
    std::uint16_t cp = fileBuf->getRawShort16();
    DRW_DBG("\ncodepage= "); DRW_DBG(cp);
    // R2007+ (AC1021+) store all text — section-map names and entity strings —
    // as UTF-16LE; the DWGCODEPAGE field is only meaningful for R2004 and
    // earlier.  Applying it here would replace the UTF-16 decoder that
    // setVersion(AC1021) installed with a single-byte ANSI table, leaving the
    // interleaved 0x00 bytes in getUCSStr()'s output so section names like
    // "AcDb:Header" never match secEnum::getEnum() -> every section collapses
    // to UNKNOWNS, HEADER stays Id==-1, and readDwgHeader() fails with
    // BAD_READ_HEADER (systemic across all R2007 files).  reader21 is dispatched
    // only for AC1021, so this guard skips the override entirely; the clause
    // mirrors dwgReader18::readFileHeader for symmetry.
    if (const char* cpName = dwgCodePageName(cp)) {
        decoder.setByteCodePage(cpName);
        if (version <= DRW::AC1018)
            decoder.setCodePage(cpName, false);
    }
    /* UNKNOUWN SECTION 2 bytes*/
    DRW_DBG("\nUNKNOWN SECTION= "); DRW_DBG(fileBuf->getRawShort16());
    DRW_DBG("\nUNKNOUWN SECTION 3b= "); DRW_DBG(fileBuf->getRawChar8());
    std::uint32_t secType = fileBuf->getRawLong32();
    DRW_DBG("\nsecurity type flag= "); DRW_DBGH(secType);
    /* UNKNOWN2 SECTION 4 bytes*/
    DRW_DBG("\nUNKNOWN SECTION 4bytes= "); DRW_DBG(fileBuf->getRawLong32());

    DRW_DBG("\nSummary info address= "); DRW_DBGH(fileBuf->getRawLong32());
    DRW_DBG("\nVBA project address= "); DRW_DBGH(fileBuf->getRawLong32());
    DRW_DBG("\n0x00000080 32b= "); DRW_DBGH(fileBuf->getRawLong32());
    DRW_DBG("\nApp info address= "); DRW_DBGH(fileBuf->getRawLong32());
    //current position are 0x30 from here to 0x80 are undocumented
    DRW_DBG("\nAnother address? = "); DRW_DBGH(fileBuf->getRawLong32());
    return fileBuf->isGood();
}

bool dwgReader21::parseSectionPageMap(
    std::uint8_t* data, std::uint64_t size,
    std::uint64_t expectedRecordCount, std::uint64_t maxPageId,
    std::uint64_t fileSize,
    std::unordered_map<std::uint64_t, dwgPageInfo>& pages,
    PageMapFailure* failure) {
    if (failure)
        *failure = PageMapFailure::None;
    const auto setFailure = [failure](PageMapFailure value) {
        if (failure && *failure == PageMapFailure::None)
            *failure = value;
    };
    if (data == nullptr || size == 0 || expectedRecordCount == 0
        || expectedRecordCount > dwgSafety::MaxPageCount || maxPageId == 0
        || maxPageId > dwgSafety::MaxPageCount) {
        setFailure(PageMapFailure::InvalidInput);
        return false;
    }

    dwgBuffer buffer(data, size);
    std::unordered_map<std::uint64_t, dwgPageInfo> parsedPages;
    if (expectedRecordCount
            > static_cast<std::uint64_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(parsedPages,
                          static_cast<int>(expectedRecordCount))) {
        setFailure(PageMapFailure::ResourceLimit);
        return false;
    }
    std::uint64_t address = 0x480;
    std::uint64_t offset = 0;
    std::uint64_t recordCount = 0;
    const auto* dataEnd = data + size;
    while (offset < size) {
        const std::uint64_t remaining = size - offset;
        if (remaining < 16) {
            const auto* tail = data + offset;
            if (!std::all_of(tail, dataEnd,
                             [](std::uint8_t value) { return value == 0; })) {
                setFailure(PageMapFailure::NonZeroTail);
                return false;
            }
            break;
        }
        const auto* record = data + offset;
        if (std::all_of(record, record + 16,
                        [](std::uint8_t value) { return value == 0; })) {
            if (!std::all_of(record, dataEnd,
                             [](std::uint8_t value) { return value == 0; })) {
                setFailure(PageMapFailure::NonZeroTail);
                return false;
            }
            break;
        }
        if (recordCount >= dwgSafety::MaxPageCount) {
            setFailure(PageMapFailure::ResourceLimit);
            return false;
        }

        const std::uint64_t pageSize = buffer.getRawLong64();
        const std::uint64_t rawId = buffer.getRawLong64();
        offset += 16;
        if (!buffer.isGood()) {
            setFailure(PageMapFailure::Truncated);
            return false;
        }
        if (pageSize == 0) {
            setFailure(PageMapFailure::InvalidPageId);
            return false;
        }

        const bool isGap = (rawId & (std::uint64_t{1} << 63)) != 0;
        if (!isGap) {
            if (rawId == 0 || rawId > maxPageId) {
                setFailure(PageMapFailure::InvalidPageId);
                return false;
            }
            if (!parsedPages.emplace(
                    rawId, dwgPageInfo(rawId, address, pageSize)).second) {
                setFailure(PageMapFailure::DuplicatePageId);
                return false;
            }
        }

        std::uint64_t pageEnd = 0;
        if (!dwgSafety::add(address, pageSize, pageEnd)
            || pageEnd > fileSize) {
            setFailure(PageMapFailure::PageRange);
            return false;
        }
        address = pageEnd;
        ++recordCount;
    }

    if (!buffer.isGood()) {
        setFailure(PageMapFailure::Truncated);
        return false;
    }
    if (recordCount != expectedRecordCount) {
        setFailure(PageMapFailure::CountMismatch);
        return false;
    }
    pages.swap(parsedPages);
    return true;
}

bool dwgReader21::parseSysPage(std::uint64_t sizeCompressed,
                               std::uint64_t sizeUncompressed,
                               std::uint64_t correctionFactor,
                               std::uint64_t offset,
                               std::uint64_t crcSeed,
                               std::uint64_t expectedCompressedCrc,
                               std::uint64_t expectedUncompressedCrc,
                               std::uint8_t *decompData,
                               DwgIntegrityPhase phase,
    std::int32_t logicalSectionId,
    std::int32_t sectionDescriptorId){
    const auto recordCrc = [&](DwgIntegrityCheckKind kind,
                               std::uint64_t expected,
                               std::uint64_t observed) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Warning,
            DwgIntegrityAddressSpace::DecodedBuffer, phase, kind,
            logicalSectionId, sectionDescriptorId, nullptr, 0, false,
            0, false, 0, false, expected, observed, true);
    };
    const auto recordFailure = [&](DwgIntegrityCheckKind kind) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile, phase, kind,
            logicalSectionId, sectionDescriptorId, nullptr, 0, false,
            offset, true);
    };
    constexpr std::uint64_t maxSystemPageSize = 128ULL * 1024ULL * 1024ULL;
    if (decompData == nullptr || sizeCompressed == 0 || sizeUncompressed == 0
        || sizeUncompressed > maxSystemPageSize || correctionFactor == 0) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }

    std::uint64_t alignedSize = 0;
    std::uint64_t scaledSize = 0;
    std::uint64_t numerator = 0;
    if (!dwgSafety::alignUp8(sizeCompressed, alignedSize)
        || !dwgSafety::multiply(alignedSize, correctionFactor, scaledSize)
        || !dwgSafety::add(scaledSize, 238, numerator)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    const std::uint64_t chunks64 = numerator / 239;
    if (chunks64 == 0 || chunks64 > std::numeric_limits<std::uint32_t>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::uint64_t fpsize = 0;
    std::uint64_t decodedSize = 0;
    if (!dwgSafety::multiply(chunks64, 255, fpsize)
        || !dwgSafety::multiply(chunks64, 239, decodedSize)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (sizeCompressed > decodedSize || fpsize > dwgSafety::MaxBufferSize
        || fpsize > std::numeric_limits<std::size_t>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (!dwgSafety::range(offset, fpsize, fileBuf->size())) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    const std::uint32_t chunks = static_cast<std::uint32_t>(chunks64);

    if (!fileBuf->setPosition(offset)) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    std::vector<std::uint8_t> tmpDataRaw;
    if (fpsize > std::numeric_limits<int>::max()
        || !DRW::resize(tmpDataRaw, static_cast<int>(fpsize))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (!fileBuf->getBytes(tmpDataRaw.data(), fpsize)) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        DRW_DBG("\nERROR: dwgReader21::parseSysPage: short read of RS-encoded data\n");
        return false;
    }
    std::vector<std::uint8_t> tmpDataRS;
    if (!DRW::resize(tmpDataRS, static_cast<int>(fpsize))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (!dwgRSCodec::decode239I(tmpDataRaw.data(), tmpDataRS.data(), chunks)) {
        recordFailure(DwgIntegrityCheckKind::ReedSolomonDecode);
        DRW_DBG("\nERROR: dwgReader21::parseSysPage: RS decode failed\n");
        return false;
    }

    if (expectedCompressedCrc != 0) {
        const auto calculated = dwgUtil::crc64Mirrored(
            dwgUtil::updateSeed1(crcSeed, sizeCompressed),
            tmpDataRS.data(), sizeCompressed);
        if (calculated != expectedCompressedCrc) {
            ++m_r2007CrcMismatch;
            recordCrc(DwgIntegrityCheckKind::SystemPageCrc,
                      expectedCompressedCrc, calculated);
            DRW_DBG("\nWARNING: dwgReader21::parseSysPage: compressed CRC mismatch\n");
        }
    }

    if (sizeCompressed < sizeUncompressed) {
        dwgCompressor comp;
        if (!comp.decompress21(tmpDataRS.data(), decompData,
                               sizeCompressed, sizeUncompressed)) {
            recordFailure(DwgIntegrityCheckKind::Decompression);
            return false;
        }
    } else {
        // Stored (incompressible) system page: the RS-decoded buffer is the
        // raw payload rather than an AC21 compressed stream.
        if (sizeUncompressed > tmpDataRS.size()) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("\nERROR: dwgReader21::parseSysPage: stored page uSize exceeds RS buffer\n");
            return false;
        }
        std::memcpy(decompData, tmpDataRS.data(), sizeUncompressed);
    }

    if (expectedUncompressedCrc != 0) {
        const auto calculated = dwgUtil::crc64Mirrored(
            dwgUtil::updateSeed1(crcSeed, sizeUncompressed),
            decompData, sizeUncompressed);
        if (calculated != expectedUncompressedCrc) {
            ++m_r2007CrcMismatch;
            recordCrc(DwgIntegrityCheckKind::SystemPageUncompressedCrc,
                      expectedUncompressedCrc, calculated);
            DRW_DBG("\nWARNING: dwgReader21::parseSysPage: uncompressed CRC mismatch\n");
        }
    }
    return true;
}

bool dwgReader21::parseDataPage(const dwgSectionInfo &si, std::uint8_t *dData){
    DRW_DBG("parseDataPage, section size: ");
    DRW_DBG(static_cast<unsigned long long>(si.size));
    const std::int32_t logicalSectionId =
        static_cast<std::int32_t>(secEnum::getEnum(si.name));
    const auto recordSectionFailure = [&](DwgIntegrityCheckKind kind) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::None,
            DwgIntegrityPhase::DataPage, kind, logicalSectionId, si.Id,
            si.name.c_str());
    };
    const auto recordPageFailure = [&](const dwgPageInfo& page,
                                       DwgIntegrityCheckKind kind) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::DataPage, kind, logicalSectionId, si.Id,
            si.name.c_str(), page.Id, true, page.address, true);
    };
    const auto recordPageValue = [&](const dwgPageInfo& page,
                                     DwgIntegrityCheckKind kind,
                                     DwgIntegritySeverity severity,
                                     std::uint64_t expected,
                                     std::uint64_t observed) {
        recordIntegrityDiagnostic(
            severity, DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::DataPage, kind, logicalSectionId, si.Id,
            si.name.c_str(), page.Id, true, page.address, true,
            0, false, expected, observed, true);
    };
    constexpr std::uint64_t maxPageCap = dwgSafety::MaxPageCap;
    constexpr std::uint64_t maxBufferSize = dwgSafety::MaxBufferSize;
    if (dData == nullptr || si.size == 0) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::uint64_t pageCap = si.maxSize != 0 ? si.maxSize : si.size;
    for (const auto& entry : si.pages)
        pageCap = std::max(pageCap, entry.second.dataSize);
    if (pageCap == 0 || pageCap > maxBufferSize
        || (si.pages.size() > 1 && pageCap > maxPageCap)
        || si.pages.size() > maxBufferSize / pageCap) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::uint64_t decodedBufferSize = 0;
    if (!dwgSafety::sectionBufferCapacity(si.size, si.pages.size(),
                                          pageCap, decodedBufferSize)) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (decodedBufferSize < si.size || decodedBufferSize > maxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::vector<std::uint8_t> decodedBuffer;
    if (decodedBufferSize > std::numeric_limits<int>::max()
        || !DRW::resize(decodedBuffer, static_cast<int>(decodedBufferSize))) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::fill(decodedBuffer.begin(), decodedBuffer.end(), 0);
    for (auto it=si.pages.begin(); it!=si.pages.end(); ++it){
        dwgPageInfo pi = it->second;
        if (pi.size < 32 || pi.size > dwgSafety::MaxBufferSize
            || !dwgSafety::range(pi.address, pi.size, fileBuf->size())) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: invalid raw page range\n");
            return false;
        }
        if (pi.startOffset > decodedBufferSize
            || pi.uSize > decodedBufferSize - pi.startOffset
            || pi.uSize > pageCap
            || pi.startOffset > si.size
            || pi.uSize > si.size - pi.startOffset) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: page range exceeds decoded capacity\n");
            return false;
        }
        if (pi.size > std::numeric_limits<std::size_t>::max()) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        if (pi.cSize > pi.size) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: compressed page size exceeds raw page size\n");
            return false;
        }
        if (pi.dataSize != 0 && pi.uSize > pi.dataSize) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: page data size declarations disagree\n");
            return false;
        }
        if (!fileBuf->setPosition(pi.address)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            return false;
        }

        std::vector<std::uint8_t> tmpPageRaw;
        if (pi.size > std::numeric_limits<int>::max()
            || !DRW::resize(tmpPageRaw, static_cast<int>(pi.size))) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        if (!fileBuf->getBytes(tmpPageRaw.data(), pi.size)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: short read of page data\n");
            return false;
        }
        std::uint8_t pageHeader[32];
        std::memcpy(pageHeader, tmpPageRaw.data(), sizeof(pageHeader));
        dwgCompressor::decrypt18Hdr(pageHeader, sizeof(pageHeader), pi.address);
        const std::uint32_t pageType = static_cast<std::uint32_t>(pageHeader[0])
                                     | (static_cast<std::uint32_t>(pageHeader[1]) << 8)
                                     | (static_cast<std::uint32_t>(pageHeader[2]) << 16)
                                     | (static_cast<std::uint32_t>(pageHeader[3]) << 24);
        constexpr std::uint32_t ac18DataPageType = 0x4163043b;
        if (pageType == ac18DataPageType) {
            if (pi.startOffset > decodedBufferSize
                || pageCap > decodedBufferSize - pi.startOffset) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                return false;
            }
            std::uint8_t *pageData = decodedBuffer.data() + pi.startOffset;
            if (pi.size < sizeof(pageHeader)) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                return false;
            }
            dwgBuffer headerBuffer(pageHeader, sizeof(pageHeader), &decoder);
            headerBuffer.getRawLong32(); // page type
            const std::uint32_t headerSectionNumber = headerBuffer.getRawLong32();
            const std::uint64_t compressedSize = headerBuffer.getRawLong32();
            const std::uint64_t uncompressedSize = headerBuffer.getRawLong32();
            const std::uint64_t headerStartOffset = headerBuffer.getRawLong32();
            headerBuffer.getRawLong32(); // unknown
            const std::uint32_t storedHeaderChecksum = headerBuffer.getRawLong32();
            const std::uint32_t storedDataChecksum = headerBuffer.getRawLong32();
            std::uint64_t payloadOffset = 0;
            if (!dwgSafety::add(pi.address, sizeof(pageHeader), payloadOffset)
                || !headerBuffer.isGood() || headerStartOffset != pi.startOffset
                || si.Id < 0
                || headerSectionNumber != static_cast<std::uint32_t>(si.Id)
                || compressedSize > pi.size - sizeof(pageHeader)
                || uncompressedSize > pi.size
                || uncompressedSize > pageCap
                || headerStartOffset >= si.size
                || (si.compressed == 1 && compressedSize != uncompressedSize)
                || !dwgSafety::range(payloadOffset, compressedSize, fileBuf->size())) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                DRW_DBG("\nERROR: dwgReader21::parseDataPage: invalid AC18 page header\n");
                return false;
            }
            const std::uint8_t *compressedData =
                tmpPageRaw.data() + sizeof(pageHeader);
            if (dwgUtil::checksum18(0, compressedData, compressedSize)
                != storedDataChecksum) {
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageChecksum,
                                DwgIntegritySeverity::Error, storedDataChecksum,
                                dwgUtil::checksum18(0, compressedData,
                                                    compressedSize));
                DRW_DBG("\nERROR: dwgReader21::parseDataPage: AC18 data checksum mismatch\n");
                return false;
            }
            for (std::uint8_t i = 24; i < 28; ++i)
                pageHeader[i] = 0;
            if (dwgUtil::checksum18(
                    dwgUtil::checksum18(0, compressedData, compressedSize),
                    pageHeader, sizeof(pageHeader)) != storedHeaderChecksum) {
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageChecksum,
                                DwgIntegritySeverity::Error, storedHeaderChecksum,
                                dwgUtil::checksum18(
                                    dwgUtil::checksum18(0, compressedData,
                                                        compressedSize),
                                    pageHeader, sizeof(pageHeader)));
                DRW_DBG("\nERROR: dwgReader21::parseDataPage: AC18 header checksum mismatch\n");
                return false;
            }
            if (si.compressed == 1) {
                const std::uint64_t copySize = std::min(compressedSize, pageCap);
                std::copy(compressedData, compressedData + copySize, pageData);
            } else {
                dwgCompressor compressor;
                if (!compressor.decompress18(
                        compressedData, pageData, compressedSize, pageCap)) {
                    recordPageFailure(pi, DwgIntegrityCheckKind::Decompression);
                    DRW_DBG("\nERROR: dwgReader21::parseDataPage: AC18 decompression failed\n");
                    return false;
                }
            }
            continue;
        }

        if (version < DRW::AC1021) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        if (pi.uSize == 0 || pi.cSize == 0) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        if (pi.startOffset > decodedBufferSize
            || pi.uSize > decodedBufferSize - pi.startOffset) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
    #ifdef DRW_DBG_DUMP
        DRW_DBG("\nSection OBJECTS raw data=\n");
        for (unsigned int i=0, j=0; i< pi.size;i++) {
            DRW_DBGH( (unsigned char)tmpPageRaw[i]);
            if (j == 7) { DRW_DBG("\n"); j = 0;
            } else { DRW_DBG(", "); j++; }
        } DRW_DBG("\n");
    #endif

        std::vector<std::uint8_t> tmpPageRS;
        if (pi.size > std::numeric_limits<int>::max()
            || !DRW::resize(tmpPageRS, static_cast<int>(pi.size))) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        std::uint64_t alignedCSize = 0;
        std::uint64_t chunksNumerator = 0;
        if (!dwgSafety::alignUp8(pi.cSize, alignedCSize)
            || !dwgSafety::add(alignedCSize, 250, chunksNumerator)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        const std::uint64_t chunks64 = chunksNumerator / 251;
        std::uint64_t decodedSize = 0;
        if (chunks64 == 0 || chunks64 > std::numeric_limits<std::uint32_t>::max()
            || !dwgSafety::multiply(chunks64, 251, decodedSize)
            || !dwgSafety::multiply(chunks64, 255, chunksNumerator)
            || chunksNumerator > pi.size
            || pi.cSize > decodedSize) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        const std::uint32_t chunks = static_cast<std::uint32_t>(chunks64);
        bool decoded = false;
        if (si.compressed == 1) {
            decoded = dwgRSCodec::decode251(
                tmpPageRaw.data(), tmpPageRS.data(), chunks);
        } else if (si.compressed == 4) {
            decoded = dwgRSCodec::decode251I(
                tmpPageRaw.data(), tmpPageRS.data(), chunks);
        } else {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: unsupported R2007 page encoding\n");
            return false;
        }
        if (!decoded) {
            recordPageFailure(pi, DwgIntegrityCheckKind::ReedSolomonDecode);
            DRW_DBG("\nERROR: dwgReader21::parseDataPage: RS decode failed\n");
            return false;
        }

        if (pi.crc != 0) {
            const auto calculated = dwgUtil::crc64Mirrored(
                dwgUtil::updateSeed1(r2007CrcSeed, pi.cSize),
                tmpPageRS.data(), pi.cSize);
            if (calculated != pi.crc) {
                ++m_r2007CrcMismatch;
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageCrc,
                                DwgIntegritySeverity::Warning, pi.crc,
                                calculated);
                DRW_DBG("\nWARNING: dwgReader21::parseDataPage: page CRC mismatch\n");
            }
        }
    #ifdef DRW_DBG_DUMP
        DRW_DBG("\nSection OBJECTS RS data=\n");
        for (unsigned int i=0, j=0; i< pi.size;i++) {
            DRW_DBGH( (unsigned char)tmpPageRS[i]);
            if (j == 7) { DRW_DBG("\n"); j = 0;
            } else { DRW_DBG(", "); j++; }
        } DRW_DBG("\n");
    #endif

        DRW_DBG("\npage uncomp size: ");
        DRW_DBG(static_cast<unsigned long long>(pi.uSize));
        DRW_DBG(" comp size: ");
        DRW_DBG(static_cast<unsigned long long>(pi.cSize));
        DRW_DBG("\noffset: ");
        DRW_DBG(static_cast<unsigned long long>(pi.startOffset));
        std::uint8_t *pageData = decodedBuffer.data() + pi.startOffset;
        dwgCompressor comp;
        if (pi.cSize == pi.uSize) {
            if (pi.uSize > tmpPageRS.size()) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                DRW_DBG("\nERROR: dwgReader21::parseDataPage: stored page uSize exceeds RS-decoded page\n");
                return false;
            }
            std::memcpy(pageData, tmpPageRS.data(), pi.uSize);
        } else if (!comp.decompress21(tmpPageRS.data(), pageData, pi.cSize,
                                     pi.uSize)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::Decompression);
            return false;
        }

        if (pi.checksum != 0) {
            const auto calculated = dwgUtil::checksum21(
                r2007CrcSeed, pageData, pi.uSize);
            if (calculated != static_cast<std::uint32_t>(pi.checksum)) {
                ++m_r2007CrcMismatch;
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageChecksum,
                                DwgIntegritySeverity::Warning, pi.checksum,
                                calculated);
                DRW_DBG("\nWARNING: dwgReader21::parseDataPage: page checksum mismatch\n");
            }
        }

    #ifdef DRW_DBG_DUMP
        DRW_DBG("\n\nSection OBJECTS decompressed data=\n");
        for (unsigned int i=0, j=0; i< pi.uSize;i++) {
            DRW_DBGH( (unsigned char)pageData[i]);
            if (j == 7) { DRW_DBG("\n"); j = 0;
            } else { DRW_DBG(", "); j++; }
        } DRW_DBG("\n");
    #endif

    }
    std::memcpy(dData, decodedBuffer.data(), static_cast<std::size_t>(si.size));
    DRW_DBG("\n");
    return true;
}

bool dwgReader21::readFileHeader() {

    DRW_DBG("\n\ndwgReader21::parsing file header\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   DwgIntegrityPhase phase,
                                   std::int32_t logicalSectionId = -1,
                                   std::int32_t sectionDescriptorId = -1,
                                   std::uint64_t pageId = 0,
                                   bool hasPageId = false,
                                   std::uint64_t offset = 0,
                                   bool hasOffset = false,
                                   std::uint64_t expected = 0,
                                   std::uint64_t observed = 0,
                                   bool hasValues = false) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            hasOffset ? DwgIntegrityAddressSpace::PhysicalFile
                      : DwgIntegrityAddressSpace::None,
            phase, kind, logicalSectionId, sectionDescriptorId, nullptr,
            pageId, hasPageId, offset, hasOffset, 0, false,
            expected, observed, hasValues);
    };
    sections.clear();
    m_unknownSections.clear();
    m_rawDwgSections.clear();
    m_dataStorageSections.clear();
    m_dataStorageLinkedRecords.clear();
    objData.reset();
    dataSize = 0;
    r2007CrcSeed = 0;
    if (!fileBuf->setPosition(kR2007FileHeaderOffset)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, kR2007FileHeaderOffset, true);
        return false;
    }
    std::uint8_t fileHdrRaw[kR2007RawFileHeaderSize]{};
    if (!fileBuf->getBytes(fileHdrRaw, kR2007RawFileHeaderSize)) {
        DRW_DBG("\nERROR: dwgReader21: file too small to contain RS-encoded file header (need 0x37D = 893 bytes)\n");
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, kR2007FileHeaderOffset, true);
        return false;
    }
    std::uint8_t fileHdrdRS[kR2007DecodedFileHeaderSize]{};
    if (!dwgRSCodec::decode239I(fileHdrRaw, fileHdrdRS, 3)) {
        DRW_DBG("\nERROR: dwgReader21: file header RS decode failed\n");
        recordFailure(DwgIntegrityCheckKind::ReedSolomonDecode,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, kR2007FileHeaderOffset, true);
        return false;
    }

#ifdef DRW_DBG_DUMP
    DRW_DBG("\ndwgReader21::parsed Reed Solomon decode:\n");
    int j = 0;
    for (int i=0, j=0; i<static_cast<int>(kR2007DecodedFileHeaderSize); i++){
        DRW_DBGH( (unsigned char)fileHdrdRS[i]);
        if (j== 15){ j=0; DRW_DBG("\n");
        } else{ j++; DRW_DBG(", "); }
    } DRW_DBG("\n");
#endif

    dwgBuffer fileHdrBuf(fileHdrdRS, kR2007DecodedFileHeaderSize, &decoder);
    DRW_DBG("\nCRC 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    DRW_DBG("\nunknown key 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    DRW_DBG("\ncomp data CRC 64b= "); DRW_DBGH(fileHdrBuf.getRawLong64());
    std::int32_t fileHdrCompLength = fileHdrBuf.getRawLong32();
    DRW_DBG("\ncompr len 4bytes= "); DRW_DBG(fileHdrCompLength);
    std::int32_t fileHdrCompLength2 = fileHdrBuf.getRawLong32();
    DRW_DBG("\nlength2 4bytes= "); DRW_DBG(fileHdrCompLength2);

    int fileHdrDataLength = static_cast<int>(kR2007FileHeaderDataSize);
    std::vector<std::uint8_t> fileHdrData;
    // The compressed/uncompressed length must fit within the remaining RS-decoded
    // header buffer. Reject obviously
    // bogus values from a failed RS decode.
    constexpr int kMaxFileHdrCompLen =
        static_cast<int>(kR2007DecodedFileHeaderSize);
    if (fileHdrCompLength == 0) {
        DRW_DBG("\nERROR: dwgReader21: empty file header payload\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    if (fileHdrCompLength < 0) {
        if (fileHdrCompLength == std::numeric_limits<std::int32_t>::min()
            || -fileHdrCompLength > kMaxFileHdrCompLen) {
            DRW_DBG("\nERROR: dwgReader21: fileHdrCompLength out of range (uncompressed)\n");
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
        fileHdrDataLength = -fileHdrCompLength;
        if (!DRW::resize(fileHdrData, fileHdrDataLength)) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
        if (!fileHdrBuf.getBytes(fileHdrData.data(), fileHdrDataLength)) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::FileHeader,
                          -1, -1, 0, false, kR2007FileHeaderOffset, true);
            return false;
        }
    }
    else {
        DRW_DBG("\ndwgReader21:: file header are compressed:\n");
        if (fileHdrCompLength > kMaxFileHdrCompLen) {
            DRW_DBG("\nERROR: dwgReader21: fileHdrCompLength out of range (compressed)\n");
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
        std::vector<std::uint8_t> compByteStr;
        if (!DRW::resize(compByteStr, fileHdrCompLength)) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
        if (!fileHdrBuf.getBytes(compByteStr.data(), fileHdrCompLength)) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::FileHeader,
                          -1, -1, 0, false, kR2007FileHeaderOffset, true);
            return false;
        }
        if (!DRW::resize(fileHdrData, fileHdrDataLength)) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
        dwgCompressor comp;
        if (!comp.decompress21(compByteStr.data(), &fileHdrData.front(),
                               fileHdrCompLength, fileHdrDataLength)) {
            recordFailure(DwgIntegrityCheckKind::Decompression,
                          DwgIntegrityPhase::FileHeader);
            return false;
        }
    }

    if (fileHdrData.size() < kR2007FileHeaderDataSize) {
        DRW_DBG("\nERROR: dwgReader21: truncated file header payload\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }

#ifdef DRW_DBG_DUMP
    DRW_DBG("\ndwgReader21::parsed file header:\n");
    for (int i=0, j=0; i<fileHdrDataLength; i++){
        DRW_DBGH( (unsigned char)fileHdrData[i]);
        if (j== 15){ j=0; DRW_DBG("\n");
        } else{ j++; DRW_DBG(", "); }
    } DRW_DBG("\n");
#endif

    dwgBuffer fileHdrDataBuf(fileHdrData.data(), fileHdrDataLength, &decoder);
    const std::uint64_t headerSize = fileHdrDataBuf.getRawLong64();
    const std::uint64_t fileSize = fileHdrDataBuf.getRawLong64();
    const std::uint64_t PagesMapCrcCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nHeader size = "); DRW_DBGH(headerSize);
    DRW_DBG("\nFile size = "); DRW_DBGH(fileSize);
    DRW_DBG("\nPagesMapCrcCompressed = "); DRW_DBGH(PagesMapCrcCompressed);
    std::uint64_t PagesMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapCorrectionFactor = ");
    DRW_DBG(static_cast<unsigned long long>(PagesMapCorrectionFactor));
    const std::uint64_t PagesMapCrcSeedEncoded = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapCrcSeed = "); DRW_DBGH(PagesMapCrcSeedEncoded);
    DRW_DBG("\nPages map2offset = "); DRW_DBGH(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    DRW_DBG("\nPages map2Id = ");
    DRW_DBG(static_cast<unsigned long long>(fileHdrDataBuf.getRawLong64()));
    std::uint64_t PagesMapOffset = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapOffset = "); DRW_DBGH(PagesMapOffset); //relative to data page map 1, add 0x480 to get stream position
    const std::uint64_t PagesMapId = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapId = ");
    DRW_DBG(static_cast<unsigned long long>(PagesMapId));
    DRW_DBG("\nHeader2offset = "); DRW_DBGH(fileHdrDataBuf.getRawLong64()); //relative to data page map 1, add 0x480 to get stream position
    std::uint64_t PagesMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapSizeCompressed = ");
    DRW_DBG(static_cast<unsigned long long>(PagesMapSizeCompressed));
    std::uint64_t PagesMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapSizeUncompressed = ");
    DRW_DBG(static_cast<unsigned long long>(PagesMapSizeUncompressed));
    const std::uint64_t PagesAmount = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesAmount = "); DRW_DBGH(PagesAmount);
    std::uint64_t PagesMaxId = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMaxId = ");
    DRW_DBG(static_cast<unsigned long long>(PagesMaxId));
    DRW_DBG("\nUnknown (normally 0x20) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 0x40) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    const std::uint64_t PagesMapCrcUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nPagesMapCrcUncompressed = "); DRW_DBGH(PagesMapCrcUncompressed);
    DRW_DBG("\nUnknown (normally 0xf800) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 4) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    DRW_DBG("\nUnknown (normally 1) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    const std::uint64_t SectionsAmount = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsAmount (number of sections + 1) = "); DRW_DBGH(SectionsAmount);
    const std::uint64_t SectionsMapCrcUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapCrcUncompressed = "); DRW_DBGH(SectionsMapCrcUncompressed);
    std::uint64_t SectionsMapSizeCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapSizeCompressed = "); DRW_DBGH(SectionsMapSizeCompressed);
    DRW_DBG("\nSectionsMap2Id = ");
    DRW_DBG(static_cast<unsigned long long>(fileHdrDataBuf.getRawLong64()));
    std::uint64_t SectionsMapId = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapId = ");
    DRW_DBG(static_cast<unsigned long long>(SectionsMapId));
    std::uint64_t SectionsMapSizeUncompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapSizeUncompressed = "); DRW_DBGH(SectionsMapSizeUncompressed);
    const std::uint64_t SectionsMapCrcCompressed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapCrcCompressed = "); DRW_DBGH(SectionsMapCrcCompressed);
    std::uint64_t SectionsMapCorrectionFactor = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapCorrectionFactor = ");
    DRW_DBG(static_cast<unsigned long long>(SectionsMapCorrectionFactor));
    const std::uint64_t SectionsMapCrcSeedEncoded = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nSectionsMapCrcSeed = "); DRW_DBGH(SectionsMapCrcSeedEncoded);
    DRW_DBG("\nStreamVersion (normally 0x60100) = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    r2007CrcSeed = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nCrcSeed = "); DRW_DBGH(r2007CrcSeed);
    const std::uint64_t CrcSeedEncoded = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nCrcSeedEncoded = "); DRW_DBGH(CrcSeedEncoded);
    DRW_DBG("\nRandomSeed = "); DRW_DBGH(fileHdrDataBuf.getRawLong64());
    const std::uint64_t HeaderCrc = fileHdrDataBuf.getRawLong64();
    DRW_DBG("\nHeader CRC64 = "); DRW_DBGH(HeaderCrc); DRW_DBG("\n");

    if (!fileHdrDataBuf.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    constexpr std::uint64_t maxSectionDescriptions = 1ULL << 16;
    if (SectionsAmount == 0 || SectionsAmount - 1 > maxSectionDescriptions) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    auto headerForCrc = fileHdrData;
    std::fill(headerForCrc.begin() + 264, headerForCrc.begin() + 272, 0);
    const auto calculatedHeaderCrc = dwgUtil::crc64Normal(
        dwgUtil::updateSeed2(0, headerForCrc.size()),
        headerForCrc.data(), headerForCrc.size());
    if (HeaderCrc != 0 && calculatedHeaderCrc != HeaderCrc) {
        ++m_r2007CrcMismatch;
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Warning,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::FileHeader,
            DwgIntegrityCheckKind::FileHeaderCrc,
            -1, -1, nullptr, 0, false, 0, false, 0, false,
            HeaderCrc, calculatedHeaderCrc, true);
        DRW_DBG("\nWARNING: dwgReader21: file header CRC mismatch\n");
    }
    const std::uint64_t PagesMapCrcSeed = dwgUtil::decodeCrcSeed(
        PagesMapCrcSeedEncoded);
    const std::uint64_t SectionsMapCrcSeed = dwgUtil::decodeCrcSeed(
        SectionsMapCrcSeedEncoded);

    DRW_DBG("\ndwgReader21::parse page map:\n");
    // Sanity-check sizes before allocating to avoid crash if the file header
    // RS decode produced garbage values (e.g. when decode239I can't correct all errors).
    constexpr std::uint64_t kR2007MaxMapSize = 128ULL * 1024ULL * 1024ULL;
    if (PagesMapSizeUncompressed > kR2007MaxMapSize
        || PagesMapSizeCompressed > kR2007MaxMapSize
        || PagesMapSizeUncompressed > std::numeric_limits<std::size_t>::max()
        || PagesAmount > dwgSafety::MaxPageCount
        || PagesMaxId == 0 || PagesMaxId > dwgSafety::MaxPageCount) {
        DRW_DBG("\nERROR: dwgReader21: page-map counts out of range, likely RS decode failure\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap);
        return false;
    }
    if (PagesMapSizeUncompressed == 0 || PagesMapSizeCompressed == 0) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap);
        return false;
    }
    std::vector<std::uint8_t> PagesMapData;
    if (!DRW::resize(PagesMapData,
                     static_cast<int>(PagesMapSizeUncompressed))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap);
        return false;
    }

    std::uint64_t pagesMapAddress = 0;
    if (!dwgSafety::add(kR2007PageMapBaseOffset, PagesMapOffset,
                        pagesMapAddress)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::PageMap);
        return false;
    }
    if (!parseSysPage(PagesMapSizeCompressed, PagesMapSizeUncompressed,
                      PagesMapCorrectionFactor, pagesMapAddress,
                      PagesMapCrcSeed, PagesMapCrcCompressed,
                      PagesMapCrcUncompressed,
                      PagesMapData.data(), DwgIntegrityPhase::PageMap)) {
        return false;
    }

    std::unordered_map<std::uint64_t, dwgPageInfo> sectionPageMapTmp;
    PageMapFailure pageMapFailure = PageMapFailure::None;
    if (!parseSectionPageMap(PagesMapData.data(), PagesMapSizeUncompressed,
                             PagesAmount, PagesMaxId,
                             fileBuf->size(),
                             sectionPageMapTmp, &pageMapFailure)) {
        DwgIntegrityCheckKind kind = DwgIntegrityCheckKind::PageMapReference;
        switch (pageMapFailure) {
        case PageMapFailure::InvalidInput:
        case PageMapFailure::ResourceLimit:
            kind = DwgIntegrityCheckKind::PageGeometry;
            break;
        case PageMapFailure::PageRange:
            kind = DwgIntegrityCheckKind::PageRange;
            break;
        case PageMapFailure::None:
        case PageMapFailure::Truncated:
        case PageMapFailure::NonZeroTail:
        case PageMapFailure::InvalidPageId:
        case PageMapFailure::DuplicatePageId:
        case PageMapFailure::CountMismatch:
            break;
        }
        recordFailure(kind, DwgIntegrityPhase::PageMap);
        return false;
    }
    const auto pagesMapIt = sectionPageMapTmp.find(PagesMapId);
    if (pagesMapIt == sectionPageMapTmp.end()
        || pagesMapIt->second.address != pagesMapAddress) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::PageMap, -1, -1, PagesMapId, true,
                      pagesMapAddress, true);
        return false;
    }
    if (PagesMapId == SectionsMapId) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::PageMap, -1, -1, PagesMapId, true);
        return false;
    }

    DRW_DBG("\n*** dwgReader21: Processing Section Map ***\n");
    if (SectionsMapSizeUncompressed > kR2007MaxMapSize
        || SectionsMapSizeCompressed > kR2007MaxMapSize
        || SectionsMapSizeUncompressed > std::numeric_limits<std::size_t>::max()) {
        DRW_DBG("\nERROR: dwgReader21: SectionsMap sizes out of range\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    if (SectionsMapSizeUncompressed == 0 || SectionsMapSizeCompressed == 0) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    std::vector<std::uint8_t> SectionsMapData;
    if (!DRW::resize(SectionsMapData,
                     static_cast<int>(SectionsMapSizeUncompressed))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    auto sectionMapIt = sectionPageMapTmp.find(SectionsMapId);
    if (sectionMapIt == sectionPageMapTmp.end()) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, SectionsMapId, true);
        return false;
    }
    dwgPageInfo sectionMap = sectionMapIt->second;
    if (!parseSysPage( SectionsMapSizeCompressed, SectionsMapSizeUncompressed,
                       SectionsMapCorrectionFactor, sectionMap.address,
                       SectionsMapCrcSeed, SectionsMapCrcCompressed,
                       SectionsMapCrcUncompressed, SectionsMapData.data(),
                       DwgIntegrityPhase::SectionMap) ) {
        return false;
    }

//reads sections:
    //Note: compressed value are not stored in file then, commpresed field are use to store
    // encoding value
    dwgBuffer SectionsMapBuf( &SectionsMapData.front(), SectionsMapSizeUncompressed, &decoder);
    std::unordered_map<int, dwgSectionInfo> parsedSections;
    std::vector<dwgSectionInfo> parsedUnknownSections;
    std::unordered_set<std::uint32_t> claimedSectionPages;
    const bool reservedMapPages =
        PagesMapId <= std::numeric_limits<std::uint32_t>::max()
        && SectionsMapId <= std::numeric_limits<std::uint32_t>::max()
        && claimedSectionPages.insert(
               static_cast<std::uint32_t>(PagesMapId)).second
        && claimedSectionPages.insert(
               static_cast<std::uint32_t>(SectionsMapId)).second;
    if (!reservedMapPages) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                      DwgIntegrityPhase::SectionMap, -1, -1, SectionsMapId,
                      true);
        return false;
    }
    std::int32_t nextId = 1;
    std::uint64_t sectionCount = 0;
    bool sawEmptySection = false;
    while(SectionsMapBuf.getPosition() < SectionsMapBuf.size()){
        const std::uint64_t mapPosition = SectionsMapBuf.getPosition();
        const std::uint64_t remaining = SectionsMapBuf.size() - mapPosition;
        if (remaining < 64) {
            const auto* tail = SectionsMapData.data() + mapPosition;
            if (!std::all_of(tail, tail + remaining,
                             [](std::uint8_t value) { return value == 0; })) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap);
                return false;
            }
            break;
        }
        const auto* record = SectionsMapData.data() + mapPosition;
        if (std::all_of(record, record + 64, [](std::uint8_t value) {
                return value == 0;
            })) {
            if (!std::all_of(record, record + remaining,
                             [](std::uint8_t value) { return value == 0; })) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap);
                return false;
            }
            break;
        }
        if (++sectionCount > maxSectionDescriptions) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        dwgSectionInfo secInfo;
        secInfo.size = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nSize of section (data size)= "); DRW_DBGH(secInfo.size);
        secInfo.maxSize = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nMax Decompressed Size= "); DRW_DBGH(secInfo.maxSize);
        secInfo.encrypted = SectionsMapBuf.getRawLong64();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        DRW_DBG("\nencription= "); DRW_DBGH(secInfo.encrypted);
        DRW_DBG("\nHashCode = "); DRW_DBGH(SectionsMapBuf.getRawLong64());
        std::uint64_t SectionNameLength = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nSectionNameLength = ");
        DRW_DBG(static_cast<unsigned long long>(SectionNameLength));
        DRW_DBG("\nUnknown = "); DRW_DBGH(SectionsMapBuf.getRawLong64());
        secInfo.compressed = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nEncoding (compressed) = "); DRW_DBGH(secInfo.compressed);
        secInfo.pageCount = SectionsMapBuf.getRawLong64();
        DRW_DBG("\nPage count= "); DRW_DBGH(secInfo.pageCount);
        if (secInfo.size > dwgSafety::MaxBufferSize
            || secInfo.maxSize > dwgSafety::MaxBufferSize
            || (secInfo.compressed != 1 && secInfo.compressed != 4)
            || secInfo.pageCount > dwgSafety::MaxPageCount
            || SectionNameLength > std::numeric_limits<std::uint16_t>::max()
            || (SectionNameLength != 0 && (SectionNameLength < 4 || (SectionNameLength & 1) != 0))) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        if (SectionsMapBuf.getPosition() > SectionsMapBuf.size()
            || SectionNameLength > SectionsMapBuf.size() - SectionsMapBuf.getPosition())
        {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        secInfo.name = SectionsMapBuf.getUCSStr(SectionNameLength);
        if (!SectionsMapBuf.isGood()) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        DRW_DBG("\nSection name = "); DRW_DBG(secInfo.name); DRW_DBG("\n");
        if (secInfo.name.empty()) {
            // The only unnamed R2007 descriptor is the required empty
            // section. It has no pages and terminates the map; any bytes
            // after it are checked as zero padding below.
            if (sawEmptySection || SectionNameLength != 0
                || secInfo.size != 0 || secInfo.pageCount != 0
                || secInfo.encrypted != 0 || secInfo.compressed != 4)
            {
                recordFailure(DwgIntegrityCheckKind::PageGeometry,
                              DwgIntegrityPhase::SectionMap);
                return false;
            }
            sawEmptySection = true;
            break;
        }
        const std::uint64_t pageCap = secInfo.maxSize != 0
            ? secInfo.maxSize : secInfo.size;
        std::uint64_t sectionCapacity = 0;
        if (!dwgSafety::sectionBufferCapacity(
                secInfo.size, secInfo.pageCount, pageCap, sectionCapacity)
            || sectionCapacity > dwgSafety::MaxBufferSize)
        {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        std::vector<std::pair<std::uint64_t, std::uint64_t>> pageRanges;
        if (secInfo.pageCount
                > static_cast<std::uint64_t>(std::numeric_limits<int>::max())
            || !DRW::reserve(pageRanges,
                              static_cast<int>(secInfo.pageCount)))
        {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        for (std::uint64_t i=0; i< secInfo.pageCount; i++){
            if (SectionsMapBuf.size() - SectionsMapBuf.getPosition() < 56) {
                recordFailure(DwgIntegrityCheckKind::PageRange,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id);
                return false;
            }
            const std::uint64_t po = SectionsMapBuf.getRawLong64();
            const std::uint64_t ds = SectionsMapBuf.getRawLong64();
            const std::uint64_t rawPageId = SectionsMapBuf.getRawLong64();
            if (rawPageId > std::numeric_limits<std::uint32_t>::max()) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              rawPageId, true);
                return false;
            }
            const std::uint32_t pn = static_cast<std::uint32_t>(rawPageId);
            DRW_DBG("  pag Id = "); DRW_DBGH(pn); DRW_DBG(" data size = "); DRW_DBGH(ds);
            auto pageIt = sectionPageMapTmp.find(pn);
            if (pageIt == sectionPageMapTmp.end()) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id, pn,
                              true);
                return false;
            }
            if (secInfo.pages.find(pn) != secInfo.pages.end()) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true);
                return false;
            }
            dwgPageInfo pi = pageIt->second; //get a copy
            pi.dataSize = ds;
            pi.startOffset = po;
            pi.uSize = SectionsMapBuf.getRawLong64();
            pi.cSize = SectionsMapBuf.getRawLong64();
            pi.checksum = SectionsMapBuf.getRawLong64();
            pi.crc = SectionsMapBuf.getRawLong64();
            if (ds == 0 || ds > dwgSafety::MaxBufferSize
                || pi.uSize == 0 || pi.cSize == 0 || pi.uSize > ds
                || po > sectionCapacity
                || pi.uSize > sectionCapacity - po
                || po > secInfo.size
                || pi.uSize > secInfo.size - po
                || pi.cSize > pi.size
                || !dwgSafety::range(pi.address, pi.size, fileBuf->size())) {
                recordFailure(DwgIntegrityCheckKind::PageRange,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id, pn,
                              true, pi.address, true);
                return false;
            }
            std::uint64_t pageEnd = 0;
            if (!dwgSafety::add(po, pi.uSize, pageEnd)) {
                recordFailure(DwgIntegrityCheckKind::PageRange,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true, po, true);
                return false;
            }
            for (const auto& range : pageRanges) {
                if (po < range.second && range.first < pageEnd) {
                    recordFailure(DwgIntegrityCheckKind::PageRange,
                                  DwgIntegrityPhase::SectionMap,
                                  secEnum::getEnum(secInfo.name), secInfo.Id,
                                  pn, true, po, true);
                    return false;
                }
            }
            pageRanges.emplace_back(po, pageEnd);
            secInfo.pages[pn]= pi;//complete copy in secInfo
            DRW_DBG("\n    Page number= "); DRW_DBGH(secInfo.pages[pn].Id);
            DRW_DBG("\n    address in file= "); DRW_DBGH(secInfo.pages[pn].address);
            DRW_DBG("\n    size in file= "); DRW_DBGH(secInfo.pages[pn].size);
            DRW_DBG("\n    Data size= "); DRW_DBGH(secInfo.pages[pn].dataSize);
            DRW_DBG("\n    Start offset= "); DRW_DBGH(secInfo.pages[pn].startOffset);
            DRW_DBG("\n    Page uncompressed size = "); DRW_DBGH(secInfo.pages[pn].uSize);
            DRW_DBG("\n    Page compressed size = "); DRW_DBGH(secInfo.pages[pn].cSize);

            DRW_DBG("\n    Page checksum = "); DRW_DBGH(pi.checksum);
            DRW_DBG("\n    Page CRC = "); DRW_DBGH(pi.crc); DRW_DBG("\n");
        }

        const int sectionId = secEnum::getEnum(secInfo.name);
        if (sectionId == secEnum::UNKNOWNS) {
            const auto duplicate = std::find_if(
                parsedUnknownSections.cbegin(), parsedUnknownSections.cend(),
                [&secInfo](const dwgSectionInfo& existing) {
                    return existing.name == secInfo.name;
                });
            if (duplicate != parsedUnknownSections.cend()) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap);
                return false;
            }
        } else if (parsedSections.find(sectionId) != parsedSections.end()) {
            recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        for (const auto& page : secInfo.pages) {
            if (!claimedSectionPages.insert(page.first).second) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap,
                              sectionId, secInfo.Id, page.first, true);
                return false;
            }
        }
        secInfo.Id = nextId++;
        DRW_DBG("Saved section Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
        if (sectionId == secEnum::UNKNOWNS)
            parsedUnknownSections.push_back(std::move(secInfo));
        else
            parsedSections.emplace(sectionId, std::move(secInfo));
    }

    // SectionsAmount includes the empty descriptor that terminates the map.
    if (!SectionsMapBuf.isGood() || !sawEmptySection
        || sectionCount != SectionsAmount) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    const std::uint64_t sectionMapTail = SectionsMapBuf.getPosition();
    if (sectionMapTail > SectionsMapData.size()
        || std::any_of(SectionsMapData.cbegin()
                           + static_cast<std::ptrdiff_t>(sectionMapTail),
                       SectionsMapData.cend(),
                       [](std::uint8_t value) { return value != 0; }))
    {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }

    // The decoded file-size field includes the repeated file-header page at
    // the end of the stream.  Page-map entries are already checked against
    // the actual input length; use their maximum referenced end as the
    // minimum physical envelope and keep trailing bytes a warning-only
    // compatibility case.
    std::uint64_t lastPageEnd = kR2007PageMapBaseOffset;
    for (const auto& entry : sectionPageMapTmp) {
        std::uint64_t pageEnd = 0;
        if (!dwgSafety::add(entry.second.address, entry.second.size,
                            pageEnd)) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::FileHeader,
                          -1, -1, entry.first, true,
                          entry.second.address, true);
            return false;
        }
        lastPageEnd = std::max(lastPageEnd, pageEnd);
    }
    std::uint64_t minimumFileSize = 0;
    if (!dwgSafety::add(lastPageEnd, kR2007FileHeaderPageSize,
                        minimumFileSize)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    if (fileSize < minimumFileSize) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::FileHeader,
            DwgIntegrityCheckKind::PageRange,
            -1, -1, nullptr, 0, false, fileSize, true,
            0, false, minimumFileSize, fileSize, true);
        return false;
    }
    if (fileSize > fileBuf->size()) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::FileHeader,
            DwgIntegrityCheckKind::PageRange,
            -1, -1, nullptr, 0, false, fileSize, true,
            0, false, fileBuf->size(), fileSize, true);
        return false;
    }
    if (fileSize != fileBuf->size()) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Warning,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::FileHeader,
            DwgIntegrityCheckKind::PageRange,
            -1, -1, nullptr, 0, false, fileSize, true,
            0, false, fileSize, fileBuf->size(), true);
    }


    sections.swap(parsedSections);
    m_unknownSections.swap(parsedUnknownSections);

    DRW_DBG("\ndwgReader21::readFileHeader END\n");
    return true;
}

bool dwgReader21::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("\ndwgReader21::readDwgHeader\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::HEADER,
            sectionId, "AcDb:Header");
    };
    const auto sectionIt = sections.find(secEnum::HEADER);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;

    DRW_DBG("\nprepare section of size ");
    DRW_DBG(static_cast<unsigned long long>(si.size));
    DRW_DBG("\n");
    if (si.size == 0 || si.size > dwgSafety::MaxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()
        || si.size > std::numeric_limits<int>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    std::vector<std::uint8_t> tmpHeaderData;
    if (!DRW::resize(tmpHeaderData, static_cast<int>(si.size))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    if (!dwgReader21::parseDataPage( si, &tmpHeaderData.front())) {
        return false;
    }

    dwgBuffer dataBuf(tmpHeaderData.data(), si.size, &decoder);
    dwgBuffer handleBuf(tmpHeaderData.data(), si.size, &decoder);
    DRW_DBG("Header section sentinel= ");
    if (!checkSentinel(&dataBuf, secEnum::HEADER, true)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    if (!dwgReader::readDwgHeader(hdr, &dataBuf, &handleBuf)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    return true;
}

bool dwgReader21::readDwgClasses(){
    DRW_DBG("\ndwgReader21::readDwgClasses");
    beginDwgClassCoverage();
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::CLASSES,
            sectionId, "AcDb:Classes");
    };
    const auto sectionIt = sections.find(secEnum::CLASSES);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;

    DRW_DBG("\nprepare section of size ");
    DRW_DBG(static_cast<unsigned long long>(si.size));
    DRW_DBG("\n");
    if (si.size == 0 || si.size > dwgSafety::MaxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()
        || si.size > std::numeric_limits<int>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    std::vector<std::uint8_t> tmpClassesData;
    if (!DRW::resize(tmpClassesData, static_cast<int>(si.size))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    bool ret = dwgReader21::parseDataPage(si, tmpClassesData.data());
    if (!ret) {
        return ret;
    }

    dwgBuffer buff(tmpClassesData.data(), si.size, &decoder);
    DRW_DBG("classes section sentinel= ");
    if (!checkSentinel(&buff, secEnum::CLASSES, true)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    std::uint32_t size = buff.getRawLong32();
    DRW_DBG("\ndata size in bytes "); DRW_DBG(size);

    std::uint32_t bitSize = buff.getRawLong32();
    DRW_DBG("\ntotal size in bits "); DRW_DBG(bitSize);

    std::uint32_t maxClassNum = buff.getBitShort();
    DRW_DBG("\nMaximum class number "); DRW_DBG(maxClassNum);
    DRW_DBG("\nRc 1 "); DRW_DBG(buff.getRawChar8());
    DRW_DBG("\nRc 2 "); DRW_DBG(buff.getRawChar8());
    DRW_DBG("\nBit "); DRW_DBG(buff.getBit());
    if (!buff.isGood() || maxClassNum < 499) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    std::uint64_t crcPosition = 0;
    std::uint64_t classDataEndBit = 0;
    std::uint64_t sectionEndBit = 0;
    if (!dwgSafety::add(size, 20, crcPosition)
        || !dwgSafety::range(crcPosition, 2, buff.size())
        || crcPosition < 16
        || crcPosition > static_cast<std::uint64_t>(
               std::numeric_limits<std::int32_t>::max() - 1)
        || !dwgSafety::multiply(crcPosition, 8, classDataEndBit)
        || !dwgSafety::multiply(si.size, 8, sectionEndBit)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    std::uint64_t declaredDataEndBit = 0;
    if (!dwgSafety::multiply(size, 8, declaredDataEndBit)
        || bitSize > declaredDataEndBit) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    classDataEndBit = bitSize;

    // The section buffer also contains the CRC, optional trailer bytes, and
    // the END sentinel. Keep class fields inside the RL-declared payload;
    // otherwise malformed class counts or strings can consume trailer bytes
    // and still appear valid because the backing buffer is larger.
    const auto cursorWithinClassData = [classDataEndBit](
            const dwgBuffer& value) {
        std::uint64_t bitPosition = 0;
        return value.isGood()
            && dwgSafety::multiply(value.getPosition(), 8, bitPosition)
            && dwgSafety::add(bitPosition, value.getBitPos(), bitPosition)
            && bitPosition <= classDataEndBit;
    };
    if (!cursorWithinClassData(buff)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    /*******************************/
    //prepare string stream
    dwgBuffer strBuff(tmpClassesData.data(), si.size, &decoder);
    const auto cursorWithinStringData = [](const dwgBuffer& value,
                                           std::uint64_t endBit) {
        std::uint64_t bitPosition = 0;
        return value.isGood()
            && dwgSafety::multiply(value.getPosition(), 8, bitPosition)
            && dwgSafety::add(bitPosition, value.getBitPos(), bitPosition)
            && bitPosition <= endBit;
    };
    std::uint64_t footerEndBit = 0;
    std::uint64_t stringStartBit = 0;
    std::uint64_t stringSize = 0;
    if (!dwgSafety::add(bitSize, 159, footerEndBit)
        || !readDwgClassStringFooter(strBuff, footerEndBit,
                                      stringStartBit, stringSize)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    std::uint64_t stringDataEndBit = 0;
    if (!dwgSafety::add(stringStartBit, stringSize, stringDataEndBit)
        || stringDataEndBit > sectionEndBit
        || stringDataEndBit > std::numeric_limits<std::uint64_t>::max() - 7) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    const std::uint64_t stringBufferSize = (stringDataEndBit + 7) / 8;
    if (stringBufferSize == 0
        || stringBufferSize > std::numeric_limits<std::size_t>::max()
        || stringBufferSize > std::numeric_limits<int>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    crcPosition = stringBufferSize;
    strBuff = dwgBuffer(tmpClassesData.data(), stringBufferSize, &decoder);
    DRW_DBG("\nclass string start bit: ");
    DRW_DBG(static_cast<unsigned long long>(stringStartBit));
    DRW_DBG("\nclass string size: ");
    DRW_DBG(static_cast<unsigned long long>(stringSize));
    if (!strBuff.setPosition(stringStartBit >> 3)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    strBuff.setBitPos(static_cast<std::uint8_t>(stringStartBit & 7));
    if (!cursorWithinStringData(strBuff, stringDataEndBit)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }


    /*******************************/

    std::uint32_t endDataPos = maxClassNum-499;
    DRW_DBG("\nbuff.getPosition: "); DRW_DBG(buff.getPosition());
    std::vector<DwgStagedClass> stagedClasses;
    std::unordered_map<std::uint32_t, DRW_Class*> stagedMap;
    if (endDataPos > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(stagedClasses, static_cast<int>(endDataPos))
        || !DRW::reserve(stagedMap, static_cast<int>(endDataPos))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    for (std::uint32_t i= 0; i<endDataPos;i++) {
        std::uint64_t dataRangeStart = 0;
        std::uint64_t stringRangeStart = 0;
        if (!cursorWithinClassData(buff)
            || !dwgClassBitPosition(buff, dataRangeStart)
            || !dwgClassBitPosition(strBuff, stringRangeStart)) {
            DRW_DwgClassCoverageEntry failed;
            failed.m_sectionDescriptorId = si.Id;
            recordDwgClassCoverageFailure(
                std::move(failed), DRW_DwgClassCoverageReason::Bounds);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        auto cl = std::make_unique<DRW_Class>();
        const bool parsed = cl->parseDwg(version, &buff, &strBuff);
        const bool classNumberValid = cl->classNum >= 500;
        const bool classIsNew = classesmap.find(cl->classNum) == classesmap.end()
            && stagedMap.find(cl->classNum) == stagedMap.end();
        const bool dataCursorValid = cursorWithinClassData(buff);
        const bool stringCursorValid = cursorWithinStringData(
            strBuff, stringDataEndBit);
        std::uint64_t dataRangeEnd = 0;
        std::uint64_t stringRangeEnd = 0;
        if (!parsed || !dataCursorValid || !stringCursorValid
            || !dwgClassBitPosition(buff, dataRangeEnd)
            || !dwgClassBitPosition(strBuff, stringRangeEnd)) {
            DRW_DwgClassCoverageEntry failed;
            failed.m_sectionDescriptorId = si.Id;
            recordDwgClassCoverageFailure(
                std::move(failed), DRW_DwgClassCoverageReason::Parse);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        DRW_DwgClassCoverageEntry coverage;
        try {
            coverage = makeDwgClassCoverageEntry(*cl, si.Id);
        } catch (...) {
            m_dwgClassCoverageCaptureFailed = true;
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        if (!setDwgClassBitRange(
                coverage.m_dataRange, dataRangeStart, dataRangeEnd,
                DRW_DwgFrameOffsetSpace::DecodedBuffer, true)
            || !setDwgClassBitRange(
                coverage.m_stringRange, stringRangeStart, stringRangeEnd,
                DRW_DwgFrameOffsetSpace::DecodedBuffer, true)) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Bounds);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        if (!classNumberValid) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Parse);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        if (!classIsNew) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Duplicate);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        try {
            stagedMap.emplace(cl->classNum, cl.get());
        } catch (...) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Publish);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        if (!stageDwgClass(stagedClasses, std::move(cl), std::move(coverage))) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        DRW_DBG("\nbuff.getPosition: "); DRW_DBG(buff.getPosition());
    }
    DRW_DBG("\nend classes data buff.getPosition: "); DRW_DBG(buff.getPosition());
    DRW_DBG("\nend classes data buff.getBitPos: "); DRW_DBG(buff.getBitPos());

    if (!buff.setPosition(crcPosition)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    const std::uint16_t crcRead = buff.getRawShort16();
    if (!buff.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    const std::uint16_t crcCalc = buff.crc8(
        0xc0c1, 16, static_cast<std::int32_t>(crcPosition));
    if (crcRead != 0 && crcCalc != crcRead) {
        ++m_classesCrcMismatch;
        try {
            DwgIntegrityDiagnostic diagnostic;
            diagnostic.severity = DwgIntegritySeverity::Warning;
            diagnostic.offsetSpace = DwgIntegrityAddressSpace::DecodedBuffer;
            diagnostic.phase = DwgIntegrityPhase::SectionParser;
            diagnostic.kind = DwgIntegrityCheckKind::ClassesCrc;
            diagnostic.logicalSectionId = secEnum::CLASSES;
            diagnostic.sectionDescriptorId = si.Id;
            diagnostic.sectionName = si.name;
            diagnostic.expected = crcRead;
            diagnostic.observed = crcCalc;
            diagnostic.hasExpected = true;
            diagnostic.hasObserved = true;
            addIntegrityDiagnostic(std::move(diagnostic));
        } catch (...) {
            // Integrity reporting is best-effort and must not alter parsing.
        }
        DRW_DBG("\nWARNING dwgReader21::readDwgClasses CRC mismatch: calc=");
        DRW_DBGH(crcCalc); DRW_DBG(" read="); DRW_DBGH(crcRead); DRW_DBG("\n");
    }
    DRW_DBG("\nCRC: "); DRW_DBGH(crcRead);
    if (!readDwgClassesTail(buff)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    DRW_DBG("\nclasses section end sentinel= ");
    // 1.4: this is the END sentinel — it was wrongly passed start=true, so it
    // compared the END bytes against the BEGIN sentinel (a latent bug). Fix
    // the arg to false. Kept WARN-ONLY: AC1024 (reader24→here) valid corpus
    // files do not carry a matching CLASSES end sentinel in libdxfrw's
    // single-buffer model, so a hard fail regresses the corpus. The arg fix
    // makes the diagnostic correct without changing the read outcome.
    checkSentinel(&buff, secEnum::CLASSES, false);
    if (!buff.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    if (!publishDwgClasses(stagedClasses)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    return true;
}


bool dwgReader21::readDwgHandles(){
    DRW_DBG("\ndwgReader21::readDwgHandles");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::ObjectMap, kind, secEnum::HANDLES,
            sectionId, "AcDb:Handles");
    };
    const auto sectionIt = sections.find(secEnum::HANDLES);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;
    const auto objectsIt = sections.find(secEnum::OBJECTS);
    if (objectsIt == sections.end() || objectsIt->second.Id < 0
        || objectsIt->second.size == 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference, si.Id);
        return false;
    }

    DRW_DBG("\nprepare section of size ");
    DRW_DBG(static_cast<unsigned long long>(si.size));
    DRW_DBG("\n");
    if (si.size == 0 || si.size > dwgSafety::MaxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()
        || si.size > std::numeric_limits<int>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    std::vector<std::uint8_t> tmpHandlesData;
    if (!DRW::resize(tmpHandlesData, static_cast<int>(si.size))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    bool ret = dwgReader21::parseDataPage(si, tmpHandlesData.data());
    if (!ret)
        return ret;

    dwgBuffer dataBuf(tmpHandlesData.data(), si.size, &decoder);

    ret = dwgReader::readDwgHandles(
        &dataBuf, 0, si.size, objectsIt->second.size,
        DwgIntegrityAddressSpace::DecodedBuffer, si.Id);
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader21::readDwgTables(DRW_Header& hdr) {
    DRW_DBG("\ndwgReader21::readDwgTables\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::OBJECTS,
            sectionId, "AcDb:AcDbObjects");
    };
    objData.reset();
    dataSize = 0;
    const auto sectionIt = sections.find(secEnum::OBJECTS);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;

    DRW_DBG("\nprepare section of size ");
    DRW_DBG(static_cast<unsigned long long>(si.size));
    DRW_DBG("\n");
    if (si.size == 0 || si.size > dwgSafety::MaxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    std::unique_ptr<std::uint8_t[]> stagedObjectData(
        new (std::nothrow) std::uint8_t[static_cast<std::size_t>(si.size)]);
    if (!stagedObjectData) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    if (!dwgReader21::parseDataPage(si, stagedObjectData.get()))
        return false;

    DRW_DBG("readDwgTables total data size= "); DRW_DBG(si.size); DRW_DBG("\n");
    dwgBuffer dataBuf(stagedObjectData.get(), si.size, &decoder);
    if (!dwgReader::readDwgTables(
            hdr, &dataBuf, DwgIntegrityAddressSpace::DecodedBuffer))
        return false;
    if (!captureRawDwgDataSections()) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference, si.Id);
        return false;
    }

    objData = std::move(stagedObjectData);
    dataSize = si.size;
    return true;
}

bool dwgReader21::captureRawDwgDataSections() {
    std::vector<DRW_RawDwgSection> stagedRawSections;
    std::vector<DRW_DataStorageSection> stagedDataStorageSections;
    const auto captureSection = [&](const dwgSectionInfo& si,
                                    const char* fallbackName,
                                    bool parseDataStorage) -> bool {
        if (si.size > dwgSafety::MaxBufferSize
            || si.size > std::numeric_limits<std::size_t>::max())
            return false;

        DRW_RawDwgSection section;
        section.m_name = si.name.empty() && fallbackName != nullptr
            ? fallbackName : si.name;
        if (section.m_name.empty())
            return false;
        section.m_version = version;
        section.m_encoding = si.compressed;
        section.m_encrypted = si.encrypted;
        section.m_maxSize = si.maxSize;
        if (si.size != 0) {
            std::vector<std::uint8_t> data;
            if (!DRW::resize(data, static_cast<int>(si.size)))
                return false;
            if (!parseDataPage(si, data.data()))
                return false;
            section.m_data = std::move(data);
        }

        if (parseDataStorage) {
            // A present zero-length prototype section is the valid empty
            // DataStorage form. It has no header or records to index.
            DRW_DataStorageSection typed;
            if (section.m_data.empty()) {
                typed.m_version = version;
                typed.sectionByteLength = 0;
            } else {
                typed = DRW_parseDataStorage(section.m_data, version);
            }
            typed.m_name = section.m_name;
            typed.m_version = version;
            // A structural DataStorage failure invalidates the section's
            // owner links. Do not publish a partial index and then continue
            // to modeler callbacks that would expose dangling presence bits.
            if (typed.parseFailed || !typed.structurallyValid
                || !typed.replayAllowed) {
                return false;
            }
            stagedDataStorageSections.push_back(std::move(typed));
        }
        populateVbaProjectSectionView(section);
        if (section.m_name == "AcDb:VBAProject"
            && !section.m_hasVbaProjectView)
            return false;
        stagedRawSections.push_back(std::move(section));
        return true;
    };

    const auto capture = [&](secEnum::DWGSection sectionEnum,
                             const char* fallbackName,
                             bool parseDataStorage) -> bool {
        const auto it = sections.find(sectionEnum);
        return it == sections.end() || it->second.Id < 0
            || captureSection(it->second, fallbackName, parseDataStorage);
    };

    if (!capture(secEnum::PROTOTYPE, "AcDb:AcDsPrototype_1b", true))
        return false;
    if (!capture(secEnum::VBAPROY, "AcDb:VBAProject", false))
        return false;
    for (const dwgSectionInfo& section : m_unknownSections) {
        if (!captureSection(section, nullptr, false))
            return false;
    }
    m_rawDwgSections = std::move(stagedRawSections);
    m_dataStorageSections = std::move(stagedDataStorageSections);
    return true;
}


bool dwgReader21::readDwgBlocks(DRW_Interface& intfa){
    if (objData == nullptr || dataSize == 0)
        return false;
    dwgBuffer dataBuf(objData.get(), dataSize, &decoder);
    return dwgReader::readDwgBlocks(
        intfa, &dataBuf, DwgIntegrityAddressSpace::DecodedBuffer);
}
