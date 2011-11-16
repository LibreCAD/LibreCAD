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

class dxfReader;

class dxfRW {
public:
    dxfRW(const char* name);
    ~dxfRW();
    //read: return 0 if all ok
    bool read(DRW_Interface *interface);
    bool write();
    void setBinary(bool b) {binary = b;}

private:
    bool processDxf();
    bool processHeader();
    bool processTables();
    bool processBlocks();
    bool processEntities();
    bool processObjects();

    bool processPoint();
    bool processLine();
    bool processCircle();
    bool processArc();
    /*    virtual PluginCapabilities getCapabilities() const;
    virtual QString name() const;
    virtual void execComm(Document_Interface *doc,
                                       QWidget *parent, QString cmd);*/
private:
    std::string fileName;
    bool binary;
    dxfReader *reader;
    DRW_Interface *iface;
    int section;
    string nextentity;
};

#endif // LIBDXFRW_H
