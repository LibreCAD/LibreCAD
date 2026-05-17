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

#ifndef DWGWRITER18_H
#define DWGWRITER18_H

#include "dwgwriter15.h"

/// R2004 (AC1018) concrete DWG writer.
///
/// Inherits all entity/table/block encoding from dwgWriter15.  All section
/// content is accumulated into m_buf using the inherited write methods.
/// finalize() then re-packages those sections into the R2004 page-based
/// container format (Section Page Map + Data Section Map + individual data
/// pages) and writes the 0x100-byte encrypted file header.
///
/// Only uncompressed (store, compression_type=1) pages are written —
/// avoids implementing an LZ compressor.  Files are readable by AutoCAD
/// and by libdxfrw's own dwgReader18.
class dwgWriter18 : public dwgWriter15 {
public:
    dwgWriter18(std::ofstream *stream, DRW_Header *header)
        : dwgWriter15(stream, header)
    {
        m_version = DRW::AC1018;
    }

    /// No-op: R2004 has no R2000-style linear file-header stub.
    /// The 0x100-byte file header is written entirely in finalize().
    bool writeFileHeaderStub() override { return true; }

    /// No-op: R2004 does not use a second-header block.
    bool writeSecondHeader() override { return true; }

    /// R2004 CLASSES section format differs from R2000:
    ///   RL size, BS maxClassNum(499), RC, RC, Bit, padding, CRC, END sentinel.
    bool writeDwgClasses() override;

    /// For R2004, HANDLES offsets are section-relative (into the OBJECTS
    /// data page).  Return the byte offset where OBJECTS content starts in
    /// m_buf so writeDwgHandles can subtract it before encoding deltas.
    duint32 objectBaseOffset() const override;


    /// Re-packages m_buf sections into R2004 container pages,
    /// builds the encrypted variable header, and flushes to disk.
    bool finalize() override;
};

#endif // DWGWRITER18_H
