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
#include "dwgbuffer.h"
#include "dwgbufferw.h"
#include "dwgutil.h"

namespace {

/// File-header CRC seed-XOR adjustment (matches reader at
/// dwgreader15.cpp:94-106). Indexed by `num_sections`.
std::uint16_t seedXorForCount(std::uint8_t numSections) {
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
    constexpr std::uint32_t BLOCK_CONTROL              = 0x01;
    constexpr std::uint32_t LAYER_CONTROL              = 0x02;
    constexpr std::uint32_t STYLE_CONTROL              = 0x03;
    constexpr std::uint32_t LTYPE_CONTROL              = 0x05;
    constexpr std::uint32_t VIEW_CONTROL               = 0x06;
    constexpr std::uint32_t UCS_CONTROL                = 0x07;
    constexpr std::uint32_t VPORT_CONTROL              = 0x08;
    constexpr std::uint32_t APPID_CONTROL              = 0x09;
    constexpr std::uint32_t DIMSTYLE_CONTROL           = 0x0A;
    constexpr std::uint32_t VPORT_ENTITY_HEADER_CONTROL = 0x0B;
    // Reserved-block / linetype handles emitted in the HEADER section's
    // trailing 5-handle block (and as phantom entries in
    // BLOCK_CONTROL / LTYPE_CONTROL).  Phase 3e populates the records.
    constexpr std::uint32_t LTYPE_BYBLOCK    = 0x0F;
    constexpr std::uint32_t LTYPE_BYLAYER    = 0x10;
    constexpr std::uint32_t LTYPE_CONTINUOUS = 0x11;
    constexpr std::uint32_t BLOCK_MODEL_SPACE = 0x17;
    constexpr std::uint32_t BLOCK_PAPER_SPACE = 0x18;
}

/// Object-type (oType) constants for the 10 R2000 control objects.
/// Same set as DRW_ObjControl::parseDwg's `oType` switch references.
namespace oType {
    constexpr std::uint16_t BLOCK_CONTROL              = 0x30;
    constexpr std::uint16_t LAYER_CONTROL              = 0x32;
    constexpr std::uint16_t STYLE_CONTROL              = 0x34;
    constexpr std::uint16_t LTYPE_CONTROL              = 0x38;
    constexpr std::uint16_t VIEW_CONTROL               = 0x3C;
    constexpr std::uint16_t UCS_CONTROL                = 0x3E;
    constexpr std::uint16_t VPORT_CONTROL              = 0x40;
    constexpr std::uint16_t APPID_CONTROL              = 0x42;
    constexpr std::uint16_t DIMSTYLE_CONTROL           = 0x44;
    constexpr std::uint16_t VPORT_ENTITY_HEADER_CONTROL = 0x46;
    // Record oTypes are the matching control oType + 1 except
    // BLOCK_RECORD which uses 0x31.  Verified against libreDWG dwg.spec.
    constexpr std::uint16_t BLOCK_RECORD               = 0x31;
    constexpr std::uint16_t LAYER                      = 0x33;
    constexpr std::uint16_t STYLE                      = 0x35;
    constexpr std::uint16_t LTYPE                      = 0x39;
    constexpr std::uint16_t VIEW                       = 0x3D;
    constexpr std::uint16_t UCS                        = 0x3F;
    constexpr std::uint16_t APPID                      = 0x43;
    constexpr std::uint16_t DIMSTYLE                   = 0x45;
    constexpr std::uint16_t VPORT                      = 0x41;
}

bool isReplayableFixedModelerRawEntity(const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && (object.m_objectType == 34   // VIEWPORT (paper-space; no typed RS entity)
            || object.m_objectType == 37 || object.m_objectType == 38
            || object.m_objectType == 39);
}

std::uint16_t underlayDefinitionClassNum(
    const DRW_UnderlayDefinition& definition) {
    switch (definition.kind) {
    case DRW_UnderlayDefinition::DGN:
        return DRW_UnderlayDefinition::kDwgClassNumDgn;
    case DRW_UnderlayDefinition::DWF:
        return DRW_UnderlayDefinition::kDwgClassNumDwf;
    case DRW_UnderlayDefinition::PDF:
    default:
        return DRW_UnderlayDefinition::kDwgClassNumPdf;
    }
}

/// Build a hard-pointer (code 4) handle with the minimum-width ref.
dwgHandle makeHardPtr(std::uint32_t ref) {
    dwgHandle h;
    h.code = (ref == 0) ? 0 : 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build an own-handle entry (code 0) — the form used for an object's own handle.
dwgHandle makeOwnHandle(std::uint32_t ref) {
    dwgHandle h;
    h.code = 0;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
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

void putAuxRL(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>(x));
    v.push_back(static_cast<std::uint8_t>(x >> 8));
    v.push_back(static_cast<std::uint8_t>(x >> 16));
    v.push_back(static_cast<std::uint8_t>(x >> 24));
}

void putAuxRS(std::vector<std::uint8_t>& v, std::uint16_t x) {
    v.push_back(static_cast<std::uint8_t>(x));
    v.push_back(static_cast<std::uint8_t>(x >> 8));
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

void splitAuxDate(double stored, std::int32_t& day, std::int32_t& msec) {
    // Canonical DWG aux-header date encoding: `stored` is julianDay + msec/86_400_000.
    constexpr double kMsecPerDay = 86400000.0;
    const double dayFloor = std::floor(stored);
    day = static_cast<std::int32_t>(dayFloor);
    const double frac = stored - dayFloor;
    msec = static_cast<std::int32_t>(std::lround(frac * kMsecPerDay));
    if (msec >= static_cast<std::int32_t>(kMsecPerDay)) {
        day += 1;
        msec = 0;
    }
}

void putAuxDate(std::vector<std::uint8_t>& v, double stored) {
    std::int32_t day = 0;
    std::int32_t msec = 0;
    splitAuxDate(stored, day, msec);
    putAuxRL(v, static_cast<std::uint32_t>(day));
    putAuxRL(v, static_cast<std::uint32_t>(msec));
}

std::vector<std::uint8_t> buildR2000AuxHeaderContent(const DRW_Header *header) {
    constexpr std::uint16_t rawVersion = 23;  // AC1015
    std::vector<std::uint8_t> v;
    v.reserve(111);
    v.push_back(0xff);
    v.push_back(0x77);
    v.push_back(0x01);
    putAuxRS(v, rawVersion);
    putAuxRS(v, 0);                     // maintenance version
    putAuxRL(v, 1);                     // number of saves
    putAuxRL(v, static_cast<std::uint32_t>(-1));
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

    const std::uint32_t handSeed = header ? header->getHandSeed() : 0;
    putAuxRL(v, handSeed <= 0x7fffffffu ? handSeed : static_cast<std::uint32_t>(-1));
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
dwgHandle makeSoftOwner(std::uint32_t ref) {
    dwgHandle h;
    h.code = 3;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
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
        m_buf.putRawChar8(static_cast<std::uint8_t>(dwgVersionString::R2000[i]));

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
    m_recordsOffset = static_cast<std::uint32_t>(m_buf.size());

    // Emit `m_numSections` stub records of (RC recno, RL address=0, RL size=0).
    // recno values are 0..(N-1) in canonical order.
    for (std::uint8_t recno = 0; recno < m_numSections; ++recno) {
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

size_t dwgWriter15::beginSentinelSection(const std::uint8_t (&beginSentinel)[16]) {
    // Caller must ensure cursor is byte-aligned before invoking.
    m_buf.alignToByte();
    m_buf.putBytes(beginSentinel, 16);
    // Reserve 4 bytes for RL section payload size — back-patched at end.
    size_t sizeOffset = m_buf.size();
    m_buf.putRawLong32(0);
    return sizeOffset;
}

void dwgWriter15::endSentinelSection(size_t sectionStart, size_t sizeOffset,
                                     const std::uint8_t (&endSentinel)[16]) {
    // Bit-packed bodies (HEADER vars, CLASSES, etc.) typically leave the
    // cursor mid-byte.  Pad to the next byte boundary so the END sentinel
    // lands aligned and the recorded payload size is in whole bytes.
    m_buf.alignToByte();

    // Patch the payload size: bytes between (sizeOffset + 4) and current cursor.
    std::uint32_t payloadSize =
        static_cast<std::uint32_t>(m_buf.size()) - static_cast<std::uint32_t>(sizeOffset + 4);
    m_buf.patchRawLong32(sizeOffset, payloadSize);

    // CRC covers RL-size + data only — spec §9: "covers the stepper and the
    // data"; sectionStart+16 skips the 16-byte beginning sentinel.
    std::uint16_t crc = m_buf.crc16(0xC0C1, sectionStart + 16, m_buf.size());
    m_buf.putRawShort16(crc);        // CRC BEFORE end sentinel
    m_buf.putBytes(endSentinel, 16); // end sentinel AFTER CRC
}

void dwgWriter15::initHeaderControlHandles() {
    if (m_header == nullptr) return;
    auto fillIfZero = [](std::uint32_t& slot, std::uint32_t value) {
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
    m_sectionOffsets[recno::HEADER] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::HEADER_BEGIN);

    // Phase 3d: populate the control-handle members on m_header so the
    // encoder's control-handle block (parseDwg:2162-2199) references the
    // canonical reserved handles.
    initHeaderControlHandles();

    // Phase 3a: full 282-var bit-packed emission via DRW_Header::encodeDwg.
    // For R2000 the handle stream is inline with the data stream — pass
    // the same accumulator for both buf and hBbuf.
    if (m_header != nullptr) {
        // Propagate encode failure: writeDwgHeader reports success as a bool.
        if (!m_header->encodeDwg(m_version, &m_buf, &m_buf))
            return false;
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeDwgClasses() {
    if (hasDwgClassConflict())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // R2000 CLASSES is just the packed class entries between sentinels.
    // Field layout mirrors DRW_Class::parseDwg and ODA class section records.
    for (const auto& definition : sortedDwgClassDefinitions())
        writeDwgClassDefinition(definition, &m_buf, nullptr);

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::CLASSES_END);

    m_sectionSizes[recno::CLASSES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

void dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    std::initializer_list<std::uint32_t> childHandles)
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
    body.putBitLong(static_cast<std::int32_t>(numEntries));
    if (typeCode == oType::DIMSTYLE_CONTROL) {
        body.putRawChar8(0);
    }
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0: no strings in control objects (R2007+)
    }
    body.putHandle(makeNullHandle());
    body.putHandle(makeSoftOwner(0));  // XDic null
    for (std::uint32_t child : childHandles)
        body.putHandle(makeHardPtr(child));

    finishObject();
}

void dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    const std::vector<std::uint32_t>& childHandles)
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
    body.putBitLong(static_cast<std::int32_t>(numEntries));
    if (typeCode == oType::DIMSTYLE_CONTROL)
        body.putRawChar8(0);
    if (m_version > DRW::AC1018)
        body.putBit(0);
    body.putHandle(makeNullHandle());
    body.putHandle(makeSoftOwner(0));
    for (std::uint32_t child : childHandles)
        body.putHandle(makeHardPtr(child));
    finishObject();
}

std::uint32_t dwgWriter15::defineBlock(const std::string& name,
                                 const DRW_Coord& basePoint,
                                 int insUnits) {
    const std::string key = toUpperCase(name);
    if (key.empty())
        return 0;
    const auto existing = m_userBlockHandles.find(key);
    if (existing != m_userBlockHandles.end())
        return existing->second;

    // Allocate a fresh handle trio.
    std::uint32_t blockRecH  = m_handles.next();
    std::uint32_t blockH     = m_handles.next();
    std::uint32_t endBlockH  = m_handles.next();

    // Block entity for the start of the block body.
    DRW_Block bk;
    bk.handle = blockH;
    bk.layerH.ref = 0x12;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.parentHandle = blockRecH;
    bk.setIsEnd(false);
    {
        dwgBufferW& body = beginObject(blockH);
        if (m_version > DRW::AC1018) {
            m_objectStrings.reset();
            m_objectHandles.reset();
            if (!bk.encodeDwg(m_version, &body, /*bs=*/0,
                              &m_objectStrings, &m_objectHandles)) {
                return 0;
            }
        } else if (!bk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return 0;
        }

        finishObject();
    }

    // ENDBLK entity.
    DRW_Block endBlk;
    endBlk.handle = endBlockH;
    endBlk.layerH.ref = 0x12;
    endBlk.color = 256;
    endBlk.parentHandle = blockRecH;
    endBlk.setIsEnd(true);
    {
        dwgBufferW& body = beginObject(endBlockH);
        if (m_version > DRW::AC1018) {
            m_objectStrings.reset();
            m_objectHandles.reset();
            if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0,
                                  &m_objectStrings, &m_objectHandles)) {
                return 0;
            }
        } else if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return 0;
        }

        finishObject();
    }

    PendingUserBlock block;
    block.blockRecordHandle = blockRecH;
    block.blockHandle = blockH;
    block.endBlockHandle = endBlockH;
    block.name = name;
    block.basePoint = basePoint;
    block.insUnits = insUnits;
    m_userBlocks.push_back(block);
    m_userBlockHandles.emplace(key, blockRecH);
    return blockRecH;
}

bool dwgWriter15::beginBlockContent(std::uint32_t blockRecordHandle) {
    if (m_activeUserBlockRecordHandle != 0)
        return false;
    for (const PendingUserBlock& block : m_userBlocks) {
        if (block.blockRecordHandle == blockRecordHandle) {
            m_activeUserBlockRecordHandle = blockRecordHandle;
            return true;
        }
    }
    return false;
}

bool dwgWriter15::endBlockContent() {
    if (m_activeUserBlockRecordHandle == 0)
        return false;
    m_activeUserBlockRecordHandle = 0;
    return true;
}

bool dwgWriter15::emitDeferredBlockControl() {
    if (m_activeUserBlockRecordHandle != 0)
        return false;
    for (const PendingUserBlock& block : m_userBlocks) {
        emitBlockRecord(block.blockRecordHandle, block.name, block.basePoint,
                        block.blockHandle, block.endBlockHandle,
                        block.entityHandles, block.insUnits);
    }
    // BLOCK_CONTROL: numEntries = user blocks count; +2 phantoms for
    // MODEL_SPACE + PAPER_SPACE are appended to the child handle list.
    std::vector<std::uint32_t> children;
    children.reserve(m_userBlocks.size() + 2);
    for (const PendingUserBlock& block : m_userBlocks)
        children.push_back(block.blockRecordHandle);
    children.push_back(reservedHandle::BLOCK_MODEL_SPACE);
    children.push_back(reservedHandle::BLOCK_PAPER_SPACE);

    dwgBufferW& body = beginObject(reservedHandle::BLOCK_CONTROL);
    body.putObjType(m_version, oType::BLOCK_CONTROL);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeOwnHandle(reservedHandle::BLOCK_CONTROL));  // own handle: code 0 (plan 3.6)
    body.putBitShort(0);  // extDataSize
    body.putBitLong(0);   // numReactors
    if (m_version > DRW::AC1015) {
        body.putBit(0);   // xDictFlag=0 (R2004+)
    }
    if (m_version > DRW::AC1024) {
        body.putBit(0);   // Have binary data (AC1027+)
    }
    body.putBitLong(static_cast<std::int32_t>(m_userBlocks.size()));
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0 (R2007+)
    }
    body.putHandle(makeNullHandle());  // NullH
    body.putHandle(makeSoftOwner(0));  // XDic null
    for (std::uint32_t h : children)
        body.putHandle(makeHardPtr(h));
    finishObject();
    return true;
}

