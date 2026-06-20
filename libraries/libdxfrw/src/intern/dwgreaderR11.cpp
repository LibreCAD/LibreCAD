/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**  Pre-R13 (R10/R11) DWG reader. See dwgreaderR11.h.                         **
******************************************************************************/

#include <cstdint>
#include <string>

#include "dwgreaderR11.h"
#include "drw_dbg.h"

namespace {
// Pre-R13 entity type codes (Dwg_Object_Type_r11).
enum R11Type {
    R11_LINE = 1, R11_POINT = 2, R11_CIRCLE = 3, R11_SHAPE = 4,
    R11_REPEAT = 5, R11_ENDREP = 6, R11_TEXT = 7, R11_ARC = 8,
    R11_TRACE = 9, R11_LOAD = 10, R11_SOLID = 11, R11_BLOCK = 12,
    R11_ENDBLK = 13, R11_INSERT = 14, R11_ATTDEF = 15, R11_ATTRIB = 16,
    R11_SEQEND = 17, R11_JUMP = 18, R11_POLYLINE = 19, R11_VERTEX = 20,
    R11_3DLINE = 21, R11_3DFACE = 22, R11_DIMENSION = 23, R11_VIEWPORT = 24
};

// Common-entity-header flag_r11 bits.
constexpr std::uint8_t FLAG_HAS_COLOR = 0x01;
constexpr std::uint8_t FLAG_HAS_LTYPE = 0x02;
constexpr std::uint8_t FLAG_HAS_ELEVATION = 0x04;
constexpr std::uint8_t FLAG_HAS_THICKNESS = 0x08;
constexpr std::uint8_t FLAG_HAS_HANDLING = 0x20;
constexpr std::uint8_t FLAG_HAS_PSPACE = 0x40;

// extra_r11 bits.
constexpr std::uint8_t EXTRA_HAS_EED = 0x02;
constexpr std::uint8_t EXTRA_HAS_VIEWPORT = 0x04;
}

bool dwgReaderR11::readMetaData() {
    // Identify the precise pre-R13 version from the 6-byte magic. Scope is
    // AC1009 (R11); AC1006 (R10) is accepted by the container parse but has no
    // validation oracle, so geometry fidelity there is best-effort.
    if (!fileBuf->setPosition(0))
        return false;
    std::string magic;
    for (int i = 0; i < 6; ++i)
        magic.push_back(static_cast<char>(fileBuf->getRawChar8()));
    if (magic == "AC1009")
        version = DRW::AC1009;
    else if (magic == "AC1006")
        version = DRW::AC1006;
    else
        return false;
    // Pre-R13 text is codepage-encoded; default to ANSI_1252 (test corpus is
    // ASCII, so this is a no-op there). Codepage from $DWGCODEPAGE is a follow-up.
    setCodePage("ANSI_1252");
    return true;
}

bool dwgReaderR11::readFileHeader() {
    // Section-pointer block. Offsets (from libredwg header.spec pre-R13 branch):
    //   0x14 entities_start RL, 0x18 entities_end RL, 0x1C blocks_start RL,
    //   0x20 blocks_size RL, 0x24 extras_start RL, 0x28 extras_size RL.
    // The *_size fields can carry sentinel high bits (0x40000000/0x80000000) and
    // must be masked to 24 bits.
    if (!fileBuf->setPosition(0x14))
        return false;
    m_entitiesStart = fileBuf->getRawLong32();
    m_entitiesEnd = fileBuf->getRawLong32();
    m_blocksStart = fileBuf->getRawLong32();
    std::uint32_t blocksSize = fileBuf->getRawLong32();
    if (blocksSize > 0xFFFFFF) blocksSize &= 0xFFFFFF;
    m_blocksEnd = m_blocksStart + blocksSize;
    m_extrasStart = fileBuf->getRawLong32();
    std::uint32_t extrasSize = fileBuf->getRawLong32();
    if (extrasSize > 0xFFFFFF) extrasSize &= 0xFFFFFF;
    m_extrasEnd = m_extrasStart + extrasSize;

    const std::uint64_t fileSize = static_cast<std::uint64_t>(fileBuf->size());
    if (m_entitiesStart == 0 || m_entitiesEnd <= m_entitiesStart
        || m_entitiesEnd > fileSize)
        return false;  // implausible header -> not a readable pre-R13 file
    return true;
}

