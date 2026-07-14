/******************************************************************************
**  libDXFrw - Pre-R2.0b DWG reader (R1.40). See dwgreaderR1_40.h.            **
******************************************************************************/

#include <cstdint>
#include <string>

#include "dwgreaderR1_40.h"
#include "drw_dbg.h"
#include "../drw_entities.h"
#include "../drw_objects.h"

namespace {
constexpr std::uint32_t R1_40_ENTITIES_START = 0x202;

// Pre-R2.0b entity type codes (Dwg_Object_Type_r11 subset; same numbering as
// the later pre-R13 types but with a wholly different record framing).
enum R1_40Type {
    T_LINE = 1, T_POINT = 2, T_CIRCLE = 3, T_SHAPE = 4, T_REPEAT = 5,
    T_ENDREP = 6, T_TEXT = 7, T_ARC = 8, T_TRACE = 9, T_LOAD = 10,
    T_SOLID = 11, T_BLOCK = 12, T_ENDBLK = 13, T_INSERT = 14
};
}

bool dwgReaderR1_40::readMetaData() {
    if (!fileBuf->setPosition(0))
        return false;
    std::string magic;
    for (int i = 0; i < 6; ++i)
        magic.push_back(static_cast<char>(fileBuf->getRawChar8()));
    if (magic != "AC1.40")
        return false;
    version = DRW::AC14;
    setCodePage("ANSI_1252");   // pure ASCII fixture; codepage is a follow-up
    return true;
}

bool dwgReaderR1_40::readFileHeader() {
    // dwg_size (u32 @0x24) is the sole termination boundary for the entity
    // stream, which begins at the fixed post-header offset 0x202.
    if (!fileBuf->setPosition(0x24))
        return false;
    m_dwgSize = fileBuf->getRawLong32();
    const std::uint32_t fileSize = static_cast<std::uint32_t>(fileBuf->size());
    if (m_dwgSize < R1_40_ENTITIES_START || m_dwgSize > fileSize)
        return false;   // implausible -> not a readable R1.40 file
    return true;
}

// R1.40 has no R10/R11-shaped header-var block or 10-byte table descriptors;
// header/tables default. Blocks are entity records inside the single stream.
bool dwgReaderR1_40::readDwgHeader(DRW_Header& /*hdr*/) { return true; }
bool dwgReaderR1_40::readDwgTables(DRW_Header& /*hdr*/) { return true; }
bool dwgReaderR1_40::readDwgBlocks(DRW_Interface& /*intfa*/) { return true; }

std::string dwgReaderR1_40::readTV(std::uint32_t end) {
    if (fileBuf->getPosition() + 2 > end)
        return {};
    std::uint16_t n = fileBuf->getRawShort16();
    std::string s;
    s.reserve(n);
    for (std::uint16_t i = 0; i < n && fileBuf->getPosition() < end; ++i)
        s.push_back(static_cast<char>(fileBuf->getRawChar8()));
    return decoder.toUtf8(s);
}

