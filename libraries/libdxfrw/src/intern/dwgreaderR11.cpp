/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**  Pre-R13 (R10/R11) DWG reader. See dwgreaderR11.h.                         **
******************************************************************************/

#include <cstdint>
#include <string>

#include "dwgreaderR11.h"
#include "drw_dbg.h"
#include "../drw_objects.h"

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
    // Identify the precise pre-R13 version from the 6-byte magic. Both AC1006
    // (R10) and AC1009 (R11) are validatable against dwgread; their containers
    // are byte-identical except for the LTYPE handle width in the entity common
    // header (R10 = 1B, R11 = 2B; branched in readEntityR11).
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

bool dwgReaderR11::readDwgHeader(DRW_Header& hdr) {
    // The pre-R13 header variables block starts at file offset 0x5E (the 5
    // leading 10-byte table-section headers end at 0x2C+50 = 0x5E). ALL fields
    // are RAW LE; the spec runs SEQUENTIAL only — every offset is the running
    // sum of prior fields, so a wrong width here desyncs everything that
    // follows. We read just the high-value drawing-state vars (through
    // PLINEWID, post-cursor 0x36f) and STOP — the long DIMxx / UCS / VPORT
    // tail uses inline 10-byte embedded-table-headers (PRER13_SECTION_HDR) and
    // has near-zero rendering value.
    //
    // CRITICAL CALL ORDER: processDwg() calls readDwgHeader BEFORE
    // readDwgTables, so the LAYER/STYLE/LTYPE name vectors are EMPTY here.
    // For CLAYER/TEXTSTYLE/CELTYPE name resolution we eagerly read the table
    // name vectors up-front (idempotent; the per-record decoders run again in
    // readDwgTables to populate ltypemap/layermap/stylemap).
    if (version != DRW::AC1009)
        return true;  // R10 header layout overlaps R11 but has different per-
                      // field widths (FIELD_CMC CECOLOR, no PSLTSCALE etc.) —
                      // out of scope.

    // Eager table-name reads for CLAYER/TEXTSTYLE/CELTYPE resolution. These
    // are seek-absolute (readNameTable does setPosition), so they do not
    // disturb our header-walk cursor.
    readNameTable(0x36, m_layerNames);
    readNameTable(0x40, m_styleNames);
    readNameTable(0x4A, m_ltypeNames);

    if (!fileBuf->setPosition(0x5E))
        return false;
    auto rc = [&]() { return fileBuf->getRawChar8(); };
    auto rs = [&]() { return fileBuf->getRawShort16(); };
    auto rsd = [&]() { return static_cast<std::int16_t>(fileBuf->getRawShort16()); };
    auto rl = [&]() { return fileBuf->getRawLong32(); };
    auto rd = [&]() { return fileBuf->getRawDouble(); };
    auto r2d = [&]() { DRW_Coord c; c.x = rd(); c.y = rd(); c.z = 0; return c; };
    auto r3d = [&]() { DRW_Coord c; c.x = rd(); c.y = rd(); c.z = rd(); return c; };
    auto skipBytes = [&](int n) {
        fileBuf->setPosition(fileBuf->getPosition() + n);
    };

    hdr.addCoord("INSBASE", r3d(), 10);
    rs();                                                 // PLINEGEN (unused)
    hdr.addCoord("EXTMIN", r3d(), 10);
    hdr.addCoord("EXTMAX", r3d(), 10);
    hdr.addCoord("LIMMIN", r2d(), 10);
    hdr.addCoord("LIMMAX", r2d(), 10);
    hdr.addCoord("VIEWCTR", r3d(), 10);
    hdr.addDouble("VIEWSIZE", rd(), 40);
    hdr.addInt("SNAPMODE", rs(), 70);
    r2d();                                                // SNAPUNIT (unused)
    r2d();                                                // SNAPBASE (unused)
    rd();                                                 // SNAPANG (unused)
    rs();                                                 // SNAPSTYLE
    rs();                                                 // SNAPISOPAIR
    hdr.addInt("GRIDMODE", rs(), 70);
    r2d();                                                // GRIDUNIT (unused)
    hdr.addInt("ORTHOMODE", rs(), 70);
    hdr.addInt("REGENMODE", rs(), 70);
    hdr.addInt("FILLMODE", rs(), 70);
    hdr.addInt("QTEXTMODE", rs(), 70);
    rs();                                                 // DRAGMODE (unused)
    hdr.addDouble("LTSCALE", rd(), 40);
    hdr.addDouble("TEXTSIZE", rd(), 40);
    hdr.addDouble("TRACEWID", rd(), 40);
    const std::int16_t clayerIdx = rsd();                 // CLAYER (signed RS index)
    rl(); rl();                                           // oldCECOLOR (DECOY — skip)
    rs();                                                 // unknown_5
    rs();                                                 // PSLTSCALE
    rs();                                                 // TREEDEPTH
    rs();                                                 // unknown_6
    rd();                                                 // aspect_ratio (calculated)
    hdr.addInt("LUNITS", rs(), 70);
    hdr.addInt("LUPREC", rs(), 70);
    rs();                                                 // AXISMODE
    r2d();                                                // AXISUNIT
    rd();                                                 // SKETCHINC
    rd();                                                 // FILLETRAD
    hdr.addInt("AUNITS", rs(), 70);
    hdr.addInt("AUPREC", rs(), 70);
    const std::int16_t textstyleIdx = rsd();              // TEXTSTYLE (signed RS index)
    hdr.addInt("OSMODE", rs(), 70);
    hdr.addInt("ATTMODE", rs(), 70);
    skipBytes(15);                                        // MENU (15 fixed bytes)
    hdr.addDouble("DIMSCALE", rd(), 40);                  // load-bearing 0x1a3 checkpoint
    rd(); rd(); rd(); rd();                               // DIMASZ DIMEXO DIMDLI DIMEXE
    rd(); rd(); rd(); rd(); rd();                         // DIMTP DIMTM DIMTXT DIMCEN DIMTSZ
    rc(); rc(); rc(); rc(); rc(); rc(); rc();             // DIMTOL DIMLIM DIMTIH DIMTOH DIMSE1 DIMSE2 DIMTAD
    rc();                                                 // LIMCHECK
    skipBytes(46);                                        // MENUEXT (46 fixed bytes)
    hdr.addDouble("ELEVATION", rd(), 40);
    hdr.addDouble("THICKNESS", rd(), 40);
    hdr.addCoord("VIEWDIR", r3d(), 10);
    for (int i = 0; i < 6; ++i) r3d();                    // VPOINT/VPOINTALT (6 x 3RD)
    rs();                                                 // flag_3d
    rs();                                                 // BLIPMODE
    rc();                                                 // DIMZIN
    rd();                                                 // DIMRND
    rd();                                                 // DIMDLE
    skipBytes(33);                                        // DIMBLK_T (33 fixed)
    rs();                                                 // circle_zoom
    rs();                                                 // COORDS
    hdr.addInt("CECOLOR", static_cast<std::int16_t>(rs()), 62);  // the REAL CECOLOR
    const std::int16_t celtypeIdx = rsd();                // CELTYPE (signed RS index)
    rl(); rl();                                           // TDCREATE (TIMERLL)
    rl(); rl();                                           // TDUPDATE
    rl(); rl();                                           // TDINDWG
    rl(); rl();                                           // TDUSRTIMER
    rs();                                                 // USRTIMER
    rs();                                                 // FASTZOOM
    rs();                                                 // SKPOLY
    for (int i = 0; i < 7; ++i) rs();                     // unknown_mon..unknown_ms
    hdr.addDouble("ANGBASE", rd(), 50);
    hdr.addInt("ANGDIR", rs(), 70);
    hdr.addInt("PDMODE", rs(), 70);
    hdr.addDouble("PDSIZE", rd(), 40);
    rd();                                                 // PLINEWID (cursor 0x36f)
    // STOP — the long DIMxx tail, UCS/VPORT/VIEW/APPID/DIMSTYLE/VX section
    // headers and per-record fields are not consumed (no reader-side value
    // and the inline PRER13_SECTION_HDR layout invites desync).

    // Resolve handle-references (CLAYER/TEXTSTYLE/CELTYPE) to names. Indices
    // are 0-based; 0x7FFF/0x7FFE are the ByLayer/ByBlock sentinels.
    auto resolveName = [&](std::int16_t idx,
                           const std::vector<std::string>& tbl,
                           const char* def) -> std::string {
        if (idx == 0x7FFF) return "BYLAYER";
        if (idx == 0x7FFE) return "BYBLOCK";
        if (idx >= 0 && static_cast<size_t>(idx) < tbl.size())
            return tbl[idx];
        return def;
    };
    hdr.addStr("CLAYER",   resolveName(clayerIdx, m_layerNames, "0"),         8);
    hdr.addStr("TEXTSTYLE", resolveName(textstyleIdx, m_styleNames, "STANDARD"), 7);
    hdr.addStr("CELTYPE",  resolveName(celtypeIdx, m_ltypeNames, "BYLAYER"),  6);
    return true;
}

