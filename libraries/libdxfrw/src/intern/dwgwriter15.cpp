/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD contributors                                  **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgwriter15.h"

#include <algorithm>
#include <cstring>

#include "../drw_base.h"
#include "../drw_entities.h"
#include "dwgutil.h"

namespace {

/// File-header CRC seed-XOR adjustment (matches reader at
/// dwgreader15.cpp:94-106). Indexed by `num_sections`.
duint16 seedXorForCount(duint8 numSections) {
    switch (numSections) {
        case 3: return 0xA598;
        case 4: return 0x8101;
        case 5: return 0x3CC4;
        case 6: return 0x8461;
        default: return 0;
    }
}

/// File-format `recno` values (NOT the libdxfrw `secEnum::DWGSection`
/// enum indices, which sort alphabetically and put HEADER at 2,
/// HANDLES at 14, etc.).  Reader maps recno → secEnum at
/// dwgreader15.cpp:60-83.
namespace recno {
    constexpr duint8 HEADER    = 0;
    constexpr duint8 CLASSES   = 1;
    constexpr duint8 HANDLES   = 2;
    constexpr duint8 UNKNOWN   = 3;
    constexpr duint8 TEMPLATE  = 4;
    constexpr duint8 AUXHEADER = 5;
}

/// Canonical reserved-handle table for R2000 control objects.  Matches
/// `HandleAllocator::seedReserved` and the Phase 3 sub-plan §"Reserved
/// (fixed) handles".  0x04 is intentionally unused.
namespace reservedHandle {
    constexpr duint32 BLOCK_CONTROL              = 0x01;
    constexpr duint32 LAYER_CONTROL              = 0x02;
    constexpr duint32 STYLE_CONTROL              = 0x03;
    constexpr duint32 LTYPE_CONTROL              = 0x05;
    constexpr duint32 VIEW_CONTROL               = 0x06;
    constexpr duint32 UCS_CONTROL                = 0x07;
    constexpr duint32 VPORT_CONTROL              = 0x08;
    constexpr duint32 APPID_CONTROL              = 0x09;
    constexpr duint32 DIMSTYLE_CONTROL           = 0x0A;
    constexpr duint32 VPORT_ENTITY_HEADER_CONTROL = 0x0B;
    // Reserved-block / linetype handles emitted in the HEADER section's
    // trailing 5-handle block (and as phantom entries in
    // BLOCK_CONTROL / LTYPE_CONTROL).  Phase 3e populates the records.
    constexpr duint32 LTYPE_BYBLOCK    = 0x0F;
    constexpr duint32 LTYPE_BYLAYER    = 0x10;
    constexpr duint32 LTYPE_CONTINUOUS = 0x11;
    constexpr duint32 BLOCK_MODEL_SPACE = 0x17;
    constexpr duint32 BLOCK_PAPER_SPACE = 0x18;
}

/// Object-type (oType) constants for the 10 R2000 control objects.
/// Same set as DRW_ObjControl::parseDwg's `oType` switch references.
namespace oType {
    constexpr duint16 BLOCK_CONTROL              = 0x30;
    constexpr duint16 LAYER_CONTROL              = 0x32;
    constexpr duint16 STYLE_CONTROL              = 0x34;
    constexpr duint16 LTYPE_CONTROL              = 0x38;
    constexpr duint16 VIEW_CONTROL               = 0x3C;
    constexpr duint16 UCS_CONTROL                = 0x3E;
    constexpr duint16 VPORT_CONTROL              = 0x40;
    constexpr duint16 APPID_CONTROL              = 0x42;
    constexpr duint16 DIMSTYLE_CONTROL           = 0x44;
    constexpr duint16 VPORT_ENTITY_HEADER_CONTROL = 0x46;
    // Record oTypes are the matching control oType + 1 except
    // BLOCK_RECORD which uses 0x31.  Verified against libreDWG dwg.spec.
    constexpr duint16 BLOCK_RECORD               = 0x31;
    constexpr duint16 LAYER                      = 0x33;
    constexpr duint16 STYLE                      = 0x35;
    constexpr duint16 LTYPE                      = 0x39;
    constexpr duint16 APPID                      = 0x43;
    constexpr duint16 DIMSTYLE                   = 0x45;
    constexpr duint16 VPORT                      = 0x41;
}

/// Build a hard-pointer (code 4) handle with the minimum-width ref.
dwgHandle makeHardPtr(duint32 ref) {
    dwgHandle h;
    h.code = (ref == 0) ? 0 : 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        duint32 t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build a null handle (code 0).
dwgHandle makeNullHandle() {
    dwgHandle h;
    h.code = 0;
    h.size = 0;
    h.ref  = 0;
    return h;
}

/// Build a soft-owner (code 3) handle — the form used for the XDic
/// slot in R2000 control objects.
dwgHandle makeSoftOwner(duint32 ref) {
    dwgHandle h;
    h.code = (ref == 0) ? 3 : 3;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        duint32 t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

} // namespace

bool dwgWriter15::writeFileHeaderStub() {
    // Bytes 0x00-0x05: version string "AC1015".
    for (int i = 0; i < 6; ++i)
        m_buf.putRawChar8(static_cast<duint8>(dwgVersionString::R2000[i]));

    // Bytes 0x06-0x0A: five zero bytes.
    for (int i = 0; i < 5; ++i)
        m_buf.putRawChar8(0);

    // Byte 0x0B: maintenance release version (R2000 = 0x0F per LibreDWG).
    m_buf.putRawChar8(0x0F);

    // Byte 0x0C: "zero_one_or_three" marker (1 for R13+ writes from scratch).
    m_buf.putRawChar8(0x01);

    // Bytes 0x0D-0x10: RL thumbnail/preview address (placeholder; back-patched
    // by finalize() once we know whether a PREVIEW section was emitted).
    m_buf.putRawLong32(0);

    // Byte 0x11: RC dwg_version (R2000 internal version = 0x17).
    m_buf.putRawChar8(0x17);

    // Byte 0x12: RC maint_version (0 by default; real files vary).
    m_buf.putRawChar8(0x00);

    // Bytes 0x13-0x14: RS codepage (30 = ANSI_1252).
    m_buf.putRawShort16(30);

    // Bytes 0x15-0x18: RL num_sections.
    m_buf.putRawLong32(m_numSections);

    // Section locator records start here. Record current offset so
    // finalize() can back-patch addresses + sizes.
    m_recordsOffset = static_cast<duint32>(m_buf.size());

    // Emit `m_numSections` stub records of (RC recno, RL address=0, RL size=0).
    // recno values are 0..(N-1) in canonical order.
    for (duint8 recno = 0; recno < m_numSections; ++recno) {
        m_buf.putRawChar8(recno);
        m_buf.putRawLong32(0); // address placeholder
        m_buf.putRawLong32(0); // size placeholder
    }

    // CRC16 over bytes [0 .. end-of-records) with seed=0, then XOR'd
    // by the per-count constant.  Stored value patched in finalize()
    // after section addresses are filled in.
    m_buf.putRawShort16(0); // placeholder

    // 16-byte FILE_HEADER_END sentinel.
    m_buf.putBytes(dwgSentinels::FILE_HEADER_END, 16);

    return true;
}

size_t dwgWriter15::beginSentinelSection(const duint8 (&beginSentinel)[16]) {
    // Caller must ensure cursor is byte-aligned before invoking.
    m_buf.alignToByte();
    m_buf.putBytes(beginSentinel, 16);
    // Reserve 4 bytes for RL section payload size — back-patched at end.
    size_t sizeOffset = m_buf.size();
    m_buf.putRawLong32(0);
    return sizeOffset;
}

void dwgWriter15::endSentinelSection(size_t sectionStart, size_t sizeOffset,
                                     const duint8 (&endSentinel)[16]) {
    // Bit-packed bodies (HEADER vars, CLASSES, etc.) typically leave the
    // cursor mid-byte.  Pad to the next byte boundary so the END sentinel
    // lands aligned and the recorded payload size is in whole bytes.
    m_buf.alignToByte();

    // Patch the payload size: bytes between (sizeOffset + 4) and current cursor.
    duint32 payloadSize =
        static_cast<duint32>(m_buf.size()) - static_cast<duint32>(sizeOffset + 4);
    m_buf.patchRawLong32(sizeOffset, payloadSize);

    // Emit the END sentinel.
    m_buf.putBytes(endSentinel, 16);

    // CRC16 LE over bytes [sectionStart .. cursor).  Seed 0xC0C1, then
    // appended as RS LE (no shift).
    duint16 crc = m_buf.crc16(0xC0C1, sectionStart, m_buf.size());
    m_buf.putRawShort16(crc);
}

bool dwgWriter15::writeDwgHeader() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HEADER] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // Phase 3d: populate the control-handle members on `m_header` so the
    // encoder's control-handle block (parseDwg:2162-2199) references the
    // canonical reserved handles.  Only override fields that the caller
    // left at zero — non-zero values are preserved on read-then-write.
    if (m_header != nullptr) {
        auto fillIfZero = [](duint32& slot, duint32 value) {
            if (slot == 0) slot = value;
        };
        fillIfZero(m_header->blockCtrl,       reservedHandle::BLOCK_CONTROL);
        fillIfZero(m_header->layerCtrl,       reservedHandle::LAYER_CONTROL);
        fillIfZero(m_header->styleCtrl,       reservedHandle::STYLE_CONTROL);
        fillIfZero(m_header->linetypeCtrl,    reservedHandle::LTYPE_CONTROL);
        fillIfZero(m_header->viewCtrl,        reservedHandle::VIEW_CONTROL);
        fillIfZero(m_header->ucsCtrl,         reservedHandle::UCS_CONTROL);
        fillIfZero(m_header->vportCtrl,       reservedHandle::VPORT_CONTROL);
        fillIfZero(m_header->appidCtrl,       reservedHandle::APPID_CONTROL);
        fillIfZero(m_header->dimstyleCtrl,    reservedHandle::DIMSTYLE_CONTROL);
        fillIfZero(m_header->vpEntHeaderCtrl, reservedHandle::VPORT_ENTITY_HEADER_CONTROL);
    }

