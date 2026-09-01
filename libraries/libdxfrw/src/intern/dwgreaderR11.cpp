/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**  Pre-R13 (R10/R11) DWG reader. See dwgreaderR11.h.                         **
******************************************************************************/

#include <cstdint>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dwgreaderR11.h"
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "../drw_objects.h"
#include "dwgsafety.h"

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

template <typename T>
bool replaceR11Records(std::unordered_map<std::uint32_t, T*>& records,
                       std::uint32_t keyPrefix,
                       std::vector<std::unique_ptr<T>>& staged) {
    std::unordered_map<std::uint32_t, T*> replacement;
    if (!DRW::reserve(replacement, static_cast<int>(staged.size())))
        return false;

    auto cleanup = [](std::unordered_map<std::uint32_t, T*>& values) {
        for (auto& value : values)
            delete value.second;
        values.clear();
    };

    try {
        for (std::size_t i = 0; i < staged.size(); ++i) {
            const std::uint32_t key =
                keyPrefix | static_cast<std::uint32_t>(i);
            const auto result = replacement.emplace(key, staged[i].get());
            if (!result.second) {
                cleanup(replacement);
                return false;
            }
            staged[i].release();
        }
        records.swap(replacement);
    } catch (...) {
        cleanup(replacement);
        return false;
    }

    cleanup(replacement);
    return true;
}

bool validR11Table(std::uint16_t recordSize, std::uint16_t recordCount,
                   std::uint32_t address, std::uint16_t minimumSize,
                   std::uint64_t fileSize) {
    if (recordCount == 0)
        return true;
    if (address == 0 || recordSize < minimumSize)
        return false;
    std::uint64_t tableSize = 0;
    return dwgSafety::multiply(recordSize, recordCount, tableSize)
        && dwgSafety::range(address, tableSize, fileSize);
}
}

// Resolve a pre-R13 $DWGCODEPAGE id (read at file offset 0x3f9) to a
// DRW_TextCodec setCodePage() name, or nullptr to keep the ANSI_1252 default.
// The gate mirrors libreDWG: numheader_vars<=129 => no codepage field present
// (header_variables_r11.spec:267). Ids 0/0xff are "undefined"; ids without a
// libdxfrw ConvTable (DOS-OEM, UTF-16, ...) resolve to nullptr via the shared
// dwgCodePageName() table so the caller keeps the safe ANSI_1252 fallback.
const char* preR13CodePageName(std::uint16_t numHeaderVars, std::uint16_t cp) {
    if (numHeaderVars <= 129) return nullptr;
    if (cp == 0 || cp == 0xff) return nullptr;
    return dwgCodePageName(cp);
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
    if (!fileBuf->isGood())
        return false;
    if (magic == "AC1009")
        version = DRW::AC1009;              // R11/R12
    else if (magic == "AC1006")
        version = DRW::AC1006;              // R10
    else if (magic == "AC1004")
        version = DRW::AC1004;              // R9  (pre-R10, same container)
    else if (magic == "AC1003")
        version = DRW::AC1003;              // R2.6
    else if (magic == "AC2.10")
        version = DRW::AC210;               // R2.10
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
    m_extrasStart = fileBuf->getRawLong32();
    std::uint32_t extrasSize = fileBuf->getRawLong32();
    if (extrasSize > 0xFFFFFF) extrasSize &= 0xFFFFFF;

    const std::uint64_t fileSize = static_cast<std::uint64_t>(fileBuf->size());
    if (!fileBuf->isGood())
        return false;

    auto validateSection = [fileSize](std::uint32_t start,
                                       std::uint32_t length,
                                       std::uint32_t& end) {
        if (start == 0) {
            if (length != 0)
                return false;
            end = 0;
            return true;
        }
        std::uint64_t sectionEnd = 0;
        if (!dwgSafety::add(start, length, sectionEnd)
            || !dwgSafety::range(start, length, fileSize)
            || sectionEnd > std::numeric_limits<std::uint32_t>::max())
            return false;
        end = static_cast<std::uint32_t>(sectionEnd);
        return true;
    };

    if (!validateSection(m_blocksStart, blocksSize, m_blocksEnd)
        || !validateSection(m_extrasStart, extrasSize, m_extrasEnd))
        return false;

    // Reject a truly implausible header, but ALLOW an empty ENTITIES section
    // (entities_end == entities_start): some files (e.g. AC2.10 block.dwg) put
    // all content in the BLOCKS section and leave ENTITIES empty. readDwgBlocks
    // still reads the blocks; an empty section walk yields 0 entities, not an error.
    if (m_entitiesStart == 0 || m_entitiesEnd < m_entitiesStart
        || m_entitiesEnd > fileSize)
        return false;  // implausible header -> not a readable pre-R13 file

    // $DWGCODEPAGE (pre-R13): the codepage id lives at the FIXED file offset
    // 0x3f9 (every preceding header var is fixed-width, so this is version-
    // invariant across R10/R11), gated on numheader_vars@0x11 > 129. Seek both
    // fields directly, map via the shared dwgCodePageName() table, and override
    // readMetaData()'s ANSI_1252 default when a supported codepage is present.
    // setPosition-guarded so a truncated file silently keeps the default. This
    // runs before processDwg() reads any table/entity string, so the decoder is
    // correct for all subsequent text. dwgread reports codepage 30 for ACEB10.
    if (fileBuf->setPosition(0x11)) {
        const std::uint16_t numHeaderVars = fileBuf->getRawShort16();
        if (fileBuf->setPosition(0x3f9)) {
            const std::uint16_t cp = fileBuf->getRawShort16();
            if (const char* name = preR13CodePageName(numHeaderVars, cp))
                setCodePage(name);
        }
    }
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
    if (version != DRW::AC1009 && version != DRW::AC1006)
        return true;  // pre-R10 only. R10 (AC1006) is byte-IDENTICAL to R11 for
                      // the entire implemented subset (0x5E..PLINEWID/0x36f):
                      // the spec branches that differ (FIELD_CMC CECOLOR, the
                      // numheader_vars<=160 early-stop) all live AFTER PLINEWID,
                      // which we do not read. Validated vs dwgread on r10/*.dwg.

    // Eager table-name reads for CLAYER/TEXTSTYLE/CELTYPE resolution. These
    // are seek-absolute (readNameTable does setPosition), so they do not
    // disturb our header-walk cursor.
    if (!readNameTable(0x36, m_layerNames)
        || !readNameTable(0x40, m_styleNames)
        || !readNameTable(0x4A, m_ltypeNames))
        return false;

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
    try {
        if (!fileBuf->setPosition(hdrPos))
            return false;
        const std::uint16_t recSize = fileBuf->getRawShort16();
        const std::uint16_t recNum = fileBuf->getRawShort16();
        fileBuf->getRawShort16();  // flags
        const std::uint32_t addr = fileBuf->getRawLong32();
        if (!fileBuf->isGood())
            return false;
        if (recNum == 0 && addr == 0) {
            out.clear();
            return true;  // an explicitly empty table
        }
        if (addr == 0 || recSize < 33)
            return false;

        std::uint64_t recordBytes = 0;
        if (!dwgSafety::multiply(recSize, recNum, recordBytes)
            || !dwgSafety::range(addr, recordBytes, fileBuf->size()))
            return false;

        std::vector<std::string> names;
        if (!DRW::reserve(names, static_cast<int>(recNum)))
            return false;
        for (std::uint16_t i = 0; i < recNum; ++i) {
            std::uint64_t recordOffset = 0;
            std::uint64_t nameOffset = 0;
            if (!dwgSafety::multiply(i, recSize, recordOffset)
                || !dwgSafety::add(addr, recordOffset, recordOffset)
                || !dwgSafety::add(recordOffset, 1, nameOffset)
                || !fileBuf->setPosition(nameOffset))
                return false;
            std::string name;
            bool ended = false;
            for (int j = 0; j < 32; ++j) {
                const char c = static_cast<char>(fileBuf->getRawChar8());
                if (c == '\0') ended = true;
                if (!ended) name.push_back(c);
            }
            if (!fileBuf->isGood())
                return false;
            names.push_back(std::move(name));
        }
        out.swap(names);
        return true;
    } catch (...) {
        return false;
    }
}

