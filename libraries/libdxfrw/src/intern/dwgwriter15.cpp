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

#include "dwgwriter15.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

#include "../drw_base.h"
#include "../drw_entities.h"
#include "../drw_objects.h"
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
    constexpr duint16 VIEW                       = 0x3D;
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

/// Build an own-handle entry (code 0) — the form used for an object's own handle.
dwgHandle makeOwnHandle(duint32 ref) {
    dwgHandle h;
    h.code = 0;
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

void putAuxRL(std::vector<duint8>& v, duint32 x) {
    v.push_back(static_cast<duint8>(x));
    v.push_back(static_cast<duint8>(x >> 8));
    v.push_back(static_cast<duint8>(x >> 16));
    v.push_back(static_cast<duint8>(x >> 24));
}

void putAuxRS(std::vector<duint8>& v, duint16 x) {
    v.push_back(static_cast<duint8>(x));
    v.push_back(static_cast<duint8>(x >> 8));
}

double headerDoubleVar(const DRW_Header *header, const std::string& key) {
    if (header == nullptr)
        return 0.0;
    auto it = header->vars.find(key);
    if (it == header->vars.end() || it->second == nullptr
        || it->second->type() != DRW_Variant::DOUBLE) {
        return 0.0;
    }
    return it->second->d_val();
}

void splitAuxDate(double stored, dint32& day, dint32& msec) {
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

void putAuxDate(std::vector<duint8>& v, double stored) {
    dint32 day = 0;
    dint32 msec = 0;
    splitAuxDate(stored, day, msec);
    putAuxRL(v, static_cast<duint32>(day));
    putAuxRL(v, static_cast<duint32>(msec));
}

std::vector<duint8> buildR2000AuxHeaderContent(const DRW_Header *header) {
    constexpr duint16 rawVersion = 23;  // AC1015
    std::vector<duint8> v;
    v.reserve(111);
    v.push_back(0xff);
    v.push_back(0x77);
    v.push_back(0x01);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);                     // maintenance version
    putAuxRL(v, 1);                     // number of saves
    putAuxRL(v, static_cast<duint32>(-1));
    putAuxRS(v, 1);                     // saves part 1
    putAuxRS(v, 0);                     // saves part 2
    putAuxRL(v, 0);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);
    putAuxRS(v, 0x0005);
    putAuxRS(v, 0x0893);
    putAuxRS(v, 0x0005);
    putAuxRS(v, 0x0893);
    putAuxRS(v, 0);
    putAuxRS(v, 1);
    for (int i = 0; i < 5; ++i)
        putAuxRL(v, 0);

    putAuxDate(v, headerDoubleVar(header, "TDCREATE"));
    putAuxDate(v, headerDoubleVar(header, "TDUPDATE"));

    const duint32 handSeed = header ? header->getHandSeed() : 0;
    putAuxRL(v, handSeed <= 0x7fffffffu ? handSeed : static_cast<duint32>(-1));
    putAuxRL(v, 0);                     // educational plot stamp
    putAuxRS(v, 0);
    putAuxRS(v, 1);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 1);                     // number of saves
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    putAuxRL(v, 0);
    return v;
}

