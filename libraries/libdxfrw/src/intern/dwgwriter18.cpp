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

#include <cmath>
#include <cstring>
#include "dwgwriter18.h"
#include "dwgutil.h"

// XOR key for the 0x6C-byte encrypted variable header (file offset 0x80).
// Matches DRW_magicNum18 in dwgreader18.h — kept local to avoid including
// the reader header from writer code.
static const duint8 kMagicNum18[0x6C] = {
    0x29, 0x23, 0xbe, 0x84, 0xe1, 0x6c, 0xd6, 0xae,
    0x52, 0x90, 0x49, 0xf1, 0xf1, 0xbb, 0xe9, 0xeb,
    0xb3, 0xa6, 0xdb, 0x3c, 0x87, 0x0c, 0x3e, 0x99,
    0x24, 0x5e, 0x0d, 0x1c, 0x06, 0xb7, 0x47, 0xde,
    0xb3, 0x12, 0x4d, 0xc8, 0x43, 0xbb, 0x8b, 0xa6,
    0x1f, 0x03, 0x5a, 0x7d, 0x09, 0x38, 0x25, 0x1f,
    0x5d, 0xd4, 0xcb, 0xfc, 0x96, 0xf5, 0x45, 0x3b,
    0x13, 0x0d, 0x89, 0x0a, 0x1c, 0xdb, 0xae, 0x32,
    0x20, 0x9a, 0x50, 0xee, 0x40, 0x78, 0x36, 0xfd,
    0x12, 0x49, 0x32, 0xf6, 0x9e, 0x7d, 0x49, 0xdc,
    0xad, 0x4f, 0x14, 0xf2, 0x44, 0x40, 0x66, 0xd0,
    0x6b, 0xc4, 0x30, 0xb7, 0x32, 0x3b, 0xa1, 0x22,
    0xf6, 0x22, 0x91, 0x9d, 0xe1, 0x8b, 0x1f, 0xda,
    0xb0, 0xca, 0x99, 0x02
};

// Tail magic emitted at file offsets 0xEC-0xFF.  Matches DRW_magicNumEnd18.
static const duint8 kMagicNumEnd18[0x14] = {
    0xf8, 0x46, 0x6a, 0x04, 0x96, 0x73, 0x0e, 0xd9,
    0x16, 0x2f, 0x67, 0x68, 0xd4, 0xf7, 0x4a, 0x4a,
    0xd0, 0x57, 0x68, 0x76
};

// --- little-endian helpers ---------------------------------------------------

static void putRL(std::vector<duint8>& v, duint32 x) {
    v.push_back(static_cast<duint8>(x));
    v.push_back(static_cast<duint8>(x >> 8));
    v.push_back(static_cast<duint8>(x >> 16));
    v.push_back(static_cast<duint8>(x >> 24));
}

static void putRS(std::vector<duint8>& v, duint16 x) {
    v.push_back(static_cast<duint8>(x));
    v.push_back(static_cast<duint8>(x >> 8));
}

static void putRLL(std::vector<duint8>& v, duint64 x) {
    putRL(v, static_cast<duint32>(x));
    putRL(v, static_cast<duint32>(x >> 32));
}

static void patchRL(std::vector<duint8>& v, size_t offset, duint32 x) {
    v[offset+0] = static_cast<duint8>(x);
    v[offset+1] = static_cast<duint8>(x >> 8);
    v[offset+2] = static_cast<duint8>(x >> 16);
    v[offset+3] = static_cast<duint8>(x >> 24);
}

static void patchRLL(std::vector<duint8>& v, size_t offset, duint64 x) {
    patchRL(v, offset,     static_cast<duint32>(x));
    patchRL(v, offset + 4, static_cast<duint32>(x >> 32));
}

// --- R2004 page builders -----------------------------------------------------

