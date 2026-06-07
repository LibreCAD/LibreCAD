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

#ifndef DWGWRITER27_H
#define DWGWRITER27_H

#include "dwgwriter24.h"

/// R2013 (AC1027) concrete DWG writer.
///
/// AC1027 uses the identical R2004 page-based file container as AC1024.
/// The only binary differences vs AC1024 are:
///   - File header version string "AC1027" instead of "AC1024"
///   - Header data section adds REQUIREDVERSIONS (BLL) before the 4 unknown BDs
///   - Header handle section adds one extra soft-owner handle after DICT VISUALSTYLE
///   - Entity common preamble adds has_ds_data (B) after xDictFlag -- NOT a
///     restored haveNextLinks; per libreDWG common_entity_data.spec the bit
///     gates an inline ACIS SAB datastore (always 0 in libdxfrw)
///   - Table record preamble adds "Have binary data" (B) after xDictFlag
///   - SPLINE entity adds splFlag1 (BL) + knotParam (BL) after scenario
///
/// All section structure, encryption, and object encoding are inherited from
/// dwgWriter24.  No method overrides are needed.
class dwgWriter27 : public dwgWriter24 {
public:
    dwgWriter27(std::ofstream *stream, DRW_Header *header)
        : dwgWriter24(stream, header)
    {
        m_version = DRW::AC1027;
    }

protected:
    const char* fileHeaderVersion() const override { return "AC1027"; }
};

#endif // DWGWRITER27_H
