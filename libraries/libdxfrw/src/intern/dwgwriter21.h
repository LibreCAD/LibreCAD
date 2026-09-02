/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**  Copyright (C) 2026 Dongxu Li (github.com/dxli)                            **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 or later.                                               **
**                                                                           **
**  This library is distributed in the hope that it will be useful,          **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of           **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       **
**  General Public License for more details.                                  **
**                                                                           **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGWRITER21_H
#define DWGWRITER21_H

#include "dwgwriter24.h"

/// R2007 (AC1021) DWG writer.
///
/// AC1021 uses the RS-backed R2007 container, which is distinct from the
/// R2004 container inherited by dwgWriter24. Section pages use per-page LZ
/// compression when it reduces the payload, with RS(255,239)/(255,251)
/// framing for the file, system, and data pages.
class dwgWriter21 final : public dwgWriter24 {
public:
    dwgWriter21(std::ofstream *stream, DRW_Header *header)
        : dwgWriter24(stream, header) {
        m_version = DRW::AC1021;
        configureTextCodec();
    }

    bool finalize() override;

protected:
    void finishObject() override;

    const char* fileHeaderVersion() const override { return "AC1021"; }
};

#endif // DWGWRITER21_H