bool dwgReaderR11::readNameTable(std::uint32_t hdrPos, std::vector<std::string>& out) {
    // Table-section header (10 bytes): size(RS) number(RS) flags(RS) address(RL).
    // Each record is `size` bytes: flag(RC) + name(32 fixed null-padded bytes) +
    // used(RS,R11) + per-table fields. We only need the name (at record+1).
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (addr == 0 || recSize < 33)
        return true;  // absent/implausible table -> leave names empty
    out.clear();
    out.reserve(recNum);
    for (std::uint16_t i = 0; i < recNum; ++i) {
        if (!fileBuf->setPosition(addr + static_cast<std::uint64_t>(i) * recSize + 1))
            break;  // skip flag(RC); name follows
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        out.push_back(name);
    }
    return true;
}

std::string dwgReaderR11::layerName(std::uint16_t idx) const {
    // Entity layer is a 0-based RS index into the LAYER table (index 0 == "0",
    // verified vs dwgread).
    if (idx < m_layerNames.size())
        return m_layerNames[idx];
    return std::string();
}

std::string dwgReaderR11::ltypeName(std::int16_t idx) const {
    // 0x7FFF == ByLayer, 0x7FFE == ByBlock (the standard AutoCAD sentinels;
    // libreDWG / dwgread report these as raw values in `ltype: [2,32767,0]`).
    // Negative or out-of-range -> empty (consumer falls back).
    if (idx == 0x7FFF) return "BYLAYER";
    if (idx == 0x7FFE) return "BYBLOCK";
    if (idx >= 0 && static_cast<size_t>(idx) < m_ltypeNames.size())
        return m_ltypeNames[idx];
    return std::string();
}

