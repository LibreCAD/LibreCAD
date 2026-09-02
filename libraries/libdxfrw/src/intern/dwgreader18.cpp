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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <new>
#include <string>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "dwgreader18.h"
#include "dwgutil.h"
#include "dwgsafety.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"

void dwgReader18::genMagicNumber(){
    int size =0x114;
    std::vector<std::uint8_t> tmpMagicStr(size);
    std::uint8_t *p = tmpMagicStr.data();
    int rSeed =1;
    while (size--) {
        rSeed *= 0x343fd;
        rSeed += 0x269ec3;
        *p++ = static_cast<std::uint8_t>(rSeed >> 0x10);
    }
    int j = 0;
    size =0x114;
    for (int i=0; i< size;i++) {
        DRW_DBGH(tmpMagicStr[i]);
        if (j == 15) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
}

std::uint32_t dwgReader18::checksum(std::uint32_t seed, std::uint8_t* data, std::uint64_t sz){
    return dwgUtil::checksum18(seed, data, sz);
}

bool dwgReader18::parseSectionPageMap(
    std::uint8_t* data, std::uint64_t size,
    std::uint64_t firstAddress,
    std::unordered_map<std::uint32_t, dwgPageInfo>& pages,
    std::uint64_t fileSize) {
    if (data == nullptr || size == 0)
        return false;

    dwgBuffer buffer(data, size);
    std::unordered_map<std::uint32_t, dwgPageInfo> parsedPages;
    std::uint64_t address = firstAddress;
    std::uint64_t offset = 0;
    std::uint64_t recordCount = 0;
    while (offset < size) {
        if (size - offset < 8 || ++recordCount > dwgSafety::MaxPageCount)
            return false;

        const std::int32_t id = buffer.getRawLong32();
        const std::uint32_t pageSize = buffer.getRawLong32();
        offset += 8;
        if (!buffer.isGood() || id == 0)
            return false;

        std::uint64_t pageEnd = 0;
        if (!dwgSafety::add(address, pageSize, pageEnd)
            // Negative records describe unreferenced physical gaps. Some
            // legacy files retain a declared trailing gap past EOF; only
            // addressable positive pages must fit in the supplied input.
            || (id > 0 && pageEnd > fileSize))
            return false;

        if (id < 0) {
            // A negative number is an unused physical gap. Its four tree
            // fields are metadata only and must not be addressable by a
            // section descriptor. Although the specification describes the
            // final field as zero, real AutoCAD files use it for another
            // signed tree link; all four fields are therefore opaque here.
            if (size - offset < 16)
                return false;
            buffer.getRawLong32(); // parent
            buffer.getRawLong32(); // left
            buffer.getRawLong32(); // right
            buffer.getRawLong32(); // reserved/tree link
            if (!buffer.isGood())
                return false;
            offset += 16;
        } else {
            const auto pageId = static_cast<std::uint32_t>(id);
            if (!parsedPages.emplace(
                    pageId, dwgPageInfo(pageId, address, pageSize)).second)
                return false;
        }

        address = pageEnd;
    }

    if (!buffer.isGood())
        return false;
    pages.swap(parsedPages);
    return true;
}

//called: Section page map: 0x41630e3b
bool dwgReader18::parseSysPage(
    std::uint8_t *decompSec, std::uint32_t decompSize,
    DwgIntegrityPhase phase, std::uint64_t pageId, bool hasPageId,
    std::uint64_t pageOffset, bool hasPageOffset) {
    DRW_DBG("\nparseSysPage:\n ");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile, phase, kind,
            -1, -1, nullptr, pageId, hasPageId, pageOffset, hasPageOffset);
    };
    const auto recordChecksum = [&](std::uint32_t expected,
                                    std::uint32_t observed) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile, phase,
            DwgIntegrityCheckKind::SystemPageChecksum,
            -1, -1, nullptr, pageId, hasPageId, pageOffset, hasPageOffset,
            0, false, expected, observed, true);
    };
    constexpr std::uint32_t maxSystemPageSize = 128U * 1024U * 1024U;
    if (decompSec == nullptr || decompSize == 0
        || decompSize > maxSystemPageSize) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::uint32_t compSize = fileBuf->getRawLong32();
    DRW_DBG("Compressed size= "); DRW_DBG(compSize); DRW_DBG(", "); DRW_DBGH(compSize);
    std::uint32_t compType = fileBuf->getRawLong32();
    DRW_DBG("\nCompression type= "); DRW_DBGH(compType);
    DRW_DBG("\nSection page checksum= "); DRW_DBGH(fileBuf->getRawLong32()); DRW_DBG("\n");
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    if (compSize > dwgSafety::MaxBufferSize) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }

    std::uint8_t hdrData[20];
    if (!fileBuf->moveBitPos(-160) || !fileBuf->getBytes(hdrData, 20)) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    std::uint32_t storedChecksum = static_cast<std::uint32_t>(hdrData[16])
                           | (static_cast<std::uint32_t>(hdrData[17]) << 8)
                           | (static_cast<std::uint32_t>(hdrData[18]) << 16)
                           | (static_cast<std::uint32_t>(hdrData[19]) << 24);
    for (std::uint8_t i= 16; i<20; ++i)
        hdrData[i]=0;
    std::uint32_t calcsH = checksum(0, hdrData, 20);
    DRW_DBG("Calc hdr checksum= "); DRW_DBGH(calcsH);
    if (!dwgSafety::range(fileBuf->getPosition(), compSize, fileBuf->size())) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    std::vector<std::uint8_t> tmpCompSec;
    if (!DRW::resize(tmpCompSec, static_cast<int>(compSize))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (!fileBuf->getBytes(tmpCompSec.data(), compSize)) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    std::uint32_t calcsD = checksum(calcsH, tmpCompSec.data(), compSize);
    DRW_DBG("\nCalc data checksum= "); DRW_DBGH(calcsD); DRW_DBG("\n");
    if (calcsD != storedChecksum) {
        recordChecksum(storedChecksum, calcsD);
        DRW_DBG("parseSysPage: checksum mismatch: stored="); DRW_DBGH(storedChecksum);
        DRW_DBG(" computed="); DRW_DBGH(calcsD); DRW_DBG("\n");
        return false;
    }

    if (compType == 1) {
        // type 1 = store (no compression): size must match
        if (compSize != decompSize) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("parseSysPage: stored page compSize != decompSize\n");
            return false;
        }
        std::copy(tmpCompSec.begin(), tmpCompSec.end(), decompSec);
    } else {
        DRW_DBG("decompressing "); DRW_DBG(compSize); DRW_DBG(" bytes in "); DRW_DBG(decompSize); DRW_DBG(" bytes\n");
        dwgCompressor comp;
        if (!comp.decompress18(tmpCompSec.data(), decompSec, compSize, decompSize)) {
            recordFailure(DwgIntegrityCheckKind::Decompression);
            return false;
        }
        // System pages (page map, section map) are FIXED-SIZE: the whole
        // decompSize must be produced. Unlike data pages (input-bounded),
        // a short fill here means a truncated page map -> reject, rather
        // than silently walking a partially-zeroed map.
        if (comp.decompressedBytes() != decompSize) {
            recordFailure(DwgIntegrityCheckKind::Decompression);
            DRW_DBG("parseSysPage: short decompression (" );
            DRW_DBG(comp.decompressedBytes()); DRW_DBG(" of "); DRW_DBG(decompSize); DRW_DBG(")\n");
            return false;
        }
    }
    return true;
}

 //called ???: Section map: 0x4163003b
