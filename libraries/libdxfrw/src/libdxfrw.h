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

#ifndef LIBDXFRW_H
#define LIBDXFRW_H

#include <string>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_interface.h"

#define DRW_VERSION     "0.1.0"

class dxfReader;
class dxfWriter;

class dxfRW {
public:
    dxfRW(const char* name);
    ~dxfRW();
    //read: return 0 if all ok
    bool read(DRW_Interface *interface_, bool ext);
    void setBinary(bool b) {binary = b;}

    bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);
    bool writeLineType(DRW_LType *ent);
    bool writeLayer(DRW_Layer *ent);
    bool writePoint(DRW_Point *ent);
    bool writeLine(DRW_Line *ent);
    bool writeCircle(DRW_Circle *ent);
    bool writeArc(DRW_Arc *ent);
    bool writeEllipse(DRW_Ellipse *ent);
    bool writeTrace(DRW_Trace *ent);
    bool writeSolid(DRW_Solid *ent);
    bool write3dface(DRW_3Dface *ent);
    bool writeLWPolyline(DRW_LWPolyline *ent);

private:
    bool processDxf();
    bool processHeader();
    bool processTables();
    bool processBlocks();
    bool processBlock();
    bool processEntities(bool isblock);
    bool processObjects();

    bool processLType();
    bool processLayer();

    bool processPoint();
    bool processLine();
    bool processCircle();
    bool processArc();
    bool processEllipse();
    bool processTrace();
    bool processSolid();
    bool processInsert();
    bool processLWPolyline();
    bool processPolyline();
    bool processVertex(DRW_Polyline* pl);
    bool processText();
    bool processMText();
    bool processHatch();
    bool processSpline();
    bool process3dface();
    bool processImage();
    bool processImageDef();
    bool processDimension();
    bool processLeader();

//    bool writeHeader();
    bool writeEntity(DRW_Entity *ent);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();

private:
    DRW::Version version;
    std::string fileName;
    bool binary;
    dxfReader *reader;
    dxfWriter *writer;
    DRW_Interface *iface;
    DRW_Header header;
//    int section;
    string nextentity;
    int entCount;
    bool wlayer0;
    bool applyExt;

};

#endif // LIBDXFRW_H
