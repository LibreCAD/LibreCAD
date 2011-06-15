/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#include "rs_debug.h"
#include "rs_blocklist.h"
#include "rs_block.h"
#include "rs_blocklistlistener.h"

/**
 * Constructor.
 * 
 * @param owner true if this is the owner of the blocks added.
 *              If so, the blocks will be deleted when the block
 *              list is deleted.
 */
RS_BlockList::RS_BlockList(bool owner) {
    this->owner = owner;
    //blocks.setAutoDelete(owner);
    activeBlock = NULL;
	setModified(false);
}


/**
 * Removes all blocks in the blocklist.
 */
void RS_BlockList::clear() {
    blocks.clear();
	setModified(true);
}


/**
 * Activates the given block.
 * Listeners are notified.
 */
void RS_BlockList::activate(const QString& name) {
    RS_DEBUG->print("RS_BlockList::activateBlock");

    activate(find(name));
}

/**
 * Activates the given block.
 * Listeners are notified.
 */
void RS_BlockList::activate(RS_Block* block) {
    RS_DEBUG->print("RS_BlockList::activateBlock");
    activeBlock = block;

    /*
       for (uint i=0; i<blockListListeners.count(); ++i) {
           RS_BlockListListener* l = blockListListeners.at(i);
    	if (l!=NULL) {
           	l->blockActivated(activeBlock);
    	}
       }
    */
}


/**
 * Adds a block to the block list. If a block with the same name
 * exists already, the given block will be deleted if the blocklist
 * owns the blocks.
 *
 * @param notify true if you want listeners to be notified.
 *
 * @return false: block already existed and was deleted.
 */
bool RS_BlockList::add(RS_Block* block, bool notify) {
    RS_DEBUG->print("RS_BlockList::add()");

    if (block==NULL) {
        return false;
    }

    // check if block already exists:
    RS_Block* b = find(block->getName());
    if (b==NULL) {
        blocks.append(block);

        if (notify) {
            addNotification();
        }
		setModified(true);

		return true;
    } else {
        if (owner) {
            delete block;
            block = NULL;
        }
		return false;
    }

}



/**
 * Notifies the listeners about blocks that were added. This can be
 * used after adding a lot of blocks without auto-update or simply
 * to force an update of GUI blocklists.
 */
void RS_BlockList::addNotification() {
    for (int i=0; i<blockListListeners.size(); ++i) {
        RS_BlockListListener* l = blockListListeners.at(i);
        l->blockAdded(NULL);
    }
}



/**
 * Removes a block from the list.
 * Listeners are notified after the block was removed from 
 * the list but before it gets deleted.
 */
void RS_BlockList::remove(RS_Block* block) {
    RS_DEBUG->print("RS_BlockList::removeBlock()");

    // here the block is removed from the list but not deleted
    blocks.removeOne(block);

    for (int i=0; i<blockListListeners.size(); ++i) {
        RS_BlockListListener* l = blockListListeners.at(i);
        l->blockRemoved(block);
    }
		
	setModified(true);

    // / *
    // activate an other block if necessary:
    if (activeBlock==block) {
    	//activate(blocks.first());
		activeBlock = NULL;
	}
    // * /

    // now it's save to delete the block
    if (owner) {
        delete block;
    }
}



/**
 * Tries to rename the given block to 'name'. Block names are unique in the
 * block list.
 *
 * @retval true block was successfully renamed.
 * @retval false block couldn't be renamed.
 */
bool RS_BlockList::rename(RS_Block* block, const QString& name) {
	if (block!=NULL) {
		if (find(name)==NULL) {
			block->setName(name);
			setModified(true);
			return true;
		}
	}

	return false;
}


/**
 * Changes a block's attributes. The attributes of block 'block'
 * are copied from block 'source'.
 * Listeners are notified.
 */
/*
void RS_BlockList::editBlock(RS_Block* block, const RS_Block& source) {
	*block = source;
	
	for (uint i=0; i<blockListListeners.count(); ++i) {
		RS_BlockListListener* l = blockListListeners.at(i);
 
		l->blockEdited(block);
	}
}
*/



/**
 * @return Pointer to the block with the given name or
 * \p NULL if no such block was found.
 */
RS_Block* RS_BlockList::find(const QString& name) {
    //RS_DEBUG->print("RS_BlockList::find");
	RS_Block* ret = NULL;

    for (int i=0; i<count(); ++i) {
        RS_Block* b = at(i);
        if (b->getName()==name) {
            ret=b;
			break;
        }
    }

    return ret;
}



/**
 * Finds a new unique block name.
 *
 * @param suggestion Suggested name the new name will be based on.
 */
QString RS_BlockList::newName(const QString& suggestion) {
    QString name;
    for (int i=0; i<1e5; ++i) {
        name = QString("%1-%2").arg(suggestion).arg(i);
        if (find(name)==NULL) {
            return name;
        }
    }

    return "0";
}



/**
 * Switches on / off the given block. 
 * Listeners are notified.
 */
void RS_BlockList::toggle(const QString& name) {
    toggle(find(name));
}



/**
 * Switches on / off the given block. 
 * Listeners are notified.
 */
void RS_BlockList::toggle(RS_Block* block) {
    if (block==NULL) {
        return;
    }

    block->toggle();

    // Notify listeners:
    for (int i=0; i<blockListListeners.size(); ++i) {
        RS_BlockListListener* l = blockListListeners.at(i);

        l->blockToggled(block);
    }
}



/**
 * Freezes or defreezes all blocks.
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_BlockList::freezeAll(bool freeze) {

    for (int l=0; l<count(); l++) {
        at(l)->freeze(freeze);
    }

    for (int i=0; i<blockListListeners.size(); ++i) {
        RS_BlockListListener* l = blockListListeners.at(i);
        l->blockToggled(NULL);
    }
}



/**
 * Switches on / off the given block. 
 * Listeners are notified.
 */
/*
void RS_BlockList::toggleBlock(const RS_String& name) {
	RS_Block* block = findBlock(name);
	block->toggle();
	
    // Notify listeners:
	for (uint i=0; i<blockListListeners.count(); ++i) {
		RS_BlockListListener* l = blockListListeners.at(i);
 
		l->blockToggled(block);
	}
}
*/


/**
 * adds a BlockListListener to the list of listeners. Listeners
 * are notified when the block list changes.
 */
void RS_BlockList::addListener(RS_BlockListListener* listener) {
    blockListListeners.append(listener);
}



/**
 * removes a BlockListListener from the list of listeners. 
 */
void RS_BlockList::removeListener(RS_BlockListListener* listener) {
    blockListListeners.removeOne(listener);
}



/**
 * Dumps the blocks to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_BlockList& b) {

    os << "Blocklist: \n";
    for (int i=0; i<b.count(); ++i) {
        RS_Block* blk = b.at(i);

        os << *blk << "\n";
    }

    return os;
}