/// Build a soft-owner (code 3) handle — the form used for the XDic
/// slot in R2000 control objects.
dwgHandle makeSoftOwner(duint32 ref) {
    dwgHandle h;
    h.code = 3;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        duint32 t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Normalize string to upper-case for writing-context map lookups.
static std::string toUpperCase(const std::string& s) {
    std::string r(s);
    for (auto& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
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

    // CRC covers RL-size + data only — spec §9: "covers the stepper and the
    // data"; sectionStart+16 skips the 16-byte beginning sentinel.
    duint16 crc = m_buf.crc16(0xC0C1, sectionStart + 16, m_buf.size());
    m_buf.putRawShort16(crc);        // CRC BEFORE end sentinel
    m_buf.putBytes(endSentinel, 16); // end sentinel AFTER CRC
}

void dwgWriter15::initHeaderControlHandles() {
    if (m_header == nullptr) return;
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

bool dwgWriter15::writeDwgHeader() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HEADER] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // Phase 3d: populate the control-handle members on m_header so the
    // encoder's control-handle block (parseDwg:2162-2199) references the
    // canonical reserved handles.
    initHeaderControlHandles();

    // Phase 3a: full 282-var bit-packed emission via DRW_Header::encodeDwg.
    // For R2000 the handle stream is inline with the data stream — pass
    // the same accumulator for both buf and hBbuf.
    if (m_header != nullptr) {
        m_header->encodeDwg(m_version, &m_buf, &m_buf);
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<duint32>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeDwgClasses() {
    if (hasDwgClassConflict())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<duint32>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // R2000 CLASSES is just the packed class entries between sentinels.
    // Field layout mirrors DRW_Class::parseDwg and ODA class section records.
    for (const auto& definition : sortedDwgClassDefinitions())
        writeDwgClassDefinition(definition, &m_buf, nullptr);

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

    // Common preamble (mirrors DRW_TableEntry::parseDwg):
    //   OT  oType  — R2010+ uses 2-bit code + RC/RS; R2000/R2004 uses BS
    //   RL  objSize — R2000/R2004 only; R2010+ computes it from buf.size()-bs
    //   H   handle  (the object's own handle)
    //   BS  extDataSize = 0
    //   BL  numReactors = 0
    //   B   xDictFlag = 0  (R2004+)
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize placeholder (back-patched by finishObject)
    }
    body.putHandle(makeOwnHandle(handle));
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);   // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);   // Have binary data (AC1027+)
    }

    // DRW_ObjControl body (mirrors dwgreader.cpp DRW_ObjControl::parseDwg):
    //   BL  numEntries
    //   RC  unkData (DIMSTYLE only, R2000+)
    //   B   stringBit = 0  (R2007+: control objects never have strings)
    //   H   null handle
    //   H   XDic handle  (xDictFlag=0 → reader reads one)
    //   H   child offset handles × numEntries (+2 for BLOCK/LTYPE controls)
    body.putBitLong(static_cast<dint32>(numEntries));
    if (typeCode == oType::DIMSTYLE_CONTROL) {
        body.putRawChar8(0);
    }
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0: no strings in control objects (R2007+)
    }
    body.putHandle(makeNullHandle());
    body.putHandle(makeSoftOwner(0));  // XDic null
    for (duint32 child : childHandles)
        body.putHandle(makeHardPtr(child));

    finishObject();
}

void dwgWriter15::emitControlObject(
    duint16 typeCode, duint32 handle, duint32 numEntries,
    const std::vector<duint32>& childHandles)
{
    dwgBufferW& body = beginObject(handle);
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024)
        body.putRawLong32(0);
    body.putHandle(makeOwnHandle(handle));
    body.putBitShort(0);   // extDataSize
    body.putBitLong(0);    // numReactors
    if (m_version > DRW::AC1015)
        body.putBit(0);    // xDictFlag
    if (m_version > DRW::AC1024)
        body.putBit(0);    // Have binary data (AC1027+)
    body.putBitLong(static_cast<dint32>(numEntries));
    if (typeCode == oType::DIMSTYLE_CONTROL)
        body.putRawChar8(0);
    if (m_version > DRW::AC1018)
        body.putBit(0);
    body.putHandle(makeNullHandle());
    body.putHandle(makeSoftOwner(0));
    for (duint32 child : childHandles)
        body.putHandle(makeHardPtr(child));
    finishObject();
}