bool dwgReaderR11::readPreR13String(std::string& out) {
    const std::uint16_t length = fileBuf->getRawShort16();
    out.clear();
    if (!fileBuf->isGood()) {
        fileBuf->invalidate();
        return false;
    }
    std::string value;
    if (!DRW::reserve(value, static_cast<int>(length))) {
        fileBuf->invalidate();
        return false;
    }
    for (std::uint16_t i = 0; i < length; ++i)
        value.push_back(static_cast<char>(fileBuf->getRawChar8()));
    if (!fileBuf->isGood()) {
        fileBuf->invalidate();
        return false;
    }
    out = std::move(value);
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
    // LTYPE record (recSize R11=191 / R10=187):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11 only)  off 35/33: description 48 FIXED bytes
    //   alignment RC ('A')   numdashes RCu (0..12)
    //   pattern_len RD       dashes_r11 12 * RD (96B FIXED)
    // R10 (AC1006) is byte-identical minus the 2-byte `used` field (gated
    // VERSION(R_11) in libreDWG COMMON_TABLE_FLAGS) -> fields shift up by 2.
    // Only the first `numdashes` doubles in dashes_r11 are valid; the unused
    // slots are uninitialised garbage and MUST be truncated.
    const bool hasUsed = (version == DRW::AC1009);
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (!fileBuf->isGood())
        return false;
    if (!validR11Table(recSize, recNum, addr, hasUsed ? 189 : 187,
                       fileBuf->size()))
        return false;
    if (recNum == 0) {
        m_ltypeNames.clear();
        return true;
    }

    std::vector<std::string> names;
    std::vector<std::unique_ptr<DRW_LType>> parsed;
    if (!DRW::reserve(names, static_cast<int>(recNum))
        || !DRW::reserve(parsed, static_cast<int>(recNum)))
        return false;
    for (std::uint16_t i = 0; i < recNum; ++i) {
        std::uint64_t offset = 0;
        if (!dwgSafety::multiply(i, recSize, offset)
            || !dwgSafety::add(addr, offset, offset)
            || !fileBuf->setPosition(offset))
            return false;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        // off 33: used (signed RS, ignored — header lists "used count"
        // sentinel; libreDWG keeps it for debugging only). R11 only; R10 omits.
        if (hasUsed) static_cast<void>(fileBuf->getRawShort16());
        // description (48 FIXED null-padded bytes).
        std::string desc;
        ended = false;
        for (int j = 0; j < 48; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) desc.push_back(c);
        }
        // off 83: alignment (always 'A' for AutoCAD ltypes); off 84: numdashes.
        const std::uint8_t alignment = fileBuf->getRawChar8();
        const std::uint8_t numdashes = fileBuf->getRawChar8();
        if (numdashes > 12)
            return false;
        const double patternLen = fileBuf->getRawDouble();    // off 85
        if (!std::isfinite(patternLen))
            return false;
        std::vector<double> path;
        std::vector<DRW_LTypeSegment> segments;
        if (!DRW::reserve(path, static_cast<int>(numdashes))
            || !DRW::reserve(segments, static_cast<int>(numdashes)))
            return false;
        for (std::uint8_t d = 0; d < 12; ++d) {
            const double v = fileBuf->getRawDouble();
            if (d < numdashes) {
                if (!std::isfinite(v))
                    return false;
                path.push_back(v);
                DRW_LTypeSegment segment;
                segment.length = v;
                segments.push_back(std::move(segment));
            }
        }
        if (!fileBuf->isGood())
            return false;
        auto lt = std::make_unique<DRW_LType>();
        lt->name = name;
        lt->desc = desc;
        lt->size = numdashes;
        lt->length = patternLen;
        lt->alignment = alignment;
        lt->path = std::move(path);
        lt->segments = std::move(segments);
        lt->flags = flag;
        lt->handle = 0x10000000u | static_cast<std::uint32_t>(i);
        names.push_back(name);
        parsed.push_back(std::move(lt));
    }
    if (!replaceR11Records(ltypemap, 0x10000000u, parsed))
        return false;
    m_ltypeNames.swap(names);
    return true;
}

