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
#include <cstdio>
#include <cstring>
#include <limits>

#include "../drw_base.h"
#include "../drw_entities.h"
#include "../drw_objects.h"
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "dwgbuffer.h"
#include "dwgbufferw.h"
#include "dwg_fixed_handles.h"
#include "dwgobjectframe.h"
#include "dwgreader.h"
#include "dwgsafety.h"
#include "dwgutil.h"

namespace {

bool isCompoundEntityChild(const DRW_Entity& entity) {
    return dynamic_cast<const DRW_Attrib*>(&entity) != nullptr
        || dynamic_cast<const DRW_SeqEnd*>(&entity) != nullptr
        || dynamic_cast<const DRW_Vertex*>(&entity) != nullptr;
}

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
    constexpr std::uint32_t BLOCK_CONTROL              = DRW::DwgBlockControlHandle;
    constexpr std::uint32_t LAYER_CONTROL              = DRW::DwgLayerControlHandle;
    constexpr std::uint32_t STYLE_CONTROL              = DRW::DwgStyleControlHandle;
    constexpr std::uint32_t LTYPE_CONTROL              = DRW::DwgLtypeControlHandle;
    constexpr std::uint32_t VIEW_CONTROL               = DRW::DwgViewControlHandle;
    constexpr std::uint32_t UCS_CONTROL                = DRW::DwgUcsControlHandle;
    constexpr std::uint32_t VPORT_CONTROL              =
        DRW::DwgVportControlHandle;
    constexpr std::uint32_t APPID_CONTROL              = DRW::DwgAppIdControlHandle;
    constexpr std::uint32_t DIMSTYLE_CONTROL           = DRW::DwgDimstyleControlHandle;
    constexpr std::uint32_t VPORT_ENTITY_HEADER_CONTROL =
        DRW::DwgViewportEntityHeaderControlHandle;
    // Reserved-block / linetype handles emitted in the HEADER section's
    // trailing 5-handle block (and as phantom entries in
    // BLOCK_CONTROL / LTYPE_CONTROL).  Phase 3e populates the records.
    constexpr std::uint32_t LTYPE_BYBLOCK = DRW::DwgLtypeByBlockHandle;
    constexpr std::uint32_t LTYPE_BYLAYER = DRW::DwgLtypeByLayerHandle;
    constexpr std::uint32_t LTYPE_CONTINUOUS = DRW::DwgLtypeContinuousHandle;
    constexpr std::uint32_t BLOCK_MODEL_SPACE = DRW::DwgModelSpaceBlockRecordHandle;
    constexpr std::uint32_t BLOCK_PAPER_SPACE = DRW::DwgPaperSpaceBlockRecordHandle;
    constexpr std::uint32_t BLOCK_MODEL_ENTITY = DRW::DwgModelSpaceBlockEntityHandle;
    constexpr std::uint32_t BLOCK_MODEL_END = DRW::DwgModelSpaceEndBlockEntityHandle;
    constexpr std::uint32_t BLOCK_PAPER_ENTITY = DRW::DwgPaperSpaceBlockEntityHandle;
    constexpr std::uint32_t BLOCK_PAPER_END = DRW::DwgPaperSpaceEndBlockEntityHandle;
}

bool controlEntryCountUsesBitShort(std::uint16_t objectType) {
    switch (objectType) {
    case DRW::DwgLTypeControlObjectType:
    case DRW::DwgUcsControlObjectType:
    case DRW::DwgVPortControlObjectType:
    case DRW::DwgAppIdControlObjectType:
    case DRW::DwgDimStyleControlObjectType:
        return true;
    default:
        return false;
    }
}

bool writeControlEntryCount(dwgBufferW& body, std::uint16_t objectType,
                            std::uint32_t count) {
    if (controlEntryCountUsesBitShort(objectType)) {
        if (count > std::numeric_limits<std::uint16_t>::max())
            return false;
        body.putBitShort(static_cast<std::uint16_t>(count));
    } else {
        if (count > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
            return false;
        body.putBitLong(static_cast<std::int32_t>(count));
    }
    return body.isGood();
}

bool isReplayableFixedModelerRawEntity(const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && (object.m_objectType == 34   // VIEWPORT (paper-space; no typed RS entity)
            || object.m_objectType == 37 || object.m_objectType == 38
            || object.m_objectType == 39);
}

bool isReplayableFixedEntityShellRawEntity(
    const DRW_UnsupportedObject& object) {
    return object.m_isEntity && !object.m_isCustomClass
        && DRW_UnsupportedObject::isFixedEntityShellType(object.m_objectType);
}

bool isReplayableSurfaceRawEntity(const DRW_UnsupportedObject& object) {
    if (!object.m_isEntity || !object.m_isCustomClass)
        return false;
    return object.m_recordName == "PLANESURFACE"
        || object.m_recordName == "EXTRUDEDSURFACE"
        || object.m_recordName == "REVOLVEDSURFACE"
        || object.m_recordName == "SWEPTSURFACE"
        || object.m_recordName == "LOFTEDSURFACE"
        || object.m_recordName == "NURBSURFACE";
}

bool isReplayableCustomRawEntity(const DRW_UnsupportedObject& object) {
    const std::uint32_t blockOwnerHandle =
        object.m_blockOwnerHandle != DRW::NoHandle
            ? object.m_blockOwnerHandle : object.m_parentHandle;
    return object.m_isEntity && object.m_isCustomClass
        && (blockOwnerHandle != DRW::NoHandle
            || isReplayableSurfaceRawEntity(object))
        && (!object.m_recordName.empty() || !object.m_className.empty());
}

std::uint32_t rawBlockOwnerHandle(const DRW_UnsupportedObject& object) {
    return object.m_blockOwnerHandle != DRW::NoHandle
        ? object.m_blockOwnerHandle : object.m_parentHandle;
}

std::uint16_t underlayDefinitionClassNum(
    const DRW_UnderlayDefinition& definition) {
    switch (definition.kind) {
    case DRW_UnderlayDefinition::DGN:
        return DRW_UnderlayDefinition::kDwgClassNumDgn;
    case DRW_UnderlayDefinition::DWF:
        return DRW_UnderlayDefinition::kDwgClassNumDwf;
    case DRW_UnderlayDefinition::PDF:
        return DRW_UnderlayDefinition::kDwgClassNumPdf;
    default:
        return 0;
    }
}

/// Build a soft-pointer (code 4) handle with the minimum-width ref.
dwgHandle makeSoftPointer(std::uint32_t ref) {
    dwgHandle h;
    h.code = 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

/// Build a soft-ownership (code 2) handle with the minimum-width ref.
dwgHandle makeSoftOwner(std::uint32_t ref) {
    dwgHandle h;
    h.code = 2;
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

/// Build a hard-ownership (code 3) handle — the form used for an
/// xdictionary and for objects that are required by their owner.
dwgHandle makeHardOwner(std::uint32_t ref) {
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

/// Build a hard-pointer (code 5) handle with the minimum-width ref.
dwgHandle makeHardPointer(std::uint32_t ref) {
    dwgHandle h;
    h.code = 5;
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

bool dwgWriter15::canWriteDwgDataStorageOperation(
    DwgDataStorageWriterOperation operation) const {
    switch (operation) {
    case DwgDataStorageWriterOperation::WriteMLeaderStyle:
    case DwgDataStorageWriterOperation::WriteTableStyle:
    case DwgDataStorageWriterOperation::WriteMaterial:
    case DwgDataStorageWriterOperation::WriteLightList:
    case DwgDataStorageWriterOperation::WriteGroup:
    case DwgDataStorageWriterOperation::WriteDictionary:
    case DwgDataStorageWriterOperation::WriteXRecord:
    case DwgDataStorageWriterOperation::WriteLayout:
    case DwgDataStorageWriterOperation::WriteMLineStyle:
    case DwgDataStorageWriterOperation::WriteAcDbPlaceholder:
    case DwgDataStorageWriterOperation::WriteRasterVariables:
    case DwgDataStorageWriterOperation::WriteWipeoutVariables:
    case DwgDataStorageWriterOperation::WriteGeoData:
    case DwgDataStorageWriterOperation::WriteSpatialFilter:
    case DwgDataStorageWriterOperation::WriteScale:
    case DwgDataStorageWriterOperation::WriteIDBuffer:
    case DwgDataStorageWriterOperation::WriteLayerIndex:
    case DwgDataStorageWriterOperation::WriteSpatialIndex:
    case DwgDataStorageWriterOperation::WriteDictionaryVar:
    case DwgDataStorageWriterOperation::WriteDictionaryWithDefault:
    case DwgDataStorageWriterOperation::WriteSortEntsTable:
    case DwgDataStorageWriterOperation::WriteFieldList:
    case DwgDataStorageWriterOperation::WriteField:
    case DwgDataStorageWriterOperation::WriteUnderlayDefinition:
    case DwgDataStorageWriterOperation::WritePointCloudDef:
    case DwgDataStorageWriterOperation::WriteNavisworksModelDef:
    case DwgDataStorageWriterOperation::WritePointCloudColorMap:
    case DwgDataStorageWriterOperation::WriteDbColor:
    case DwgDataStorageWriterOperation::WriteDimensionAssociation:
    case DwgDataStorageWriterOperation::WriteEvaluationGraph:
    case DwgDataStorageWriterOperation::WriteTvDeviceProperties:
    case DwgDataStorageWriterOperation::WriteVxControl:
    case DwgDataStorageWriterOperation::WriteVxTableRecord:
        return true;
    case DwgDataStorageWriterOperation::None:
        return false;
    }
    return false;
}

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

    // Bytes 0x13-0x14: RS codepage matching $DWGCODEPAGE.
    m_buf.putRawShort16(fileCodePageId());

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
                                     const std::uint8_t (&endSentinel)[16],
                                     size_t unknownTailBytes) {
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
    for (size_t i = 0; i < unknownTailBytes; ++i)
        m_buf.putRawChar8(0);
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
    if (m_version < DRW::AC1018)
        fillIfZero(m_header->vpEntHeaderCtrl,
                   reservedHandle::VPORT_ENTITY_HEADER_CONTROL);
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
        const std::uint32_t dataStartBit = m_buf.bitCount();
        // Propagate encode failure: writeDwgHeader reports success as a bool.
        if (!m_header->encodeDwg(m_version, &m_buf, &m_buf))
            return false;
        recordHeaderHandseedOffset(dataStartBit);
    }

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::HEADER_END);

    m_sectionSizes[recno::HEADER] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::writeDwgClasses() {
    if (hasDwgClassConflict() || !validateDwgClassManifest())
        return false;

    size_t sectionStart = m_buf.size();
    m_sectionOffsets[recno::CLASSES] = static_cast<std::uint32_t>(sectionStart);

    size_t sizeOffset = beginSentinelSection(dwgSentinels::CLASSES_BEGIN);

    // R2000 CLASSES is just the packed class entries between sentinels.
    // Field layout mirrors DRW_Class::parseDwg and ODA class section records.
    if (!emitDwgClassDefinitions(
            [this](const DwgClassDefinition& definition) {
                return writeDwgClassDefinition(definition, &m_buf, nullptr);
            }))
        return false;

    endSentinelSection(sectionStart, sizeOffset, dwgSentinels::CLASSES_END);

    m_sectionSizes[recno::CLASSES] =
        static_cast<std::uint32_t>(m_buf.size() - sectionStart);
    return true;
}

bool dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    std::initializer_list<std::uint32_t> childHandles)
{
    if (numEntries > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
        return false;

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
    //   BS/BL numEntries (the DWG type determines the field width)
    //   RC  unkData (DIMSTYLE only, R2000+)
    //   B   stringBit = 0  (R2007+: control objects never have strings)
    //   H   null handle
    //   H   XDic handle  (xDictFlag=0 → reader reads one)
    //   H   child offset handles × numEntries (+2 for BLOCK/LTYPE controls)
    if (!writeControlEntryCount(body, typeCode, numEntries))
        return false;
    if (typeCode == DRW::DwgDimStyleControlObjectType) {
        body.putRawChar8(0);
    }
    if (m_version > DRW::AC1018) {
        body.putBit(0);   // stringBit = 0: no strings in control objects (R2007+)
    }
    dwgBufferW *hdlBuf = &body;
    const bool useControlHandleScratch =
        (m_version > DRW::AC1014 && m_version < DRW::AC1021) ||
        m_version > DRW::AC1021;
    if (useControlHandleScratch) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0));
    hdlBuf->putHandle(makeHardOwner(0));  // XDic null
    for (std::uint32_t child : childHandles)
        hdlBuf->putHandle(makeSoftOwner(child));

    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableControlHandles.insert(handle);
    return true;
}

bool dwgWriter15::emitControlObject(
    std::uint16_t typeCode, std::uint32_t handle, std::uint32_t numEntries,
    const std::vector<std::uint32_t>& childHandles)
{
    if (numEntries > static_cast<std::uint32_t>(
                         std::numeric_limits<std::int32_t>::max()))
        return false;

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
    if (!writeControlEntryCount(body, typeCode, numEntries))
        return false;
    if (typeCode == DRW::DwgDimStyleControlObjectType)
        body.putRawChar8(0);
    if (m_version > DRW::AC1018)
        body.putBit(0);
    dwgBufferW *hdlBuf = &body;
    const bool useControlHandleScratch =
        (m_version > DRW::AC1014 && m_version < DRW::AC1021) ||
        m_version > DRW::AC1021;
    if (useControlHandleScratch) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0));
    hdlBuf->putHandle(makeHardOwner(0));
    for (std::uint32_t child : childHandles)
        hdlBuf->putHandle(makeSoftOwner(child));
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableControlHandles.insert(handle);
    return true;
}

std::uint32_t dwgWriter15::defineBlock(const std::string& name,
                                 const DRW_Coord& basePoint,
                                 int insUnits) {
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle != 0)
        return 0;
    const std::string key = toUpperCase(name);
    if (key.empty())
        return 0;
    if (insUnits < 0
        || static_cast<std::uint64_t>(insUnits)
               > std::numeric_limits<std::uint16_t>::max())
        return 0;
    const auto existing = m_userBlockHandles.find(key);
    if (existing != m_userBlockHandles.end())
        return existing->second;

    const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    const auto fail = [this, &checkpoint]() {
        rollbackCompoundWrite(checkpoint);
        return std::uint32_t{0};
    };

    // Allocate a fresh handle trio.
    std::uint32_t blockRecH  = m_handles.next();
    std::uint32_t blockH     = m_handles.next();
    std::uint32_t endBlockH  = m_handles.next();

    // Block entity for the start of the block body.
    DRW_Block bk;
    bk.handle = blockH;
    bk.layerH.ref = DRW::DwgLayer0Handle;
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
                return fail();
            }
        } else if (m_version > DRW::AC1014) {
            m_objectHandles.reset();
            if (!bk.encodeDwg(m_version, &body, /*bs=*/0,
                              nullptr, &m_objectHandles)) {
                return fail();
            }
        } else if (!bk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return fail();
        }

        finishObject();
        if (objectWriteFailed())
            return fail();
    }

    if (consumeWriteFailurePointForTest(kBeforeEndBlockFrame))
        return fail();

    // ENDBLK entity.
    DRW_Block endBlk;
    endBlk.handle = endBlockH;
    endBlk.layerH.ref = DRW::DwgLayer0Handle;
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
                return fail();
            }
        } else if (m_version > DRW::AC1014) {
            m_objectHandles.reset();
            if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0,
                                  nullptr, &m_objectHandles)) {
                return fail();
            }
        } else if (!endBlk.encodeDwg(m_version, &body, /*bs=*/0)) {
            return fail();
        }

        finishObject();
        if (objectWriteFailed())
            return fail();
    }

    if (consumeWriteFailurePointForTest(kAfterEndBlockFrame))
        return fail();

    m_emittedDwgBlockEntityHandles.insert(blockH);
    m_emittedDwgBlockEntityHandles.insert(endBlockH);

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
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle != 0)
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
    if (m_blockControlEmitted || m_activeUserBlockRecordHandle == 0)
        return false;
    m_activeUserBlockRecordHandle = 0;
    return true;
}

dwgWriter::CompoundWriteCheckpoint dwgWriter15::checkpointCompoundWrite() const {
    CompoundWriteCheckpoint checkpoint = dwgWriter::checkpointCompoundWrite();
    checkpoint.objectMapSize = m_objectMap.size();
    checkpoint.modelSpaceEntityCount = m_modelSpaceEntityHandles.size();
    checkpoint.paperSpaceEntityCount = m_paperSpaceEntityHandles.size();
    checkpoint.modelSpaceInsertCount = m_modelSpaceInsertHandles.size();
    checkpoint.paperSpaceInsertCount = m_paperSpaceInsertHandles.size();
    checkpoint.userBlockEntityCounts.reserve(m_userBlocks.size());
    checkpoint.userBlockInsertCounts.reserve(m_userBlocks.size());
    for (const PendingUserBlock& block : m_userBlocks)
        checkpoint.userBlockEntityCounts.push_back(block.entityHandles.size());
    for (const PendingUserBlock& block : m_userBlocks)
        checkpoint.userBlockInsertCounts.push_back(block.insertHandles.size());
    checkpoint.emittedDwgTableRecordHandles =
        m_emittedDwgTableRecordHandles;
    checkpoint.emittedDwgTableControlHandles =
        m_emittedDwgTableControlHandles;
    checkpoint.dwgTableRecordsComplete = m_dwgTableRecordsComplete;
    checkpoint.emittedDwgNamedObjectDictionaryEntries =
        m_emittedDwgNamedObjectDictionaryEntries;
    checkpoint.dwgNamedObjectDictionaryComplete =
        m_dwgNamedObjectDictionaryComplete;
    checkpoint.emittedDwgBlockRecords = m_emittedDwgBlockRecords;
    checkpoint.emittedDwgBlockEntityHandles = m_emittedDwgBlockEntityHandles;
    checkpoint.dwgBlockStructureComplete = m_dwgBlockStructureComplete;
    checkpoint.frameWriteError = m_frameWriteError;
    checkpoint.entityEedWriteFailure = m_entityEedWriteFailure;
    checkpoint.objectEedWriteFailure = m_objectEedWriteFailure;
    checkpoint.userBlockCount = m_userBlocks.size();
    checkpoint.userBlockHandles.clear();
    checkpoint.userBlockHandles.insert(m_userBlockHandles.cbegin(),
                                       m_userBlockHandles.cend());
    checkpoint.activeUserBlockRecordHandle = m_activeUserBlockRecordHandle;
    checkpoint.blockControlEmitted = m_blockControlEmitted;
    return checkpoint;
}

dwgWriter::CompoundWriteCheckpoint
dwgWriter15::checkpointPublicTransaction() const {
    CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    checkpoint.hasAdmissionState = true;
    checkpoint.writingContext = m_writingCtx;
    checkpoint.pendingLTypes = m_pendingLTypes;
    checkpoint.pendingLayers = m_pendingLayers;
    checkpoint.layer0 = m_layer0;
    checkpoint.haveLayer0 = m_haveLayer0;
    checkpoint.pendingStyles = m_pendingStyles;
    checkpoint.pendingUcs = m_pendingUcs;
    checkpoint.pendingViews = m_pendingViews;
    checkpoint.pendingVports = m_pendingVports;
    checkpoint.pendingViewportEntityHeaders =
        m_pendingViewportEntityHeaders;
    checkpoint.pendingDimstyles = m_pendingDimstyles;
    checkpoint.pendingAppIds = m_pendingAppIds;
    checkpoint.ltypeByBlock = m_ltypeByBlock;
    checkpoint.ltypeByLayer = m_ltypeByLayer;
    checkpoint.ltypeContinuous = m_ltypeContinuous;
    checkpoint.haveLtypeByBlock = m_haveLtypeByBlock;
    checkpoint.haveLtypeByLayer = m_haveLtypeByLayer;
    checkpoint.haveLtypeContinuous = m_haveLtypeContinuous;
    checkpoint.standardStyle = m_standardStyle;
    checkpoint.haveStandardStyle = m_haveStandardStyle;
    checkpoint.activeVport = m_activeVport;
    checkpoint.haveActiveVport = m_haveActiveVport;
    checkpoint.standardDimstyle = m_standardDimstyle;
    checkpoint.haveStandardDimstyle = m_haveStandardDimstyle;
    checkpoint.pendingDwgNamedObjectDictionaryEntries =
        m_pendingDwgNamedObjectDictionaryEntries;
    return checkpoint;
}

