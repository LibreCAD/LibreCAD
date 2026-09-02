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
#include <limits>

namespace {
static std::string toUpperCase(const std::string& s) {
    std::string r = s;
    for (auto& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}
} // namespace

bool dwgWriter24::appendR2007StringStream(dwgBufferW& data,
                                          const dwgBufferW& strings,
                                          bool writeAbsentFooter) {
    const std::size_t stringSize = strings.data().size();
    if (stringSize > std::numeric_limits<std::uint32_t>::max())
        return false;

    const auto stringBytes = static_cast<std::uint32_t>(stringSize);
    if (stringBytes == 0 && !writeAbsentFooter)
        return true;

    if (stringBytes == 0) {
        for (int bit = 0; bit < 7; ++bit)
            data.putBit(0);
        data.putRawShort16(0);
        data.putBit(0);
        return true;
    }

    // The extended trailer stores a 31-bit bit count split as 16 + 15 bits.
    const std::uint64_t stringBitSize =
        static_cast<std::uint64_t>(stringBytes) * 8u + 7u;
    if (stringBitSize > 0x7FFFFFFFULL)
        return false;

    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int bit = 0; bit < 7; ++bit)
        data.putBit(0);
    if (stringBitSize <= 0x7FFFU) {
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    } else {
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize >> 15));
        data.putRawShort16(static_cast<std::uint16_t>(
            0x8000U | (stringBitSize & 0x7FFFU)));
    }
    data.putBit(stringBytes != 0 ? 1 : 0);
    return true;
}

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
    m_sectionOffsets[recno::HEADER] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // R2013 files emitted by this writer use application maintenance 6, so
    // the R2010/R2013 high-size field is required. R2018 always carries it.
    if (m_version == DRW::AC1027 || m_version >= DRW::AC1032)
        m_buf.putRawLong32(0);

    // AC1024+: RL bitSize placeholder.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    initHeaderControlHandles();

    if (m_header != nullptr) {
        dwgBufferW dataBuf;
        dwgBufferW handleBuf;
        dwgBufferW strBuf;
        // Propagate encode failure: writeDwgHeader reports success as a bool.
        if (!m_header->encodeDwg(m_version, &dataBuf, &handleBuf, &strBuf))
            return false;
        dataBuf.alignToByte();
        strBuf.alignToByte();
        handleBuf.alignToByte();

        if (m_version > DRW::AC1018) {
            if (!appendR2007StringStream(dataBuf, strBuf, true))
                return false;
        }

        if (dataBuf.size() >
            (std::numeric_limits<std::uint32_t>::max() - 32u) / 8u)
            return false;
        std::uint32_t bitSize = 32u + static_cast<std::uint32_t>(dataBuf.size()) * 8u;
        m_buf.patchRawLong32(bitSizeOffset, bitSize);

        const std::uint32_t dataStartBit = m_buf.bitCount();
        m_buf.putBytes(dataBuf.data().data(), dataBuf.size());
        recordHeaderHandseedOffset(dataStartBit);
        if (handleBuf.size() > 0)
            m_buf.putBytes(handleBuf.data().data(), handleBuf.size());
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::writeDwgClasses -------------------------------------------
// AC1024 CLASSES section differs from AC1018 in two ways:
//   1. An RL bitSize field is inserted after RL size.
//   2. Eight ODA-defined unknown zero bytes are written before the end
//      sentinel.
//
bool dwgWriter24::writeDwgClasses() {
    if (hasDwgClassConflict() || !validateDwgClassManifest())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    const bool hasHSize = m_version == DRW::AC1027 || m_version >= DRW::AC1032;
    if (hasHSize)
        m_buf.putRawLong32(0);

    // RL bitSize placeholder — back-patched after class data is written.
    size_t bitSizeOffset = m_buf.size();
    m_buf.putRawLong32(0);

    // Class numeric data and class strings are split for R2007+ in the same
    // way object bodies split data/strings/handles. The reader finds the
    // string stream by using the bitSize value plus the fixed CLASSES header
    // prefix (159 bits, or 191 bits when AC1032 hSize is present).
    m_buf.putBitShort(maxDwgClassNumber());
    m_buf.putRawChar8(0);   // rc1
    m_buf.putRawChar8(0);   // rc2
    m_buf.putBit(0);        // flag

    dwgBufferW classStrings;
    if (!emitDwgClassDefinitions(
            [this, &classStrings](const DwgClassDefinition& definition) {
                return writeDwgClassDefinition(definition, &m_buf,
                                               &classStrings);
            }))
        return false;

    m_buf.alignToByte();
    classStrings.alignToByte();

    if (classStrings.size() > 0)
        m_buf.putBytes(classStrings.data().data(), classStrings.size());
    const std::uint64_t stringBitSize =
        static_cast<std::uint64_t>(classStrings.size()) * 8u;
    if (stringBitSize > 0x7FFFFFFFu)
        return false;
    if (stringBitSize > 0x7FFFu) {
        // The extended footer is written high word first because the reader
        // walks backwards from the low word at the end of the stream.
        m_buf.putRawShort16(static_cast<std::uint16_t>(stringBitSize >> 15));
        m_buf.putRawShort16(static_cast<std::uint16_t>(
            0x8000u | (stringBitSize & 0x7FFFu)));
    } else {
        m_buf.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    }
    // The class string stream terminates with a true endbit.  bitSize includes
    // its own 32-bit field and everything through that marker; the reader's
    // 159/191-bit base then lands on the marker's first bit.
    m_buf.putBit(1);
    const std::uint64_t endBit =
        static_cast<std::uint64_t>(m_buf.bitCount())
        - static_cast<std::uint64_t>(sectionStart) * 8u;
    const std::uint64_t dataStartBit =
        (static_cast<std::uint64_t>(bitSizeOffset) + 4u) * 8u
        - static_cast<std::uint64_t>(sectionStart) * 8u;
    const std::uint64_t bitSize64 = 32u + endBit - dataStartBit;
    if (bitSize64 > std::numeric_limits<std::uint32_t>::max())
        return false;
    const std::uint32_t bitSize = static_cast<std::uint32_t>(bitSize64);
    m_buf.patchRawLong32(bitSizeOffset, bitSize);

    m_buf.alignToByte();

    // The RL covers the payload up to, but not including, the CRC.
    std::uint32_t payloadSize =
        static_cast<std::uint32_t>(m_buf.size()) - static_cast<std::uint32_t>(sizeOffset + 4);
    m_buf.patchRawLong32(sizeOffset, payloadSize);

    const std::uint16_t crc = m_buf.crc16(0xC0C1, sectionStart + 16, m_buf.size());
    m_buf.putRawShort16(crc);
    for (std::size_t i = 0; i < dwgSpec::kClassesUnknownTailBytes; ++i)
        m_buf.putRawChar8(0);

    m_buf.putBytes(dwgSentinels::CLASSES_END, 16);

    m_sectionSizes[recno::CLASSES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

// --- dwgWriter24::encodeEntity ----------------------------------------------
// Encodes a single entity using the AC1024 three-stream model:
//   m_objectBody   — numeric DATA fields
//   m_objectStrings — variable-text (TV/TU) string fields
//   m_objectHandles — handle fields
// finishObject() assembles these into the AC1024 wire format.

bool dwgWriter24::encodeEntity(DRW_Entity *ent) {
    if (ent == nullptr || objectWriteFailed() || blockControlEmitted())
        return false;
    if (!prepareEntityEed(*ent)) {
        m_entityEedWriteFailure = !ent->extData.empty();
        return false;
    }
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
            : static_cast<std::uint32_t>(0x12);
    }
    if (ent->lTypeH.ref == 0 && !ent->lineType.empty()) {
        if (auto it = m_writingCtx.ltypeMap.find(toUpperCase(ent->lineType));
            it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }
    if (ent->eType == DRW::MLINE) {
        auto *mline = dynamic_cast<DRW_MLine *>(ent);
        if (mline != nullptr && mline->styleHandle == 0 && !mline->styleName.empty()) {
            if (auto styleIt = m_writingCtx.mlineStyleMap.find(toUpperCase(mline->styleName));
                styleIt != m_writingCtx.mlineStyleMap.end())
                mline->styleHandle = styleIt->second;
        }
    }
    prepareBlockOwnedEntity(*ent);
    if (!canRecordBlockOwnedEntity(*ent))
        return false;

    beginObject(ent->handle);
    bool ok = false;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        ok = ent->encodeDwg(m_version, &m_objectBody, 0,
                             &m_objectStrings, &m_objectHandles);
    } else {
        ok = ent->encodeDwg(m_version, &m_objectBody, 0);
    }
    if (!ok) {
        if (!ent->extData.empty())
            m_entityEedWriteFailure = true;
        return false;
    }
    finishObject();
    if (objectWriteFailed())
        return false;
    if (!recordBlockOwnedEntity(*ent))
        return false;
    return recordBlockInsertReference(*ent);
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
    // Keep the R2010+ three-stream writer consistent with the legacy frame
    // assembler: reject duplicate handles before appending a second frame.
    if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                    [this](const auto& entry) {
                        return entry.first == m_currentHandle;
                    })) {
        m_writeError = true;
        return;
    }

    // --- Assemble the DATA section -----------------------------------------
    if (!m_buf.isGood() || !m_objectBody.isGood() || !m_objectStrings.isGood()
        || !m_objectHandles.isGood()) {
        m_writeError = true;
        return;
    }
    // The footer stores strBytes * 8 + 7 bits. Its low size word carries
    // 0x8000 when the preceding word supplies the upper 16 bits.
    m_objectBody.alignToByte();
    const std::uint64_t mergedStringBaseBit = m_objectBody.bitCount();
    if (!appendR2007StringStream(m_objectBody, m_objectStrings, true)) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    // The short trailer occupies 3 bytes; extended trailers occupy 5 bytes.

    // --- Byte-align the handle section -------------------------------------
    m_objectHandles.alignToByte();

    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectHandles.size() > std::numeric_limits<std::uint32_t>::max()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const auto dataBytes = static_cast<std::uint32_t>(m_objectBody.size());
    const auto handleBytes = static_cast<std::uint32_t>(m_objectHandles.size());
    const std::uint64_t totalBytes64 =
        static_cast<std::uint64_t>(dataBytes) + handleBytes;
    if (handleBytes > std::numeric_limits<std::uint32_t>::max() / 8u
        || totalBytes64 > std::numeric_limits<std::int32_t>::max()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    const auto totalBytes = static_cast<std::uint32_t>(totalBytes64);
    const auto bs = handleBytes * 8u;  // bit count of handle section

    std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());

    // MS totalBodyBytes + UMC bs + body bytes.
    m_buf.putModularShort(static_cast<std::int32_t>(totalBytes));
    if (!m_buf.isGood()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    m_buf.putUModularChar(bs);
    if (!m_buf.isGood()) {
        m_writeError = true;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    size_t bodyStart = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), dataBytes);
    if (handleBytes > 0)
        m_buf.putBytes(m_objectHandles.data().data(), handleBytes);

    // CRC covers MS prefix + UMC bs + body bytes.
    std::uint16_t crc = m_buf.crc16(0xC0C1, frameStart, bodyStart + totalBytes);
    m_buf.putRawShort16(crc);

    if (!captureLastDwgObjectHandleOccurrences(mergedStringBaseBit, 0, true,
                                               false)) {
        m_buf.truncate(frameStart);
        m_writeError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    m_objectMap.emplace_back(m_currentHandle, frameStart);
    markDwgClassInstanceEmitted(m_currentHandle);
    m_currentHandle = 0;
}

bool dwgWriter24::finalize() {
    return !m_writeError && dwgWriter18::finalize();
}