    // Phase 3a: full 282-var bit-packed emission via DRW_Header::encodeDwg.
    // For R2000 the handle stream is inline with the data stream — pass
    // the same accumulator for both buf and hBbuf.
    if (m_header != nullptr) {
        m_header->encodeDwg(DRW::AC1015, &m_buf, &m_buf);
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeDwgClasses() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);
    // Zero classes — but emit one dummy padding byte.  The reader at
    // [dwgreader15.cpp:155](../intern/dwgreader15.cpp) does a `size--`
    // before its `while (size > buff.getPosition())` loop, which
    // underflows to 0xFFFFFFFF when size==0 and spins indefinitely.
    // With size==1 the underflow is avoided: `size--` yields 0 and the
    // loop terminates cleanly without reading any class.
    m_buf.putRawChar8(0);

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::CLASSES_END);

    m_sectionSizes[recno::CLASSES] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

void dwgWriter15::emitControlObject(
    duint16 typeCode, duint32 handle, duint32 numEntries,
    std::initializer_list<duint32> childHandles)
{
    dwgBufferW& body = beginObject(handle);

    // Common preamble (mirror of DRW_TableEntry::parseDwg for R2000):
    //   BS  oType
    //   RL  objSize  (in bits, stored but unused by reader for R2000;
    //                 emit 0 as a placeholder — see note below)
    //   H   handle   (the object's own handle, hard-owner code 4)
    //   BS  extDataSize = 0
    //   BL  numReactors = 0
    //   (no xDictFlag bit on R2000 — DRW_TableEntry::parseDwg only
    //    reads it for version > AC1015)
    body.putBitShort(typeCode);
    // objSize precise value would require a two-pass encode (count the
    // bits after the RL placeholder, then back-patch).  For R2000 the
    // reader stores the value but never uses it; ODA validators do.
    // Phase 3d ships with 0 and revisits if ODA cross-check flags it.
    body.putRawLong32(0);
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors

    // DRW_ObjControl body (mirror of dwgreader.cpp:1633-1680 for R2000):
    //   BL  numEntries
    //   [RC unkData byte if DIMSTYLE (oType 0x44) on R2000+]
    //   H   null handle  (terminator slot)
    //   H   XDic handle  (xDictFlag defaults to 0 → reader expects one)
    //   <numEntries + 2 if BLOCK_CONTROL/LTYPE_CONTROL> offset handles
    //                    (caller supplies them via childHandles —
    //                     count must match the post-+2 expectation)
    body.putBitLong(static_cast<dint32>(numEntries));
    if (typeCode == oType::DIMSTYLE_CONTROL) {
        body.putRawChar8(0);
    }
    body.putHandle(makeNullHandle());
    body.putHandle(makeSoftOwner(0));  // XDic null
    for (duint32 child : childHandles)
        body.putHandle(makeHardPtr(child));

    finishObject();
}

