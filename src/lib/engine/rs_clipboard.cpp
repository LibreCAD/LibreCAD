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


#include "rs_clipboard.h"
#include "rs_block.h"
#include "rs_layer.h"
#include "rs_entity.h"

RS_Clipboard* RS_Clipboard::uniqueInstance = NULL;



void RS_Clipboard::clear() {
	graphic.clear();
	graphic.clearBlocks();
	graphic.clearLayers();
	graphic.clearVariables();
}





void RS_Clipboard::addBlock(RS_Block* b) {
	if (b!=NULL) {
		graphic.addBlock(b, false);
	}
}


bool RS_Clipboard::hasBlock(const RS_String& name) {
	return (graphic.findBlock(name)!=NULL);
}


void RS_Clipboard::addLayer(RS_Layer* l) {
	if (l!=NULL) {
		//graphic.addLayer(l->clone());
		graphic.addLayer(l);
	}
}



bool RS_Clipboard::hasLayer(const RS_String& name) {
	return (graphic.findLayer(name)!=NULL);
}



void RS_Clipboard::addEntity(RS_Entity* e) {
	if (e!=NULL) {
		//graphic.addEntity(e->clone());
		graphic.addEntity(e);
		e->reparent(&graphic);
	}
}

/**
 * Dumps the clipboard contents to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Clipboard& cb) {
	os << "Clipboard: " << cb.graphic << "\n";

	return os;
}

