/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_CLIPBOARD_H
#define RS_CLIPBOARD_H

#include <iosfwd>
#include "rs_graphic.h"

#define RS_CLIPBOARD RS_Clipboard::instance()

class RS_Block;
class RS_Layer;
class RS_Entity;

/**
 * LibreCAD internal clipboard. We don't use the system clipboard for
 * better portaility.
 * Implemented as singleton.
 *
 * @author Andrew Mustun
 */
class RS_Clipboard {
protected:
    RS_Clipboard() {
    }

public:
    /**
     * @return Instance to the unique clipboard object.
     */
    static RS_Clipboard* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_Clipboard();
        }
        return uniqueInstance;
    }

	void clear();

	void addBlock(RS_Block* b);
        bool hasBlock(const QString& name);
	int  countBlocks() {
		return graphic.countBlocks();
	}
	RS_Block* blockAt(int i) {
		return graphic.blockAt(i);
	}
	
	void addLayer(RS_Layer* l);
        bool hasLayer(const QString& name);
	int  countLayers() {
		return graphic.countLayers();
	}
	RS_Layer* layerAt(int i) {
		return graphic.layerAt(i);
	}

	void addEntity(RS_Entity* e);

    unsigned count() {
		return graphic.count();
	}
    RS_Entity* entityAt(unsigned i) {
		return graphic.entityAt(i);
	}
	RS_Entity* firstEntity() {
		return graphic.firstEntity();
	}
	
	RS_Entity* nextEntity() {
		return graphic.nextEntity();
	}

	RS_Graphic* getGraphic() {
		return &graphic;
	}

    friend std::ostream& operator << (std::ostream& os, RS_Clipboard& cb);

protected:
    static RS_Clipboard* uniqueInstance;

	RS_Graphic graphic;
};

#endif

