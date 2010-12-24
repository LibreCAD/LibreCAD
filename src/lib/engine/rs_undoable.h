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


#ifndef RS_UNDOABLE_H
#define RS_UNDOABLE_H

#include "rs_flags.h"

class RS_UndoCycle;



/**
 * Base class for something that can be added and deleted and every 
 * addition and deletion can be undone.
 *
 * @see RS_Undo
 * @author Andrew Mustun
 */
class RS_Undoable : public RS_Flags {
public:
    RS_Undoable();
    virtual ~RS_Undoable();

    /**
     * Runtime type identification for undoables.
     * Note that this is voluntarily. The default implementation 
     * returns RS2::UndoableUnknown.
     */
    virtual RS2::UndoableType undoRtti() {
        return RS2::UndoableUnknown;
    }

    virtual void setUndoCycle(RS_UndoCycle* cycle);
    virtual void changeUndoState();
    virtual void setUndoState(bool undone);
    virtual bool isUndone() const;

	/**
	 * Can be overwriten by the implementing class to be notified
	 * when the undo state changes (the undoable becomes visible / invisible).
	 */
	virtual void undoStateChanged(bool /*undone*/) {}

    //friend std::ostream& operator << (std::ostream& os, RS_Undoable& a);

private:
    RS_UndoCycle* cycle;
};

#endif
