/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2022 Michał Grzybowski, michal@grzybowscy.org              **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGREADER32_H
#define DWGREADER32_H

//#include "drw_textcodec.h"
//#include "dwgbuffer.h"
#include "dwgreader27.h"

class dwgReader32 : public dwgReader27 {
public:
    dwgReader32(std::ifstream *stream, dwgR *p):dwgReader27(stream, p){ }
    bool readFileHeader() override;
    bool readDwgHeader(DRW_Header& hdr) override;
    bool readDwgClasses() override;
};

#endif // DWGREADER32_H