bool dwgReaderR11::readLayerTable(std::uint32_t hdrPos) {
    // LAYER record (recSize R11=41 / R10=37):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11 only)  color SIGNED RS (neg => OFF)
    //   ltype-index SIGNED RS (0 => CONTINUOUS)
    // R10 (AC1006) omits the 2-byte `used` field; the rest is identical.
    const bool hasUsed = (version == DRW::AC1009);
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (!fileBuf->isGood())
        return false;
    if (!validR11Table(recSize, recNum, addr, hasUsed ? 39 : 37,
                       fileBuf->size()))
        return false;
    if (recNum == 0) {
        m_layerNames.clear();
        return true;
    }
    std::vector<std::string> names;
    std::vector<std::unique_ptr<DRW_Layer>> parsed;
    if (!DRW::reserve(names, static_cast<int>(recNum))
        || !DRW::reserve(parsed, static_cast<int>(recNum)))
        return false;
    for (std::uint16_t i = 0; i < recNum; ++i) {
        std::uint64_t offset = 0;
        if (!dwgSafety::multiply(i, recSize, offset)
            || !dwgSafety::add(addr, offset, offset)
            || !fileBuf->setPosition(offset))
            return false;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        if (hasUsed) static_cast<void>(fileBuf->getRawShort16()); // off33 used (R11)
        const std::int16_t color =
            static_cast<std::int16_t>(fileBuf->getRawShort16());
        const std::int16_t ltypeIdx =
            static_cast<std::int16_t>(fileBuf->getRawShort16());
        auto ly = std::make_unique<DRW_Layer>();
        ly->name = name;
        ly->color = color;        // negative => layer OFF, mirrors R2000+ path
        ly->flags = flag;
        ly->lineType = ltypeName(ltypeIdx);
        if (ly->lineType.empty()) ly->lineType = "CONTINUOUS";
        ly->handle = 0x20000000u | static_cast<std::uint32_t>(i);
        if (!fileBuf->isGood())
            return false;
        names.push_back(name);
        parsed.push_back(std::move(ly));
    }
    if (!replaceR11Records(layermap, 0x20000000u, parsed))
        return false;
    m_layerNames.swap(names);
    return true;
}

bool dwgReaderR11::readStyleTable(std::uint32_t hdrPos) {
    // STYLE record (recSize R11=198 / R10=194):
    //   off 0: flag RC               off 1: name 32 FIXED bytes
    //   off 33: used RSd (R11 only)  text_size RD
    //   width_factor RD      oblique_angle RD (radians)
    //   generation RC        last_height RD
    //   font_file 64 FIXED   bigfont_file 64 FIXED
    // R10 (AC1006) omits the 2-byte `used` field; the rest is identical.
    const bool hasUsed = (version == DRW::AC1009);
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::uint16_t recNum = fileBuf->getRawShort16();
    fileBuf->getRawShort16();  // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (!fileBuf->isGood())
        return false;
    if (!validR11Table(recSize, recNum, addr, hasUsed ? 196 : 194,
                       fileBuf->size()))
        return false;
    if (recNum == 0) {
        m_styleNames.clear();
        return true;
    }
    std::vector<std::string> names;
    std::vector<std::unique_ptr<DRW_Textstyle>> parsed;
    if (!DRW::reserve(names, static_cast<int>(recNum))
        || !DRW::reserve(parsed, static_cast<int>(recNum)))
        return false;
    for (std::uint16_t i = 0; i < recNum; ++i) {
        std::uint64_t offset = 0;
        if (!dwgSafety::multiply(i, recSize, offset)
            || !dwgSafety::add(addr, offset, offset)
            || !fileBuf->setPosition(offset))
            return false;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        if (hasUsed) static_cast<void>(fileBuf->getRawShort16()); // off33 used (R11)
        const double textSize = fileBuf->getRawDouble();
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
        auto st = std::make_unique<DRW_Textstyle>();
        st->name = name;
        st->height = textSize;
        st->width = widthFactor;
        st->oblique = obliqueAngle;
        st->genFlag = generation;
        st->lastHeight = lastHeight;
        st->font = font;
        st->bigFont = bigFont;
        st->flags = flag;
        st->handle = 0x30000000u | static_cast<std::uint32_t>(i);
        if (!fileBuf->isGood())
            return false;
        names.push_back(name);
        parsed.push_back(std::move(st));
    }
    if (!replaceR11Records(stylemap, 0x30000000u, parsed))
        return false;
    m_styleNames.swap(names);
    return true;
}

