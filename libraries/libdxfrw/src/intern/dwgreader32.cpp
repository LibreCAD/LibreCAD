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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "drw_dbg.h"
#include "dwgreader32.h"
#include "drw_textcodec.h"
#include "../libdwgr.h"


bool dwgReader32::readFileHeader() {
    DRW_DBG("dwgReader32::readFileHeader\n");
    bool ret = dwgReader27::readFileHeader();
    DRW_DBG("dwgReader32::readFileHeader END\n");
    return ret;
}

bool dwgReader32::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("dwgReader32::readDwgHeader\n");
    bool ret = dwgReader27::readDwgHeader(hdr);
    DRW_DBG("dwgReader32::readDwgHeader END\n");
    return ret;
}

bool dwgReader32::readDwgClasses(){
    DRW_DBG("\ndwgReader32::readDwgClasses");
    bool ret = dwgReader27::readDwgClasses();
    DRW_DBG("\ndwgReader32::readDwgClasses END\n");
    return ret;
}
