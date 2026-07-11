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

#ifndef DWGBUFFERW_H
#define DWGBUFFERW_H

#include <vector>
#include <string>
#include "../drw_base.h"

class DRW_Coord;
class DRW_TextCodec;

/// Writer-side companion to dwgBuffer. Accumulates bit-packed DWG
/// output into an in-memory byte vector and exposes the inverse of
/// every reader primitive in dwgBuffer (get* → put*).
///
/// Bit-packing convention matches the reader: bits within a byte are
/// filled MSB-first.  Multi-byte primitives (RS, RL, RD) are
/// little-endian on disk, except putBERawShort16 which is big-endian
/// for use with the HANDLES (object map) section CRC.
///
/// Lifecycle: instantiate, call put* to append, then read out the
/// accumulated bytes via data().  No seek-back; sections that need
/// post-hoc patching (e.g. file-header section-locator addresses)
/// must be patched in the final std::vector<std::uint8_t> by the caller.
class dwgBufferW {
public:
    /// @param decoder Codec used to convert UTF-8 input strings down
    ///        to the on-disk codepage (CP1252 by default for R2000).
    ///        May be null for tests or for callers that pre-encode.
    explicit dwgBufferW(DRW_TextCodec *decoder = nullptr);
    ~dwgBufferW() = default;

    // ---- accessors ------------------------------------------------------

    /// Accumulated bytes. After a partial byte (bitPos != 0) the trailing
    /// byte's unused low bits are zero — they will be overwritten by the
    /// next put* if more bits are appended.
    const std::vector<std::uint8_t>& data() const { return m_buf; }
    std::vector<std::uint8_t>& data() { return m_buf; }

    /// Current write position in bytes (size of accumulator).  When
    /// bitPos() != 0 the last byte is partially filled.
    size_t size() const { return m_buf.size(); }

    /// Bit position within the trailing byte, 0..7.  0 means the next
    /// put will start a fresh byte.
    std::uint8_t bitPos() const { return m_bitPos; }

    /// Reserve capacity in the underlying vector to avoid repeated
    /// reallocations during large emits.
    void reserve(size_t n) { m_buf.reserve(n); }

    /// Reset the buffer to empty, clearing both accumulated bytes and the
    /// partial-byte cursor.  Use before reusing a scratch buffer.
    void reset() { m_buf.clear(); m_bitPos = 0; }

    /// Round the cursor up to the next byte boundary by appending zero
    /// bits as needed.  No-op when already byte-aligned.
    void alignToByte();

    // ---- bit-level primitives (inverse of dwgBuffer get*) ---------------

    void putBit(std::uint8_t b);                  //B
    void putBoolBit(bool b);                //B as bool
    void put2Bits(std::uint8_t b);                //BB
    void put3Bits(std::uint8_t b);                //3B
    void putBitShort(std::uint16_t v);            //BS
    void putSBitShort(std::int16_t v);            //BS
    void putBitLong(std::int32_t v);              //BL
    void putBitLongLong(std::uint64_t v);         //BLL (R24)
    void putBitDouble(double d);            //BD
    void put3BitDouble(const DRW_Coord& c); //3BD

    // ---- raw fixed-width primitives -------------------------------------

    void putRawChar8(std::uint8_t v);             //RC
    void putRawShort16(std::uint16_t v);          //RS (little-endian)
    void putBERawShort16(std::uint16_t v);        //RS_BE (big-endian; HANDLES CRC)
    void putRawDouble(double d);            //RD
    void putRawLong32(std::uint32_t v);           //RL (little-endian)
    void putRawLong64(std::uint64_t v);           //RLL (little-endian)
    void put2RawDouble(const DRW_Coord& c); //2RD

    // ---- modular/variable-length encodings ------------------------------

    /// Unsigned modular char (UMC) — used for handle/offset deltas
    /// in the object map.  Emits 7-bit chunks LSB-first, top bit set
    /// on all chunks except the last (terminator).
    void putUModularChar(std::uint32_t v);

    /// Signed modular char (MC) — last chunk uses 6 bits + sign bit.
    void putModularChar(std::int32_t v);

    /// Modular short (MS) — 15-bit chunks; up to 2 chunks (reader
    /// only consumes 2). Unsigned only (matches reader behavior).
    void putModularShort(std::int32_t v);

    // ---- handles --------------------------------------------------------

    /// Emit a handle as RC(code<<4 | size) + size bytes of ref MSB-first.
    /// size is computed as the minimum byte width that fits ref
    /// (1..4 bytes, or 0 when ref == 0).
    void putHandle(const dwgHandle& h);

    /// Object type (OT).  R2010+ uses a 2-bit code + variable-width
    /// value; earlier versions use a plain BS.
    void putObjType(DRW::Version v, std::uint16_t oType);