void dwgWriter15::emitBlockEntity(std::uint32_t handle, const std::string& name,
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
        (void)bk.encodeDwg(m_version, &body, 0, &m_objectStrings, &m_objectHandles);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    } else {
        (void)bk.encodeDwg(m_version, &body, 0);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    }
    finishObject();
}

void dwgWriter15::emitBlockRecord(std::uint32_t handle, const std::string& name,
                                  const DRW_Coord& basePoint,
                                  std::uint32_t blockHandle,
                                  std::uint32_t endBlockHandle,
                                  const std::vector<std::uint32_t>& entityHandles,
                                  int insUnits) {
    dwgBufferW& body = beginObject(handle);

    // Common table-entry preamble.
    body.putObjType(m_version, oType::BLOCK_RECORD);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);              // objSize stub (R2000/R2004 only)
    }
    body.putHandle(makeOwnHandle(handle));  // own handle: code 0 (plan 3.6)
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
        body.putBitLong(static_cast<std::int32_t>(entityHandles.size()));
    }
    body.put3BitDouble(basePoint);         // basePoint 3BD
    strBuf->putVariableText(m_version, std::string{});  // xrefPath empty
    body.putRawChar8(0);                   // insertCount terminator (R2000+)
    strBuf->putVariableText(m_version, std::string{});  // bkdesc empty
    body.putBitLong(0);                    // prevData BL = 0
    if (m_version > DRW::AC1018) {
        body.putBitShort(static_cast<std::uint16_t>(insUnits));
        body.putBit(0);                    // canExplode B (R2007+)
        body.putRawChar8(0);               // bkScaling RC (R2007+)
    }

    // Handle stream.
    hdlBuf->putHandle(makeHardPtr(reservedHandle::BLOCK_CONTROL));  // parent
    hdlBuf->putHandle(makeSoftOwner(0));                            // XDic null
    hdlBuf->putHandle(makeNullHandle());                            // NullH
    hdlBuf->putHandle(makeHardPtr(blockHandle));                    // block ref
    if (m_version <= DRW::AC1015) {
        const std::uint32_t first = entityHandles.empty() ? 0 : entityHandles.front();
        const std::uint32_t last = entityHandles.empty() ? 0 : entityHandles.back();
        hdlBuf->putHandle(first == 0 ? makeNullHandle() : makeHardPtr(first));
        hdlBuf->putHandle(last == 0 ? makeNullHandle() : makeHardPtr(last));
    } else {
        for (std::uint32_t entityHandle : entityHandles)
            hdlBuf->putHandle(makeHardPtr(entityHandle));
    }
    hdlBuf->putHandle(makeHardPtr(endBlockHandle));  // endBlock
    hdlBuf->putHandle(makeSoftOwner(0));             // layoutH null

    finishObject();
}