bool dwgReaderR11::readDwgTables(DRW_Header& /*hdr*/) {
    // The 5 leading table-section headers (BLOCK, LAYER, STYLE, LTYPE, VIEW) are
    // 10 bytes each starting at file offset 0x2C.
    if (!readNameTable(0x2C, m_blockNames))
        return false;                    // BLOCK table
    // Per-record decoders for BOTH R11/AC1009 and R10/AC1006: R10 records are
    // byte-identical minus the 2-byte `used` field, which the walkers skip via
    // their internal hasUsed = (version == AC1009) gate.
    // Read LTYPE BEFORE LAYER so the layer's ltype-index resolves to a name.
    if (!readLTypeTable(0x4A)             // LTYPE table (0x2C + 30)
        || !readLayerTable(0x36)          // LAYER table (0x2C + 10)
        || !readStyleTable(0x40))         // STYLE table (0x2C + 20)
        return false;

    // EMBEDDED extended tables. These exist only when the header carries the
    // extended variable block: numheader_vars@0x11 > 158 (R10/AC1006 reports
    // 158 -> none; R11/AC1009 reports 205 -> present). dwgTs reads APPID and
    // DIMSTYLE here name-only (its "recommended name-only path"); VPORT/VIEW/
    // UCS/VX are DORMANT in dwgTs too, so we match parity by omitting them.
    // Fixed on-disk descriptor offsets (byte-verified vs ACEB10.dwg, matching
    // dwgTs PRE_R13_EMBEDDED_SECTION_HEADERS): APPID @0x512, DIMSTYLE @0x522.
    std::uint16_t numHeaderVars = 0;
    if (fileBuf->setPosition(0x11))
        numHeaderVars = fileBuf->getRawShort16();
    if (numHeaderVars > 158) {
        if (!readExtendedNameTable(0x512, /*isDimstyle=*/false) // APPID
            || !readExtendedNameTable(0x522, /*isDimstyle=*/true)) // DIMSTYLE
            return false;
    }
    return true;
}

bool dwgReaderR11::readExtendedNameTable(std::uint32_t hdrPos, bool isDimstyle) {
    // Descriptor: recSize RS, recNum RSd(signed), flags RS, address RL (10 bytes,
    // same as the @0x2C headers). Each record is recSize bytes: flag RC + name
    // 32 FIXED. We keep name+flag only (dwgTs's name-only path); the record is
    // stored into the base dimstylemap/appIdmap, which processDwg delivers via
    // addDimStyle/addAppId (libdwgr.cpp).
    if (!fileBuf->setPosition(hdrPos))
        return false;
    const std::uint16_t recSize = fileBuf->getRawShort16();
    const std::int16_t recNum =
        static_cast<std::int16_t>(fileBuf->getRawShort16());
    fileBuf->getRawShort16(); // flags
    const std::uint32_t addr = fileBuf->getRawLong32();
    if (!fileBuf->isGood() || recNum < 0)
        return false;
    if (!validR11Table(recSize, static_cast<std::uint16_t>(recNum), addr, 33,
                       fileBuf->size()))
        return false;
    if (recNum == 0)
        return true;

    if (isDimstyle) {
        std::vector<std::unique_ptr<DRW_Dimstyle>> parsed;
        if (!DRW::reserve(parsed, recNum))
            return false;
        for (std::int16_t i = 0; i < recNum; ++i) {
            std::uint64_t offset = 0;
            if (!dwgSafety::multiply(static_cast<std::uint16_t>(i), recSize,
                                     offset)
                || !dwgSafety::add(addr, offset, offset)
                || !fileBuf->setPosition(offset))
                return false;
            const std::uint8_t flag = fileBuf->getRawChar8();
            std::string name;
            bool ended = false;
            for (int j = 0; j < 32; ++j) {
                const char c = static_cast<char>(fileBuf->getRawChar8());
                if (c == '\0') ended = true;
                if (!ended) name.push_back(c);
            }
            if (!fileBuf->isGood())
                return false;
            auto ds = std::make_unique<DRW_Dimstyle>();
            ds->name = std::move(name);
            ds->flags = flag;
            ds->handle = 0x50000000u | static_cast<std::uint32_t>(i);
            parsed.push_back(std::move(ds));
        }
        return replaceR11Records(dimstylemap, 0x50000000u, parsed);
    }

    std::vector<std::unique_ptr<DRW_AppId>> parsed;
    if (!DRW::reserve(parsed, recNum))
        return false;
    for (std::int16_t i = 0; i < recNum; ++i) {
        std::uint64_t offset = 0;
        if (!dwgSafety::multiply(static_cast<std::uint16_t>(i), recSize,
                                 offset)
            || !dwgSafety::add(addr, offset, offset)
            || !fileBuf->setPosition(offset))
            return false;
        const std::uint8_t flag = fileBuf->getRawChar8();
        std::string name;
        bool ended = false;
        for (int j = 0; j < 32; ++j) {
            const char c = static_cast<char>(fileBuf->getRawChar8());
            if (c == '\0') ended = true;
            if (!ended) name.push_back(c);
        }
        if (!fileBuf->isGood())
            return false;
        auto ai = std::make_unique<DRW_AppId>();
        ai->name = std::move(name);
        ai->flags = flag;
        ai->handle = 0x40000000u | static_cast<std::uint32_t>(i);
        parsed.push_back(std::move(ai));
    }
    return replaceR11Records(appIdmap, 0x40000000u, parsed);
}

bool dwgReaderR11::readDwgBlocks(DRW_Interface& intfa) {
    // The BLOCKS section has the same flat record format as ENTITIES; each
    // BLOCK(12) opens a block scope (addBlock), its child entities follow, and
    // ENDBLK(13) closes it (endBlock) -- handled in readEntityR11.
    DRW_DBG("\n=== pre-R13 BLOCKS section ["); DRW_DBGH(m_blocksStart);
    DRW_DBG(","); DRW_DBGH(m_blocksEnd); DRW_DBG(") ===\n");
    bool ret = readEntitySection(m_blocksStart, m_blocksEnd, intfa);
    if (m_blockOpen) {
        // Keep the consumer's block stack balanced even when ENDBLK was
        // truncated or absent; report the structural failure to the caller.
        intfa.endBlock();
        m_blockOpen = false;
        ret = false;
    }
    return ret;
}

