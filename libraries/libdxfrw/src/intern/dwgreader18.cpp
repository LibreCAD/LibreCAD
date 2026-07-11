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
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "drw_dbg.h"
#include "dwgreader18.h"
#include "dwgutil.h"
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

//called: Section page map: 0x41630e3b
bool dwgReader18::parseSysPage(std::uint8_t *decompSec, std::uint32_t decompSize){
    DRW_DBG("\nparseSysPage:\n ");
    std::uint32_t compSize = fileBuf->getRawLong32();
    DRW_DBG("Compressed size= "); DRW_DBG(compSize); DRW_DBG(", "); DRW_DBGH(compSize);
    std::uint32_t compType = fileBuf->getRawLong32();
    DRW_DBG("\nCompression type= "); DRW_DBGH(compType);
    DRW_DBG("\nSection page checksum= "); DRW_DBGH(fileBuf->getRawLong32()); DRW_DBG("\n");

    std::uint8_t hdrData[20];
    fileBuf->moveBitPos(-160);
    fileBuf->getBytes(hdrData, 20);
    std::uint32_t storedChecksum = static_cast<std::uint32_t>(hdrData[16])
                           | (static_cast<std::uint32_t>(hdrData[17]) << 8)
                           | (static_cast<std::uint32_t>(hdrData[18]) << 16)
                           | (static_cast<std::uint32_t>(hdrData[19]) << 24);
    for (std::uint8_t i= 16; i<20; ++i)
        hdrData[i]=0;
    std::uint32_t calcsH = checksum(0, hdrData, 20);
    DRW_DBG("Calc hdr checksum= "); DRW_DBGH(calcsH);
    std::vector<std::uint8_t> tmpCompSec(compSize);
    fileBuf->getBytes(tmpCompSec.data(), compSize);
    std::uint32_t calcsD = checksum(calcsH, tmpCompSec.data(), compSize);
    DRW_DBG("\nCalc data checksum= "); DRW_DBGH(calcsD); DRW_DBG("\n");
    if (calcsD != storedChecksum) {
        DRW_DBG("parseSysPage: checksum mismatch: stored="); DRW_DBGH(storedChecksum);
        DRW_DBG(" computed="); DRW_DBGH(calcsD); DRW_DBG("\n");
        return false;
    }

    if (compType == 1) {
        // type 1 = store (no compression): size must match
        if (compSize != decompSize) {
            DRW_DBG("parseSysPage: stored page compSize != decompSize\n");
            return false;
        }
        std::copy(tmpCompSec.begin(), tmpCompSec.end(), decompSec);
    } else {
        DRW_DBG("decompressing "); DRW_DBG(compSize); DRW_DBG(" bytes in "); DRW_DBG(decompSize); DRW_DBG(" bytes\n");
        dwgCompressor comp;
        if (!comp.decompress18(tmpCompSec.data(), decompSec, compSize, decompSize)) {
            return false;
        }
        // System pages (page map, section map) are FIXED-SIZE: the whole
        // decompSize must be produced. Unlike data pages (input-bounded),
        // a short fill here means a truncated page map -> reject, rather
        // than silently walking a partially-zeroed map.
        if (comp.decompressedBytes() != decompSize) {
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
    if (si.size == 0)
        return false;
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
    constexpr std::uint64_t maxPageCap  = 0x144400;   // libreDWG per-type max
    constexpr std::uint64_t maxBufSize  = 0x2f000000; // libreDWG total cap
    if (pageCap == 0 || pageCap > maxBufSize
        || (si.pages.size() > 1 && pageCap > maxPageCap)
        || si.pages.size() > maxBufSize / pageCap) {
        DRW_DBG("parseDataPage: invalid page geometry (pageCap/pageCount)\n");
        return false;
    }
    // ODA p.29: all-zero pages are NOT written to file, so pages.size() counts
    // only the WRITTEN pages — a section with trailing zero pages needs a
    // buffer large enough for its declared si.size (rounded up to pageCap), not
    // just written_pages × pageCap. (No zero-page section exists in the current
    // corpus, where si.size == written_pages × pageCap, so this is a no-op there.)
    std::uint64_t bufSize = static_cast<std::uint64_t>(si.pages.size()) * pageCap;
    const std::uint64_t sizeRounded = ((si.size + pageCap - 1) / pageCap) * pageCap;
    if (sizeRounded > bufSize && sizeRounded <= maxBufSize)
        bufSize = sizeRounded;
    if (bufSize == 0 || bufSize > maxBufSize) {
        DRW_DBG("parseDataPage: invalid decompression buffer size\n");
        return false;
    }
    sectionData.reset(new std::uint8_t[bufSize]);
    std::fill(sectionData.get(), sectionData.get() + bufSize, 0);
    sectionSize = bufSize; // buffer size for section readers' dwgBuffer spans

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
        if (!fileBuf->setPosition(pi.address))
            return false;
        std::uint8_t hdrData[32];
        if (!fileBuf->getBytes(hdrData, 32))
            return false;
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
                DRW_DBG("parseDataPage: bad page type "); DRW_DBGH(pageType); DRW_DBG("\n");
                return false;
            }
            if (pi.startOffset > bufSize || pi.uSize > bufSize - pi.startOffset) {
                DRW_DBG("parseDataPage (AC1021+): page range exceeds buffer size\n");
                return false;
            }
            if (pi.size > fileBuf->size()
                || pi.address > fileBuf->size() - pi.size) {
                DRW_DBG("parseDataPage (AC1021+): page extends past end of file\n");
                return false;
            }
            if (!fileBuf->setPosition(pi.address))
                return false;
            std::vector<std::uint8_t> tmpPageRaw(pi.size);
            if (!fileBuf->getBytes(&tmpPageRaw.front(), pi.size)) {
                DRW_DBG("parseDataPage (AC1021+): short read of page data\n");
                return false;
            }
            std::vector<std::uint8_t> tmpPageRS(pi.size);
            std::uint32_t chunks = pi.size / 255;
            if (!dwgRSCodec::decode251I(tmpPageRaw.data(), tmpPageRS.data(), chunks)) {
                DRW_DBG("parseDataPage (AC1021+): RS decode failed\n");
                return false;
            }
            std::uint8_t *pageData = sectionData.get() + pi.startOffset;
            dwgCompressor comp;
            if (!comp.decompress21(tmpPageRS.data(), pageData, pi.cSize, pi.uSize)) {
                return false;
            }
            continue;
        }

        // ---- AC1018 page format: 32-byte encrypted header + compressed data ----
        if (pi.startOffset > bufSize || pageCap > bufSize - pi.startOffset) {
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
        DRW_DBG("\n      section number= "); DRW_DBGH(bufHdr.getRawLong32());
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
        if (pi.address > UINT64_MAX - 32 || pi.cSize > pi.size
            || pi.cSize > fileBuf->size()
            || pi.address + 32 + pi.cSize > fileBuf->size()) {
            DRW_DBG("parseDataPage: compressed page extends past end of file\n");
            return false;
        }
        std::vector<std::uint8_t> cData(pi.cSize);
        if (!fileBuf->setPosition(pi.address + 32)) {
            return false;
        }
        if (!fileBuf->getBytes(cData.data(), pi.cSize)) {
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
            DRW_DBG("parseDataPage: checksum mismatch: storedHdr="); DRW_DBGH(storedHdrCk);
            DRW_DBG(" calcsH="); DRW_DBGH(calcsH);
            DRW_DBG(" storedData="); DRW_DBGH(storedDataCk);
            DRW_DBG(" calcsD="); DRW_DBGH(calcsD); DRW_DBG("\n");
            return false;
        }

        if (headerStartOffset != pi.startOffset) {
            DRW_DBG("parseDataPage: page header start offset mismatch\n");
            return false;
        }
        // Decompress input-bounded (the page's compressed bytes) with an
        // output window of pageCap at the page's startOffset — libreDWG's
        // dec.byte = address; dec.size = dec.byte + max_decomp_size. The page
        // header's uSize is NOT the real content limit: the actual content
        // can extend past it up to maxSize (e.g. the CLASSES string stream).
        std::uint8_t* oData = sectionData.get() + pi.startOffset;
        if (si.compressed == 1) {
            // type 1 = store (no compression)
            const std::uint64_t copyLen =
                static_cast<std::uint64_t>(cData.size()) < pageCap
                    ? static_cast<std::uint64_t>(cData.size()) : pageCap;
            std::copy(cData.begin(), cData.begin() + copyLen, oData);
        } else {
            DRW_DBG("decompressing ");
            DRW_DBG(static_cast<unsigned long long>(pi.cSize));
            DRW_DBG(" bytes, window ");
            DRW_DBG(static_cast<unsigned long long>(pageCap));
            DRW_DBG(" bytes\n");
            dwgCompressor comp;
            if (!comp.decompress18(cData.data(), oData, pi.cSize, pageCap)) {
                return false;
            }
        }
    }
    return true;
}

