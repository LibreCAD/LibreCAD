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


#ifndef RS_UNDOLISTITEM_H
#define RS_UNDOLISTITEM_H

#include <iostream>

#include "rs.h"
#include "rs_entity.h"
#include "rs_ptrlist.h"
#include "rs_undoable.h"

/**
 * An Undo Cycle represents an action that was triggered and can 
 * be undone. It stores all the pointers to the Undoables affected by 
 * the action. Undoables are entities in a container that can be
 * created and deleted.
 *
 * Undo Cycles are stored within classes derrived from RS_Undo.
 *
 * @see RS_Undoable
 * @see RS_Undo
 *
 * @author Andrew Mustun
 */
class RS_UndoCycle {
public:
    /**
     * @param type Type of undo item.
     */
    RS_UndoCycle(/*RS2::UndoType type*/) {
        //this->type = type;
        undoables.setAutoDelete(false);
    }

    /**
     * Adds an Undoable to this Undo Cycle. Every Cycle can contain one or
     * more Undoables.
     */
    void addUndoable(const RS_Undoable* u) {
        undoables.append(u);
    }

    /**
     * Removes an undoable from the list.
     */
    void removeUndoable(RS_Undoable* u) {
        undoables.remove(u);
    }

    /**
     * Iteration through undoable elements in this item.
     */
    RS_Undoable* getFirstUndoable() {
        return undoables.first();
    }

    /**
     * Iteration through undoable elements in this item.
     */
    RS_Undoable* getNextUndoable() {
        return undoables.next();
    }

    friend std::ostream& operator << (std::ostream& os,
                                      RS_UndoCycle& i) {
        os << " Undo item: " << "\n";
        //os << "   Type: ";
        /*switch (i.type) {
        case RS2::UndoAdd:
            os << "RS2::UndoAdd";
            break;
        case RS2::UndoDel:
            os << "RS2::UndoDel";
            break;
    }*/
        os << "   Undoable ids: ";
        for (RS_Undoable* u=i.getFirstUndoable();
                u!=NULL; u=i.getNextUndoable()) {

            if (u->undoRtti()==RS2::UndoableEntity) {
                RS_Entity* e = (RS_Entity*)u;
                os << e->getId() << (u->isUndone() ? "*" : "") << " ";
            } else {
                os << "|";
            }
        }

        return os;
    }

    friend class RS_Undo;

private:
    //! Undo type:
    //RS2::UndoType type;
    //! List of entity id's that were affected by this action
    RS_PtrList<RS_Undoable> undoables;
};

#endif
