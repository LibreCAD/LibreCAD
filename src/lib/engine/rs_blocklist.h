/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#ifndef RS_BLOCKLIST_H
#define RS_BLOCKLIST_H


#include "rs_block.h"
#include "rs_blocklistlistener.h"
#include "rs_entity.h"
#include "rs_ptrlist.h"

/**
 * List of blocks.
 *
 * @see RS_Block
 *
 * @author Andrew Mustun
 */
class RS_BlockList {
public:
    RS_BlockList(bool owner=false);
    virtual ~RS_BlockList() {}

    void clear();
    /**
     * @return Number of blocks available.
     */
    uint count() {
        return blocks.count();
    }

    /**
     * @return Block at given position or NULL if i is out of range.
     */
    RS_Block* at(uint i) {
        return blocks.at(i);
    }

    void activate(const RS_String& name);
    void activate(RS_Block* block);
    //! @return The active block of NULL if no block is activated.
    RS_Block* getActive() {
        return activeBlock;
    }

    virtual bool add(RS_Block* block, bool notify=true);
    virtual void addNotification();
    virtual void remove(RS_Block* block);
    virtual bool rename(RS_Block* block, const RS_String& name);
    //virtual void editBlock(RS_Block* block, const RS_Block& source);
    RS_Block* find(const RS_String& name);
    RS_String newName(const RS_String& suggestion = "");
    void toggle(const RS_String& name);
    void toggle(RS_Block* block);
    void freezeAll(bool freeze);

    /**
     * @return First block of the list.
     */
    //RS_Block* firstBlock() {
    //    return blocks.first();
    //}

    /**
     * @return Next block from the list after 
     * calling firstBlock() or nextBlock().
     */
    //RS_Block* nextBlock() {
    //    return blocks.next();
    //}

    void addListener(RS_BlockListListener* listener);
    void removeListener(RS_BlockListListener* listener);
	
	/**
	 * Sets the layer lists modified status to 'm'.
	 */
	void setModified(bool m) {
		modified = m;
	}
	
	/**
	 * @retval true The layer list has been modified.
	 * @retval false The layer list has not been modified.
	 */
    virtual bool isModified() const {
        return modified;
    }

    friend std::ostream& operator << (std::ostream& os, RS_BlockList& b);

private:
    //! Is the list owning the blocks?
    bool owner;
    //! Blocks in the graphic
    RS_PtrList<RS_Block> blocks;
    //! List of registered BlockListListeners
    RS_PtrList<RS_BlockListListener> blockListListeners;
    //! Currently active block
    RS_Block* activeBlock;
    /** Flag set if the layer list was modified and not yet saved. */
    bool modified;
};

#endif
