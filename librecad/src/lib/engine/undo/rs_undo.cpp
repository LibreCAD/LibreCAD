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

#include "rs_undo.h"

#include<iostream>
#include <qassert.h>
#include <unordered_set>

#include "rs_debug.h"
#include "rs_undocycle.h"

/**
 * @return Number of Cycles that can be undone.
 */
int RS_Undo::countUndoCycles() {
    RS_DEBUG->print("RS_Undo::countUndoCycles");

    return std::distance(m_undoList.cbegin(), m_redoPointer);
}

/**
 * @return Number of Cycles that can be redone.
 */
int RS_Undo::countRedoCycles() {
    RS_DEBUG->print("RS_Undo::countRedoCycles");

    return std::distance(m_redoPointer, m_undoList.cend());
}

/**
 * @return true, when current undo cycle has at least one undoable
 */
bool RS_Undo::hasUndoable() {
    return nullptr != m_currentCycle && !m_currentCycle->empty();
}

/**
 * Adds an Undo Cycle at the current position in the list.
 * All Cycles after the new one are removed and the Undoabels
 * on them deleted.
 */
void RS_Undo::addUndoCycle(std::shared_ptr<RS_UndoCycle> undoCycle) {
    RS_DEBUG->print("RS_Undo::addUndoCycle");

    m_undoList.push_back(std::move(undoCycle));
    m_redoPointer = m_undoList.cend();

    RS_DEBUG->print("RS_Undo::addUndoCycle: ok");
}

/**
 * Starts a new cycle for one undo step. Every undoable that is
 * added after calling this method goes into this cycle.
 */
void RS_Undo::startUndoCycle() {
    if (1 < ++m_refCount) {
        // only the first fresh top call starts a new cycle
        return;
    }

    // anything after the current existing undoCycle will be removed
    // if there are undo cycles behind undoPointer
    // remove obsolete entities and undoCycles
    if (m_undoList.cend() != m_redoPointer) {
        // collect remaining undoables
        std::unordered_set<RS_Undoable*> keep;
        for (auto it = m_undoList.begin(); it != m_redoPointer; ++it) {
            for (RS_Undoable* undoable : (*it)->getUndoables()) {
                keep.insert(undoable);
            }
        }

        // collect obsolete undoables
        std::unordered_set<RS_Undoable*> obsolete;
        for (auto it = m_redoPointer; it != m_undoList.end(); ++it) {
            for (RS_Undoable* undoable : (*it)->getUndoables()) {
                obsolete.insert(undoable);
            }
        }

        if (!obsolete.empty()) {
            // delete obsolete undoables which are not in keep list
            startBulkUndoablesCleanup(); // avoid document borders recalculation on each remove
            for (RS_Undoable* undoable : obsolete) {
                if (keep.end() == keep.find(undoable)) {
                    removeUndoable(undoable);
                }
            }
            endBulkUndoablesCleanup();
        }
        // clean up obsolete undoCycles
        m_undoList.erase(m_redoPointer, m_undoList.cend());
        m_redoPointer = m_undoList.cend();
    }

    // alloc new undoCycle
    m_currentCycle = std::make_shared<RS_UndoCycle>();
}

/**
 * Adds an undoable to the current undo cycle.
 */
void RS_Undo::addUndoable(RS_Undoable* u) {
    RS_DEBUG->print("RS_Undo::%s(): begin", __func__);
    Q_ASSERT(m_currentCycle != nullptr);
    if (nullptr == m_currentCycle) {
        RS_DEBUG->print(RS_Debug::D_CRITICAL, "RS_Undo::%s(): invalid currentCycle, possibly missing startUndoCycle()", __func__);
        return;
    }
    m_currentCycle->addUndoable(u);
    RS_DEBUG->print("RS_Undo::%s(): end", __func__);
}

/**
 * Ends the current undo cycle.
 */
void RS_Undo::endUndoCycle() {
    if (0 < m_refCount) {
        // compensate nested calls of start-/endUndoCycle()
        if (0 < --m_refCount) {
            // not the final nested call, nothing to do yet
            return;
        }
    }
    else {
        RS_DEBUG->print(RS_Debug::D_WARNING, "Warning: RS_Undo::endUndoCycle() called without previous startUndoCycle()  %d", m_refCount);
        return;
    }

    if (hasUndoable()) {
        // only keep the undoCycle, when it contains undoables
        addUndoCycle(m_currentCycle);
    }
    updateUndoState();
    m_currentCycle.reset(); // invalidate currentCycle for next startUndoCycle()
}