duint32 dwgWriter15::defineBlock(const std::string& name,
                                 const DRW_Coord& basePoint,
                                 int insUnits) {
    // Allocate a fresh handle trio.
    duint32 blockRecH  = m_handles.next();
    duint32 blockH     = m_handles.next();
    duint32 endBlockH  = m_handles.next();

    // Block entity for the start of the block body.
    DRW_Block bk;
    bk.handle = blockH;
    bk.layerH.ref = 0x12;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.setIsEnd(false);
    {
        dwgBufferW& body = beginObject(blockH);
        bk.encodeDwg(m_version, &body, /*bs=*/0);
        finishObject();
    }

    // ENDBLK entity.
    DRW_Block endBlk;
    endBlk.handle = endBlockH;
    endBlk.layerH.ref = 0x12;
    endBlk.color = 256;
    endBlk.setIsEnd(true);
    {
        dwgBufferW& body = beginObject(endBlockH);
        endBlk.encodeDwg(m_version, &body, /*bs=*/0);
        finishObject();
    }

    // Block_Record (with the caller's basePoint).
    {
        dwgBufferW& body = beginObject(blockRecH);
        body.putObjType(m_version, oType::BLOCK_RECORD);
        if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
            body.putRawLong32(0);              // objSize stub (R2000/R2004 only)
        }
        body.putHandle(makeHardPtr(blockRecH));
        body.putBitShort(0);                   // extDataSize
        body.putBitLong(0);                    // numReactors
        if (m_version > DRW::AC1015) {
            body.putBit(0);                    // xDictFlag=0 (R2004+)
        }
        if (m_version > DRW::AC1024) {
            body.putBit(0);                    // Have binary data (AC1027+)
        }
        body.putVariableText(m_version, name);
        body.putBit(0);                        // flags bit 6 (xref-ref)
        body.putBitShort(0);                   // xrefindex BS (R2004-)
        body.putBit(0);                        // xdep
        body.putBit(0);                        // anon
        body.putBit(0);                        // attdefs
        body.putBit(0);                        // xref
        body.putBit(0);                        // overlaid
        body.putBit(0);                        // R2000+ loaded-xref
        if (m_version > DRW::AC1015) {
            body.putBitLong(0);                // R2004+: objectCount = 0 (empty block)
        }
        body.put3BitDouble(basePoint);
        body.putVariableText(m_version, std::string{});  // xrefPath
        body.putRawChar8(0);                   // insertCount terminator
        body.putVariableText(m_version, std::string{});  // bkdesc
        body.putBitLong(0);                    // prevData BL
        if (m_version > DRW::AC1018) {
            body.putBitShort(static_cast<duint16>(insUnits));
            body.putBit(0);                    // canExplode B (R2007+)
            body.putRawChar8(0);               // bkScaling RC (R2007+)
        }
        body.putHandle(makeHardPtr(reservedHandle::BLOCK_CONTROL));
        body.putHandle(makeSoftOwner(0));      // XDic null
        body.putHandle(makeNullHandle());      // NullH
        body.putHandle(makeHardPtr(blockH));   // block
        if (m_version <= DRW::AC1015) {
            body.putHandle(makeNullHandle());  // firstEH (R2000- chain)
            body.putHandle(makeNullHandle());  // lastEH
        }
        // R2004+: objectCount=0, no entity handles.
        body.putHandle(makeHardPtr(endBlockH));// endBlock
        body.putHandle(makeSoftOwner(0));      // layoutH null
        finishObject();
    }

    m_userBlockRecordHandles.push_back(blockRecH);
    return blockRecH;
}

bool dwgWriter15::emitDeferredBlockControl() {
    // BLOCK_CONTROL: numEntries = user blocks count; +2 phantoms for
    // MODEL_SPACE + PAPER_SPACE are appended to the child handle list.
    std::vector<duint32> children;
    children.reserve(m_userBlockRecordHandles.size() + 2);
    for (duint32 h : m_userBlockRecordHandles) children.push_back(h);
    children.push_back(reservedHandle::BLOCK_MODEL_SPACE);
    children.push_back(reservedHandle::BLOCK_PAPER_SPACE);

    dwgBufferW& body = beginObject(reservedHandle::BLOCK_CONTROL);
    body.putObjType(m_version, oType::BLOCK_CONTROL);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeHardPtr(reservedHandle::BLOCK_CONTROL));
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);   // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);   // Have binary data (AC1027+)
    }
    body.putBitLong(static_cast<dint32>(m_userBlockRecordHandles.size()));
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0 (R2007+)
    }
    body.putHandle(makeNullHandle());  // NullH
    body.putHandle(makeSoftOwner(0));  // XDic null
    for (duint32 h : children)
        body.putHandle(makeHardPtr(h));
    finishObject();
    return true;
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
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        bk.encodeDwg(m_version, &body, 0, &m_objectStrings, &m_objectHandles);
    } else {
        bk.encodeDwg(m_version, &body, 0);
    }
    finishObject();
}