// Build a sys page (Section Page Map or Data Section Map).
// Layout: +0 pageType  +4 decompSize  +8 compSize  +12 compType  +16 checksum
//         +20 data
static std::vector<duint8> buildSysPage(duint32 pageType,
                                        const duint8* data, duint32 dataSz) {
    std::vector<duint8> pg;
    pg.reserve(20 + dataSz);
    putRL(pg, pageType);    // +0
    putRL(pg, dataSz);      // +4 decompressed size (= data size, store mode)
    putRL(pg, dataSz);      // +8 compressed size (= data size, store mode)
    putRL(pg, 1);           // +12 compression type 1 = store
    putRL(pg, 0);           // +16 checksum placeholder

    // Compute checksum: seed over header[0..15] with [16..19]=0, then over data.
    duint32 ckH = dwgUtil::checksum18(0, pg.data(), 20);
    duint32 ckD = dwgUtil::checksum18(ckH, data, dataSz);
    patchRL(pg, 16, ckD);

    pg.insert(pg.end(), data, data + dataSz);
    return pg;
}

// Build a data page.  The 32-byte page header is XOR-encrypted with
// decrypt18Hdr using pageAddr as the key offset.
// Layout: 32-byte encrypted header + data bytes (uncompressed).
static std::vector<duint8> buildDataPage(duint64 pageAddr,
                                         duint32 sectionNum,
                                         const duint8* data,
                                         duint32 dataSz) {
    // Build decrypted 32-byte header.
    duint8 hdr[32] = {};
    auto rl = [&](int off, duint32 x) {
        hdr[off+0] = static_cast<duint8>(x);
        hdr[off+1] = static_cast<duint8>(x >> 8);
        hdr[off+2] = static_cast<duint8>(x >> 16);
        hdr[off+3] = static_cast<duint8>(x >> 24);
    };
    rl( 0, 0x4163043b);  // data page type
    rl( 4, sectionNum);  // section number
    rl( 8, dataSz);      // compressed size (= dataSz, store mode)
    rl(12, dataSz);      // uncompressed size
    rl(16, 0);           // start offset = 0 (single-page section)
    rl(20, 0);           // unknown
    rl(24, 0);           // header checksum placeholder
    rl(28, 0);           // data checksum placeholder

    // Data checksum (seed=0 over the page data).
    duint32 ckData = dwgUtil::checksum18(0, data, dataSz);
    rl(28, ckData);
    // Header checksum: bytes 24-27 zeroed (they hold the field being computed).
    rl(24, 0);
    duint32 ckHdr = dwgUtil::checksum18(ckData, hdr, 32);
    rl(24, ckHdr);

    // Encrypt the header using the page's file address.
    dwgCompressor::decrypt18Hdr(hdr, 32, pageAddr);  // symmetric XOR

    std::vector<duint8> pg;
    pg.reserve(32 + dataSz);
    pg.insert(pg.end(), hdr, hdr + 32);
    pg.insert(pg.end(), data, data + dataSz);
    return pg;
}

// Build the decompressed content of the Section Page Map.
// Each entry: dint32 id (RL) + duint32 size (RL).  All IDs are positive
// (no gap pages needed for a fresh write).
static std::vector<duint8> buildSectionPageMapContent(
        const std::vector<duint32>& pageSizes) {
    std::vector<duint8> v;
    v.reserve(pageSizes.size() * 8);
    for (size_t i = 0; i < pageSizes.size(); ++i) {
        putRL(v, static_cast<duint32>(i + 1));
        putRL(v, pageSizes[i]);
    }
    return v;
}