bool dwgReaderR11::readLTypeTable(std::uint32_t hdrPos) {
    // LTYPE record (recSize 191):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11)       off 35: description 48 FIXED bytes
    //   off 83: alignment RC ('A')   off 84: numdashes RCu (0..12)
    //   off 85: pattern_len RD       off 93: dashes_r11 12 * RD (96B FIXED)
    //   off 189: CRC RS
    // Only the first `numdashes` doubles in dashes_r11 are valid; the unused
    // slots are uninitialised garbage and MUST be truncated.
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (addr == 0 || recSize < 189 || recNum == 0)
        return true;  // absent/implausible
    m_ltypeNames.clear();
    m_ltypeNames.reserve(recNum);
    for (std::uint16_t i = 0; i < recNum; ++i) {
        const std::uint64_t base = addr + static_cast<std::uint64_t>(i) * recSize;
        if (!fileBuf->setPosition(base))
            break;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        m_ltypeNames.push_back(name);
        // off 33: used (signed RS, ignored — header lists "used count"
        // sentinel; libreDWG keeps it for debugging only).
        static_cast<void>(fileBuf->getRawShort16());
        // off 35: description (48 FIXED null-padded bytes).
        std::string desc;
        ended = false;
        for (int j = 0; j < 48; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) desc.push_back(c);
        }
        // off 83: alignment (always 'A' for AutoCAD ltypes); off 84: numdashes.
        static_cast<void>(fileBuf->getRawChar8());            // alignment
        const std::uint8_t numdashes = fileBuf->getRawChar8();
        const double patternLen = fileBuf->getRawDouble();    // off 85
        std::vector<double> path;
        path.reserve(numdashes);
        for (std::uint8_t d = 0; d < 12; ++d) {
            const double v = fileBuf->getRawDouble();
            if (d < numdashes) path.push_back(v);             // truncate garbage
        }
        auto* lt = new DRW_LType();
        lt->name = name;
        lt->desc = desc;
        lt->size = numdashes;
        lt->length = patternLen;
        lt->path = std::move(path);
        lt->flags = flag;
        // Use a synthetic monotonic key; consumers do not interpret .handle.
        const std::uint32_t key = 0x10000000u | static_cast<std::uint32_t>(i);
        lt->handle = key;
        ltypemap[key] = lt;
    }
    return true;
}