bool dwgReaderR11::readDwgEntities(DRW_Interface& intfa) {
    DRW_DBG("\n=== pre-R13 ENTITIES section ["); DRW_DBGH(m_entitiesStart);
    DRW_DBG(","); DRW_DBGH(m_entitiesEnd); DRW_DBG(") ===\n");
    if (!readEntitySection(m_entitiesStart, m_entitiesEnd, intfa))
        return false;

    // EXTRAS section: same flat record format as ENTITIES. It holds entity
    // records that overflow the main section -- including JUMP-split POLYLINE
    // continuations (a VERTEX run interrupted by a JUMP resumes here). dwgTs
    // reads entities+blocks+extras; we read EXTRAS right after ENTITIES so an
    // open polyline (m_curPoly) accumulates across the section boundary. Bounds
    // are best-effort (a malformed/empty EXTRAS is simply skipped).
    const std::uint32_t fileSize = static_cast<std::uint32_t>(fileBuf->size());
    if (m_extrasStart != 0 && m_extrasEnd > m_extrasStart
        && m_extrasEnd <= fileSize) {
        DRW_DBG("\n=== pre-R13 EXTRAS section ["); DRW_DBGH(m_extrasStart);
        DRW_DBG(","); DRW_DBGH(m_extrasEnd); DRW_DBG(") ===\n");
        if (!readEntitySection(m_extrasStart, m_extrasEnd, intfa))
            return false;
    }

    // Flush a polyline left open by the last section (missing/JUMP-deferred
    // SEQEND) so its vertices are not lost.
    if (m_curPoly) { intfa.addPolyline(*m_curPoly); m_curPoly.reset(); }
    return true;
}

bool dwgReaderR11::readEntitySection(std::uint32_t start, std::uint32_t end,
                                     DRW_Interface& intfa) {
    if (start == 0 || end <= start)
        return true;
    if (!dwgSafety::range(start, end - start, fileBuf->size()))
        return false;
    if (!fileBuf->setPosition(start))
        return false;
    std::uint32_t guard = 0;
    while (fileBuf->getPosition() < end) {
        const std::uint64_t position = fileBuf->getPosition();
        if (end - position < 4 || ++guard > 2000000)
            return false;
        if (!readEntityR11(intfa, end))
            return false;
        if (fileBuf->getPosition() <= position
            || fileBuf->getPosition() > end)
            return false;
    }
    return fileBuf->getPosition() == end;
}