void dwgWriter15::rollbackCompoundWrite(
    const CompoundWriteCheckpoint& checkpoint) {
    // Rollback must remove bytes and ownership state, but an EED validation
    // failure is a file-level error and must remain visible to dwgRW::write().
    const bool entityEedFailure = m_entityEedWriteFailure;
    const bool objectEedFailure = m_objectEedWriteFailure;
    dwgWriter::rollbackCompoundWrite(checkpoint);
    if (checkpoint.objectMapSize <= m_objectMap.size())
        m_objectMap.resize(checkpoint.objectMapSize);
    if (checkpoint.modelSpaceEntityCount <= m_modelSpaceEntityHandles.size())
        m_modelSpaceEntityHandles.resize(checkpoint.modelSpaceEntityCount);
    if (checkpoint.paperSpaceEntityCount <= m_paperSpaceEntityHandles.size())
        m_paperSpaceEntityHandles.resize(checkpoint.paperSpaceEntityCount);
    if (checkpoint.modelSpaceInsertCount <= m_modelSpaceInsertHandles.size())
        m_modelSpaceInsertHandles.resize(checkpoint.modelSpaceInsertCount);
    if (checkpoint.paperSpaceInsertCount <= m_paperSpaceInsertHandles.size())
        m_paperSpaceInsertHandles.resize(checkpoint.paperSpaceInsertCount);
    if (checkpoint.userBlockEntityCounts.size() == m_userBlocks.size()) {
        for (std::size_t index = 0; index < m_userBlocks.size(); ++index) {
            const std::size_t count = checkpoint.userBlockEntityCounts[index];
            if (count <= m_userBlocks[index].entityHandles.size())
                m_userBlocks[index].entityHandles.resize(count);
        }
    }
    if (checkpoint.userBlockInsertCounts.size() == m_userBlocks.size()) {
        for (std::size_t index = 0; index < m_userBlocks.size(); ++index) {
            const std::size_t count = checkpoint.userBlockInsertCounts[index];
            if (count <= m_userBlocks[index].insertHandles.size())
                m_userBlocks[index].insertHandles.resize(count);
        }
    }
    if (checkpoint.userBlockCount <= m_userBlocks.size())
        m_userBlocks.resize(checkpoint.userBlockCount);
    m_userBlockHandles.clear();
    m_userBlockHandles.insert(checkpoint.userBlockHandles.cbegin(),
                              checkpoint.userBlockHandles.cend());
    m_emittedDwgTableRecordHandles =
        checkpoint.emittedDwgTableRecordHandles;
    m_emittedDwgTableControlHandles =
        checkpoint.emittedDwgTableControlHandles;
    m_dwgTableRecordsComplete = checkpoint.dwgTableRecordsComplete;
    m_emittedDwgNamedObjectDictionaryEntries =
        checkpoint.emittedDwgNamedObjectDictionaryEntries;
    m_dwgNamedObjectDictionaryComplete =
        checkpoint.dwgNamedObjectDictionaryComplete;
    m_emittedDwgBlockRecords = checkpoint.emittedDwgBlockRecords;
    m_emittedDwgBlockEntityHandles =
        checkpoint.emittedDwgBlockEntityHandles;
    m_dwgBlockStructureComplete = checkpoint.dwgBlockStructureComplete;
    m_frameWriteError = checkpoint.frameWriteError;
    m_entityEedWriteFailure = checkpoint.entityEedWriteFailure
        || entityEedFailure;
    m_objectEedWriteFailure = checkpoint.objectEedWriteFailure
        || objectEedFailure;
    m_activeUserBlockRecordHandle = checkpoint.activeUserBlockRecordHandle;
    m_blockControlEmitted = checkpoint.blockControlEmitted;
    if (checkpoint.hasAdmissionState) {
        m_writingCtx = checkpoint.writingContext;
        m_pendingLTypes = checkpoint.pendingLTypes;
        m_pendingLayers = checkpoint.pendingLayers;
        m_layer0 = checkpoint.layer0;
        m_haveLayer0 = checkpoint.haveLayer0;
        m_pendingStyles = checkpoint.pendingStyles;
        m_pendingUcs = checkpoint.pendingUcs;
        m_pendingViews = checkpoint.pendingViews;
        m_pendingVports = checkpoint.pendingVports;
        m_pendingViewportEntityHeaders =
            checkpoint.pendingViewportEntityHeaders;
        m_pendingDimstyles = checkpoint.pendingDimstyles;
        m_pendingAppIds = checkpoint.pendingAppIds;
        m_ltypeByBlock = checkpoint.ltypeByBlock;
        m_ltypeByLayer = checkpoint.ltypeByLayer;
        m_ltypeContinuous = checkpoint.ltypeContinuous;
        m_haveLtypeByBlock = checkpoint.haveLtypeByBlock;
        m_haveLtypeByLayer = checkpoint.haveLtypeByLayer;
        m_haveLtypeContinuous = checkpoint.haveLtypeContinuous;
        m_standardStyle = checkpoint.standardStyle;
        m_haveStandardStyle = checkpoint.haveStandardStyle;
        m_activeVport = checkpoint.activeVport;
        m_haveActiveVport = checkpoint.haveActiveVport;
        m_standardDimstyle = checkpoint.standardDimstyle;
        m_haveStandardDimstyle = checkpoint.haveStandardDimstyle;
        m_pendingDwgNamedObjectDictionaryEntries =
            checkpoint.pendingDwgNamedObjectDictionaryEntries;
    }
    m_objectBody.reset();
    m_objectStrings.reset();
    m_objectHandles.reset();
    m_currentHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
}

bool dwgWriter15::emitDeferredBlockControl() {
    if (m_blockControlEmitted)
        return true;
    if (m_activeUserBlockRecordHandle != 0)
        return false;

    const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    m_dwgBlockStructureComplete = false;
    m_emittedDwgBlockRecords.clear();

    const auto fail = [this, &checkpoint]() {
        rollbackCompoundWrite(checkpoint);
        return false;
    };

    if (m_version <= DRW::AC1015) {
        const auto isContiguousChain = [](const std::vector<std::uint32_t>& handles) {
            for (std::size_t index = 1; index < handles.size(); ++index) {
                if (handles[index - 1] == std::numeric_limits<std::uint32_t>::max()
                    || handles[index] != handles[index - 1] + 1u)
                    return false;
            }
            return true;
        };
        std::vector<std::uint32_t> modelSpaceEntityHandles = m_modelSpaceEntityHandles;
        std::vector<std::uint32_t> paperSpaceEntityHandles = m_paperSpaceEntityHandles;
        std::sort(modelSpaceEntityHandles.begin(), modelSpaceEntityHandles.end());
        std::sort(paperSpaceEntityHandles.begin(), paperSpaceEntityHandles.end());
        if (!isContiguousChain(modelSpaceEntityHandles)
            || !isContiguousChain(paperSpaceEntityHandles))
            return fail();
        for (const PendingUserBlock& block : m_userBlocks) {
            std::vector<std::uint32_t> entityHandles = block.entityHandles;
            std::sort(entityHandles.begin(), entityHandles.end());
            if (!isContiguousChain(entityHandles))
                return fail();
        }
    }

    // Every child listed by a BLOCK_RECORD must already have a committed
    // object frame. Publishing an orphan handle would make the ownership
    // graph disagree with HANDLES and leave the entity unreachable.
    const auto validateEntityList = [this](
            const std::vector<std::uint32_t>& handles) {
        if (handles.size() > dwgSafety::MaxOwnedObjectCount)
            return false;
        for (std::size_t index = 0; index < handles.size(); ++index) {
            const std::uint32_t handle = handles[index];
            if (handle == 0
                || std::find_if(m_objectMap.cbegin(), m_objectMap.cend(),
                                [handle](const auto& entry) {
                                    return entry.first == handle;
                                }) == m_objectMap.cend()
                || std::find(handles.cbegin(), handles.cbegin()
                             + static_cast<std::ptrdiff_t>(index), handle)
                       != handles.cbegin()
                             + static_cast<std::ptrdiff_t>(index))
                return false;
        }
        return true;
    };
    for (const PendingUserBlock& block : m_userBlocks) {
        if (!validateEntityList(block.entityHandles)
            || !validateEntityList(block.insertHandles))
            return fail();
    }
    if (!validateEntityList(m_modelSpaceEntityHandles)
        || !validateEntityList(m_paperSpaceEntityHandles)
        || !validateEntityList(m_modelSpaceInsertHandles)
        || !validateEntityList(m_paperSpaceInsertHandles))
        return fail();

    for (const PendingUserBlock& block : m_userBlocks) {
        std::vector<std::uint32_t> entityHandles = block.entityHandles;
        if (m_version <= DRW::AC1015)
            std::sort(entityHandles.begin(), entityHandles.end());
        if (!emitBlockRecord(block.blockRecordHandle, block.name,
                             block.basePoint, block.blockHandle,
                             block.endBlockHandle, entityHandles,
                             block.insertHandles,
                             block.insUnits))
            return fail();
        if (m_emittedDwgBlockRecords.size() == 1
            && consumeWriteFailurePointForTest(kAfterFirstBlockRecord))
            return fail();
    }

    // Modelspace and paperspace records are emitted only after the entity
    // callback has supplied every member of their owned-handle lists. Their
    // entities use ownerless entmode 2/1; the BLOCK_RECORD list is the
    // authoritative reachability graph.
    const DRW_Coord origin{0.0, 0.0, 0.0};
    std::vector<std::uint32_t> modelSpaceEntityHandles = m_modelSpaceEntityHandles;
    std::vector<std::uint32_t> paperSpaceEntityHandles = m_paperSpaceEntityHandles;
    if (m_version <= DRW::AC1015) {
        std::sort(modelSpaceEntityHandles.begin(), modelSpaceEntityHandles.end());
        std::sort(paperSpaceEntityHandles.begin(), paperSpaceEntityHandles.end());
    }
    if (!emitBlockRecord(reservedHandle::BLOCK_MODEL_SPACE, "*Model_Space",
                         origin, reservedHandle::BLOCK_MODEL_ENTITY,
                         reservedHandle::BLOCK_MODEL_END,
                         modelSpaceEntityHandles, m_modelSpaceInsertHandles)
        || !emitBlockRecord(reservedHandle::BLOCK_PAPER_SPACE, "*Paper_Space",
                            origin, reservedHandle::BLOCK_PAPER_ENTITY,
                            reservedHandle::BLOCK_PAPER_END,
                            paperSpaceEntityHandles, m_paperSpaceInsertHandles))
        return fail();

    // BLOCK_CONTROL: numEntries = user blocks count; +2 phantoms for
    // MODEL_SPACE + PAPER_SPACE are appended to the child handle list.
    std::vector<std::uint32_t> children;
    children.reserve(m_userBlocks.size() + 2);
    for (const PendingUserBlock& block : m_userBlocks)
        children.push_back(block.blockRecordHandle);
    children.push_back(reservedHandle::BLOCK_MODEL_SPACE);
    children.push_back(reservedHandle::BLOCK_PAPER_SPACE);

    dwgBufferW& body = beginObject(reservedHandle::BLOCK_CONTROL);
    body.putObjType(m_version, DRW::DwgBlockControlObjectType);
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
    dwgBufferW *hdlBuf = &body;
    if (m_version > DRW::AC1021) {
        m_objectHandles.reset();
        hdlBuf = &m_objectHandles;
    }
    hdlBuf->putHandle(makeSoftPointer(0)); // NullH (typed soft pointer)
    hdlBuf->putHandle(makeHardOwner(0));  // XDic null
    for (std::size_t index = 0; index < children.size(); ++index) {
        const std::uint32_t handle = children[index];
        const bool isModelOrPaperSpace = index >= m_userBlocks.size();
        hdlBuf->putHandle(isModelOrPaperSpace
                              ? makeHardOwner(handle)
                              : makeSoftOwner(handle));
    }
    finishObject();
    if (objectWriteFailed())
        return fail();
    m_blockControlEmitted = true;
    m_dwgBlockStructureComplete = true;
    return true;
}

bool dwgWriter15::emitBlockEntity(std::uint32_t handle,
                                  const std::string& name, bool isEnd) {
    DRW_Block bk;
    bk.handle = handle;
    bk.layerH.ref = DRW::DwgLayer0Handle;
    bk.color = 256;  // BYLAYER
    bk.name = name;
    bk.setIsEnd(isEnd);
    dwgBufferW& body = beginObject(handle);
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!bk.encodeDwg(m_version, &body, 0, &m_objectStrings,
                          &m_objectHandles)) {
            m_frameWriteError = true;
            return false;
        }
    } else if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
        if (!bk.encodeDwg(m_version, &body, 0, nullptr, &m_objectHandles)) {
            m_frameWriteError = true;
            return false;
        }
    } else {
        if (!bk.encodeDwg(m_version, &body, 0)) {
            m_frameWriteError = true;
            return false;
        }
    }
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgBlockEntityHandles.insert(handle);
    return true;
}

bool dwgWriter15::emitBlockRecord(std::uint32_t handle,
                                  const std::string& name,
                                  const DRW_Coord& basePoint,
                                  std::uint32_t blockHandle,
                                  std::uint32_t endBlockHandle,
                                  const std::vector<std::uint32_t>& entityHandles,
                                  const std::vector<std::uint32_t>& insertHandles,
                                  int insUnits) {
    if (entityHandles.size() > dwgSafety::MaxOwnedObjectCount
        || insertHandles.size() > dwgSafety::MaxOwnedObjectCount)
        return false;

    dwgBufferW& body = beginObject(handle);

    // Common table-entry preamble.
    body.putObjType(m_version, DRW::DwgBlockRecordObjectType);
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

    // Block_Record body — mirrors DRW_Block_Record::parseDwg. AC1015/AC1018
    // keep strings inline but use objSize for the trailing handle stream.
    dwgBufferW *strBuf = (m_version > DRW::AC1018) ? &m_objectStrings : &body;
    dwgBufferW *hdlBuf = (m_version > DRW::AC1014) ? &m_objectHandles : &body;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
    }
    if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
    }

    strBuf->putVariableText(m_version, name);
    if (m_version > DRW::AC1018) {
        // R2007 derives is_xref_ref from the table-record form and stores
        // only the BS resolved-state value. The dependent bit is derived
        // only when that value is 256; it is not emitted separately.
        body.putBitShort(0);               // is_xref_resolved
    } else {
        body.putBit(0);                    // flags bit 6 (xref-ref)
        body.putBitShort(0);               // xrefindex BS (R2004-)
        body.putBit(0);                    // flags bit 4 (xref dep)
    }
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
    for (std::size_t index = 0; index < insertHandles.size(); ++index) {
        body.putRawChar8(1);               // one RC marker per INSERT
    }
    body.putRawChar8(0);                   // insertCount terminator (R2000+)
    strBuf->putVariableText(m_version, std::string{});  // bkdesc empty
    body.putBitLong(0);                    // prevData BL = 0
    if (m_version > DRW::AC1018) {
        body.putBitShort(static_cast<std::uint16_t>(insUnits));
        body.putBit(0);                    // canExplode B (R2007+)
        body.putRawChar8(0);               // bkScaling RC (R2007+)
    }

    // Handle stream.
    hdlBuf->putHandle(makeSoftPointer(reservedHandle::BLOCK_CONTROL)); // parent
    hdlBuf->putHandle(makeHardOwner(0));                              // XDic null
    hdlBuf->putHandle(makeHardPointer(0));                            // NullH
    hdlBuf->putHandle(makeHardOwner(blockHandle));                    // BLOCK entity
    if (m_version <= DRW::AC1015) {
        const std::uint32_t first = entityHandles.empty() ? 0 : entityHandles.front();
        const std::uint32_t last = entityHandles.empty() ? 0 : entityHandles.back();
        hdlBuf->putHandle(makeSoftPointer(first));
        hdlBuf->putHandle(makeSoftPointer(last));
    } else {
        for (std::uint32_t entityHandle : entityHandles)
            hdlBuf->putHandle(makeHardOwner(entityHandle));
    }
    hdlBuf->putHandle(makeHardOwner(endBlockHandle)); // ENDBLK entity
    for (std::uint32_t insertHandle : insertHandles)
        hdlBuf->putHandle(makeSoftPointer(insertHandle));
    hdlBuf->putHandle(makeHardPointer(0));            // layoutH null

    finishObject();
    if (objectWriteFailed())
        return false;
    DRW::DwgBlockWriteRecord result;
    result.blockRecordHandle = handle;
    result.blockHandle = blockHandle;
    result.endBlockHandle = endBlockHandle;
    result.entityHandles = entityHandles;
    result.insertHandles = insertHandles;
    m_emittedDwgBlockRecords.push_back(std::move(result));
    return true;
}

// --- Shared preamble helper for full table-record emitters -----------------
// Writes OT + objSize stub (R2000/R2004) + own-handle + extDataSize +
// numReactors + xDictFlag.  Sets up strBuf/hdlBuf for AC1024 three-stream;
// caller passes back the strBuf/hdlBuf pointers via the out-params so the
// subsequent encodeDwg call lands its strings/handles in the right buffer.
static bool emitRecordPreamble(dwgBufferW& body, DRW::Version version,
                                std::uint16_t otype, std::uint32_t handle,
                                dwgBufferW& strBuf, dwgBufferW& hdlBuf,
                                dwgBufferW*& sb, dwgBufferW*& hb,
                                std::int32_t numReactors = 0,
                                std::uint8_t xDictFlag = 0,
                                bool hasDsData = false,
                                const std::vector<DRW_Variant*>* extData = nullptr,
                                const std::vector<DRW_Entity::PendingHandleRef>* appIdRefs = nullptr,
                                const std::vector<DRW_Entity::PendingHandleRef>* layerRefs = nullptr,
                                std::uint16_t codePage = 30) {
    body.putObjType(version, otype);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        body.putRawLong32(0);
    // The object's OWN handle at the start of the object data must use code 0
    // (handle-holder), not a code-4 reference: a code-4 here desyncs the whole
    // object on read ("Invalid object handle ... @7.2"). The control objects via
    // emitControlObject already use makeOwnHandle; the table records did not.
    // (write-review / plan 3.6)
    body.putHandle(makeOwnHandle(handle));
    if (extData == nullptr) {
        body.putBitShort(0);
    } else if (appIdRefs == nullptr || layerRefs == nullptr
               || !DRW_Entity::encodeDwgEed(version, *extData, *appIdRefs,
                                             *layerRefs, codePage, body)) {
        return false;
    }
    body.putBitLong(numReactors);
    if (version > DRW::AC1015)
        body.putBit(xDictFlag);   // xDictFlag
    if (version > DRW::AC1024)
        body.putBit(hasDsData ? 1 : 0);   // Have binary data (AC1027+)
    if (version > DRW::AC1018) {
        strBuf.reset();
        sb = &strBuf;
    } else {
        sb = &body;
    }
    if (version > DRW::AC1014) {
        hdlBuf.reset();
        hb = &hdlBuf;
    } else {
        hb = &body;
    }
    return body.isGood();
}