void dwgWriter15::emitBlockEntity(duint32 handle, const std::string& name,
                                  bool isEnd) {
    DRW_Block bk;
    bk.handle = handle;
    bk.layerH.ref = 0x12;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.setIsEnd(isEnd);
    dwgBufferW& body = beginObject(handle);
    bk.encodeDwg(DRW::AC1015, &body, /*bs=*/0);
    finishObject();
}

void dwgWriter15::emitBlockRecord(duint32 handle, const std::string& name,
                                  duint32 blockHandle,
                                  duint32 endBlockHandle) {
    dwgBufferW& body = beginObject(handle);

    // Common table-entry preamble (R2000, no xDictFlag).
    body.putBitShort(oType::BLOCK_RECORD);
    body.putRawLong32(0);                  // objSize stub
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);                   // extDataSize
    body.putBitLong(0);                    // numReactors

    // R2000 Block_Record body.  Mirror of DRW_Block_Record::parseDwg
    // at drw_objects.cpp:709-840 — collapsed for our empty/canonical
    // *Model_Space / *Paper_Space records.
    body.putVariableText(DRW::AC1015, name);
    body.putBit(0);                        // flags bit 6 (xref-ref)
    body.putBitShort(0);                   // xrefindex BS (R2004-)
    body.putBit(0);                        // flags bit 4 (xref dep)
    body.putBit(0);                        // anon (*U bit)
    body.putBit(0);                        // attdefs
    body.putBit(0);                        // xref
    body.putBit(0);                        // overlaid xref
    body.putBit(0);                        // R2000+ loaded-xref
    // (R2004+ objectCount BL omitted for R2000.)
    DRW_Coord origin;
    origin.x = origin.y = origin.z = 0.0;
    body.put3BitDouble(origin);            // basePoint 3BD
    body.putVariableText(DRW::AC1015, std::string{});  // xrefPath empty
    // R2000+ insertCount loop: emit terminating 0 byte.
    body.putRawChar8(0);
    body.putVariableText(DRW::AC1015, std::string{});  // bkdesc empty
    body.putBitLong(0);                    // prevData BL = 0 (no bytes)
    // (R2007+ insUnits/canExplode/bkScaling omitted.)

    // Handle stream.
    body.putHandle(makeHardPtr(reservedHandle::BLOCK_CONTROL));  // parent
    body.putHandle(makeSoftOwner(0));                            // XDic null
    body.putHandle(makeNullHandle());                            // NullH
    body.putHandle(makeHardPtr(blockHandle));                    // block ref
    // R2000- non-xref: emit firstEH + lastEH (both null = empty).
    body.putHandle(makeNullHandle());
    body.putHandle(makeNullHandle());
    body.putHandle(makeHardPtr(endBlockHandle));                 // endBlock
    // R2000+ insertCount=0 → no inserts handles in the loop body.
    body.putHandle(makeSoftOwner(0));                            // layoutH null

    finishObject();
}

