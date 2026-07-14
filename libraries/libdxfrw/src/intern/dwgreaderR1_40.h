/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**  Dedicated pre-R2.0b DWG reader (R1.40 / magic "AC1.40").                  **
**                                                                           **
**  Unlike R2.6..R11 (dwgReaderR11), the R1.40 container has NO @0x14 section **
**  pointers, NO 10-byte @0x2C table headers, NO per-record size field, NO    **
**  per-record CRC, and NO entity common prefix. The entity stream runs       **
**  contiguously from 0x202 to dwg_size (u32 @0x24); each record is           **
**  type(RS) + layer(RS) + a type-specific body with a fixed or TV-terminated **
**  length, so the walker must decode each body exactly to find the next.     **
**  Mirrors dwgTs's DwgReaderR1_40 + libredwg decode_r1_4.                     **
******************************************************************************/

#ifndef DWGREADERR1_40_H
#define DWGREADERR1_40_H

#include <cstdint>
#include <memory>
#include <string>

#include "dwgreader.h"

class dwgReaderR1_40 : public dwgReader {
public:
    dwgReaderR1_40(std::unique_ptr<dwgBuffer> buffer, dwgRW *p)
        : dwgReader(std::move(buffer), p) {}
    ~dwgReaderR1_40() override {}

    bool readMetaData() override;
    bool readFileHeader() override;
    bool readDwgHeader(DRW_Header& hdr) override;
    bool readDwgClasses() override { return true; }   // pre-R2.0b: no CLASSES section
    bool readDwgHandles() override { return true; }   // pre-R2.0b: no handle stream
    bool readDwgTables(DRW_Header& hdr) override;
    bool readDwgBlocks(DRW_Interface& intfa) override;   // blocks are inline in the entity stream
    bool readDwgEntities(DRW_Interface& intfa) override;
    bool readDwgObjects(DRW_Interface& /*intfa*/) override { return true; }  // no OBJECTS section

private:
    std::uint32_t m_dwgSize = 0;   // end of the entity stream (u32 @0x24)

    // Decode one entity at *cursor, deliver it via intfa, advance *cursor past
    // the body. Returns false to stop the walk (unknown type / out of bounds).
    bool readEntity(DRW_Interface& intfa, std::uint32_t& cursor);
    // FIELD_TV (PRE R_2_0b): 2-byte RS length + N raw bytes, no NUL. Advances
    // the buffer; clamps at `end`.
    std::string readTV(std::uint32_t end);
};

#endif // DWGREADERR1_40_H