bool dwgReaderR11::readLayerTable(std::uint32_t hdrPos) {
    // LAYER record (recSize 41):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11)       off 35: color SIGNED RS (neg => OFF)
    //   off 37: ltype-index SIGNED RS (0 => CONTINUOUS)
    //   off 39: CRC RS
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (addr == 0 || recSize < 39 || recNum == 0)
        return true;
    m_layerNames.clear();
    m_layerNames.reserve(recNum);
    for (std::uint16_t i = 0; i < recNum; ++i) {
        const std::uint64_t base = addr + static_cast<std::uint64_t>(i) * recSize;
        if (!fileBuf->setPosition(base))
            break;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        m_layerNames.push_back(name);
        static_cast<void>(fileBuf->getRawShort16());          // off33 used (signed)
        const std::int16_t color =
            static_cast<std::int16_t>(fileBuf->getRawShort16());
        const std::int16_t ltypeIdx =
            static_cast<std::int16_t>(fileBuf->getRawShort16());
        auto* ly = new DRW_Layer();
        ly->name = name;
        ly->color = color;        // negative => layer OFF, mirrors R2000+ path
        ly->flags = flag;
        ly->lineType = ltypeName(ltypeIdx);
        if (ly->lineType.empty()) ly->lineType = "CONTINUOUS";
        const std::uint32_t key = 0x20000000u | static_cast<std::uint32_t>(i);
        ly->handle = key;
        layermap[key] = ly;
    }
    return true;
}

bool dwgReaderR11::readStyleTable(std::uint32_t hdrPos) {
    // STYLE record (recSize 198):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11)       off 35: text_size RD
    //   off 43: width_factor RD      off 51: oblique_angle RD (radians)
    //   off 59: generation RC        off 60: last_height RD
    //   off 68: font_file 64 FIXED   off 132: bigfont_file 64 FIXED
    //   off 196: CRC RS
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (addr == 0 || recSize < 196 || recNum == 0)
        return true;
    m_styleNames.clear();
    m_styleNames.reserve(recNum);
    for (std::uint16_t i = 0; i < recNum; ++i) {
        const std::uint64_t base = addr + static_cast<std::uint64_t>(i) * recSize;
        if (!fileBuf->setPosition(base))
            break;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        m_styleNames.push_back(name);
        static_cast<void>(fileBuf->getRawShort16());          // off33 used (signed)
        const double textSize = fileBuf->getRawDouble();      // off35
        const double widthFactor = fileBuf->getRawDouble();   // off43
        const double obliqueAngle = fileBuf->getRawDouble();  // off51
        const std::uint8_t generation = fileBuf->getRawChar8(); // off59
        const double lastHeight = fileBuf->getRawDouble();    // off60
        std::string font, bigFont;
        ended = false;
        for (int j = 0; j < 64; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) font.push_back(c);
        }
        ended = false;
        for (int j = 0; j < 64; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) bigFont.push_back(c);
        }
        auto* st = new DRW_Textstyle();
        st->name = name;
        st->height = textSize;
        st->width = widthFactor;
        st->oblique = obliqueAngle;
        st->genFlag = generation;
        st->lastHeight = lastHeight;
        st->font = font;
        st->bigFont = bigFont;
        st->flags = flag;
        const std::uint32_t key = 0x30000000u | static_cast<std::uint32_t>(i);
        st->handle = key;
        stylemap[key] = st;
    }
    return true;
}