void dwgWriter15::emitTableRecord(duint16 typeCode, duint32 handle,
                                  const std::string& name) {
    dwgBufferW& body = beginObject(handle);

    // Common preamble — same as control objects.
    body.putBitShort(typeCode);
    body.putRawLong32(0);  // objSize placeholder (informational on R2000)
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);   // extDataSize
    body.putBitLong(0);    // numReactors
    // (No xDictFlag bit for R2000)

    // Record name — this is the first field after the preamble that
    // every per-record parseDwg reads.  Reader uses
    // `sBuf->getVariableText(version, false)` which for R2000 reads a
    // BS-length + bytes (CP8) pair.  Stopping here is enough: the
    // reader's subsequent reads return zeros/empty when it runs off
    // the end of the body and parseDwg returns false with a per-record
    // warning — the record is still stored in the map with `name` set.
    body.putVariableText(DRW::AC1015, name);

    finishObject();
}

bool dwgWriter15::encodeEntity(DRW_Entity *ent) {
    if (ent == nullptr) return false;
    // Honor caller-set handles (round-trip preservation); allocate a
    // fresh one only when ent->handle is zero.  Either way, reserve
    // the handle in the allocator so a later next() can't return it
    // for a different entity.
    duint32 handle = ent->handle;
    if (handle == 0) {
        handle = m_handles.next();
        ent->handle = handle;
    } else {
        m_handles.reserve(handle);
    }
    // Default layer "0" (handle 0x12) when caller didn't set one.  Phase
    // 4d will do real layer-name → handle resolution via the layermap.
    if (ent->layerH.ref == 0) {
        ent->layerH.ref = 0x12;
    }
    dwgBufferW& body = beginObject(handle);
    bool ok = ent->encodeDwg(DRW::AC1015, &body, /*bs=*/0);
    if (!ok) return false;
    finishObject();
    return true;
}