bool dwgReaderR11::readDwgHeader(DRW_Header& /*hdr*/) {
    return true;  // header variables not needed for entity geometry (follow-up)
}

bool dwgReaderR11::readDwgTables(DRW_Header& /*hdr*/) {
    return true;  // tables (layer/ltype/style/block names) -> follow-up
}

bool dwgReaderR11::readDwgBlocks(DRW_Interface& /*intfa*/) {
    return true;  // block-definition section -> follow-up
}

bool dwgReaderR11::readDwgEntities(DRW_Interface& intfa) {
    DRW_DBG("\n=== pre-R13 ENTITIES section ["); DRW_DBGH(m_entitiesStart);
    DRW_DBG(","); DRW_DBGH(m_entitiesEnd); DRW_DBG(") ===\n");
    return readEntitySection(m_entitiesStart, m_entitiesEnd, intfa);
}

bool dwgReaderR11::readEntitySection(std::uint32_t start, std::uint32_t end,
                                     DRW_Interface& intfa) {
    if (start == 0 || end <= start)
        return true;
    if (!fileBuf->setPosition(start))
        return false;
    int guard = 0;
    while (fileBuf->getPosition() + 4 <= end) {
        if (++guard > 2000000) break;  // runaway guard
        if (!readEntityR11(intfa))
            break;  // desync -> stop this section (entities already read stand)
    }
    return true;
}

