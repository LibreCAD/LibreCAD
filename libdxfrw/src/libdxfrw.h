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
#include "drw_interface.h"

#define DRW_VERSION     "0.0.1"

class dxfReader;
class dxfWriter;

class dxfRW {
public:
    dxfRW(const char* name);
    ~dxfRW();
    //read: return 0 if all ok
    bool read(DRW_Interface *interface);
    void setBinary(bool b) {binary = b;}

    bool write(DRW_Interface *interface, DRW::Version ver, bool bin);
    bool writeLine(DRW_Line *ent);
    bool writeCircle(DRW_Circle *ent);
    bool writeArc(DRW_Arc *ent);

private:
    bool processDxf();
    bool processHeader();
    bool processTables();
    bool processBlocks();
    bool processEntities();
    bool processObjects();

    bool processLayer();

    bool processPoint();
    bool processLine();
    bool processCircle();
    bool processArc();

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
//    int section;
    string nextentity;
    int entCount;

};

#endif // LIBDXFRW_H