// Append one section description to the Data Section Map buffer.
// secId: 0-based section index.  pageNum: page ID in the Section Page Map.
// dataSize: size of decompressed section content.
static void appendSectionDesc(std::vector<duint8>& v,
                               duint32 secId, duint32 pageNum,
                               duint64 dataSize, const char* name) {
    putRLL(v, dataSize);         // size of section (uint64)
    putRL (v, 1);                // pageCount = 1
    putRL (v, static_cast<duint32>(dataSize));  // maxSize
    putRL (v, 0);                // unknown
    putRL (v, 1);                // compressed = 1 (store)
    putRL (v, secId);            // section Id
    putRL (v, 0);                // encrypted = 0

    // 64-byte null-padded name.
    duint8 nameBuf[64] = {};
    std::strncpy(reinterpret_cast<char*>(nameBuf), name, 63);
    v.insert(v.end(), nameBuf, nameBuf + 64);

    // 1 page entry: page number (RL) + dataSize (RL) + startOffset (RLL=0).
    putRL (v, pageNum);
    putRL (v, static_cast<duint32>(dataSize));  // page dataSize
    putRLL(v, 0);                               // startOffset = 0
}

// Build the decompressed content of the Data Section Map.
static std::vector<duint8> buildDataSectionMapContent(
        duint32 hdrSz, duint32 clsSz, duint32 objSz, duint32 hdlSz,
        bool hasAuxHeader, duint32 auxHeaderSz) {
    std::vector<duint8> v;
    const duint32 numDescriptions = hasAuxHeader ? 5 : 4;
    // 20-byte header.
    putRL(v, numDescriptions);
    putRL(v, 0x02);
    putRL(v, 0x00007400);
    putRL(v, 0x00);
    putRL(v, numDescriptions);

    appendSectionDesc(v, 0, 1, hdrSz, "AcDb:Header");
    appendSectionDesc(v, 1, 2, clsSz, "AcDb:Classes");
    appendSectionDesc(v, 2, 3, objSz, "AcDb:AcDbObjects");
    appendSectionDesc(v, 3, 4, hdlSz, "AcDb:Handles");
    if (hasAuxHeader)
        appendSectionDesc(v, 4, 5, auxHeaderSz, "AcDb:AuxHeader");
    return v;
}

static duint16 auxHeaderRawVersion(DRW::Version version) {
    switch (version) {
    case DRW::AC1018: return 25;
    case DRW::AC1021: return 27;
    case DRW::AC1024: return 29;
    case DRW::AC1027: return 31;
    case DRW::AC1032: return 33;
    default: return 0;
    }
}

static double headerDoubleVar(const DRW_Header *header, const std::string& key) {
    if (header == nullptr)
        return 0.0;
    auto it = header->vars.find(key);
    if (it == header->vars.end() || it->second == nullptr
        || it->second->type() != DRW_Variant::DOUBLE) {
        return 0.0;
    }
    return it->second->d_val();
}

static void splitAuxDate(double stored, dint32& day, dint32& msec) {
    day = static_cast<dint32>(stored);
    double frac = stored - static_cast<double>(day);
    if (frac == 0.0) {
        msec = 0;
        return;
    }
    for (int i = 0; i < 10; ++i) {
        frac *= 10.0;
        const double rounded = std::round(frac);
        if (std::abs(frac - rounded) < 1e-9 && rounded != 0.0) {
            msec = static_cast<dint32>(rounded);
            return;
        }
    }
    msec = static_cast<dint32>(std::round(frac));
}

static void putAuxDate(std::vector<duint8>& v, double stored) {
    dint32 day = 0;
    dint32 msec = 0;
    splitAuxDate(stored, day, msec);
    putRL(v, static_cast<duint32>(day));
    putRL(v, static_cast<duint32>(msec));
}