bool dwgReaderR11::readEntityR11(DRW_Interface& intfa,
                                 std::uint32_t sectionEnd) {
    const std::uint64_t recStart = fileBuf->getPosition();
    if (recStart > sectionEnd || sectionEnd - recStart < 4)
        return false;
    const std::uint8_t typeByte = fileBuf->getRawChar8();
    const bool deleted = (typeByte & 0x80) != 0;
    const std::uint8_t type = typeByte & 0x7F;
    const std::uint8_t flag = fileBuf->getRawChar8();
    const std::uint16_t size = fileBuf->getRawShort16();
    if (!fileBuf->isGood() || size < 5)
        return false;  // below the minimum header -> unrecoverable desync
    std::uint64_t recEnd = 0;
    if (!dwgSafety::add(recStart, size, recEnd) || recEnd > sectionEnd)
        return false;  // a record may not cross its containing section

    // Decode the record body through a private bounded cursor.  The shared
    // file cursor has no record-level limit, so a short record could otherwise
    // consume the next record before its malformed body was noticed.
    std::vector<std::uint8_t> recordBytes;
    if (!DRW::resize(recordBytes, static_cast<int>(size - 4)))
        return false;
    if (!fileBuf->getBytes(recordBytes.data(), recordBytes.size()))
        return false;
    auto sourceBuffer = std::move(fileBuf);
    fileBuf = std::make_unique<dwgBuffer>(recordBytes.data(),
                                          recordBytes.size(), &decoder);

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
    // Pre-R10 (R2.6/R9/R2.10) common-header/body deltas vs R10/R11: 1-byte LTYPE
    // override handle; HAS_ELEVATION read for ALL types; LINE/POINT/3DLINE/3DFACE
    // bodies are 2D (Z from elevation). R10 already uses the 1-byte handle.
    const bool preR10 = (version < DRW::AC1006);
    bool hasLtypeOverride = false;
    if (flag & FLAG_HAS_LTYPE) {
        if (version != DRW::AC1009)   // PRE(R_11): 1-byte handle (R10 + pre-R10)
            ltypeOverride =
                static_cast<std::int16_t>(static_cast<std::int8_t>(fileBuf->getRawChar8()));
        else                          // R11: 2-byte handle
            ltypeOverride = static_cast<std::int16_t>(fileBuf->getRawShort16());
        hasLtypeOverride = true;
    }
    double elevation = 0.0;
    // HAS_ELEVATION: read for ALL types in pre-R10; for R10/R11 it is suppressed
    // for LINE/POINT/3DFACE (their Z lives in the body).
    if ((flag & FLAG_HAS_ELEVATION)
        && (preR10
            || (type != R11_LINE && type != R11_POINT && type != R11_3DFACE)))
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
    // Route pre-R13 codepage-encoded bytes through the decoder set in
    // readFileHeader() (ANSI_1252 by default). ASCII without \U+/\M+ escapes
    // is a byte-identical no-op (existing synthetic R11 fixtures unaffected);
    // high-byte Latin-1/Cyrillic/CJK bytes become valid UTF-8 instead of the
    // mojibake delivered before. Note: DRW_ConvTable::toUtf8 always interprets
    // \U+XXXX / \M+cXXXX escapes even on ASCII (matches AutoCAD).
    auto toUtf8 = [&](const std::string& s) { return decoder.toUtf8(s); };
    auto readTv = [&]() {  // pre-R13 length-prefixed string: RS count + bytes
        std::string s;
        readPreR13String(s);
        return toUtf8(s); };

    if (!deleted) {
        switch (type) {
        case R11_LINE: {
            DRW_Line e;
            if (!preR10 && !hasElev) {   // R10/R11: full 3D
                e.basePoint = rd3();
                e.secPoint = rd3();
            } else {                     // pre-R10 always 2D; R10/R11 w/ elevation
                e.basePoint = fileBuf->get2RawDouble(); e.basePoint.z = elevation;
                e.secPoint = fileBuf->get2RawDouble();  e.secPoint.z = elevation;
            }
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addLine(e);
            break; }
        case R11_3DLINE: {
            DRW_Line e;
            if (preR10) {
                // pre-R10: each endpoint is 3D iff its opts bit is set, else 2D
                // (Z from elevation); no extrusion vector.
                if (opts & 0x01) e.basePoint = rd3();
                else { e.basePoint = fileBuf->get2RawDouble(); e.basePoint.z = elevation; }
                if (opts & 0x02) e.secPoint = rd3();
                else { e.secPoint = fileBuf->get2RawDouble(); e.secPoint.z = elevation; }
            } else {
                e.basePoint = rd3();
                e.secPoint = rd3();
                if (opts & 0x01)
                    e.extPoint = rd3();
            }
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addLine(e);
            break; }
        case R11_POINT: {
            DRW_Point e;
            e.basePoint.x = rd(); e.basePoint.y = rd();
            if (preR10) e.basePoint.z = elevation;   // pre-R10: 2D, Z from elevation
            else if (!hasElev) e.basePoint.z = rd();
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addPoint(e);
            break; }
        case R11_CIRCLE: {
            DRW_Circle e;
            e.basePoint = fileBuf->get2RawDouble();
            e.radious = rd();
            e.basePoint.z = elevation;
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addCircle(e);
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
            if (fileBuf->isGood()) intfa.addArc(e);
            break; }
        case R11_TEXT: {
            DRW_Text e;
            e.basePoint = fileBuf->get2RawDouble();
            e.basePoint.z = elevation;
            e.height = rd();
            std::string s;
            readPreR13String(s);
            e.text = toUtf8(s);
            if (opts & 0x01) e.angle = rd();
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addText(e);
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
            if (fileBuf->isGood()) {
                if (type == R11_TRACE) intfa.addTrace(e); else intfa.addSolid(e);
            }
            break; }
        case R11_3DFACE: {
            DRW_3Dface e;
            if (preR10 || hasElev) {     // pre-R10 always 2D; R10/R11 w/ elevation
                e.basePoint = fileBuf->get2RawDouble();   e.basePoint.z = elevation;
                e.secPoint = fileBuf->get2RawDouble();     e.secPoint.z = elevation;
                e.thirdPoint = fileBuf->get2RawDouble();   e.thirdPoint.z = elevation;
                e.fourPoint = fileBuf->get2RawDouble();    e.fourPoint.z = elevation;
            } else {
                e.basePoint = rd3();
                e.secPoint = rd3();
                e.thirdPoint = rd3();
                e.fourPoint = rd3();
            }
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.add3dFace(e);
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
            if (m_curPoly && fileBuf->isGood())
                m_curPoly->addVertex(DRW_Vertex(p.x, p.y, elevation, bulge));
            break; }
        case R11_SEQEND: {
            if (fileBuf->isGood() && m_curPoly) {
                intfa.addPolyline(*m_curPoly);
                m_curPoly.reset();
            }
            break; }
        case R11_BLOCK: {
            DRW_Coord base = fileBuf->get2RawDouble();
            auto readTv = [&]() {
                std::string s;
                readPreR13String(s);
                return s; };
            std::string xref, name;
            if (opts & 0x02) xref = readTv();   // xref path name
            if (opts & 0x04) name = readTv();   // block name (inline)
            DRW_Block blk;
            blk.basePoint = base; blk.basePoint.z = elevation;
            blk.name = name;
            blk.xrefPath = xref;
            if (fileBuf->isGood() && !m_blockOpen) {
                intfa.addBlock(blk); // opens the block scope
                m_blockOpen = true;
            }
            break; }
        case R11_ENDBLK: {
            if (fileBuf->isGood() && m_blockOpen) {
                intfa.endBlock(); // closes the block scope
                m_blockOpen = false;
            }
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
            if (fileBuf->isGood()) intfa.addInsert(e);
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
            if (opts & 0x02) e.angle = rd();   // rotation, R11OPTS(2) dwg.spec:216/419
            e.thickness = thickness;
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addText(e);
            break; }
        case R11_DIMENSION: {
            // The pre-R13 DIMENSION record carries the typed dimension fields
            // (defpoints/textpoint/dimtype/etc.) PLUS a reference to an
            // anonymous `*D` block that holds the rendered graphics (lines,
            // arrows, text). Decode each legacy grammar through the matching
            // existing DRW_Dim* callback; the block remains the fallback only
            // for an unknown dimension type.
            //
            // Sequence (after the common header already consumed above):
            //   block RS         -> *D block index
            //   def_pt 3RD (2RD before R10)
            //   text_midpt 2RD
            //   opts&0x1: clone_ins_pt 2RD
            //   opts&0x2: flag RC (dimtype = flag & 15)
            //   opts&0x4: user_text TV
            //   per-type fields (each opts-gated; see dwg.spec dimension.spec)
            const std::uint16_t blockIdx = fileBuf->getRawShort16();
            auto rdDimPoint = [&]() {
                // The pre-R13 specification uses 2RD for dimension points
                // before R10 and 3RD from R10 onward.
                return preR10 ? fileBuf->get2RawDouble() : rd3();
            };
            auto rdDiametricPoint = [&]() {
                // DIAMETER is the one subtype whose optional point remains
                // 2RD when the common entity carries an elevation.
                return (preR10 || hasElev) ? fileBuf->get2RawDouble() : rd3();
            };
            DRW_Coord defPt = rdDimPoint();
            DRW_Coord textMid = fileBuf->get2RawDouble();   // 2RD; z=0
            DRW_Coord cloneIns(0.0, 0.0, 0.0);
            if (opts & 0x1) cloneIns = fileBuf->get2RawDouble();
            std::uint8_t dimFlag = 0;
            if (opts & 0x2) dimFlag = fileBuf->getRawChar8();
            std::string userText;
            if (opts & 0x4) userText = readTv();
            const std::uint8_t dimtype = dimFlag & 0x0F;

            // Common DRW_Dimension setup: shared across all dimtypes; only
            // the public setters are usable (setPt2..setPt6 etc. are
            // protected, friend-scoped to the base reader path). The typed
            // subclass exposes per-type public setters (setClonePoint on
            // Aligned, setDef1Point/setDef2Point, setAngle/setOblique on
            // Linear). Layer + per-entity overrides via applyAttrs (DRW_Dim*
            // IS-A DRW_Entity).
            auto setupBase = [&](DRW_Dimension& dim) {
                dim.setDefPoint(defPt);
                dim.setTextPoint(textMid);
                dim.setText(userText);
                dim.type = dimFlag;
                dim.setStyle("");                           // consumer falls back to m_dimStyle
                dim.setName(blockIdx < m_blockNames.size()
                                ? m_blockNames[blockIdx]
                                : std::string());
                applyAttrs(dim);
            };

            const double RAD2DEG = 57.29577951308232;        // 180/pi

            switch (dimtype) {
            case 0: {  // LINEAR
                DRW_Coord x1(0, 0, 0), x2(0, 0, 0);
                double dim_rotation = 0.0, oblique = 0.0, text_rotation = 0.0;
                if (opts & 0x008) x1 = rdDimPoint();
                if (opts & 0x010) x2 = rdDimPoint();
                if (opts & 0x100) dim_rotation = rd();      // RD radians
                if (opts & 0x200) oblique = rd();           // RD radians
                if (opts & 0x400) text_rotation = rd();     // RD radians
                DRW_Coord extrusion(0.0, 0.0, 1.0);
                if (opts & 0x4000) extrusion = rd3();       // extrusion 3RD
                if (opts & 0x8000) fileBuf->getRawShort16();// dimstyle RS index (unresolved)
                DRW_DimLinear dim;
                setupBase(dim);
                dim.setClonePoint(cloneIns);                // public on Aligned/Linear
                dim.setDef1Point(x1);
                dim.setDef2Point(x2);
                dim.setAngle(dim_rotation * RAD2DEG);       // consumer deg2rads
                dim.setOblique(oblique * RAD2DEG);
                dim.setDir(text_rotation);                  // RADIANS (consumer raw)
                dim.setExtrusion(extrusion);
                if (fileBuf->isGood()) intfa.addDimLinear(&dim);
                break; }
            case 1: {  // ALIGNED
                DRW_Coord x1(0, 0, 0), x2(0, 0, 0);
                double oblique_unused = 0.0, text_rotation = 0.0;
                if (opts & 0x008) x1 = rdDimPoint();
                if (opts & 0x010) x2 = rdDimPoint();
                if (opts & 0x100) oblique_unused = rd();    // 0x100 here = oblique;
                                                            // DRW_DimAligned has no
                                                            // public oblique setter
                                                            // AND addDimAlign ignores
                                                            // it -> read+discard.
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x8000) fileBuf->getRawShort16();
                (void)oblique_unused;
                DRW_DimAligned dim;
                setupBase(dim);
                dim.setClonePoint(cloneIns);
                dim.setDef1Point(x1);
                dim.setDef2Point(x2);
                dim.setDir(text_rotation);                  // RADIANS
                if (fileBuf->isGood()) intfa.addDimAlign(&dim);
                break; }
            case 2: {  // ANGULAR (two lines)
                DRW_Coord firstLine1(0, 0, 0), firstLine2(0, 0, 0);
                DRW_Coord secondLine1(0, 0, 0), dimPoint(0, 0, 0);
                double text_rotation = 0.0;
                if (opts & 0x008) firstLine1 = rdDimPoint();
                if (opts & 0x010) firstLine2 = rdDimPoint();
                if (opts & 0x020) secondLine1 = rdDimPoint();
                if (opts & 0x040) dimPoint = fileBuf->get2RawDouble();
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x8000) fileBuf->getRawShort16();
                DRW_DimAngular dim;
                setupBase(dim);
                dim.setFirstLine1(firstLine1);
                dim.setFirstLine2(firstLine2);
                dim.setSecondLine1(secondLine1);
                dim.setSecondLine2(defPt);
                dim.setDimPoint(dimPoint);
                dim.setDir(text_rotation);
                if (fileBuf->isGood()) intfa.addDimAngular(&dim);
                break; }
            case 3: {  // DIAMETER
                DRW_Coord diameterPoint(0, 0, 0);
                double leader_length = 0.0, text_rotation = 0.0;
                DRW_Coord extrusion(0.0, 0.0, 1.0);
                if (opts & 0x020) diameterPoint = rdDiametricPoint();
                if (opts & 0x080) leader_length = rd();
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x4000) extrusion = rd3();
                if (opts & 0x8000) fileBuf->getRawShort16();
                DRW_DimDiametric dim;
                setupBase(dim);
                dim.setDiameter1Point(diameterPoint);
                dim.setDiameter2Point(defPt);
                dim.setLeaderLength(leader_length);
                dim.setDir(text_rotation);
                dim.setExtrusion(extrusion);
                if (fileBuf->isGood()) intfa.addDimDiametric(&dim);
                break; }
            case 4: {  // RADIUS
                DRW_Coord diameterPoint(0, 0, 0);
                double leader_length = 0.0, text_rotation = 0.0;
                DRW_Coord extrusion(0.0, 0.0, 1.0);
                if (opts & 0x020) diameterPoint = rdDimPoint();
                if (opts & 0x080) leader_length = rd();
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x4000) extrusion = rd3();
                if (opts & 0x8000) fileBuf->getRawShort16();
                DRW_DimRadial dim;
                setupBase(dim);
                dim.setCenterPoint(defPt);
                dim.setDiameterPoint(diameterPoint);
                dim.setLeaderLength(leader_length);
                dim.setDir(text_rotation);
                dim.setExtrusion(extrusion);
                if (fileBuf->isGood()) intfa.addDimRadial(&dim);
                break; }
            case 5: {  // ANGULAR (three points)
                DRW_Coord firstLine(0, 0, 0), secondLine(0, 0, 0);
                DRW_Coord vertex(0, 0, 0), dimPoint(0, 0, 0);
                double text_rotation = 0.0;
                if (opts & 0x008) firstLine = rdDimPoint();
                if (opts & 0x010) secondLine = rdDimPoint();
                if (opts & 0x020) vertex = rdDimPoint();
                if (opts & 0x040) dimPoint = fileBuf->get2RawDouble();
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x8000) fileBuf->getRawShort16();
                DRW_DimAngular3p dim;
                setupBase(dim);
                dim.setFirstLine(firstLine);
                dim.setSecondLine(secondLine);
                dim.SetVertexPoint(vertex);
                dim.setDimPoint(defPt);
                // The legacy code-16 point has no public field in
                // DRW_DimAngular3p; consume it for alignment and retain the
                // common definition point used by the current model.
                (void)dimPoint;
                dim.setDir(text_rotation);
                if (fileBuf->isGood()) intfa.addDimAngular3P(&dim);
                break; }
            case 6: {  // ORDINATE
                DRW_Coord featurePoint(0, 0, 0), leaderEnd(0, 0, 0);
                double text_rotation = 0.0;
                if (opts & 0x008) featurePoint = rdDimPoint();
                if (opts & 0x010) leaderEnd = rdDimPoint();
                if (opts & 0x400) text_rotation = rd();
                if (opts & 0x8000) fileBuf->getRawShort16();
                DRW_DimOrdinate dim;
                setupBase(dim);
                dim.setOriginPoint(defPt);
                dim.setFirstLine(featurePoint);
                dim.setSecondLine(leaderEnd);
                dim.setDir(text_rotation);
                if (fileBuf->isGood()) intfa.addDimOrdinate(&dim);
                break; }
            default: {
                // Unknown dimtype -> render via the *D block. The *D block
                // was already delivered by readDwgBlocks; an INSERT at (0,0)
                // renders the graphics while preserving the old fallback.
                if (blockIdx < m_blockNames.size()
                    && !m_blockNames[blockIdx].empty()) {
                    DRW_Insert e;
                    e.name = m_blockNames[blockIdx];
                    e.basePoint = DRW_Coord(0.0, 0.0, 0.0);
                    applyAttrs(e);
                    if (fileBuf->isGood()) intfa.addInsert(e);
                }
                break; }
            }
            break; }
        case R11_SHAPE: {
            // Non-rendering in LibreCAD (kept as metadata), but read so it is not
            // a parse miss. style handle/index resolution is a follow-up.
            DRW_Shape e;
            e.m_insertionPoint = fileBuf->get2RawDouble();
            e.m_insertionPoint.z = elevation;
            e.m_scale = rd();
            // style_id is an RC (1 byte, dwg.spec:2338 FIELD_CAST(style_id,RC,
            // BS,0)) -> the SHAPEFILE glyph index. Reading it as a 2-byte RS
            // over-consumed one byte and desynced the following rotation RD.
            e.m_shapeIndex = fileBuf->getRawChar8();   // 1B RC, was RS (desync)
            if (opts & 0x01) e.m_rotation = rd();      // R11OPTS(1), dwg.spec:2340
            if (fileBuf->isGood()) intfa.addShape(e);
            break; }
        case R11_VIEWPORT: {
            DRW_Viewport e;
            e.basePoint = rd3();                 // center (3RD)
            e.pswidth = rd();                    // width
            e.psheight = rd();                   // height
            fileBuf->getRawShort16();            // viewport id (RS)
            applyAttrs(e);
            if (fileBuf->isGood()) intfa.addViewport(e);
            break; }
        case R11_REPEAT:
        case R11_ENDREP:
        case R11_LOAD:
            // Pre-R10 structural markers: REPEAT(5)/ENDREP(6) bracket a repeated
            // entity group, LOAD(10) is a shapefile load directive. They carry no
            // standalone geometry; skip (the recEnd advance consumes the body) so
            // they are not miscounted as parse failures. The bracketed entities
            // are still read once (repeat multiplicity is not expanded).
            break;
        case R11_JUMP:
            // Section-spanning continuation marker (pre-R13). It carries no
            // geometry and MUST NOT be counted as a parse failure or close an
            // open polyline accumulation: a POLYLINE's VERTEX run can be split
            // across the entities/blocks/extras sections by a JUMP, and the
            // vertices resume in the next section (with m_curPoly still open).
            // The authoritative advance to recEnd below skips its body.
            break;
        default:
            // Unhandled type -> skipped for now; counted as a parse "miss" but
            // not a failure (the recEnd advance below keeps the walk aligned).
            ++m_entityParseFailures;
            break;
        }
    }

    // Restore the shared cursor after the bounded body parse.  It already
    // advanced over the record body while filling recordBytes; setPosition is
    // retained as an explicit invariant for file-backed streams.
    const bool recordGood = fileBuf->isGood();
    fileBuf = std::move(sourceBuffer);
    if (!fileBuf->setPosition(recEnd))
        return false;
    return recordGood;
}
