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
//#include <math.h>
#include <fstream>
#include "dxfreader.h"
#include "dxfwriter.h"


using namespace std;

#ifdef DRW_DBG2
#include <iostream> //for debug
#define DBG(a) std::cerr << a
#else
#define DBG(a)
#endif

enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};

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
    filestr.open (fileName.c_str(), ios_base::in);
    if (!filestr.is_open())
        return isOk;
    if (!filestr.good())
        return isOk;

    char line[256];
    filestr.getline (line,256, '\n');
    string str ("AutoCAD Binary DXF\r");
    filestr.close();
    iface = interface;
    DBG("dxfRW::read 2\n");
    if (str.compare(line) == 0) {
        filestr.open (fileName.c_str(), ios_base::out | ios::binary);
        binary = true;
        //skip sentinel
        filestr.seekg (22, ios::beg);
        reader = new dxfReaderBinary(&filestr);
        DBG("dxfRW::read binary file\n");
    } else {
        binary = false;
        filestr.open (fileName.c_str(), ios_base::out);
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
    binary = bin;
    iface = interface;
    if (binary) {
        filestr.open (fileName.c_str(), ios_base::out | ios::binary | ios::trunc);
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
//        string str ("AutoCAD Binary DXF\r");
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
        writer->writeString(5, "0xFFFF");
    }

    writer->writeString(9, "$DWGCODEPAGE");
    writer->writeString(3, "ANSI_1252");
    writer->writeString(0, "ENDSEC");/*
    writer->writeString(0, "SECTION");
    writer->writeString(2, "CLASSES");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "BLOCKS");
    writer->writeString(0, "ENDSEC");*/

    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    entCount =100;
    iface->writeEntity();
    writer->writeString(0, "ENDSEC");

/*    writer->writeString(0, "SECTION");
    writer->writeString(2, "OBJECTS");
    writer->writeString(0, "ENDSEC");*/
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
    writer->writeInt16(370, ent->lWeight);
    writer->writeString(6, ent->lineType);
    writer->writeInt16(62, ent->color);
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

bool dxfRW::writeArc(DRW_Arc *ent) {
    writer->writeString(0, "ARC");
    writeEntity(ent);
    writer->writeString(100, "AcDbArc");
    writer->writeDouble(10, ent->x);
    writer->writeDouble(20, ent->y);
    if (ent->z != 0.0) {
        writer->writeDouble(30, ent->z);
    }
    writer->writeDouble(40, ent->radious);
    writer->writeDouble(50, ent->staangle);
    writer->writeDouble(51, ent->endangle);
    return true;
}

bool dxfRW::processDxf() {
    DBG("dxfRW::processDxf()\n");
    int code;
    bool more = true;
    string sectionstr;
    section = secUnknown;
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
                    } else if (sectionstr == "TABLES") {
                        processTables();
                    } else if (sectionstr == "BLOCKS") {
                        processBlocks();
                    } else if (sectionstr == "ENTITIES") {
                        processEntities();
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
        }
    }
    return true;
}
bool dxfRW::processTables() {
    DBG("dxfRW::processTables\n");
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

bool dxfRW::processBlocks() {
    DBG("dxfRW::processBlocks\n");
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

bool dxfRW::processEntities() {
    DBG("dxfRW::processEntities\n");
    int code;
    if (reader->readRec(&code, !binary)){
        bool next = true;
        if (code == 0) {
            nextentity = reader->getString();
        } else
            return false;  //first record in entities is 0
        do {
            if (nextentity == "ENDSEC") {
                return true;  //found ENDSEC terminate
            } else if (nextentity == "POINT") {
                processPoint();
            } else if (nextentity == "LINE") {
                processLine();
            } else if (nextentity == "CIRCLE") {
                processCircle();
            } else if (nextentity == "ARC") {
                processArc();
            } else {
                if (reader->readRec(&code, !binary)){
                    if (code == 0)
                        nextentity = reader->getString();
                } else
                    return false; //end of file without ENDSEC
            }

        } while (next);
    }
    return true;
}

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
