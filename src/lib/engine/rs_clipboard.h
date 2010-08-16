/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <iostream>
#include "rs_graphic.h"

#define RS_CLIPBOARD RS_Clipboard::instance()

class RS_Block;
class RS_Layer;
class RS_Entity;

/**
 * CADuntu internal clipboard. We don't use the system clipboard for
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
	bool hasBlock(const RS_String& name);
	int  countBlocks() {
		return graphic.countBlocks();
	}
	RS_Block* blockAt(int i) {
		return graphic.blockAt(i);
	}
	
	void addLayer(RS_Layer* l);
	bool hasLayer(const RS_String& name);
	int  countLayers() {
		return graphic.countLayers();
	}
	RS_Layer* layerAt(int i) {
		return graphic.layerAt(i);
	}

	void addEntity(RS_Entity* e);

	uint count() {
		return graphic.count();
	}
	RS_Entity* entityAt(uint i) {
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