template <typename ExtData>
bool prepareDwgEedRefs(
    DRW::Version version, const ExtData& extData,
    const DRW_WritingContext& context,
    std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    appIdRefs.clear();
    layerRefs.clear();
    if (extData.empty())
        return true;

    bool haveAppId = false;
    try {
        for (std::size_t index = 0; index < extData.size(); ++index) {
            const auto& value = extData[index];
            if (!value)
                return false;
            if (value->code() == 1001) {
                if (value->type() != DRW_Variant::STRING
                    || value->content.s == nullptr || value->content.s->empty()) {
                    return false;
                }
                const auto appId = context.appidMap.find(
                    toUpperCase(*value->content.s));
                if (appId == context.appidMap.end())
                    return false;
                appIdRefs.push_back({index, appId->second});
                haveAppId = true;
            } else if (value->code() == 1003) {
                if (!haveAppId || value->type() != DRW_Variant::STRING
                    || value->content.s == nullptr || value->content.s->empty()) {
                    return false;
                }
                if (value->canReplayDwgRawLayerReference(version)) {
                    layerRefs.push_back(
                        {index, 0, value->rawDwgLayerReference(), true});
                    continue;
                }
                const auto layer = context.layerMap.find(
                    toUpperCase(*value->content.s));
                if (layer == context.layerMap.end())
                    return false;
                layerRefs.push_back({index, layer->second});
            } else if (!haveAppId) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    return haveAppId;
}

template <typename Object>
bool prepareDwgCommonObjectState(Object& object, DRW::Version version) {
    if (object.reactorHandles.size() > dwgSafety::MaxReactorCount
        || object.reactorHandles.size()
            > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())
        || object.extensionDictionaryFlag() > 1)
        return false;

    const std::uint8_t effectiveXDictFlag = version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());
    return true;
}

bool dwgWriter15::finishTableRecord(std::uint32_t handle) {
    finishObject();
    if (objectWriteFailed())
        return false;
    m_emittedDwgTableRecordHandles.insert(handle);
    if (m_emittedDwgTableRecordHandles.size() == 1
        && consumeWriteFailurePointForTest(kAfterFirstDeferredTableRecord))
        return false;
    return true;
}

bool dwgWriter15::getEmittedDwgTableRecordHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    if (!m_dwgTableRecordsComplete)
        return false;
    handles.assign(m_emittedDwgTableRecordHandles.cbegin(),
                   m_emittedDwgTableRecordHandles.cend());
    return true;
}

bool dwgWriter15::getEmittedDwgTableControlHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    if (!m_dwgTableRecordsComplete)
        return false;
    handles.assign(m_emittedDwgTableControlHandles.cbegin(),
                   m_emittedDwgTableControlHandles.cend());
    return true;
}

bool dwgWriter15::registerDwgNamedObjectDictionaryEntry(
    const std::string& name, std::uint32_t childHandle) {
    if (m_dwgTableRecordsComplete || name.empty() || childHandle == 0
        || childHandle == DRW::DwgNamedObjectsDictionaryHandle
        || childHandle == DRW::DwgAcadGroupDictionaryHandle
        || name == "ACAD_GROUP"
        || std::any_of(m_pendingDwgNamedObjectDictionaryEntries.cbegin(),
                       m_pendingDwgNamedObjectDictionaryEntries.cend(),
                       [&name](const auto& entry) {
                           return entry.name == name;
                       })) {
        return false;
    }
    try {
        m_handles.reserve(childHandle);
    } catch (const std::exception&) {
        return false;
    }
    m_pendingDwgNamedObjectDictionaryEntries.push_back({name, childHandle});
    return true;
}

bool dwgWriter15::getEmittedDwgNamedObjectDictionaryEntries(
    std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const {
    entries.clear();
    if (!m_dwgNamedObjectDictionaryComplete)
        return false;
    entries = m_emittedDwgNamedObjectDictionaryEntries;
    return true;
}

bool dwgWriter15::validateDwgNamedObjectDictionaryEntries() const {
    if (!m_dwgNamedObjectDictionaryComplete
        || m_emittedDwgNamedObjectDictionaryEntries.empty()) {
        return false;
    }
    const auto frameCount = [this](std::uint32_t handle) {
        return static_cast<std::size_t>(std::count_if(
            m_objectMap.cbegin(), m_objectMap.cend(), [handle](const auto& entry) {
                return entry.first == handle;
            }));
    };
    if (frameCount(DRW::DwgNamedObjectsDictionaryHandle) != 1
        || frameCount(DRW::DwgAcadGroupDictionaryHandle) != 1) {
        return false;
    }

    std::set<std::string> names;
    for (const auto& entry : m_emittedDwgNamedObjectDictionaryEntries) {
        if (entry.name.empty() || entry.childHandle == 0
            || !names.insert(entry.name).second
            || frameCount(entry.childHandle) != 1) {
            return false;
        }
    }
    const auto group = m_emittedDwgNamedObjectDictionaryEntries.cbegin();
    return group->name == "ACAD_GROUP"
        && group->childHandle == DRW::DwgAcadGroupDictionaryHandle;
}

bool dwgWriter15::getEmittedDwgBlockWriteResult(
    DRW::DwgBlockWriteResult& result) const {
    result = {};
    if (!m_dwgBlockStructureComplete)
        return false;
    result.blockControlHandle = DRW::DwgBlockControlHandle;
    result.blockEntityHandles.assign(m_emittedDwgBlockEntityHandles.cbegin(),
                                     m_emittedDwgBlockEntityHandles.cend());
    result.blockRecords = m_emittedDwgBlockRecords;
    return true;
}

// Shared body of the per-type table-record emitters below.  Each of them
// differs only in the DRW type it copies and the DWG object-type code it
// stamps into the preamble, so the sequence - copy, prepare common object
// state, prepare table-entry EED, open the object, write the standard
// preamble, encode the payload, finish the record - lives here once.
template <typename T>
bool dwgWriter15::emitTableRecord(std::uint32_t handle, const T& source,
                                  std::uint16_t objectType) {
    T object = source;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs))
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(body, m_version, objectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            object.reactorCount(), object.extensionDictionaryFlag(),
                            object.hasDataStorageBinaryData(), &object.extData,
                            &appIdRefs, &layerRefs, fileCodePageId()))
        return false;
    if (!object.encodeDwg(m_version, &body, sb, hb))
        return false;

    return finishTableRecord(handle);
}

bool dwgWriter15::emitLtypeRecord(std::uint32_t handle, const DRW_LType& lt) {
    return emitTableRecord(handle, lt, DRW::DwgLTypeObjectType);
}

bool dwgWriter15::emitLayerRecord(std::uint32_t handle, const DRW_Layer& lay) {
    // Resolve linetype name to a handle before encoding.
    DRW_Layer layerCopy = lay;
    std::string ltUpper = toUpperCase(lay.lineType);
    auto ltIt = m_writingCtx.ltypeMap.find(ltUpper);
    if (ltIt != m_writingCtx.ltypeMap.end()) {
        layerCopy.lTypeH = makeHardPointer(ltIt->second);
    } else {
        layerCopy.lTypeH = makeHardPointer(reservedHandle::LTYPE_CONTINUOUS);
    }
    if (!prepareDwgCommonObjectState(layerCopy, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(layerCopy, appIdRefs, layerRefs))
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(body, m_version, DRW::DwgLayerObjectType, handle,
                            m_objectStrings, m_objectHandles, sb, hb,
                            layerCopy.reactorCount(),
                            layerCopy.extensionDictionaryFlag(),
                            layerCopy.hasDataStorageBinaryData(),
                            &layerCopy.extData, &appIdRefs, &layerRefs,
                            fileCodePageId()))
        return false;
    if (!layerCopy.encodeDwg(m_version, &body, sb, hb))
        return false;

    return finishTableRecord(handle);
}

bool dwgWriter15::emitStyleRecord(std::uint32_t handle, const DRW_Textstyle& ts) {
    return emitTableRecord(handle, ts, DRW::DwgStyleObjectType);
}

bool dwgWriter15::emitUcsRecord(std::uint32_t handle, const DRW_UCS& ucs) {
    return emitTableRecord(handle, ucs, DRW::DwgUcsObjectType);
}

bool dwgWriter15::emitViewRecord(std::uint32_t handle, const DRW_View& view) {
    return emitTableRecord(handle, view, DRW::DwgViewObjectType);
}

bool dwgWriter15::emitVportRecord(std::uint32_t handle, const DRW_Vport& vp) {
    return emitTableRecord(handle, vp, DRW::DwgVPortObjectType);
}

bool dwgWriter15::emitViewportEntityHeaderRecord(
    std::uint32_t handle, const DRW_ViewportEntityHeader& source) {
    // No typed DataStorage section is emitted for this table-side phase.
    // Do not serialize a presence bit that cannot be accounted for by the
    // OBJECTS replay ledger.
    if (m_version > DRW::AC1024 && source.hasDataStorageBinaryData())
        return false;
    DRW_ViewportEntityHeader header = source;
    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? header.extensionDictionaryFlag() : 0;
    header.setDwgCommonObjectState(
        static_cast<std::int32_t>(header.reactorHandles.size()),
        effectiveXDictFlag, header.hasDataStorageBinaryData());
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(header, appIdRefs, layerRefs))
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version, DRW_ViewportEntityHeader::kDwgType, handle,
            m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(header.reactorHandles.size()),
            effectiveXDictFlag, header.hasDataStorageBinaryData(),
            &header.extData, &appIdRefs, &layerRefs, fileCodePageId()))
        return false;
    if (!header.encodeDwg(m_version, &body, sb, hb))
        return false;
    return finishTableRecord(handle);
}

bool dwgWriter15::emitAppIdRecord(std::uint32_t handle, const DRW_AppId& ai) {
    return emitTableRecord(handle, ai, DRW::DwgAppIdObjectType);
}

bool dwgWriter15::emitDimstyleRecord(std::uint32_t handle, const DRW_Dimstyle& ds) {
    return emitTableRecord(handle, ds, DRW::DwgDimStyleObjectType);
}

