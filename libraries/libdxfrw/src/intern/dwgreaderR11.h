/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Pre-R13 (R10/R11, AC1006/AC1009) DWG reader.                             **
**                                                                           **
**  The pre-R13 container is a different format from R13+: no CLASSES        **
**  section, no handle stream, no bit-packed codes -- a fixed file header     **
**  with section pointers, fixed-layout table sections, and a flat sequence  **
**  of entity records read as RAW little-endian values. Algorithm ported     **
**  from LibreDWG (decode_r11.c + specs); validated against dwgread on the    **
**  AC1006 (R10) and AC1009 (R11) corpora. Scope: read-only.                  **
******************************************************************************/
#ifndef DWGREADERR11_H
#define DWGREADERR11_H

#include <cstdint>
#include <memory>

#include "drw_textcodec.h"
#include "dwgreader.h"

//! Resolve a pre-R13 $DWGCODEPAGE id to a DRW_TextCodec setCodePage() name.
/*!
*  @param numHeaderVars  numheader_vars gate (file offset 0x11); <=129 means no
*                        codepage field is present -> returns nullptr.
*  @param cp             the codepage id read at file offset 0x3f9.
*  @return the ANSI_* name for setCodePage(), or nullptr to keep the default
*          (undefined id 0/0xff, or a codepage with no libdxfrw ConvTable).
*/
const char* preR13CodePageName(std::uint16_t numHeaderVars, std::uint16_t cp);

/// Read a pre-R13 fixed-width, NUL-padded text field. Always consumes @p width
/// bytes so the sequential record layout stays aligned, keeps only the bytes
/// before the first NUL, and decodes them with @p codec ($DWGCODEPAGE). Table
/// records store names as raw codepage bytes, so without this they reach the
/// document undecoded. Declared here so it can be unit tested.
std::string preR13FixedText(dwgBuffer& buf, int width, DRW_TextCodec& codec);

/// Decode a pre-R13 section-size field (blocks_size at file offset 0x20,
/// extras_size at 0x28). AutoCAD keeps two flags in the top bits - 0x40000000
/// on a section that is present, 0x80000000 on one that is absent - and the
/// size itself is the low 30 bits, so clear exactly those two.
///
/// Masking any narrower silently truncates a section larger than the mask
/// rather than rejecting it: a 24-bit mask turned the 22.5 MB blocks section
/// of a real R11 drawing (blocks_size 0x41585E43) into a 5.6 MB one, and the
/// read then failed on the record that straddled the false end. Declared here
/// so it can be unit tested.
constexpr std::uint32_t preR13SectionSize(std::uint32_t raw) {
    return raw & 0x3FFFFFFFu;
}

/// Smallest STYLE record a pre-R13 file of this version may declare.
/// R11 adds a 2-byte `used` field to R10's 194; R2.10 and older predate big
/// fonts and stop after font_file, 64 bytes short of R10. A reader that
/// demands R10's width rejects those older files outright.
constexpr std::uint16_t preR13StyleRecordMinSize(DRW::Version version) {
    if (version == DRW::AC1009)
        return 196;
    return version > DRW::AC210 ? 194 : 130;
}

/// Upper bound on how many records a pre-R13 entity section can hold.
/// Each record advances by its own size field, which the walker rejects
/// below 5 bytes. Deriving the bound from the section rather than fixing it
/// at a constant matters: a constant that a large drawing exceeds does not
/// degrade, it fails the whole file.
constexpr std::uint64_t preR13MaxRecordCount(std::uint32_t start,
                                             std::uint32_t end) {
    return end <= start ? 0 : (static_cast<std::uint64_t>(end - start) / 5 + 1);
}

/// Which fields a pre-R13 VERTEX record carries, decoded from its `opts`
/// word (libredwg dwg.h VERTEX_PFACE_FACE). The members are listed in the
/// order they appear in the record body.
///
/// HAS_NOT_X_Y marks a polyface FACE record: it carries up to four vertex
/// indices INSTEAD of a point. Reading a point there runs past the end of
/// the record and takes the whole section down with it.
struct PreR13VertexLayout {
    bool hasPoint;        ///< 2RD, unless HAS_NOT_X_Y (0x4000)
    bool hasStartWidth;   ///< RD, 0x01
    bool hasEndWidth;     ///< RD, 0x02
    bool hasBulge;        ///< RD, 0x04
    bool hasFlag;         ///< RC, 0x08
    bool hasTangent;      ///< RD, 0x10 - point records only
    bool hasIndex1;       ///< RSd, 0x20  - face records only
    bool hasIndex2;       ///< RSd, 0x40  - face records only
    bool hasIndex3;       ///< RSd, 0x80  - face records only
    bool hasIndex4;       ///< RSd, 0x100 - face records only
};