bool dwgReader18::parseDataPage(const dwgSectionInfo &si/*, std::uint8_t *dData*/){
    return parseDataPage(si, objData, uncompSize);
}

bool dwgReader18::parseDataPage(const dwgSectionInfo& si,
                                std::unique_ptr<std::uint8_t[]>& sectionData,
                                std::uint64_t& sectionSize) {
    DRW_DBG("\nparseDataPage\n ");
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
                                     std::uint32_t expected,
                                     std::uint32_t observed) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::DataPage, kind, logicalSectionId, si.Id,
            si.name.c_str(), page.Id, true, page.address, true,
            0, false, expected, observed, true);
    };
    if (si.size == 0
        || (si.compressed != 1 && si.compressed != 2)) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    // Buffer model (libreDWG read_2004_compressed_section): the decompressed
    // buffer is num_pages × maxSize ("Max Decompressed Size" from the section
    // descriptor; 0x7400 in real AutoCAD files) and page k's startOffset is
    // k × maxSize. Each page decompresses INPUT-BOUNDED (by its compressed
    // data) with an output window of maxSize at its startOffset. The previous
    // model — buffer of si.size, output capped at the page header's uSize —
    // truncated real sections (e.g. the CLASSES string stream lives past
    // uSize) and rejected multi-page sections whose startOffset (k × 0x7400)
    // exceeds si.size. libdxfrw's own writer emits maxSize == dataSize with a
    // single page at offset 0, so for round-trip files this degenerates to
    // the old sizing.
    const std::uint64_t pageCap = (si.maxSize != 0) ? si.maxSize : si.size;
    // Crafted-file hardening. Guard the multiply BEFORE the total-size cap so
    // a huge pageCount x maxSize descriptor cannot wrap std::uint64_t past the
    // check. For multi-page geometry (the k x maxSize model of real AutoCAD
    // files, maxSize normally 0x7400) additionally cap the per-page window at
    // libreDWG's loosest per-type max; single-page sections keep only the
    // total cap because libdxfrw's own writer legitimately emits one page with
    // maxSize == dataSize (which can exceed any per-page bound).
    constexpr std::uint64_t maxPageCap  = dwgSafety::MaxPageCap;
    constexpr std::uint64_t maxBufSize  = dwgSafety::MaxBufferSize;
    std::uint64_t decodedPageCap = pageCap;
    for (const auto& entry : si.pages)
        decodedPageCap = std::max(decodedPageCap, entry.second.size);
    if (decodedPageCap == 0 || decodedPageCap > maxBufSize
        || (si.pages.size() > 1 && decodedPageCap > maxPageCap)
        || si.pages.size() > maxBufSize / decodedPageCap) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        DRW_DBG("parseDataPage: invalid page geometry (pageCap/pageCount)\n");
        return false;
    }
    // ODA p.29: all-zero pages are NOT written to file, so pages.size() counts
    // only the WRITTEN pages — a section with trailing zero pages needs a
    // buffer large enough for its declared si.size (rounded up to the decoded
    // page capacity), not
    // just written_pages × pageCap. (No zero-page section exists in the current
    // corpus, where si.size == written_pages × pageCap, so this is a no-op there.)
    std::uint64_t bufSize = 0;
    if (!dwgSafety::sectionBufferCapacity(si.size, si.pages.size(),
                                          decodedPageCap, bufSize)) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    if (bufSize == 0 || bufSize > maxBufSize) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        DRW_DBG("parseDataPage: invalid decompression buffer size\n");
        return false;
    }
    // Keep the caller's decoded state intact until every page succeeds. This
    // matters when a reader is retried after malformed input.
    std::unique_ptr<std::uint8_t[]> decodedData(
        new (std::nothrow) std::uint8_t[static_cast<std::size_t>(bufSize)]);
    if (!decodedData) {
        recordSectionFailure(DwgIntegrityCheckKind::PageGeometry);
        return false;
    }
    std::fill(decodedData.get(), decodedData.get() + bufSize, 0);

    // libdxfrw produces AC1018-format data pages even for AC1024/AC1027
    // (it has no Reed-Solomon ENCODER), whereas real AutoCAD R2010+ files
    // RS-encode each page. The page format therefore cannot be decided from
    // the version alone — it must be detected per page. An AC1018-format
    // compressed data page carries the section-page-type magic 0x4163043b in
    // its 32-byte (address-keyed, decrypted) header; an RS-encoded page's
    // first 32 bytes are RS codewords that decrypt to a non-matching value.
    // So: magic match -> AC1018 decode (libdxfrw's own files + real R2004);
    // else, for AC1021+, -> Reed-Solomon decode (real R2010+).
    constexpr std::uint32_t ac18DataPageType = 0x4163043b;
    for (auto it=si.pages.begin(); it!=si.pages.end(); ++it){
        dwgPageInfo pi = it->second;
        if (pi.size < 32 || pi.size > dwgSafety::MaxBufferSize
            || pi.startOffset > bufSize
            || pi.size > std::numeric_limits<std::size_t>::max()
            || !dwgSafety::range(pi.address, pi.size, fileBuf->size())) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            DRW_DBG("parseDataPage: raw page range exceeds file size\n");
            return false;
        }
        if (!fileBuf->setPosition(pi.address)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            return false;
        }
        std::uint8_t hdrData[32];
        if (!fileBuf->getBytes(hdrData, 32)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            return false;
        }
        dwgCompressor::decrypt18Hdr(hdrData, 32, pi.address);
        std::uint32_t pageType = static_cast<std::uint32_t>(hdrData[0])
                            | (static_cast<std::uint32_t>(hdrData[1]) << 8)
                            | (static_cast<std::uint32_t>(hdrData[2]) << 16)
                            | (static_cast<std::uint32_t>(hdrData[3]) << 24);

        if (pageType != ac18DataPageType) {
            // Not an AC1018 page. For AC1021+ this is a Reed-Solomon-encoded
            // page (real AutoCAD R2010+); decode the whole page then
            // decompress21. (Pre-AC1021 has no other page format -> error.)
            if (version < DRW::AC1021) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageMapReference);
                DRW_DBG("parseDataPage: bad page type "); DRW_DBGH(pageType); DRW_DBG("\n");
                return false;
            }
            if (pi.startOffset > bufSize
                || pi.uSize > bufSize - pi.startOffset
                || pi.startOffset > si.size
                || pi.uSize > si.size - pi.startOffset) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                DRW_DBG("parseDataPage (AC1021+): page range exceeds buffer size\n");
                return false;
            }
            if (!fileBuf->setPosition(pi.address)) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
                return false;
            }
            if (pi.size > std::numeric_limits<std::size_t>::max()) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
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
                DRW_DBG("parseDataPage (AC1021+): short read of page data\n");
                return false;
            }
            std::vector<std::uint8_t> tmpPageRS;
            if (!DRW::resize(tmpPageRS, static_cast<int>(pi.size))) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                return false;
            }
            std::uint64_t alignedCSize = 0;
            std::uint64_t chunksNumerator = 0;
            if (!dwgSafety::alignUp8(pi.cSize, alignedCSize)
                || !dwgSafety::add(alignedCSize, 250, chunksNumerator))
            {
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
            // AC1024+ uses the R2004-style section map, where this field is
            // the ordinary 1/2 compression flag. The RS fallback still uses
            // the interleaved data-page layout. Encoding 1/4 is specific to
            // the AC1021 R2007 section map handled by dwgReader21.
            if (si.compressed != 1 && si.compressed != 2) {
                recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                DRW_DBG("parseDataPage (AC1021+): invalid section compression flag\n");
                return false;
            }
            if (!dwgRSCodec::decode251I(
                    tmpPageRaw.data(), tmpPageRS.data(), chunks)) {
                recordPageFailure(pi, DwgIntegrityCheckKind::ReedSolomonDecode);
                DRW_DBG("parseDataPage (AC1021+): RS decode failed\n");
                return false;
            }
            std::uint8_t *pageData = decodedData.get() + pi.startOffset;
            if (pi.cSize < pi.uSize) {
                dwgCompressor comp;
                if (!comp.decompress21(tmpPageRS.data(), pageData, pi.cSize, pi.uSize)) {
                    recordPageFailure(pi, DwgIntegrityCheckKind::Decompression);
                    return false;
                }
            } else {
                // Stored (incompressible) page: cSize >= uSize means the writer left
                // this page uncompressed because LZ77 would not shrink it. Copy the
                // RS-decoded payload verbatim rather than running the LZ77 decoder on
                // non-LZ77 bytes. Mirrors LibreDWG read_data_page (decode_r2007.c):
                // size_comp < size_uncomp -> decompress; else memcpy.
                if (pi.uSize > decodedSize) {
                    recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
                    DRW_DBG("parseDataPage (AC1021+): stored page uSize exceeds raw page size\n");
                    return false;
                }
                std::memcpy(pageData, tmpPageRS.data(), pi.uSize);
            }
            continue;
        }

        // ---- AC1018 page format: 32-byte encrypted header + compressed data ----
        if (pi.startOffset > bufSize
            || decodedPageCap > bufSize - pi.startOffset) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("parseDataPage: page range exceeds buffer size\n");
            return false;
        }
        DRW_DBG("Section  "); DRW_DBG(si.name); DRW_DBG(" page header=\n");
        for (unsigned int i=0, j=0; i< 32;i++) {
            DRW_DBGH( static_cast<unsigned char>(hdrData[i]));
            if (j == 7) {
                DRW_DBG("\n");
                j = 0;
            } else {
                DRW_DBG(", ");
                j++;
            }
        } DRW_DBG("\n");

        DRW_DBG("\n    Page number= "); DRW_DBGH(pi.Id);
        DRW_DBG("\n    size in file= "); DRW_DBGH(pi.size);
        DRW_DBG("\n    address in file= "); DRW_DBGH(pi.address);
        DRW_DBG("\n    Data size= "); DRW_DBGH(pi.dataSize);
        DRW_DBG("\n    Start offset= "); DRW_DBGH(pi.startOffset); DRW_DBG("\n");
        dwgBuffer bufHdr(hdrData, 32, &decoder);
        DRW_DBG("      section page type= "); DRW_DBGH(bufHdr.getRawLong32());
        const std::uint32_t headerSectionNumber = bufHdr.getRawLong32();
        DRW_DBG("\n      section number= "); DRW_DBGH(headerSectionNumber);
        pi.cSize = bufHdr.getRawLong32();
        DRW_DBG("\n      data size (compressed)= ");
        DRW_DBGH(pi.cSize);
        DRW_DBG(" dec ");
        DRW_DBG(static_cast<unsigned long long>(pi.cSize));
        pi.uSize = bufHdr.getRawLong32();
        DRW_DBG("\n      page size (decompressed)= ");
        DRW_DBGH(pi.uSize);
        DRW_DBG(" dec ");
        DRW_DBG(static_cast<unsigned long long>(pi.uSize));
        std::uint32_t headerStartOffset = bufHdr.getRawLong32();
        DRW_DBG("\n      start offset (in decompressed buffer)= "); DRW_DBGH(headerStartOffset);
        DRW_DBG("\n      unknown= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      header checksum= "); DRW_DBGH(bufHdr.getRawLong32());
        DRW_DBG("\n      data checksum= "); DRW_DBGH(bufHdr.getRawLong32()); DRW_DBG("\n");

        //get compressed data
        std::uint64_t payloadOffset = 0;
        if (!dwgSafety::add(pi.address, 32, payloadOffset)
            || !bufHdr.isGood()
            || si.Id < 0
            || headerSectionNumber != static_cast<std::uint32_t>(si.Id)
            || pi.cSize > pi.size - 32
            || pi.uSize > decodedPageCap
            || pi.startOffset >= si.size
            || (si.compressed == 1 && pi.cSize != pi.uSize)
            || (pi.dataSize != 0 && pi.dataSize != pi.cSize)
            || !dwgSafety::range(payloadOffset, pi.cSize, fileBuf->size())) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            DRW_DBG("parseDataPage: compressed page extends past end of file\n");
            return false;
        }
        std::vector<std::uint8_t> cData;
        if (pi.cSize > std::numeric_limits<int>::max()
            || !DRW::resize(cData, static_cast<int>(pi.cSize))) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            return false;
        }
        if (!fileBuf->setPosition(payloadOffset)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            return false;
        }
        if (!fileBuf->getBytes(cData.data(), pi.cSize)) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageRange);
            return false;
        }

        //calculate checksum
        std::uint32_t storedHdrCk = static_cast<std::uint32_t>(hdrData[24])
                            | (static_cast<std::uint32_t>(hdrData[25]) << 8)
                            | (static_cast<std::uint32_t>(hdrData[26]) << 16)
                            | (static_cast<std::uint32_t>(hdrData[27]) << 24);
        std::uint32_t storedDataCk = static_cast<std::uint32_t>(hdrData[28])
                             | (static_cast<std::uint32_t>(hdrData[29]) << 8)
                             | (static_cast<std::uint32_t>(hdrData[30]) << 16)
                             | (static_cast<std::uint32_t>(hdrData[31]) << 24);
        std::uint32_t calcsD = checksum(0, cData.data(), pi.cSize);
        for (std::uint8_t i= 24; i<28; ++i)
            hdrData[i]=0;
        std::uint32_t calcsH = checksum(calcsD, hdrData, 32);
        DRW_DBG("Calc header checksum= "); DRW_DBGH(calcsH);
        DRW_DBG("\nCalc data checksum= "); DRW_DBGH(calcsD); DRW_DBG("\n");
        if (calcsD != storedDataCk || calcsH != storedHdrCk) {
            if (calcsD != storedDataCk)
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageChecksum,
                                storedDataCk, calcsD);
            if (calcsH != storedHdrCk)
                recordPageValue(pi, DwgIntegrityCheckKind::DataPageChecksum,
                                storedHdrCk, calcsH);
            DRW_DBG("parseDataPage: checksum mismatch: storedHdr="); DRW_DBGH(storedHdrCk);
            DRW_DBG(" calcsH="); DRW_DBGH(calcsH);
            DRW_DBG(" storedData="); DRW_DBGH(storedDataCk);
            DRW_DBG(" calcsD="); DRW_DBGH(calcsD); DRW_DBG("\n");
            return false;
        }

        if (headerStartOffset != pi.startOffset) {
            recordPageFailure(pi, DwgIntegrityCheckKind::PageGeometry);
            DRW_DBG("parseDataPage: page header start offset mismatch\n");
            return false;
        }
        // Decompress input-bounded (the page's compressed bytes) with an
        // output window of decodedPageCap at the page's startOffset — libreDWG's
        // dec.byte = address; dec.size = dec.byte + max_decomp_size. The page
        // header's uSize is NOT the real content limit: the actual content
        // can extend past it up to maxSize (e.g. the CLASSES string stream).
        std::uint8_t* oData = decodedData.get() + pi.startOffset;
        if (si.compressed == 1) {
            // type 1 = store (no compression)
            const std::uint64_t copyLen =
                static_cast<std::uint64_t>(cData.size()) < decodedPageCap
                    ? static_cast<std::uint64_t>(cData.size()) : decodedPageCap;
            std::copy(cData.begin(), cData.begin() + copyLen, oData);
        } else {
            DRW_DBG("decompressing ");
            DRW_DBG(static_cast<unsigned long long>(pi.cSize));
            DRW_DBG(" bytes, window ");
            DRW_DBG(static_cast<unsigned long long>(decodedPageCap));
            DRW_DBG(" bytes\n");
            dwgCompressor comp;
            if (!comp.decompress18(cData.data(), oData, pi.cSize,
                                   decodedPageCap)) {
                recordPageFailure(pi, DwgIntegrityCheckKind::Decompression);
                return false;
            }
        }
    }
    sectionData = std::move(decodedData);
    // Consumers use the section's logical size. Keep any rounded page
    // capacity private to this decoder so a physical page envelope cannot
    // expose synthetic zero padding as section data.
    sectionSize = si.size;
    return true;
}

