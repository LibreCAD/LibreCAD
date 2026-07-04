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

#include <memory>

#include "drw_textcodec.h"
#include "dwgreader.h"

//! Class to read pre-R13 (R10/R11) DWG files
/*!
*  Reads the fixed pre-R13 container: file header section pointers, then the
*  flat entity records in the ENTITIES section. Each record advances by its
*  own size field, so unhandled entity types are skipped safely.
*  @author Claude
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
    bool readDwgObjects(DRW_Interface& /*intfa*/) override { return true; } // no OBJECTS section

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
    bool readEntityR11(DRW_Interface& intfa);

    // POLYLINE..VERTEX..SEQEND accumulate across consecutive records; the
    // POLYLINE opens this, each VERTEX appends, SEQEND delivers + clears it.
    std::unique_ptr<DRW_Polyline> m_curPoly;

    // Table-record names by record index (filled in readDwgTables). An entity's
    // layer / an INSERT's block are stored as 0-based RS indices into these.
    // Index 0 == "0" (the default layer), verified vs dwgread.
    std::vector<std::string> m_layerNames;
    std::vector<std::string> m_blockNames;
    std::vector<std::string> m_ltypeNames;
    std::vector<std::string> m_styleNames;
    bool readNameTable(std::uint32_t hdrPos, std::vector<std::string>& out);
    std::string layerName(std::uint16_t idx) const;
    std::string ltypeName(std::int16_t idx) const;  // signed: -1/sentinels -> ""

    // Full per-record table decoders (R11/AC1009 only — R10 uses a different
    // per-table layout: no `used` field, fields at off33 not off35). They
    // heap-allocate DRW_LType/DRW_Layer/DRW_Textstyle and insert into the base
    // ltypemap/layermap/stylemap (the base dtor's mapCleanUp deletes them).
    bool readLTypeTable(std::uint32_t hdrPos);
    bool readLayerTable(std::uint32_t hdrPos);
    bool readStyleTable(std::uint32_t hdrPos);
};

#endif // DWGREADERR11_H
