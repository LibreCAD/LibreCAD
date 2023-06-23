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

#include<iostream>
#include "rs_block.h"

#include "rs_graphic.h"
#include "rs_insert.h"

RS_BlockData::RS_BlockData(const QString& _name,
						   const RS_Vector& _basePoint,
						   bool _frozen):
	name(_name)
  ,basePoint(_basePoint)
  ,frozen(_frozen)
{
}

bool RS_BlockData::isValid() const{
	return (!name.isEmpty() && basePoint.valid);
}

/**
 * @param parent The graphic this block belongs to.
 * @param name The name of the block used as an identifier.
 * @param basePoint Base point (offset) of the block.
 */
RS_Block::RS_Block(RS_EntityContainer* parent,
                   const RS_BlockData& d)
        : RS_Document(parent), data(d) {

    pen = RS_Pen(RS_Color(128,128,128), RS2::Width01, RS2::SolidLine);
}


RS_Entity* RS_Block::clone() const {
    RS_Block* blk = new RS_Block(*this);
    blk->setOwner(isOwner());
    blk->detach();
    blk->initId();
    return blk;
}



RS_LayerList* RS_Block::getLayerList() {
    RS_Graphic* g = getGraphic();
    if (g) {
        return g->getLayerList();
    } else {
        return NULL;
    }
}



RS_BlockList* RS_Block::getBlockList() {
    RS_Graphic* g = getGraphic();
    if (g) {
        return g->getBlockList();
    } else {
        return NULL;
    }
}


bool RS_Block::save(bool isAutoSave) {
    RS_Graphic* g = getGraphic();
    if (g) {
        return g->save(isAutoSave);
    } else {
        return false;
    }
}


bool RS_Block::saveAs(const QString& filename, RS2::FormatType type, bool force) {
    RS_Graphic* g = getGraphic();
    if (g) {
        return g->saveAs(filename, type, force);
    } else {
        return false;
    }
}



/**
 * Sets the parent documents modified status to 'm'.
 */
void RS_Block::setModified(bool m) {
    RS_Graphic* p = getGraphic();
    if (p) {
        p->setModified(m);
    }
    modified = m;
}


/**
 * Sets the visibility of the Block in block list
 *
 * @param v true: visible, false: invisible
 */
void RS_Block::visibleInBlockList(bool v) {
    data.visibleInBlockList = v;
}


/**
 * Returns the visibility of the Block in block list
 */
bool RS_Block::isVisibleInBlockList() const {
    return data.visibleInBlockList;
}


/**
 * Sets selection state of the block in block list
 *
 * @param v true: selected, false: deselected
 */
void RS_Block::selectedInBlockList(bool v) const {
    data.selectedInBlockList = v;
}

/**
 * Returns selection state of the block in block list
 */
bool RS_Block::isSelectedInBlockList() const {
    return data.selectedInBlockList;
}

/**
 * Block may contain inserts of other blocks.
 * Find name of the nested block that contain the insert
 * of specified block.
 *
 * @param bName name of the block the nested insert references to
 *
 * @return block name chain to the block that contain searched insert
 */
QStringList RS_Block::findNestedInsert(const QString& bName) {

    QStringList bnChain;

    for (RS_Entity* e: entities) {
        if (e->rtti()==RS2::EntityInsert) {
            RS_Insert* i = ((RS_Insert*)e);
            QString iName = i->getName();
            if (iName == bName) {
                bnChain << data.name;
                break;
            } else {
                RS_BlockList* bList = getBlockList();
                if (bList) {
                    RS_Block* nestedBlock = bList->find(iName);
                    if (nestedBlock) {
                        QStringList nestedChain;
                        nestedChain = nestedBlock->findNestedInsert(bName);
                        if (!nestedChain.empty()) {
                            bnChain << data.name;
                            bnChain << nestedChain;
                            break;
                        }
                    }
                }
            }
        }
    }

    return bnChain;
}

std::ostream& operator << (std::ostream& os, const RS_Block& b) {
    os << " name: " << b.getName().toLatin1().data() << "\n";
    os << " entities: " << (RS_EntityContainer&)b << "\n";
    return os;
}
