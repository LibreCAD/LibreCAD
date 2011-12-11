/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/


#include "libdxfrw.h"
#include <fstream>
#include <algorithm>
#include "dxfreader.h"
#include "dxfwriter.h"


using namespace std;

#ifdef DRW_DBG2
#include <iostream> //for debug
#define DBG(a) std::cerr << a
#else
#define DBG(a)
#endif

#define FIRSTHANDLE 48

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dxfRW::dxfRW(const char* name){
    fileName = name;
    reader = NULL;
    writer = NULL;
}
dxfRW::~dxfRW(){
    if (reader != NULL)
        delete reader;

}

bool dxfRW::read(DRW_Interface *interface){
    bool isOk = false;
    ifstream filestr;
    DBG("dxfRW::read 1def\n");
    filestr.open (fileName.c_str(), ios_base::in | ios::binary);
    if (!filestr.is_open())
        return isOk;
    if (!filestr.good())
        return isOk;

    char line[22];
    char line2[22] = "AutoCAD Binary DXF\r\n";
    line2[20] = (char)26;
    line2[21] = '\0';
    filestr.read (line, 22);
    filestr.close();
    iface = interface;
    DBG("dxfRW::read 2\n");
    if (strcmp(line, line2) == 0) {
        filestr.open (fileName.c_str(), ios_base::in | ios::binary);
        binary = true;
        //skip sentinel
        filestr.seekg (22, ios::beg);
        reader = new dxfReaderBinary(&filestr);
        DBG("dxfRW::read binary file\n");
    } else {
        binary = false;
        filestr.open (fileName.c_str(), ios_base::in);
        reader = new dxfReaderAscii(&filestr);
    }

    isOk = processDxf();
    filestr.close();
    delete reader;
    reader = NULL;
    return isOk;
}

bool dxfRW::write(DRW_Interface *interface, DRW::Version ver, bool bin){
    bool isOk = false;
    ofstream filestr;
    version = ver;
//TODO allow to write more versions than 2000
    version = DRW::AC1015;
    binary = bin;
    iface = interface;
    if (binary) {
        filestr.open (fileName.c_str(), ios_base::out | ios::binary | ios::trunc);
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
        writer = new dxfWriterBinary(&filestr);
        DBG("dxfRW::read binary file\n");
    } else {
        filestr.open (fileName.c_str(), ios_base::out | ios::trunc);
        writer = new dxfWriterAscii(&filestr);
        std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
        writer->writeString(999, comm);
    }
    iface->writeHeader();
    writer->writeString(0, "SECTION");
    writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER");
    switch (version) {
    case DRW::AC1009:
        writer->writeString(1, "AC1009");
        break;
    case DRW::AC1012:
        writer->writeString(1, "AC1012");
        break;
    case DRW::AC1014:
        writer->writeString(1, "AC1014");
        break;
//    case DRW::AC1015:
//acad2000 default version
    default:
        writer->writeString(1, "AC1015");
        break;
    }

    if (version> DRW::AC1014) {
        writer->writeString(9, "$HANDSEED");
//RLZ        dxfHex(5, 0xFFFF);
        writer->writeString(5, "20000");
    }

    entCount =FIRSTHANDLE;
    writer->writeString(9, "$DWGCODEPAGE");
    writer->writeString(3, "ANSI_1252");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "CLASSES");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writeTables();
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "BLOCKS");
    writeBlocks();
    writer->writeString(0, "ENDSEC");

    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    iface->writeEntities();
    writer->writeString(0, "ENDSEC");

    writer->writeString(0, "SECTION");
    writer->writeString(2, "OBJECTS");
    writeObjects();
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    filestr.flush();
    filestr.close();
    isOk = true;
    delete writer;
    writer = NULL;
    return isOk;
}

bool dxfRW::writeEntity(DRW_Entity *ent) {
    char buffer[5];
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, buffer);
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, ent->layer);
    writer->writeString(6, ent->lineType);
    writer->writeInt16(62, ent->color);
    writer->writeInt16(370, ent->lWeight);
    return true;
}