bool dwgReader18::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    DRW_DBG("dwgReader18::readMetaData\n");
    if (! fileBuf->setPosition(11))
        return false;
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
    // R2007+ (AC1021+) store all text as UTF-16LE; the DWGCODEPAGE field is only
    // meaningful for R2004 and earlier. Applying it for 2007+ would clobber the
    // UTF-16 decoder configured by setVersion() and corrupt Unicode names.
    if (version <= DRW::AC1018) {
        if (const char* cpName = dwgCodePageName(cp))
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
    return true;
}

bool dwgReader18::readFileHeader() {

    if (! fileBuf->setPosition(0x80))
        return false;

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
    std::uint64_t secPageMapAddr = buff.getRawLong64()+0x100;
    DRW_DBG("\nSection Page Map address 64b= "); DRW_DBGH(secPageMapAddr);
    DRW_DBG("\nSection Page Map address 64b dec= "); DRW_DBG(secPageMapAddr);
    std::uint32_t secMapId = buff.getRawLong32();
    DRW_DBG("\nSection Map Id= "); DRW_DBGH(secMapId);
    DRW_DBG("\nSection page array size= "); DRW_DBGH(buff.getRawLong32());
    DRW_DBG("\nGap array size= "); DRW_DBGH(buff.getRawLong32());
    //TODO: verify CRC
    DRW_DBG("\nCRC32= "); DRW_DBGH(buff.getRawLong32());
    for (std::uint8_t i = 0x68; i < 0x6c; ++i)
        byteStr[i] = '\0';
//    byteStr[i] = '\0';
    std::uint32_t crcCalc = buff.crc32(0x00,0,0x6C);
    DRW_DBG("\nCRC32 calculated= "); DRW_DBGH(crcCalc);

    DRW_DBG("\nEnd Encrypted Data. Reads 0x14 bytes, equal to magic number:\n");
    for (int i=0, j=0; i< 0x14;i++) {
        DRW_DBG("magic num: "); DRW_DBGH( static_cast<unsigned char>(DRW_magicNumEnd18[i]));
        DRW_DBG(",read "); DRW_DBGH( static_cast<unsigned char>(fileBuf->getRawChar8()));
        if (j == 3) {
            DRW_DBG("\n");
            j = 0;
        } else {
            DRW_DBG(", ");
            j++;
        }
    }
// At this point are parsed the first 256 bytes
    DRW_DBG("\nJump to Section Page Map address: "); DRW_DBGH(secPageMapAddr);

    if (! fileBuf->setPosition(secPageMapAddr))
        return false;
    std::uint32_t pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    std::uint32_t decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (pageType != 0x41630e3b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x41630e3b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        return false;
    }
    std::vector<std::uint8_t> tmpDecompSec(decompSize);
    if (!parseSysPage(tmpDecompSec.data(), decompSize)) {
        return false;
    }

//parses "Section page map" decompressed data
    dwgBuffer buff2(tmpDecompSec.data(), decompSize, &decoder);
    std::uint32_t address = 0x100;
    //stores temporarily info of all pages:
    std::unordered_map<std::uint64_t, dwgPageInfo >sectionPageMapTmp;

    for (unsigned int i = 0; i < decompSize;) {
        std::int32_t id = buff2.getRawLong32();//RLZ bad can be +/-
        std::uint32_t size = buff2.getRawLong32();
        i += 8;
        DRW_DBG("Page num= "); DRW_DBG(id); DRW_DBG(" size= "); DRW_DBGH(size);
        DRW_DBG(" address= "); DRW_DBGH(address);  DRW_DBG("\n");
        //TODO num can be negative indicating gap
//        std::uint64_t ind = id > 0 ? id : -id;
        if (id < 0){
            DRW_DBG("Parent= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG("\nLeft= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG(", Right= "); DRW_DBG(buff2.getRawLong32());
            DRW_DBG(", 0x00= ");DRW_DBGH(buff2.getRawLong32()); DRW_DBG("\n");
            i += 16;
        }

        sectionPageMapTmp[id] = dwgPageInfo(id, address, size);
        address += size;
    }

    DRW_DBG("\n*** dwgReader18: Processing Data Section Map ***\n");
    auto sectionMapIt = sectionPageMapTmp.find(secMapId);
    if (sectionMapIt == sectionPageMapTmp.end())
        return false;
    dwgPageInfo sectionMap = sectionMapIt->second;
    if (!fileBuf->setPosition(sectionMap.address))
        return false;
    pageType = fileBuf->getRawLong32();
    DRW_DBG("\nSection page type= "); DRW_DBGH(pageType);
    decompSize = fileBuf->getRawLong32();
    DRW_DBG("\nDecompressed size= "); DRW_DBG(decompSize); DRW_DBG(", "); DRW_DBGH(decompSize);
    if (pageType != 0x4163003b){
        //bad page type, ends
        DRW_DBG("Warning, bad page type, was expected 0x4163003b instead of");  DRW_DBGH(pageType); DRW_DBG("\n");
        return false;
    }
    tmpDecompSec.resize(decompSize);
    if (!parseSysPage(tmpDecompSec.data(), decompSize)) {
        return false;
    }

//reads sections:
    DRW_DBG("\n*** dwgReader18: reads sections:");
    dwgBuffer buff3(tmpDecompSec.data(), decompSize, &decoder);
    std::uint32_t numDescriptions = buff3.getRawLong32();
    DRW_DBG("\nnumDescriptions (sections)= "); DRW_DBG(numDescriptions);
    DRW_DBG("\n0x02 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00007400 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\n0x00 long= "); DRW_DBGH(buff3.getRawLong32());
    DRW_DBG("\nunknown long (numDescriptions?)= "); DRW_DBG(buff3.getRawLong32()); DRW_DBG("\n");

    for (unsigned int i = 0; i < numDescriptions; i++) {
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
        std::uint8_t nameCStr[64];
        buff3.getBytes(nameCStr, 64);
        secInfo.name = reinterpret_cast<char*>(nameCStr);
        DRW_DBG("\nSection std::Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
        for (unsigned int i = 0; i < secInfo.pageCount; i++){
            std::uint32_t pn = buff3.getRawLong32();
            auto pageIt = sectionPageMapTmp.find(pn);
            if (pageIt == sectionPageMapTmp.end())
                return false;
            dwgPageInfo pi = pageIt->second; //get a copy
            DRW_DBG(" reading pag num = "); DRW_DBGH(pn);
            pi.dataSize = buff3.getRawLong32();
            pi.startOffset = buff3.getRawLong64();
            secInfo.pages[pn]= pi;//complete copy in secInfo
            DRW_DBG("\n    Page number= "); DRW_DBGH(secInfo.pages[pn].Id);
            DRW_DBG("\n    size in file= "); DRW_DBGH(secInfo.pages[pn].size);
            DRW_DBG("\n    address in file= "); DRW_DBGH(secInfo.pages[pn].address);
            DRW_DBG("\n    Data size= "); DRW_DBGH(secInfo.pages[pn].dataSize);
            DRW_DBG("\n    Start offset= "); DRW_DBGH(secInfo.pages[pn].startOffset); DRW_DBG("\n");
        }
        //do not save empty section
        if (!secInfo.name.empty()) {
            DRW_DBG("Saved section Name= "); DRW_DBG( secInfo.name.c_str() ); DRW_DBG("\n");
            sections[secEnum::getEnum(secInfo.name)] = secInfo;
        }
    }

    if (! fileBuf->isGood())
        return false;
    DRW_DBG("\ndwgReader18::readFileHeader END\n\n");
    return true;
}

bool dwgReader18::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("dwgReader18::readDwgHeader\n");
    dwgSectionInfo si = sections[secEnum::HEADER];
    if (si.Id<0)//not found, ends
        return false;
    bool ret = parseDataPage(si/*, objData*/);
    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    if (ret) {
        dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);
        DRW_DBG("Header section sentinel= ");
        checkSentinel(&dataBuf, secEnum::HEADER, true);
        if (version == DRW::AC1018){
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &dataBuf);
        } else {
            dwgBuffer handleBuf(objData.get(), si.size, &decoder);
            ret = dwgReader::readDwgHeader(hdr, &dataBuf, &handleBuf);
        }
    }
    //Cleanup: global store for uncompressed data of all pages
    objData.reset();
    return ret;
}


