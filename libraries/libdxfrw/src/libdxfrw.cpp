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

#ifdef DRW_DBG
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
    applyExt = false;
}
dxfRW::~dxfRW(){
    if (reader != NULL)
        delete reader;

}

bool dxfRW::read(DRW_Interface *interface_, bool ext){
    bool isOk = false;
    applyExt = ext;
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
    iface = interface_;
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

bool dxfRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin){
    bool isOk = false;
    ofstream filestr;
    version = ver;
    binary = bin;
    iface = interface_;
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
    DRW_Header header;
    iface->writeHeader(header);
    writer->writeString(0, "SECTION");
    entCount =FIRSTHANDLE;
    header.write(writer, version);
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

    if (version > DRW::AC1009) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "OBJECTS");
        writeObjects();
        writer->writeString(0, "ENDSEC");
    }
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
    sprintf(buffer, "%X", ++entCount);
    ent->handle = buffer;
    writer->writeString(5, buffer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, ent->layer);
    writer->writeString(6, ent->lineType);
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1014) {
        writer->writeInt16(370, ent->lWeight);
    }
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
    if (version > DRW::AC1009) {
        ++entCount;
        sprintf(buffer, "%X", entCount);
        writer->writeString(5, buffer);
        if (version > DRW::AC1012) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
    }
    writer->writeString(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeString(3, ent->desc);
    ent->update();
    writer->writeInt16(72, 65);
    writer->writeInt16(73, ent->size);
    writer->writeDouble(40, ent->length);

    for (unsigned int i = 0;  i< ent->path.size(); i++){
        writer->writeDouble(49, ent->path.at(i));
        if (version > DRW::AC1009) {
            writer->writeInt16(74, 0);
        }
    }
    return true;
}

bool dxfRW::writeLayer(DRW_Layer *ent){
    char buffer[5];
    writer->writeString(0, "LAYER");
    if (!wlayer0 && ent->name == "0") {
        wlayer0 = true;
        if (version > DRW::AC1009) {
            writer->writeString(5, "10");
        }
    } else {
        if (version > DRW::AC1009) {
            ++entCount;
            sprintf(buffer, "%X", entCount);
            writer->writeString(5, buffer);
        }
    }
    if (version > DRW::AC1012) {
        writer->writeString(330, "2");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLayerTableRecord");
        writer->writeString(2, ent->name);
    } else {
    string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),::toupper);
    writer->writeString(2, strname);
    }
    writer->writeInt16(70, ent->flags);
    writer->writeInt16(62, ent->color);
    writer->writeString(6, ent->lineType);
    if (version > DRW::AC1009) {
    if (! ent->plotF)
        writer->writeBool(290, ent->plotF);
    writer->writeInt16(370, ent->lWeight);
    writer->writeString(390, "F");
    }
//    writer->writeString(347, "10012");
    return true;
}

bool dxfRW::writeDimstyle(DRW_Dimstyle *ent){
    char buffer[5];
    writer->writeString(0, "DIMSTYLE");
    if (!dimstyleStd) {
            dimstyleStd = true;
    }
        if (version > DRW::AC1009) {
            ++entCount;
            sprintf(buffer, "%X", entCount);
            writer->writeString(105, buffer);
        }

    if (version > DRW::AC1012) {
        writer->writeString(330, "A");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbDimStyleTableRecord");
    }
    writer->writeString(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeString(2, ent->name);
    writer->writeString(3, ent->dimpost);
    writer->writeString(4, ent->dimapost);
    writer->writeString(5, ent->dimblk);
    writer->writeString(6, ent->dimblk1);
    writer->writeString(7, ent->dimblk2);
    writer->writeDouble(41, ent->dimasz);
    writer->writeDouble(42, ent->dimexo);
    writer->writeDouble(44, ent->dimexe);
    writer->writeDouble(140, ent->dimtxt);
    writer->writeDouble(147, ent->dimgap);
    return true;
}

bool dxfRW::writePoint(DRW_Point *ent) {
    writer->writeString(0, "POINT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbPoint");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent) {
    writer->writeString(0, "LINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbLine");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    } else {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
    }
    return true;
}

bool dxfRW::writeRay(DRW_Ray *ent) {
    writer->writeString(0, "RAY");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbRay");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    return true;
}