bool dxfRW::writeLineType(DRW_LType *ent){
    char buffer[5];
    string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
//do not write linetypes handled by library
    if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") {
        return true;
    }
    writer->writeString(0, "LTYPE");
    ++entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, buffer);

    writer->writeString(330, "5");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLinetypeTableRecord");
    writer->writeString(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeString(3, ent->desc);
    ent->update();
    writer->writeInt16(72, 65);
    writer->writeInt16(73, ent->size);
    writer->writeDouble(40, ent->length);

    for (unsigned int i = 0;  i< ent->path.size(); i++){
        writer->writeDouble(49, ent->path.at(i));
        writer->writeInt16(74, 0);
    }
    return true;
}

bool dxfRW::writeLayer(DRW_Layer *ent){
    char buffer[5];
    writer->writeString(0, "LAYER");
    if (!wlayer0 && ent->name == "0") {
            wlayer0 = true;
            writer->writeString(5, "10");
    } else {
        ++entCount;
        sprintf(buffer, "%X", entCount);
        writer->writeString(5, buffer);
    }
    writer->writeString(330, "2");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLayerTableRecord");
    writer->writeString(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeInt16(62, ent->color);
    writer->writeString(6, ent->lineType);
    writer->writeInt16(370, ent->lWeight);
    writer->writeString(390, "F");
//    writer->writeString(347, "10012");
    return true;
}

bool dxfRW::writePoint(DRW_Point *ent) {
    writer->writeString(0, "POINT");
    writeEntity(ent);
    writer->writeString(100, "AcDbPoint");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    if (ent->z != 0.0) {
        writer->writeDouble(30, ent->z);
    }
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent) {
    writer->writeString(0, "LINE");
    writeEntity(ent);
    writer->writeString(100, "AcDbLine");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    if (ent->z != 0.0 || ent->bz != 0.0) {
        writer->writeDouble(30, ent->z);
        writer->writeDouble(11, ent->bx);
        writer->writeDouble(21, ent->by);
        writer->writeDouble(31, ent->bz);
    } else {
        writer->writeDouble(11, ent->bx);
        writer->writeDouble(21, ent->by);
    }
    return true;
}

bool dxfRW::writeCircle(DRW_Circle *ent) {
    writer->writeString(0, "CIRCLE");
    writeEntity(ent);
    writer->writeString(100, "AcDbCircle");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    if (ent->z != 0.0) {
        writer->writeDouble(30, ent->z);
    }
    writer->writeDouble(40, ent->radious);
    return true;
}

bool dxfRW::writeArc(DRW_Arc *ent) {
    writer->writeString(0, "ARC");
    writeEntity(ent);
    writer->writeString(100, "AcDbCircle");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    if (ent->z != 0.0) {
        writer->writeDouble(30, ent->z);
    }
    writer->writeDouble(40, ent->radious);
    writer->writeString(100, "AcDbArc");
    writer->writeDouble(50, ent->staangle);
    writer->writeDouble(51, ent->endangle);
    return true;
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent){
    writer->writeString(0, "ELLIPSE");
    writeEntity(ent);
    writer->writeString(100, "AcDbEllipse");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    writer->writeDouble(30, ent->z);
    writer->writeDouble(11, ent->bx);
    writer->writeDouble(21, ent->by);
    writer->writeDouble(31, ent->bz);
    writer->writeDouble(40, ent->ratio);
    writer->writeDouble(41, ent->staparam);
    writer->writeDouble(42, ent->endparam);
    return true;
}

bool dxfRW::writeTrace(DRW_Trace *ent){
    writer->writeString(0, "TRACE");
    writeEntity(ent);
    writer->writeString(100, "AcDbTrace");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    writer->writeDouble(30, ent->z);
    writer->writeDouble(11, ent->bx);
    writer->writeDouble(21, ent->by);
    writer->writeDouble(31, ent->bz);
    writer->writeDouble(12, ent->cx);
    writer->writeDouble(22, ent->cy);
    writer->writeDouble(32, ent->cz);
    writer->writeDouble(13, ent->dx);
    writer->writeDouble(23, ent->dy);
    writer->writeDouble(33, ent->dz);
    return true;
}

bool dxfRW::writeSolid(DRW_Solid *ent){
    writer->writeString(0, "SOLID");
    writeEntity(ent);
    writer->writeString(100, "AcDbTrace");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    writer->writeDouble(30, ent->z);
    writer->writeDouble(11, ent->bx);
    writer->writeDouble(21, ent->by);
    writer->writeDouble(31, ent->bz);
    writer->writeDouble(12, ent->cx);
    writer->writeDouble(22, ent->cy);
    writer->writeDouble(32, ent->cz);
    writer->writeDouble(13, ent->dx);
    writer->writeDouble(23, ent->dy);
    writer->writeDouble(33, ent->dz);
    return true;
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent){
    writer->writeString(0, "LWPOLYLINE");
    writeEntity(ent);
    writer->writeString(100, "AcDbPolyline");
    ent->vertexnum = ent->vertlist.size();
    writer->writeInt32(90, ent->vertexnum);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(43, ent->width);
    for (int i = 0;  i< ent->vertexnum; i++){
        DRW_Vertex2D *v = ent->vertlist.at(i);
        writer->writeDouble(10, v->x);
        writer->writeDouble(20, v->y);
        if (v->stawidth != 0)
            writer->writeDouble(40, v->stawidth);
        if (v->endwidth != 0)
            writer->writeDouble(41, v->endwidth);
        if (v->bulge != 0)
            writer->writeDouble(42, v->bulge);
    }
    return true;
}

bool dxfRW::writeTables() {
    char buffer[5];
    writer->writeString(0, "TABLE");
    writer->writeString(2, "VPORT");
    writer->writeString(5, "8");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "VPORT");
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, buffer);
    writer->writeString(330, "8");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbViewportTableRecord");
    writer->writeString(2, "*Active");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(11, 1.0);
    writer->writeDouble(21, 1.0);
    writer->writeDouble(12, 0.651828);
    writer->writeDouble(22, -0.16);
    writer->writeDouble(13, 0.0);
    writer->writeDouble(23, 0.0);
    writer->writeDouble(14, 10.0);
    writer->writeDouble(24, 10.0);
    writer->writeDouble(15, 10.0);
    writer->writeDouble(25, 10.0);
    writer->writeDouble(16, 0.0);
    writer->writeDouble(26, 0.0);
    writer->writeDouble(36, 1.0);
    writer->writeDouble(17, 0.0);
    writer->writeDouble(27, 0.0);
    writer->writeDouble(37, 0.0);
    writer->writeDouble(40, 5.13732);
    writer->writeDouble(41, 2.4426877);
    writer->writeDouble(42, 50.0);
    writer->writeDouble(43, 0.0);
    writer->writeDouble(44, 0.0);
    writer->writeDouble(50, 0.0);
    writer->writeDouble(51, 0.0);
    writer->writeInt16(71, 0);
    writer->writeInt16(72, 100);
    writer->writeInt16(73, 1);
    writer->writeInt16(74, 3);
    writer->writeInt16(75, 0);
    writer->writeInt16(76, 1);
    writer->writeInt16(77, 0);
    writer->writeInt16(78, 0);
    writer->writeInt16(281, 0);
    writer->writeInt16(65, 1);
    writer->writeDouble(110, 0.0);
    writer->writeDouble(120, 0.0);
    writer->writeDouble(130, 0.0);
    writer->writeDouble(111, 1.0);
    writer->writeDouble(121, 0.0);
    writer->writeDouble(131, 0.0);
    writer->writeDouble(112, 0.0);
    writer->writeDouble(122, 1.0);
    writer->writeDouble(132, 0.0);
    writer->writeInt16(79, 0);
    writer->writeDouble(146, 0.0);
    writer->writeString(348, "10020");
    writer->writeInt16(60, 3);
    writer->writeInt16(61, 5);
    writer->writeBool(292, 1);
    writer->writeInt16(282, 1);
    writer->writeDouble(141, 0.0);
    writer->writeDouble(142, 0.0);
    writer->writeInt16(63, 250);
    writer->writeInt32(421, 3358443);
    writer->writeString(0, "ENDTAB");