bool dwgWriter15::writeDwgObjects() {
    // Object stream — the unsentinel'd byte region between CLASSES end
    // and HANDLES start.  Phase 3d: emit 10 empty control objects at
    // the canonical reserved handles.  BLOCK_CONTROL and LTYPE_CONTROL
    // each carry 2 phantom child handles because the reader applies
    // `numEntries += 2` for those types (see Risk 4f).

    // BLOCK_CONTROL: numEntries=0, +2 phantoms (MODEL_SPACE+PAPER_SPACE)
    //   resolve in the reader's child loop.  We don't emit the Block_Record
    //   records yet (Phase 4), so the lookups are silent warnings.
    emitControlObject(oType::BLOCK_CONTROL, reservedHandle::BLOCK_CONTROL,
                      0,
                      {reservedHandle::BLOCK_MODEL_SPACE,
                       reservedHandle::BLOCK_PAPER_SPACE});

    // LAYER_CONTROL: 1 real entry (layer "0").
    emitControlObject(oType::LAYER_CONTROL, reservedHandle::LAYER_CONTROL,
                      1, {0x12});

    // STYLE_CONTROL: 1 real entry (textstyle "STANDARD").
    emitControlObject(oType::STYLE_CONTROL, reservedHandle::STYLE_CONTROL,
                      1, {0x13});

    // LTYPE_CONTROL: 1 real entry (CONTINUOUS) + 2 phantoms
    //   (BYBLOCK, BYLAYER) → reader walks 3 offset handles.
    emitControlObject(oType::LTYPE_CONTROL, reservedHandle::LTYPE_CONTROL,
                      1,
                      {reservedHandle::LTYPE_BYBLOCK,
                       reservedHandle::LTYPE_BYLAYER,
                       reservedHandle::LTYPE_CONTINUOUS});

    // VIEW_CONTROL: no entries.
    emitControlObject(oType::VIEW_CONTROL, reservedHandle::VIEW_CONTROL,
                      0, {});

    // UCS_CONTROL: no entries.
    emitControlObject(oType::UCS_CONTROL, reservedHandle::UCS_CONTROL,
                      0, {});

    // VPORT_CONTROL: 1 real entry (*ACTIVE).
    emitControlObject(oType::VPORT_CONTROL, reservedHandle::VPORT_CONTROL,
                      1, {0x16});

    // APPID_CONTROL: 1 real entry (ACAD).
    emitControlObject(oType::APPID_CONTROL, reservedHandle::APPID_CONTROL,
                      1, {0x14});

    // DIMSTYLE_CONTROL: 1 real entry (STANDARD).
    emitControlObject(oType::DIMSTYLE_CONTROL,
                      reservedHandle::DIMSTYLE_CONTROL, 1, {0x15});

    // VPORT_ENTITY_HEADER_CONTROL: no entries (R2000-only slot).
    emitControlObject(oType::VPORT_ENTITY_HEADER_CONTROL,
                      reservedHandle::VPORT_ENTITY_HEADER_CONTROL,
                      0, {});

    // Phase 3e: minimum table records at the reserved handles.  Emit
    // 8 of the 10 — Block_Records (*Model_Space / *Paper_Space) are
    // deferred to Phase 4 because the reader's `readDwgBlocks` will
    // attempt to look up `Block_Record::block` in ObjectMap, and that
    // handle points at a DRW_Block entity we don't emit until entity
    // work lands.  Skipping the two Block_Record records keeps the
    // BLOCK_CONTROL phantom-handle lookups silent (as they were in
    // Phase 3d) without triggering BAD_READ_BLOCKS.  The other 8
    // records resolve the LTYPE_CONTROL phantom handles and provide
    // the standard layer / textstyle / appid / dimstyle / vport that
    // AutoCAD expects to find in every R2000 document.
    emitTableRecord(oType::LTYPE,    reservedHandle::LTYPE_BYBLOCK,    "BYBLOCK");
    emitTableRecord(oType::LTYPE,    reservedHandle::LTYPE_BYLAYER,    "BYLAYER");
    emitTableRecord(oType::LTYPE,    reservedHandle::LTYPE_CONTINUOUS, "CONTINUOUS");
    emitTableRecord(oType::LAYER,    0x12, "0");
    emitTableRecord(oType::STYLE,    0x13, "STANDARD");
    emitTableRecord(oType::APPID,    0x14, "ACAD");
    emitTableRecord(oType::DIMSTYLE, 0x15, "STANDARD");
    emitTableRecord(oType::VPORT,    0x16, "*ACTIVE");

    // Phase 4d: emit Block_Record records for *Model_Space and
    // *Paper_Space + the BLOCK + ENDBLK entities they own.  Handles
    // 0x1B-0x1E come from the master plan's reserved-but-unused range.
    constexpr duint32 BLK_MODEL_START  = 0x1B;
    constexpr duint32 BLK_MODEL_END    = 0x1C;
    constexpr duint32 BLK_PAPER_START  = 0x1D;
    constexpr duint32 BLK_PAPER_END    = 0x1E;

    emitBlockEntity(BLK_MODEL_START, "*Model_Space", /*isEnd=*/false);
    emitBlockEntity(BLK_MODEL_END,   std::string{},   /*isEnd=*/true);
    emitBlockEntity(BLK_PAPER_START, "*Paper_Space", /*isEnd=*/false);
    emitBlockEntity(BLK_PAPER_END,   std::string{},   /*isEnd=*/true);

    emitBlockRecord(reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space",
                    BLK_MODEL_START, BLK_MODEL_END);
    emitBlockRecord(reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space",
                    BLK_PAPER_START, BLK_PAPER_END);

    return true;
}