bool dwgReader18::readMetaData() {
    const auto recordFailure = [&](DwgIntegrityCheckKind kind) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::Metadata, kind,
            -1, -1, nullptr, 0, false,
            fileBuf ? fileBuf->getPosition() : 0,
            fileBuf != nullptr);
    };
    version = parent->getVersion();
    decoder.setVersion(version, false);
    DRW_DBG("dwgReader18::readMetaData\n");
    if (!fileBuf->setPosition(11)) {
        recordFailure(DwgIntegrityCheckKind::PageRange);
        return false;
    }
    maintenanceVersion = fileBuf->getRawChar8();
    DRW_DBG("maintenance version= "); DRW_DBGH(maintenanceVersion);
    DRW_DBG("\nbyte at 0x0C= "); DRW_DBGH(fileBuf->getRawChar8());
    previewImagePos = fileBuf->getRawLong32(); //+ page header size (0x20).
    DRW_DBG("\npreviewImagePos (seekerImageData) = "); DRW_DBG(previewImagePos);
    DRW_DBG("\napp Dwg version= "); DRW_DBGH(fileBuf->getRawChar8()); DRW_DBG(", ");
    appMaintenanceVersion = fileBuf->getRawChar8(); // byte 0x12 — hSize gate
    DRW_DBG("\napp maintenance version= "); DRW_DBGH(appMaintenanceVersion);
    std::uint16_t cp = fileBuf->getRawShort16();
    DRW_DBG("\ncodepage= "); DRW_DBG(cp);
    // R2007+ (AC1021+) store ordinary text as UTF-16LE; applying the file
    // codepage to the primary decoder would corrupt Unicode names. ENC names
    // remain byte-oriented, however, so initialize their secondary codec for
    // every version handled by this reader.
    if (const char* cpName = dwgCodePageName(cp)) {
        decoder.setByteCodePage(cpName);
        if (version <= DRW::AC1018)
            decoder.setCodePage(cpName, false);
    }
    DRW_DBG("\n3 0x00 bytes(seems 0x00, appDwgV & appMaintV) = "); DRW_DBGH(fileBuf->getRawChar8()); DRW_DBG(", ");
    DRW_DBGH(fileBuf->getRawChar8()); DRW_DBG(", "); DRW_DBGH(fileBuf->getRawChar8());
    securityFlags = fileBuf->getRawLong32();
    DRW_DBG("\nsecurity flags= "); DRW_DBG(securityFlags);
    // UNKNOWN SECTION 4 bytes
    std::uint32_t uk =    fileBuf->getRawLong32();
    DRW_DBG("\nUNKNOWN SECTION ( 4 bytes) = "); DRW_DBG(uk);
    std::uint32_t sumInfoAddr =    fileBuf->getRawLong32();
    DRW_DBG("\nsummary Info Address= "); DRW_DBG(sumInfoAddr);
    std::uint32_t vbaAdd =    fileBuf->getRawLong32();
    DRW_DBG("\nVBA address= "); DRW_DBGH(vbaAdd);
    DRW_DBG("\npos 0x28 are 0x00000080= "); DRW_DBGH(fileBuf->getRawLong32());
     DRW_DBG("\n");
    if (!fileBuf->isGood())
        recordFailure(DwgIntegrityCheckKind::PageRange);
    return fileBuf->isGood();
}

