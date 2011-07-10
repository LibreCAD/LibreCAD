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

#include "qc_dialogfactory.h"

#include "qc_applicationwindow.h"
#include "rs_blocklist.h"


/**
 * Provides a new window for editing the active block.
 */
void QC_DialogFactory::requestEditBlockWindow(RS_BlockList* blockList) {
    RS_DEBUG->print("QC_DialogFactory::requestEditBlockWindow()");

    QC_ApplicationWindow* appWindow = QC_ApplicationWindow::getAppWindow();
    QC_MDIWindow* parent = appWindow->getMDIWindow();
    if (parent!=NULL) {
        //RS_BlockList* blist = blockWidget->getBlockList();
        if (blockList!=NULL) {
            RS_Block* blk = blockList->getActive();
            if (blk!=NULL) {
                QC_MDIWindow* w = appWindow->slotFileNew(blk);
                // the parent needs a pointer to the block window and
                //   vice versa
                parent->addChildWindow(w);
                w->getGraphicView()->zoomAuto(false);
            }
        }
    }
}



/**
 * Closes all windows that are editing the given block.
 */
void QC_DialogFactory::closeEditBlockWindow(RS_Block* block) {
	RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow");
	
	QC_ApplicationWindow* appWindow = QC_ApplicationWindow::getAppWindow();
	QWorkspace* workspace = appWindow->getWorkspace();

    if (workspace!=NULL) {
		RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: workspace found");
		
        QWidgetList windows = workspace->windowList();
        for (int i = 0; i < int(windows.count()); ++i) {
			RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: window: %d",
				i);
            QC_MDIWindow* m = (QC_MDIWindow*)windows.at(i);
            if (m!=NULL) {
				RS_DEBUG->print(
					"QC_DialogFactory::closeEditBlockWindow: got mdi");
				if (m->getDocument()==block) {
					RS_DEBUG->print(
						"QC_DialogFactory::closeEditBlockWindow: closing mdi");
					//m->closeMDI(true, false);
					m->close(true);
				}
			}
		}
	}
	appWindow->slotWindowActivated(NULL);
	
	RS_DEBUG->print("QC_DialogFactory::closeEditBlockWindow: OK");
}