bool dwgReaderR11::readDwgTables(DRW_Header& /*hdr*/) {
    // The 5 leading table-section headers (BLOCK, LAYER, STYLE, LTYPE, VIEW) are
    // 10 bytes each starting at file offset 0x2C.
    readNameTable(0x2C, m_blockNames);   // BLOCK table
    // Per-record decoders only for R11/AC1009 (the validatable subset; R10 has
    // a different record width — `used` absent, fields at off33 not off35).
    if (version == DRW::AC1009) {
        // Read LTYPE BEFORE LAYER so the layer's ltype-index resolves to a name.
        readLTypeTable(0x4A);            // LTYPE table (0x2C + 30)
        readLayerTable(0x36);            // LAYER table (0x2C + 10)
        readStyleTable(0x40);            // STYLE table (0x2C + 20)
    } else {
        // R10 fallback: still capture names so entities get layer attribution.
        readNameTable(0x36, m_layerNames);
    }
    return true;
}

bool dwgReaderR11::readDwgBlocks(DRW_Interface& intfa) {
    // The BLOCKS section has the same flat record format as ENTITIES; each
    // BLOCK(12) opens a block scope (addBlock), its child entities follow, and
    // ENDBLK(13) closes it (endBlock) -- handled in readEntityR11.
    DRW_DBG("\n=== pre-R13 BLOCKS section ["); DRW_DBGH(m_blocksStart);
    DRW_DBG(","); DRW_DBGH(m_blocksEnd); DRW_DBG(") ===\n");
    return readEntitySection(m_blocksStart, m_blocksEnd, intfa);
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
    std::uint16_t layerIdx = 0;
    if (type != R11_JUMP) {
        layerIdx = fileBuf->getRawShort16();   // 1-based index into the LAYER table
        opts = fileBuf->getRawShort16();       // per-type optional-field flags
    }
    std::uint8_t extra = 0;
    if (flag & FLAG_HAS_PSPACE)
        extra = fileBuf->getRawChar8();
    if (extra & EXTRA_HAS_EED) {
        const std::uint16_t eed = fileBuf->getRawShort16();
        for (std::uint16_t i = 0; i < eed; ++i) fileBuf->getRawChar8();
    }
    // Per-entity color OVERRIDE (signed RC; AutoCAD palette index). Captured
    // into DRW_Entity.color below; 0 == ByBlock, 256 == ByLayer (the layer
    // default); negative values mean the layer is OFF.
    std::int8_t colorOverride = 0;
    bool hasColorOverride = false;
    if (flag & FLAG_HAS_COLOR) {
        colorOverride = static_cast<std::int8_t>(fileBuf->getRawChar8());
        hasColorOverride = true;
    }
    // Per-entity LTYPE OVERRIDE (signed handle: 1B RC on R10, 2B RS on R11).
    // Resolved to a name via m_ltypeNames; ByLayer/ByBlock sentinels handled
    // by ltypeName(). Stored on DRW_Entity.lineType below.
    std::int16_t ltypeOverride = 0;
    bool hasLtypeOverride = false;
    if (flag & FLAG_HAS_LTYPE) {
        if (version == DRW::AC1006)
            ltypeOverride =
                static_cast<std::int16_t>(static_cast<std::int8_t>(fileBuf->getRawChar8()));
        else
            ltypeOverride = static_cast<std::int16_t>(fileBuf->getRawShort16());
        hasLtypeOverride = true;
    }
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
    const std::string curLayer = layerName(layerIdx);
    // applyAttrs: stamp layer + per-entity color/ltype overrides. Color
    // defaults to ByLayer (256); lineType defaults to "BYLAYER" (DRW_Entity's
    // constructor). The override is applied INSTEAD of the layer default.
    auto applyAttrs = [&](DRW_Entity& ent) {
        ent.layer = curLayer;
        if (hasColorOverride) ent.color = colorOverride;
        if (hasLtypeOverride) {
            const std::string n = ltypeName(ltypeOverride);
            if (!n.empty()) ent.lineType = n;
        }
    };
    auto rd = [&]() { return fileBuf->getRawDouble(); };
    auto rd3 = [&]() { DRW_Coord c; c.x = fileBuf->getRawDouble();
                       c.y = fileBuf->getRawDouble(); c.z = fileBuf->getRawDouble();
                       return c; };
    auto readTv = [&]() {  // pre-R13 length-prefixed string: RS count + bytes
        const std::uint16_t n = fileBuf->getRawShort16();
        std::string s; s.reserve(n);
        for (std::uint16_t i = 0; i < n; ++i)
            s.push_back(static_cast<char>(fileBuf->getRawChar8()));
        return s; };

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
            applyAttrs(e);
            intfa.addLine(e);
            break; }
        case R11_POINT: {
            DRW_Point e;
            e.basePoint.x = rd(); e.basePoint.y = rd();
            if (!hasElev) e.basePoint.z = rd();
            e.thickness = thickness;
            applyAttrs(e);
            intfa.addPoint(e);
            break; }
        case R11_CIRCLE: {
            DRW_Circle e;
            e.basePoint = fileBuf->get2RawDouble();
            e.radious = rd();
            e.basePoint.z = elevation;
            e.thickness = thickness;
            applyAttrs(e);
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
            applyAttrs(e);
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
            applyAttrs(e);
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
            applyAttrs(e);
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
            applyAttrs(e);
            intfa.add3dFace(e);
            break; }
        case R11_POLYLINE: {
            // Opens a vertex accumulation; VERTEX records append, SEQEND delivers.
            m_curPoly = std::make_unique<DRW_Polyline>();
            m_curPoly->basePoint.z = elevation;
            applyAttrs(*m_curPoly);
            m_curPoly->thickness = thickness;
            if (opts & 0x01) m_curPoly->flags = fileBuf->getRawChar8(); // closed/3d bits
            break; }
        case R11_VERTEX: {
            DRW_Coord p = fileBuf->get2RawDouble();
            double bulge = 0.0;
            if (opts & 0x01) rd();        // start width
            if (opts & 0x02) rd();        // end width
            if (opts & 0x04) bulge = rd();
            if (m_curPoly)
                m_curPoly->addVertex(DRW_Vertex(p.x, p.y, elevation, bulge));
            break; }
        case R11_SEQEND: {
            if (m_curPoly) { intfa.addPolyline(*m_curPoly); m_curPoly.reset(); }
            break; }
        case R11_BLOCK: {
            DRW_Coord base = fileBuf->get2RawDouble();
            auto readTv = [&]() { const std::uint16_t n = fileBuf->getRawShort16();
                std::string s; s.reserve(n);
                for (std::uint16_t i = 0; i < n; ++i)
                    s.push_back(static_cast<char>(fileBuf->getRawChar8()));
                return s; };
            std::string xref, name;
            if (opts & 0x02) xref = readTv();   // xref path name
            if (opts & 0x04) name = readTv();   // block name (inline)
            DRW_Block blk;
            blk.basePoint = base; blk.basePoint.z = elevation;
            blk.name = name;
            blk.xrefPath = xref;
            intfa.addBlock(blk);                // opens the block scope
            break; }
        case R11_ENDBLK: {
            intfa.endBlock();                   // closes the block scope
            break; }
        case R11_INSERT: {
            DRW_Insert e;
            const std::uint16_t blockIdx = fileBuf->getRawShort16(); // 1-based BLOCK index
            e.basePoint = fileBuf->get2RawDouble();
            e.basePoint.z = elevation;
            if (opts & 0x01) e.xscale = rd();
            if (opts & 0x02) e.yscale = rd();
            if (opts & 0x04) e.angle = rd();
            if (opts & 0x08) e.zscale = rd();
            if (opts & 0x10) e.colcount = fileBuf->getRawShort16();
            if (opts & 0x20) e.rowcount = fileBuf->getRawShort16();
            if (opts & 0x40) e.colspace = rd();
            if (opts & 0x80) e.rowspace = rd();
            if (blockIdx < m_blockNames.size())  // 0-based, verified vs dwgread
                e.name = m_blockNames[blockIdx];
            applyAttrs(e);
            intfa.addInsert(e);
            break; }
        case R11_ATTRIB:
        case R11_ATTDEF: {
            // Attribute (value, follows an INSERT) / attribute definition (in a
            // block). Both are text annotations -> render the visible text as TEXT
            // (the tag/prompt typing is dropped; rendering-first).
            DRW_Text e;
            e.basePoint = fileBuf->get2RawDouble();
            e.basePoint.z = elevation;
            e.height = rd();
            e.text = readTv();                 // ATTRIB value / ATTDEF default
            if (type == R11_ATTDEF) readTv();  // ATTDEF prompt (not rendered)
            readTv();                          // tag (not rendered)
            fileBuf->getRawChar8();            // attribute flags (RC 70)
            if (opts & 0x01) e.angle = rd();   // rotation (shared TEXT opt bit)
            e.thickness = thickness;
            applyAttrs(e);
            intfa.addText(e);
            break; }
        case R11_DIMENSION: {
            // A dimension's graphics (lines/arrows/text) live in an anonymous
            // *D block referenced by the first common field (block, HANDLE 2,2 =
            // RS index). Render it by inserting that block; the typed dimension
            // (defpoints/measurement) is dropped (rendering-first). The block was
            // already delivered by readDwgBlocks.
            const std::uint16_t blockIdx = fileBuf->getRawShort16();
            if (blockIdx < m_blockNames.size() && !m_blockNames[blockIdx].empty()) {
                DRW_Insert e;
                e.name = m_blockNames[blockIdx];
                e.basePoint = DRW_Coord(0.0, 0.0, 0.0);  // *D geometry is in WCS
                applyAttrs(e);
                intfa.addInsert(e);
            }
            break; }
        case R11_SHAPE: {
            // Non-rendering in LibreCAD (kept as metadata), but read so it is not
            // a parse miss. style handle/index resolution is a follow-up.
            DRW_Shape e;
            e.m_insertionPoint = fileBuf->get2RawDouble();
            e.m_insertionPoint.z = elevation;
            e.m_scale = rd();
            fileBuf->getRawShort16();           // style handle (RS for R11)
            if (opts & 0x01) e.m_rotation = rd();
            intfa.addShape(e);
            break; }
        case R11_VIEWPORT: {
            DRW_Viewport e;
            e.basePoint = rd3();                 // center (3RD)
            e.pswidth = rd();                    // width
            e.psheight = rd();                   // height
            fileBuf->getRawShort16();            // viewport id (RS)
            applyAttrs(e);
            intfa.addViewport(e);
            break; }
        default:
            // Unhandled type (INSERT/ATTRIB/ATTDEF/SHAPE/DIMENSION/...) -> skipped
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