bool dwgReader18::readFileHeader() {

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
    uncompSize = 0;
    if (!fileBuf->setPosition(0x80)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, 0x80, true);
        return false;
    }

//    genMagicNumber(); DBG("\n"); DBG("\n");
    DRW_DBG("Encrypted Header Data=\n");
    std::uint8_t byteStr[0x6C];
    int size =0x6C;
    for (int i=0, j=0; i< 0x6C;i++) {
        std::uint8_t ch = fileBuf->getRawChar8();
        DRW_DBGH(ch);
        if (j == 15) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
        byteStr[i] = DRW_magicNum18[i] ^ ch;
    }
    DRW_DBG("\n");
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, 0x80, true);
        return false;
    }

    const auto headerLong = [&](std::size_t offset) {
        return static_cast<std::uint32_t>(byteStr[offset])
             | (static_cast<std::uint32_t>(byteStr[offset + 1]) << 8)
             | (static_cast<std::uint32_t>(byteStr[offset + 2]) << 16)
             | (static_cast<std::uint32_t>(byteStr[offset + 3]) << 24);
    };
    if (std::memcmp(byteStr, "AcFssFcAJMB", 11) != 0
        || byteStr[11] != 0
        || headerLong(12) != 0
        || headerLong(16) != 0x6c
        || headerLong(20) != 0x04
        || headerLong(68) != 0x20
        || headerLong(72) != 0x80
        || headerLong(76) != 0x40) {
        DRW_DBG("Invalid R2004 file-header identity or invariant field\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, 0x80, true);
        return false;
    }

//    size =0x6C;
    DRW_DBG("Decrypted Header Data=\n");
    for (int i=0, j = 0; i< size;i++) {
        DRW_DBGH( static_cast<unsigned char>(byteStr[i]));
        if (j == 15) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
    dwgBuffer buff(byteStr, 0x6C, &decoder);
    std::string name = reinterpret_cast<char*>(byteStr);
    DRW_DBG("\nFile ID string (AcFssFcAJMB)= "); DRW_DBG(name.c_str());
    //ID string + NULL = 12
    buff.setPosition(12);
    DRW_DBG("\n0x00 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x6c long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x04 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nRoot tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLowermost left tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLowermost right tree node gap= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nUnknown long (1)= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLast section page Id= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nLast section page end address 64b= "); DRW_DBGH(buff.getRawLong64());
    DRW_DBG("\nStart of second header data address 64b= "); DRW_DBGH(buff.getRawLong64());
    DRW_DBG("\nGap amount= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nSection page amount= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x20 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x80 long= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\n0x40 long= "); DRW_DBGH(buff.getRawLong32());
    std::int32_t secPageMapId = buff.getRawLong32();
    DRW_DBG("\nSection Page Map Id= "); DRW_DBGH(secPageMapId);
    const std::uint64_t secPageMapOffset = buff.getRawLong64();
    std::uint64_t secPageMapAddr = 0;
    if (!buff.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader,
                      -1, -1, 0, false, 0x80, true);
        return false;
    }
    if (!dwgSafety::add(secPageMapOffset, 0x100, secPageMapAddr)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    DRW_DBG("\nSection Page Map address 64b= "); DRW_DBGH(secPageMapAddr);
    DRW_DBG("\nSection Page Map address 64b dec= "); DRW_DBG(secPageMapAddr);
    std::uint32_t secMapId = buff.getRawLong32();
    DRW_DBG("\nSection Map Id= "); DRW_DBGH(secMapId);
    DRW_DBG("\nSection page array size= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nGap array size= "); DRW_DBGH(buff.getRawLong32());
    const std::uint32_t crcRead = buff.getRawLong32();
    DRW_DBG("\nCRC32= "); DRW_DBGH(crcRead);
    for (std::uint8_t i = 0x68; i < 0x6c; ++i)
        byteStr[i] = '\0';
//    byteStr[i] = '\0';
    std::uint32_t crcCalc = buff.crc32(0x00,0,0x6C);
    DRW_DBG("\nCRC32 calculated= "); DRW_DBGH(crcCalc);
    if (crcRead != 0 && crcRead != crcCalc) {
        ++m_r2004CrcMismatch;
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Warning,
            DwgIntegrityAddressSpace::PhysicalFile,
            DwgIntegrityPhase::FileHeader,
            DwgIntegrityCheckKind::FileHeaderCrc,
            -1, -1, nullptr, 0, false, 0x80, true, 0, false,
            crcRead, crcCalc, true);
        DRW_DBG("\nWARNING dwgReader18::readFileHeader CRC mismatch: calc=");
        DRW_DBGH(crcCalc); DRW_DBG(" read="); DRW_DBGH(crcRead); DRW_DBG("\n");
    }

    DRW_DBG("\nEnd Encrypted Data. Reads 0x14 bytes, equal to magic number:\n");
    bool endMagicOk = true;
    for (int i=0, j=0; i< 0x14;i++) {
        const std::uint8_t expected =
            static_cast<std::uint8_t>(DRW_magicNumEnd18[i]);
        const std::uint8_t actual = fileBuf->getRawChar8();
        DRW_DBG("magic num: "); DRW_DBGH( static_cast<unsigned char>(expected));
        DRW_DBG(",read "); DRW_DBGH( static_cast<unsigned char>(actual));
        endMagicOk = endMagicOk && actual == expected;
        if (j == 3) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
    if (!endMagicOk) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
    if (secPageMapId <= 0 || secMapId <= 0) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::FileHeader);
        return false;
    }