bool dxfRW::writeXline(DRW_Xline *ent) {
    writer->writeString(0, "XLINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbXline");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    return true;
}

bool dxfRW::writeCircle(DRW_Circle *ent) {
    writer->writeString(0, "CIRCLE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    return true;
}

bool dxfRW::writeArc(DRW_Arc *ent) {
    writer->writeString(0, "ARC");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbArc");
    }
    writer->writeDouble(50, ent->staangle);
    writer->writeDouble(51, ent->endangle);
    return true;
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent){
    if (version > DRW::AC1009) {
        if (ent->staparam == ent->endparam)
            ent->endparam = 6.283185307179586; //2*M_PI;
        writer->writeString(0, "ELLIPSE");
        writeEntity(ent);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbEllipse");
        }
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(40, ent->ratio);
        writer->writeDouble(41, ent->staparam);
        writer->writeDouble(42, ent->endparam);
    } else {
//RLZ: TODO convert ellipse in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeTrace(DRW_Trace *ent){
    writer->writeString(0, "TRACE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    return true;
}

bool dxfRW::writeSolid(DRW_Solid *ent){
    writer->writeString(0, "SOLID");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    return true;
}

bool dxfRW::write3dface(DRW_3Dface *ent){
    writer->writeString(0, "3DFACE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbFace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    writer->writeInt16(70, ent->invisibleflag);
    return true;
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LWPOLYLINE");
        writeEntity(ent);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbPolyline");
        }
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
    } else {
        //RLZ: TODO convert lwpolyline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writePolyline(DRW_Polyline *ent) {
    writer->writeString(0, "POLYLINE");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        if (ent->flags & 8 || ent->flags & 16)
            writer->writeString(100, "AcDb2dPolyline");
        else
            writer->writeString(100, "AcDb3dPolyline");
    }
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeInt16(70, ent->flags);
    if (ent->defstawidth != 0) {
        writer->writeDouble(40, ent->defstawidth);
    }
    if (ent->defendwidth != 0) {
        writer->writeDouble(41, ent->defendwidth);
    }
    if (ent->flags & 16 || ent->flags & 32) {
        writer->writeInt16(71, ent->flags);
        writer->writeInt16(72, ent->flags);
    }
    if (ent->smoothM != 0) {
        writer->writeInt16(73, ent->smoothM);
    }
    if (ent->smoothN != 0) {
        writer->writeInt16(74, ent->smoothM);
    }
    if (ent->curvetype != 0) {
        writer->writeInt16(75, ent->smoothM);
    }
    DRW_Coord crd  = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }

    int vertexnum = ent->vertlist.size();
    for (int i = 0;  i< vertexnum; i++){
        DRW_Vertex *v = ent->vertlist.at(i);
        writer->writeString(0, "VERTEX");
        writeEntity(ent);
        if (version > DRW::AC1009)
            writer->writeString(100, "AcDbVertex");
        if ( (v->flags & 128) && !(v->flags & 64) ) {
            writer->writeDouble(10, 0);
            writer->writeDouble(20, 0);
            writer->writeDouble(30, 0);
        } else {
            writer->writeDouble(10, v->basePoint.x);
            writer->writeDouble(20, v->basePoint.y);
            writer->writeDouble(30, v->basePoint.y);
        }
        if (v->stawidth != 0)
            writer->writeDouble(40, v->stawidth);
        if (v->endwidth != 0)
            writer->writeDouble(41, v->endwidth);
        if (v->bulge != 0)
            writer->writeDouble(42, v->bulge);
        if (v->flags != 0) {
            writer->writeInt16(70, ent->flags);
        }
        if (v->flags & 2) {
            writer->writeDouble(50, v->tgdir);
        }
        if ( v->flags & 128 ) {
            if (v->vindex1 != 0) {
                writer->writeInt16(71, v->vindex1);
            }
            if (v->vindex2 != 0) {
                writer->writeInt16(72, v->vindex2);
            }
            if (v->vindex3 != 0) {
                writer->writeInt16(73, v->vindex3);
            }
            if (v->vindex4 != 0) {
                writer->writeInt16(74, v->vindex4);
            }
            if ( !(v->flags & 64) ) {
                writer->writeInt32(91, v->identifier);
            }
        }
    }
    writer->writeString(0, "SEQEND");
    writeEntity(ent);
    return true;
}