void dwgWriter15::emitBlockRecord(duint32 handle, const std::string& name,
                                  duint32 blockHandle,
                                  duint32 endBlockHandle,
                                  int insUnits) {
    dwgBufferW& body = beginObject(handle);

    // Common table-entry preamble.
    body.putObjType(m_version, oType::BLOCK_RECORD);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);              // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);                   // extDataSize
    body.putBitLong(0);                    // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);                    // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);                    // Have binary data (AC1027+)
    }

    // Block_Record body — mirrors DRW_Block_Record::parseDwg.
    // For AC1024, strings go to m_objectStrings and handles to m_objectHandles.
    dwgBufferW *strBuf = (m_version > DRW::AC1018) ? &m_objectStrings : &body;
    dwgBufferW *hdlBuf = (m_version > DRW::AC1018) ? &m_objectHandles : &body;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
    }

    strBuf->putVariableText(m_version, name);
    body.putBit(0);                        // flags bit 6 (xref-ref)
    if (m_version <= DRW::AC1018) {
        body.putBitShort(0);               // xrefindex BS (R2004-)
    }
    body.putBit(0);                        // flags bit 4 (xref dep)
    body.putBit(0);                        // anon (*U bit)
    body.putBit(0);                        // attdefs
    body.putBit(0);                        // xref
    body.putBit(0);                        // overlaid xref
    body.putBit(0);                        // R2000+ loaded-xref
    if (m_version > DRW::AC1015) {
        body.putBitLong(0);                // objectCount = 0 (R2004+)
    }
    DRW_Coord origin;
    origin.x = origin.y = origin.z = 0.0;
    body.put3BitDouble(origin);            // basePoint 3BD
    strBuf->putVariableText(m_version, std::string{});  // xrefPath empty
    body.putRawChar8(0);                   // insertCount terminator (R2000+)
    strBuf->putVariableText(m_version, std::string{});  // bkdesc empty
    body.putBitLong(0);                    // prevData BL = 0
    if (m_version > DRW::AC1018) {
        body.putBitShort(static_cast<duint16>(insUnits));
        body.putBit(0);                    // canExplode B (R2007+)
        body.putRawChar8(0);               // bkScaling RC (R2007+)
    }

    // Handle stream.
    hdlBuf->putHandle(makeHardPtr(reservedHandle::BLOCK_CONTROL));  // parent
    hdlBuf->putHandle(makeSoftOwner(0));                            // XDic null
    hdlBuf->putHandle(makeNullHandle());                            // NullH
    hdlBuf->putHandle(makeHardPtr(blockHandle));                    // block ref
    if (m_version <= DRW::AC1015) {
        hdlBuf->putHandle(makeNullHandle());  // firstEH
        hdlBuf->putHandle(makeNullHandle());  // lastEH
    }
    hdlBuf->putHandle(makeHardPtr(endBlockHandle));  // endBlock
    hdlBuf->putHandle(makeSoftOwner(0));             // layoutH null

    finishObject();
}

void dwgWriter15::emitTableRecord(duint16 typeCode, duint32 handle,
                                  const std::string& name) {
    dwgBufferW& body = beginObject(handle);

    // Common preamble — same structure as control objects.
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize placeholder (R2000/R2004 only)
    }
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);   // extDataSize
    body.putBitLong(0);    // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);    // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);    // Have binary data (AC1027+)
    }

    // Record name: for AC1024 go into the string section (m_objectStrings),
    // which finishObject() appends to the data body with a footer.
    // For R2000/R2004 write inline in body.
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        m_objectStrings.putVariableText(m_version, name);
    } else {
        body.putVariableText(m_version, name);
    }

    finishObject();
}

// --- Shared preamble helper for full table-record emitters -----------------
// Writes OT + objSize stub (R2000/R2004) + own-handle + extDataSize +
// numReactors + xDictFlag.  Sets up strBuf/hdlBuf for AC1024 three-stream;
// caller passes back the strBuf/hdlBuf pointers via the out-params so the
// subsequent encodeDwg call lands its strings/handles in the right buffer.
static void emitRecordPreamble(dwgBufferW& body, DRW::Version version,
                                duint16 otype, duint32 handle,
                                dwgBufferW& strBuf, dwgBufferW& hdlBuf,
                                dwgBufferW*& sb, dwgBufferW*& hb) {
    body.putObjType(version, otype);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        body.putRawLong32(0);
    body.putHandle(makeHardPtr(handle));
    body.putBitShort(0);
    body.putBitLong(0);
    if (version > DRW::AC1015)
        body.putBit(0);   // xDictFlag
    if (version > DRW::AC1024)
        body.putBit(0);   // Have binary data (AC1027+)
    if (version > DRW::AC1018) {
        strBuf.reset();
        hdlBuf.reset();
        sb = &strBuf;
        hb = &hdlBuf;
    } else {
        sb = &body;
        hb = &body;
    }
}

void dwgWriter15::emitLtypeRecord(duint32 handle, const DRW_LType& lt) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::LTYPE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    lt.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitLayerRecord(duint32 handle, const DRW_Layer& lay) {
    // Resolve linetype name to a handle before encoding.
    DRW_Layer layerCopy = lay;
    std::string ltUpper = toUpperCase(lay.lineType);
    auto ltIt = m_writingCtx.ltypeMap.find(ltUpper);
    if (ltIt != m_writingCtx.ltypeMap.end()) {
        layerCopy.lTypeH = makeHardPtr(ltIt->second);
    } else {
        layerCopy.lTypeH = makeHardPtr(reservedHandle::LTYPE_CONTINUOUS);
    }

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::LAYER, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    layerCopy.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitStyleRecord(duint32 handle, const DRW_Textstyle& ts) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::STYLE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    ts.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitViewRecord(duint32 handle, const DRW_View& view) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::VIEW, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    view.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitVportRecord(duint32 handle, const DRW_Vport& vp) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::VPORT, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    vp.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitAppIdRecord(duint32 handle, const DRW_AppId& ai) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::APPID, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    ai.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitDimstyleRecord(duint32 handle, const DRW_Dimstyle& ds) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::DIMSTYLE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    ds.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

void dwgWriter15::emitAcDbPlaceholderObject(
    duint32 handle, const DRW_AcDbPlaceholder& placeholder) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 80, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    placeholder.encodeDwg(m_version, &body, hb);
    finishObject();
}