bool dwgReaderR1_40::readEntity(DRW_Interface& intfa, std::uint32_t& cursor) {
    const std::uint32_t end = m_dwgSize;
    if (cursor + 4 > end)
        return false;                 // need type(2)+layer(2)
    if (!fileBuf->setPosition(cursor))
        return false;
    const std::uint16_t rawType = fileBuf->getRawShort16();
    fileBuf->getRawShort16();          // layer index (into the 128-slot palette; unused for now)

    // Deleted marker: rawType > 127 -> real type = abs((int8_t)(rawType & 0xff)).
    std::uint8_t type;
    if (rawType > 127) {
        const std::int8_t sgn = static_cast<std::int8_t>(rawType & 0xff);
        type = static_cast<std::uint8_t>(sgn < 0 ? -sgn : sgn);
    } else {
        type = static_cast<std::uint8_t>(rawType);
    }

    auto rd = [&]() { return fileBuf->getRawDouble(); };
    auto p2 = [&]() { DRW_Coord c; c.x = rd(); c.y = rd(); c.z = 0.0; return c; };
    const std::uint32_t bodyStart = cursor + 4;   // buffer is positioned here
    auto bound = [&](std::uint32_t need) { return bodyStart + need <= end; };

    bool ok = true;
    switch (type) {
    case T_LINE: {
        if (!bound(32)) { ok = false; break; }
        DRW_Line e; e.basePoint = p2(); e.secPoint = p2();
        intfa.addLine(e); break; }
    case T_POINT: {
        if (!bound(16)) { ok = false; break; }
        DRW_Point e; e.basePoint = p2();
        intfa.addPoint(e); break; }
    case T_CIRCLE: {
        if (!bound(24)) { ok = false; break; }
        DRW_Circle e; e.basePoint = p2(); e.radious = rd();
        intfa.addCircle(e); break; }
    case T_ARC: {
        if (!bound(40)) { ok = false; break; }
        DRW_Arc e; e.basePoint = p2(); e.radious = rd();
        e.staangle = rd(); e.endangle = rd();
        intfa.addArc(e); break; }
    case T_TEXT: {
        if (!bound(32)) { ok = false; break; }
        DRW_Text e; e.basePoint = p2(); e.height = rd();
        rd();                          // oblique (inline PRE R_2_0; not rendered)
        e.text = readTV(end);
        intfa.addText(e); break; }
    case T_TRACE: {
        if (!bound(64)) { ok = false; break; }
        DRW_Trace e; e.basePoint = p2(); e.secPoint = p2();
        e.thirdPoint = p2(); e.fourPoint = p2();
        intfa.addTrace(e); break; }
    case T_SOLID: {
        if (!bound(64)) { ok = false; break; }
        DRW_Solid e; e.basePoint = p2(); e.secPoint = p2();
        e.thirdPoint = p2(); e.fourPoint = p2();
        intfa.addSolid(e); break; }
    case T_SHAPE: {
        if (!bound(34)) { ok = false; break; }
        DRW_Shape e; e.m_insertionPoint = p2();
        e.m_scale = rd(); e.m_rotation = rd();
        e.m_shapeIndex = fileBuf->getRawShort16();
        intfa.addShape(e); break; }
    case T_BLOCK: {
        DRW_Block e; e.name = readTV(end);
        if (fileBuf->getPosition() + 16 > end) { ok = false; break; }
        e.basePoint = p2();
        intfa.addBlock(e); break; }
    case T_ENDBLK: {
        intfa.endBlock(); break; }
    case T_INSERT: {
        DRW_Insert e; e.name = readTV(end);
        if (fileBuf->getPosition() + 40 > end) { ok = false; break; }
        e.basePoint = p2();            // ins_pt 2RD
        e.xscale = rd(); e.yscale = rd();   // scale 2RD
        e.angle = rd();                // rotation RD
        intfa.addInsert(e); break; }
    case T_ENDREP: {
        if (!bound(20)) { ok = false; break; }
        fileBuf->getRawShort16();      // numcols
        fileBuf->getRawShort16();      // numrows
        rd(); rd();                    // col/row spacing
        break; }                       // structural marker; no geometry
    case T_LOAD: {
        readTV(end); break; }          // shapefile load directive; skip
    case T_REPEAT: {
        break; }                       // structural marker; empty body
    default:
        ok = false; break;             // unknown type -> stop the walk (no size to skip)
    }

    if (!ok)
        return false;
    // Advance by the exact number of bytes the body consumed.
    cursor = static_cast<std::uint32_t>(fileBuf->getPosition());
    return true;
}

bool dwgReaderR1_40::readDwgEntities(DRW_Interface& intfa) {
    DRW_DBG("\n=== R1.40 entity stream ["); DRW_DBGH(R1_40_ENTITIES_START);
    DRW_DBG(","); DRW_DBGH(m_dwgSize); DRW_DBG(") ===\n");
    std::uint32_t cursor = R1_40_ENTITIES_START;
    while (cursor < m_dwgSize) {
        if (!readEntity(intfa, cursor))
            break;
    }
    return true;
}