dwgBufferW& dwgWriter15::beginObject(duint32 handle) {
    m_currentHandle = handle;
    m_objectBody.data().clear();
    return m_objectBody;
}

void dwgWriter15::finishObject() {
    // Byte-align the body — the trailing CRC must start aligned and
    // the per-object frame size must be in whole bytes.
    m_objectBody.alignToByte();
    duint32 bodyBytes = static_cast<duint32>(m_objectBody.size());

    duint32 frameStart = static_cast<duint32>(m_buf.size());

    // MS objectSize = byte count of body (no CRC).  Per the master plan
    // interpretation (b): CRC follows immediately after the slurped body
    // and is not included in `size`.
    m_buf.putModularShort(static_cast<dint32>(bodyBytes));
    size_t bodyStartOffset = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), bodyBytes);

    // CRC16 LE seed=0xC0C1 over the body bytes (no MS prefix).  The
    // libdxfrw reader does not validate object CRCs (it just consumes
    // `size` bytes via getBytes), so this is for ODA/AutoCAD validators.
    duint16 crc = m_buf.crc16(0xC0C1, bodyStartOffset,
                              bodyStartOffset + bodyBytes);
    m_buf.putRawShort16(crc);

    m_objectMap.emplace_back(m_currentHandle, frameStart);
    m_currentHandle = 0;
}