void dwgWriter15::emitTableRecord(std::uint16_t typeCode, std::uint32_t handle,
                                  const std::string& name) {
    dwgBufferW& body = beginObject(handle);

    // Common preamble — same structure as control objects.
    body.putObjType(m_version, typeCode);
    if (m_version > DRW::AC1014 && m_version < DRW::AC1024) {
        body.putRawLong32(0);  // objSize placeholder (R2000/R2004 only)
    }
    body.putHandle(makeOwnHandle(handle));  // own handle: code 0, not code 4 (plan 3.6)
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
                                std::uint16_t otype, std::uint32_t handle,
                                dwgBufferW& strBuf, dwgBufferW& hdlBuf,
                                dwgBufferW*& sb, dwgBufferW*& hb) {
    body.putObjType(version, otype);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        body.putRawLong32(0);
    // The object's OWN handle at the start of the object data must use code 0
    // (handle-holder), not a code-4 reference: a code-4 here desyncs the whole
    // object on read ("Invalid object handle ... @7.2"). The control objects via
    // emitControlObject already use makeOwnHandle; the table records did not.
    // (write-review / plan 3.6)
    body.putHandle(makeOwnHandle(handle));
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

void dwgWriter15::emitLtypeRecord(std::uint32_t handle, const DRW_LType& lt) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::LTYPE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)lt.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitLayerRecord(std::uint32_t handle, const DRW_Layer& lay) {
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
    (void)layerCopy.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitStyleRecord(std::uint32_t handle, const DRW_Textstyle& ts) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::STYLE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)ts.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitUcsRecord(std::uint32_t handle, const DRW_UCS& ucs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::UCS, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)ucs.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitViewRecord(std::uint32_t handle, const DRW_View& view) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::VIEW, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)view.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitVportRecord(std::uint32_t handle, const DRW_Vport& vp) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::VPORT, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)vp.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitAppIdRecord(std::uint32_t handle, const DRW_AppId& ai) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::APPID, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)ai.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitDimstyleRecord(std::uint32_t handle, const DRW_Dimstyle& ds) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, oType::DIMSTYLE, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)ds.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

void dwgWriter15::emitAcDbPlaceholderObject(
    std::uint32_t handle, const DRW_AcDbPlaceholder& placeholder) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 80, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)placeholder.encodeDwg(m_version, &body, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeAcDbPlaceholder(
    const DRW_AcDbPlaceholder& placeholder) {
    // ACDBPLACEHOLDER (ODA fixed type 80) is universally available since
    // R2000.  Encoder is version-clean — only the standard string/handle
    // split-buffer routing on `version > AC1018` (no AC1018+-only fields).
    // PR 13d broadened the writer gate from AC1021+ to AC1015+ in step
    // with the filter-side `canWriteFixedTypeObjects` dispatch.
    if (m_version < DRW::AC1015)
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

void dwgWriter15::emitSunObject(std::uint32_t handle, const DRW_Sun& sun) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_Sun::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)sun.encodeDwg(m_version, &body, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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

void dwgWriter15::emitMLeaderStyleObject(std::uint32_t handle,
                                         const DRW_MLeaderStyle& style) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_MLeaderStyle::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)style.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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
void dwgWriter15::emitDictionaryObject(std::uint32_t handle,
                                       const DRW_Dictionary& dictionary) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 42, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)dictionary.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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
void dwgWriter15::emitXRecordObject(std::uint32_t handle,
                                    const DRW_XRecord& xrecord) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 79, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)xrecord.encodeDwg(m_version, &body, nullptr, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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
void dwgWriter15::emitLayoutObject(std::uint32_t handle, const DRW_Layout& layout) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 82, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)layout.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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
void dwgWriter15::emitGroupObject(std::uint32_t handle, const DRW_Group& group) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 72, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)group.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

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

