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

#include "rs_block.h"

#include<iostream>

#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

RS_BlockData::RS_BlockData(const QString& name, const RS_Vector& basePoint, const bool frozen) : name(name), basePoint(basePoint),
    frozen(frozen) {
}

bool RS_BlockData::isValid() const {
    return !name.isEmpty() && basePoint.valid;
}

/**
 * @param parent The graphic this block belongs to.
 * @param blockData
 */
RS_Block::RS_Block(RS_EntityContainer* parent, const RS_BlockData& blockData)
    : RS_Document(parent), m_data(blockData) {
    setPen({RS_Color(128, 128, 128), RS2::Width01, RS2::SolidLine});
}

RS_Entity* RS_Block::clone() const {
    const auto blk = new RS_Block(getParent(), RS_BlockData(m_data));
    blk->setGraphicView(getGraphicView()); // fixme - remove this dependency

    if (isOwner()) {
        const auto entitylist = getEntityList();
        for (const RS_Entity* entity : entitylist) {
            if (entity != nullptr) {
                blk->push_back(entity->clone());
            }
        }
    }
    else {
        const auto entitylist = getEntityList();
        for (RS_Entity* entity : entitylist) {
            if (entity != nullptr) {
                blk->push_back(entity);
            }
        }
    }
    blk->detach();
    return blk;
}

RS_LayerList* RS_Block::getLayerList() {
    RS_Graphic* g = getGraphic();
    return (g != nullptr) ? g->getLayerList() : nullptr;
}

RS_BlockList* RS_Block::getBlockList() {
    RS_Graphic* g = getGraphic();
    return (g != nullptr) ? g->getBlockList() : nullptr;
}

LC_DimStylesList* RS_Block::getDimStyleList() {
    RS_Graphic* g = getGraphic();
    return (g != nullptr) ? g->getDimStyleList() : nullptr;
}

LC_TextStyleList* RS_Block::getTextStyleList() {
    RS_Graphic* g = getGraphic();
    return (g != nullptr) ? g->getTextStyleList() : nullptr;
}

// bool RS_Block::save(bool isAutoSave) {
//     RS_Graphic* g = getGraphic();
//     if (g) {
//         return g->save(isAutoSave);
//     } else {
//         return false;
//     }
// }

// bool RS_Block::saveAs(const QString& filename, RS2::FormatType type, bool force) {
//     RS_Graphic* g = getGraphic();
//     if (g) {
//         return g->saveAs(filename, type, force);
//     } else {
//         return false;
//     }
// }

bool RS_Block::isVisible() const {
    if (!getFlag(RS2::FlagVisible)) {
        return false;
    }

    if (isDeleted()) {
        return false;
    }
    return true;
}

/**
 * Sets the parent documents modified status to 'm'.
 */
void RS_Block::setModified(const bool m) {
    RS_Graphic* p = getGraphic();
    if (p != nullptr) {
        p->setModified(m);
    }
    m_modified = m;
}

/**
 * Sets the visibility of the Block in block list
 *
 * @param v true: visible, false: invisible
 */
void RS_Block::visibleInBlockList(const bool v) const {
    m_data.visibleInBlockList = v;
}

/**
 * Returns the visibility of the Block in block list
 */
bool RS_Block::isVisibleInBlockList() const {
    return m_data.visibleInBlockList;
}

/**
 * Sets selection state of the block in block list
 *
 * @param v true: selected, false: deselected
 */
void RS_Block::selectedInBlockList(const bool v) const {
    m_data.selectedInBlockList = v;
}

/**
 * Returns selection state of the block in block list
 */
bool RS_Block::isSelectedInBlockList() const {
    return m_data.selectedInBlockList;
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

    for (RS_Entity* e : *this) {
        if (e->rtti() == RS2::EntityInsert) {
            const auto i = static_cast<RS_Insert*>(e);
            QString iName = i->getName();
            if (iName == bName) {
                bnChain << m_data.name;
                break;
            }
            RS_BlockList* bList = getBlockList();
            if (bList != nullptr) {
                RS_Block* nestedBlock = bList->find(iName);
                if (nestedBlock != nullptr) {
                    QStringList nestedChain;
                    nestedChain = nestedBlock->findNestedInsert(bName);
                    if (!nestedChain.empty()) {
                        bnChain << m_data.name;
                        bnChain << nestedChain;
                        break;
                    }
                }
            }
        }
    }

    return bnChain;
}

void RS_Block::addByBlockLine(const RS_Vector& start, const RS_Vector& end) {
    addByBlockEntity(new RS_Line(start, end));
}

void RS_Block::addByBlockEntity(const RS_Entity* entity) {
    const RS_Pen byBlockPen(RS2::FlagByBlock, RS2::WidthByBlock, RS2::LineByBlock);
    entity->setPen(byBlockPen);
    addEntity(entity);
}

std::ostream& operator <<(std::ostream& os, const RS_Block& b) {
    os << " name: " << b.getName().toLatin1().data() << "\n";
    auto asContainer = static_cast<const RS_EntityContainer&>(b);
    os << " entities: " << asContainer << "\n";
    return os;
}
