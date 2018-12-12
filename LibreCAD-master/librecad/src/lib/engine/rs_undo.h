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


#ifndef RS_UNDO_H
#define RS_UNDO_H

#include <memory>
#include <vector>

class RS_UndoCycle;
class RS_Undoable;

/**
 * Undo / redo functionality. The internal undo list consists of
 * RS_UndoCycle entries.
 *
 * @see RS_UndoCycle
 * @author Andrew Mustun
 */
class RS_Undo {
public:
	virtual ~RS_Undo() = default;

    virtual bool undo();
    virtual bool redo();

//	virtual std::shared_ptr<RS_UndoCycle> getUndoCycle();
//	virtual std::shared_ptr<RS_UndoCycle> getRedoCycle();

    virtual int countUndoCycles();
    virtual int countRedoCycles();
    virtual bool hasUndoable();

    virtual void startUndoCycle();
    virtual void addUndoable(RS_Undoable* u);
    virtual void endUndoCycle();

    /**
     * Must be overwritten by the implementing class and delete
     * the given Undoable (unrecoverable). This method is called
     * for Undoables that are no longer in the undo buffer.
     */
    virtual void removeUndoable(RS_Undoable* u) = 0;

    /**
	  *\brief enable/disable redo/undo buttons in main application window
	  *\author: Dongxu Li
      **/
	void setGUIButtons() const;

    friend std::ostream& operator << (std::ostream& os, RS_Undo& a);

    static bool test();

private:

	void addUndoCycle(std::shared_ptr<RS_UndoCycle> const& i);
    //! List of undo list items. every item is something that can be undone.
	std::vector<std::shared_ptr<RS_UndoCycle>> undoList;

    /**
     * Index that points to the current position in the undo list.
     * The item it points on will be undone the next time undo is called.
     * The item after will be redone (if there is an item) when redo
     * is called.
     */
	int undoPointer = -1;

    /**
     * Current undo cycle.
     */
    std::shared_ptr<RS_UndoCycle> currentCycle {nullptr};

    int refCount {0}; ///< reference counter for nested start/end calls
};


/**
 * Stub for testing the RS_Undo class.
 */
#ifdef RS_TEST
class RS_UndoStub : public RS_Undo {
    virtual void removeUndoable(RS_Undoable* u) {
        delete u;
        u = NULL;
    }
};
#endif

#endif