/**
 * Undoes the last undo cycle.
 */
bool RS_Undo::undo() {
    RS_DEBUG->print("RS_Undo::undo");

    if (m_redoPointer == m_undoList.cbegin()) {
        return false;
    }

    m_redoPointer = std::prev(m_redoPointer);
    const std::shared_ptr<RS_UndoCycle> uc = *m_redoPointer;

    updateUndoState();
    uc->changeUndoState();
    return true;
}

/**
 * Redoes the undo cycle which was at last undone.
 */
bool RS_Undo::redo() {
    RS_DEBUG->print("RS_Undo::redo");

    if (m_redoPointer != m_undoList.cend()) {
        const std::shared_ptr<RS_UndoCycle> uc = *m_redoPointer;
        m_redoPointer = std::next(m_redoPointer);

        updateUndoState();
        uc->changeUndoState();
        return true;
    }
    return false;
}

/**
  * enable/disable redo/undo buttons in main application window
  * Author: Dongxu Li
  **/
void RS_Undo::updateUndoState() const {
    const bool redoAvailable = m_redoPointer != m_undoList.end();
    const bool undoAvailable = !m_undoList.empty() && m_redoPointer != m_undoList.cbegin();
    fireUndoStateChanged(undoAvailable, redoAvailable);
}

void RS_Undo::collectUndoState(bool& undoAvailable, bool& redoAvailable) const {
    redoAvailable = m_redoPointer != m_undoList.end();
    undoAvailable = !m_undoList.empty() && m_redoPointer != m_undoList.cbegin();
}

/**
 * Dumps the undo list to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Undo& l) {
    os << "Undo List: " << "\n";
    const int position = std::distance(l.m_undoList.cbegin(), l.m_redoPointer);
    os << " Redo Pointer is at: " << position << "\n";

    for (auto it = l.m_undoList.cbegin(); it != l.m_undoList.cend(); ++it) {
        os << ((it != l.m_redoPointer) ? "    " : " -->");
        os << *it << "\n";
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
    for (i = 1; i <= 500; ++i) {
        //c1 = new RS_UndoCycle();
        undo.startUndoCycle();
        for (k = 1; k <= i; ++k) {
            u1 = new RS_Undoable();
            //c1->
            undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
        undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 500);
    assert(undo.countRedoCycles() == 0);

    std::cout << "  Undo 500 cycles..";
    // Undo all 500 cycles
    for (i = 1; i <= 500; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 0);
    assert(undo.countRedoCycles() == 500);

    std::cout << "  Redo 500 cycles..";
    // Redo all 500 cycles
    for (i = 1; i <= 500; ++i) {
        undo.redo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 500);
    assert(undo.countRedoCycles() == 0);

    std::cout << "  Undo 250 cycles..";
    // Undo all 500 cycles
    for (i = 1; i <= 250; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 250);
    assert(undo.countRedoCycles() == 250);

    std::cout << "  Adding 10 cycles..";
    for (i = 1; i <= 10; ++i) {
        //c1 = new RS_UndoCycle();
        undo.startUndoCycle();
        for (k = 1; k <= 10; ++k) {
            u1 = new RS_Undoable();
            //c1->addUndoable(u1);
            undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
        undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 260);
    assert(undo.countRedoCycles() == 0);

    std::cout << "  Undo 5 cycles..";
    for (i = 1; i <= 5; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 255);
    assert(undo.countRedoCycles() == 5);

    std::cout << "  Redo 5 cycles..";
    for (i = 1; i <= 5; ++i) {
        undo.redo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 260);
    assert(undo.countRedoCycles() == 0);

    std::cout << "  Undo 15 cycles..";
    for (i = 1; i <= 15; ++i) {
        undo.undo();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 245);
    assert(undo.countRedoCycles() == 15);

    std::cout << "  Adding 1 cycle..";
    for (i = 1; i <= 1; ++i) {
        //c1 = new RS_UndoCycle();
        undo.startUndoCycle();
        for (k = 1; k <= 10; ++k) {
            u1 = new RS_Undoable();
            //c1->addUndoable(u1);
            undo.addUndoable(u1);
        }
        //undo.addUndoCycle(c1);
        undo.endUndoCycle();
    }
    std::cout << "OK\n";

    assert(undo.countUndoCycles() == 246);
    assert(undo.countRedoCycles() == 0);

    return true;
}
#endif
