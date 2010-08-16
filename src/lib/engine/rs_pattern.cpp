/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#include "rs_pattern.h"

#include "rs_stringlist.h"
#include "rs_fileinfo.h"
#include "rs_system.h"
#include "rs_fileio.h"


/**
 * Constructor.
 *
 * @param fileName File name of a DXF file defining the pattern
 */
RS_Pattern::RS_Pattern(const RS_String& fileName)
        : RS_EntityContainer(NULL) {

    RS_DEBUG->print("RS_Pattern::RS_Pattern() ");

	this->fileName = fileName;
	loaded = false;
}



/**
 * Constructor.
 *
 * @param fileName File name of a PAT file which defines this 
 *         pattern among others.
 * @param name Pattern name.
 *
 */
/*RS_Pattern::RS_Pattern(const RS_String& fileName, const RS_String& name)
        : RS_EntityContainer(NULL) {
	this->fileName = fileName;
	this->name = name;
	loaded = false;
}*/



RS_Pattern::~RS_Pattern() {}


/**
 * Loads the given pattern file into this pattern.
 * Entities other than lines are ignored.
 *
 * @param filename File name of the pattern file (without path and 
 * extension or full path.
 */
bool RS_Pattern::loadPattern() {
    if (loaded) {
        return true;
    }

	RS_DEBUG->print("RS_Pattern::loadPattern");

    RS_String path;

    // Search for the appropriate pattern if we have only the name of the pattern:
    if (!fileName.lower().contains(".dxf")) {
        RS_StringList patterns = RS_SYSTEM->getPatternList();
        RS_FileInfo file;
        for (RS_StringList::Iterator it = patterns.begin();
                it!=patterns.end();
                it++) {

            if (RS_FileInfo(*it).baseName().lower()==fileName.lower()) {
                path = *it;
				RS_DEBUG->print("Pattern found: %s", path.latin1());
                break;
            }
        }
    }

    // We have the full path of the pattern:
    else {
        path = fileName;
    }

    // No pattern paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print("No pattern \"%s\"available.", fileName.latin1());
        return false;
    }

	RS_Graphic* gr = new RS_Graphic();

	// TODO: Find out why the new dxf filter doesn't work for patterns:
	RS_FILEIO->fileImport(*gr, path, RS2::FormatDXF1);

	for (RS_Entity* e=gr->firstEntity(); e!=NULL; e=gr->nextEntity()) {
		if (e->rtti()==RS2::EntityLine || e->rtti()==RS2::EntityArc) {
			RS_Layer* l = e->getLayer();
			RS_Entity* cl = e->clone();
			cl->reparent(this);
			if (l!=NULL) {
				cl->setLayer(l->getName());
			}
			addEntity(cl);
		}
	}
	delete gr;

    loaded = true;
	RS_DEBUG->print("RS_Pattern::loadPattern: OK");

	return true;
}