// At this point are parsed the first 256 bytes
    DRW_DBG("\nJump to Section Page Map address: "); DRW_DBGH(secPageMapAddr);

    if (!fileBuf->setPosition(secPageMapAddr)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    std::uint32_t pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    std::uint32_t decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    if (decompSize == 0 || decompSize > dwgSafety::MaxBufferSize) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    if (pageType != 0x41630e3b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x41630e3b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    std::vector<std::uint8_t> tmpDecompSec;
    if (decompSize > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || !DRW::resize(tmpDecompSec, static_cast<int>(decompSize))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    if (!parseSysPage(tmpDecompSec.data(), decompSize,
                      DwgIntegrityPhase::PageMap,
                      static_cast<std::uint64_t>(secPageMapId),
                      secPageMapId > 0, secPageMapAddr, true)) {
        return false;
    }

// parses "Section page map" decompressed data
    std::unordered_map<std::uint32_t, dwgPageInfo> sectionPageMapTmp;
    // The global R2004 page map may describe unreferenced physical extent
    // beyond the stored input. Physical bounds are enforced when the section
    // map resolves a page into an actual section payload below.
    if (!parseSectionPageMap(
            tmpDecompSec.data(), decompSize, 0x100, sectionPageMapTmp)) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::PageMap);
        return false;
    }

    const auto sectionPageMapIt = sectionPageMapTmp.find(
        static_cast<std::uint32_t>(secPageMapId));
    if (sectionPageMapIt == sectionPageMapTmp.end()
        || sectionPageMapIt->second.address != secPageMapAddr) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true,
                      secPageMapAddr, true);
        return false;
    }
    if (static_cast<std::uint32_t>(secPageMapId) == secMapId) {
        recordFailure(DwgIntegrityCheckKind::PageMapReference,
                      DwgIntegrityPhase::PageMap,
                      -1, -1, static_cast<std::uint64_t>(secPageMapId), true);
        return false;
    }

    DRW_DBG("\n*** dwgReader18: Processing Data Section Map ***\n");
    auto sectionMapIt = sectionPageMapTmp.find(secMapId);
    if (sectionMapIt == sectionPageMapTmp.end()) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true);
        return false;
    }
    dwgPageInfo sectionMap = sectionMapIt->second;
    if (!dwgSafety::range(sectionMap.address, sectionMap.size,
                          fileBuf->size())) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    if (!fileBuf->setPosition(sectionMap.address)) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (!fileBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    if (decompSize == 0 || decompSize > dwgSafety::MaxBufferSize) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    if (pageType != 0x4163003b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x4163003b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    if (decompSize > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        || !DRW::resize(tmpDecompSec, static_cast<int>(decompSize))) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap,
                      -1, -1, secMapId, true, sectionMap.address, true);
        return false;
    }
    if (!parseSysPage(tmpDecompSec.data(), decompSize,
                      DwgIntegrityPhase::SectionMap,
                      static_cast<std::uint64_t>(secMapId), true,
                      sectionMap.address, true)) {
        return false;
    }

    //reads sections:
    DRW_DBG("\n*** dwgReader18: reads sections:");
    dwgBuffer buff3(tmpDecompSec.data(), decompSize, &decoder);
    std::unordered_map<int, dwgSectionInfo> parsedSections;
    std::vector<dwgSectionInfo> parsedUnknownSections;
    constexpr std::uint64_t sectionDescriptionHeaderSize = 96;
    constexpr std::uint64_t sectionPageEntrySize = 16;
    std::unordered_set<std::uint32_t> claimedSectionPages;
    if (!claimedSectionPages.insert(static_cast<std::uint32_t>(secPageMapId)).second
        || !claimedSectionPages.insert(secMapId).second) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    if (decompSize < 20) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    std::uint32_t numDescriptions = buff3.getRawLong32();
    DRW_DBG("\nnumDescriptions (sections)= "); DRW_DBG(numDescriptions);
    DRW_DBG("\n0x02 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00007400 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\nunknown long (numDescriptions?)= "); DRW_DBG(buff3.getRawLong32()); DRW_DBG("\n");
    if (!buff3.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    if (numDescriptions > (decompSize - buff3.getPosition()) /
                          sectionDescriptionHeaderSize) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }

    for (unsigned int i = 0; i < numDescriptions; i++) {
        if (decompSize - buff3.getPosition() < sectionDescriptionHeaderSize) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        dwgSectionInfo secInfo;
        secInfo.size = buff3.getRawLong64();
        DRW_DBG("\nSize of section= "); DRW_DBGH(secInfo.size);
        secInfo.pageCount = buff3.getRawLong32();
        DRW_DBG("\nPage count= "); DRW_DBGH(secInfo.pageCount);
        secInfo.maxSize = buff3.getRawLong32();
        DRW_DBG("\nMax Decompressed Size= "); DRW_DBGH(secInfo.maxSize);
        DRW_DBG("\nunknown long= "); DRW_DBGH(buff3.getRawLong32());
        secInfo.compressed = buff3.getRawLong32();
        DRW_DBG("\nis Compressed? 1:no, 2:yes= "); DRW_DBGH(secInfo.compressed);
        secInfo.Id = buff3.getRawLong32();
        DRW_DBG("\nSection Id= "); DRW_DBGH(secInfo.Id);
        secInfo.encrypted = buff3.getRawLong32();
        //encrypted (doc: 0 no, 1 yes, 2 unkn) on read: objects 0 and encrypted yes
        DRW_DBG("\nEncrypted= "); DRW_DBGH(secInfo.encrypted);
        if (!buff3.isGood()) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        if (secInfo.pageCount > dwgSafety::MaxPageCount
            || secInfo.size > dwgSafety::MaxBufferSize
            || secInfo.maxSize > dwgSafety::MaxBufferSize
            || (secInfo.compressed != 1 && secInfo.compressed != 2)) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        std::uint8_t nameCStr[64]{};
        if (!buff3.getBytes(nameCStr, 64)) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap);
            return false;
        }
        const auto nameEnd = std::find(std::begin(nameCStr),
                                       std::end(nameCStr), std::uint8_t{0});
        secInfo.name.assign(reinterpret_cast<const char*>(nameCStr),
                            static_cast<std::size_t>(nameEnd - nameCStr));
        DRW_DBG("\nSection std::Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
        if (secInfo.pageCount > (decompSize - buff3.getPosition()) /
                                sectionPageEntrySize) {
            recordFailure(DwgIntegrityCheckKind::PageRange,
                          DwgIntegrityPhase::SectionMap,
                          secEnum::getEnum(secInfo.name), secInfo.Id);
            return false;
        }
        const std::uint64_t pageWindow =
            secInfo.maxSize != 0 ? secInfo.maxSize : secInfo.size;
        if (secInfo.pageCount != 0 && pageWindow == 0) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap,
                          secEnum::getEnum(secInfo.name), secInfo.Id);
            return false;
        }
        std::vector<std::pair<std::uint64_t, std::uint64_t>> pageRanges;
        if (secInfo.pageCount
                > static_cast<std::uint64_t>(std::numeric_limits<int>::max())
            || !DRW::reserve(pageRanges,
                              static_cast<int>(secInfo.pageCount))) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry,
                          DwgIntegrityPhase::SectionMap,
                          secEnum::getEnum(secInfo.name), secInfo.Id);
            return false;
        }
        for (unsigned int i = 0; i < secInfo.pageCount; i++){
            std::uint32_t pn = buff3.getRawLong32();
            auto pageIt = sectionPageMapTmp.find(pn);
            if (pageIt == sectionPageMapTmp.end()) {
                recordFailure(DwgIntegrityCheckKind::SectionPageReference,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true);
                return false;
            }
            dwgPageInfo pi = pageIt->second; //get a copy
            DRW_DBG(" reading pag num = "); DRW_DBGH(pn);
            pi.dataSize = buff3.getRawLong32();
            pi.startOffset = buff3.getRawLong64();
            if (!buff3.isGood()) {
                recordFailure(DwgIntegrityCheckKind::PageRange,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true);
                return false;
            }
            if (pi.dataSize == 0
                || !dwgSafety::range(pi.address, pi.size, fileBuf->size())
                || !secInfo.pages.emplace(pn, pi).second) {
                const auto kind = !dwgSafety::range(
                    pi.address, pi.size, fileBuf->size())
                    ? DwgIntegrityCheckKind::PageRange
                    : DwgIntegrityCheckKind::SectionPageReference;
                recordFailure(kind, DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true, pi.address, true);
                return false;
            }
            std::uint64_t pageEnd = 0;
            if (!dwgSafety::add(pi.startOffset, pageWindow, pageEnd)) {
                recordFailure(DwgIntegrityCheckKind::PageRange,
                              DwgIntegrityPhase::SectionMap,
                              secEnum::getEnum(secInfo.name), secInfo.Id,
                              pn, true, pi.startOffset, true);
                return false;
            }
            for (const auto& range : pageRanges) {
                if (pi.startOffset < range.second && range.first < pageEnd) {
                    recordFailure(DwgIntegrityCheckKind::PageRange,
                                  DwgIntegrityPhase::SectionMap,
                                  secEnum::getEnum(secInfo.name), secInfo.Id,
                                  pn, true, pi.startOffset, true);
                    return false;
                }
            }
            pageRanges.emplace_back(pi.startOffset, pageEnd);
            DRW_DBG("\n    Page number= "); DRW_DBGH(secInfo.pages[pn].Id);
            DRW_DBG("\n    size in file= "); DRW_DBGH(secInfo.pages[pn].size);
            DRW_DBG("\n    address in file= "); DRW_DBGH(secInfo.pages[pn].address);
            DRW_DBG("\n    Data size= "); DRW_DBGH(secInfo.pages[pn].dataSize);
            DRW_DBG("\n    Start offset= "); DRW_DBGH(secInfo.pages[pn].startOffset); DRW_DBG("\n");
        }
        //do not save empty section
        if (!secInfo.name.empty()) {
            DRW_DBG("Saved section Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
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
            if (sectionId == secEnum::UNKNOWNS)
                parsedUnknownSections.push_back(std::move(secInfo));
            else
                parsedSections.emplace(sectionId, std::move(secInfo));
        }
    }

    if (!buff3.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange,
                      DwgIntegrityPhase::SectionMap);
        return false;
    }
    const std::uint64_t sectionMapTail = buff3.getPosition();
    if (sectionMapTail > tmpDecompSec.size()
        || std::any_of(tmpDecompSec.cbegin()
                           + static_cast<std::ptrdiff_t>(sectionMapTail),
                       tmpDecompSec.cend(),
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
    sections.swap(parsedSections);
    m_unknownSections.swap(parsedUnknownSections);
    DRW_DBG("\ndwgReader18::readFileHeader END\n\n");
    return true;
}

bool dwgReader18::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("dwgReader18::readDwgHeader\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::HEADER,
            sectionId, "AcDb:Header");
    };
    objData.reset();
    uncompSize = 0;
    const auto sectionIt = sections.find(secEnum::HEADER);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;
    bool ret = parseDataPage(si/*, objData*/);
    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    if (ret) {
        dwgBuffer dataBuf(objData.get(), si.size, &decoder);
        DRW_DBG("Header section sentinel= ");
        if (!checkSentinel(&dataBuf, secEnum::HEADER, true)) {
            ret = false;
        } else if (version == DRW::AC1018){
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &dataBuf);
        } else {
            dwgBuffer handleBuf(objData.get(), si.size, &decoder);
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &handleBuf);
        }
        if (!ret)
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
    }
    // Cleanup: global store for uncompressed data of all pages.
    objData.reset();
    uncompSize = 0;
    return ret;
}


