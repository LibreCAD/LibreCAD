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

#include <set>
#include <iostream>
#include <QString>
#include <QRegExp>
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
	activeBlock = nullptr;
	setModified(false);
}


/**
 * Removes all blocks in the blocklist.
 */
void RS_BlockList::clear() {
    blocks.clear();
	activeBlock = nullptr;
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

	if (!block) {
        return false;
    }

    // check if block already exists:
    RS_Block* b = find(block->getName());
	if (!b) {
        blocks.append(block);

        if (notify) {
            addNotification();
        }
		setModified(true);

		return true;
    } else {
        if (owner) {
            delete block;
			block = nullptr;
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
	for(auto l: blockListListeners){
		l->blockAdded(nullptr);
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

	for(auto l: blockListListeners){
		l->blockRemoved(block);
	}
		
	setModified(true);

    // / *
    // activate an other block if necessary:
    if (activeBlock==block) {
    	//activate(blocks.first());
		activeBlock = nullptr;
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
	if (block) {
		if (!find(name)) {
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
	
    for (unsigned i=0; i<blockListListeners.count(); ++i) {
		RS_BlockListListener* l = blockListListeners.at(i);
 
		l->blockEdited(block);
	}
}
*/

/**
 * @return Pointer to the block with the given name or
 * \p nullptr if no such block was found.
 */
RS_Block* RS_BlockList::find(const QString& name) {
    try {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_BlockList::find(): %s", name.toLatin1().constData());
    }
    catch(...) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_BlockList::find(): wrong name to find");
        return nullptr;
    }
	// Todo : reduce this from O(N) to O(log(N)) complexity based on sorted list or hash
	//DFS
	std::vector<RS_BlockList const*> nodes;
	std::set<RS_BlockList const*> searched;
	searched.insert(nullptr);
	nodes.push_back(this);
	while (nodes.size()) {
		auto list = nodes.back();
		nodes.pop_back();
        for (RS_Block* blk: *list) {
            if (blk->getName() == name) {
                RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_BlockList::find(): OK");
                return blk;
            }
            auto node = blk->getBlockList();
			if (!searched.count(node)) {
				searched.insert(list);
                nodes.push_back(blk->getBlockList());
			}
		}
	}
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_BlockList::find(): bad");
	return nullptr;
}

/**
 * Finds a new unique block name.
 *
 * @param suggestion Suggested name the new name will be based on.
 */
QString RS_BlockList::newName(const QString& suggestion) {
//	qDebug()<<"begin: suggestion: "<<suggestion;
	if(!find(suggestion))
		return suggestion;

	QString name=suggestion;
	QRegExp const rx(R"(-\d+$)");
	int index=name.lastIndexOf(rx);
	int i=-1;
	if(index>0){
		i=name.mid(index+1).toInt();
		name=name.mid(0, index);
	}
	QString ret = QString("%1-%2").arg(name).arg(i+1);
	RS_Block* b;
	while((b = find(ret))){
		index=b->getName().lastIndexOf(rx);
		if(index<0) continue;
		QString const part1= b->getName().mid(0, index);
		if(part1 != name) continue;
		i=std::max(b->getName().mid(index+1).toInt(),i);
		ret = QString("%1-%2").arg(name).arg(i+1);
	}
	return ret;
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
	if (!block) {
        return;
    }

    block->toggle();
    // TODO LordOfBikes: when block attributes are saved, activate this
    //setModified(true);

    // Notify listeners:
	for(auto l: blockListListeners){
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
        if (at(l)->isVisibleInBlockList()) {
            at(l)->freeze(freeze);
        }
    }
    // TODO LordOfBikes: when block attributes are saved, activate this
    //setModified(true);

	for(auto l: blockListListeners){
		l->blockToggled(nullptr);
	}
}


/**
 * Switches on / off the given block. 
 * Listeners are notified.
 */
/*
void RS_BlockList::toggleBlock(const QString& name) {
	RS_Block* block = findBlock(name);
	block->toggle();
	
    // Notify listeners:
    for (unsigned i=0; i<blockListListeners.count(); ++i) {
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

int RS_BlockList::count() const{
	return blocks.count();
}

/**
 * @return Block at given position or nullptr if i is out of range.
 */
RS_Block* RS_BlockList::at(int i) {
	return blocks.at(i);
}
RS_Block* RS_BlockList::at(int i) const{
	return blocks.at(i);
}
QList<RS_Block*>::iterator RS_BlockList::begin()
{
	return blocks.begin();
}

QList<RS_Block*>::iterator RS_BlockList::end()
{
	return blocks.end();
}

QList<RS_Block*>::const_iterator RS_BlockList::begin()const
{
	return blocks.begin();
}

QList<RS_Block*>::const_iterator RS_BlockList::end()const
{
	return blocks.end();
}

//! @return The active block of nullptr if no block is activated.
RS_Block* RS_BlockList::getActive() {
	return activeBlock;
}

/**
 * Sets the block list modified status to 'm'.
 */
void RS_BlockList::setModified(bool m) {
	modified = m;

	// Update each block modified status,
	// but only when the status is set to false.
	if (m == false) {
		for (auto b: blocks) {
			b->setModifiedFlag(false);
		}
	}

	// Notify listeneres
	for (auto l: blockListListeners) {
		l->blockListModified(m);
	}
}

/**
 * @retval true The block list has been modified.
 * @retval false The block list has not been modified.
 */
bool RS_BlockList::isModified() const {
	return modified;
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

