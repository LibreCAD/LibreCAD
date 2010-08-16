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


#include "rs_undoable.h"

#include "rs_undocycle.h"


/**
 * Default constructor.
 */
RS_Undoable::RS_Undoable() {
    cycle = NULL;
}



/**
 * Destructor. Makes sure that this undoable is removed from 
 * its undo cycle before it is deleted.
 */
RS_Undoable::~RS_Undoable() {
    if (cycle!=NULL) {
        cycle->removeUndoable(this);
    }
}



/**
 * Sets the undo cycle this entity is in. This is necessary to
 * make sure the entity can remove itself from the cycle before
 * being deleted.
 */
void RS_Undoable::setUndoCycle(RS_UndoCycle* cycle) {
    this->cycle = cycle;
}



/**
 * The undoable thing gets activated if it was undone and 
 * deactivated otherwise.
 */
void RS_Undoable::changeUndoState() {
    toggleFlag(RS2::FlagUndone);
	undoStateChanged(isUndone());
}



/**
 * Undoes or redoes an undoable.
 */
void RS_Undoable::setUndoState(bool undone) {
    if (undone) {
        setFlag(RS2::FlagUndone);
    } else {
        delFlag(RS2::FlagUndone);
    }
	undoStateChanged(isUndone());
}



/**
 * Is this entity in the Undo memory and not active?
 */
bool RS_Undoable::isUndone() const {
    return getFlag(RS2::FlagUndone);
}

