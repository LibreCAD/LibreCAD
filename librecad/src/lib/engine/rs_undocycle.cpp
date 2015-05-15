#include"rs_undocycle.h"

/**
 * Adds an Undoable to this Undo Cycle. Every Cycle can contain one or
 * more Undoables.
 */
void RS_UndoCycle::addUndoable(RS_Undoable* u) {
	undoables.append(u);
}

/**
 * Removes an undoable from the list.
 */
void RS_UndoCycle::removeUndoable(RS_Undoable* u) {
#if QT_VERSION < 0x040400
	emu_qt44_removeOne(undoables, u);
#else
	undoables.removeOne(u);
#endif

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
	for (int i = 0; i < uc.undoables.size(); ++i) {
		RS_Undoable *u = uc.undoables.at(i);
		if (u->undoRtti()==RS2::UndoableEntity) {
			RS_Entity* e = (RS_Entity*)u;
			os << e->getId() << (u->isUndone() ? "*" : "") << " ";
		} else {
			os << "|";
		}

	}

	return os;
}

