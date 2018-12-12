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

#include <iostream>
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
	if (b) {
		graphic.addBlock(b, false);
	}
}


bool RS_Clipboard::hasBlock(const QString& name) {
	return (graphic.findBlock(name));
}


void RS_Clipboard::addLayer(RS_Layer* l) {
	if (l) {
		//graphic.addLayer(l->clone());
		graphic.addLayer(l);
	}
}



bool RS_Clipboard::hasLayer(const QString& name) {
	return (graphic.findLayer(name));
}



void RS_Clipboard::addEntity(RS_Entity* e) {
	if (e) {
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