static std::vector<duint8> buildAuxHeaderContent(DRW::Version version,
                                                 const DRW_Header *header) {
    std::vector<duint8> v;
    const duint16 rawVersion = auxHeaderRawVersion(version);
    if (rawVersion == 0)
        return v;

    v.push_back(0xff);
    v.push_back(0x77);
    v.push_back(0x01);
    putRS(v, rawVersion);
    putRS(v, 0);                       // maintenance version
    putRL(v, 1);                       // number of saves
    putRL(v, static_cast<duint32>(-1));
    putRS(v, 1);                       // saves part 1
    putRS(v, 0);                       // saves part 2
    putRL(v, 0);
    putRS(v, rawVersion);
    putRS(v, 0);
    putRS(v, rawVersion);
    putRS(v, 0);
    putRS(v, 0x0005);
    putRS(v, 0x0893);
    putRS(v, 0x0005);
    putRS(v, 0x0893);
    putRS(v, 0);
    putRS(v, 1);
    for (int i = 0; i < 5; ++i)
        putRL(v, 0);

    putAuxDate(v, headerDoubleVar(header, "TDCREATE"));
    putAuxDate(v, headerDoubleVar(header, "TDUPDATE"));

    const duint32 handSeed = header ? header->getHandSeed() : 0;
    putRL(v, handSeed <= 0x7fffffffu ? handSeed : static_cast<duint32>(-1));
    putRL(v, 0);                       // educational plot stamp
    putRS(v, 0);
    putRS(v, 1);
    putRL(v, 0);
    putRL(v, 0);
    putRL(v, 0);
    putRL(v, 1);                       // number of saves
    putRL(v, 0);
    putRL(v, 0);
    putRL(v, 0);
    putRL(v, 0);

    if (version >= DRW::AC1032) {
        putRS(v, 0);
        putRS(v, 0);
        putRS(v, 0);
    }
    return v;
}

// Build the 0x100-byte fixed file header.
// secPageMapAddr: absolute file address of the Section Page Map sys page.
// secMapId: page ID of the Data Section Map page (= 5).
// lastPageEndAddr: absolute file address after the last page listed in the
//                  Section Page Map (= start of Section Page Map page).
// numPages: total pages listed in the Section Page Map (= 5).
static std::vector<duint8> buildFileHeader(duint64 secPageMapAddr,
                                           duint32 secMapId,
                                           duint64 lastPageEndAddr,
                                           duint32 numPages,
                                           const char* verStr = "AC1018") {
    std::vector<duint8> hdr(0x100, 0);

    std::memcpy(hdr.data(), verStr, 6);
    // Byte 11: maintenance version = 0 (already 0).
    // Byte 12: 0x00 (already 0).
    // Bytes 13-16: preview image position = 0 (no preview).
    // Bytes 17-18: app version / maintenance = 0.
    // Bytes 19-20: codepage = 30 (ANSI_1252), little-endian.
    hdr[19] = 30;
    hdr[20] = 0;
    // Bytes 21-23: zeros.  Bytes 24-27: security flags = 0.
    // Bytes 28-31: unknown = 0.  Bytes 32-35: summaryInfo = 0.
    // Bytes 36-39: VBA address = 0.
    // Bytes 40-43: constant 0x00000080.
    hdr[40] = 0x80; hdr[41] = 0; hdr[42] = 0; hdr[43] = 0;
    // Bytes 44-127: zeros (padding to 0x80).

    // ---- Encrypted variable header at offset 0x80 (0x6C bytes) -----
    duint8 varHdr[0x6C] = {};
    // Bytes 0-11: ID string "AcFssFcAJMB\0".
    std::memcpy(varHdr, "AcFssFcAJMB", 11);
    varHdr[11] = 0;

    auto vrl = [&](int off, duint32 x) {
        varHdr[off+0] = static_cast<duint8>(x);
        varHdr[off+1] = static_cast<duint8>(x >> 8);
        varHdr[off+2] = static_cast<duint8>(x >> 16);
        varHdr[off+3] = static_cast<duint8>(x >> 24);
    };
    auto vrll = [&](int off, duint64 x) {
        vrl(off,   static_cast<duint32>(x));
        vrl(off+4, static_cast<duint32>(x >> 32));
    };

    vrl(12, 0x00000000);
    vrl(16, 0x0000006C);
    vrl(20, 0x00000004);
    vrl(24, 0);  // root tree node gap
    vrl(28, 0);  // lowermost left tree node gap
    vrl(32, 0);  // lowermost right tree node gap
    vrl(36, 1);  // unknown long (1)
    vrl(40, numPages);  // last section page Id = last page in Section Page Map
    vrll(44, lastPageEndAddr - 0x100);  // last section page end addr (relative: - 0x100)
    vrll(52, 0);         // start of second header = 0
    vrl(60, 0);          // gap amount
    vrl(64, numPages);   // section page amount
    vrl(68, 0x20);
    vrl(72, 0x80);
    vrl(76, 0x40);
    vrl(80, static_cast<duint32>(-1));  // Section Page Map Id (informational)
    vrll(84, secPageMapAddr - 0x100);   // Section Page Map address (relative)
    vrl(92, secMapId);           // Section Map Id
    vrl(96, numPages);           // Section page array size
    vrl(100, 0);                 // Gap array size
    vrl(104, 0);                 // CRC32 placeholder (zeroed for computation)

    // Compute CRC32 over the 0x6C bytes with last 4 zeroed.
    duint32 crc = dwgUtil::crc32(0, varHdr, 0x6C);
    vrl(104, crc);

    // XOR-encrypt with kMagicNum18 and store at file offset 0x80.
    for (int i = 0; i < 0x6C; ++i)
        hdr[0x80 + i] = static_cast<duint8>(kMagicNum18[i] ^ varHdr[i]);

    // Tail magic at 0xEC.
    std::memcpy(hdr.data() + 0xEC, kMagicNumEnd18, 0x14);

    return hdr;
}