/*** LTYPE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    writer->writeString(5, "5");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 4); //end table def
//Mandatory linetypes
    writer->writeString(0, "LTYPE");
    writer->writeString(5, "14");
    writer->writeString(330, "5");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLinetypeTableRecord");
    writer->writeString(2, "ByBlock");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    writer->writeString(5, "15");
    writer->writeString(330, "5");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLinetypeTableRecord");
    writer->writeString(2, "ByLayer");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    writer->writeString(5, "16");
    writer->writeString(330, "5");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLinetypeTableRecord");
    writer->writeString(2, "CONTINUOUS");
    writer->writeInt16(70, 0);
    writer->writeString(3, "Solid line");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);
//Aplication linetypes
    iface->writeLTypes();
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "LAYER");
    writer->writeString(5, "2");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 1); //end table def
    wlayer0 =false;
    iface->writeLayers();
    if (!wlayer0) {
        DRW_Layer lay0;
        lay0.name = "0";
        writeLayer(&lay0);
    }
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "STYLE");
    writer->writeString(5, "3");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    writer->writeString(5, "6");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    writer->writeString(5, "7");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    writer->writeString(5, "9");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    writer->writeString(5, "12");
    writer->writeString(330, "9");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbRegAppTableRecord");
    writer->writeString(2, "ACAD");
    writer->writeInt16(70, 0);
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    writer->writeString(5, "A");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 0); //end table def
    writer->writeString(100, "AcDbDimStyleTable");
    writer->writeInt16(71, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "BLOCK_RECORD");
    writer->writeString(5, "1");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 2); //end table def
    writer->writeString(0, "BLOCK_RECORD");
    writer->writeString(5, "1F");
    writer->writeString(330, "1");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbBlockTableRecord");
    writer->writeString(2, "*Model_Space");
//    writer->writeInt16(340, 22);
    writer->writeInt16(70, 0);
    writer->writeInt16(280, 1);
    writer->writeInt16(281, 0);
    writer->writeString(0, "BLOCK_RECORD");
    writer->writeString(5, "1E");
    writer->writeString(330, "1");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbBlockTableRecord");
    writer->writeString(2, "*Paper_Space");
//    writer->writeInt16(340, 22);
    writer->writeInt16(70, 0);
    writer->writeInt16(280, 1);
    writer->writeInt16(281, 0);
    writer->writeString(0, "ENDTAB");
return true;
}

bool dxfRW::writeBlocks() {
    writer->writeString(0, "BLOCK");
    writer->writeString(5, "20");
    writer->writeString(330, "1F");
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockBegin");
    writer->writeString(2, "*Model_Space");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    writer->writeString(3, "*Model_Space");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    writer->writeString(5, "21");
    writer->writeString(330, "1F");
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockEnd");

    writer->writeString(0, "BLOCK");
    writer->writeString(5, "1C");
    writer->writeString(330, "1B");
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockBegin");
    writer->writeString(2, "*Paper_Space");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    writer->writeString(3, "*Paper_Space");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    writer->writeString(5, "1D");
    writer->writeString(330, "1F");
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockEnd");
    return true;
}

bool dxfRW::writeObjects() {
    writer->writeString(0, "DICTIONARY");
    char buffer[5];
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, "C");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    writer->writeString(3, "ACAD_GROUP");
    writer->writeString(350, "D");
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "D");
    writer->writeString(330, "C");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    return true;
}

/********* Reader Process *********/