bool dwgReaderR11::readEntityR11(DRW_Interface& intfa) {
    const std::uint64_t recStart = fileBuf->getPosition();
    const std::uint8_t typeByte = fileBuf->getRawChar8();
    const bool deleted = (typeByte & 0x80) != 0;
    const std::uint8_t type = typeByte & 0x7F;
    const std::uint8_t flag = fileBuf->getRawChar8();
    const std::uint16_t size = fileBuf->getRawShort16();
    if (size < 5)
        return false;  // below the minimum header -> unrecoverable desync
    const std::uint64_t recEnd = recStart + size;

    // Common entity header (read to consume; we apply only a subset for now).
    std::uint16_t opts = 0;
    if (type != R11_JUMP) {
        fileBuf->getRawShort16();              // layer index (resolution: follow-up)
        opts = fileBuf->getRawShort16();       // per-type optional-field flags
    }
    std::uint8_t extra = 0;
    if (flag & FLAG_HAS_PSPACE)
        extra = fileBuf->getRawChar8();
    if (extra & EXTRA_HAS_EED) {
        const std::uint16_t eed = fileBuf->getRawShort16();
        for (std::uint16_t i = 0; i < eed; ++i) fileBuf->getRawChar8();
    }
    if (flag & FLAG_HAS_COLOR)
        fileBuf->getRawChar8();
    if (flag & FLAG_HAS_LTYPE)
        fileBuf->getRawShort16();
    double elevation = 0.0;
    // HAS_ELEVATION is suppressed for LINE/POINT/3DFACE (their Z is in the body).
    if ((flag & FLAG_HAS_ELEVATION)
        && type != R11_LINE && type != R11_POINT && type != R11_3DFACE)
        elevation = fileBuf->getRawDouble();
    double thickness = 0.0;
    if (flag & FLAG_HAS_THICKNESS)
        thickness = fileBuf->getRawDouble();
    if (flag & FLAG_HAS_HANDLING) {
        const std::uint8_t hl = fileBuf->getRawChar8();
        for (std::uint8_t i = 0; i < hl; ++i) fileBuf->getRawChar8();
    }
    if (extra & EXTRA_HAS_VIEWPORT)
        fileBuf->getRawShort16();

    const bool hasElev = (flag & FLAG_HAS_ELEVATION) != 0;
    auto rd = [&]() { return fileBuf->getRawDouble(); };
    auto rd3 = [&]() { DRW_Coord c; c.x = fileBuf->getRawDouble();
                       c.y = fileBuf->getRawDouble(); c.z = fileBuf->getRawDouble();
                       return c; };

    if (!deleted) {
        switch (type) {
        case R11_LINE: {
            DRW_Line e;
            if (!hasElev) {
                e.basePoint = rd3();
                e.secPoint = rd3();
            } else {
                e.basePoint = fileBuf->get2RawDouble(); e.basePoint.z = elevation;
                e.secPoint = fileBuf->get2RawDouble();  e.secPoint.z = elevation;
            }
            e.thickness = thickness;
            intfa.addLine(e);
            break; }
        case R11_POINT: {
            DRW_Point e;
            e.basePoint.x = rd(); e.basePoint.y = rd();
            if (!hasElev) e.basePoint.z = rd();
            e.thickness = thickness;
            intfa.addPoint(e);
            break; }
        case R11_CIRCLE: {
            DRW_Circle e;
            e.basePoint = fileBuf->get2RawDouble();
            e.radious = rd();
            e.basePoint.z = elevation;
            e.thickness = thickness;
            intfa.addCircle(e);
            break; }
        case R11_ARC: {
            DRW_Arc e;
            e.basePoint = fileBuf->get2RawDouble();
            e.radious = rd();
            e.staangle = rd();
            e.endangle = rd();
            e.basePoint.z = elevation;
            e.thickness = thickness;
            intfa.addArc(e);
            break; }
        case R11_TEXT: {
            DRW_Text e;
            e.basePoint = fileBuf->get2RawDouble();
            e.basePoint.z = elevation;
            e.height = rd();
            const std::uint16_t tlen = fileBuf->getRawShort16();
            std::string s; s.reserve(tlen);
            for (std::uint16_t i = 0; i < tlen; ++i)
                s.push_back(static_cast<char>(fileBuf->getRawChar8()));
            e.text = s;
            if (opts & 0x01) e.angle = rd();
            e.thickness = thickness;
            intfa.addText(e);
            break; }
        case R11_SOLID:
        case R11_TRACE: {
            DRW_Solid e;  // DRW_Solid : DRW_Trace, same 4-corner layout
            e.basePoint = fileBuf->get2RawDouble();
            e.secPoint = fileBuf->get2RawDouble();
            e.thirdPoint = fileBuf->get2RawDouble();
            e.fourPoint = fileBuf->get2RawDouble();
            e.basePoint.z = e.secPoint.z = e.thirdPoint.z = e.fourPoint.z = elevation;
            e.thickness = thickness;
            if (type == R11_TRACE) intfa.addTrace(e); else intfa.addSolid(e);
            break; }
        case R11_3DFACE: {
            DRW_3Dface e;
            if (hasElev) {
                e.basePoint = fileBuf->get2RawDouble();
                e.secPoint = fileBuf->get2RawDouble();
                e.thirdPoint = fileBuf->get2RawDouble();
                e.fourPoint = fileBuf->get2RawDouble();
            } else {
                e.basePoint = rd3();
                e.secPoint = rd3();
                e.thirdPoint = rd3();
                e.fourPoint = rd3();
            }
            intfa.add3dFace(e);
            break; }
        default:
            // Unhandled type (INSERT/POLYLINE/VERTEX/SEQEND/BLOCK/... ) -> skipped
            // for now; counted as a parse "miss" but not a failure (advance by size).
            ++m_entityParseFailures;
            break;
        }
    }

    // Authoritative advance: the header `size` is the full record length
    // (type byte .. trailing CRC), so jump there regardless of body-parse drift.
    if (!fileBuf->setPosition(recEnd))
        return false;
    return true;
}