bool dwgReader18::readDwgClasses(){
    DRW_DBG("\ndwgReader18::readDwgClasses\n");
    beginDwgClassCoverage();
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::CLASSES,
            sectionId, "AcDb:Classes");
    };
    objData.reset();
    uncompSize = 0;
    const auto sectionIt = sections.find(secEnum::CLASSES);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;
    struct DataPageCleanup {
        std::unique_ptr<std::uint8_t[]>& data;
        std::uint64_t& size;
        ~DataPageCleanup() {
            data.reset();
            size = 0;
        }
    } cleanup{objData, uncompSize};
    if (!parseDataPage(si/*, objData*/)) {
        return false;
    }

    // The CLASSES string stream can occupy the unused tail of its final fixed
    // size data page. parseDataPage has already bounded and zero-filled this
    // decoded envelope, so use that capacity for the reader's physical bound.
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    DRW_DBG("classes section sentinel= ");
    if (!checkSentinel(&dataBuf, secEnum::CLASSES, true)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        objData.reset();
        uncompSize = 0;
        return false;
    }

    std::uint32_t size = dataBuf.getRawLong32();
    DRW_DBG("\ndata size in bytes "); DRW_DBG(size);
    if ((DRW::AC1024 <= version && 3 < appMaintenanceVersion)
        || DRW::AC1032 <= version) { //2010+ MV>3
        std::uint32_t hSize = dataBuf.getRawLong32();
        DRW_DBG("\n2010+ & MV> 3, height 32b: "); DRW_DBG(hSize);
    }
    std::uint32_t bitSize = 0;
    if (version > DRW::AC1021) {//2007+
        bitSize = dataBuf.getRawLong32();
        DRW_DBG("\ntotal size in bits "); DRW_DBG(bitSize);
    }
    std::uint32_t maxClassNum = dataBuf.getBitShort();
    DRW_DBG("\nMaximum class number "); DRW_DBG(maxClassNum);
    DRW_DBG("\nRc 1 "); DRW_DBG(dataBuf.getRawChar8());
    DRW_DBG("\nRc 2 "); DRW_DBG(dataBuf.getRawChar8());
    DRW_DBG("\nBit "); DRW_DBG(dataBuf.getBit());
    // DWG custom-class numbers start at 500. maxClassNum is the highest class
    // number used; the loop below iterates (maxClassNum - 499) times. Any
    // value < 499 produces a std::uint32_t underflow (huge loop), so reject it as
    // structural corruption. maxClassNum == 499 (zero custom classes) is
    // legitimate — empty drawings saved by AutoCAD 2010 RTM (maintenanceVersion=2)
    // were previously rejected outright.
    if (!dataBuf.isGood() || maxClassNum < 499) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    std::uint64_t crcPosition = 0;
    std::uint64_t classDataEndBit = 0;
    std::uint64_t sectionEndBit = 0;
    if (!dwgSafety::add(size, 20, crcPosition)
        || !dwgSafety::range(crcPosition, 2, dataBuf.size())
        || crcPosition > static_cast<std::uint64_t>(
               std::numeric_limits<std::int32_t>::max() - 1)
        || !dwgSafety::multiply(crcPosition, 8, classDataEndBit)
        || !dwgSafety::multiply(uncompSize, 8, sectionEndBit)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    if (version > DRW::AC1018)
        classDataEndBit = bitSize;
    if (version > DRW::AC1021) {
        std::uint64_t declaredDataEndBit = 0;
        if (!dwgSafety::multiply(size, 8, declaredDataEndBit)
            || bitSize > declaredDataEndBit) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
    }

    // Do not let class fields decode the CRC, R2007+ trailer, or END
    // sentinel. The decompressed page buffer is intentionally larger than
    // the logical section for sparse/multi-page files.
    const auto cursorWithinClassData = [classDataEndBit](
            const dwgBuffer& value) {
        std::uint64_t bitPosition = 0;
        return value.isGood()
            && dwgSafety::multiply(value.getPosition(), 8, bitPosition)
            && dwgSafety::add(bitPosition, value.getBitPos(), bitPosition)
            && bitPosition <= classDataEndBit;
    };
    if (!cursorWithinClassData(dataBuf)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }

    /*******************************/
    dwgBuffer *strBuf = &dataBuf;
    dwgBuffer strBuff(objData.get(), uncompSize, &decoder);
    std::uint64_t stringDataEndBit = classDataEndBit;
    const auto cursorWithinStringData = [](const dwgBuffer& value,
                                           std::uint64_t endBit) {
        std::uint64_t bitPosition = 0;
        return value.isGood()
            && dwgSafety::multiply(value.getPosition(), 8, bitPosition)
            && dwgSafety::add(bitPosition, value.getBitPos(), bitPosition)
            && bitPosition <= endBit;
    };
    //prepare string stream for 2007+
    if (version > DRW::AC1021) {//2007+
        bool hasHSize = ((DRW::AC1024 <= version && 3 < appMaintenanceVersion)
                         || DRW::AC1032 <= version);
        std::uint64_t footerEndBit = 0;
        std::uint64_t stringStartBit = 0;
        std::uint64_t stringSize = 0;
        if (!dwgSafety::add(bitSize, hasHSize ? 191 : 159, footerEndBit)) {
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        if (!readDwgClassStringFooter(strBuff, footerEndBit,
                                      stringStartBit, stringSize)) {
            recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
            return false;
        }
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
        strBuff = dwgBuffer(objData.get(), stringBufferSize, &decoder);
        strBuf = &strBuff;
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
    }

    /*******************************/

    std::uint32_t endDataPos = maxClassNum-499;
    DRW_DBG("\nbuff.getPosition: "); DRW_DBG(dataBuf.getPosition());
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
        if (!cursorWithinClassData(dataBuf)
            || !dwgClassBitPosition(dataBuf, dataRangeStart)
            || !dwgClassBitPosition(*strBuf, stringRangeStart)) {
            DRW_DwgClassCoverageEntry failed;
            failed.m_sectionDescriptorId = si.Id;
            recordDwgClassCoverageFailure(
                std::move(failed), DRW_DwgClassCoverageReason::Bounds);
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
            return false;
        }
        auto cl = std::make_unique<DRW_Class>();
        const bool parsed = cl->parseDwg(version, &dataBuf, strBuf);
        const bool classNumberValid = cl->classNum >= 500;
        const bool classIsNew = classesmap.find(cl->classNum) == classesmap.end()
            && stagedMap.find(cl->classNum) == stagedMap.end();
        const bool dataCursorValid = cursorWithinClassData(dataBuf);
        const bool stringCursorValid = version <= DRW::AC1021
            ? cursorWithinClassData(*strBuf)
            : cursorWithinStringData(*strBuf, stringDataEndBit);
        std::uint64_t dataRangeEnd = 0;
        std::uint64_t stringRangeEnd = 0;
        if (!parsed || !dataCursorValid || !stringCursorValid
            || !dwgClassBitPosition(dataBuf, dataRangeEnd)
            || !dwgClassBitPosition(*strBuf, stringRangeEnd)) {
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
            || !(strBuf == &dataBuf
                    ? setDwgClassBitRange(
                        coverage.m_stringRange, dataRangeStart, dataRangeEnd,
                        DRW_DwgFrameOffsetSpace::DecodedBuffer, true)
                    : setDwgClassBitRange(
                        coverage.m_stringRange, stringRangeStart,
                        stringRangeEnd,
                        DRW_DwgFrameOffsetSpace::DecodedBuffer, true))) {
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
        DRW_DBG("\nbuff.getPosition: "); DRW_DBG(dataBuf.getPosition());
    }
    DRW_DBG("\nend classes data buff.getPosition: "); DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\nend classes data buff.getBitPos: "); DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\nend classes strings buff.getPosition: "); DRW_DBG(strBuf->getPosition());
    DRW_DBG("\nend classes strings buff.getBitPos: "); DRW_DBG(strBuf->getBitPos());

/***************/

    if (!dataBuf.setPosition(crcPosition)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    const std::uint16_t crcRead = dataBuf.getRawShort16();
    if (!dataBuf.isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    const std::uint16_t crcCalc = dataBuf.crc8(
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
        DRW_DBG("\nWARNING dwgReader18::readDwgClasses CRC mismatch: calc=");
        DRW_DBGH(crcCalc); DRW_DBG(" read="); DRW_DBGH(crcRead); DRW_DBG("\n");
    }
    DRW_DBG("\nCRC: "); DRW_DBGH(crcRead);
    if (!readDwgClassesTail(dataBuf)) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    DRW_DBG("\nclasses section end sentinel= ");
    // 1.4: the END sentinel is checked but kept WARN-ONLY here. libdxfrw's
    // decompressed-buffer model for AC1024 (reader24→reader21) and AC1027
    // (reader27→this) does not land a reliable CLASSES end sentinel at this
    // position on valid files, so a hard fail here regresses the corpus.
    // Only reader15 (AC1015), whose end sentinel is reliable, hard-fails.
    checkSentinel(&dataBuf, secEnum::CLASSES, false);

    //Cleanup: global store for uncompressed data of all pages
    objData.reset();

    if (!dataBuf.isGood() || !strBuf->isGood()) {
        recordFailure(DwgIntegrityCheckKind::PageRange, si.Id);
        return false;
    }
    if (!publishDwgClasses(stagedClasses)) {
        recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    return true;
}


/*********** objects map ************************/
/** Note: object map are split in sections with max size 2035?
 *  heach section are 2 bytes size + data bytes + 2 bytes crc
 *  size value are data bytes + 2 and to calculate crc are used
 *  2 bytes size + data bytes
 *  last section are 2 bytes size + 2 bytes crc (size value always 2)
**/
bool dwgReader18::readDwgHandles() {
    DRW_DBG("\ndwgReader18::readDwgHandles\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::ObjectMap, kind, secEnum::HANDLES,
            sectionId, "AcDb:Handles");
    };
    objData.reset();
    uncompSize = 0;
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

    if (!parseDataPage(si)) {
        return false;
    }

    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    bool ret {dwgReader::readDwgHandles(
        &dataBuf, 0, si.size, objectsIt->second.size,
        DwgIntegrityAddressSpace::DecodedBuffer, si.Id)};

    // Cleanup: global store for uncompressed data of all pages.
    objData.reset();
    uncompSize = 0;

    return ret;
}


/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader18::readDwgTables(DRW_Header& hdr) {
    DRW_DBG("\ndwgReader18::readDwgTables\n");
    const auto recordFailure = [&](DwgIntegrityCheckKind kind,
                                   std::int32_t sectionId = -1) {
        recordIntegrityDiagnostic(
            DwgIntegritySeverity::Error,
            DwgIntegrityAddressSpace::DecodedBuffer,
            DwgIntegrityPhase::SectionParser, kind, secEnum::OBJECTS,
            sectionId, "AcDb:AcDbObjects");
    };
    objData.reset();
    uncompSize = 0;
    const auto sectionIt = sections.find(secEnum::OBJECTS);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference);
        return false;
    }
    const dwgSectionInfo& si = sectionIt->second;

    std::unique_ptr<std::uint8_t[]> stagedObjectData;
    std::uint64_t stagedObjectSize = 0;
    if (!parseDataPage(si, stagedObjectData, stagedObjectSize)
        || si.size > stagedObjectSize || stagedObjectData == nullptr) {
        if (stagedObjectData != nullptr && si.size > stagedObjectSize)
            recordFailure(DwgIntegrityCheckKind::PageGeometry, si.Id);
        return false;
    }
    dwgBuffer dataBuf(stagedObjectData.get(), si.size, &decoder);

    if (!dwgReader::readDwgTables(
            hdr, &dataBuf, DwgIntegrityAddressSpace::DecodedBuffer))
        return false;
    if (!captureRawDwgDataSections()) {
        recordFailure(DwgIntegrityCheckKind::SectionPageReference, si.Id);
        return false;
    }

    objData = std::move(stagedObjectData);
    uncompSize = stagedObjectSize;
    return true;
}