bool dwgReader18::readDwgClasses(){
    DRW_DBG("\ndwgReader18::readDwgClasses\n");
    dwgSectionInfo si = sections[secEnum::CLASSES];
    if (si.Id < 0) //not found, ends
        return false;
    if (!parseDataPage(si/*, objData*/)) {
        return false;
    }

    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    DRW_DBG("classes section sentinel= ");
    checkSentinel(&dataBuf, secEnum::CLASSES, true);

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
    if (maxClassNum < 499) {
        return false;
    }

    /*******************************/
    dwgBuffer *strBuf = &dataBuf;
    dwgBuffer strBuff(objData.get(), uncompSize, &decoder);
    //prepare string stream for 2007+
    if (version > DRW::AC1021) {//2007+
        strBuf = &strBuff;
        //byte offset to the bit-stream start: 16 (start sentinel) + 4 (size)
        //+ 4 (hSize, only when read) = 20 or 24 bytes. -1 bit for the endBit.
        //The hSize gating must match the size-read gate above (line ~598),
        //otherwise AC1024 RTM files (appMaintenanceVersion <= 3) misalign by
        //32 bits and fail BAD_READ_CLASSES.
        bool hasHSize = ((DRW::AC1024 <= version && 3 < appMaintenanceVersion)
                         || DRW::AC1032 <= version);
        std::uint32_t strStartPos = bitSize + (hasHSize ? 191 : 159);
        DRW_DBG("\nstrStartPos: "); DRW_DBG(strStartPos);
        strBuff.setPosition(strStartPos >> 3);
        strBuff.setBitPos(strStartPos & 7);
        DRW_DBG("\nclasses strings buff.getPosition: "); DRW_DBG(strBuff.getPosition());
        DRW_DBG("\nclasses strings buff.getBitPos: "); DRW_DBG(strBuff.getBitPos());
        DRW_DBG("\nendBit "); DRW_DBG(strBuff.getBit());
        strStartPos -= 16;//decrement 16 bits
        DRW_DBG("\nstrStartPos: "); DRW_DBG(strStartPos);
        strBuff.setPosition(strStartPos >> 3);
        strBuff.setBitPos(strStartPos & 7);
        DRW_DBG("\nclasses strings buff.getPosition: "); DRW_DBG(strBuff.getPosition());
        DRW_DBG("\nclasses strings buff.getBitPos: "); DRW_DBG(strBuff.getBitPos());
        std::uint32_t strDataSize = strBuff.getRawShort16();
        DRW_DBG("\nstrDataSize: "); DRW_DBG(strDataSize);
        if (strDataSize & 0x8000) {
            strStartPos -= 16;//decrement 16 bits
            strDataSize &= 0x7FFF; //strip 0x8000;
            strBuff.setPosition(strStartPos >> 3);
            strBuff.setBitPos(strStartPos & 7);
            std::uint32_t hiSize = strBuff.getRawShort16();
            strDataSize |= (hiSize << 15);
        }
        strStartPos -= strDataSize;
        DRW_DBG("\nstrStartPos: "); DRW_DBG(strStartPos);
        strBuff.setPosition(strStartPos >> 3);
        strBuff.setBitPos(strStartPos & 7);
        DRW_DBG("\nclasses strings buff.getPosition: "); DRW_DBG(strBuff.getPosition());
        DRW_DBG("\nclasses strings buff.getBitPos: "); DRW_DBG(strBuff.getBitPos());
    }

    /*******************************/

    std::uint32_t endDataPos = maxClassNum-499;
    DRW_DBG("\nbuff.getPosition: "); DRW_DBG(dataBuf.getPosition());
    for (std::uint32_t i= 0; i<endDataPos;i++) {
        DRW_Class *cl = new DRW_Class();
        if (!cl->parseDwg(version, &dataBuf, strBuf)
            || cl->classNum < 500
            || classesmap.find(cl->classNum) != classesmap.end()) {
            delete cl;
            return false;
        }
        classesmap[cl->classNum] = cl;
        DRW_DBG("\nbuff.getPosition: "); DRW_DBG(dataBuf.getPosition());
    }
    DRW_DBG("\nend classes data buff.getPosition: "); DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\nend classes data buff.getBitPos: "); DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\nend classes strings buff.getPosition: "); DRW_DBG(strBuf->getPosition());
    DRW_DBG("\nend classes strings buff.getBitPos: "); DRW_DBG(strBuf->getBitPos());