// --- dwgWriter18::objectBaseOffset -------------------------------------------

duint32 dwgWriter18::objectBaseOffset() const {
    auto it1 = m_sectionOffsets.find(recno::CLASSES);
    auto it2 = m_sectionSizes.find(recno::CLASSES);
    if (it1 == m_sectionOffsets.end() || it2 == m_sectionSizes.end()) return 0;
    return it1->second + it2->second;
}

// --- dwgWriter18::writeDwgClasses --------------------------------------------
// R2004 CLASSES format differs from R2000.  R2000 stores raw class-entry bytes;
// R2004 adds BS maxClassNum + RC + RC + Bit before the class entries.  The
// R2004 reader (dwgreader18) reads those fields and rejects any maxClassNum<499.

bool dwgWriter18::writeDwgClasses() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // maxClassNum = 499 means zero custom classes (DRW classes start at 500).
    m_buf.putBitShort(499);
    m_buf.putRawChar8(0);  // rc1
    m_buf.putRawChar8(0);  // rc2
    m_buf.putBit(0);       // flag
    m_buf.alignToByte();
    // One padding byte that the reader skips via setPosition(pos+1).
    m_buf.putRawChar8(0);

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::CLASSES_END);

    m_sectionSizes[recno::CLASSES] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter18::finalize ---------------------------------------------------

