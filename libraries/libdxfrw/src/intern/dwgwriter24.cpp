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

#include "dwgwriter24.h"
#include "dwgutil.h"
#include "../drw_entities.h"
#include <cctype>

namespace {
static std::string toUpperCase(const std::string& s) {
    std::string r = s;
    for (auto& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}
} // namespace

// --- dwgWriter24::writeDwgHeader --------------------------------------------
// AC1024 HEADER section differs from AC1018 in that the handle stream is
// separated from the data stream.  Layout:
//   [begin sentinel 16B][RL size][RL bitSize][data bytes][handle bytes][RS CRC16][end sentinel 16B]
//
// bitSize = 32 + dataBytes * 8, so that the reader computes:
//   hBbuf->setPosition((160 + bitSize) >> 3) = 24 + dataBytes
// which is exactly the first byte after the data section.

bool dwgWriter24::writeDwgHeader() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HEADER] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // AC1024: RL bitSize placeholder.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    initHeaderControlHandles();

    if (m_header != nullptr) {
        dwgBufferW dataBuf;
        dwgBufferW handleBuf;
        m_header->encodeDwg(m_version, &dataBuf, &handleBuf);
        dataBuf.alignToByte();
        handleBuf.alignToByte();

        duint32 bitSize = 32u + static_cast<duint32>(dataBuf.size()) * 8u;
        m_buf.patchRawLong32(bitSizeOffset, bitSize);

        m_buf.putBytes(dataBuf.data().data(), dataBuf.size());
        if (handleBuf.size() > 0)
            m_buf.putBytes(handleBuf.data().data(), handleBuf.size());
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::writeDwgClasses -------------------------------------------
// AC1024 CLASSES section differs from AC1018 in two ways:
//   1. An RL bitSize field is inserted after RL size.
//   2. An extra RS (unknown CRC = 0) is written before the end sentinel.
//
// For empty classes (no custom entries, maxClassNum=499), the strBuf
// navigation in the reader uses:
//   strStartPos = bitSize + 159  (hasHSize=false for maintenanceVersion=0)
// where bitSize + 143 must point (in byte units within the section data
// starting from the begin sentinel) to the extra padding byte that the
// reader skips with setPosition(pos+1) before reading the CRC.
//
// With our layout:
//   begin sentinel (16B) + RL size (4B) + RL bitSize (4B) +
//   class data (5B: BS+RC+RC+B+pad) + extra padding RC (1B)
// the extra padding byte is at byte offset 29 from the section start.
// bitSize = 29*8 - 143 = 89.
//
// This value is computed dynamically below so it stays correct if the
// class data size ever changes.

bool dwgWriter24::writeDwgClasses() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // RL bitSize placeholder — back-patched after class data is written.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    // Class header: maxClassNum=499 (no custom classes), RC, RC, B, align.
    m_buf.putBitShort(499);
    m_buf.putRawChar8(0);   // rc1
    m_buf.putRawChar8(0);   // rc2
    m_buf.putBit(0);        // flag
    m_buf.alignToByte();

    // 5-byte tail starting at extraPaddingByteOffset:
    //   bytes +0,+1: RS strDataSize — reader reads this from (bitSize+143)/8;
    //                MUST be 0x0000 for empty classes or strStartPos underflows.
    //   bytes +2,+3: RS "CRC"        — reader DBGs it but never validates.
    //   byte  +4:    RC unknownCRC   — reader DBGs it but never validates.
    // bitSize = extraPaddingByteOffset * 8 - 143, so (bitSize+143)/8 = extraPaddingByteOffset.
    size_t extraPaddingByteOffset = m_buf.size() - sectionStart;
    m_buf.putRawShort16(0);   // strDataSize = 0x0000
    m_buf.putRawShort16(0);   // CRC placeholder (not validated by reader)
    m_buf.putRawChar8(0);     // unknownCRC tail (not validated by reader)

    // Back-patch RL bitSize.
    duint32 bitSize = static_cast<duint32>(extraPaddingByteOffset) * 8 - 143;
    m_buf.patchRawLong32(bitSizeOffset, bitSize);

    // Patch RL size and write end sentinel.
    duint32 payloadSize =
        static_cast<duint32>(m_buf.size()) - static_cast<duint32>(sizeOffset + 4);
    m_buf.patchRawLong32(sizeOffset, payloadSize);

    m_buf.putBytes(dwgSentinels::CLASSES_END, 16);

    m_sectionSizes[recno::CLASSES] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::encodeEntity ----------------------------------------------
// Encodes a single entity using the AC1024 three-stream model:
//   m_objectBody   — numeric DATA fields
//   m_objectStrings — variable-text (TV/TU) string fields
//   m_objectHandles — handle fields
// finishObject() assembles these into the AC1024 wire format.

bool dwgWriter24::encodeEntity(DRW_Entity *ent) {
    if (ent == nullptr) return false;
    if (ent->handle == 0) {
        ent->handle = m_handles.next();
    } else {
        m_handles.reserve(ent->handle);
    }
    // Layer name → handle resolution (same logic as dwgWriter15::encodeEntity).
    if (ent->layerH.ref == 0) {
        auto layerUp = toUpperCase(ent->layer);
        auto it = m_writingCtx.layerMap.find(layerUp);
        ent->layerH.ref = (it != m_writingCtx.layerMap.end())
            ? it->second
            : static_cast<duint32>(0x12);
    }
    if (ent->lTypeH.ref == 0 && !ent->lineType.empty()) {
        auto ltUp = toUpperCase(ent->lineType);
        auto it = m_writingCtx.ltypeMap.find(ltUp);
        if (it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }

    beginObject(ent->handle);
    m_objectStrings.reset();
    m_objectHandles.reset();

    bool ok = ent->encodeDwg(m_version, &m_objectBody, 0,
                             &m_objectStrings, &m_objectHandles);
    if (!ok) return false;
    finishObject();
    return true;
}

// --- dwgWriter24::finishObject ----------------------------------------------
// AC1024 object wire format:
//   [MS totalBodyBytes][UMC handleBits][data section bytes][handle section bytes][RS CRC16]
//
// data section = m_objectBody bytes + string section tail (RS strBitCount + B flag) + pad
// handle section = m_objectHandles bytes (byte-aligned)
//
// objSize (used by reader to locate handle section) = dataSectionBytes * 8.
// handleBits (= bs) = handleSectionBytes * 8.

void dwgWriter24::finishObject() {
    // --- Assemble the DATA section -----------------------------------------
    // String section footer: 7 zero bits + RS(strDataSize) + flag bit = 24 bits.
    // Flag at bit objSize-1 (= LSB of last data byte after alignToByte + 7 zeros
    // + 16 RS bits + 1 flag bit).  RS at bits [objSize-17..objSize-2].
    // strDataSize = strBytes*8+7 so moveBitPos(-strDataSize-16) from after RS
    // lands at the first string byte (byte-aligned, bitPos=0).
    // Derivation: after getRawShort16 at bitPos=7, getPosition()=filePos-1.
    // moveBitPos(b) with b=-strDataSize-9 sets bitPos=b&7 and shifts by b>>3.
    // Need bitPos=0: b&7=0 → strDataSize≡7(mod 8). Need landing at preamble
    // start: (dataBytes-1)+(b>>3)=dataBytesBeforeStrings=(dataBytes-strBytes-3)
    // → b=-((strBytes+2)*8) → strDataSize=strBytes*8+7.
    m_objectBody.alignToByte();
    duint32 strBytes = static_cast<duint32>(m_objectStrings.data().size());
    bool hasStrings = (strBytes > 0);
    if (hasStrings)
        m_objectBody.putBytes(m_objectStrings.data().data(), strBytes);
    duint16 strDataSize = hasStrings
        ? static_cast<duint16>(strBytes * 8u + 7u)
        : 0u;
    for (int i = 0; i < 7; ++i) m_objectBody.putBit(0);
    m_objectBody.putRawShort16(strDataSize);
    m_objectBody.putBit(hasStrings ? 1 : 0);
    // 7+16+1 = 24 bits = 3 bytes; body is now byte-aligned.

    // --- Byte-align the handle section -------------------------------------
    m_objectHandles.alignToByte();

    duint32 dataBytes   = static_cast<duint32>(m_objectBody.size());
    duint32 handleBytes = static_cast<duint32>(m_objectHandles.size());
    duint32 totalBytes  = dataBytes + handleBytes;
    duint32 bs          = handleBytes * 8;  // bit count of handle section

    duint32 frameStart = static_cast<duint32>(m_buf.size());

    // MS totalBodyBytes + UMC bs + body bytes.
    m_buf.putModularShort(static_cast<dint32>(totalBytes));
    m_buf.putUModularChar(bs);
    size_t bodyStart = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), dataBytes);
    if (handleBytes > 0)
        m_buf.putBytes(m_objectHandles.data().data(), handleBytes);

    // CRC covers MS prefix + UMC bs + body bytes.
    duint16 crc = m_buf.crc16(0xC0C1, frameStart, bodyStart + totalBytes);
    m_buf.putRawShort16(crc);

    m_objectMap.emplace_back(m_currentHandle, frameStart);
    m_currentHandle = 0;
}