// MLINESTYLE (fixed object type 73) needs no CLASS registration. Record the
// emitted handle by style name so later MLINE entities can point at it.
void dwgWriter15::emitMLineStyleObject(std::uint32_t handle,
                                       const DRW_MLineStyle& style) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, 73, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)style.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeMLineStyle(const DRW_MLineStyle& style) {
    DRW_MLineStyle object = style;
    const std::string upper = toUpperCase(object.name);
    auto existing = m_writingCtx.mlineStyleMap.find(upper);
    if (!upper.empty() && existing != m_writingCtx.mlineStyleMap.end()) {
        if (object.handle == 0 || object.handle == existing->second)
            return true;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    for (DRW_MLineElement& element : object.elements) {
        if (element.linetypeHandle != 0 || element.linetype.empty())
            continue;
        auto lineTypeIt = m_writingCtx.ltypeMap.find(toUpperCase(element.linetype));
        if (lineTypeIt != m_writingCtx.ltypeMap.end())
            element.linetypeHandle = lineTypeIt->second;
    }
    if (!upper.empty())
        m_writingCtx.mlineStyleMap[upper] = object.handle;
    emitMLineStyleObject(object.handle, object);
    return true;
}

// RASTERVARIABLES (AcDbRasterVariables, custom class 505) — class
// registration required so the reader's CLASSES section dispatch can
// resolve the recordName back to DRW_RasterVariables::parseDwg. Standard
// preamble + DRW_RasterVariables::encodeDwg sandwich. Encoder ignores
// strBuf (no string fields), so pass nullptr to mirror parse semantics.
void dwgWriter15::emitRasterVariablesObject(
    std::uint32_t handle, const DRW_RasterVariables& rasterVariables) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version,
                       DRW_RasterVariables::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    if (!rasterVariables.encodeDwg(m_version, &body, nullptr, hb))
        return;

    finishObject();
}

bool dwgWriter15::writeRasterVariables(
    const DRW_RasterVariables& rasterVariables) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).  Parser mirrors the same shape.  The matching class
    // registration in writeDwgClasses now gates on
    // canRegisterCustomClassObjects (≥AC1015).
    if (m_version < DRW::AC1015)
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

// WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class 529). Body has one
// display-frame flag, then the common handle prefix in the object handle
// stream.
void dwgWriter15::emitWipeoutVariablesObject(
    std::uint32_t handle, const DRW_WipeoutVariables& wipeoutVariables) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version,
                       DRW_WipeoutVariables::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    if (!wipeoutVariables.encodeDwg(m_version, &body, nullptr, hb))
        return;

    finishObject();
}

bool dwgWriter15::writeWipeoutVariables(
    const DRW_WipeoutVariables& wipeoutVariables) {
    if (m_version < DRW::AC1015)
        return false;

    DRW_WipeoutVariables object = wipeoutVariables;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerWipeoutVariablesObjectClass(object.handle))
        return false;

    emitWipeoutVariablesObject(object.handle, object);
    return true;
}

// IMAGEDEF (fixed type 102 / dwgObjType::IMAGEDEF) — no CLASSES entry.
// Inverts DRW_ImageDef::parseDwg; handle is assigned when the caller left it 0.
bool dwgWriter15::writeImageDef(DRW_ImageDef& imageDef) {
    if (imageDef.handle == 0) {
        imageDef.handle = m_handles.next();
    } else {
        m_handles.reserve(imageDef.handle);
    }
    dwgBufferW& body = beginObject(imageDef.handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, /*typeCode=*/102, imageDef.handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    if (!imageDef.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return true;
}

// IMAGEDEF_REACTOR (custom class 532). Body is class_version + common handles.
bool dwgWriter15::writeImageDefinitionReactor(
    DRW_ImageDefinitionReactor& reactor) {
    if (reactor.handle == 0) {
        reactor.handle = m_handles.next();
    } else {
        m_handles.reserve(reactor.handle);
    }
    if (!registerImageDefReactorObjectClass(reactor.handle))
        return false;
    dwgBufferW& body = beginObject(reactor.handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_ImageDefinitionReactor::kDwgClassNum,
                       reactor.handle, m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    if (!reactor.encodeDwg(m_version, &body, nullptr, hb))
        return false;
    finishObject();
    return true;
}

// GEODATA (AcDbGeoData, custom class 506) - class registration required.
// Standard preamble + DRW_GeoData::encodeDwg sandwich. Encoder writes its own
// common-handle prefix into hb (the preamble does not) plus several
// variable-text fields into sb.
void dwgWriter15::emitGeoDataObject(std::uint32_t handle, const DRW_GeoData& geoData) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_GeoData::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    if (!geoData.encodeDwg(m_version, &body, sb, hb))
        return;

    finishObject();
}

bool dwgWriter15::writeGeoData(const DRW_GeoData& geoData) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015)
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

// SPATIAL_FILTER (AcDbSpatialFilter, custom class 507) — class registration
// required.  Standard preamble + DRW_SpatialFilter::encodeDwg sandwich.
// Encoder writes its own common-handle prefix into hb; body fields are
// pure numeric (no strings), so strBuf is unused.
void dwgWriter15::emitSpatialFilterObject(std::uint32_t handle,
                                          const DRW_SpatialFilter& filter) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version,
                       DRW_SpatialFilter::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    if (!filter.encodeDwg(m_version, &body, nullptr, hb))
        return;

    finishObject();
}

bool dwgWriter15::writeSpatialFilter(const DRW_SpatialFilter& filter) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015)
        return false;

    DRW_SpatialFilter object = filter;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSpatialFilterObjectClass(object.handle))
        return false;

    emitSpatialFilterObject(object.handle, object);
    return true;
}

// SCALE (AcDbScale, custom class 508) — class registration required.  The
// SCALE encoder writes body fields ONLY (no common handle prefix and no
// type-specific tail handles, matching its parser which intentionally leaves
// the trailing handle stream "to the caller").  The wrapper therefore emits
// the common handle prefix (parentHandle + reactors + xdic) itself after
// encodeDwg returns.  This is the inverse of the usual pattern.  PR 8d.2a.
void dwgWriter15::emitScaleObject(std::uint32_t handle, const DRW_Scale& scale) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_Scale::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)scale.encodeDwg(m_version, &body, sb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    // Emit the common handle prefix that the SCALE encoder skips.  emitRecordPreamble
    // wrote the data-stream numReactors as a hardcoded 0 (like every object on this
    // write path — LibreCAD does not preserve reactor lists), so the handle stream
    // must emit ZERO reactor handles to match.  Emitting scale.numReactors here
    // desynced the stream: the reader parses numReactors=0 and never consumes them,
    // shifting the xdic / following handle on a round-tripped SCALE with reactors.
    hb->putHandle(makeSoftOwner(static_cast<std::uint32_t>(scale.parentHandle)));
    if (scale.xDictFlag != 1)
        hb->putHandle(makeSoftOwner(0));
    finishObject();
}

bool dwgWriter15::writeScale(const DRW_Scale& scale) {
    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (body-only emit; common handle prefix written by
    // emitScaleObject itself).
    if (m_version < DRW::AC1015)
        return false;

    DRW_Scale object = scale;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerScaleObjectClass(object.handle))
        return false;

    emitScaleObject(object.handle, object);
    return true;
}

// IDBUFFER (AcDbIdBuffer, custom class 509) — class registration required.
// Standard preamble + DRW_IDBuffer::encodeDwg sandwich.  Encoder owns its
// common-handle prefix + object-id handles in hb.  PR 8d.2a.
void dwgWriter15::emitIDBufferObject(std::uint32_t handle, const DRW_IDBuffer& idBuffer) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_IDBuffer::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)idBuffer.encodeDwg(m_version, &body, nullptr, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeIDBuffer(const DRW_IDBuffer& idBuffer) {
    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).
    if (m_version < DRW::AC1015)
        return false;

    DRW_IDBuffer object = idBuffer;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerIDBufferObjectClass(object.handle))
        return false;

    emitIDBufferObject(object.handle, object);
    return true;
}