constexpr PreR13VertexLayout preR13VertexLayout(std::uint16_t opts) {
    const bool isFace = (opts & 0x4000) != 0;
    return PreR13VertexLayout{
        !isFace,
        !isFace && (opts & 0x0001) != 0,
        !isFace && (opts & 0x0002) != 0,
        !isFace && (opts & 0x0004) != 0,
        (opts & 0x0008) != 0,
        !isFace && (opts & 0x0010) != 0,
        isFace && (opts & 0x0020) != 0,
        isFace && (opts & 0x0040) != 0,
        isFace && (opts & 0x0080) != 0,
        isFace && (opts & 0x0100) != 0,
    };
}

//! Class to read pre-R13 (R10/R11) DWG files
/*!
*  Reads the fixed pre-R13 container: file header section pointers, then the
*  flat entity records in the ENTITIES section. Each record advances by its
*  own size field, so unhandled entity types are skipped safely.
*  @author libdxfrw
*/
class dwgReaderR11 : public dwgReader {
public:
    dwgReaderR11(std::unique_ptr<dwgBuffer> buffer, dwgRW *p)
        : dwgReader(std::move(buffer), p) {}
    virtual ~dwgReaderR11() {}

    bool readMetaData() override;
    bool readFileHeader() override;
    bool readDwgHeader(DRW_Header& hdr) override;
    bool readDwgClasses() override { return true; }   // pre-R13 has no CLASSES section
    bool readDwgHandles() override { return true; }    // pre-R13 has no handle stream
    bool readDwgTables(DRW_Header& hdr) override;
    bool readDwgBlocks(DRW_Interface& intfa) override;
    bool readDwgEntities(DRW_Interface& intfa) override;
    bool readDwgObjects(DRW_Interface& intfa) override {
        // Legacy files have no OBJECTS section, but readDwgTables may queue
        // the VPORT_ENTITY_HEADER control for raw same-version replay.
        return publishDeferredRawObjects(intfa, m_deferredRawObjects);
    }

private:
    // Section pointers from the file header (absolute file offsets).
    std::uint32_t m_entitiesStart = 0;
    std::uint32_t m_entitiesEnd = 0;
    std::uint32_t m_blocksStart = 0;
    std::uint32_t m_blocksEnd = 0;
    std::uint32_t m_extrasStart = 0;
    std::uint32_t m_extrasEnd = 0;

    // Walk a flat entity sequence in [start,end) delivering each entity.
    bool readEntitySection(std::uint32_t start, std::uint32_t end, DRW_Interface& intfa);
    // Decode one entity at the current buffer position; always leaves the
    // buffer at recStart+size. Returns false only on an unrecoverable desync.
    bool readEntityR11(DRW_Interface& intfa, std::uint32_t sectionEnd);

    // POLYLINE..VERTEX..SEQEND accumulate across consecutive records; the
    // POLYLINE opens this, each VERTEX appends, SEQEND delivers + clears it.
    std::unique_ptr<DRW_Polyline> m_curPoly;
    // R10/R11 BLOCK and ENDBLK records delimit one flat callback scope. Keep
    // the state explicit so malformed or stray terminators cannot corrupt the
    // next section's destination container.
    bool m_blockOpen = false;

    // Table-record names by record index (filled in readDwgTables). An entity's
    // layer / an INSERT's block are stored as 0-based RS indices into these.
    // Index 0 == "0" (the default layer), verified vs dwgread.
    std::vector<std::string> m_layerNames;
    std::vector<std::string> m_blockNames;
    std::vector<std::string> m_ltypeNames;
    std::vector<std::string> m_styleNames;
    bool readNameTable(std::uint32_t hdrPos, std::vector<std::string>& out);
    bool readPreR13String(std::string& out);
    std::string layerName(std::uint16_t idx) const;
    std::string ltypeName(std::int16_t idx) const;  // signed: -1/sentinels -> ""

    // Full per-record table decoders (R11/AC1009 only — R10 uses a different
    // per-table layout: no `used` field, fields at off33 not off35). They
    // heap-allocate DRW_LType/DRW_Layer/DRW_Textstyle and insert into the base
    // ltypemap/layermap/stylemap (the base dtor's mapCleanUp deletes them).
    bool readLTypeTable(std::uint32_t hdrPos);
    bool readLayerTable(std::uint32_t hdrPos);
    bool readStyleTable(std::uint32_t hdrPos);
    // Name-only reader for the EMBEDDED extended tables (APPID/DIMSTYLE). The
    // 10-byte descriptor @hdrPos matches the @0x2C layout; records carry
    // flag(RC)+name(32). isDimstyle selects dimstylemap (else appIdmap).
    bool readExtendedNameTable(std::uint32_t hdrPos, bool isDimstyle);
};

#endif // DWGREADERR11_H
