/****************************************************************************
This file is part of the LibreCAD project, a 2D CAD program

** Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "rs_actionblockssave.h"

#include <memory>
#include <unordered_set>

#include <QApplication>

#include "lc_containertraverser.h"
#include "lc_documentsstorage.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_blockwidget.h"
#include "qg_filedialog.h"
#include "rs_block.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_insert.h"

class RS_Block;
// fixme - sand - files - refactor action, move the logic outside as it might be reused
RS_ActionBlocksSave::RS_ActionBlocksSave(LC_ActionContext *actionContext)
        :RS_ActionInterface("Edit Block", actionContext, RS2::ActionBlocksSave) {}

namespace {
/**
 * Lists on @p target the block @p insert inserts, and the blocks that block
 * inserts in turn. @p insert must still belong to the source drawing: that is
 * where its block name resolves. @p done holds the blocks already listed, which
 * also ends a cycle of blocks that insert each other.
 */
void addInsertedBlocks(const RS_Insert* insert, RS_Graphic* target, std::unordered_set<const RS_Block*>& done) {
    RS_Block* block = insert->getBlockForInsert();
    if (block == nullptr || !done.insert(block).second) {
        return;
    }
    target->addBlock(block, false);
    for (const RS_Entity* e : *block) {
        if (e != nullptr && e->rtti() == RS2::EntityInsert && !e->getFlag(RS2::FlagDeleted)) {
            addInsertedBlocks(static_cast<const RS_Insert*>(e), target, done);
        }
    }
}
}

// fixme - sand - investigate why layers from this block are not added to graphic..
std::unique_ptr<RS_Graphic> RS_ActionBlocksSave::createGraphicForBlock(const RS_Block *activeBlock){
    auto result = std::make_unique<RS_Graphic>();
    // The blocks the entities insert stay the source drawing's: they are listed
    // here so the file gets them, but this list must not free them.
    result->getBlockList()->setOwner(false);
    result->clearLayers();

    std::unordered_set<const RS_Block*> insertedBlocks;
    for (const RS_Entity* e : lc::LC_ContainerTraverser{*activeBlock, RS2::ResolveNone}.entities()) {
        if (e->getFlag(RS2::FlagDeleted)) {
            continue; // deleted in the block editor, kept in the list as undo history
        }
        // A copy: this drawing is deleted as soon as the file is written, and
        // adding the block's own entity would re-parent it to this drawing.
        if (RS_Entity* copy = e->clone()) {
            result->addEntity(copy);
        }
        if (e->rtti() == RS2::EntityInsert) {
            // through the original, whose block name resolves in the source drawing
            addInsertedBlocks(static_cast<const RS_Insert*>(e), result.get(), insertedBlocks);
        }
    }
    return result;
}

void RS_ActionBlocksSave::trigger() {
    const auto& appWindow = QC_ApplicationWindow::getAppWindow();
    if(appWindow == nullptr) {
        finish();
        return;
    }
    const RS_BlockList* blockList = appWindow->getBlockWidget() -> getBlockList();
    if (blockList != nullptr) {
        const auto activeBlock= blockList->getActive();
        if(activeBlock != nullptr) {
            RS2::FormatType format = RS2::FormatDXFRW;
            QG_FileDialog dlg(appWindow->getCurrentMDIWindow(), {}, QG_FileDialog::BlockFile);
            const QString fileName = dlg.getSaveFile(&format, activeBlock->getName());
            if (fileName.isEmpty()) {
                // canceled, do nothing
            }
            else {
                QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
                const std::unique_ptr<RS_Graphic> graphic = createGraphicForBlock(activeBlock);
                graphic->setModified(true);

                LC_DocumentsStorage storage;
                storage.saveBlockAs(graphic.get(), fileName);

                QApplication::restoreOverrideCursor();
            }
        } else {
            commandMessage(tr("No block activated to save"));
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_ActionBlocksSave::trigger():  blockList is NULL");
    }
    finish();
}

void RS_ActionBlocksSave::init(const int status) {
    RS_ActionInterface::init(status);
    trigger();
}