// LAYER_INDEX (AcDbLayerIndex, custom class 510) — class registration
// required.  Standard preamble + DRW_LayerIndex::encodeDwg sandwich.  Encoder
// writes entry names through sb (TV); per-entry handles + common prefix go
// to hb.  PR 8d.2a.
void dwgWriter15::emitLayerIndexObject(std::uint32_t handle,
                                       const DRW_LayerIndex& layerIndex) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_LayerIndex::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)layerIndex.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeLayerIndex(const DRW_LayerIndex& layerIndex) {
    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
    if (m_version < DRW::AC1015)
        return false;

    DRW_LayerIndex object = layerIndex;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerLayerIndexObjectClass(object.handle))
        return false;

    emitLayerIndexObject(object.handle, object);
    return true;
}

// SPATIAL_INDEX (AcDbSpatialIndex, custom class 511) — class registration
// required.  Standard preamble + DRW_SpatialIndex::encodeDwg sandwich.
// Encoder writes timestamps to the body and the common-handle prefix to hb
// (the latter only at R2007+; gated to AC1021+ anyway).  PR 8d.2a.
void dwgWriter15::emitSpatialIndexObject(std::uint32_t handle,
                                         const DRW_SpatialIndex& spatialIndex) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_SpatialIndex::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)spatialIndex.encodeDwg(m_version, &body, nullptr, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeSpatialIndex(const DRW_SpatialIndex& spatialIndex) {
    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder gates the
    // common handle prefix on `version > AC1018` (parser does the same),
    // so at AC1015/AC1018 the wrapper emits an opaque body and no handle
    // tail — matching the parser's expectation.
    if (m_version < DRW::AC1015)
        return false;

    DRW_SpatialIndex object = spatialIndex;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSpatialIndexObjectClass(object.handle))
        return false;

    emitSpatialIndexObject(object.handle, object);
    return true;
}

// DICTIONARYVAR (AcDbDictionaryVar, custom class 512) — class registration
// required.  Standard preamble + DRW_DictionaryVar::encodeDwg sandwich.
// Encoder owns its common-handle prefix; the m_value string goes through
// sb at AC1018+.  PR 8d.2a.
void dwgWriter15::emitDictionaryVarObject(std::uint32_t handle,
                                          const DRW_DictionaryVar& dictionaryVar) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_DictionaryVar::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)dictionaryVar.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeDictionaryVar(const DRW_DictionaryVar& dictionaryVar) {
    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
    if (m_version < DRW::AC1015)
        return false;

    DRW_DictionaryVar object = dictionaryVar;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerDictionaryVarObjectClass(object.handle))
        return false;

    emitDictionaryVarObject(object.handle, object);
    return true;
}

// DICTIONARYWDFLT (AcDbDictionaryWithDefault, custom class 513) — class
// registration required.  The encoder delegates to
// DRW_Dictionary::encodeDwg for the body + per-entry handle list, then
// appends the single default-entry handle at the tail (mirroring its
// parser).  Standard preamble + encoder sandwich.  PR 8d.2b.
void dwgWriter15::emitDictionaryWithDefaultObject(std::uint32_t handle,
                                                   const DRW_DictionaryWithDefault& dictionary) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version,
                       DRW_DictionaryWithDefault::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)dictionary.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeDictionaryWithDefault(const DRW_DictionaryWithDefault& dictionary) {
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (delegates to DRW_Dictionary::encodeDwg, then
    // appends a single default-entry hard pointer at the tail).
    if (m_version < DRW::AC1015)
        return false;

    DRW_DictionaryWithDefault object = dictionary;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerDictionaryWithDefaultObjectClass(object.handle))
        return false;

    emitDictionaryWithDefaultObject(object.handle, object);
    return true;
}

// SORTENTSTABLE (AcDbSortentsTable, custom class 514) — class registration
// required.  Standard preamble + DRW_SortEntsTable::encodeDwg sandwich.
// Encoder inverts the usual "all handles in handle stream" convention: per-
// entry sort handles go inline in the body section BEFORE the common
// prefix; block-owner + entity handles follow in the handle stream.
// PR 8d.2b.
void dwgWriter15::emitSortEntsTableObject(std::uint32_t handle,
                                           const DRW_SortEntsTable& sortEntsTable) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_SortEntsTable::kDwgClassNum,
                       handle, m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)sortEntsTable.encodeDwg(m_version, &body, nullptr, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeSortEntsTable(const DRW_SortEntsTable& sortEntsTable) {
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (inline sort handles in body, then standard hb =
    // version > AC1018 split-buffer routing for the common prefix +
    // block-owner + entity handles).
    if (m_version < DRW::AC1015)
        return false;

    DRW_SortEntsTable object = sortEntsTable;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSortEntsTableObjectClass(object.handle))
        return false;

    emitSortEntsTableObject(object.handle, object);
    return true;
}

// FIELDLIST (AcDbFieldList, custom class 515) — class registration
// required.  Standard preamble + DRW_FieldList::encodeDwg sandwich.
// Encoder owns common-handle prefix + per-field handle list in hb.  No
// strings.  PR 8d.2b.
void dwgWriter15::emitFieldListObject(std::uint32_t handle, const DRW_FieldList& fieldList) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_FieldList::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    DRW_UNUSED(sb);
    (void)fieldList.encodeDwg(m_version, &body, nullptr, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeFieldList(const DRW_FieldList& fieldList) {
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (no strings; only the standard hb = version > AC1018
    // split-buffer routing).
    if (m_version < DRW::AC1015)
        return false;

    DRW_FieldList object = fieldList;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerFieldListObjectClass(object.handle))
        return false;

    emitFieldListObject(object.handle, object);
    return true;
}

// FIELD (AcDbField, custom class 516) — class registration required.
// Standard preamble + DRW_Field::encodeDwg sandwich.  Encoder writes
// evaluator/code/format/messages through sb (TV), CadValue + child values
// through buf+sb, and common-prefix + child handles + object handles
// inline through hb.  PR 8d.2b.
void dwgWriter15::emitFieldObject(std::uint32_t handle, const DRW_Field& field) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, DRW_Field::kDwgClassNum, handle,
                       m_objectStrings, m_objectHandles, sb, hb);
    (void)field.encodeDwg(m_version, &body, sb, hb);  // (void): void emitter; object serializes into in-memory buffer (no failure path)

    finishObject();
}

bool dwgWriter15::writeField(const DRW_Field& field) {
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder has a
    // `version < AC1021` branch for m_formatString, mirrored in the
    // parser, so the pre-R2007 path is exercised; nested CadValue /
    // child values go through writeCadValue/readCadValue (also
    // version-clean).
    if (m_version < DRW::AC1015)
        return false;

    DRW_Field object = field;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerFieldObjectClass(object.handle))
        return false;

    emitFieldObject(object.handle, object);
    return true;
}

void dwgWriter15::emitUnderlayDefinitionObject(
    std::uint32_t handle, const DRW_UnderlayDefinition& definition) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    emitRecordPreamble(body, m_version, underlayDefinitionClassNum(definition),
                       handle, m_objectStrings, m_objectHandles, sb, hb);
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return;

    finishObject();
}

