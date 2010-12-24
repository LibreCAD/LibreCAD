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


#include "rs_undo.h"



/**
 * Default constructor.
 */
RS_Undo::RS_Undo() {
    undoList.setAutoDelete(true);
    undoPointer = -1;
    currentCycle = NULL;
}



/**
 * @return Number of Cycles that can be undone.
 */
int RS_Undo::countUndoCycles() {
    RS_DEBUG->print("RS_Undo::countUndoCycles");

    return undoPointer+1;
}



/**
 * @return Number of Cycles that can be redone.
 */
int RS_Undo::countRedoCycles() {
    RS_DEBUG->print("RS_Undo::countRedoCycles");

    return (int)undoList.count()-1-undoPointer;
}



/**
 * Adds an Undo Cycle at the current position in the list.
 * All Cycles after the new one are removed and the Undoabels
 * on them deleted.
 */
void RS_Undo::addUndoCycle(RS_UndoCycle* i) {
    RS_DEBUG->print("RS_Undo::addUndoCycle");
	
    undoList.insert(++undoPointer, i);
	
    RS_DEBUG->print("RS_Undo::addUndoCycle: ok");
}



/**
 * Starts a new cycle for one undo step. Every undoable that is
 * added after calling this method goes into this cycle.
 */
void RS_Undo::startUndoCycle() {
    RS_DEBUG->print("RS_Undo::startUndoCycle");

    // definitely delete Undo Cycles and all Undoables in them
    //   that cannot be redone now:
    while ((int)undoList.count()>undoPointer+1 && (int)undoList.count()>0) {

        RS_UndoCycle* l = undoList.last();
        if (l!=NULL) {
            RS_Undoable* u=NULL;
            bool done = false;
            do {
                u = l->getFirstUndoable();
                if (u!=NULL) {
                    // Remove the pointer from _all_ cycles:
					for (RS_UndoCycle* l2=undoList.first(); l2!=NULL; 
						l2=undoList.next()) {
						l2->removeUndoable(u);
					}

                    // Delete the Undoable for good:
                    if (u->isUndone()) {
                        removeUndoable(u);
                    }
                } else {
                    done = true;
                }
            } while(!done);
        }

        // Remove obsolete undo cycles:
        undoList.removeLast();
    }
	
    currentCycle = new RS_UndoCycle();
}



/**
 * Adds an undoable to the current undo cycle.
 */
void RS_Undo::addUndoable(RS_Undoable* u) {
    RS_DEBUG->print("RS_Undo::addUndoable");
	
    if (currentCycle!=NULL) {
        currentCycle->addUndoable(u);
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Undo::addUndoable(): No undo cycle active.");
    }
}



/**
 * Ends the current undo cycle.
 */
void RS_Undo::endUndoCycle() {
    addUndoCycle(currentCycle);
    currentCycle = NULL;
}



/**
 * Undoes the last undo cycle.
 */
void RS_Undo::undo() {
    RS_DEBUG->print("RS_Undo::undo");

    if (undoPointer>=0) {
        RS_UndoCycle* i = undoList.at(undoPointer);
        if (i!=NULL) {
            for (RS_Undoable* u=i->undoables.first();
                    u!=NULL; u=i->undoables.next()) {
                u->changeUndoState();
            }
            undoPointer--;
        }
    }
}



/**
 * Redoes the undo cycle which was at last undone.
 */
void RS_Undo::redo() {
    RS_DEBUG->print("RS_Undo::redo");

    if (undoPointer+1<(int)undoList.count()) {
        undoPointer++;
        RS_UndoCycle* i = undoList.at(undoPointer);
        if (i!=NULL) {
            for (RS_Undoable* u=i->undoables.first();
                    u!=NULL; u=i->undoables.next()) {
                u->changeUndoState();
            }
        }
    }
}



/**
 * @return The undo item that is next if we're about to undo
 * or NULL.
 */
RS_UndoCycle* RS_Undo::getUndoCycle() {
	RS_UndoCycle* ret = NULL;
	
    RS_DEBUG->print("RS_Undo::getUndoCycle");

    if (undoPointer>=0 && undoPointer<(int)undoList.count()) {
        ret = undoList.at(undoPointer);
    }
    RS_DEBUG->print("RS_Undo::getUndoCycle: OK");

    return ret;
}



/**
 * @return The redo item that is next if we're about to redo
 * or NULL.
 */
RS_UndoCycle* RS_Undo::getRedoCycle() {
    RS_DEBUG->print("RS_Undo::getRedoCycle");

    if (undoPointer+1>=0 && undoPointer+1<(int)undoList.count()) {
        return undoList.at(undoPointer+1);
    }

    return NULL;
}



/**
 * Dumps the undo list to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_Undo& l) {
    os << "Undo List: " <<  "\n";
    os << " Pointer is at: " << l.undoPointer << "\n";
    for (RS_UndoCycle* i = l.undoList.first();
            i!=NULL; i = l.undoList.next()) {

        if (l.undoList.at()==l.undoPointer) {
            os << " -->";
        }
		else {
            os << "    ";
		}
        os << *i << "\n";
    }
    return os;
}



/**
 * Testing Undoables, Undo Cycles and the Undo container.
 */
#ifdef RS_TEST
bool RS_Undo::test() {

    int i, k;
    RS_UndoStub undo;
    //RS_UndoCycle* c1;
    RS_Undoable* u1;

    std::cout << "Testing RS_Undo\n";

    std::cout << "  Adding 500 cycles..";
    // Add 500 Undo Cycles with i Undoables in every Cycle
    for (i=1; i<=500; ++i) {
        //c1 = new RS_UndoCycle();
		undo.startUndoCycle();
        for (k=1; k<=i; ++k) {
            u1 = new RS_Undoable();
            //c1->
			undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
		undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==500);
    assert(undo.countRedoCycles()==0);

    std::cout << "  Undo 500 cycles..";
    // Undo all 500 cycles
    for (i=1; i<=500; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==0);
    assert(undo.countRedoCycles()==500);

    std::cout << "  Redo 500 cycles..";
    // Redo all 500 cycles
    for (i=1; i<=500; ++i) {
        undo.redo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==500);
    assert(undo.countRedoCycles()==0);

    std::cout << "  Undo 250 cycles..";
    // Undo all 500 cycles
    for (i=1; i<=250; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==250);
    assert(undo.countRedoCycles()==250);

    std::cout << "  Adding 10 cycles..";
    for (i=1; i<=10; ++i) {
        //c1 = new RS_UndoCycle();
		undo.startUndoCycle();
        for (k=1; k<=10; ++k) {
            u1 = new RS_Undoable();
            //c1->addUndoable(u1);
			undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
		undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==260);
    assert(undo.countRedoCycles()==0);

    std::cout << "  Undo 5 cycles..";
    for (i=1; i<=5; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==255);
    assert(undo.countRedoCycles()==5);

    std::cout << "  Redo 5 cycles..";
    for (i=1; i<=5; ++i) {
        undo.redo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==260);
    assert(undo.countRedoCycles()==0);

    std::cout << "  Undo 15 cycles..";
    for (i=1; i<=15; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==245);
    assert(undo.countRedoCycles()==15);

    std::cout << "  Adding 1 cycle..";
    for (i=1; i<=1; ++i) {
        //c1 = new RS_UndoCycle();
		undo.startUndoCycle();
        for (k=1; k<=10; ++k) {
            u1 = new RS_Undoable();
            //c1->addUndoable(u1);
			undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
		undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles()==246);
    assert(undo.countRedoCycles()==0);

    return true;

}
#endif