bool dwgWriter15::writeAcDbPlaceholder(
    const DRW_AcDbPlaceholder& placeholder) {
    if (m_version < DRW::AC1021)
        return false;

    DRW_AcDbPlaceholder object = placeholder;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    emitAcDbPlaceholderObject(object.handle, object);
    return true;
}

void dwgWriter15::emitSunObject(duint32 handle, const DRW_Sun& sun) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_Sun::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    sun.encodeDwg(m_version, &body, hb);
    finishObject();
}

bool dwgWriter15::writeSun(const DRW_Sun& sun) {
    if (m_version < DRW::AC1021)
        return false;

    DRW_Sun object = sun;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSunObjectClass(object.handle))
        return false;

    emitSunObject(object.handle, object);
    return true;
}

void dwgWriter15::emitMLeaderStyleObject(duint32 handle,
                                         const DRW_MLeaderStyle& style) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_MLeaderStyle::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    style.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

bool dwgWriter15::writeMLeaderStyle(const DRW_MLeaderStyle& style) {
    if (m_version < DRW::AC1021)
        return false;

    DRW_MLeaderStyle object = style;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerMLeaderStyleObjectClass(object.handle))
        return false;

    emitMLeaderStyleObject(object.handle, object);
    return true;
}

// DICTIONARY (ODA fixed type 42) — no class registration required.  Uses the
// standard preamble + DRW_Dictionary::encodeDwg sandwich, mirroring the
// SUN / PLACEHOLDER native-writer pattern.  Per-entry string handles live
// in sb so the AC1018+ split-buffer convention applies.
void dwgWriter15::emitDictionaryObject(duint32 handle,
                                       const DRW_Dictionary& dictionary) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 42, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    dictionary.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

bool dwgWriter15::writeDictionary(const DRW_Dictionary& dictionary) {
    DRW_Dictionary object = dictionary;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    emitDictionaryObject(object.handle, object);
    return true;
}

// XRECORD (ODA fixed type 0x4f = 79) — no class registration required.
// XRECORD's encoder takes (buf, /*strBuf=*/nullptr, hb): strings are emitted
// inline as byte-counted data within the data section, so no separate
// string buffer is needed.
void dwgWriter15::emitXRecordObject(duint32 handle,
                                    const DRW_XRecord& xrecord) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 79, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    xrecord.encodeDwg(m_version, &body, nullptr, hb);
    finishObject();
}

bool dwgWriter15::writeXRecord(const DRW_XRecord& xrecord) {
    DRW_XRecord object = xrecord;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    emitXRecordObject(object.handle, object);
    return true;
}

// LAYOUT (ODA fixed type 82, §20.4.84) — no class registration required.
// Encoder needs both string and handle buffers (AC1018+ split, inline
// pre-AC1018).
void dwgWriter15::emitLayoutObject(duint32 handle, const DRW_Layout& layout) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 82, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    layout.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

bool dwgWriter15::writeLayout(const DRW_Layout& layout) {
    DRW_Layout object = layout;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    emitLayoutObject(object.handle, object);
    return true;
}

// GROUP (ODA fixed type 72) — no class registration required.  Standard
// preamble + DRW_Group::encodeDwg sandwich.  description is the only
// string field (carried in sb for AC1018+ split); entityHandles live in
// the handle stream.  Mirrors the DICTIONARY / LAYOUT shape from PR 8b/8c.
void dwgWriter15::emitGroupObject(duint32 handle, const DRW_Group& group) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 72, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    group.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

bool dwgWriter15::writeGroup(const DRW_Group& group) {
    DRW_Group object = group;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    emitGroupObject(object.handle, object);
    return true;
}

// RASTERVARIABLES (AcDbRasterVariables, custom class 505) — class
// registration required so the reader's CLASSES section dispatch can
// resolve the recordName back to DRW_RasterVariables::parseDwg.  Standard
// preamble + DRW_RasterVariables::encodeDwg sandwich.  Encoder ignores
// strBuf (no string fields) — pass nullptr to mirror parse semantics.
void dwgWriter15::emitRasterVariablesObject(
    duint32 handle, const DRW_RasterVariables& rasterVariables) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version,
                       DRW_RasterVariables::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    rasterVariables.encodeDwg(m_version, &body, nullptr, hb);
    finishObject();
}

