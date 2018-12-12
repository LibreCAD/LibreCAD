/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGREADER27_H
#define DWGREADER27_H

#include <map>
#include <list>
#include "drw_textcodec.h"
#include "dwgbuffer.h"
#include "dwgreader18.h"

class dwgReader27 : public dwgReader18 {
public:
    dwgReader27(std::ifstream *stream, dwgR *p):dwgReader18(stream, p){ }
	bool readFileHeader() override;
	bool readDwgHeader(DRW_Header& hdr) override;
	bool readDwgClasses() override;
//    bool readDwgHandles(){return false;}
//    bool readDwgTables(){return false;}
	bool readDwgBlocks(DRW_Interface& intfa) override{
        bool ret = true;
		dwgBuffer dataBuf(objData.data(), uncompSize, &decoder);
        ret = dwgReader::readDwgBlocks(intfa, &dataBuf);
        return ret;
    }
	virtual bool readDwgEntities(DRW_Interface& intfa) override{
        bool ret = true;
		dwgBuffer dataBuf(objData.data(), uncompSize, &decoder);
        ret = dwgReader::readDwgEntities(intfa, &dataBuf);
        return ret;
    }
	virtual bool readDwgObjects(DRW_Interface& intfa) override{
        bool ret = true;
		dwgBuffer dataBuf(objData.data(), uncompSize, &decoder);
        ret = dwgReader::readDwgObjects(intfa, &dataBuf);
        return ret;
    }
//    bool readDwgEntity(objHandle& obj, DRW_Interface& intfa){
//        DRW_UNUSED(obj);
//        DRW_UNUSED(intfa);
//        return false;}
};

#endif // DWGREADER21_H
