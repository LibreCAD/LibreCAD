/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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


#ifndef RS_UNDOLISTITEM_H
#define RS_UNDOLISTITEM_H

#include <iosfwd>
#include <set>

#include "rs_entity.h"
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
	RS_UndoCycle(/*RS2::UndoType type*/)=default;

    /**
     * Adds an Undoable to this Undo Cycle. Every Cycle can contain one or
     * more Undoables.
     */
    void addUndoable(RS_Undoable* u);

    /**
     * Removes an undoable from the list.
     */
    void removeUndoable(RS_Undoable* u);

    /**
     * Return number of undoables in cycle
     */
    size_t size(void);


    //! change undo state of all undoable in the current cycle
    void changeUndoState();

    friend std::ostream& operator << (std::ostream& os, RS_UndoCycle& uc);

    friend class RS_Undo;

    std::set<RS_Undoable*> const& getUndoables() const;

private:
    //! Undo type:
    //RS2::UndoType type;
    //! List of entity id's that were affected by this action
    std::set<RS_Undoable*> undoables;
};

#endif