bool dwgReader18::captureRawDwgDataSections() {
    std::vector<DRW_RawDwgSection> stagedRawSections;
    std::vector<DRW_DataStorageSection> stagedDataStorageSections;
    const auto captureSection = [&](const dwgSectionInfo& si,
                                    const char* fallbackName,
                                    bool parseDataStorage) -> bool {
        DRW_RawDwgSection section;
        section.m_name = si.name.empty() && fallbackName != nullptr
            ? fallbackName : si.name;
        if (section.m_name.empty())
            return false;
        section.m_version = version;
        // R2004 section descriptors use 1 for stored pages and 2 for LZ
        // pages. Preserve the source value so the writer can make an
        // explicit compatibility decision instead of replaying R2007's 4.
        section.m_encoding = si.compressed;
        section.m_encrypted = si.encrypted;
        section.m_maxSize = si.maxSize;

        if (si.size != 0) {
            std::unique_ptr<std::uint8_t[]> sectionData;
            std::uint64_t sectionSize = 0;
            if (!parseDataPage(si, sectionData, sectionSize))
                return false;
            if (si.size > sectionSize)
                return false;
            if (si.size > static_cast<std::uint64_t>(
                               std::numeric_limits<int>::max())
                || !DRW::resize(section.m_data, static_cast<int>(si.size)))
                return false;
            std::copy_n(sectionData.get(), static_cast<std::size_t>(si.size),
                        section.m_data.begin());
        }

        if (parseDataStorage) {
            // PR-2a: typed DataStorage index (handles + payload ranges). Raw
            // bytes remain available for replay consumers.
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
