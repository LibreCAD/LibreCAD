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

#ifndef DWGWRITER32_H
#define DWGWRITER32_H

#include "dwgwriter27.h"

/// R2018 (AC1032) concrete DWG writer.
///
/// ODA chapter 8 describes the R2018 organization as structurally identical
/// to R2013. The writer therefore reuses the AC1027 page container/object
/// framing and relies on version-gated entity/header encoders for payload
/// deltas such as MTEXT and MLINESTYLE.
class dwgWriter32 : public dwgWriter27 {
public:
    dwgWriter32(std::ofstream *stream, DRW_Header *header)
        : dwgWriter27(stream, header)
    {
        m_version = DRW::AC1032;
    }

protected:
    const char* fileHeaderVersion() const override { return "AC1032"; }
};

#endif // DWGWRITER32_H
