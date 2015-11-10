#include <iostream>
#include"rs_undocycle.h"

/**
 * Adds an Undoable to this Undo Cycle. Every Cycle can contain one or
 * more Undoables.
 */
void RS_UndoCycle::addUndoable(RS_Undoable* u) {
	undoables.insert(u);
}

/**
 * Removes an undoable from the list.
 */
void RS_UndoCycle::removeUndoable(RS_Undoable* u) {
	undoables.erase(u);
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

