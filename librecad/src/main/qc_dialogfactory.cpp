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

#include "qc_dialogfactory.h"

#include <QMdiArea>

#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"

#include "qg_blockwidget.h"
#include "qg_graphicview.h"

#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_grid.h"



QC_DialogFactory::QC_DialogFactory(QWidget* parent, QToolBar* ow) :
  QG_DialogFactory(parent, ow)
{}



/**
 * Provides a window for editing the active block.
 */
void QC_DialogFactory::requestEditBlockWindow(RS_BlockList* blockList) {

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "QC_DialogFactory::requestEditBlockWindow()");

    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    QC_MDIWindow* parent = appWindow->getMDIWindow();

    if (!appWindow || !parent) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QC_DialogFactory::requestEditBlockWindow(): nullptr ApplicationWindow or MDIWindow");
        return;
    }

    // If block is opened from another block the parent must be set
    // to graphic that contain all these blocks.
    if (parent->getDocument()->rtti() == RS2::EntityBlock) {
        parent = parent->getParentWindow();
    }

    //get blocklist from block widget, bug#3497154
    if (!blockList ) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QC_DialogFactory::requestEditBlockWindow(): get blockList from appWindow");
        blockList = appWindow->getBlockWidget()->getBlockList();
    }

    RS_Block* blk = blockList->getActive();
    if (!blk) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QC_DialogFactory::requestEditBlockWindow(): no active block is selected");
        return;
    }
    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "QC_DialogFactory::requestEditBlockWindow(): edit block %s", blk->getName().toLatin1().data());
//            std::cout<<"QC_DialogFactory::requestEditBlockWindow(): size()="<<((blk==NULL)?0:blk->count() )<<std::endl;

    QC_MDIWindow* blockWindow = appWindow->getWindowWithDoc(blk);
    if (blockWindow) {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "QC_DialogFactory::requestEditBlockWindow(): activate existing window");
        appWindow->getMdiArea()->setActiveSubWindow(blockWindow);
    } else {
        RS_DEBUG->print(RS_Debug::D_DEBUGGING, "QC_DialogFactory::requestEditBlockWindow(): create new window");
        QC_MDIWindow* w = appWindow->slotFileNew(blk);
        if (!w) {
            RS_DEBUG->print(RS_Debug::D_ERROR, "QC_DialogFactory::requestEditBlockWindow(): can't create new child window");
            return;
        }

        // the parent needs a pointer to the block window and vice versa
        parent->addChildWindow(w);
        w->getGraphicView()->zoomAuto(false);
        //update grid settings, bug#3443293
        w->getGraphicView()->getGrid()->updatePointArray();
    }

    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "QC_DialogFactory::requestEditBlockWindow(): OK");
}



/**
 * Closes the window that is editing the given block.
 */
void QC_DialogFactory::closeEditBlockWindow(RS_Block* block) {
    RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow");

    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    QC_MDIWindow* blockWindow = appWindow->getWindowWithDoc(block);

    if (!blockWindow) {
        RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: block has no opened window");
        return;
    }

    RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: closing mdi");
    appWindow->slotFileClosing(blockWindow);

    RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: OK");
}