bool dwgWriter15::writeDwgHandles() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HANDLES] = static_cast<duint32>(sectionStart);

    if (m_objectMap.empty()) {
        // Empty object map: single terminator page of `RS_BE(2) + CRC16_BE`.
        // size_BE = 2 means "this page contains 2 bytes including the size word".
        duint16 pageSize = 2;
        m_buf.putBERawShort16(pageSize);
        duint16 crc = m_buf.crc16(0xC0C1, sectionStart, m_buf.size());
        m_buf.putBERawShort16(crc);

        m_sectionSizes[recno::HANDLES] =
            static_cast<duint32>(m_buf.size() - sectionStart);
        return true;
    }

    // Sort by handle for monotonic UMC deltas.  Per the master plan
    // (LibreDWG insertion-sort at encode.c:2394-2406): in practice the
    // map is already monotonic because objects are emitted in handle
    // order, but the sort defends against future out-of-order emit.
    std::sort(m_objectMap.begin(), m_objectMap.end());

    // Page-emit walk.  Each page is bounded at ≤2030 bytes of (size
    // field + entries) to leave 2 bytes for the trailing CRC under the
    // 2032-byte ODA spec limit.
    constexpr size_t kMaxPagePayloadBytes = 2030;

    size_t entryStart = 0;
    while (entryStart < m_objectMap.size()) {
        size_t pageStart = m_buf.size();
        // Reserve the size field; we know its width is 2 bytes (RS BE)
        // so we know the payload budget is kMaxPagePayloadBytes - 2.
        m_buf.putBERawShort16(0);  // patched after entries

        duint32 prevHandle = 0;
        duint32 prevOffset = 0;
        size_t entryEnd = entryStart;
        while (entryEnd < m_objectMap.size()) {
            duint32 h = m_objectMap[entryEnd].first;
            duint32 off = m_objectMap[entryEnd].second;

            // Cheap upper-bound check: UMC + MC are each ≤5 bytes; if
            // adding 10 bytes would overflow the page, close it now.
            // Tightens if the actual encoded widths are smaller, but
            // never overshoots.
            if ((m_buf.size() - pageStart) + 10 > kMaxPagePayloadBytes)
                break;

            m_buf.putUModularChar(h - prevHandle);
            m_buf.putModularChar(static_cast<dint32>(off - prevOffset));
            prevHandle = h;
            prevOffset = off;
            ++entryEnd;
        }

        // Patch the page size (BE) — overwrite the placeholder we wrote
        // at pageStart at the top of the loop.  Note: patchRawShort16
        // emits LE, so we write the BE bytes directly here.
        duint16 pageSize = static_cast<duint16>(m_buf.size() - pageStart);
        m_buf.data()[pageStart]     = static_cast<duint8>((pageSize >> 8) & 0xFF);
        m_buf.data()[pageStart + 1] = static_cast<duint8>(pageSize & 0xFF);

        // CRC16 BE over (size + entries).
        duint16 crc = m_buf.crc16(0xC0C1, pageStart, m_buf.size());
        m_buf.putBERawShort16(crc);

        entryStart = entryEnd;
    }

    // Terminator page (2-byte size word + CRC of just the size).
    size_t termStart = m_buf.size();
    duint16 termSize = 2;
    m_buf.putBERawShort16(termSize);
    duint16 termCrc = m_buf.crc16(0xC0C1, termStart, m_buf.size());
    m_buf.putBERawShort16(termCrc);

    m_sectionSizes[recno::HANDLES] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeSecondHeader() {
    // v1: skip the 2NDHEADER section entirely.  AutoCAD treats this
    // section as optional metadata recovery; libdxfrw's reader does
    // not emit a warning when it is absent (sections[2NDHEADER].Id<0
    // path is silently tolerated).  Phase 3 may add a minimal stub
    // here once we measure how third-party readers behave.
    return true;
}

bool dwgWriter15::finalize() {
    // Patch each section record in the file-header locator table.
    // Each record is 9 bytes: RC recno @ +0, RL address @ +1, RL size @ +5.
    for (duint8 recno = 0; recno < m_numSections; ++recno) {
        auto offIt  = m_sectionOffsets.find(recno);
        auto sizeIt = m_sectionSizes.find(recno);
        duint32 address = (offIt  != m_sectionOffsets.end()) ? offIt->second  : 0;
        duint32 size    = (sizeIt != m_sectionSizes.end())   ? sizeIt->second : 0;
        size_t base = m_recordsOffset + static_cast<size_t>(recno) * 9;
        m_buf.patchRawLong32(base + 1, address);
        m_buf.patchRawLong32(base + 5, size);
    }

    // Recompute file-header CRC over bytes [0 .. m_recordsOffset + 9N)
    // with seed=0, then XOR'd by the per-count constant, then stored at
    // (m_recordsOffset + 9N).
    size_t crcOffset = m_recordsOffset + static_cast<size_t>(m_numSections) * 9;
    duint16 crc = m_buf.crc16(0, 0, crcOffset);
    crc = static_cast<duint16>(crc ^ seedXorForCount(m_numSections));
    m_buf.patchRawShort16(crcOffset, crc);

    // Flush the accumulator to disk in one write().
    if (m_stream == nullptr || !m_stream->good()) return false;
    const std::vector<duint8>& bytes = m_buf.data();
    m_stream->write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    return m_stream->good();
}
