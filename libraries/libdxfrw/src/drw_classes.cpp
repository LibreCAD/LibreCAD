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

#include "drw_classes.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_dbg.h"


bool DRW_Class::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf){
    DRW_DBG("\n***************************** parsing Class *********************************************\n");

    classNum = buf->getBitShort();
    DRW_DBG("Class number: "); DRW_DBG(classNum);
    proxyFlag = buf->getBitShort(); //in dwg specs says "version"

    appName = strBuf->getVariableText(version, false);
    className = strBuf->getVariableText(version, false);
    recName = strBuf->getVariableText(version, false);

    DRW_DBG("\napp name: "); DRW_DBG(appName.c_str());
    DRW_DBG("\nclass name: "); DRW_DBG(className.c_str());
    DRW_DBG("\ndxf rec name: "); DRW_DBG(recName.c_str());
    wasaProxyFlag = buf->getBit(); //in dwg says wasazombie
    entityFlag = buf->getBitShort();
    entityFlag = entityFlag == 0x1F2 ? 1: 0;

    DRW_DBG("\nProxy capabilities flag: "); DRW_DBG(proxyFlag);
    DRW_DBG(", proxy flag (280): "); DRW_DBG(wasaProxyFlag);
    DRW_DBG(", entity flag: "); DRW_DBGH(entityFlag);

    if (version > DRW::AC1015) {//2004+
        // ODA + libreDWG read_2004_section_classes (decode.c:2367-2371):
        //   num_instances BL, dwg_version BS, maint_version BS, unknown_1 BL,
        //   unknown_2 BL. (libreDWG's BL variant for dwg/maint is #if 0 dead
        //   code.) BS and BL share encodings for values < 256, so this only
        //   matters once a class trailer carries a >= 256 dwg/maint version.
        instanceCount = buf->getBitLong();
        DRW_DBG("\nInstance Count: "); DRW_DBG(instanceCount);
        std::uint32_t dwgVersion = buf->getBitShort();
        DRW_DBG("\nDWG version: "); DRW_DBG(dwgVersion);
        DRW_DBG("\nmaintenance version: "); DRW_DBG(buf->getBitShort());
        DRW_DBG("\nunknown 1: "); DRW_DBG(buf->getBitLong());
        DRW_DBG("\nunknown 2: "); DRW_DBG(buf->getBitLong());
    }
    DRW_DBG("\n");
    toDwgType();
    return buf->isGood();
}

void DRW_Class::write(dxfWriter *writer, DRW::Version ver){
    if (ver > DRW::AC1009) {
        writer->writeString(0, "CLASS");
        writer->writeString(1, recName);
        writer->writeString(2, className);
        writer->writeString(3, appName);
        writer->writeInt32(90, proxyFlag);
        if (ver > DRW::AC1015) { //2004+
            writer->writeInt32(91, instanceCount);
        }
        writer->writeInt16(280, wasaProxyFlag);
        writer->writeInt16(281, entityFlag);
    }
}

void DRW_Class::toDwgType(){
    if (recName == "LWPOLYLINE")
        dwgType = 77;
    else if (recName == "HATCH")
        dwgType = 78;
    else if (recName == "GROUP")
        dwgType = 72;
/*    else if (recName == "GROUP")
        dwgType = 72;*/
    else if (recName == "LAYOUT")
        dwgType = 82;
    else if (recName == "IMAGE")
        dwgType = 101;
    else if (recName == "IMAGEDEF")
        dwgType = 102;
    else
        dwgType = 0;
    // NOTE: ARC_DIMENSION must NOT be added here. It has no fixed DWG type < 500.
    // Its classNum (>= 500) is assigned by the DWG class table at read time and by
    // writeDwgClasses() at write time. Dispatch goes via classesmap recName lookup
    // in dwgreader.cpp default: block. Adding dwgType=27 (or any < 500) would silently
    // redirect ARC_DIMENSION entities to the wrong parser (POINT is case 27).
}