bool dxfRW::processDxf() {
    DBG("dxfRW::processDxf()\n");
    int code;
    bool more = true;
    string sectionstr;
//    section = secUnknown;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "EOF") {
                return true;  //found EOF terminate
            }
            if (sectionstr == "SECTION") {
                more = reader->readRec(&code, !binary);
                DBG(code); DBG("\n");
                if (!more)
                    return false; //wrong dxf file
                if (code == 2) {
                    sectionstr = reader->getString();
                    DBG(sectionstr); DBG("\n");
                //found section, process it
                    if (sectionstr == "HEADER") {
                        processHeader();
                    } else if (sectionstr == "CLASSES") {
//                        processClasses();
                    } else if (sectionstr == "TABLES") {
                        processTables();
                    } else if (sectionstr == "BLOCKS") {
                        processBlocks();
                    } else if (sectionstr == "ENTITIES") {
                        processEntities(false);
                    } else if (sectionstr == "OBJECTS") {
                        processObjects();
                    }
                }
            }
        }
/*    if (!more)
        return true;*/
    }
    return true;
}

/********* Header Section *********/

bool dxfRW::processHeader() {
    DBG("dxfRW::processHeader\n");
    int code;
    string sectionstr;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        } else header.parseCode(code, reader);
    }
    return true;
}