bool dxfRW::writeSpline(DRW_Spline *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "SPLINE");
        writeEntity(ent);
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbSpline");
        }
        writer->writeDouble(210, ent->ex);
        writer->writeDouble(220, ent->ey);
        writer->writeDouble(230, ent->ez);
        writer->writeInt16(70, ent->flags);
        writer->writeInt16(71, ent->degree);
        writer->writeInt16(72, ent->nknots);
        writer->writeInt16(73, ent->ncontrol);
        writer->writeInt16(74, ent->nfit);
        writer->writeDouble(42, ent->tolknot);
        writer->writeDouble(43, ent->tolcontrol);
        //RLZ: warning check if nknots are correct and ncontrol
        for (int i = 0;  i< ent->nknots; i++){
            writer->writeDouble(40, ent->knotslist.at(i));
        }
        for (int i = 0;  i< ent->ncontrol; i++){
            DRW_Coord *crd = ent->controllist.at(i);
            writer->writeDouble(10, crd->x);
            writer->writeDouble(20, crd->y);
            writer->writeDouble(30, crd->z);
        }
    } else {
        //RLZ: TODO convert spline in polyline (not exist in acad 12)
    }
    return true;
}

bool dxfRW::writeHatch(DRW_Hatch *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "HATCH");
        writeEntity(ent);
        writer->writeString(100, "AcDbHatch");
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(70, ent->solid);
        writer->writeInt16(71, ent->associative);
        ent->loopsnum = ent->looplist.size();
        writer->writeInt16(91, ent->loopsnum);
        //write paths data
        for (int i = 0;  i< ent->loopsnum; i++){
            DRW_HatchLoop *loop = ent->looplist.at(i);
            writer->writeInt16(92, loop->type);
            if ( (loop->type & 2) == 2){
                //RLZ: polyline boundary writeme
            } else {
                //boundary path
                loop->update();
                writer->writeInt16(93, loop->numedges);
                for (int j = 0; j<loop->numedges; ++j) {
                    switch ( (loop->objlist.at(j))->eType) {
                    case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j);
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                        break; }
                    case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j);
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle);
                        writer->writeDouble(51, a->endangle);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::ELLIPSE:
                        //RLZ: elliptic arc boundary writeme
//                        writer->writeInt16(72, 3);
                        break;
                    case DRW::SPLINE:
                        //RLZ: spline boundary writeme
//                        writer->writeInt16(72, 4);
                        break;
                    default:
                        break;
                    }
                }
                writer->writeInt16(97, 0);
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        writer->writeDouble(52, ent->angle);
        writer->writeDouble(41, ent->scale);
        writer->writeInt16(77, ent->doubleflag);
        writer->writeInt16(78, ent->deflines);
        writer->writeInt16(98, 0);
    } else {
        //RLZ: TODO verify in acad12
    }
    return true;
}

