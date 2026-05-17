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
/// must be patched in the final std::vector<duint8> by the caller.
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
    const std::vector<duint8>& data() const { return m_buf; }
    std::vector<duint8>& data() { return m_buf; }

    /// Current write position in bytes (size of accumulator).  When
    /// bitPos() != 0 the last byte is partially filled.
    size_t size() const { return m_buf.size(); }

    /// Bit position within the trailing byte, 0..7.  0 means the next
    /// put will start a fresh byte.
    duint8 bitPos() const { return m_bitPos; }

    /// Reserve capacity in the underlying vector to avoid repeated
    /// reallocations during large emits.
    void reserve(size_t n) { m_buf.reserve(n); }

    /// Round the cursor up to the next byte boundary by appending zero
    /// bits as needed.  No-op when already byte-aligned.
    void alignToByte();

    // ---- bit-level primitives (inverse of dwgBuffer get*) ---------------

    void putBit(duint8 b);                  //B
    void putBoolBit(bool b);                //B as bool
    void put2Bits(duint8 b);                //BB
    void put3Bits(duint8 b);                //3B
    void putBitShort(duint16 v);            //BS
    void putSBitShort(dint16 v);            //BS
    void putBitLong(dint32 v);              //BL
    void putBitLongLong(duint64 v);         //BLL (R24)
    void putBitDouble(double d);            //BD
    void put3BitDouble(const DRW_Coord& c); //3BD

    // ---- raw fixed-width primitives -------------------------------------

    void putRawChar8(duint8 v);             //RC
    void putRawShort16(duint16 v);          //RS (little-endian)
    void putBERawShort16(duint16 v);        //RS_BE (big-endian; HANDLES CRC)
    void putRawDouble(double d);            //RD
    void putRawLong32(duint32 v);           //RL (little-endian)
    void putRawLong64(duint64 v);           //RLL (little-endian)
    void put2RawDouble(const DRW_Coord& c); //2RD

    // ---- modular/variable-length encodings ------------------------------

    /// Unsigned modular char (UMC) — used for handle/offset deltas
    /// in the object map.  Emits 7-bit chunks LSB-first, top bit set
    /// on all chunks except the last (terminator).
    void putUModularChar(duint32 v);

    /// Signed modular char (MC) — last chunk uses 6 bits + sign bit.
    void putModularChar(dint32 v);

    /// Modular short (MS) — 15-bit chunks; up to 2 chunks (reader
    /// only consumes 2). Unsigned only (matches reader behavior).
    void putModularShort(dint32 v);

    // ---- handles --------------------------------------------------------

    /// Emit a handle as RC(code<<4 | size) + size bytes of ref MSB-first.
    /// size is computed as the minimum byte width that fits ref
    /// (1..4 bytes, or 0 when ref == 0).
    void putHandle(const dwgHandle& h);

    // ---- strings --------------------------------------------------------

    /// Variable text (TV).  R2000 emits as BS(byte-length) + bytes in
    /// the active codepage (typically CP1252).  Reader: getVariableText
    /// branches on version; this writer is R2000-only and always emits
    /// the 8-bit form.
    void putVariableText(DRW::Version v, const std::string& utf8);

    /// 8-bit codepage text (T).  Converts utf8 → codepage via decoder
    /// if present, then emits BS(len) + bytes.
    void putCP8Text(const std::string& utf8);

    /// Append raw bytes at the current bit position.  Handles
    /// non-byte-aligned writes via per-byte bit shifting.
    void putBytes(const duint8* buf, size_t n);

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
    void putCmColor(DRW::Version v, duint16 colorIndex);

    /// ENC color (R2000): emits BS color index.  Matches reader's
    /// getEnColor for AC1015.
    void putEnColor(DRW::Version v, duint16 colorIndex);

    // ---- CRC ------------------------------------------------------------

    /// CRC16 with seed (typically 0xC0C1) over bytes [start, end) in
    /// the accumulator.  Caller is responsible for byte-aligning the
    /// range and for emitting the result via putRawShort16 (LE) for
    /// most sections, or putBERawShort16 (BE) for HANDLES page CRCs.
    duint16 crc16(duint16 seed, size_t start, size_t end) const;

    // ---- in-place patching ---------------------------------------------

    /// Overwrite 16-bit little-endian at byte offset (no bit shift).
    /// Used to back-patch the file-header section-locator records once
    /// final section addresses are known.
    void patchRawShort16(size_t byteOffset, duint16 v);

    /// Overwrite 32-bit little-endian at byte offset (no bit shift).
    void patchRawLong32(size_t byteOffset, duint32 v);

private:
    /// Append a single byte assuming the cursor is byte-aligned.
    void appendAlignedByte(duint8 b);

    std::vector<duint8> m_buf;
    duint8 m_bitPos {0};
    DRW_TextCodec *m_decoder {nullptr};
};

#endif // DWGBUFFERW_H