bool dwgWriter15::writeUnderlayDefinition(
    const DRW_UnderlayDefinition& definition) {
    if (m_version < DRW::AC1015)
        return false;

    DRW_UnderlayDefinition object = definition;
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerUnderlayDefinitionObjectClass(object.kind, object.handle))
        return false;

    emitUnderlayDefinitionObject(object.handle, object);
    return true;
}

// --- add*() methods: register user table records for deferred emission -----

void dwgWriter15::addLType(const DRW_LType& lt) {
    std::string upper = toUpperCase(lt.name);
    if (upper.empty() || m_writingCtx.ltypeMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.ltypeMap[upper] = h;
    m_pendingLTypes.emplace_back(h, lt);
}

void dwgWriter15::addLayer(const DRW_Layer& lay) {
    std::string upper = toUpperCase(lay.name);
    if (upper == "0") {
        // "0" is the reserved layer at fixed handle 0x12; keep its real data
        // (plot flag, color, linetype, ...) so it is emitted faithfully rather
        // than as a default stub. Do not add it to m_pendingLayers (avoids a
        // duplicate record at a second handle).
        m_layer0 = lay;
        m_haveLayer0 = true;
        return;
    }
    if (upper.empty() || m_writingCtx.layerMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.layerMap[upper] = h;
    m_pendingLayers.emplace_back(h, lay);
}

void dwgWriter15::addTextstyle(const DRW_Textstyle& ts) {
    std::string upper = toUpperCase(ts.name);
    if (upper.empty() || m_writingCtx.styleMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.styleMap[upper] = h;
    m_pendingStyles.emplace_back(h, ts);
}

void dwgWriter15::addUcs(const DRW_UCS& ucs) {
    std::string upper = toUpperCase(ucs.name);
    if (upper.empty() || m_writingCtx.ucsMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.ucsMap[upper] = h;
    m_pendingUcs.emplace_back(h, ucs);
}

void dwgWriter15::addView(const DRW_View& view) {
    std::string upper = toUpperCase(view.name);
    if (upper.empty() || m_writingCtx.viewMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.viewMap[upper] = h;
    m_pendingViews.emplace_back(h, view);
}

void dwgWriter15::addVport(const DRW_Vport& vp) {
    std::string upper = toUpperCase(vp.name);
    if (upper.empty() || m_writingCtx.vportMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.vportMap[upper] = h;
    m_pendingVports.emplace_back(h, vp);
}

void dwgWriter15::addDimstyle(const DRW_Dimstyle& ds) {
    std::string upper = toUpperCase(ds.name);
    if (upper.empty() || m_writingCtx.dimstyleMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.dimstyleMap[upper] = h;
    m_pendingDimstyles.emplace_back(h, ds);
}

void dwgWriter15::addAppId(const DRW_AppId& ai) {
    std::string upper = toUpperCase(ai.name);
    if (upper.empty() || m_writingCtx.appidMap.count(upper)) return;
    std::uint32_t h = m_handles.next();
    m_writingCtx.appidMap[upper] = h;
    m_pendingAppIds.emplace_back(h, ai);
}

void dwgWriter15::prepareBlockOwnedEntity(DRW_Entity& entity) {
    if (auto* insert = dynamic_cast<DRW_Insert*>(&entity)) {
        if (insert->blockRecH.ref == 0 && !insert->name.empty()) {
            const auto block = m_userBlockHandles.find(toUpperCase(insert->name));
            if (block != m_userBlockHandles.end())
                insert->blockRecH.ref = block->second;
        }
    }
    if (m_activeUserBlockRecordHandle != 0)
        entity.parentHandle = m_activeUserBlockRecordHandle;
}

void dwgWriter15::recordBlockOwnedEntity(std::uint32_t entityHandle) {
    if (m_activeUserBlockRecordHandle == 0)
        return;
    for (PendingUserBlock& block : m_userBlocks) {
        if (block.blockRecordHandle == m_activeUserBlockRecordHandle) {
            block.entityHandles.push_back(entityHandle);
            return;
        }
    }
}

bool dwgWriter15::encodeEntity(DRW_Entity *ent) {
    if (ent == nullptr) return false;
    // Honor caller-set handles (round-trip preservation); allocate a
    // fresh one only when ent->handle is zero.  Either way, reserve
    // the handle in the allocator so a later next() can't return it
    // for a different entity.
    std::uint32_t handle = ent->handle;
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
            : static_cast<std::uint32_t>(0x12);  // fallback: layer "0"
    }
    // Resolve linetype name → handle when caller specified a non-empty name.
    if (ent->lTypeH.ref == 0 && !ent->lineType.empty()) {
        auto ltUp = toUpperCase(ent->lineType);
        auto it = m_writingCtx.ltypeMap.find(ltUp);
        if (it != m_writingCtx.ltypeMap.end())
            ent->lTypeH.ref = it->second;
    }
    // Drive ltFlags from resolved name/handle so encodeDwgCommon emits the
    // correct BB and encodeDwgEntHandle emits lTypeH when needed.
    if (ent->ltFlags == 0) {
        const auto ltUp = toUpperCase(ent->lineType);
        if (ent->lTypeH.ref != 0
            && ltUp != "BYLAYER" && ltUp != "BYBLOCK" && ltUp != "CONTINUOUS"
            && !ltUp.empty())
            ent->ltFlags = 3;
        else if (ltUp == "BYBLOCK")
            ent->ltFlags = 1;
        else if (ltUp == "CONTINUOUS")
            ent->ltFlags = 2;
    }
    if (ent->eType == DRW::MLINE) {
        auto *mline = dynamic_cast<DRW_MLine *>(ent);
        if (mline != nullptr && mline->styleHandle == 0 && !mline->styleName.empty()) {
            auto styleIt = m_writingCtx.mlineStyleMap.find(toUpperCase(mline->styleName));
            if (styleIt != m_writingCtx.mlineStyleMap.end())
                mline->styleHandle = styleIt->second;
        }
    }
    prepareBlockOwnedEntity(*ent);

    dwgBufferW& body = beginObject(handle);
    bool ok = ent->encodeDwg(m_version, &body, /*bs=*/0);
    if (!ok) return false;
    finishObject();

    recordBlockOwnedEntity(handle);
    return true;
}

bool dwgWriter15::writeDwgObjects() {
    // Ensure standard reserved entries are present.  The constructor
    // pre-seeds these; insert is a no-op if a key already exists, which
    // protects user entries added via add*() before this call.
    m_writingCtx.ltypeMap.insert({"CONTINUOUS", reservedHandle::LTYPE_CONTINUOUS});
    m_writingCtx.ltypeMap.insert({"BYLAYER",    reservedHandle::LTYPE_BYLAYER});
    m_writingCtx.ltypeMap.insert({"BYBLOCK",    reservedHandle::LTYPE_BYBLOCK});
    m_writingCtx.layerMap.insert({"0",        static_cast<std::uint32_t>(0x12)});
    m_writingCtx.styleMap.insert({"STANDARD", static_cast<std::uint32_t>(0x13)});
    // Named VIEW has no required standard record.
    m_writingCtx.appidMap.insert({"ACAD",     static_cast<std::uint32_t>(0x14)});
    m_writingCtx.dimstyleMap.insert({"STANDARD", static_cast<std::uint32_t>(0x15)});
    m_writingCtx.vportMap.insert({"*ACTIVE",  static_cast<std::uint32_t>(0x16)});

    // --- LTYPE section --- (emitted before LAYER so ltypeMap is populated
    // when emitLayerRecord resolves lineType→handle)
    {
        std::vector<std::uint32_t> ltypeChildren = {
            reservedHandle::LTYPE_BYBLOCK,
            reservedHandle::LTYPE_BYLAYER,
            reservedHandle::LTYPE_CONTINUOUS
        };
        for (auto& p : m_pendingLTypes) ltypeChildren.push_back(p.first);
        // numEntries excludes BYBLOCK/BYLAYER phantoms (reader adds +2)
        emitControlObject(oType::LTYPE_CONTROL, reservedHandle::LTYPE_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingLTypes.size()),
                          ltypeChildren);
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_BYBLOCK,    "BYBLOCK");
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_BYLAYER,    "BYLAYER");
        emitTableRecord(oType::LTYPE, reservedHandle::LTYPE_CONTINUOUS, "CONTINUOUS");
        for (auto& p : m_pendingLTypes)
            emitLtypeRecord(p.first, p.second);
    }

    // --- LAYER section ---
    {
        std::vector<std::uint32_t> layerChildren = {0x12};
        for (auto& p : m_pendingLayers) layerChildren.push_back(p.first);
        emitControlObject(oType::LAYER_CONTROL, reservedHandle::LAYER_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingLayers.size()),
                          layerChildren);
        if (m_haveLayer0)
            emitLayerRecord(0x12, m_layer0);   // faithful "0" (plot flag, color, ...)
        else
            emitTableRecord(oType::LAYER, 0x12, "0");  // fallback stub
        for (auto& p : m_pendingLayers)
            emitLayerRecord(p.first, p.second);
    }

    // --- STYLE section ---
    {
        std::vector<std::uint32_t> styleChildren = {0x13};
        for (auto& p : m_pendingStyles) styleChildren.push_back(p.first);
        emitControlObject(oType::STYLE_CONTROL, reservedHandle::STYLE_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingStyles.size()),
                          styleChildren);
        emitTableRecord(oType::STYLE, 0x13, "STANDARD");
        for (auto& p : m_pendingStyles)
            emitStyleRecord(p.first, p.second);
    }

    // --- VIEW section --- (named views; no required standard record)
    {
        std::vector<std::uint32_t> viewChildren;
        for (auto& p : m_pendingViews) viewChildren.push_back(p.first);
        emitControlObject(oType::VIEW_CONTROL, reservedHandle::VIEW_CONTROL,
                          static_cast<std::uint32_t>(m_pendingViews.size()),
                          viewChildren);
        for (auto& p : m_pendingViews)
            emitViewRecord(p.first, p.second);
    }

    // --- UCS section --- (named UCS records; no required standard record)
    {
        std::vector<std::uint32_t> ucsChildren;
        for (auto& p : m_pendingUcs) ucsChildren.push_back(p.first);
        emitControlObject(oType::UCS_CONTROL, reservedHandle::UCS_CONTROL,
                          static_cast<std::uint32_t>(m_pendingUcs.size()),
                          ucsChildren);
        for (auto& p : m_pendingUcs)
            emitUcsRecord(p.first, p.second);
    }

    // --- VPORT section ---
    {
        std::vector<std::uint32_t> vportChildren = {0x16};
        for (auto& p : m_pendingVports) vportChildren.push_back(p.first);
        emitControlObject(oType::VPORT_CONTROL, reservedHandle::VPORT_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingVports.size()),
                          vportChildren);
        emitTableRecord(oType::VPORT, 0x16, "*ACTIVE");
        for (auto& p : m_pendingVports)
            emitVportRecord(p.first, p.second);
    }

    // --- APPID section ---
    {
        std::vector<std::uint32_t> appidChildren = {0x14};
        for (auto& p : m_pendingAppIds) appidChildren.push_back(p.first);
        emitControlObject(oType::APPID_CONTROL, reservedHandle::APPID_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingAppIds.size()),
                          appidChildren);
        emitTableRecord(oType::APPID, 0x14, "ACAD");
        for (auto& p : m_pendingAppIds)
            emitAppIdRecord(p.first, p.second);
    }

    // --- DIMSTYLE section ---
    {
        std::vector<std::uint32_t> dimChildren = {0x15};
        for (auto& p : m_pendingDimstyles) dimChildren.push_back(p.first);
        emitControlObject(oType::DIMSTYLE_CONTROL, reservedHandle::DIMSTYLE_CONTROL,
                          1 + static_cast<std::uint32_t>(m_pendingDimstyles.size()),
                          dimChildren);
        emitTableRecord(oType::DIMSTYLE, 0x15, "STANDARD");
        for (auto& p : m_pendingDimstyles)
            emitDimstyleRecord(p.first, p.second);
    }

    // VPORT_ENTITY_HEADER_CONTROL: no entries.
    emitControlObject(oType::VPORT_ENTITY_HEADER_CONTROL,
                      reservedHandle::VPORT_ENTITY_HEADER_CONTROL, 0, {});

    // Block entities + Block_Records for *Model_Space and *Paper_Space.
    constexpr std::uint32_t blkModelStart = 0x1B;
    constexpr std::uint32_t blkModelEnd   = 0x1C;
    constexpr std::uint32_t blkPaperStart = 0x1D;
    constexpr std::uint32_t blkPaperEnd   = 0x1E;

    emitBlockEntity(blkModelStart, "*Model_Space", /*isEnd=*/false);
    emitBlockEntity(blkModelEnd,   std::string{},   /*isEnd=*/true);
    emitBlockEntity(blkPaperStart, "*Paper_Space", /*isEnd=*/false);
    emitBlockEntity(blkPaperEnd,   std::string{},   /*isEnd=*/true);

    const DRW_Coord origin{0.0, 0.0, 0.0};
    const std::vector<std::uint32_t> noEntities;
    emitBlockRecord(reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space", origin,
                    blkModelStart, blkModelEnd, noEntities);
    emitBlockRecord(reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space", origin,
                    blkPaperStart, blkPaperEnd, noEntities);

    return true;
}

