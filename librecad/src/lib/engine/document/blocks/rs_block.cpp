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

#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

#include <QSet>

#include "rs_block.h"

#include "rs_blocklist.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"

namespace {

// Presentation-only color for BLOCK definition previews; it is not serialized
// BLOCK geometry and must not participate in INSERT transform decisions.
constexpr int kBlockPreviewGrayChannel = 128;

RS_Pen blockDefinitionPreviewPen() {
    return {RS_Color(kBlockPreviewGrayChannel, kBlockPreviewGrayChannel,
                     kBlockPreviewGrayChannel),
            RS2::Width01, RS2::SolidLine};
}

} // namespace

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
    setPen(blockDefinitionPreviewPen());
}

RS_Entity* RS_Block::clone() const {
    const auto blk = new RS_Block(getParent(), RS_BlockData(m_data));
    blk->setGraphicView(getGraphicView()); // fixme - remove this dependency

    for (const RS_Entity* entity : getEntityList()) {
        // The block editor edits this RS_Block as its own document: a
        // deletion there only sets the entity's own FlagDeleted and keeps it
        // in the list as undo history (RS_Document::undoableDelete). Skip
        // those here, the same way every writer already does, so a clone
        // does not resurrect them as live geometry. Check the entity's own
        // flag, not isDeleted(), which also follows the parent chain and
        // would make a clone of a deleted block come out empty.
        if (entity != nullptr && !entity->getFlag(RS2::FlagDeleted)) {
            RS_Entity* copy = entity->clone();
            copy->setParent(blk);
            blk->push_back(copy);
        }
    }
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

const RS_BlockList* RS_Block::getBlockList() const {
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
QStringList RS_Block::findNestedInsert(const QString& bName) const {
    struct BlockSearchFrame {
        const RS_Block* block = nullptr;
        std::size_t nextEntityIndex = 0U;
    };

    QSet<const RS_Block*> activeBlocks;
    std::vector<BlockSearchFrame> work;
    work.push_back({this, 0});
    activeBlocks.insert(this);

    while (!work.empty()) {
        BlockSearchFrame& frame = work.back();
        if (frame.block == nullptr
            || frame.nextEntityIndex >= static_cast<std::size_t>(frame.block->count())) {
            activeBlocks.remove(frame.block);
            work.pop_back();
            continue;
        }

        if (frame.nextEntityIndex > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            return {};
        const RS_Entity* entity = frame.block->entityAt(
            static_cast<int>(frame.nextEntityIndex++));
        if (entity == nullptr || entity->rtti() != RS2::EntityInsert)
            continue;

        const auto& insert = static_cast<const RS_Insert&>(*entity);
        if (insert.getName() == bName) {
            QStringList result;
            for (const BlockSearchFrame& pathFrame : work)
                result.push_back(pathFrame.block->m_data.name);
            return result;
        }

        const RS_Block* nestedBlock = insert.getBlockForInsert();
        if (nestedBlock == nullptr || activeBlocks.contains(nestedBlock))
            continue;
        activeBlocks.insert(nestedBlock);
        work.push_back({nestedBlock, 0});
    }
    return {};
}

void RS_Block::addByBlockLine(const RS_Vector& start, const RS_Vector& end) {
    addByBlockEntity(std::make_unique<RS_Line>(start, end));
}

void RS_Block::addByBlockEntity(std::unique_ptr<RS_Entity> entity) {
    if (entity == nullptr)
        return;
    addByBlockEntity(entity.release());
}

void RS_Block::addByBlockEntity(RS_Entity* entity) {
    if (entity == nullptr)
        return;
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
