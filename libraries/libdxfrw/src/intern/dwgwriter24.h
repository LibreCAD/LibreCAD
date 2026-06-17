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

#ifndef DWGWRITER24_H
#define DWGWRITER24_H

#include "dwgwriter18.h"

/// R2010 (AC1024) concrete DWG writer.
///
/// Inherits all section structure from dwgWriter18 (R2004 page-based container).
/// Differences from AC1018:
///   - Object bodies use the R2010 three-stream format: numeric DATA + string
///     section + handle section (each byte-aligned); wire format is
///     [MS totalBodyBytes][UMC handleBits][dataSectionBytes][handleSectionBytes][RS CRC16]
///   - CLASSES section adds an RL bitSize field after RL size, and an extra
///     RS "unknown CRC" before the end sentinel.
///   - Entity common data adds materialFlag (BB) + shadowFlag (RC) for
///     version > AC1018, and three visual-style B flags for version > AC1021.
///   - Object type encoding uses the R2010 two-bit OT prefix instead of BS.
///   - String fields (TV) are encoded as UTF-16LE (TU form) for R2007+.
class dwgWriter24 : public dwgWriter18 {
public:
    dwgWriter24(std::ofstream *stream, DRW_Header *header)
        : dwgWriter18(stream, header)
    {
        m_version = DRW::AC1024;
    }

    bool writeDwgHeader() override;

    bool writeDwgClasses() override;

    bool encodeEntity(DRW_Entity *ent) override;

protected:
    void finishObject() override;

    const char* fileHeaderVersion() const override { return "AC1024"; }
};

#endif // DWGWRITER24_H
