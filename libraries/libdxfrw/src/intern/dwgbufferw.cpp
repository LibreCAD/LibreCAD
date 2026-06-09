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

#include "dwgbufferw.h"

#include <cstring>

#include "../drw_base.h"
#include "drw_textcodec.h"

// CRC-CCITT lookup table — same constants the reader uses (the file's
// crctable[256] is static-linked, so we duplicate it locally; small
// (512 bytes) and keeps the writer independent of the reader's TU).
static const unsigned int crcTable[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

dwgBufferW::dwgBufferW(DRW_TextCodec *decoder)
    : m_decoder{decoder}
{}

void dwgBufferW::alignToByte() {
    if (m_bitPos != 0)
        m_bitPos = 0;
}

void dwgBufferW::appendAlignedByte(duint8 b) {
    m_buf.push_back(b);
}

// ---- bit-level primitives -------------------------------------------------

void dwgBufferW::putBit(duint8 b) {
    if (m_bitPos == 0)
        m_buf.push_back(0);
    m_buf.back() = static_cast<duint8>(m_buf.back() | ((b & 1) << (7 - m_bitPos)));
    m_bitPos = static_cast<duint8>((m_bitPos + 1) & 7);
}

void dwgBufferW::putBoolBit(bool b) {
    putBit(b ? 1 : 0);
}

void dwgBufferW::put2Bits(duint8 b) {
    // MSB first: bit-1 of the 2-bit field comes out first.
    putBit((b >> 1) & 1);
    putBit(b & 1);
}

void dwgBufferW::put3Bits(duint8 b) {
    putBit((b >> 2) & 1);
    putBit((b >> 1) & 1);
    putBit(b & 1);
}

void dwgBufferW::putBitShort(duint16 v) {
    // Inverse of getBitShort: 2-bit code selects width.
    //   code 0b00 → followed by RS (16 bits LE)
    //   code 0b01 → followed by RC (8 bits)
    //   code 0b10 → value is 0, no payload
    //   code 0b11 → value is 256, no payload
    if (v == 0) {
        put2Bits(0b10);
    } else if (v == 256) {
        put2Bits(0b11);
    } else if (v < 256) {
        put2Bits(0b01);
        putRawChar8(static_cast<duint8>(v));
    } else {
        put2Bits(0b00);
        putRawShort16(v);
    }
}

void dwgBufferW::putSBitShort(dint16 v) {
    // Reader's getSBitShort returns signed; same compression rules but
    // the 8-bit shortcut sign-extends.  For round-trip safety only use
    // the shortcut when the value fits an unsigned byte (0..255).
    if (v == 0) {
        put2Bits(0b10);
    } else if (v == 256) {
        put2Bits(0b11);
    } else if (v >= 0 && v < 256) {
        put2Bits(0b01);
        putRawChar8(static_cast<duint8>(v));
    } else {
        put2Bits(0b00);
        putRawShort16(static_cast<duint16>(v));
    }
}

void dwgBufferW::putBitLong(dint32 v) {
    // Inverse of getBitLong:
    //   code 0b00 → RL (32 bits LE)
    //   code 0b01 → RC (8 bits)
    //   code 0b10 → value is 0
    if (v == 0) {
        put2Bits(0b10);
    } else if (v > 0 && v < 256) {
        put2Bits(0b01);
        putRawChar8(static_cast<duint8>(v));
    } else {
        put2Bits(0b00);
        putRawLong32(static_cast<duint32>(v));
    }
}

void dwgBufferW::putBitLongLong(duint64 v) {
    // Inverse of getBitLongLong: 3 bits = byte count, then that many
    // bytes MSB-first.
    duint8 n = 0;
    duint64 t = v;
    while (t != 0 && n < 7) {
        t >>= 8;
        ++n;
    }
    put3Bits(n);
    for (int i = n - 1; i >= 0; --i)
        putRawChar8(static_cast<duint8>((v >> (i * 8)) & 0xFF));
}

void dwgBufferW::putBitDouble(double d) {
    // Inverse of getBitDouble:
    //   code 0b00 → followed by 8 raw bytes (IEEE 754)
    //   code 0b01 → value is 1.0
    //   code 0b10 → value is 0.0
    // Bit-exact compare on 0.0 / 1.0 (their IEEE patterns are unique).
    duint64 bits;
    std::memcpy(&bits, &d, 8);
    if (bits == 0x3FF0000000000000ULL) { // 1.0
        put2Bits(0b01);
    } else if (bits == 0x0000000000000000ULL) { // +0.0
        put2Bits(0b10);
    } else {
        put2Bits(0b00);
        // Emit the 8 bytes via putRawChar8 to respect current bit cursor.
        duint8 buf[8];
        std::memcpy(buf, &d, 8);
        for (int i = 0; i < 8; ++i)
            putRawChar8(buf[i]);
    }
}

void dwgBufferW::put3BitDouble(const DRW_Coord& c) {
    putBitDouble(c.x);
    putBitDouble(c.y);
    putBitDouble(c.z);
}

// ---- raw fixed-width primitives ------------------------------------------

void dwgBufferW::putRawChar8(duint8 v) {
    if (m_bitPos == 0) {
        m_buf.push_back(v);
        return;
    }
    // Top (8 - bitPos) bits of v fill the remaining low bits of the
    // current byte; bottom bitPos bits become the high bits of a new byte.
    m_buf.back() = static_cast<duint8>(m_buf.back() | (v >> m_bitPos));
    m_buf.push_back(static_cast<duint8>((v << (8 - m_bitPos)) & 0xFF));
}

void dwgBufferW::putRawShort16(duint16 v) {
    putRawChar8(static_cast<duint8>(v & 0xFF));
    putRawChar8(static_cast<duint8>((v >> 8) & 0xFF));
}

void dwgBufferW::putBERawShort16(duint16 v) {
    putRawChar8(static_cast<duint8>((v >> 8) & 0xFF));
    putRawChar8(static_cast<duint8>(v & 0xFF));
}

void dwgBufferW::putRawDouble(double d) {
    duint8 buf[8];
    std::memcpy(buf, &d, 8);
    for (int i = 0; i < 8; ++i)
        putRawChar8(buf[i]);
}

void dwgBufferW::putRawLong32(duint32 v) {
    putRawShort16(static_cast<duint16>(v & 0xFFFF));
    putRawShort16(static_cast<duint16>((v >> 16) & 0xFFFF));
}

void dwgBufferW::putRawLong64(duint64 v) {
    putRawLong32(static_cast<duint32>(v & 0xFFFFFFFFu));
    putRawLong32(static_cast<duint32>((v >> 32) & 0xFFFFFFFFu));
}

void dwgBufferW::put2RawDouble(const DRW_Coord& c) {
    putRawDouble(c.x);
    putRawDouble(c.y);
}

// ---- modular / variable-length encodings ---------------------------------

void dwgBufferW::putUModularChar(duint32 v) {
    // Emit 7-bit chunks LSB-first; all but the last have 0x80 set.
    // Reader caps at 4 chunks (the `for (int i=0; i<4; i++)` in
    // dwgBuffer::getUModularChar), so the writer caps at 4 too — a
    // 5-chunk emit would leave one stale byte the reader never reads.
    // Max representable: 2^28 - 1.  Values >= 2^28 silently truncate
    // to 28 bits, matching what the reader could represent anyway.
    duint8 chunks[4];
    int n = 0;
    do {
        chunks[n++] = static_cast<duint8>(v & 0x7F);
        v >>= 7;
    } while (v != 0 && n < 4);
    // Set continuation bit on all but the last chunk.
    for (int i = 0; i < n - 1; ++i)
        chunks[i] = static_cast<duint8>(chunks[i] | 0x80);
    for (int i = 0; i < n; ++i)
        putRawChar8(chunks[i]);
}

void dwgBufferW::putModularChar(dint32 v) {
    // Reader caps at 4 chunks (dwgBuffer::getModularChar `for (int i=0;
    // i<4; i++)`).  Writer matches the cap so a stale 5th chunk never
    // gets emitted.  Max representable magnitude with 1 sign bit:
    // 3 × 7-bit chunks + 1 × 6-bit chunk = 2^27 - 1.
    bool negative = v < 0;
    duint32 absv = negative ? static_cast<duint32>(-static_cast<dint64>(v))
                            : static_cast<duint32>(v);
    duint8 chunks[4];
    int n = 0;
    do {
        chunks[n++] = static_cast<duint8>(absv & 0x7F);
        absv >>= 7;
    } while (absv != 0 && n < 4);
    // The last chunk reserves 0x40 for the sign bit.  If magnitude
    // already occupies that bit, spill into an extra chunk — but only
    // if we have room (n < 4).  When n == 4 and 0x40 is taken, the
    // magnitude is out of MC spec; the value is truncated on the
    // reader side anyway, so we just drop the sign bit silently.
    if (chunks[n - 1] & 0x40) {
        if (n < 4) {
            chunks[n - 1] = static_cast<duint8>(chunks[n - 1] | 0x80);
            chunks[n++] = 0;
        }
    }
    if (negative && !(chunks[n - 1] & 0x40))
        chunks[n - 1] = static_cast<duint8>(chunks[n - 1] | 0x40);
    for (int i = 0; i < n - 1; ++i)
        chunks[i] = static_cast<duint8>(chunks[i] | 0x80);
    for (int i = 0; i < n; ++i)
        putRawChar8(chunks[i]);
}

void dwgBufferW::putModularShort(dint32 v) {
    // Reader handles at most 2 RS chunks; positive only.
    duint32 absv = static_cast<duint32>(v);
    duint16 lo = static_cast<duint16>(absv & 0x7FFF);
    duint16 hi = static_cast<duint16>((absv >> 15) & 0x7FFF);
    if (hi == 0) {
        // Single chunk (no continuation bit).
        putRawShort16(lo);
    } else {
        putRawShort16(static_cast<duint16>(lo | 0x8000));
        putRawShort16(hi);
    }
}

// ---- handles --------------------------------------------------------------

void dwgBufferW::putHandle(const dwgHandle& h) {
    duint8 size = 0;
    duint32 ref = h.ref;
    if (ref != 0) {
        duint32 t = ref;
        while (t != 0) {
            t >>= 8;
            ++size;
        }
    }
    duint8 header = static_cast<duint8>(((h.code & 0x0F) << 4) | (size & 0x0F));
    putRawChar8(header);
    // Emit MSB byte first.
    for (int i = size - 1; i >= 0; --i)
        putRawChar8(static_cast<duint8>((ref >> (i * 8)) & 0xFF));
}

// ---- strings -------------------------------------------------------------

void dwgBufferW::putVariableText(DRW::Version v, const std::string& utf8) {
    // R2000 (and earlier) → CP8 codepage path. R2007+ would emit UCS-2
    // via TU, which is out of scope for the R2000-only writer.
    (void)v;
    putCP8Text(utf8);
}

void dwgBufferW::putCP8Text(const std::string& utf8) {
    std::string encoded = m_decoder ? m_decoder->fromUtf8(utf8) : utf8;
    duint16 byteLen = static_cast<duint16>(encoded.size());
    putBitShort(byteLen);
    if (byteLen != 0)
        putBytes(reinterpret_cast<const duint8*>(encoded.data()), byteLen);
}

void dwgBufferW::putBytes(const duint8* buf, size_t n) {
    if (buf == nullptr || n == 0) return;
    if (m_bitPos == 0) {
        m_buf.insert(m_buf.end(), buf, buf + n);
        return;
    }
    for (size_t i = 0; i < n; ++i)
        putRawChar8(buf[i]);
}

// ---- specialty primitives ------------------------------------------------

void dwgBufferW::putExtrusion(const DRW_Coord& ext, bool b_R2000_style) {
    if (b_R2000_style) {
        if (ext.x == 0.0 && ext.y == 0.0 && ext.z == 1.0) {
            putBit(1);
            return;
        }
        putBit(0);
    }
    putBitDouble(ext.x);
    putBitDouble(ext.y);
    putBitDouble(ext.z);
}

void dwgBufferW::putDefaultDouble(double defaultVal, double d) {
    // Simplest correct encoding: always use code 0b11 (full RD) so the
    // value is emitted byte-exact.  The reader handles all four codes.
    // Phase 4 can revisit and pick smaller codes when low bytes match
    // the default, but correctness first.
    (void)defaultVal;
    put2Bits(0b11);
    putRawDouble(d);
}

void dwgBufferW::putThickness(double t, bool b_R2000_style) {
    if (b_R2000_style) {
        if (t == 0.0) {
            putBit(1);
            return;
        }
        putBit(0);
    }
    putBitDouble(t);
}

void dwgBufferW::putCmColor(DRW::Version v, duint16 colorIndex) {
    // R2000 emits raw BS index; AC1018+ adds RGB/method bytes which
    // are deferred to Phase 5.
    (void)v;
    putBitShort(colorIndex);
}

void dwgBufferW::putEnColor(DRW::Version v, duint16 colorIndex) {
    (void)v;
    putSBitShort(static_cast<dint16>(colorIndex));
}

// ---- CRC ------------------------------------------------------------------

duint16 dwgBufferW::crc16(duint16 seed, size_t start, size_t end) const {
    if (end > m_buf.size()) end = m_buf.size();
    duint16 dx = seed;
    for (size_t i = start; i < end; ++i) {
        duint8 al = static_cast<duint8>(m_buf[i] ^ static_cast<duint8>(dx & 0xFF));
        dx = static_cast<duint16>((dx >> 8) & 0xFF);
        dx = static_cast<duint16>(dx ^ crcTable[al]);
    }
    return dx;
}

// ---- in-place patching ---------------------------------------------------

void dwgBufferW::patchRawShort16(size_t byteOffset, duint16 v) {
    if (byteOffset + 2 > m_buf.size()) return;
    m_buf[byteOffset]     = static_cast<duint8>(v & 0xFF);
    m_buf[byteOffset + 1] = static_cast<duint8>((v >> 8) & 0xFF);
}

void dwgBufferW::patchRawLong32(size_t byteOffset, duint32 v) {
    if (byteOffset + 4 > m_buf.size()) return;
    m_buf[byteOffset]     = static_cast<duint8>(v & 0xFF);
    m_buf[byteOffset + 1] = static_cast<duint8>((v >> 8) & 0xFF);
    m_buf[byteOffset + 2] = static_cast<duint8>((v >> 16) & 0xFF);
    m_buf[byteOffset + 3] = static_cast<duint8>((v >> 24) & 0xFF);
}