bool dxfRW::writeLeader(DRW_Leader *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "LEADER");
        writeEntity(ent);
        writer->writeString(100, "AcDbLeader");
        writer->writeString(3, ent->style);
        writer->writeInt16(71, ent->arrow);
        writer->writeInt16(72, ent->leadertype);
        writer->writeInt16(73, ent->flag);
        writer->writeInt16(74, ent->hookline);
        writer->writeInt16(75, ent->hookflag);
        writer->writeDouble(40, ent->textheight);
        writer->writeDouble(41, ent->textwidth);
        writer->writeDouble(76, ent->vertnum);
        writer->writeDouble(76, ent->vertexlist.size());
        for (unsigned int i=0; i<ent->vertexlist.size(); i++) {
            DRW_Coord *vert = ent->vertexlist.at(i);
            writer->writeDouble(10, vert->x);
            writer->writeDouble(20, vert->y);
            writer->writeDouble(30, vert->z);
        }
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}
bool dxfRW::writeDimension(DRW_Dimension *ent) {
    if (version > DRW::AC1009) {
        writer->writeString(0, "DIMENSION");
        writeEntity(ent);
        writer->writeString(100, "AcDbDimension");
//        writer->writeString(2, ent->name);
        writer->writeDouble(10, ent->getDefPoint().x);
        writer->writeDouble(20, ent->getDefPoint().y);
        writer->writeDouble(30, ent->getDefPoint().z);
        writer->writeDouble(11, ent->getTextPoint().x);
        writer->writeDouble(21, ent->getTextPoint().y);
        writer->writeDouble(31, ent->getTextPoint().z);
        if ( !(ent->type & 32))
            ent->type = ent->type +32;
        writer->writeInt16(70, ent->type);
        if ( !(ent->getText().empty()) )
            writer->writeString(1, ent->getText());
        writer->writeInt16(71, ent->getAlign());
        if ( ent->getTextLineStyle() != 1)
            writer->writeInt16(72, ent->getTextLineStyle());
        if ( ent->getTextLineFactor() != 1)
            writer->writeDouble(41, ent->getTextLineFactor());
        writer->writeString(3, ent->getStyle());
        if ( ent->getTextLineFactor() != 0)
            writer->writeDouble(53, ent->getDir());
        writer->writeDouble(210, ent->getExtrusion().x);
        writer->writeDouble(220, ent->getExtrusion().y);
        writer->writeDouble(230, ent->getExtrusion().z);

        switch (ent->eType) {
        case DRW::DIMALIGNED:
        case DRW::DIMLINEAR: {
            DRW_DimAligned * dd = (DRW_DimAligned*)ent;
            writer->writeString(100, "AcDbAlignedDimension");
            DRW_Coord crd = dd->getClonepoint();
            if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
                writer->writeDouble(12, crd.x);
                writer->writeDouble(22, crd.y);
                writer->writeDouble(32, crd.z);
            }
            writer->writeDouble(13, dd->getDef1Point().x);
            writer->writeDouble(23, dd->getDef1Point().y);
            writer->writeDouble(33, dd->getDef1Point().z);
            writer->writeDouble(14, dd->getDef2Point().x);
            writer->writeDouble(24, dd->getDef2Point().y);
            writer->writeDouble(34, dd->getDef2Point().z);
            if (ent->eType == DRW::DIMLINEAR) {
                DRW_DimLinear * dl = (DRW_DimLinear*)ent;
                writer->writeString(100, "AcDbRotatedDimension");
                if (dl->getAngle() != 0)
                    writer->writeDouble(50, dl->getAngle());
                if (dl->getOblique() != 0)
                    writer->writeDouble(52, dl->getOblique());
            }
            break; }
        case DRW::DIMRADIAL: {
            DRW_DimRadial * dd = (DRW_DimRadial*)ent;
            writer->writeString(100, "AcDbRadialDimension");
            writer->writeDouble(15, dd->getDiameterPoint().x);
            writer->writeDouble(25, dd->getDiameterPoint().y);
            writer->writeDouble(35, dd->getDiameterPoint().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMDIAMETRIC: {
            DRW_DimDiametric * dd = (DRW_DimDiametric*)ent;
            writer->writeString(100, "AcDbDiametricDimension");
            writer->writeDouble(15, dd->getDiameter1Point().x);
            writer->writeDouble(25, dd->getDiameter1Point().y);
            writer->writeDouble(35, dd->getDiameter1Point().z);
            writer->writeDouble(40, dd->getLeaderLength());
            break; }
        case DRW::DIMANGULAR: {
            DRW_DimAngular * dd = (DRW_DimAngular*)ent;
            writer->writeString(100, "AcDb2LineAngularDimension");
            writer->writeDouble(13, dd->getFirstLine1().x);
            writer->writeDouble(23, dd->getFirstLine1().y);
            writer->writeDouble(33, dd->getFirstLine1().z);
            writer->writeDouble(14, dd->getFirstLine2().x);
            writer->writeDouble(24, dd->getFirstLine2().y);
            writer->writeDouble(34, dd->getFirstLine2().z);
            writer->writeDouble(15, dd->getSecondLine1().x);
            writer->writeDouble(25, dd->getSecondLine1().y);
            writer->writeDouble(35, dd->getSecondLine1().z);
            writer->writeDouble(16, dd->getDimPoint().x);
            writer->writeDouble(26, dd->getDimPoint().y);
            writer->writeDouble(36, dd->getDimPoint().z);
            break; }
        case DRW::DIMANGULAR3P: {
            DRW_DimAngular3p * dd = (DRW_DimAngular3p*)ent;
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            writer->writeDouble(15, dd->getVertexPoint().x);
            writer->writeDouble(25, dd->getVertexPoint().y);
            writer->writeDouble(35, dd->getVertexPoint().z);
            break; }
        case DRW::DIMORDINATE: {
            DRW_DimOrdinate * dd = (DRW_DimOrdinate*)ent;
            writer->writeString(100, "AcDbOrdinateDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            break; }
        default:
            break;
        }
    } else  {
        //RLZ: todo not supported by acad 12 saved as unnamed block
    }
    return true;
}

bool dxfRW::writeInsert(DRW_Insert *ent){
    writer->writeString(0, "INSERT");
    writeEntity(ent);
/*    char buffer[5];
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, buffer);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbEntity");
    }
    if (version > DRW::AC1014) {
        writer->writeString(330, "1F");
    }
    writer->writeString(8, ent->layer);
    writer->writeString(6, ent->lineType);
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1014) {
        writer->writeInt16(370, ent->lWeight);
    }*/
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockReference");
    }
    writer->writeString(2, ent->name);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(41, ent->xscale);
    writer->writeDouble(42, ent->yscale);
    writer->writeDouble(43, ent->zscale);
    writer->writeDouble(50, ent->angle);
    writer->writeInt16(70, ent->colcount);
    writer->writeInt16(71, ent->rowcount);
    writer->writeDouble(44, ent->colspace);
    writer->writeDouble(45, ent->rowspace);
    return true;
}

bool dxfRW::writeText(DRW_Text *ent){
    writer->writeString(0, "TEXT");
    writeEntity(ent);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
//    writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeString(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    writer->writeString(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW::HAlignLeft) {
        writer->writeInt16(72, ent->alignH);
    }
    if (ent->alignH != DRW::HAlignLeft || ent->alignV != DRW::VAlignBaseLine) {
        writer->writeInt16(72, ent->alignH);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->alignV != DRW::VAlignBaseLine) {
        writer->writeInt16(73, ent->alignV);
    }
    return true;
}

bool dxfRW::writeMText(DRW_MText *ent){
    if (version > DRW::AC1009) {
        writer->writeString(0, "MTEXT");
        writeEntity(ent);
        writer->writeString(100, "AcDbMText");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(40, ent->height);
        writer->writeDouble(41, ent->widthscale);
        writer->writeInt16(71, ent->textgen);
        writer->writeInt16(72, ent->alignH);
        int i;
        for(i =0; (ent->text.size()-i) > 250; ) {
            writer->writeString(3, ent->text.substr(i, 250));
            i +=250;
        }
        writer->writeString(1, ent->text.substr(i));
        writer->writeString(7, ent->style);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeDouble(50, ent->angle);
        writer->writeInt16(73, ent->alignV);
        writer->writeDouble(44, ent->interlin);
//RLZ ... 11, 21, 31 needed?
    } else {
        //RLZ: TODO convert mtext in text lines (not exist in acad 12)
    }
    return true;
}

DRW_ImageDef* dxfRW::writeImage(DRW_Image *ent, std::string name){
    if (version > DRW::AC1009) {
        //search if exist imagedef with this mane (image inserted more than 1 time)
        //RLZ: imagedef_reactor seem needed to read in acad
        DRW_ImageDef *id = NULL;
        for (unsigned int i=0; i<imageDef.size(); i++) {
            if (imageDef.at(i)->name == name ) {
                id = imageDef.at(i);
                continue;
            }
        }
        char buffer[5];
        if (id == NULL) {
            id = new DRW_ImageDef();
            imageDef.push_back(id);
            sprintf(buffer, "%X", ++entCount);
            id->handle = buffer;
        }
        id->name = name;
        sprintf(buffer, "%X", ++entCount);

        writer->writeString(0, "IMAGE");
        writeEntity(ent);
        writer->writeString(100, "AcDbRasterImage");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
        writer->writeDouble(12, ent->vx);
        writer->writeDouble(22, ent->vy);
        writer->writeDouble(32, ent->vz);
        writer->writeDouble(13, ent->sizeu);
        writer->writeDouble(23, ent->sizev);
        writer->writeString(340, id->handle);
        writer->writeInt16(70, 1);
        writer->writeInt16(280, ent->clip);
        writer->writeInt16(281, ent->brightness);
        writer->writeInt16(282, ent->contrast);
        writer->writeInt16(283, ent->fade);
        writer->writeString(360, buffer);
        id->reactors[buffer] = ent->handle;
        return id;
    }
    return NULL; //not exist in acad 12
}

bool dxfRW::writeBlockRecord(std::string name){
    writer->writeString(0, "BLOCK_RECORD");
    char buffer[5];
    entCount = 1+entCount;
    sprintf(buffer, "%X", entCount);
    writer->writeString(5, buffer);
    blockMap[name] = entCount;
    entCount = 2+entCount;//reserve 2 for BLOCK & ENDBLOCK
    if (version > DRW::AC1014) {
        writer->writeString(330, "1");
    }
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbBlockTableRecord");
    writer->writeString(2, name);
    if (version > DRW::AC1018) {
    //    writer->writeInt16(340, 22);
    writer->writeInt16(70, 0);
    writer->writeInt16(280, 1);
    writer->writeInt16(281, 0);
    }
    return true;
}

bool dxfRW::writeBlock(DRW_Block *bk){
    char buffer[5];
    if (writingBlock) {
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            sprintf(buffer, "%X", currHandle+2);
            writer->writeString(5, buffer);
            if (version > DRW::AC1014) {
                sprintf(buffer, "%X", currHandle);
                writer->writeString(330, buffer);
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        writer->writeString(100, "AcDbBlockEnd");
    }
    writingBlock = true;
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        currHandle = (*(blockMap.find(bk->name))).second;
        sprintf(buffer, "%X", currHandle+1);
        writer->writeString(5, buffer);
        if (version > DRW::AC1014) {
            sprintf(buffer, "%X", currHandle);
            writer->writeString(330, buffer);
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockBegin");
    writer->writeString(2, bk->name);
    writer->writeInt16(70, bk->flags);
    writer->writeDouble(10, bk->basePoint.x);
    writer->writeDouble(20, bk->basePoint.y);
    if (bk->basePoint.z != 0.0) {
        writer->writeDouble(30, bk->basePoint.z);
    }
    writer->writeString(3, bk->name);
    writer->writeString(1, "");

    return true;
}

bool dxfRW::writeTables() {
    char buffer[5];
    writer->writeString(0, "TABLE");
    writer->writeString(2, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, "8");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "VPORT");
    if (version > DRW::AC1009) {
        entCount = 1+entCount;
        sprintf(buffer, "%X", entCount);
        writer->writeString(5, buffer);
        if (version > DRW::AC1014) {
            writer->writeString(330, "8");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewportTableRecord");
    }
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
    if (version > DRW::AC1014) {
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
        if (version > DRW::AC1018) {
            writer->writeString(348, "10020");
            writer->writeInt16(60, 3);//v2007 undocummented
            writer->writeInt16(61, 5);
            writer->writeBool(292, 1);
            writer->writeInt16(282, 1);
            writer->writeDouble(141, 0.0);
            writer->writeDouble(142, 0.0);
            writer->writeInt16(63, 250);
            writer->writeInt32(421, 3358443);
        }
    }
    writer->writeString(0, "ENDTAB");
/*** LTYPE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "5");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 4); //end table def
//Mandatory linetypes
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "14");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByBlock");
    } else
        writer->writeString(2, "BYBLOCK");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "15");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByLayer");
    } else
        writer->writeString(2, "BYLAYER");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "16");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
    }
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
    if (version > DRW::AC1009) {
        writer->writeString(5, "2");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
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
    if (version > DRW::AC1009) {
        writer->writeString(5, "3");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, "6");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, "7");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0); //end table def
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "9");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "12");
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
    }
    writer->writeString(2, "ACAD");
    writer->writeInt16(70, 0);
    writer->writeString(0, "ENDTAB");

    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "A");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    if (version > DRW::AC1014) {
        writer->writeString(100, "AcDbDimStyleTable");
        writer->writeInt16(71, 1); //end table def
    }
    dimstyleStd =false;
    iface->writeDimstyles();
    if (!dimstyleStd) {
        DRW_Dimstyle dsty;
        dsty.name = "Standard";
        writeDimstyle(&dsty);
    }
    writer->writeString(0, "ENDTAB");

    if (version > DRW::AC1009) {
        writer->writeString(0, "TABLE");
        writer->writeString(2, "BLOCK_RECORD");
        writer->writeString(5, "1");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        writer->writeInt16(70, 2); //end table def
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1F");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Model_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1E");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Paper_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
        iface->writeBlockRecords();
        writer->writeString(0, "ENDTAB");
    }
return true;
}

bool dxfRW::writeBlocks() {
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "20");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
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
    if (version > DRW::AC1009) {
        writer->writeString(5, "21");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockEnd");

    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1C");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1B");
        }
        writer->writeString(100, "AcDbEntity");
    }
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
    if (version > DRW::AC1009) {
        writer->writeString(5, "1D");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbBlockEnd");
    writingBlock = false;
    iface->writeBlocks();
    if (writingBlock) {
        writingBlock = false;
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            char buffer[5];
            sprintf(buffer, "%X", currHandle+2);
            writer->writeString(5, buffer);
//            writer->writeString(5, "1D");
            if (version > DRW::AC1014) {
                sprintf(buffer, "%X", currHandle);
                writer->writeString(330, buffer);
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        writer->writeString(100, "AcDbBlockEnd");
    }
    return true;
}

bool dxfRW::writeObjects() {
    writer->writeString(0, "DICTIONARY");
    char buffer[5];
    writer->writeString(5, "C");
    if (version > DRW::AC1014) {
        writer->writeString(330, "0");
    }
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    writer->writeString(3, "ACAD_GROUP");
    writer->writeString(350, "D");
    if (imageDef.size() != 0) {
        sprintf(buffer, "%X", ++entCount);
        writer->writeString(3, "ACAD_IMAGE_DICT");
        writer->writeString(350, buffer);
    }
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "D");
    writer->writeString(330, "C");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
//write IMAGEDEF_REACTOR
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        map<string, string>::iterator it;
        for ( it=id->reactors.begin() ; it != id->reactors.end(); it++ ) {
            writer->writeString(0, "IMAGEDEF_REACTOR");
            writer->writeString(5, (*it).first);
            writer->writeString(330, (*it).second);
            writer->writeString(100, "AcDbRasterImageDefReactor");
            writer->writeInt16(90, 2); //version 2=R14 to v2010
            writer->writeString(330, (*it).second);
        }
    }
    if (imageDef.size() != 0) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, buffer);
        writer->writeString(330, "C");
        writer->writeString(100, "AcDbDictionary");
        writer->writeInt16(281, 1);
        for (unsigned int i=0; i<imageDef.size(); i++) {
            size_t f1, f2;
            f1 = imageDef.at(i)->name.find_last_of("/\\");
            f2 =imageDef.at(i)->name.find_last_of('.');
            ++f1;
            writer->writeString(3, imageDef.at(i)->name.substr(f1,f2-f1));
            writer->writeString(350, imageDef.at(i)->handle);
        }
    }
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        writer->writeString(0, "IMAGEDEF");
        writer->writeString(5, id->handle);
        if (version > DRW::AC1014) {
//            writer->writeString(330, "0"); handle to DICTIONARY
        }
        writer->writeString(102, "{ACAD_REACTORS");
        map<string, string>::iterator it;
        for ( it=id->reactors.begin() ; it != id->reactors.end(); it++ ) {
            writer->writeString(330, (*it).first);
        }
        writer->writeString(102, "}");
        writer->writeString(100, "AcDbRasterImageDef");
        writer->writeInt16(90, 0); //version 0=R14 to v2010
        writer->writeString(1, id->name);
        writer->writeDouble(10, id->u);
        writer->writeDouble(20, id->v);
        writer->writeDouble(11, id->up);
        writer->writeDouble(21, id->vp);
        writer->writeInt16(280, id->loaded);
        writer->writeInt16(281, id->resolution);
    }
    //no more needed imageDef, delete it
    while (!imageDef.empty()) {
       imageDef.pop_back();
    }

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
                iface->addHeader(&header);
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
                        processDimStyle();
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

bool dxfRW::processDimStyle() {
    DBG("dxfRW::processDimStyle");
    int code;
    string sectionstr;
    bool reading = false;
    DRW_Dimstyle dimSty;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        if (code == 0) {
            if (reading)
                iface->addDimStyle(dimSty);
            sectionstr = reader->getString();
            DBG(sectionstr); DBG("\n");
            if (sectionstr == "DIMSTYLE") {
                reading = true;
            } else if (sectionstr == "ENDTAB") {
                return true;  //found ENDTAB terminate
            }
        } else if (reading)
            dimSty.parseCode(code, reader);
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
        } else if (nextentity == "MTEXT") {
            processMText();
        } else if (nextentity == "HATCH") {
            processHatch();
        } else if (nextentity == "SPLINE") {
            processSpline();
        } else if (nextentity == "3DFACE") {
            process3dface();
        } else if (nextentity == "IMAGE") {
            processImage();
        } else if (nextentity == "DIMENSION") {
            processDimension();
        } else if (nextentity == "LEADER") {
            processLeader();
        } else if (nextentity == "RAY") {
            processRay();
        } else if (nextentity == "XLINE") {
            processXline();
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
            if (applyExt)
                trace.applyExtrusion();
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
            if (applyExt)
                solid.applyExtrusion();
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

bool dxfRW::process3dface() {
    DBG("dxfRW::process3dface");
    int code;
    DRW_3Dface face;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->add3dFace(face);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            face.parseCode(code, reader);
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

bool dxfRW::processRay() {
    DBG("dxfRW::processRay\n");
    int code;
    DRW_Ray line;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addRay(line);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            line.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processXline() {
    DBG("dxfRW::processXline\n");
    int code;
    DRW_Xline line;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addXline(line);
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
            if (applyExt)
                circle.applyExtrusion();
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
            if (applyExt)
                arc.applyExtrusion();
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
            if (applyExt)
                pl.applyExtrusion();
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

bool dxfRW::processMText() {
    DBG("dxfRW::processMText");
    int code;
    DRW_MText txt;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            txt.updateAngle();
            iface->addMText(txt);
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
    int code;
    DRW_Spline sp;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addSpline(&sp);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            sp.parseCode(code, reader);
            break;
        }
    }
    return true;
}


bool dxfRW::processImage() {
    DBG("dxfRW::processImage");
    int code;
    DRW_Image img;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            img.parseCode(code, reader);
            break;
        }
    }
    return true;
}


bool dxfRW::processDimension() {
    DBG("dxfRW::processDimension");
    int code;
    DRW_Dimension dim;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            int type = dim.type & 0x0F;
            switch (type) {
            case 0: {
                DRW_DimLinear d(dim);
                iface->addDimLinear(&d);
                break; }
            case 1: {
                DRW_DimAligned d(dim);
                iface->addDimAlign(&d);
                break; }
            case 2:  {
                DRW_DimAngular d(dim);
                iface->addDimAngular(&d);
                break;}
            case 3: {
                DRW_DimDiametric d(dim);
                iface->addDimDiametric(&d);
                break; }
            case 4: {
                DRW_DimRadial d(dim);
                iface->addDimRadial(&d);
                break; }
            case 5: {
                DRW_DimAngular3p d(dim);
                iface->addDimAngular3P(&d);
                break; }
            case 6: {
                DRW_DimOrdinate d(dim);
                iface->addDimOrdinate(&d);
                break; }
            }
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            dim.parseCode(code, reader);
            break;
        }
    }
    return true;
}

bool dxfRW::processLeader() {
    DBG("dxfRW::processLeader");
    int code;
    DRW_Leader leader;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->addLeader(&leader);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            leader.parseCode(code, reader);
            break;
        }
    }
    return true;
}


/********* Objects Section *********/

bool dxfRW::processObjects() {
    DBG("dxfRW::processObjects\n");
    int code;
    if (!reader->readRec(&code, !binary)){
        return false;
    }
    bool next = true;
    if (code == 0) {
            nextentity = reader->getString();
    } else {
            return false;  //first record in objects is 0
   }
    do {
        if (nextentity == "ENDSEC") {
            return true;  //found ENDSEC terminate
        } else if (nextentity == "IMAGEDEF") {
            processImageDef();
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

bool dxfRW::processImageDef() {
    DBG("dxfRW::processImageDef");
    int code;
    DRW_ImageDef img;
    while (reader->readRec(&code, !binary)) {
        DBG(code); DBG("\n");
        switch (code) {
        case 0: {
            nextentity = reader->getString();
            DBG(nextentity); DBG("\n");
            iface->linkImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }
        default:
            img.parseCode(code, reader);
            break;
        }
    }
    return true;
}