namespace {

/// Rebuild raw object body with a replacement OT when raw class nums remapped.
bool patchRawObjectType(const std::vector<std::uint8_t>& raw,
                        DRW::Version version,
                        std::uint16_t newType,
                        std::vector<std::uint8_t>& out) {
    if (raw.empty())
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (!reader.isGood())
        return false;

    dwgBufferW rebuilt;
    rebuilt.putObjType(version, newType);
    // Copy remaining bits after the original OT.
    while (reader.numRemainingBytes() > 0 || reader.getBitPos() != 0) {
        // Prefer whole remaining bytes when aligned.
        if (reader.getBitPos() == 0 && reader.numRemainingBytes() > 0) {
            const size_t n = reader.numRemainingBytes();
            std::vector<std::uint8_t> tail(n);
            if (!reader.getBytes(tail.data(), static_cast<int>(n)))
                return false;
            rebuilt.putBytes(tail.data(), n);
            break;
        }
        const std::uint8_t bit = reader.getBit();
        if (!reader.isGood())
            return false;
        rebuilt.putBit(bit);
    }
    rebuilt.alignToByte();
    out = rebuilt.data();
    return !out.empty();
}

} // namespace

bool dwgWriter15::replayRawObject(const DRW_UnsupportedObject& object) {
    if (object.m_version != m_version
        || (object.m_isEntity && !isReplayableFixedModelerRawEntity(object))
        || object.m_handle == 0 || object.m_rawBytes.empty()) {
        return false;
    }

    m_handles.reserve(object.m_handle);
    if (!registerRawObjectClass(object))
        return false;

    const std::uint16_t sourceType =
        static_cast<std::uint16_t>(object.m_objectType);
    const std::uint16_t writerType = remappedRawClassNum(sourceType);
    const std::vector<std::uint8_t>* bodyBytes = &object.m_rawBytes;
    std::vector<std::uint8_t> patched;
    if (object.m_isCustomClass && sourceType >= 500 && writerType != sourceType) {
        if (!patchRawObjectType(object.m_rawBytes, m_version, writerType, patched))
            return false;
        bodyBytes = &patched;
    }

    const std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes->size()));
    if (m_version > DRW::AC1021)
        m_buf.putUModularChar(object.m_bodyBitSize);
    const size_t bodyStart = m_buf.size();
    m_buf.putBytes(bodyBytes->data(), bodyBytes->size());

    const std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                                    bodyStart + bodyBytes->size());
    m_buf.putRawShort16(crc);
    m_objectMap.emplace_back(object.m_handle, frameStart);
    return true;
}