    // ---- strings --------------------------------------------------------

    /// Variable text (TV/TU).  For version > AC1018 (R2007+) emits the
    /// R2007 TU form: BS char-count + UTF-16LE code units.  For earlier
    /// versions emits the 8-bit TV form: BS byte-count + CP8 bytes.
    void putVariableText(DRW::Version v, const std::string& utf8);

    /// 8-bit codepage text (T).  Converts utf8 → codepage via decoder
    /// if present, then emits BS(len) + bytes.
    void putCP8Text(const std::string& utf8);

    /// Unicode text (TU, R2007+).  Converts UTF-8 → UTF-16LE and emits
    /// BS(char-count) + char-count × 2 bytes.
    void putUCSText(const std::string& utf8);

    /// Bit count of data written so far (total bits, accounting for a
    /// partial trailing byte when bitPos() != 0).
    std::uint32_t bitCount() const;

    /// Append raw bytes at the current bit position.  Handles
    /// non-byte-aligned writes via per-byte bit shifting.
    void putBytes(const std::uint8_t* buf, size_t n);

    // ---- specialty primitives -------------------------------------------

    /// Extrusion (BE) — R2000 style.  Emits 1 bit; if extrusion is
    /// exactly (0,0,1), no further data.  Otherwise emits 3BD.
    void putExtrusion(const DRW_Coord& ext, bool b_R2000_style);

    /// Default-double (DD) — emits 2-bit code + truncation of d
    /// against default.  Simplified to always emit code 3 (full 8-byte
    /// RD) for correctness; readers accept this.  Optimal compression
    /// can be added later without breaking output.
    void putDefaultDouble(double defaultVal, double d);

    /// Bit-thickness (BT) — R2000-style. 1 bit shortcut when 0.0.
    void putThickness(double t, bool b_R2000_style);

    /// CMC color (R2000): emits BS color index.  Higher-version
    /// AcDbColor references are deferred to Phase 5.
    void putCmColor(DRW::Version v, std::uint16_t colorIndex);

    /// CMC color (R2004+) with optional 24-bit truecolor + color/book name
    /// (P4-08). For version < AC1018 this is identical to the index-only
    /// overload. For R2004+: when rgb24 >= 0 it emits the 0xC2 truecolor
    /// packing plus the name flags and (in strBuf, defaulting to this) the
    /// color name / book name strings; otherwise it falls back to the
    /// index path. Inverse of dwgBuffer::getCmColor.
    void putCmColor(DRW::Version v, std::uint16_t colorIndex, std::int32_t rgb24,
                    const UTF8STRING& colorName,
                    const UTF8STRING& bookName,
                    dwgBufferW* strBuf = nullptr);

    /// ENC color (R2000): emits BS color index.  Matches reader's
    /// getEnColor for AC1015.
    void putEnColor(DRW::Version v, std::uint16_t colorIndex);

    // ---- CRC ------------------------------------------------------------

    /// CRC16 with seed (typically 0xC0C1) over bytes [start, end) in
    /// the accumulator.  Caller is responsible for byte-aligning the
    /// range and for emitting the result via putRawShort16 (LE) for
    /// most sections, or putBERawShort16 (BE) for HANDLES page CRCs.
    std::uint16_t crc16(std::uint16_t seed, size_t start, size_t end) const;

    // ---- in-place patching ---------------------------------------------

    /// Overwrite 16-bit little-endian at byte offset (no bit shift).
    /// Used to back-patch the file-header section-locator records once
    /// final section addresses are known.
    void patchRawShort16(size_t byteOffset, std::uint16_t v);

    /// Overwrite 32-bit little-endian at byte offset (no bit shift).
    void patchRawLong32(size_t byteOffset, std::uint32_t v);

    /// Back-patch the RL objSize field at the given stream-bit offset.
    /// The RL straddles bytes [bitOffset/8 .. bitOffset/8+4], each shifted
    /// `bitOffset % 8` bits to the right relative to byte boundaries.
    /// Precondition: bitOffset % 8 == 2 (all BS forms that appear in our
    /// writer leave a remainder of 2: "01"+RC = 10 bits, "00"+RS = 18 bits,
    /// "10"/"11" = 2 bits — the byte+2-bit shift is always 2).
    /// Call this BEFORE alignToByte().
    void patchRawLong32AtBit(size_t bitOffset, std::uint32_t val);

private:
    /// Append a single byte assuming the cursor is byte-aligned.
    void appendAlignedByte(std::uint8_t b);

    std::vector<std::uint8_t> m_buf;
    std::uint8_t m_bitPos {0};
    DRW_TextCodec *m_decoder {nullptr};
};

#endif // DWGBUFFERW_H
