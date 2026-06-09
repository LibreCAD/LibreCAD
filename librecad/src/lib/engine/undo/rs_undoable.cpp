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

#include "rs_undoable.h"

/**
 * The undoable thing gets activated if it was undone and 
 * deactivated otherwise.
 */
void RS_Undoable::changeDeleteState() {
    toggleFlag(RS2::FlagDeleted);
    deletedStateChanged(isDeleted());
}

/**
 * Undoes or redoes an undoable.
 */
void RS_Undoable::mark(const bool deleted) {
    if (deleted) {
        setFlag(RS2::FlagDeleted);
    }
    else {
        delFlag(RS2::FlagDeleted);
    }
    deletedStateChanged(isDeleted());
}

void RS_Undoable::markDeleted() {
    setFlag(RS2::FlagDeleted);
    deletedStateChanged(true);
}

void RS_Undoable::markLive() {
    delFlag(RS2::FlagDeleted);
    deletedStateChanged(false);
}