/***************/

    // PR 13g — only advance to the next byte when the bit cursor sits
    // mid-byte (mirrors the writer's `alignToByte`, which is a no-op if
    // already byte-aligned).  The previous unconditional `+1` consumed
    // an extra byte when the final class entry's bit stream landed on a
    // byte boundary, causing reader/writer misalignment that surfaced
    // when an 11-entry CLASSES section ended at `bitPos==0` (LAYER_INDEX
    // at AC1018 smoke).
    if (strBuf->getBitPos() != 0) {
        strBuf->setPosition(strBuf->getPosition()+1);
        strBuf->setBitPos(0);
    }
    DRW_DBG("\nCRC: "); DRW_DBGH(strBuf->getRawShort16());
    if (version > DRW::AC1018){
        DRW_DBG("\nunknown CRC: "); DRW_DBGH(strBuf->getRawShort16());
    }
    DRW_DBG("\nclasses section end sentinel= ");
    // 1.4: the END sentinel is checked but kept WARN-ONLY here. libdxfrw's
    // decompressed-buffer model for AC1024 (reader24→reader21) and AC1027
    // (reader27→this) does not land a reliable CLASSES end sentinel at this
    // position on valid files, so a hard fail here regresses the corpus.
    // Only reader15 (AC1015), whose end sentinel is reliable, hard-fails.
    checkSentinel(strBuf, secEnum::CLASSES, false);

    //Cleanup: global store for uncompressed data of all pages
    objData.reset();

    return strBuf->isGood();
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
    dwgSectionInfo si = sections[secEnum::HANDLES];
    if (si.Id<0)//not found, ends
        return false;

    if (!parseDataPage(si)) {
        return false;
    }

    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    bool ret {dwgReader::readDwgHandles(&dataBuf, 0, si.size)};

    //Cleanup: global store for uncompressed data of all pages
    if (objData){
        objData.reset();
        uncompSize = 0;
    }

    return ret;
}


