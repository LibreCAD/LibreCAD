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


#include <ostream>
#include"rs_undocycle.h"

/**
 * Adds an Undoable to this Undo Cycle. Every Cycle can contain one or
 * more Undoables.
 */
void RS_UndoCycle::addUndoable(RS_Undoable* u) {
    if (!u)
        return;

    undoables.insert(u);
}

/**
 * Removes an undoable from the list.
 */
void RS_UndoCycle::removeUndoable(RS_Undoable* u) {
    if (!u)
        return;

    undoables.erase(u);
}

/**
 * Return number of undoables in cycle
 */
size_t RS_UndoCycle::size()
{
    return undoables.size();
}

void RS_UndoCycle::changeUndoState()
{
	for (RS_Undoable* u: undoables)
		u->changeUndoState();
}

std::set<RS_Undoable*> const& RS_UndoCycle::getUndoables() const
{
    return undoables;
}


std::ostream& operator << (std::ostream& os,
								  RS_UndoCycle& uc) {
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
	for (auto u: uc.undoables) {
		if (u->undoRtti()==RS2::UndoableEntity) {
			RS_Entity* e = (RS_Entity*)u;
			os << e->getId() << (u->isUndone() ? "*" : "") << " ";
		} else {
			os << "|";
		}

	}

	return os;
}