bool dwgWriter18::finalize() {
    if (m_stream == nullptr || !m_stream->good()) return false;

    // Extract section ranges from the inherited m_buf accumulator.
    const std::vector<duint8>& raw = m_buf.data();
    const duint8* rawPtr = raw.data();

    auto offIt = m_sectionOffsets.find(0);  // recno::HEADER
    auto szIt  = m_sectionSizes  .find(0);
    if (offIt == m_sectionOffsets.end() || szIt == m_sectionSizes.end())
        return false;
    duint32 hdrOff = offIt->second;
    duint32 hdrSz  = szIt->second;

    offIt = m_sectionOffsets.find(1);  // recno::CLASSES
    szIt  = m_sectionSizes  .find(1);
    if (offIt == m_sectionOffsets.end() || szIt == m_sectionSizes.end())
        return false;
    duint32 clsOff = offIt->second;
    duint32 clsSz  = szIt->second;

    offIt = m_sectionOffsets.find(2);  // recno::HANDLES
    szIt  = m_sectionSizes  .find(2);
    if (offIt == m_sectionOffsets.end() || szIt == m_sectionSizes.end())
        return false;
    duint32 hdlOff = offIt->second;
    duint32 hdlSz  = szIt->second;

    duint32 objOff = clsOff + clsSz;
    duint32 objSz  = hdlOff - objOff;

    // Build the four data pages (with correct file addresses).
    // Addresses accumulate from 0x100; data page size = 32 + dataSize.
    constexpr duint64 kBase = 0x100;
    duint64 addr1 = kBase;
    duint64 addr2 = addr1 + 32 + hdrSz;
    duint64 addr3 = addr2 + 32 + clsSz;
    duint64 addr4 = addr3 + 32 + objSz;

    auto hdrPage = buildDataPage(addr1, 0, rawPtr + hdrOff, hdrSz);
    auto clsPage = buildDataPage(addr2, 1, rawPtr + clsOff, clsSz);
    auto objPage = buildDataPage(addr3, 2, rawPtr + objOff, objSz);
    auto hdlPage = buildDataPage(addr4, 3, rawPtr + hdlOff, hdlSz);

    const std::vector<duint8> auxHeaderData =
        (m_version >= DRW::AC1027) ? buildAuxHeaderContent(m_version, m_header)
                                   : std::vector<duint8>();
    const bool hasAuxHeader = !auxHeaderData.empty();

    duint64 addr5 = addr4 + 32 + hdlSz;
    std::vector<duint8> auxHeaderPage;
    if (hasAuxHeader)
        auxHeaderPage = buildDataPage(addr5, 4, auxHeaderData.data(),
                                      static_cast<duint32>(auxHeaderData.size()));

    // Build the Data Section Map sys page.
    auto dsmData = buildDataSectionMapContent(
        hdrSz, clsSz, objSz, hdlSz, hasAuxHeader,
        static_cast<duint32>(auxHeaderData.size()));
    auto dsmPage = buildSysPage(0x4163003b, dsmData.data(),
                                static_cast<duint32>(dsmData.size()));

    // Data Section Map sys page follows the optional AuxHeader data page.
    const duint64 addrDsm = hasAuxHeader
        ? addr5 + static_cast<duint64>(auxHeaderPage.size())
        : addr5;
    duint64 addrSPM = addrDsm + static_cast<duint64>(dsmPage.size());

    // Build Section Page Map sys page.
    std::vector<duint32> pageSizes = {
        static_cast<duint32>(hdrPage.size()),
        static_cast<duint32>(clsPage.size()),
        static_cast<duint32>(objPage.size()),
        static_cast<duint32>(hdlPage.size())
    };
    if (hasAuxHeader)
        pageSizes.push_back(static_cast<duint32>(auxHeaderPage.size()));
    pageSizes.push_back(static_cast<duint32>(dsmPage.size()));
    auto spmData = buildSectionPageMapContent(pageSizes);
    auto spmPage = buildSysPage(0x41630e3b, spmData.data(),
                                static_cast<duint32>(spmData.size()));

    // Build the 0x100-byte file header.
    // lastPageEndAddr = byte address just past the end of the last page listed
    // in the Section Page Map.  The SPM page is the last entry, so its end is
    // addrSPM + spmPage.size().
    const duint32 dataSectionMapPageId = hasAuxHeader ? 6 : 5;
    auto fileHdr = buildFileHeader(addrSPM, dataSectionMapPageId,
                                   addrSPM + static_cast<duint64>(spmPage.size()),
                                   static_cast<duint32>(pageSizes.size()),
                                   fileHeaderVersion());

    // Write everything.
    auto writeVec = [&](const std::vector<duint8>& v) {
        m_stream->write(reinterpret_cast<const char*>(v.data()),
                        static_cast<std::streamsize>(v.size()));
    };
    writeVec(fileHdr);
    writeVec(hdrPage);
    writeVec(clsPage);
    writeVec(objPage);
    writeVec(hdlPage);
    if (hasAuxHeader)
        writeVec(auxHeaderPage);
    writeVec(dsmPage);
    writeVec(spmPage);

    return m_stream->good();
}