dwgBufferW& dwgWriter15::beginObject(std::uint32_t handle) {
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
    std::uint32_t bitCount = static_cast<std::uint32_t>(m_objectBody.size()) * 8;
    if (m_objectBody.bitPos() != 0)
        bitCount -= static_cast<std::uint32_t>(8 - m_objectBody.bitPos());
    if (!m_objectBody.data().empty()) {
        std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
        size_t rlBitOffset = (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        m_objectBody.patchRawLong32AtBit(rlBitOffset, bitCount);
    }

    // Byte-align the body — the trailing CRC must start aligned and
    // the per-object frame size must be in whole bytes.
    m_objectBody.alignToByte();
    std::uint32_t bodyBytes = static_cast<std::uint32_t>(m_objectBody.size());

    std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());

    // MS objectSize = byte count of body (no CRC).  Per the master plan
    // interpretation (b): CRC follows immediately after the slurped body
    // and is not included in `size`.
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes));
    size_t bodyStartOffset = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), bodyBytes);

    // CRC covers MS prefix + body — spec §20.2: "The CRC includes the size
    // bytes."  frameStart was recorded before putModularShort.
    std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                              bodyStartOffset + bodyBytes);
    m_buf.putRawShort16(crc);

    m_objectMap.emplace_back(m_currentHandle, frameStart);
    m_currentHandle = 0;
}

bool dwgWriter15::writeDwgHandles() {
    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::HANDLES] = static_cast<std::uint32_t>(sectionStart);

    if (m_objectMap.empty()) {
        // Empty object map: single terminator page of `RS_BE(2) + CRC16_BE`.
        // size_BE = 2 means "this page contains 2 bytes including the size word".
        std::uint16_t pageSize = 2;
        m_buf.putBERawShort16(pageSize);
        std::uint16_t crc = m_buf.crc16(0xC0C1, sectionStart, m_buf.size());
        m_buf.putBERawShort16(crc);

        m_sectionSizes[recno::HANDLES] =
            static_cast<std::uint32_t>(m_buf.size() - sectionStart);
        return true;
    }

    // Sort by handle for monotonic UMC deltas.  Per the master plan
    // (LibreDWG insertion-sort at encode.c:2394-2406): in practice the
    // map is already monotonic because objects are emitted in handle
    // order, but the sort defends against future out-of-order emit.
    std::sort(m_objectMap.begin(), m_objectMap.end());
    for (size_t i = 1; i < m_objectMap.size(); ++i) {
        if (m_objectMap[i - 1].first == m_objectMap[i].first)
            return false;
    }

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

        std::uint32_t prevHandle = 0;
        std::uint32_t prevOffset = 0;
        std::uint32_t base = objectBaseOffset();
        size_t entryEnd = entryStart;
        while (entryEnd < m_objectMap.size()) {
            std::uint32_t h = m_objectMap[entryEnd].first;
            std::uint32_t off = m_objectMap[entryEnd].second - base;

            // Cheap upper-bound check: UMC + MC are each ≤5 bytes; if
            // adding 10 bytes would overflow the page, close it now.
            // Tightens if the actual encoded widths are smaller, but
            // never overshoots.
            if ((m_buf.size() - pageStart) + 10 > kMaxPagePayloadBytes)
                break;

            m_buf.putUModularChar(h - prevHandle);
            m_buf.putModularChar(static_cast<std::int32_t>(off - prevOffset));
            prevHandle = h;
            prevOffset = off;
            ++entryEnd;
        }

        // Patch the page size (BE) — overwrite the placeholder we wrote
        // at pageStart at the top of the loop.  Note: patchRawShort16
        // emits LE, so we write the BE bytes directly here.
        std::uint16_t pageSize = static_cast<std::uint16_t>(m_buf.size() - pageStart);
        m_buf.data()[pageStart]     = static_cast<std::uint8_t>((pageSize >> 8) & 0xFF);
        m_buf.data()[pageStart + 1] = static_cast<std::uint8_t>(pageSize & 0xFF);

        // CRC16 BE over (size + entries).
        std::uint16_t crc = m_buf.crc16(0xC0C1, pageStart, m_buf.size());
        m_buf.putBERawShort16(crc);

        entryStart = entryEnd;
    }

    // Terminator page (2-byte size word + CRC of just the size).
    size_t termStart = m_buf.size();
    std::uint16_t termSize = 2;
    m_buf.putBERawShort16(termSize);
    std::uint16_t termCrc = m_buf.crc16(0xC0C1, termStart, m_buf.size());
    m_buf.putBERawShort16(termCrc);

    m_sectionSizes[recno::HANDLES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeSecondHeader() {
    const std::vector<std::uint8_t> auxHeader = buildR2000AuxHeaderContent(m_header);
    m_sectionOffsets[recno::AUXHEADER] = static_cast<std::uint32_t>(m_buf.size());
    if (!auxHeader.empty())
        m_buf.putBytes(auxHeader.data(), auxHeader.size());
    m_sectionSizes[recno::AUXHEADER] = static_cast<std::uint32_t>(auxHeader.size());
    return true;
}

bool dwgWriter15::finalize() {
    // Patch each section record in the file-header locator table.
    // Each record is 9 bytes: RC recno @ +0, RL address @ +1, RL size @ +5.
    for (std::uint8_t recno = 0; recno < m_numSections; ++recno) {
        auto offIt  = m_sectionOffsets.find(recno);
        auto sizeIt = m_sectionSizes.find(recno);
        std::uint32_t address = (offIt  != m_sectionOffsets.end()) ? offIt->second  : 0;
        std::uint32_t size    = (sizeIt != m_sectionSizes.end())   ? sizeIt->second : 0;
        size_t base = m_recordsOffset + static_cast<size_t>(recno) * 9;
        m_buf.patchRawLong32(base + 1, address);
        m_buf.patchRawLong32(base + 5, size);
    }

    // Recompute file-header CRC over bytes [0 .. m_recordsOffset + 9N)
    // with seed=0, then XOR'd by the per-count constant, then stored at
    // (m_recordsOffset + 9N).
    size_t crcOffset = m_recordsOffset + static_cast<size_t>(m_numSections) * 9;
    std::uint16_t crc = m_buf.crc16(0, 0, crcOffset);
    crc = static_cast<std::uint16_t>(crc ^ seedXorForCount(m_numSections));
    m_buf.patchRawShort16(crcOffset, crc);

    // Flush the accumulator to disk in one write().
    if (m_stream == nullptr || !m_stream->good()) return false;
    const std::vector<std::uint8_t>& bytes = m_buf.data();
    m_stream->write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    return m_stream->good();
}