bool dwgWriter15::emitAcDbPlaceholderObject(
    std::uint32_t handle, const DRW_AcDbPlaceholder& placeholder,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::ACDBPLACEHOLDER, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, placeholder.reactorCount(),
            placeholder.extensionDictionaryFlag(),
            placeholder.hasDataStorageBinaryData(), &placeholder.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!placeholder.encodeDwg(m_version, &body, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::emitVbaProjectObject(
    std::uint32_t handle, const DRW_VbaProject& project,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb;
    dwgBufferW *hb;
    if (!emitRecordPreamble(
            body, m_version, DRW_VbaProject::kDwgType, handle,
            m_objectStrings, m_objectHandles, sb, hb, project.reactorCount(),
            project.extensionDictionaryFlag(),
            project.hasDataStorageBinaryData(), &project.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!project.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeAcDbPlaceholder(
    const DRW_AcDbPlaceholder& placeholder) {
    // ACDBPLACEHOLDER (ODA fixed type 80) is universally available since
    // R2000.  Encoder is version-clean — only the standard string/handle
    // split-buffer routing on `version > AC1018` (no AC1018+-only fields).
    // PR 13d broadened the writer gate from AC1021+ to AC1015+ in step
    // with the filter-side `canWriteFixedTypeObjects` dispatch.
    if (m_version < DRW::AC1015
        || placeholder.reactorHandles.size() > dwgSafety::MaxReactorCount
        || placeholder.xDictFlag > 1)
        return false;

    DRW_AcDbPlaceholder object = placeholder;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());
    if (!emitAcDbPlaceholderObject(object.handle, object, appIdRefs,
                                   layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeVbaProject(const DRW_VbaProject& project) {
    if (m_version < DRW::AC1015
        || project.m_data.size() > dwgSafety::MaxBufferSize
        || project.m_dataSize != project.m_data.size()
        || project.extensionDictionaryFlag() > 1
        || project.reactorHandles.size() > dwgSafety::MaxReactorCount
        || project.hasDataStorageBinaryData()) {
        return false;
    }

    DRW_VbaProject object = project;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!emitVbaProjectObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeDbColor(const DRW_DbColor& color) {
    if (m_version < DRW::AC1018
        || color.reactorHandles.size() > dwgSafety::MaxReactorCount
        || color.extensionDictionaryFlag() > 1)
        return false;

    DRW_DbColor object = color;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    const std::uint16_t classNum = typedDwgClassNum(
        "AcDbColor", "DBCOLOR", DRW_DbColor::kDwgClassNum);
    if (!emitRecordPreamble(
            body, m_version, classNum, object.handle, m_objectStrings,
            m_objectHandles, sb, hb, object.reactorCount(),
            object.extensionDictionaryFlag(), object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeDimensionAssociation(
    const DRW_DimensionAssociation& association) {
    if (m_version < DRW::AC1021
        || association.reactorHandles.size() > dwgSafety::MaxReactorCount
        || association.extensionDictionaryFlag() > 1)
        return false;

    DRW_DimensionAssociation object = association;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerDimensionAssociationObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDimAssoc", "DIMASSOC",
                             DRW_DimensionAssociation::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeEvaluationGraph(const DRW_EvaluationGraph& graph) {
    if (m_version < DRW::AC1021
        || graph.reactorHandles.size() > dwgSafety::MaxReactorCount
        || graph.extensionDictionaryFlag() > 1)
        return false;

    DRW_EvaluationGraph object = graph;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerEvaluationGraphObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbEvalGraph", "ACAD_EVALUATION_GRAPH",
                             DRW_EvaluationGraph::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::emitBlockRepresentationDataObject(
    std::uint32_t handle, const DRW_BlockRepresentationData& data,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::BLOCKREPRESENTATION, handle,
            m_objectStrings, m_objectHandles, sb, hb, data.reactorCount(),
            data.extensionDictionaryFlag(), data.hasDataStorageBinaryData(),
            &data.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!data.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeBlockRepresentationData(
    const DRW_BlockRepresentationData& data) {
    if (m_version < DRW::AC1015
        || data.reactorHandles.size() > dwgSafety::MaxReactorCount
        || data.extensionDictionaryFlag() > 1
        || data.hasDataStorageBinaryData())
        return false;

    DRW_BlockRepresentationData object = data;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);

    if (!emitBlockRepresentationDataObject(object.handle, object, appIdRefs,
                                           layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitSunObject(
    std::uint32_t handle, const DRW_Sun& sun,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSun", "SUN", DRW_Sun::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            sun.reactorCount(), sun.extensionDictionaryFlag(),
            sun.hasDataStorageBinaryData(), &sun.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!sun.encodeDwg(m_version, &body, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSun(const DRW_Sun& sun) {
    if (m_version < DRW::AC1021
        || sun.reactorHandles.size() > dwgSafety::MaxReactorCount
        || sun.xDictFlag > 1 || sun.hasDataStorageBinaryData())
        return false;

    DRW_Sun object = sun;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSunObjectClass(object.handle))
        return false;

    if (!emitSunObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitTvDevicePropertiesObject(
    std::uint32_t handle, const DRW_TvDeviceProperties& properties,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbTvDeviceProperties", "TVDEVICEPROPERTIES",
                             DRW_TvDeviceProperties::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            properties.reactorCount(), properties.extensionDictionaryFlag(),
            properties.hasDataStorageBinaryData(), &properties.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!properties.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeTvDeviceProperties(
    const DRW_TvDeviceProperties& properties) {
    if (m_version < DRW::AC1015
        || properties.reactorHandles.size() > dwgSafety::MaxReactorCount
        || properties.xDictFlag > 1)
        return false;

    DRW_TvDeviceProperties object = properties;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerTvDevicePropertiesObjectClass(object.handle))
        return false;

    if (!emitTvDevicePropertiesObject(object.handle, object, appIdRefs,
                                      layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeVxControl(const DRW_VxControl& control) {
    if (m_version < DRW::AC1015
        || control.recordHandles.size() > dwgSafety::MaxReactorCount
        || control.reactorHandles.size() > dwgSafety::MaxReactorCount
        || (control.hasDwgRawData()
            && control.dwgRawDataVersion() != DRW::UNKNOWNV
            && control.dwgRawDataVersion() != m_version))
        return false;

    DRW_VxControl object = control;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVxControlObjectClass(object.handle))
        return false;

    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVxControl", "VXCONTROL",
                             DRW_VxControl::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(object.reactorHandles.size()),
            effectiveXDictFlag, object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    if (objectWriteFailed()) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeVxTableRecord(const DRW_VxTableRecord& record) {
    if (m_version < DRW::AC1015
        || record.reactorHandles.size() > dwgSafety::MaxReactorCount
        || (record.hasDwgRawData()
            && record.dwgRawDataVersion() != DRW::UNKNOWNV
            && record.dwgRawDataVersion() != m_version))
        return false;

    DRW_VxTableRecord object = record;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVxTableRecordObjectClass(object.handle))
        return false;

    const std::uint8_t effectiveXDictFlag = m_version > DRW::AC1015
        ? object.extensionDictionaryFlag() : 0;
    object.setDwgCommonObjectState(
        static_cast<std::int32_t>(object.reactorHandles.size()),
        effectiveXDictFlag, object.hasDataStorageBinaryData());

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVxTableRecord", "VXTABLERECORD",
                             DRW_VxTableRecord::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            static_cast<std::int32_t>(object.reactorHandles.size()),
            effectiveXDictFlag, object.hasDataStorageBinaryData(),
            &object.extData, &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    if (objectWriteFailed()) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitMLeaderStyleObject(std::uint32_t handle,
                                         const DRW_MLeaderStyle& style,
                                         const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                         const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMLeaderStyle", "MLEADERSTYLE",
                             DRW_MLeaderStyle::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

// Shared body of the typed OBJECT writers below.  Each differs only in the DRW
// payload type, the minimum file version it supports and which register/emit
// pair it drives; the sequence - version gate, copy, prepare common object
// state, prepare table-entry EED (recording an EED failure so a caller can
// tell it apart from a plain refusal), mint or reserve the handle, register
// the class, emit the object - is identical.
//
// Writers whose gate inspects more than the version, or that keep extra state,
// keep their own bodies.
template <typename T>
bool dwgWriter15::writeTypedObject(
    const T& source, DRW::Version minVersion,
    bool (dwgWriter::*registerClass)(std::uint32_t),
    bool (dwgWriter15::*emitObject)(
        std::uint32_t, const T&,
        const std::vector<DRW_Entity::PendingHandleRef>&,
        const std::vector<DRW_Entity::PendingHandleRef>&)) {
    if (m_version < minVersion)
        return false;

    T object = source;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!(this->*registerClass)(object.handle))
        return false;

    if (!(this->*emitObject)(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeMLeaderStyle(const DRW_MLeaderStyle& style) {
    return writeTypedObject(style, DRW::AC1015,
                            &dwgWriter::registerMLeaderStyleObjectClass,
                            &dwgWriter15::emitMLeaderStyleObject);
}

bool dwgWriter15::emitTableStyleObject(std::uint32_t handle,
                                       const DRW_TableStyle& style,
                                       const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                       const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbTableStyle", "TABLESTYLE",
                             DRW_TableStyle::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeTableStyle(const DRW_TableStyle& style) {
    if (m_version < DRW::AC1015 || m_version > DRW::AC1021)
        return false;

    DRW_TableStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerTableStyleObjectClass(object.handle))
        return false;

    if (!emitTableStyleObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitMaterialObject(
    std::uint32_t handle, const DRW_Material& material,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMaterial", "MATERIAL",
                             DRW_Material::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            material.reactorCount(), material.extensionDictionaryFlag(),
            material.hasDataStorageBinaryData(), &material.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!material.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMaterial(const DRW_Material& material) {
    DRW_Material object = material;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerMaterialObjectClass(object.handle))
        return false;

    if (!emitMaterialObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitLightListObject(
    std::uint32_t handle, const DRW_LightList& lightList,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbLightList", "LIGHTLIST", 0), handle,
            m_objectStrings, m_objectHandles, sb, hb,
            lightList.reactorCount(), lightList.extensionDictionaryFlag(),
            lightList.hasDataStorageBinaryData(), &lightList.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!lightList.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeLightList(const DRW_LightList& lightList) {
    if (m_version < DRW::AC1015
        || lightList.m_lightCount > DRW_LightList::kMaxLightCount
        || lightList.m_lights.size()
            != static_cast<std::size_t>(lightList.m_lightCount)
        || lightList.m_classVersion > static_cast<std::uint32_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return false;
    }

    DRW_LightList object = lightList;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerLightListObjectClass(object.handle))
        return false;

    if (!emitLightListObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// DICTIONARY (ODA fixed type 42) — no class registration required.  Uses the
// standard preamble + DRW_Dictionary::encodeDwg sandwich, mirroring the
// SUN / PLACEHOLDER native-writer pattern.  Per-entry string handles live
// in sb so the AC1018+ split-buffer convention applies.
bool dwgWriter15::emitDictionaryObject(
    std::uint32_t handle, const DRW_Dictionary& dictionary,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::DICTIONARY, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, dictionary.reactorCount(),
            dictionary.extensionDictionaryFlag(),
            dictionary.hasDataStorageBinaryData(), &dictionary.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!dictionary.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeDictionary(const DRW_Dictionary& dictionary) {
    if (dictionary.handle == DRW::DwgNamedObjectsDictionaryHandle
        || dictionary.handle == DRW::DwgAcadGroupDictionaryHandle) {
        return false;
    }
    DRW_Dictionary object = dictionary;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!emitDictionaryObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// XRECORD (ODA fixed type 0x4f = 79) — no class registration required.
// XRECORD's encoder takes (buf, /*strBuf=*/nullptr, hb): strings are emitted
// inline as byte-counted data within the data section, so no separate
// string buffer is needed.
bool dwgWriter15::emitXRecordObject(std::uint32_t handle,
                                    const DRW_XRecord& xrecord,
                                    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::XRECORD, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, xrecord.reactorCount(), xrecord.extensionDictionaryFlag(),
            xrecord.hasDataStorageBinaryData(), &xrecord.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!xrecord.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeXRecord(const DRW_XRecord& xrecord) {
    DRW_XRecord object = xrecord;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!emitXRecordObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::emitPlotSettingsObject(
    std::uint32_t handle, const DRW_PlotSettings& plotSettings,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    const std::uint16_t classNum = typedDwgClassNum(
        "AcDbPlotSettings", "PLOTSETTINGS", 0);
    if (classNum < 500)
        return false;

    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, classNum, handle, m_objectStrings,
            m_objectHandles, sb, hb, plotSettings.reactorCount(),
            plotSettings.extensionDictionaryFlag(),
            plotSettings.hasDataStorageBinaryData(), &plotSettings.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!plotSettings.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePlotSettings(
    const DRW_PlotSettings& plotSettings) {
    if (m_version < DRW::AC1015
        || plotSettings.reactorHandles.size() > dwgSafety::MaxReactorCount
        || plotSettings.extensionDictionaryFlag() > 1
        || plotSettings.hasDataStorageBinaryData())
        return false;

    DRW_PlotSettings object = plotSettings;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!emitPlotSettingsObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    const std::uint16_t classNumber = typedDwgClassNum(
        "AcDbPlotSettings", "PLOTSETTINGS", 0);
    if (plotSettings.handle != 0) {
        markDwgClassInstanceEmitted(object.handle);
        std::uint16_t emittedClassNumber = 0;
        return getDwgClassInstanceClass(object.handle, emittedClassNumber)
            ? emittedClassNumber == classNumber
            : false;
    }
    return noteDwgClassInstanceEmitted(classNumber);
}

// LAYOUT (ODA fixed type 82, §20.4.84) — no class registration required.
// Encoder needs both string and handle buffers (AC1018+ split, inline
// pre-AC1018).
bool dwgWriter15::emitLayoutObject(std::uint32_t handle,
                                   const DRW_Layout& layout,
                                   const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                   const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::LAYOUT, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, layout.reactorCount(), layout.extensionDictionaryFlag(),
            layout.hasDataStorageBinaryData(), &layout.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!layout.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeLayout(const DRW_Layout& layout) {
    if (layout.viewportCount < 0
        || layout.viewportCount > DRW_Layout::kMaxViewportCount
        || layout.viewportHandles.size()
            != static_cast<std::size_t>(layout.viewportCount))
        return false;
    DRW_Layout object = layout;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!emitLayoutObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// GROUP (ODA fixed type 72) — no class registration required.  Standard
// preamble + DRW_Group::encodeDwg sandwich.  description is the only
// string field (carried in sb for AC1018+ split); entityHandles live in
// the handle stream.  Mirrors the DICTIONARY / LAYOUT shape from PR 8b/8c.
bool dwgWriter15::emitGroupObject(std::uint32_t handle,
                                  const DRW_Group& group,
                                  const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                  const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::GROUP, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, group.reactorCount(), group.extensionDictionaryFlag(),
            group.hasDataStorageBinaryData(), &group.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!group.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeGroup(const DRW_Group& group) {
    DRW_Group object = group;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!emitGroupObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// MLINESTYLE (fixed object type 73) needs no CLASS registration. Record the
// emitted handle by style name so later MLINE entities can point at it.
bool dwgWriter15::emitMLineStyleObject(std::uint32_t handle,
                                       const DRW_MLineStyle& style,
                                       const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                       const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, dwgObjType::MLINESTYLE, handle,
            m_objectStrings, m_objectHandles,
            sb, hb, style.reactorCount(), style.extensionDictionaryFlag(),
            style.hasDataStorageBinaryData(), &style.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!style.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMLineStyle(const DRW_MLineStyle& style) {
    DRW_MLineStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    const std::string upper = toUpperCase(object.name);
    auto existing = m_writingCtx.mlineStyleMap.find(upper);
    if (!upper.empty() && existing != m_writingCtx.mlineStyleMap.end()) {
        if (object.handle == 0 || object.handle == existing->second) {
            if (!object.extData.empty()) {
                m_objectEedWriteFailure = true;
                return false;
            }
            return true;
        }
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
    if (!emitMLineStyleObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (!upper.empty())
        m_writingCtx.mlineStyleMap[upper] = object.handle;
    return true;
}

// RASTERVARIABLES (AcDbRasterVariables, custom class 505) — class
// registration required so the reader's CLASSES section dispatch can
// resolve the recordName back to DRW_RasterVariables::parseDwg. Standard
// preamble + DRW_RasterVariables::encodeDwg sandwich. Encoder ignores
// strBuf (no string fields), so pass nullptr to mirror parse semantics.
bool dwgWriter15::emitRasterVariablesObject(
    std::uint32_t handle, const DRW_RasterVariables& rasterVariables,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbRasterVariables", "RASTERVARIABLES",
                             DRW_RasterVariables::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            rasterVariables.reactorCount(),
            rasterVariables.extensionDictionaryFlag(),
            rasterVariables.hasDataStorageBinaryData(), &rasterVariables.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!rasterVariables.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
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
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerRasterVariablesObjectClass(object.handle))
        return false;

    if (!emitRasterVariablesObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class 529). Body has one
// display-frame flag, then the common handle prefix in the object handle
// stream.
bool dwgWriter15::emitWipeoutVariablesObject(
    std::uint32_t handle, const DRW_WipeoutVariables& wipeoutVariables,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbWipeoutVariables", "WIPEOUTVARIABLES",
                             DRW_WipeoutVariables::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            wipeoutVariables.reactorCount(),
            wipeoutVariables.extensionDictionaryFlag(),
            wipeoutVariables.hasDataStorageBinaryData(),
            &wipeoutVariables.extData, &appIdRefs, &layerRefs,
            fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!wipeoutVariables.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeWipeoutVariables(
    const DRW_WipeoutVariables& wipeoutVariables) {
    if (m_version < DRW::AC1015)
        return false;

    DRW_WipeoutVariables object = wipeoutVariables;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerWipeoutVariablesObjectClass(object.handle))
        return false;

    if (!emitWipeoutVariablesObject(object.handle, object, appIdRefs,
                                    layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// IMAGEDEF (fixed type 102 / dwgObjType::IMAGEDEF) — no CLASSES entry.
// Inverts DRW_ImageDef::parseDwg; handle is assigned when the caller left it 0.
bool dwgWriter15::writeImageDef(DRW_ImageDef& imageDef) {
    if (imageDef.reactorHandles.size() > dwgSafety::MaxReactorCount
        || imageDef.extensionDictionaryFlag() > 1
        || (m_version > DRW::AC1024
            && imageDef.hasDataStorageBinaryData()))
        return false;
    if (!prepareDwgCommonObjectState(imageDef, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(imageDef, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !imageDef.extData.empty();
        return false;
    }
    if (imageDef.handle == 0) {
        imageDef.handle = m_handles.next();
    } else {
        m_handles.reserve(imageDef.handle);
    }
    dwgBufferW& body = beginObject(imageDef.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, /*typeCode=*/102, imageDef.handle,
            m_objectStrings, m_objectHandles, sb, hb,
            imageDef.reactorCount(), imageDef.extensionDictionaryFlag(),
            imageDef.hasDataStorageBinaryData(), &imageDef.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !imageDef.extData.empty();
        return false;
    }
    if (!imageDef.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

// IMAGEDEF_REACTOR (custom class 532). Body is class_version + common handles.
bool dwgWriter15::writeImageDefinitionReactor(
    DRW_ImageDefinitionReactor& reactor) {
    if (reactor.reactorHandles.size() > dwgSafety::MaxReactorCount
        || reactor.extensionDictionaryFlag() > 1
        || (m_version > DRW::AC1024
            && reactor.hasDataStorageBinaryData()))
        return false;
    if (!prepareDwgCommonObjectState(reactor, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(reactor, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !reactor.extData.empty();
        return false;
    }
    if (reactor.handle == 0) {
        reactor.handle = m_handles.next();
    } else {
        m_handles.reserve(reactor.handle);
    }
    if (!registerImageDefReactorObjectClass(reactor.handle))
        return false;
    dwgBufferW& body = beginObject(reactor.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbRasterImageDefReactor", "IMAGEDEF_REACTOR",
                             DRW_ImageDefinitionReactor::kDwgClassNum),
            reactor.handle, m_objectStrings, m_objectHandles, sb, hb,
            reactor.reactorCount(), reactor.extensionDictionaryFlag(),
            reactor.hasDataStorageBinaryData(), &reactor.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !reactor.extData.empty();
        return false;
    }
    DRW_UNUSED(sb);
    if (!reactor.encodeDwg(m_version, &body, nullptr, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointCloudDef(DRW_PointCloudDef& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(definition, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(definition, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (definition.handle == 0)
        definition.handle = m_handles.next();
    else
        m_handles.reserve(definition.handle);
    if (!registerPointCloudDefObjectClass(definition.m_kind,
                                          definition.handle))
        return false;

    std::uint16_t classNum = DRW_PointCloudDef::kDwgClassNumDefinition;
    const char *className = "AcDbPointCloudDef";
    const char *recordName = "POINTCLOUDDEFINITION";
    switch (definition.m_kind) {
    case DRW_PointCloudDef::Definition:
        classNum = DRW_PointCloudDef::kDwgClassNumDefinition;
        break;
    case DRW_PointCloudDef::DefinitionEx:
        classNum = DRW_PointCloudDef::kDwgClassNumDefinitionEx;
        className = "AcDbPointCloudDefEx";
        recordName = "POINTCLOUDDEFINITIONEX";
        break;
    case DRW_PointCloudDef::Reactor:
        classNum = DRW_PointCloudDef::kDwgClassNumReactor;
        className = "AcDbPointCloudDefReactor";
        recordName = "POINTCLOUDDEFREACTOR";
        break;
    case DRW_PointCloudDef::ReactorEx:
        classNum = DRW_PointCloudDef::kDwgClassNumReactorEx;
        className = "AcDbPointCloudDefReactorEx";
        recordName = "POINTCLOUDDEFREACTOREX";
        break;
    }

    dwgBufferW& body = beginObject(definition.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version, typedDwgClassNum(className, recordName, classNum),
            definition.handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeNavisworksModelDef(DRW_NavisworksModelDef& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(definition, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(definition, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (definition.handle == 0)
        definition.handle = m_handles.next();
    else
        m_handles.reserve(definition.handle);
    if (!registerNavisworksModelDefObjectClass(definition.handle))
        return false;

    dwgBufferW& body = beginObject(definition.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbNavisworksModelDef", "NAVISWORKSMODELDEF",
                             DRW_NavisworksModelDef::kDwgClassNum),
            definition.handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !definition.extData.empty();
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointCloudColorMap(DRW_PointCloudColorMap& colorMap) {
    if (m_version < DRW::AC1015
        || colorMap.reactorHandles.size() > dwgSafety::MaxReactorCount
        || colorMap.extensionDictionaryFlag() > 1)
        return false;
    if (!prepareDwgCommonObjectState(colorMap, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(colorMap, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !colorMap.extData.empty();
        return false;
    }
    if (colorMap.handle == 0)
        colorMap.handle = m_handles.next();
    else
        m_handles.reserve(colorMap.handle);
    if (!registerPointCloudColorMapObjectClass(colorMap.handle))
        return false;

    dwgBufferW& body = beginObject(colorMap.handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPointCloudColorMap", "POINTCLOUDCOLORMAP",
                             DRW_PointCloudColorMap::kDwgClassNum),
            colorMap.handle, m_objectStrings, m_objectHandles, sb, hb,
            colorMap.reactorCount(), colorMap.extensionDictionaryFlag(),
            colorMap.hasDataStorageBinaryData(), &colorMap.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        m_objectEedWriteFailure = !colorMap.extData.empty();
        return false;
    }
    if (!colorMap.encodeDwg(m_version, &body, sb, hb))
        return false;
    finishObject();
    return !objectWriteFailed();
}

// GEODATA (AcDbGeoData, custom class 506) - class registration required.
// Standard preamble + DRW_GeoData::encodeDwg sandwich. Encoder writes its own
// common-handle prefix into hb (the preamble does not) plus several
// variable-text fields into sb.
bool dwgWriter15::emitGeoDataObject(
    std::uint32_t handle, const DRW_GeoData& geoData,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbGeoData", "GEODATA",
                             DRW_GeoData::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            geoData.reactorCount(), geoData.extensionDictionaryFlag(),
            geoData.hasDataStorageBinaryData(), &geoData.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!geoData.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeGeoData(const DRW_GeoData& geoData) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015
        || geoData.reactorHandles.size() > dwgSafety::MaxReactorCount
        || geoData.extensionDictionaryFlag() > 1)
        return false;

    DRW_GeoData object = geoData;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerGeoDataObjectClass(object.handle))
        return false;

    if (!emitGeoDataObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// SPATIAL_FILTER (AcDbSpatialFilter, custom class 507) — class registration
// required.  Standard preamble + DRW_SpatialFilter::encodeDwg sandwich.
// Encoder writes its own common-handle prefix into hb; body fields are
// pure numeric (no strings), so strBuf is unused.
bool dwgWriter15::emitSpatialFilterObject(std::uint32_t handle,
                                          const DRW_SpatialFilter& filter,
                                          const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                          const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSpatialFilter", "SPATIAL_FILTER",
                             DRW_SpatialFilter::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            filter.reactorCount(), filter.extensionDictionaryFlag(),
            filter.hasDataStorageBinaryData(), &filter.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!filter.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSpatialFilter(const DRW_SpatialFilter& filter) {
    // PR 13f — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).  Parser mirrors the same shape.
    if (m_version < DRW::AC1015
        || filter.reactorHandles.size() > dwgSafety::MaxReactorCount
        || filter.extensionDictionaryFlag() > 1)
        return false;

    DRW_SpatialFilter object = filter;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerSpatialFilterObjectClass(object.handle))
        return false;

    if (!emitSpatialFilterObject(object.handle, object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// SCALE (AcDbScale, custom class 508) — class registration required.  The
// SCALE encoder writes body fields only; the wrapper emits the common handle
// prefix (parentHandle + reactors + xdic) in the handle stream.  PR 8d.2a.
bool dwgWriter15::emitScaleObject(
    std::uint32_t handle, const DRW_Scale& scale,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbScale", "SCALE", DRW_Scale::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            scale.reactorCount(), scale.extensionDictionaryFlag(),
            scale.hasDataStorageBinaryData(), &scale.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!scale.encodeDwg(m_version, &body, sb))
        return false;

    hb->putHandle(makeSoftPointer(static_cast<std::uint32_t>(scale.parentHandle)));
    for (std::int32_t i = 0; i < scale.reactorCount(); ++i) {
        const std::uint32_t reactor =
            static_cast<std::size_t>(i) < scale.reactorHandles.size()
                ? scale.reactorHandles[static_cast<std::size_t>(i)] : 0u;
        hb->putHandle(makeSoftPointer(reactor));
    }
    if (scale.extensionDictionaryFlag() != 1)
        hb->putHandle(makeHardOwner(scale.xDictHandle));
    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (body-only emit; common handle prefix written by
    // emitScaleObject itself).
bool dwgWriter15::writeScale(const DRW_Scale& scale) {
    return writeTypedObject(scale, DRW::AC1015,
                            &dwgWriter::registerScaleObjectClass,
                            &dwgWriter15::emitScaleObject);
}

// IDBUFFER (AcDbIdBuffer, custom class 509) — class registration required.
// Standard preamble + DRW_IDBuffer::encodeDwg sandwich.  Encoder owns its
// common-handle prefix + object-id handles in hb.  PR 8d.2a.
bool dwgWriter15::emitIDBufferObject(
    std::uint32_t handle, const DRW_IDBuffer& idBuffer,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbIdBuffer", "IDBUFFER",
                             DRW_IDBuffer::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            idBuffer.reactorCount(), idBuffer.extensionDictionaryFlag(),
            idBuffer.hasDataStorageBinaryData(), &idBuffer.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!idBuffer.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard hb = version > AC1018 split-buffer
    // routing).
bool dwgWriter15::writeIDBuffer(const DRW_IDBuffer& idBuffer) {
    return writeTypedObject(idBuffer, DRW::AC1015,
                            &dwgWriter::registerIDBufferObjectClass,
                            &dwgWriter15::emitIDBufferObject);
}

// LAYER_INDEX (AcDbLayerIndex, custom class 510) — class registration
// required.  Standard preamble + DRW_LayerIndex::encodeDwg sandwich.  Encoder
// writes entry names through sb (TV); per-entry handles + common prefix go
// to hb.  PR 8d.2a.
bool dwgWriter15::emitLayerIndexObject(
    std::uint32_t handle, const DRW_LayerIndex& layerIndex,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbLayerIndex", "LAYER_INDEX",
                             DRW_LayerIndex::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            layerIndex.reactorCount(), layerIndex.extensionDictionaryFlag(),
            layerIndex.hasDataStorageBinaryData(), &layerIndex.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!layerIndex.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
bool dwgWriter15::writeLayerIndex(const DRW_LayerIndex& layerIndex) {
    return writeTypedObject(layerIndex, DRW::AC1015,
                            &dwgWriter::registerLayerIndexObjectClass,
                            &dwgWriter15::emitLayerIndexObject);
}

// SPATIAL_INDEX (AcDbSpatialIndex, custom class 511) — class registration
// required.  Standard preamble + DRW_SpatialIndex::encodeDwg sandwich.
// Encoder writes timestamps to the body and the common-handle prefix to hb
// (the latter only at R2007+; gated to AC1021+ anyway).  PR 8d.2a.
bool dwgWriter15::emitSpatialIndexObject(
    std::uint32_t handle, const DRW_SpatialIndex& spatialIndex,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSpatialIndex", "SPATIAL_INDEX",
                             DRW_SpatialIndex::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            spatialIndex.reactorCount(), spatialIndex.extensionDictionaryFlag(),
            spatialIndex.hasDataStorageBinaryData(), &spatialIndex.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!spatialIndex.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder gates the
    // common handle prefix on `version > AC1018` (parser does the same),
    // so at AC1015/AC1018 the wrapper emits an opaque body and no handle
    // tail — matching the parser's expectation.
bool dwgWriter15::writeSpatialIndex(const DRW_SpatialIndex& spatialIndex) {
    return writeTypedObject(spatialIndex, DRW::AC1015,
                            &dwgWriter::registerSpatialIndexObjectClass,
                            &dwgWriter15::emitSpatialIndexObject);
}

// DICTIONARYVAR (AcDbDictionaryVar, custom class 512) — class registration
// required.  Standard preamble + DRW_DictionaryVar::encodeDwg sandwich.
// Encoder owns its common-handle prefix; the m_value string goes through
// sb at AC1018+.  PR 8d.2a.
bool dwgWriter15::emitDictionaryVarObject(
    std::uint32_t handle, const DRW_DictionaryVar& dictionaryVar,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDictionaryVar", "DICTIONARYVAR",
                             DRW_DictionaryVar::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            dictionaryVar.reactorCount(),
            dictionaryVar.extensionDictionaryFlag(),
            dictionaryVar.hasDataStorageBinaryData(), &dictionaryVar.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!dictionaryVar.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13g — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (only the standard sb/hb = version > AC1018 split-
    // buffer routing).
bool dwgWriter15::writeDictionaryVar(const DRW_DictionaryVar& dictionaryVar) {
    return writeTypedObject(dictionaryVar, DRW::AC1015,
                            &dwgWriter::registerDictionaryVarObjectClass,
                            &dwgWriter15::emitDictionaryVarObject);
}

// DICTIONARYWDFLT (AcDbDictionaryWithDefault, custom class 513) — class
// registration required.  The encoder delegates to
// DRW_Dictionary::encodeDwg for the body + per-entry handle list, then
// appends the single default-entry handle at the tail (mirroring its
// parser).  Standard preamble + encoder sandwich.  PR 8d.2b.
bool dwgWriter15::emitDictionaryWithDefaultObject(
    std::uint32_t handle, const DRW_DictionaryWithDefault& dictionary,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbDictionaryWithDefault",
                             "ACDBDICTIONARYWDFLT",
                             DRW_DictionaryWithDefault::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            dictionary.reactorCount(), dictionary.extensionDictionaryFlag(),
            dictionary.hasDataStorageBinaryData(), &dictionary.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!dictionary.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeDictionaryWithDefault(const DRW_DictionaryWithDefault& dictionary) {
    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (delegates to DRW_Dictionary::encodeDwg, then
    // appends a single default-entry hard pointer at the tail).
    if (m_version < DRW::AC1015)
        return false;

    DRW_DictionaryWithDefault object = dictionary;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerDictionaryWithDefaultObjectClass(object.handle))
        return false;

    if (!emitDictionaryWithDefaultObject(object.handle, object, appIdRefs,
                                         layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

// SORTENTSTABLE (AcDbSortentsTable, custom class 514) — class registration
// required.  Standard preamble + DRW_SortEntsTable::encodeDwg sandwich.
// Encoder inverts the usual "all handles in handle stream" convention: per-
// entry sort handles go inline in the body section BEFORE the common
// prefix; block-owner + entity handles follow in the handle stream.
// PR 8d.2b.
bool dwgWriter15::emitSortEntsTableObject(
    std::uint32_t handle, const DRW_SortEntsTable& sortEntsTable,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSortentsTable", "SORTENTSTABLE",
                             DRW_SortEntsTable::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            sortEntsTable.reactorCount(),
            sortEntsTable.extensionDictionaryFlag(),
            sortEntsTable.hasDataStorageBinaryData(), &sortEntsTable.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!sortEntsTable.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (inline sort handles in body, then standard hb =
    // version > AC1018 split-buffer routing for the common prefix +
    // block-owner + entity handles).
bool dwgWriter15::writeSortEntsTable(const DRW_SortEntsTable& sortEntsTable) {
    return writeTypedObject(sortEntsTable, DRW::AC1015,
                            &dwgWriter::registerSortEntsTableObjectClass,
                            &dwgWriter15::emitSortEntsTableObject);
}

// FIELDLIST (AcDbFieldList, custom class 515) — class registration
// required.  Standard preamble + DRW_FieldList::encodeDwg sandwich.
// Encoder owns common-handle prefix + per-field handle list in hb.  No
// strings.  PR 8d.2b.
bool dwgWriter15::emitFieldListObject(std::uint32_t handle,
                                      const DRW_FieldList& fieldList,
                                      const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
                                      const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbFieldList", "FIELDLIST",
                             DRW_FieldList::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            fieldList.reactorCount(), fieldList.extensionDictionaryFlag(),
            fieldList.hasDataStorageBinaryData(), &fieldList.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    DRW_UNUSED(sb);
    if (!fieldList.encodeDwg(m_version, &body, nullptr, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder is
    // version-clean (no strings; only the standard hb = version > AC1018
    // split-buffer routing).
bool dwgWriter15::writeFieldList(const DRW_FieldList& fieldList) {
    return writeTypedObject(fieldList, DRW::AC1015,
                            &dwgWriter::registerFieldListObjectClass,
                            &dwgWriter15::emitFieldListObject);
}

// FIELD (AcDbField, custom class 516) — class registration required.
// Standard preamble + DRW_Field::encodeDwg sandwich.  Encoder writes
// evaluator/code/format/messages through sb (TV), CadValue + child values
// through buf+sb, and common-prefix + child handles + object handles
// inline through hb.  PR 8d.2b.
bool dwgWriter15::emitFieldObject(
    std::uint32_t handle, const DRW_Field& field,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbField", "FIELD", DRW_Field::kDwgClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            field.reactorCount(), field.extensionDictionaryFlag(),
            field.hasDataStorageBinaryData(), &field.extData, &appIdRefs,
            &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!field.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

    // PR 13h — broaden gate from AC1021+ to AC1015+.  Encoder has a
    // `version < AC1021` branch for m_formatString, mirrored in the
    // parser, so the pre-R2007 path is exercised; nested CadValue /
    // child values go through writeCadValue/readCadValue (also
    // version-clean).
bool dwgWriter15::writeField(const DRW_Field& field) {
    return writeTypedObject(field, DRW::AC1015,
                            &dwgWriter::registerFieldObjectClass,
                            &dwgWriter15::emitFieldObject);
}

bool dwgWriter15::emitUnderlayDefinitionObject(
    std::uint32_t handle, const DRW_UnderlayDefinition& definition,
    const std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    const std::vector<DRW_Entity::PendingHandleRef>& layerRefs) {
    const std::uint16_t sourceClassNum = underlayDefinitionClassNum(definition);
    if (sourceClassNum == 0)
        return false;
    dwgBufferW& body = beginObject(handle);
    dwgBufferW *sb, *hb;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(
                definition.kind == DRW_UnderlayDefinition::DGN
                    ? "AcDbDgnDefinition"
                    : definition.kind == DRW_UnderlayDefinition::DWF
                        ? "AcDbDwfDefinition"
                        : "AcDbPdfDefinition",
                definition.kind == DRW_UnderlayDefinition::DGN
                    ? "DGNDEFINITION"
                    : definition.kind == DRW_UnderlayDefinition::DWF
                        ? "DWFDEFINITION"
                        : "PDFDEFINITION",
                sourceClassNum),
            handle, m_objectStrings, m_objectHandles, sb, hb,
            definition.reactorCount(), definition.extensionDictionaryFlag(),
            definition.hasDataStorageBinaryData(), &definition.extData,
            &appIdRefs, &layerRefs, fileCodePageId())) {
        return false;
    }
    if (!definition.encodeDwg(m_version, &body, sb, hb))
        return false;

    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeUnderlayDefinition(
    const DRW_UnderlayDefinition& definition) {
    if (m_version < DRW::AC1015
        || definition.reactorHandles.size() > dwgSafety::MaxReactorCount
        || definition.xDictFlag > 1
        || underlayDefinitionClassNum(definition) == 0)
        return false;

    DRW_UnderlayDefinition object = definition;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0) {
        object.handle = m_handles.next();
    } else {
        m_handles.reserve(object.handle);
    }
    if (!registerUnderlayDefinitionObjectClass(object.kind, object.handle))
        return false;

    if (!emitUnderlayDefinitionObject(object.handle, object, appIdRefs,
                                      layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    return true;
}

bool dwgWriter15::writeSection(const DRW_Section& section) {
    if (m_version < DRW::AC1021 || section.hasDataStorageBinaryData())
        return false;

    DRW_Section object = section;
    if ((object.m_kind != DRW_Section::Manager
         && object.m_kind != DRW_Section::Settings)
        || !prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerSectionObjectClass(object.m_kind, object.handle))
        return false;
    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(
                object.m_kind == DRW_Section::Manager
                    ? "AcDbSectionManager" : "AcDbSectionSettings",
                object.m_kind == DRW_Section::Manager
                    ? "SECTION_MANAGER" : "SECTION_SETTINGS",
                object.m_kind == DRW_Section::Manager
                    ? DRW_Section::kDwgClassNumManager
                    : DRW_Section::kDwgClassNumSettings),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeBackground(const DRW_Background& background) {
    const int kind = static_cast<int>(background.m_kind);
    if (m_version < DRW::AC1015
        || kind < static_cast<int>(DRW_Background::Solid)
        || kind > static_cast<int>(DRW_Background::Skylight)
        || background.reactorHandles.size() > dwgSafety::MaxReactorCount
        || background.extensionDictionaryFlag() > 1
        || background.hasDataStorageBinaryData())
        return false;

    DRW_Background object = background;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerBackgroundObjectClass(object.m_kind, object.handle))
        return false;

    const char* className = "AcDbSolidBackground";
    const char* recordName = "SOLIDBACKGROUND";
    std::uint16_t fallbackClassNum = DRW_Background::kDwgClassNumSolid;
    switch (object.m_kind) {
    case DRW_Background::Gradient:
        className = "AcDbGradientBackground";
        recordName = "GRADIENTBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumGradient;
        break;
    case DRW_Background::GroundPlane:
        className = "AcDbGroundPlaneBackground";
        recordName = "GROUNDPLANEBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumGroundPlane;
        break;
    case DRW_Background::Image:
        className = "AcDbImageBackground";
        recordName = "IMAGEBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumImage;
        break;
    case DRW_Background::Ibl:
        className = "AcDbIBLBackground";
        recordName = "IBLBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumIbl;
        break;
    case DRW_Background::Skylight:
        className = "AcDbSkyBackground";
        recordName = "SKYLIGHTBACKGROUND";
        fallbackClassNum = DRW_Background::kDwgClassNumSkylight;
        break;
    case DRW_Background::Solid:
        break;
    }

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(className, recordName, fallbackClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeSunStudy(const DRW_SunStudy& study) {
    if (m_version < DRW::AC1015 || study.hasDataStorageBinaryData())
        return false;

    DRW_SunStudy object = study;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerSunStudyObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbSunStudy", "SUNSTUDY",
                             DRW_SunStudy::kDwgClassNum),
            object.handle,
            m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeMotionPath(const DRW_MotionPath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_MotionPath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerMotionPathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbMotionPath", "MOTIONPATH",
                             DRW_MotionPath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeCurvePath(const DRW_CurvePath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_CurvePath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerCurvePathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbCurvePath", "CURVEPATH",
                             DRW_CurvePath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePointPath(const DRW_PointPath& path) {
    if (m_version < DRW::AC1015
        || path.reactorHandles.size() > dwgSafety::MaxReactorCount
        || path.extensionDictionaryFlag() > 1
        || path.hasDataStorageBinaryData())
        return false;

    DRW_PointPath object = path;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerPointPathObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPointPath", "POINTPATH",
                             DRW_PointPath::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeObjectPtr(const DRW_ObjectPtr& objectPtr) {
    if (m_version < DRW::AC1015 || objectPtr.hasDataStorageBinaryData())
        return false;

    DRW_ObjectPtr object = objectPtr;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerObjectPtrObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbObjectPtr", "OBJECT_PTR",
                             DRW_ObjectPtr::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writePartialViewingIndex(
    const DRW_PartialViewingIndex& index) {
    if (m_version < DRW::AC1015 || index.hasDataStorageBinaryData())
        return false;

    DRW_PartialViewingIndex object = index;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerPartialViewingIndexObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbPartialViewingIndex", "PARTIAL_VIEWING_INDEX",
                             DRW_PartialViewingIndex::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeRenderSettings(const DRW_RenderSettings& settings) {
    if (m_version < DRW::AC1015 || settings.hasDataStorageBinaryData())
        return false;

    DRW_RenderSettings object = settings;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerRenderSettingsObjectClass(object.m_kind, object.handle))
        return false;

    const char *className = nullptr;
    const char *recordName = nullptr;
    std::uint16_t fallbackClassNum = 0;
    switch (object.m_kind) {
    case DRW_RenderSettings::Settings:
        className = "AcDbRenderSettings";
        recordName = "RENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumSettings;
        break;
    case DRW_RenderSettings::MentalRay:
        className = "AcDbMentalRayRenderSettings";
        recordName = "MENTALRAYRENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumMentalRay;
        break;
    case DRW_RenderSettings::RapidRT:
        className = "AcDbRapidRTRenderSettings";
        recordName = "RAPIDRTRENDERSETTINGS";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumRapidRT;
        break;
    case DRW_RenderSettings::Entry:
        className = "AcDbRenderEntry";
        recordName = "RENDERENTRY";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumEntry;
        break;
    case DRW_RenderSettings::Environment:
        className = "AcDbRenderEnvironment";
        recordName = "RENDERENVIRONMENT";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumEnvironment;
        break;
    case DRW_RenderSettings::Global:
        className = "AcDbRenderGlobal";
        recordName = "RENDERGLOBAL";
        fallbackClassNum = DRW_RenderSettings::kDwgClassNumGlobal;
        break;
    default:
        return false;
    }

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum(className, recordName, fallbackClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

bool dwgWriter15::writeVisualStyle(const DRW_VisualStyle& style) {
    if (m_version < DRW::AC1015 || style.hasDataStorageBinaryData())
        return false;

    DRW_VisualStyle object = style;
    if (!prepareDwgCommonObjectState(object, m_version))
        return false;
    std::vector<DRW_Entity::PendingHandleRef> appIdRefs;
    std::vector<DRW_Entity::PendingHandleRef> layerRefs;
    if (!prepareTableEntryEed(object, appIdRefs, layerRefs)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    if (object.handle == 0)
        object.handle = m_handles.next();
    else
        m_handles.reserve(object.handle);
    if (!registerVisualStyleObjectClass(object.handle))
        return false;

    dwgBufferW& body = beginObject(object.handle);
    dwgBufferW *sb = nullptr;
    dwgBufferW *hb = nullptr;
    if (!emitRecordPreamble(
            body, m_version,
            typedDwgClassNum("AcDbVisualStyle", "VISUALSTYLE",
                             DRW_VisualStyle::kDwgClassNum),
            object.handle, m_objectStrings, m_objectHandles, sb, hb,
            object.reactorCount(), object.extensionDictionaryFlag(),
            object.hasDataStorageBinaryData(), &object.extData, &appIdRefs,
            &layerRefs, fileCodePageId())
        || !object.encodeDwg(m_version, &body, sb, hb)) {
        m_objectEedWriteFailure = !object.extData.empty();
        return false;
    }
    finishObject();
    return !objectWriteFailed();
}

// --- add*() methods: register user table records for deferred emission -----

std::uint32_t dwgWriter15::addLType(const DRW_LType& lt) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(lt.name);
    if (upper.empty())
        return 0;
    const auto existing = m_writingCtx.ltypeMap.find(upper);
    if (existing != m_writingCtx.ltypeMap.end()) {
        if (existing->second == reservedHandle::LTYPE_BYBLOCK) {
            m_ltypeByBlock = lt;
            m_haveLtypeByBlock = true;
        } else if (existing->second == reservedHandle::LTYPE_BYLAYER) {
            m_ltypeByLayer = lt;
            m_haveLtypeByLayer = true;
        } else if (existing->second == reservedHandle::LTYPE_CONTINUOUS) {
            m_ltypeContinuous = lt;
            m_haveLtypeContinuous = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.ltypeMap[upper] = h;
    m_pendingLTypes.emplace_back(h, lt);
    return h;
}

std::uint32_t dwgWriter15::addLayer(const DRW_Layer& lay) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(lay.name);
    if (upper == "0") {
        // "0" is the reserved layer at fixed handle 0x12; keep its real data
        // (plot flag, color, linetype, ...) so it is emitted faithfully rather
        // than as a default stub. Do not add it to m_pendingLayers (avoids a
        // duplicate record at a second handle).
        m_layer0 = lay;
        m_haveLayer0 = true;
        return DRW::DwgLayer0Handle;
    }
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.layerMap.find(upper);
        existing != m_writingCtx.layerMap.end())
        return existing->second;
    std::uint32_t h = m_handles.next();
    m_writingCtx.layerMap[upper] = h;
    m_pendingLayers.emplace_back(h, lay);
    return h;
}

std::uint32_t dwgWriter15::addTextstyle(const DRW_Textstyle& ts) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ts.name);
    if (upper.empty())
        return 0;
    const auto existing = m_writingCtx.styleMap.find(upper);
    if (existing != m_writingCtx.styleMap.end()) {
        if (existing->second == DRW::DwgStandardTextStyleHandle) {
            m_standardStyle = ts;
            m_haveStandardStyle = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.styleMap[upper] = h;
    m_pendingStyles.emplace_back(h, ts);
    return h;
}

bool dwgWriter15::addUcs(DRW_UCS& ucs) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(ucs.name);
    if (upper.empty())
        return false;
    const auto existing = m_writingCtx.ucsMap.find(upper);
    if (existing != m_writingCtx.ucsMap.end()) {
        ucs.handle = existing->second;
        return true;
    }
    std::uint32_t h = m_handles.next();
    ucs.handle = h;
    m_writingCtx.ucsMap[upper] = h;
    m_pendingUcs.emplace_back(h, ucs);
    return true;
}

bool dwgWriter15::addView(DRW_View& view) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(view.name);
    if (upper.empty())
        return false;
    const auto existing = m_writingCtx.viewMap.find(upper);
    if (existing != m_writingCtx.viewMap.end()) {
        view.handle = existing->second;
        return true;
    }
    std::uint32_t h = m_handles.next();
    view.handle = h;
    m_writingCtx.viewMap[upper] = h;
    m_pendingViews.emplace_back(h, view);
    return true;
}

bool dwgWriter15::addVport(DRW_Vport& vp) {
    if (m_dwgTableRecordsComplete)
        return false;
    std::string upper = toUpperCase(vp.name);
    if (upper.empty())
        return false;

    // Every VPORT table record is owned by the fixed VPORT control object;
    // source-file ownership is not part of the output namespace.
    vp.parentHandle = DRW_Vport::kDwgControlHandle;
    const auto existing = m_writingCtx.vportMap.find(upper);
    if (existing != m_writingCtx.vportMap.end()) {
        vp.handle = existing->second;
        if (existing->second == DRW_Vport::kDwgActiveHandle) {
            m_activeVport = vp;
            m_haveActiveVport = true;
        }
        return true;
    }
    std::uint32_t h = m_handles.next();
    vp.handle = h;
    m_writingCtx.vportMap[upper] = h;
    m_pendingVports.emplace_back(h, vp);
    return true;
}

bool dwgWriter15::addViewportEntityHeader(
    DRW_ViewportEntityHeader& header) {
    if (m_version < DRW::AC1015 || m_dwgTableRecordsComplete
        || (m_version > DRW::AC1024
            && header.hasDataStorageBinaryData()))
        return false;

    if (header.reactorHandles.size() > dwgSafety::MaxReactorCount
        || header.extensionDictionaryFlag() > 1) {
        return false;
    }
    if (header.handle == 0)
        header.handle = m_handles.next();
    else
        m_handles.reserve(header.handle);

    DRW_ViewportEntityHeader accepted = header;

    const auto duplicate = std::find_if(
        m_pendingViewportEntityHeaders.cbegin(),
        m_pendingViewportEntityHeaders.cend(),
        [&header](const auto& entry) { return entry.first == header.handle; });
    if (duplicate == m_pendingViewportEntityHeaders.cend())
        m_pendingViewportEntityHeaders.emplace_back(header.handle,
                                                    std::move(accepted));
    return true;
}

std::uint32_t dwgWriter15::addDimstyle(const DRW_Dimstyle& ds) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ds.name);
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.dimstyleMap.find(upper);
        existing != m_writingCtx.dimstyleMap.end()) {
        if (existing->second == DRW::DwgStandardDimstyleHandle) {
            m_standardDimstyle = ds;
            m_haveStandardDimstyle = true;
        }
        return existing->second;
    }
    std::uint32_t h = m_handles.next();
    m_writingCtx.dimstyleMap[upper] = h;
    m_pendingDimstyles.emplace_back(h, ds);
    return h;
}

std::uint32_t dwgWriter15::addAppId(const DRW_AppId& ai) {
    if (m_dwgTableRecordsComplete)
        return 0;
    std::string upper = toUpperCase(ai.name);
    if (upper.empty())
        return 0;
    if (const auto existing = m_writingCtx.appidMap.find(upper);
        existing != m_writingCtx.appidMap.end())
        return existing->second;
    std::uint32_t h = m_handles.next();
    m_writingCtx.appidMap[upper] = h;
    m_pendingAppIds.emplace_back(h, ai);
    return h;
}

bool dwgWriter15::collectPendingTableEedAppIds() {
    const auto collect = [this](const DRW_TableEntry& entry) {
        for (const DRW_Variant* value : entry.extData) {
            if (value == nullptr)
                return false;
            if (value->code() != 1001)
                continue;
            if (value->type() != DRW_Variant::STRING
                || value->content.s == nullptr || value->content.s->empty()) {
                return false;
            }
            DRW_AppId appId;
            appId.name = *value->content.s;
            if (addAppId(appId) == 0)
                return false;
        }
        return true;
    };
    const auto collectAll = [&collect](const auto& entries) {
        for (const auto& entry : entries) {
            if (!collect(entry.second))
                return false;
        }
        return true;
    };

    return (!m_haveLayer0 || collect(m_layer0))
        && (!m_haveLtypeByBlock || collect(m_ltypeByBlock))
        && (!m_haveLtypeByLayer || collect(m_ltypeByLayer))
        && (!m_haveLtypeContinuous || collect(m_ltypeContinuous))
        && (!m_haveStandardStyle || collect(m_standardStyle))
        && (!m_haveActiveVport || collect(m_activeVport))
        && (!m_haveStandardDimstyle || collect(m_standardDimstyle))
        && collectAll(m_pendingLTypes)
        && collectAll(m_pendingLayers)
        && collectAll(m_pendingStyles)
        && collectAll(m_pendingUcs)
        && collectAll(m_pendingViews)
        && collectAll(m_pendingVports)
        && collectAll(m_pendingViewportEntityHeaders)
        && collectAll(m_pendingDimstyles)
        && [&]() {
            const std::size_t appIdCount = m_pendingAppIds.size();
            for (std::size_t index = 0; index < appIdCount; ++index) {
                const DRW_AppId appId = m_pendingAppIds[index].second;
                if (!collect(appId))
                    return false;
            }
            return true;
        }();
}

bool dwgWriter15::prepareEntityEed(DRW_Entity& entity) {
    entity.dwgEedCodePage = fileCodePageId();
    return prepareDwgEedRefs(m_version, entity.extData, m_writingCtx,
                              entity.dwgEedAppIdWriteRefs,
                              entity.dwgEedLayerWriteRefs);
}

bool dwgWriter15::prepareTableEntryEed(
    const DRW_TableEntry& entry,
    std::vector<DRW_Entity::PendingHandleRef>& appIdRefs,
    std::vector<DRW_Entity::PendingHandleRef>& layerRefs) const {
    return prepareDwgEedRefs(m_version, entry.extData, m_writingCtx, appIdRefs,
                              layerRefs);
}

void dwgWriter15::prepareBlockOwnedEntity(DRW_Entity& entity) {
    if (auto* insert = dynamic_cast<DRW_Insert*>(&entity)) {
        if (insert->blockRecH.ref == 0 && !insert->name.empty()) {
            const auto block = m_userBlockHandles.find(toUpperCase(insert->name));
            if (block != m_userBlockHandles.end())
                insert->blockRecH.ref = block->second;
        }
    }
    if (entity.parentHandle == DRW::NoHandle) {
        if (m_activeUserBlockRecordHandle != 0)
            entity.parentHandle = m_activeUserBlockRecordHandle;
    } else if (m_activeUserBlockRecordHandle == 0
               && (entity.parentHandle == reservedHandle::BLOCK_MODEL_SPACE
                   || entity.parentHandle == reservedHandle::BLOCK_PAPER_SPACE)) {
        // Modelspace/paperspace entities are indexed by their BLOCK_RECORD,
        // but their common entity header uses ownerless entmode 2/1.
        entity.parentHandle = DRW::NoHandle;
    }
}

bool dwgWriter15::canRecordBlockOwnedEntity(const DRW_Entity& entity) const {
    if (entity.parentHandle == DRW::NoHandle)
        return true;

    const bool isCompoundChild = isCompoundEntityChild(entity);
    if (isCompoundChild && m_activeUserBlockRecordHandle != 0)
        return true;

    const auto owner = std::find_if(
        m_userBlocks.cbegin(), m_userBlocks.cend(),
        [&entity](const PendingUserBlock& block) {
            return block.blockRecordHandle == entity.parentHandle;
        });
    if (owner == m_userBlocks.cend()) {
        return isCompoundChild;
    }
    return m_activeUserBlockRecordHandle == 0
        || m_activeUserBlockRecordHandle == entity.parentHandle;
}

bool dwgWriter15::recordBlockOwnedEntity(const DRW_Entity& entity) {
    std::vector<std::uint32_t>* handles = nullptr;
    const bool isCompoundChild = isCompoundEntityChild(entity);
    if (isCompoundChild && m_activeUserBlockRecordHandle != 0) {
        const auto owner = std::find_if(
            m_userBlocks.begin(), m_userBlocks.end(),
            [this](const PendingUserBlock& block) {
                return block.blockRecordHandle == m_activeUserBlockRecordHandle;
            });
        if (owner == m_userBlocks.end())
            return false;
        handles = &owner->entityHandles;
    } else if (entity.parentHandle == DRW::NoHandle
               || (isCompoundChild
                   && std::find_if(m_userBlocks.cbegin(), m_userBlocks.cend(),
                                   [&entity](const PendingUserBlock& block) {
                                       return block.blockRecordHandle
                                           == entity.parentHandle;
                                   }) == m_userBlocks.cend())) {
        handles = entity.space == DRW::PaperSpace
            ? &m_paperSpaceEntityHandles : &m_modelSpaceEntityHandles;
    } else {
        const auto owner = std::find_if(
            m_userBlocks.begin(), m_userBlocks.end(),
            [&entity](const PendingUserBlock& block) {
                return block.blockRecordHandle == entity.parentHandle;
            });
        if (owner == m_userBlocks.end()
            || (m_activeUserBlockRecordHandle != 0
                && m_activeUserBlockRecordHandle != entity.parentHandle))
            return false;
        handles = &owner->entityHandles;
    }

    if (std::find(handles->cbegin(), handles->cend(), entity.handle)
        != handles->cend()) {
        return false;
    }
    handles->push_back(entity.handle);
    return true;
}

bool dwgWriter15::recordBlockInsertReference(const DRW_Entity& entity) {
    if (m_version <= DRW::AC1014)
        return true;

    const auto* insert = dynamic_cast<const DRW_Insert*>(&entity);
    if (insert == nullptr)
        return true;
    if (insert->blockRecH.ref == DRW::NoHandle)
        return false;

    std::vector<std::uint32_t>* handles = nullptr;
    if (insert->blockRecH.ref == reservedHandle::BLOCK_MODEL_SPACE) {
        handles = &m_modelSpaceInsertHandles;
    } else if (insert->blockRecH.ref == reservedHandle::BLOCK_PAPER_SPACE) {
        handles = &m_paperSpaceInsertHandles;
    } else {
        const auto owner = std::find_if(
            m_userBlocks.begin(), m_userBlocks.end(),
            [insert](const PendingUserBlock& block) {
                return block.blockRecordHandle == insert->blockRecH.ref;
            });
        if (owner == m_userBlocks.end())
            return false;
        handles = &owner->insertHandles;
    }

    if (handles->size() >= dwgSafety::MaxOwnedObjectCount
        || std::find(handles->cbegin(), handles->cend(), entity.handle)
               != handles->cend())
        return false;
    handles->push_back(entity.handle);
    return true;
}

bool dwgWriter15::canRecordRawBlockOwnedEntity(
    const DRW_UnsupportedObject& object) const {
    if (!isReplayableCustomRawEntity(object))
        return false;
    if (object.m_blockOwnerHandle != DRW::NoHandle
        && object.m_parentHandle == DRW::NoHandle
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_MODEL_SPACE
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_PAPER_SPACE)
        return false;
    const std::uint32_t blockOwnerHandle = rawBlockOwnerHandle(object);
    if (blockOwnerHandle == reservedHandle::BLOCK_MODEL_SPACE
        || blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE) {
        const auto& handles = blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
            ? m_paperSpaceEntityHandles : m_modelSpaceEntityHandles;
        return std::find(handles.cbegin(), handles.cend(), object.m_handle)
            == handles.cend();
    }
    if (blockOwnerHandle == DRW::NoHandle) {
        return std::find(m_modelSpaceEntityHandles.cbegin(),
                         m_modelSpaceEntityHandles.cend(), object.m_handle)
            == m_modelSpaceEntityHandles.cend();
    }
    const auto owner = std::find_if(
        m_userBlocks.cbegin(), m_userBlocks.cend(),
        [blockOwnerHandle](const PendingUserBlock& block) {
            return block.blockRecordHandle == blockOwnerHandle;
        });
    if (owner == m_userBlocks.cend())
        return false;
    return std::find(owner->entityHandles.cbegin(), owner->entityHandles.cend(),
                     object.m_handle) == owner->entityHandles.cend();
}

bool dwgWriter15::recordRawBlockOwnedEntity(
    const DRW_UnsupportedObject& object) {
    if (!canRecordRawBlockOwnedEntity(object))
        return false;
    const std::uint32_t blockOwnerHandle = rawBlockOwnerHandle(object);
    if (blockOwnerHandle == reservedHandle::BLOCK_MODEL_SPACE
        || blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
        || blockOwnerHandle == DRW::NoHandle) {
        auto& handles = blockOwnerHandle == reservedHandle::BLOCK_PAPER_SPACE
            ? m_paperSpaceEntityHandles : m_modelSpaceEntityHandles;
        handles.push_back(object.m_handle);
        return true;
    }
    const auto owner = std::find_if(
        m_userBlocks.begin(), m_userBlocks.end(),
        [blockOwnerHandle](PendingUserBlock& block) {
            return block.blockRecordHandle == blockOwnerHandle;
        });
    if (owner == m_userBlocks.end())
        return false;
    owner->entityHandles.push_back(object.m_handle);
    return true;
}

bool dwgWriter15::encodeEntity(DRW_Entity *ent) {
    if (ent == nullptr || objectWriteFailed() || m_blockControlEmitted)
        return false;

    const CompoundWriteCheckpoint checkpoint = checkpointCompoundWrite();
    const auto fail = [this, &checkpoint]() {
        rollbackCompoundWrite(checkpoint);
        return false;
    };

    if (!prepareEntityEed(*ent)) {
        m_entityEedWriteFailure = !ent->extData.empty();
        return fail();
    }
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
            : DRW::DwgLayer0Handle;  // fallback: layer "0"
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
    // R2000+ INSERT records must identify an existing BLOCK_RECORD. The
    // legacy name-only representation is retained for R14 and earlier, but
    // silently emitting a zero target in modern DWG would orphan the INSERT.
    if (m_version > DRW::AC1014) {
        const auto* insert = dynamic_cast<const DRW_Insert*>(ent);
        if (insert != nullptr && insert->blockRecH.ref == DRW::NoHandle)
            return fail();
    }
    if (!canRecordBlockOwnedEntity(*ent))
        return fail();

    dwgBufferW& body = beginObject(handle);
    bool ok = false;
    if (m_version > DRW::AC1018) {
        m_objectStrings.reset();
        m_objectHandles.reset();
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0,
                             &m_objectStrings, &m_objectHandles);
    } else if (m_version > DRW::AC1014) {
        m_objectHandles.reset();
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0,
                             nullptr, &m_objectHandles);
    } else {
        ok = ent->encodeDwg(m_version, &body, /*bs=*/0);
    }
    if (!ok) {
        if (!ent->extData.empty())
            m_entityEedWriteFailure = true;
        return fail();
    }
    finishObject();

    if (objectWriteFailed())
        return fail();

    if (!recordBlockOwnedEntity(*ent))
        return fail();
    if (!recordBlockInsertReference(*ent))
        return fail();
    return true;
}

bool dwgWriter15::writeDwgObjects() {
    if (m_dwgTableRecordsComplete)
        return true;

    struct DwgObjectsPhaseGuard final {
        dwgWriter15* writer;
        CompoundWriteCheckpoint checkpoint;
        bool committed {false};

        ~DwgObjectsPhaseGuard() {
            if (!committed)
                writer->rollbackCompoundWrite(checkpoint);
        }
    } phaseGuard{this, checkpointCompoundWrite()};

    m_emittedDwgTableRecordHandles.clear();
    m_emittedDwgTableControlHandles.clear();
    m_dwgTableRecordsComplete = false;
    m_emittedDwgNamedObjectDictionaryEntries.clear();
    m_dwgNamedObjectDictionaryComplete = false;
    m_emittedDwgBlockRecords.clear();
    m_emittedDwgBlockEntityHandles.clear();
    m_dwgBlockStructureComplete = false;

    // Ensure standard reserved entries are present.  The constructor
    // pre-seeds these; insert is a no-op if a key already exists, which
    // protects user entries added via add*() before this call.
    m_writingCtx.ltypeMap.insert({"CONTINUOUS", reservedHandle::LTYPE_CONTINUOUS});
    m_writingCtx.ltypeMap.insert({"BYLAYER",    reservedHandle::LTYPE_BYLAYER});
    m_writingCtx.ltypeMap.insert({"BYBLOCK",    reservedHandle::LTYPE_BYBLOCK});
    m_writingCtx.layerMap.insert({"0",        DRW::DwgLayer0Handle});
    m_writingCtx.styleMap.insert({"STANDARD", DRW::DwgStandardTextStyleHandle});
    // Named VIEW has no required standard record.
    m_writingCtx.appidMap.insert({"ACAD",     DRW::DwgAcadAppIdHandle});
    m_writingCtx.dimstyleMap.insert({"STANDARD", DRW::DwgStandardDimstyleHandle});
    m_writingCtx.vportMap.insert({"*ACTIVE", DRW::DwgActiveVportHandle});

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
        if (!emitControlObject(
                DRW::DwgLTypeControlObjectType, reservedHandle::LTYPE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingLTypes.size()),
                ltypeChildren))
            return false;
        DRW_LType byBlock = m_haveLtypeByBlock ? m_ltypeByBlock : DRW_LType{};
        DRW_LType byLayer = m_haveLtypeByLayer ? m_ltypeByLayer : DRW_LType{};
        DRW_LType continuous = m_haveLtypeContinuous ? m_ltypeContinuous
                                                      : DRW_LType{};
        byBlock.name = "BYBLOCK";
        byLayer.name = "BYLAYER";
        continuous.name = "CONTINUOUS";
        if (!emitLtypeRecord(reservedHandle::LTYPE_BYBLOCK, byBlock)
            || !emitLtypeRecord(reservedHandle::LTYPE_BYLAYER, byLayer)
            || !emitLtypeRecord(reservedHandle::LTYPE_CONTINUOUS, continuous)) {
            return false;
        }
        for (auto& p : m_pendingLTypes)
            if (!emitLtypeRecord(p.first, p.second))
                return false;
    }

    // --- LAYER section ---
    {
        std::vector<std::uint32_t> layerChildren = {DRW::DwgLayer0Handle};
        for (auto& p : m_pendingLayers) layerChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgLayerControlObjectType, reservedHandle::LAYER_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingLayers.size()),
                layerChildren))
            return false;
        if (m_haveLayer0) {
            if (!emitLayerRecord(DRW::DwgLayer0Handle, m_layer0))
                return false;
        } else {
            DRW_Layer layer0;
            layer0.name = "0";
            if (!emitLayerRecord(DRW::DwgLayer0Handle, layer0))
                return false;
        }
        for (auto& p : m_pendingLayers)
            if (!emitLayerRecord(p.first, p.second))
                return false;
    }

    // --- STYLE section ---
    {
        std::vector<std::uint32_t> styleChildren = {
            DRW::DwgStandardTextStyleHandle};
        for (auto& p : m_pendingStyles) styleChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgStyleControlObjectType, reservedHandle::STYLE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingStyles.size()),
                styleChildren))
            return false;
        DRW_Textstyle standardStyle = m_haveStandardStyle
            ? m_standardStyle : DRW_Textstyle{};
        standardStyle.name = "STANDARD";
        if (!emitStyleRecord(DRW::DwgStandardTextStyleHandle, standardStyle))
            return false;
        for (auto& p : m_pendingStyles)
            if (!emitStyleRecord(p.first, p.second))
                return false;
    }

    // --- VIEW section --- (named views; no required standard record)
    {
        std::vector<std::uint32_t> viewChildren;
        for (auto& p : m_pendingViews) viewChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgViewControlObjectType, reservedHandle::VIEW_CONTROL,
                static_cast<std::uint32_t>(m_pendingViews.size()),
                viewChildren))
            return false;
        for (auto& p : m_pendingViews)
            if (!emitViewRecord(p.first, p.second))
                return false;
    }

    // --- UCS section --- (named UCS records; no required standard record)
    {
        std::vector<std::uint32_t> ucsChildren;
        for (auto& p : m_pendingUcs) ucsChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgUcsControlObjectType, reservedHandle::UCS_CONTROL,
                static_cast<std::uint32_t>(m_pendingUcs.size()),
                ucsChildren))
            return false;
        for (auto& p : m_pendingUcs)
            if (!emitUcsRecord(p.first, p.second))
                return false;
    }

    // --- VPORT section ---
    {
        std::vector<std::uint32_t> vportChildren = {
            DRW_Vport::kDwgActiveHandle};
        for (auto& p : m_pendingVports) vportChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgVPortControlObjectType, reservedHandle::VPORT_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingVports.size()),
                vportChildren))
            return false;
        DRW_Vport activeVport = m_haveActiveVport
            ? m_activeVport : DRW_Vport{};
        activeVport.name = "*ACTIVE";
        if (!emitVportRecord(DRW_Vport::kDwgActiveHandle, activeVport))
            return false;
        for (auto& p : m_pendingVports)
            if (!emitVportRecord(p.first, p.second))
                return false;
    }

    // --- APPID section ---
    {
        std::vector<std::uint32_t> appidChildren = {DRW::DwgAcadAppIdHandle};
        for (auto& p : m_pendingAppIds) appidChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgAppIdControlObjectType, reservedHandle::APPID_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingAppIds.size()),
                appidChildren))
            return false;
        DRW_AppId acadAppId;
        acadAppId.name = "ACAD";
        if (!emitAppIdRecord(DRW::DwgAcadAppIdHandle, acadAppId))
            return false;
        for (auto& p : m_pendingAppIds)
            if (!emitAppIdRecord(p.first, p.second))
                return false;
    }

    // --- DIMSTYLE section ---
    {
        std::vector<std::uint32_t> dimChildren = {
            DRW::DwgStandardDimstyleHandle};
        for (auto& p : m_pendingDimstyles) dimChildren.push_back(p.first);
        if (!emitControlObject(
                DRW::DwgDimStyleControlObjectType, reservedHandle::DIMSTYLE_CONTROL,
                1 + static_cast<std::uint32_t>(m_pendingDimstyles.size()),
                dimChildren))
            return false;
        DRW_Dimstyle standardDimstyle = m_haveStandardDimstyle
            ? m_standardDimstyle : DRW_Dimstyle{};
        standardDimstyle.name = "STANDARD";
        standardDimstyle.syncStructToVars();
        if (!emitDimstyleRecord(DRW::DwgStandardDimstyleHandle,
                                standardDimstyle))
            return false;
        for (auto& p : m_pendingDimstyles)
            if (!emitDimstyleRecord(p.first, p.second))
                return false;
    }

    // VPORT_ENTITY_HEADER_CONTROL is a R2000-era table control.  R2004 and
    // later files omit it from both HEADER and OBJECTS, but fixed type-71
    // records remain valid table entries and are emitted below.
    if (m_version < DRW::AC1018
        && !hasRawObjectOverride(
            DRW_ViewportEntityHeader::kDwgControlType,
            DRW_ViewportEntityHeader::kDwgControlHandle)) {
        std::vector<std::uint32_t> viewportHeaderChildren;
        viewportHeaderChildren.reserve(m_pendingViewportEntityHeaders.size());
        for (const auto& entry : m_pendingViewportEntityHeaders)
            viewportHeaderChildren.push_back(entry.first);
        if (!emitControlObject(
                DRW_ViewportEntityHeader::kDwgControlType,
                reservedHandle::VPORT_ENTITY_HEADER_CONTROL,
                static_cast<std::uint32_t>(viewportHeaderChildren.size()),
                viewportHeaderChildren))
            return false;
    }
    for (const auto& entry : m_pendingViewportEntityHeaders)
        if (!emitViewportEntityHeaderRecord(entry.first, entry.second))
            return false;

    // Block entities + Block_Records for *Model_Space and *Paper_Space.
    if (!emitBlockEntity(reservedHandle::BLOCK_MODEL_ENTITY, "*Model_Space",
                         /*isEnd=*/false)
        || !emitBlockEntity(reservedHandle::BLOCK_MODEL_END, std::string{},
                            /*isEnd=*/true)
        || !emitBlockEntity(reservedHandle::BLOCK_PAPER_ENTITY, "*Paper_Space",
                            /*isEnd=*/false)
        || !emitBlockEntity(reservedHandle::BLOCK_PAPER_END, std::string{},
                            /*isEnd=*/true))
        return false;

    // The corresponding BLOCK_RECORD objects are deferred until
    // emitDeferredBlockControl(), after writeEntities() has populated their
    // owned-handle lists.

    DRW_Dictionary namedObjects;
    namedObjects.handle = DRW::DwgNamedObjectsDictionaryHandle;
    namedObjects.cloning = 1;
    namedObjects.hardOwner = 0;
    namedObjects.m_entries.push_back(
        {"ACAD_GROUP", DRW::DwgAcadGroupDictionaryHandle});
    for (const auto& entry : m_pendingDwgNamedObjectDictionaryEntries)
        namedObjects.m_entries.push_back({entry.name, entry.childHandle});

    DRW_Dictionary acadGroup;
    acadGroup.handle = DRW::DwgAcadGroupDictionaryHandle;
    acadGroup.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    acadGroup.cloning = 1;
    acadGroup.hardOwner = 0;
    const std::vector<DRW_Entity::PendingHandleRef> noEedRefs;
    if (!emitDictionaryObject(namedObjects.handle, namedObjects, noEedRefs,
                              noEedRefs)
        || !emitDictionaryObject(acadGroup.handle, acadGroup, noEedRefs,
                                 noEedRefs)) {
        return false;
    }
    m_emittedDwgNamedObjectDictionaryEntries.push_back(
        {"ACAD_GROUP", DRW::DwgAcadGroupDictionaryHandle});
    m_emittedDwgNamedObjectDictionaryEntries.insert(
        m_emittedDwgNamedObjectDictionaryEntries.end(),
        m_pendingDwgNamedObjectDictionaryEntries.cbegin(),
        m_pendingDwgNamedObjectDictionaryEntries.cend());
    m_dwgNamedObjectDictionaryComplete = true;

    m_dwgTableRecordsComplete = true;
    phaseGuard.committed = true;
    return true;
}

namespace {

/// Rebuild raw object body with a replacement OT when raw class nums remapped.
bool patchRawObjectType(const std::vector<std::uint8_t>& raw,
                        DRW::Version version,
                        std::uint16_t newType, std::vector<std::uint8_t>& out,
                        std::int64_t* bitDelta = nullptr) {
    if (raw.empty())
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (!reader.isGood())
        return false;
    const std::uint64_t oldTypeBits =
        reader.getPosition() * 8u + reader.getBitPos();
    dwgBufferW typeWriter;
    typeWriter.putObjType(version, newType);
    if (!typeWriter.isGood())
        return false;
    if (bitDelta != nullptr) {
        const std::uint64_t newTypeBits = typeWriter.bitCount();
        if (newTypeBits > std::numeric_limits<std::int64_t>::max()
            || oldTypeBits > std::numeric_limits<std::int64_t>::max())
            return false;
        *bitDelta = static_cast<std::int64_t>(newTypeBits)
            - static_cast<std::int64_t>(oldTypeBits);
    }
    dwgBufferW rebuilt;
    rebuilt.putObjType(version, newType);
    // Copy remaining bits after the original OT.
    while (reader.numRemainingBytes() > 0 || reader.getBitPos() != 0) {
            // Prefer whole remaining bytes when aligned.
        if (reader.getBitPos() == 0 && reader.numRemainingBytes() > 0) {
            const size_t n = reader.numRemainingBytes();
            std::vector<std::uint8_t> tail;
            if (!DRW::resize(tail, static_cast<int>(n)))
                return false;
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

bool readRawObjectHandle(const std::vector<std::uint8_t>& raw,
                         DRW::Version version, dwgHandle& handle) {
    if (raw.empty())
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        (void)reader.getRawLong32();
    if (!reader.isGood())
        return false;
    handle = reader.getHandle();
    return reader.isGood();
}

/// Replace the common object handle in a raw body while retaining every other
/// bit, including the padding after the meaningful R2007+ body bit count.
bool patchRawObjectHandle(const std::vector<std::uint8_t>& raw,
                          DRW::Version version, std::uint32_t newHandle,
                          std::vector<std::uint8_t>& out,
                          std::int64_t* bitDelta = nullptr) {
    if (raw.empty() || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;
    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        (void)reader.getRawLong32();
    if (!reader.isGood())
        return false;

    const std::uint64_t handleStart =
        reader.getPosition() * 8u + reader.getBitPos();
    const dwgHandle oldHandle = reader.getHandle();
    if (!reader.isGood())
        return false;
    const std::uint64_t handleEnd =
        reader.getPosition() * 8u + reader.getBitPos();
    const std::uint64_t totalBits = static_cast<std::uint64_t>(raw.size()) * 8u;
    if (handleEnd > totalBits)
        return false;

    dwgBufferW handleWriter;
    dwgHandle replacement = oldHandle;
    replacement.ref = newHandle;
    replacement.ref64 = newHandle;
    replacement.size = 0;
    handleWriter.putHandle(replacement);
    if (!handleWriter.isGood())
        return false;
    if (bitDelta != nullptr) {
        const std::uint64_t oldHandleBits = handleEnd - handleStart;
        const std::uint64_t newHandleBits = handleWriter.bitCount();
        if (newHandleBits > std::numeric_limits<std::int64_t>::max()
            || oldHandleBits > std::numeric_limits<std::int64_t>::max())
            return false;
        *bitDelta = static_cast<std::int64_t>(newHandleBits)
            - static_cast<std::int64_t>(oldHandleBits);
    }

    dwgBuffer prefixReader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    dwgBufferW rebuilt;
    for (std::uint64_t i = 0; i < handleStart; ++i) {
        const std::uint8_t bit = prefixReader.getBit();
        if (!prefixReader.isGood())
            return false;
        rebuilt.putBit(bit);
    }

    rebuilt.putHandle(replacement);

    for (std::uint64_t i = handleEnd; i < totalBits; ++i) {
        const std::uint8_t bit = reader.getBit();
        if (!reader.isGood())
            return false;
        rebuilt.putBit(bit);
    }
    rebuilt.alignToByte();
    out = rebuilt.data();
    return !out.empty();
}

struct RawEntityOwnerProbe final : DRW_Entity {
    void applyExtrusion() override {}

    bool parseDwg(DRW::Version version, dwgBuffer *buffer,
                  std::uint32_t bodyBitSize = 0) override {
        return DRW_Entity::parseDwg(version, buffer, nullptr, bodyBitSize)
            && buffer != nullptr && buffer->isGood();
    }

    bool hasOwnerHandle() const noexcept { return ownerHandle; }
    std::uint32_t objectDataBitSize() const noexcept { return objSize; }
};

std::uint64_t rawBitPosition(const dwgBuffer& buffer) {
    return buffer.getPosition() * 8u + buffer.getBitPos();
}

bool copyRawBits(const std::vector<std::uint8_t>& raw,
                 std::uint64_t begin, std::uint64_t end,
                 dwgBufferW& output) {
    if (begin > end
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u
        || end > static_cast<std::uint64_t>(raw.size()) * 8u)
        return false;

    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!reader.setPosition(begin >> 3))
        return false;
    reader.setBitPos(static_cast<std::uint8_t>(begin & 7u));
    if (!reader.isGood())
        return false;
    for (std::uint64_t bit = begin; bit < end; ++bit) {
        output.putBit(reader.getBit());
        if (!reader.isGood())
            return false;
    }
    return output.isGood();
}

/// Keep the embedded AC1015/AC1018/AC1021 object-data boundary synchronized
/// after a prefix rewrite changes the encoded bit width of OT or own-H.
bool adjustRawObjectDataBitSize(const std::vector<std::uint8_t>& raw,
                                DRW::Version version,
                                std::int64_t bitDelta,
                                std::vector<std::uint8_t>& out) {
    if (bitDelta == 0) {
        out = raw;
        return !out.empty();
    }
    if (version <= DRW::AC1014 || version >= DRW::AC1024 || raw.empty()
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;

    dwgBuffer reader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    (void)reader.getObjType(version);
    if (!reader.isGood())
        return false;
    const std::uint64_t sizeFieldBit = rawBitPosition(reader);
    if ((sizeFieldBit & 7u) != 2u)
        return false;
    const std::uint32_t currentBits = reader.getRawLong32();
    if (!reader.isGood())
        return false;
    const std::int64_t adjustedBits =
        static_cast<std::int64_t>(currentBits) + bitDelta;
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(raw.size()) * 8u;
    if (adjustedBits <= 0
        || static_cast<std::uint64_t>(adjustedBits) > totalBits
        || adjustedBits > std::numeric_limits<std::uint32_t>::max())
        return false;

    dwgBufferW rebuilt;
    if (!copyRawBits(raw, 0, totalBits, rebuilt))
        return false;
    rebuilt.patchRawLong32AtBit(
        static_cast<std::size_t>(sizeFieldBit),
        static_cast<std::uint32_t>(adjustedBits));
    rebuilt.alignToByte();
    if (!rebuilt.isGood() || rebuilt.data().empty())
        return false;
    out = rebuilt.data();
    return true;
}

/// Patch only the validated common owner handle of an AC1021+ entity.
/// R2000/R2004 do not expose a trustworthy opaque body/handle boundary, so
/// their owner remaps remain rejected by the caller.
bool patchRawEntityOwner(const std::vector<std::uint8_t>& raw,
                         DRW::Version version,
                         std::uint32_t sourceOwner,
                         std::uint32_t targetOwner,
                         std::uint32_t bodyBitSize,
                         std::vector<std::uint8_t>& out,
                         std::uint32_t& outputBodyBitSize) {
    if (version < DRW::AC1021 || sourceOwner == DRW::NoHandle
        || targetOwner == DRW::NoHandle || sourceOwner == targetOwner
        || raw.empty()
        || raw.size() > std::numeric_limits<std::uint64_t>::max() / 8u)
        return false;

    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(raw.size()) * 8u;
    RawEntityOwnerProbe probe;
    dwgBuffer common(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!probe.parseDwg(version, &common, bodyBitSize)
        || !common.isGood() || !probe.hasOwnerHandle())
        return false;

    const std::uint64_t ownerStart = probe.objectDataBitSize();
    if (ownerStart == 0 || ownerStart >= totalBits)
        return false;

    auto seekBits = [ownerStart](dwgBuffer& reader) {
        if (!reader.setPosition(ownerStart >> 3))
            return false;
        reader.setBitPos(static_cast<std::uint8_t>(ownerStart & 7u));
        return reader.isGood();
    };

    dwgBuffer resolvedReader(const_cast<std::uint8_t*>(raw.data()), raw.size());
    if (!seekBits(resolvedReader))
        return false;
    const dwgHandle resolvedOwner =
        resolvedReader.getOffsetHandle(probe.handle);
    if (!resolvedReader.isGood() || resolvedOwner.ref64 != sourceOwner)
        return false;
    const std::uint64_t ownerEnd = rawBitPosition(resolvedReader);
    if (ownerEnd <= ownerStart || ownerEnd > totalBits)
        return false;

    dwgBufferW rebuilt;
    if (!copyRawBits(raw, 0, ownerStart, rebuilt))
        return false;
    dwgHandle replacement;
    replacement.code = 4; // absolute soft-pointer form, as emitted by entities
    replacement.ref = targetOwner;
    replacement.ref64 = targetOwner;
    rebuilt.putHandle(replacement);
    if (!rebuilt.isGood() || !copyRawBits(raw, ownerEnd, totalBits, rebuilt))
        return false;
    rebuilt.alignToByte();
    if (!rebuilt.isGood() || rebuilt.data().empty())
        return false;

    out = rebuilt.data();
    if (version > DRW::AC1021) {
        const std::uint64_t dataBits = ownerStart;
        const std::uint64_t newTotalBits =
            static_cast<std::uint64_t>(out.size()) * 8u;
        if (newTotalBits < dataBits
            || newTotalBits - dataBits
                   > std::numeric_limits<std::uint32_t>::max())
            return false;
        outputBodyBitSize = static_cast<std::uint32_t>(newTotalBits - dataBits);
    } else {
        outputBodyBitSize = 0;
    }
    return true;
}

} // namespace

bool dwgWriter15::replayRawObject(const DRW_UnsupportedObject& object) {
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_currentDwgObjectFrameProvenance =
        m_pendingDwgObjectFrameProvenance;
    m_pendingDwgObjectFrameProvenance = {};
    if ((blockControlEmitted() && object.m_isEntity)
        || object.m_version != m_version
        || (object.m_isEntity && !isReplayableFixedModelerRawEntity(object)
            && !isReplayableFixedEntityShellRawEntity(object)
            && !isReplayableSurfaceRawEntity(object)
            && !isReplayableCustomRawEntity(object))
        || object.m_handle == 0 || object.m_rawBytes.empty()) {
        return false;
    }
    if (m_version > DRW::AC1021) {
        if (object.m_rawBytes.size()
            > std::numeric_limits<std::uint64_t>::max() / 8u
            || static_cast<std::uint64_t>(object.m_bodyBitSize)
                > static_cast<std::uint64_t>(object.m_rawBytes.size()) * 8u) {
            return false;
        }
    }
    if (object.m_objectSize != 0
        && static_cast<std::uint64_t>(object.m_objectSize)
            != static_cast<std::uint64_t>(object.m_rawBytes.size())) {
        return false;
    }
    if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                    [&object](const auto& entry) {
                        return entry.first == object.m_handle;
                    })) {
        return false;
    }

    const bool replayableCustomEntity = isReplayableCustomRawEntity(object);
    if (replayableCustomEntity
        && object.m_blockOwnerHandle != DRW::NoHandle
        && object.m_parentHandle == DRW::NoHandle
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_MODEL_SPACE
        && object.m_blockOwnerHandle != reservedHandle::BLOCK_PAPER_SPACE)
        return false;
    if (replayableCustomEntity && !canRecordRawBlockOwnedEntity(object))
        return false;

    m_handles.reserve(object.m_handle);
    const std::vector<DwgClassDefinition> classDefinitionsBefore =
        m_dwgClassDefinitions;
    const auto rawClassInstanceHandlesBefore = m_rawClassInstanceHandles;
    const auto classInstanceHandleOwnersBefore =
        m_dwgClassInstanceHandleOwners;
    const auto classInstanceEmittedHandlesBefore =
        m_dwgClassInstanceEmittedHandles;
    const auto classInstanceEmissionCountsBefore =
        m_dwgClassInstanceEmissionCounts;
    const auto rawClassNumRemapBefore = m_rawClassNumRemap;
    const auto rawObjectOverridesBefore = m_rawObjectOverrides;
    const bool classConflictBefore = m_hasDwgClassConflict;
    const std::size_t outputStart = m_buf.size();
    const std::size_t objectMapStart = m_objectMap.size();
    bool committed = false;
    const auto rollback = [&]() {
        if (committed)
            return;
        m_dwgClassDefinitions = classDefinitionsBefore;
        m_rawClassInstanceHandles = rawClassInstanceHandlesBefore;
        m_dwgClassInstanceHandleOwners = classInstanceHandleOwnersBefore;
        m_dwgClassInstanceEmittedHandles =
            classInstanceEmittedHandlesBefore;
        m_dwgClassInstanceEmissionCounts = classInstanceEmissionCountsBefore;
        m_rawClassNumRemap = rawClassNumRemapBefore;
        m_rawObjectOverrides = rawObjectOverridesBefore;
        m_hasDwgClassConflict = classConflictBefore;
        m_buf.truncate(outputStart);
        if (objectMapStart <= m_objectMap.size())
            m_objectMap.resize(objectMapStart);
    };
    const auto fail = [&]() {
        m_currentDwgObjectFrameProvenance = {};
        rollback();
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        return false;
    };
    if (!registerRawObjectClass(object))
        return fail();

    const std::uint16_t sourceType =
        static_cast<std::uint16_t>(object.m_objectType);
    const std::uint16_t writerType = remappedRawClassNum(object);
    if (m_currentDwgObjectFrameProvenance.classNumber != 0
        && m_currentDwgObjectFrameProvenance.classNumber != writerType)
        return fail();
    const std::vector<std::uint8_t>* bodyBytes = &object.m_rawBytes;
    std::vector<std::uint8_t> patched;
    std::vector<std::uint8_t> patchedHandle;
    std::vector<std::uint8_t> patchedDataSize;
    std::vector<std::uint8_t> patchedOwner;
    std::uint32_t bodyBitSize = object.m_bodyBitSize;
    std::int64_t objectDataBitDelta = 0;
    if (object.m_isCustomClass && sourceType >= 500 && writerType != sourceType) {
        std::int64_t bitDelta = 0;
        if (!patchRawObjectType(object.m_rawBytes, m_version, writerType,
                                patched, &bitDelta))
            return fail();
        bodyBytes = &patched;
        objectDataBitDelta += bitDelta;
    }
    dwgHandle encodedHandle;
    if (!readRawObjectHandle(*bodyBytes, m_version, encodedHandle))
        return fail();
    if (encodedHandle.ref64 != object.m_handle) {
        std::int64_t bitDelta = 0;
        if (!patchRawObjectHandle(*bodyBytes, m_version, object.m_handle,
                                  patchedHandle, &bitDelta))
            return fail();
        bodyBytes = &patchedHandle;
        objectDataBitDelta += bitDelta;
    }
    if (m_version < DRW::AC1024 && objectDataBitDelta != 0) {
        if (!adjustRawObjectDataBitSize(*bodyBytes, m_version,
                                        objectDataBitDelta, patchedDataSize))
            return fail();
        bodyBytes = &patchedDataSize;
    }
    const std::uint32_t sourceOwner = object.m_parentHandle;
    const std::uint32_t targetOwner = rawBlockOwnerHandle(object);
    if (replayableCustomEntity && sourceOwner != DRW::NoHandle
        && targetOwner != sourceOwner) {
        if (!patchRawEntityOwner(*bodyBytes, m_version, sourceOwner,
                                 targetOwner, bodyBitSize, patchedOwner,
                                 bodyBitSize))
            return fail();
        bodyBytes = &patchedOwner;
    }
    if (bodyBytes->empty())
        return fail();
    dwgBuffer typeReader(
        const_cast<std::uint8_t*>(bodyBytes->data()), bodyBytes->size());
    const std::uint16_t actualType = typeReader.getObjType(m_version);
    if (!typeReader.isGood() || actualType != writerType)
        return fail();
    if (bodyBytes->size() > DwgObjectFrame::MaxBodySize
        || bodyBytes->size() > static_cast<std::size_t>(
            std::numeric_limits<std::int32_t>::max())) {
        return fail();
    }
    if (m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        return fail();
    }

    const std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes->size()));
    if (!m_buf.isGood()) {
        return fail();
    }
    if (m_version > DRW::AC1021)
        m_buf.putUModularChar(bodyBitSize);
    if (!m_buf.isGood()) {
        return fail();
    }
    const size_t bodyStart = m_buf.size();
    m_buf.putBytes(bodyBytes->data(), bodyBytes->size());

    const std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                                    bodyStart + bodyBytes->size());
    m_buf.putRawShort16(crc);
    if (!m_buf.isGood()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        return fail();
    }
    m_objectMap.emplace_back(object.m_handle, frameStart);
    if (replayableCustomEntity && !recordRawBlockOwnedEntity(object)) {
        return fail();
    }
    markDwgClassInstanceEmitted(object.m_handle);
    if (m_nextDwgObjectFrameGeneration == 0
        || m_nextDwgObjectFrameGeneration
               == std::numeric_limits<std::uint64_t>::max()) {
        return fail();
    }
    m_lastDwgObjectFrame.valid = true;
    m_lastDwgObjectFrame.generation = m_nextDwgObjectFrameGeneration++;
    m_lastDwgObjectFrame.objectHandle = object.m_handle;
    m_lastDwgObjectFrame.version = m_version;
    m_lastDwgObjectFrame.dataBitSize = m_version > DRW::AC1021
        ? bodyBitSize : static_cast<std::uint64_t>(bodyBytes->size()) * 8u;
    m_lastDwgObjectFrame.handleBitSize = 0;
    m_lastDwgObjectFrame.classNumber = actualType;
    m_lastDwgObjectFrame.writerOperation =
        m_currentDwgObjectFrameProvenance.writerOperation;
    m_lastDwgObjectFrame.admissionToken =
        m_currentDwgObjectFrameProvenance.admissionToken;
    committed = true;
    m_lastDwgObjectFrameValid = true;
    m_currentDwgObjectFrameProvenance = {};
    return true;
}

dwgBufferW& dwgWriter15::beginObject(std::uint32_t handle) {
    m_currentHandle = handle;
    m_currentDwgObjectFrameProvenance =
        m_pendingDwgObjectFrameProvenance;
    m_pendingDwgObjectFrameProvenance = {};
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_objectBody.reset();
    m_objectStrings.reset();
    m_objectHandles.reset();
    return m_objectBody;
}

bool dwgWriter15::captureLastDwgObjectHandleOccurrences(
    std::uint64_t mergedStringBaseBit, std::uint64_t mergedHandleBaseBit,
    bool stringsMergedIntoData, bool handlesMergedIntoData) {
    DRW::DwgObjectFrameReceipt frame;
    frame.objectHandle = m_currentHandle;
    frame.version = m_version;
    frame.dataBitSize = m_objectBody.bitCount();
    frame.handleBitSize = handlesMergedIntoData
        ? 0 : m_objectHandles.bitCount();

    if (m_objectBody.data().empty()) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return false;
    }
    dwgBuffer typeReader(
        const_cast<std::uint8_t*>(m_objectBody.data().data()),
        m_objectBody.data().size());
    frame.classNumber = typeReader.getObjType(m_version);
    if (!typeReader.isGood()
        || (m_currentDwgObjectFrameProvenance.classNumber != 0
            && frame.classNumber
                   != m_currentDwgObjectFrameProvenance.classNumber)) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_currentDwgObjectFrameProvenance = {};
        return false;
    }
    frame.writerOperation =
        m_currentDwgObjectFrameProvenance.writerOperation;
    frame.admissionToken =
        m_currentDwgObjectFrameProvenance.admissionToken;

    std::uint32_t dataOrdinal = 0;
    std::uint32_t handleOrdinal = 0;
    bool valid = true;
    const auto append = [&frame, &valid](
            const dwgBufferW& buffer, DRW::DwgObjectHandleStream stream,
            DRW::DwgObjectSerializedSegment segment, std::uint64_t baseBit,
            std::uint32_t& segmentOrdinal) {
        for (const DRW::DwgHandleWriteOccurrence& token
             : buffer.handleOccurrences()) {
            if (token.endBit < token.startBit
                || baseBit > std::numeric_limits<std::uint64_t>::max()
                                  - token.endBit
                || segmentOrdinal == std::numeric_limits<std::uint32_t>::max()) {
                valid = false;
                return;
            }
            DRW::DwgHandleWriteOccurrence rebased = token;
            rebased.startBit += baseBit;
            rebased.endBit += baseBit;
            frame.occurrences.push_back(
                {frame.objectHandle, stream, segment, segmentOrdinal++, rebased});
        }
    };

    append(m_objectBody, DRW::DwgObjectHandleStream::Data,
           DRW::DwgObjectSerializedSegment::Data, 0, dataOrdinal);
    if (stringsMergedIntoData) {
        append(m_objectStrings, DRW::DwgObjectHandleStream::Strings,
               DRW::DwgObjectSerializedSegment::Data, mergedStringBaseBit,
               dataOrdinal);
    } else if (!m_objectStrings.handleOccurrences().empty()) {
        valid = false;
    }
    append(m_objectHandles, DRW::DwgObjectHandleStream::Handles,
           handlesMergedIntoData ? DRW::DwgObjectSerializedSegment::Data
                                  : DRW::DwgObjectSerializedSegment::Handles,
           handlesMergedIntoData ? mergedHandleBaseBit : 0,
           handlesMergedIntoData ? dataOrdinal : handleOrdinal);

    if (!valid || frame.objectHandle == 0 || frame.version == DRW::UNKNOWNV
        || m_nextDwgObjectFrameGeneration == 0) {
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        return false;
    }
    frame.generation = m_nextDwgObjectFrameGeneration++;
    frame.valid = true;
    m_lastDwgObjectFrame = std::move(frame);
    m_lastDwgObjectFrameValid = true;
    m_currentDwgObjectFrameProvenance = {};
    return true;
}

bool dwgWriter15::getLastDwgObjectHandleOccurrences(
    std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const {
    occurrences.clear();
    if (!m_lastDwgObjectFrameValid)
        return false;
    occurrences = m_lastDwgObjectFrame.occurrences;
    return true;
}

bool dwgWriter15::getLastDwgObjectFrame(
    DRW::DwgObjectFrameReceipt& frame) const {
    frame = {};
    if (!m_lastDwgObjectFrameValid)
        return false;
    frame = m_lastDwgObjectFrame;
    return true;
}

void dwgWriter15::configureTextCodec() {
    m_textCodec.setVersion(m_version, false);
    const char* codePage = dwgCodePageName(fileCodePageId());
    m_textCodec.setByteCodePage(codePage == nullptr ? "ANSI_1252" : codePage);
    m_buf.setDecoder(&m_textCodec);
    m_objectBody.setDecoder(&m_textCodec);
    m_objectStrings.setDecoder(&m_textCodec);
    m_objectHandles.setDecoder(&m_textCodec);
}

std::uint16_t dwgWriter15::fileCodePageId() const {
    if (m_header != nullptr) {
        const auto it = m_header->vars.find("$DWGCODEPAGE");
        if (it != m_header->vars.end() && it->second != nullptr
            && it->second->type() == DRW_Variant::STRING
            && it->second->content.s != nullptr
            && !it->second->content.s->empty()) {
            return dwgCodePageId(it->second->content.s->c_str());
        }
    }
    return dwgCodePageId(nullptr);
}

void dwgWriter15::finishObject() {
    // Reject a duplicate before writing another frame.  The HANDLES pass also
    // validates uniqueness, but admitting the frame here would leave the
    // object stream and ownership bookkeeping inconsistent after failure.
    if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                    [this](const auto& entry) {
                        return entry.first == m_currentHandle;
                    })) {
        m_frameWriteError = true;
        return;
    }

    const bool splitLegacyHandles =
        m_version > DRW::AC1014 && m_version < DRW::AC1021;
    const bool stagedLegacyHandles =
        splitLegacyHandles && !m_objectHandles.data().empty();

    if (!m_buf.isGood() || !m_objectBody.isGood() || !m_objectStrings.isGood()
        || !m_objectHandles.isGood()) {
        m_frameWriteError = true;
        return;
    }

    // The legacy object-size field is a signed modular short and the
    // patched RL is a 32-bit bit count. Reject an unrepresentable frame
    // before any narrowing conversion or frame publication.
    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max() / 8u
        || m_objectHandles.size() > std::numeric_limits<std::uint32_t>::max()
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }
    const std::uint32_t dataBitCount = m_objectBody.bitCount();

    std::uint64_t mergedHandleBaseBit = 0;
    if (stagedLegacyHandles) {
        // AC1015/AC1018 use one on-disk stream, but objSize still marks the
        // start of its trailing handle stream. Keep the handle bytes in a
        // scratch buffer until that boundary has been patched.
        mergedHandleBaseBit = m_objectBody.bitCount();
        if (!m_objectBody.data().empty()) {
            const std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
            const size_t rlBitOffset =
                (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
            m_objectBody.patchRawLong32AtBit(rlBitOffset, dataBitCount);
        }
        if (m_objectHandles.size()
            > std::numeric_limits<std::uint32_t>::max() - m_objectBody.size()) {
            m_frameWriteError = true;
            m_objectHandles.reset();
            return;
        }
        m_objectBody.putBytes(m_objectHandles.data().data(),
                              m_objectHandles.data().size());
    }

    // Back-patch the RL objSize ("size of object in bits, not including CRC")
    // at the correct stream-bit offset.  The offset equals the BS width for
    // the oType, which we detect from the first 2 bits of the body:
    //   code "01" → RC payload → 10-bit BS → RL at bit 10
    //   code "00" → RS payload → 18-bit BS → RL at bit 18
    //   code "10" or "11" → no payload → 2-bit BS → RL at bit 2
    std::uint32_t bitCount = 0;
    if (!stagedLegacyHandles) {
        if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max() / 8u) {
            m_frameWriteError = true;
            return;
        }
        bitCount = static_cast<std::uint32_t>(m_objectBody.size()) * 8;
        if (m_objectBody.bitPos() != 0)
            bitCount -= static_cast<std::uint32_t>(8 - m_objectBody.bitPos());
    }
    if (!stagedLegacyHandles && !m_objectBody.data().empty()) {
        std::uint8_t bsCode = (m_objectBody.data()[0] >> 6) & 0x03;
        size_t rlBitOffset = (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
        m_objectBody.patchRawLong32AtBit(rlBitOffset, bitCount);
    }

    // Byte-align the body — the trailing CRC must start aligned and
    // the per-object frame size must be in whole bytes.
    m_objectBody.alignToByte();
    if (m_objectBody.size() > std::numeric_limits<std::uint32_t>::max()
        || m_objectBody.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        || m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }
    std::uint32_t bodyBytes = static_cast<std::uint32_t>(m_objectBody.size());

    std::uint32_t frameStart = static_cast<std::uint32_t>(m_buf.size());

    // MS objectSize = byte count of body (no CRC).  Per the master plan
    // interpretation (b): CRC follows immediately after the slurped body
    // and is not included in `size`.
    m_buf.putModularShort(static_cast<std::int32_t>(bodyBytes));
    if (!m_buf.isGood()) {
        m_frameWriteError = true;
        return;
    }
    size_t bodyStartOffset = m_buf.size();
    m_buf.putBytes(m_objectBody.data().data(), bodyBytes);

    // CRC covers MS prefix + body — spec §20.2: "The CRC includes the size
    // bytes."  frameStart was recorded before putModularShort.
    std::uint16_t crc = m_buf.crc16(0xC0C1, static_cast<size_t>(frameStart),
                              bodyStartOffset + bodyBytes);
    m_buf.putRawShort16(crc);

    if (m_buf.size() > std::numeric_limits<std::uint32_t>::max()) {
        m_frameWriteError = true;
        return;
    }

    if (!captureLastDwgObjectHandleOccurrences(
            0, mergedHandleBaseBit, false, stagedLegacyHandles)) {
        m_buf.truncate(frameStart);
        m_frameWriteError = true;
        m_currentHandle = 0;
        m_objectStrings.reset();
        m_objectHandles.reset();
        return;
    }
    m_objectMap.emplace_back(m_currentHandle, frameStart);
    markDwgClassInstanceEmitted(m_currentHandle);
    m_currentHandle = 0;
    m_objectStrings.reset();
    m_objectHandles.reset();
}

bool dwgWriter15::writeDwgHandles() {
    if (!m_buf.isGood())
        return false;

    std::uint32_t maxHandle = 0;
    if (!validateEmittedHandleMap(maxHandle))
        return false;

    std::uint32_t base = 0;
    if (!objectBaseOffset(base))
        return false;

    size_t sectionStart = m_buf.size();
    dwgBufferW handles;

    if (m_objectMap.empty()) {
        // Empty object map: single terminator page of `RS_BE(2) + CRC16_BE`.
        // size_BE = 2 means "this page contains 2 bytes including the size word".
        std::uint16_t pageSize = 2;
        handles.putBERawShort16(pageSize);
        std::uint16_t crc = handles.crc16(0xC0C1, 0, handles.size());
        handles.putBERawShort16(crc);
    } else {
        // Sort by handle for monotonic UMC deltas.  Per the master plan
        // (LibreDWG insertion-sort at encode.c:2394-2406): in practice the
        // map is already monotonic because objects are emitted in handle
        // order, but the sort defends against future out-of-order emit.
        std::sort(m_objectMap.begin(), m_objectMap.end());
        for (size_t i = 1; i < m_objectMap.size(); ++i) {
            if (m_objectMap[i - 1].first == m_objectMap[i].first) {
                // Two objects were emitted with the same handle, so the
                // object map is ambiguous and the file would be unreadable.
                DRW_DBG("dwgWriter15::writeDwgHandles: duplicate handle ");
                DRW_DBGH(m_objectMap[i].first);
                DRW_DBG("\n");
                return false;
            }
        }

        // HANDLES stores section-relative offsets. Validate the subtraction
        // before emitting a page so an invalid writer state cannot wrap an
        // unsigned delta and leave a partial HANDLES section behind.
        if (std::any_of(m_objectMap.cbegin(), m_objectMap.cend(),
                        [base](const auto& entry) {
                            return entry.second < base;
                        })) {
            return false;
        }

        // Page-emit walk. Each page is bounded at <=2030 bytes of (size
        // field + entries) to leave 2 bytes for the trailing CRC under the
        // 2032-byte ODA spec limit.
        constexpr size_t kMaxPagePayloadBytes = 2030;

        size_t entryStart = 0;
        while (entryStart < m_objectMap.size()) {
            size_t pageStart = handles.size();
            // Reserve the size field; we know its width is 2 bytes (RS BE)
            // so we know the payload budget is kMaxPagePayloadBytes - 2.
            handles.putBERawShort16(0);  // patched after entries

            std::uint32_t prevHandle = 0;
            std::uint32_t prevOffset = 0;
            size_t entryEnd = entryStart;
            while (entryEnd < m_objectMap.size()) {
                std::uint32_t h = m_objectMap[entryEnd].first;
                std::uint32_t off = m_objectMap[entryEnd].second - base;

                // UMC + MC are each at most five bytes. If adding that
                // upper bound would exceed the page, close it now.
                if ((handles.size() - pageStart) + 10 > kMaxPagePayloadBytes)
                    break;

                handles.putUModularChar(h - prevHandle);
                const std::int64_t locationDelta =
                    static_cast<std::int64_t>(off)
                    - static_cast<std::int64_t>(prevOffset);
                handles.putModularChar(locationDelta);
                if (!handles.isGood())
                    return false;
                prevHandle = h;
                prevOffset = off;
                ++entryEnd;
            }

            // A valid entry must fit in an otherwise empty page. This guard
            // prevents an empty page loop if the encoding bound changes.
            if (entryEnd == entryStart)
                return false;

            // Patch the page size (BE) at pageStart. patchRawShort16 emits
            // LE, so write the BE bytes directly here.
            std::uint16_t pageSize = static_cast<std::uint16_t>(
                handles.size() - pageStart);
            handles.data()[pageStart] =
                static_cast<std::uint8_t>((pageSize >> 8) & 0xFF);
            handles.data()[pageStart + 1] =
                static_cast<std::uint8_t>(pageSize & 0xFF);

            // CRC16 BE over (size + entries).
            std::uint16_t crc = handles.crc16(
                0xC0C1, pageStart, handles.size());
            handles.putBERawShort16(crc);

            entryStart = entryEnd;
        }

        // Terminator page (2-byte size word + CRC of just the size).
        size_t termStart = handles.size();
        std::uint16_t termSize = 2;
        handles.putBERawShort16(termSize);
        std::uint16_t termCrc = handles.crc16(
            0xC0C1, termStart, handles.size());
        handles.putBERawShort16(termCrc);
    }

    std::uint64_t finalSize = 0;
    if (!handles.isGood() || handles.bitPos() != 0
        || handles.size() > std::numeric_limits<std::uint32_t>::max()
        || sectionStart > std::numeric_limits<std::uint32_t>::max()
        || !dwgSafety::add(static_cast<std::uint64_t>(sectionStart),
                           static_cast<std::uint64_t>(handles.size()),
                           finalSize)
        || finalSize > std::numeric_limits<std::uint32_t>::max())
        return false;
    m_buf.putBytes(handles.data().data(), handles.size());
    if (!m_buf.isGood())
        return false;
    m_sectionOffsets[recno::HANDLES] =
        static_cast<std::uint32_t>(sectionStart);
    m_sectionSizes[recno::HANDLES] =
        static_cast<std::uint32_t>(handles.size());
    return true;
}

bool dwgWriter15::validateEmittedHandleMap(std::uint32_t& maxHandle) const {
    maxHandle = 0;
    std::set<std::uint32_t> handles;
    for (const auto& entry : m_objectMap) {
        if (entry.first == 0 || entry.second >= m_buf.size()
            || !handles.insert(entry.first).second)
            return false;
        maxHandle = std::max(maxHandle, entry.first);
    }
    return true;
}

bool dwgWriter15::validateDwgClassInstanceFrames() const {
    if (!m_rawClassInstanceHandles.empty()
        || m_dwgClassInstanceHandleOwners.size()
               != m_dwgClassInstanceEmittedHandles.size())
        return false;

    for (std::uint32_t handle : m_dwgClassInstanceEmittedHandles) {
        const auto owner = m_dwgClassInstanceHandleOwners.find(handle);
        if (owner == m_dwgClassInstanceHandleOwners.end())
            return false;
        std::size_t frameCount = 0;
        for (const auto& entry : m_objectMap) {
            if (entry.first == handle)
                ++frameCount;
        }
        if (frameCount != 1)
            return false;
    }
    return true;
}

bool dwgWriter15::writeSecondHeader() {
    if (m_version == DRW::AC1015) {
        // R2000 keeps an empty four-byte Template record in the locator table.
        // Leaving its address and size at zero makes libreDWG decode the file
        // header as a T16 description instead of treating the record as empty.
        static constexpr std::uint8_t emptyTemplate[] = {0, 0, 0, 0};
        m_sectionOffsets[recno::TEMPLATE] =
            static_cast<std::uint32_t>(m_buf.size());
        m_buf.putBytes(emptyTemplate, sizeof(emptyTemplate));
        m_sectionSizes[recno::TEMPLATE] = sizeof(emptyTemplate);
    }

    const std::vector<std::uint8_t> auxHeader = buildR2000AuxHeaderContent(m_header);
    m_sectionOffsets[recno::AUXHEADER] = static_cast<std::uint32_t>(m_buf.size());
    if (!auxHeader.empty())
        m_buf.putBytes(auxHeader.data(), auxHeader.size());
    m_sectionSizes[recno::AUXHEADER] = static_cast<std::uint32_t>(auxHeader.size());
    return true;
}

bool dwgWriter15::finalize() {
    if (objectWriteFailed())
        return false;

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