/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader18::readDwgTables(DRW_Header& hdr) {
    DRW_DBG("\ndwgReader18::readDwgTables\n");
    dwgSectionInfo si = sections[secEnum::OBJECTS];

    if (si.Id < 0   //not found, ends
        || !parseDataPage( si/*, objData*/)) {
        return false;
    }

    //global store for uncompressed data of all pages
    // uncompSize set by parseDataPage (num_pages x maxSize buffer model)
    dwgBuffer dataBuf(objData.get(), uncompSize, &decoder);

    //Do not delete objData in this point, needed in the remaining code

    if (!dwgReader::readDwgTables(hdr, &dataBuf))
        return false;

    if (!captureRawDwgDataSections())
        return false;

    return true;
}

bool dwgReader18::captureRawDwgDataSections() {
    auto it = sections.find(secEnum::PROTOTYPE);
    if (it == sections.end() || it->second.Id < 0)
        return true;

    const dwgSectionInfo& si = it->second;
    DRW_RawDwgSection section;
    section.m_name = si.name.empty() ? "AcDb:AcDsPrototype_1b" : si.name;
    section.m_version = version;

    if (si.size != 0) {
        std::unique_ptr<std::uint8_t[]> sectionData;
        std::uint64_t sectionSize = 0;
        if (!parseDataPage(si, sectionData, sectionSize))
            return false;
        if (si.size > sectionSize)
            return false;
        section.m_data.assign(sectionData.get(), sectionData.get() + si.size);
    }

    m_rawDwgSections.push_back(std::move(section));
    return true;
}