bool dwgWriter15::writeRasterVariables(
    const DRW_RasterVariables& rasterVariables) {
    if (m_version < DRW::AC1021)
        return false;

    DRW_RasterVariables object = rasterVariables;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerRasterVariablesObjectClass(object.handle))
        return false;

    emitRasterVariablesObject(object.handle, object);
    return true;
}

// GEODATA (AcDbGeoData, custom class 506) — class registration required.
// Standard preamble + DRW_GeoData::encodeDwg sandwich.  Encoder writes its
// own common-handle prefix into hb (the preamble does NOT) plus several
// variable-text fields into sb.
void dwgWriter15::emitGeoDataObject(duint32 handle, const DRW_GeoData& geoData) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_GeoData::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    geoData.encodeDwg(m_version, &body, sb, hb);
    finishObject();
}

bool dwgWriter15::writeGeoData(const DRW_GeoData& geoData) {
    if (m_version < DRW::AC1021)
        return false;

    DRW_GeoData object = geoData;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerGeoDataObjectClass(object.handle))
        return false;

    emitGeoDataObject(object.handle, object);
    return true;
}

// --- add*() methods: register user table records for deferred emission -----

void dwgWriter15::addLType(const DRW_LType& lt) {
    std::string upper = toUpperCase(lt.name);
    if (upper.empty() || m_writingCtx.ltypeMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.ltypeMap[upper] = h;
    m_pendingLTypes.emplace_back(h, lt);
}

void dwgWriter15::addLayer(const DRW_Layer& lay) {
    std::string upper = toUpperCase(lay.name);
    if (upper.empty() || m_writingCtx.layerMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.layerMap[upper] = h;
    m_pendingLayers.emplace_back(h, lay);
}

void dwgWriter15::addTextstyle(const DRW_Textstyle& ts) {
    std::string upper = toUpperCase(ts.name);
    if (upper.empty() || m_writingCtx.styleMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.styleMap[upper] = h;
    m_pendingStyles.emplace_back(h, ts);
}

void dwgWriter15::addView(const DRW_View& view) {
    std::string upper = toUpperCase(view.name);
    if (upper.empty() || m_writingCtx.viewMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.viewMap[upper] = h;
    m_pendingViews.emplace_back(h, view);
}

void dwgWriter15::addVport(const DRW_Vport& vp) {
    std::string upper = toUpperCase(vp.name);
    if (upper.empty() || m_writingCtx.vportMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.vportMap[upper] = h;
    m_pendingVports.emplace_back(h, vp);
}

void dwgWriter15::addDimstyle(const DRW_Dimstyle& ds) {
    std::string upper = toUpperCase(ds.name);
    if (upper.empty() || m_writingCtx.dimstyleMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.dimstyleMap[upper] = h;
    m_pendingDimstyles.emplace_back(h, ds);
}

void dwgWriter15::addAppId(const DRW_AppId& ai) {
    std::string upper = toUpperCase(ai.name);
    if (upper.empty() || m_writingCtx.appidMap.count(upper)) return;
    duint32 h = m_handles.next();
    m_writingCtx.appidMap[upper] = h;
    m_pendingAppIds.emplace_back(h, ai);
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
    // Resolve layer name → handle.  Caller may have pre-set layerH.ref for
    // round-trip; only resolve from name when the handle is unset.
    if (ent->layerH.ref == 0) {
        auto layerUp = toUpperCase(ent->layer);
        auto it = m_writingCtx.layerMap.find(layerUp);
        ent->layerH.ref = (it != m_writingCtx.layerMap.end())
            ? it->second
            : static_cast<duint32>(0x12);  // fallback: layer "0"
    }
    // Resolve linetype name → handle when caller specified a non-empty name.
    if (ent->lTypeH.ref == 0 && !ent->lineType.empty()) {
        auto ltUp = toUpperCase(ent->lineType);
        auto it = m_writingCtx.ltypeMap.find(ltUp);
        if (it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }
    dwgBufferW& body = beginObject(handle);
    bool ok = ent->encodeDwg(m_version, &body, /*bs=*/0);
    if (!ok) return false;
    finishObject();
    return true;
}

bool dwgWriter15::writeDwgObjects() {
    // Ensure standard reserved entries are present.  The constructor
    // pre-seeds these; insert is a no-op if a key already exists, which
    // protects user entries added via add*() before this call.
    m_writingCtx.ltypeMap.insert({"CONTINUOUS", reservedHandle::LTYPE_CONTINUOUS});
    m_writingCtx.ltypeMap.insert({"BYLAYER",    reservedHandle::LTYPE_BYLAYER});
    m_writingCtx.ltypeMap.insert({"BYBLOCK",    reservedHandle::LTYPE_BYBLOCK});
    m_writingCtx.layerMap.insert({"0",        static_cast<duint32>(0x12)});
    m_writingCtx.styleMap.insert({"STANDARD", static_cast<duint32>(0x13)});
    // Named VIEW has no required standard record.
    m_writingCtx.appidMap.insert({"ACAD",     static_cast<duint32>(0x14)});
    m_writingCtx.dimstyleMap.insert({"STANDARD", static_cast<duint32>(0x15)});
    m_writingCtx.vportMap.insert({"*ACTIVE",  static_cast<duint32>(0x16)});

    // --- LTYPE section --- (emitted before LAYER so ltypeMap is populated
    // when emitLayerRecord resolves lineType→handle)
    {
        std::vector<duint32> ltypeChildren = {
            reservedHandle::LTYPE_BYBLOCK,
            reservedHandle::LTYPE_BYLAYER,
            reservedHandle::LTYPE_CONTINUOUS
        };
        for (auto& p : m_pendingLTypes) ltypeChildren.push_back(p.first);
        // numEntries excludes BYBLOCK/BYLAYER phantoms (reader adds +2)
        emitControlObject(oType::LTYPE_CONTROL, reservedHandle::LTYPE_CONTROL,
                          1 + static_cast<duint32>(m_pendingLTypes.size()),
                          ltypeChildren);
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_BYBLOCK,    "BYBLOCK");
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_BYLAYER,    "BYLAYER");
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_CONTINUOUS, "CONTINUOUS");
        for (auto& p : m_pendingLTypes)
            emitLtypeRecord(p.first, p.second);
    }

    // --- LAYER section ---
    {
        std::vector<duint32> layerChildren = {0x12};
        for (auto& p : m_pendingLayers) layerChildren.push_back(p.first);
        emitControlObject(oType::LAYER_CONTROL, reservedHandle::LAYER_CONTROL,
                          1 + static_cast<duint32>(m_pendingLayers.size()),
                          layerChildren);
        emitTableRecord(oType::LAYER, 0x12, "0");
        for (auto& p : m_pendingLayers)
            emitLayerRecord(p.first, p.second);
    }

    // --- STYLE section ---
    {
        std::vector<duint32> styleChildren = {0x13};
        for (auto& p : m_pendingStyles) styleChildren.push_back(p.first);
        emitControlObject(oType::STYLE_CONTROL, reservedHandle::STYLE_CONTROL,
                          1 + static_cast<duint32>(m_pendingStyles.size()),
                          styleChildren);
        emitTableRecord(oType::STYLE, 0x13, "STANDARD");
        for (auto& p : m_pendingStyles)
            emitStyleRecord(p.first, p.second);
    }

    // --- VIEW section --- (named views; no required standard record)
    {
        std::vector<duint32> viewChildren;
        for (auto& p : m_pendingViews) viewChildren.push_back(p.first);
        emitControlObject(oType::VIEW_CONTROL, reservedHandle::VIEW_CONTROL,
                          static_cast<duint32>(m_pendingViews.size()),
                          viewChildren);
        for (auto& p : m_pendingViews)
            emitViewRecord(p.first, p.second);
    }

    // UCS_CONTROL: no entries.
    emitControlObject(oType::UCS_CONTROL, reservedHandle::UCS_CONTROL, 0, {});

    // --- VPORT section ---
    {
        std::vector<duint32> vportChildren = {0x16};
        for (auto& p : m_pendingVports) vportChildren.push_back(p.first);
        emitControlObject(oType::VPORT_CONTROL, reservedHandle::VPORT_CONTROL,
                          1 + static_cast<duint32>(m_pendingVports.size()),
                          vportChildren);
        emitTableRecord(oType::VPORT, 0x16, "*ACTIVE");
        for (auto& p : m_pendingVports)
            emitVportRecord(p.first, p.second);
    }

    // --- APPID section ---
    {
        std::vector<duint32> appidChildren = {0x14};
        for (auto& p : m_pendingAppIds) appidChildren.push_back(p.first);
        emitControlObject(oType::APPID_CONTROL, reservedHandle::APPID_CONTROL,
                          1 + static_cast<duint32>(m_pendingAppIds.size()),
                          appidChildren);
        emitTableRecord(oType::APPID, 0x14, "ACAD");
        for (auto& p : m_pendingAppIds)
            emitAppIdRecord(p.first, p.second);
    }

    // --- DIMSTYLE section ---
    {
        std::vector<duint32> dimChildren = {0x15};
        for (auto& p : m_pendingDimstyles) dimChildren.push_back(p.first);
        emitControlObject(oType::DIMSTYLE_CONTROL, reservedHandle::DIMSTYLE_CONTROL,
                          1 + static_cast<duint32>(m_pendingDimstyles.size()),
                          dimChildren);
        emitTableRecord(oType::DIMSTYLE, 0x15, "STANDARD");
        for (auto& p : m_pendingDimstyles)
            emitDimstyleRecord(p.first, p.second);
    }

    // VPORT_ENTITY_HEADER_CONTROL: no entries.
    emitControlObject(oType::VPORT_ENTITY_HEADER_CONTROL,
                      reservedHandle::VPORT_ENTITY_HEADER_CONTROL, 0, {});

    // Block entities + Block_Records for *Model_Space and *Paper_Space.
    constexpr duint32 blkModelStart = 0x1B;
    constexpr duint32 blkModelEnd   = 0x1C;
    constexpr duint32 blkPaperStart = 0x1D;
    constexpr duint32 blkPaperEnd   = 0x1E;

    emitBlockEntity(blkModelStart, "*Model_Space", /*isEnd=*/false);
    emitBlockEntity(blkModelEnd,   std::string{},   /*isEnd=*/true);
    emitBlockEntity(blkPaperStart, "*Paper_Space", /*isEnd=*/false);
    emitBlockEntity(blkPaperEnd,   std::string{},   /*isEnd=*/true);

    emitBlockRecord(reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space",
                    blkModelStart, blkModelEnd);
    emitBlockRecord(reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space",
                    blkPaperStart, blkPaperEnd);

    return true;
}

bool dwgWriter15::replayRawObject(const DRW_UnsupportedObject& object) {
    if (object.m_isEntity || object.m_handle == 0 || object.m_rawBytes.empty())
        return false;

    m_handles.reserve(object.m_handle);
    if (!registerRawObjectClass(object))
        return false;

    const duint32 frameStart = static_cast<duint32>(m_buf.size());
    m_buf.putModularShort(static_cast<dint32>(object.m_rawBytes.size()));
    if (m_version > DRW::AC1021)
        m_buf.putUModularChar(object.m_bodyBitSize);
    const size_t bodyStart = m_buf.size();
    m_buf.putBytes(object.m_rawBytes.data(), object.m_rawBytes.size());

    const duint16 crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                                    bodyStart + object.m_rawBytes.size());
    m_buf.putRawShort16(crc);
    m_objectMap.emplace_back(object.m_handle, frameStart);
    return true;
}

dwgBufferW& dwgWriter15::beginObject(duint32 handle) {
    m_currentHandle = handle;
    m_objectBody.data().clear();
    return m_objectBody;
}

void dwgWriter15::finishObject() {
    // Back-patch the RL objSize ("size of object in bits, not including CRC")
    // at the correct stream-bit offset.  The offset equals the BS width for
    // the oType, which we detect from the first 2 bits of the body:
    //   code "01" → RC payload → 10-bit BS → RL at bit 10
    //   code "00" → RS payload → 18-bit BS → RL at bit 18
    //   code "10" or "11" → no payload → 2-bit BS → RL at bit 2
    duint32 bitCount = static_cast<duint32>(m_objectBody.size()) * 8;
    if (m_objectBody.bitPos() != 0)
        bitCount -= static_cast<duint32>(8 - m_objectBody.bitPos());
    if (!m_objectBody.data().empty()) {
        duint8 bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
        size_t rlBitOffset = (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        m_objectBody.patchRawLong32AtBit(rlBitOffset, bitCount);
    }

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

    // CRC covers MS prefix + body — spec §20.2: "The CRC includes the size
    // bytes."  frameStart was recorded before putModularShort.
    duint16 crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
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
        duint32 base = objectBaseOffset();
        size_t entryEnd = entryStart;
        while (entryEnd < m_objectMap.size()) {
            duint32 h = m_objectMap[entryEnd].first;
            duint32 off = m_objectMap[entryEnd].second - base;

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
    const std::vector<duint8> auxHeader = buildR2000AuxHeaderContent(m_header);
    m_sectionOffsets[recno::AUXHEADER] = static_cast<duint32>(m_buf.size());
    if (!auxHeader.empty())
        m_buf.putBytes(auxHeader.data(), auxHeader.size());
    m_sectionSizes[recno::AUXHEADER] = static_cast<duint32>(auxHeader.size());
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