/********* Tables Section *********/

bool dxfRW::processTables() {
    DBG("dxfRW::processTables\n");
    int code;
    string sectionstr;
    bool more = true;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "TABLE") {
                more = reader->readRec(&code, !binary);
                DBG(code); DBG("\n");
                if (!more)
                    return false; //wrong dxf file
                if (code == 2) {
                    sectionstr = reader->getString();
                    DBG(sectionstr); DBG("\n");
                //found section, process it
                    if (sectionstr == "VPORT") {
//                        processVPort();
                    } else if (sectionstr == "LTYPE") {
                        processLType();
                    } else if (sectionstr == "LAYER") {
                        processLayer();
                    } else if (sectionstr == "STYLE") {
//                        processStyle();
                    } else if (sectionstr == "VIEW") {
//                        processView();
                    } else if (sectionstr == "UCS") {
//                        processUCS();
                    } else if (sectionstr == "APPID") {
//                        processAppId();
                    } else if (sectionstr == "DIMSTYLE") {
//                        processDimStyle();
                    } else if (sectionstr == "BLOCK_RECORD") {
//                        processBlockRecord();
                    }
                }
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }
    return true;
}

bool dxfRW::processLType() {
    DBG("dxfRW::processLType\n");
    int code;
    string sectionstr;
    bool reading = false;
    DRW_LType ltype;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            if (reading) {
                ltype.update();
                iface->addLType(ltype);
            }
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "LTYPE") {
                reading = true;
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            ltype.parseCode(code, reader);
    }
    return true;
}

bool dxfRW::processLayer() {
    DBG("dxfRW::processLayer\n");
    int code;
    string sectionstr;
    bool reading = false;
    DRW_Layer layer;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addLayer(layer);
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "LAYER") {
                reading = true;
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            layer.parseCode(code, reader);
    }
    return true;
}


/********* Block Section *********/

bool dxfRW::processBlocks() {
    DBG("dxfRW::processBlocks\n");
    int code;
    string sectionstr;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "BLOCK") {
                processBlock();
            } else if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }
    return true;
}

bool dxfRW::processBlock() {
    DBG("dxfRW::processBlock");
    int code;
    DRW_Block block;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addBlock(block);
            if (nextentity == "ENDBLK") {
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            } else {
                processEntities(true);
                iface->endBlock();
                return true;  //found ENDBLK, terminate
            }
        }
        default:
            block.parseCode(code, reader);
            break;
        }
    }
    return true;
}


/********* Entities Section *********/

