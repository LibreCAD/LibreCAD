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

#include <QApplication>

#include "lc_documentsstorage.h"
#include "lc_containertraverser.h"
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

/*recursive add blocks in graphic*/
void RS_ActionBlocksSave::addBlock(RS_Insert *in, RS_Graphic *g) {
    for (auto e: *in) {
        if (e->rtti() == RS2::EntityInsert) {
            auto *insert = static_cast<RS_Insert *>(e);
            addBlock(insert, g);
            g->addBlock(insert->getBlockForInsert());
        }
    }
}

// fixme - sand - investigate why layers from this block are not added to graphic..
RS_Graphic* RS_ActionBlocksSave::createGraphicForBlock(RS_Block *activeBlock){
    auto* result = new RS_Graphic();
    result->setOwner(false);
    result->getBlockList()->setOwner(false);
    result->clearLayers();
    // g.addLayer(b->getLayer());

    for (RS_Entity* e : lc::LC_ContainerTraverser{*activeBlock, RS2::ResolveNone}.entities()) {
        result->addEntity(e);
        if (e->rtti() == RS2::EntityInsert) {
            auto *insert = static_cast<RS_Insert *>(e);
            result->addBlock(insert->getBlockForInsert());
            addBlock(insert,result);
        }
        // g.addLayer(e->getLayer());
    }
    return result;
}

void RS_ActionBlocksSave::trigger() {
    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    if(!appWindow) {
        finish(false);
        return;
    }
    RS_BlockList* blockList = appWindow->getBlockWidget() -> getBlockList();
    if (blockList != nullptr) {
        auto activeBlock= blockList->getActive();
        if(activeBlock != nullptr) {
            RS2::FormatType format = RS2::FormatDXFRW;
            QG_FileDialog dlg(appWindow->getCurrentMDIWindow(), {}, QG_FileDialog::BlockFile);
            const QString fileName = dlg.getSaveFile(&format, activeBlock->getName());
            if (fileName.isEmpty()) {
                // canceled, do nothing
            }
            else {
                QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
                RS_Graphic* graphic = createGraphicForBlock(activeBlock);
                graphic->setModified(true);

                LC_DocumentsStorage storage;
                storage.saveBlockAs(graphic, fileName);

                delete graphic;
                QApplication::restoreOverrideCursor();
            }
        } else
            commandMessage(tr("No block activated to save"));
    } else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_ActionBlocksSave::trigger():  blockList is NULL");
    }
    finish(false);
}

void RS_ActionBlocksSave::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}