bool dxfRW::processEntities(bool isblock) {
    DBG("dxfRW::processEntities\n");
    int code;
    if (!reader->readRec(&code, !binary)){
        return false;
    }
    bool next = true;
    if (code == 0) {
            nextentity = reader->getString();
    } else if (!isblock) {
            return false;  //first record in entities is 0
   }
    do {
        if (nextentity == "ENDSEC" || nextentity == "ENDBLK") {
            return true;  //found ENDSEC or ENDBLK terminate
        } else if (nextentity == "POINT") {
            processPoint();
        } else if (nextentity == "LINE") {
            processLine();
        } else if (nextentity == "CIRCLE") {
            processCircle();
        } else if (nextentity == "ARC") {
            processArc();
        } else if (nextentity == "ELLIPSE") {
            processEllipse();
        } else if (nextentity == "TRACE") {
            processTrace();
        } else if (nextentity == "SOLID") {
            processSolid();
        } else if (nextentity == "INSERT") {
            processInsert();
        } else if (nextentity == "LWPOLYLINE") {
            processLWPolyline();
        } else if (nextentity == "POLYLINE") {
            processPolyline();
        } else if (nextentity == "TEXT") {
            processText();
        } else if (nextentity == "HATCH") {
            processHatch();
        } else if (nextentity == "SPLINE") {
            processSpline();
        } else {
            if (reader->readRec(&code, !binary)){
                if (code == 0)
                    nextentity = reader->getString();
            } else
                return false; //end of file without ENDSEC
        }

    } while (next);
    return true;
}

bool dxfRW::processEllipse() {
    DBG("dxfRW::processEllipse");
    int code;
    DRW_Ellipse ellipse;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addEllipse(ellipse);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            ellipse.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processTrace() {
    DBG("dxfRW::processTrace");
    int code;
    DRW_Trace trace;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addTrace(trace);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            trace.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processSolid() {
    DBG("dxfRW::processSolid");
    int code;
    DRW_Solid solid;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addSolid(solid);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            solid.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processPoint() {
    DBG("dxfRW::processPoint\n");
    int code;
    DRW_Point point;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addPoint(point);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            point.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processLine() {
    DBG("dxfRW::processLine\n");
    int code;
    DRW_Line line;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addLine(line);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            line.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processCircle() {
    DBG("dxfRW::processPoint\n");
    int code;
    DRW_Circle circle;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addCircle(circle);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            circle.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processArc() {
    DBG("dxfRW::processPoint\n");
    int code;
    DRW_Arc arc;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addArc(arc);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            arc.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processInsert() {
    DBG("dxfRW::processInsert");
    int code;
    DRW_Insert insert;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addInsert(insert);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            insert.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processLWPolyline() {
    DBG("dxfRW::processLWPolyline");
    int code;
    DRW_LWPolyline pl;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addLWPolyline(pl);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            pl.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processPolyline() {
    DBG("dxfRW::processPolyline");
    int code;
    DRW_Polyline pl;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            if (nextentity != "VERTEX") {
            iface->addPolyline(pl);
            return true;  //found new entity or ENDSEC, terminate
            } else {
                processVertex(&pl);
            }
        }
        default:
            pl.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processVertex(DRW_Polyline *pl) {
    DBG("dxfRW::processVertex");
    int code;
    DRW_Vertex *v = new DRW_Vertex();
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            pl->appendVertex(v);
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            if (nextentity == "SEQEND") {
            return true;  //found SEQEND no more vertex, terminate
            } else if (nextentity == "VERTEX"){
                v = new DRW_Vertex(); //another vertex
            }
        }
        default:
            v->parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processText() {
    DBG("dxfRW::processText");
    int code;
    DRW_Text txt;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            txt.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processHatch() {
    DBG("dxfRW::processHatch");
    int code;
    DRW_Hatch hatch;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addHatch(&hatch);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            hatch.parseCode(code, reader);
            break;
        }
    }
    return true;
}


bool dxfRW::processSpline() {
    DBG("dxfRW::processSpline");
/*    int code;
    DRW_Text txt;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            txt.parseCode(code, reader);
            break;
        }
    }*/
    return true;
}


/********* Objects Section *********/

bool dxfRW::processObjects() {
    DBG("dxfRW::processObjects\n");
    int code;
    string sectionstr;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "ENDSEC") {
                return true;  //found ENDSEC terminate
            }
        }
    }
    return true;
}

