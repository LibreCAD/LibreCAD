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

#include <QAction>
#include <QApplication>
#include "qg_blockwidget.h"
#include "qg_filedialog.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"
#include "rs_dialogfactory.h"



RS_ActionBlocksSave::RS_ActionBlocksSave(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Block", container, graphicView) {}


QAction* RS_ActionBlocksSave::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	//  tr("&Edit Block")
    QAction* action = new QAction( tr("&Save Block"), NULL);
    //action->zetStatusTip(tr("Edit Block"));
    action->setIcon(QIcon(":/main/filesave.png"));
	return action;
}


void RS_ActionBlocksSave::trigger() {
    RS_DEBUG->print("save block to file");
    QC_ApplicationWindow* appWindow = QC_ApplicationWindow::getAppWindow();
    if(appWindow==NULL) {
        finish(false);
        return;
    }
    RS_BlockList* bList = appWindow->getBlockWidget() -> getBlockList();
    if (bList!=NULL) {
        auto b=bList->getActive();
        if(b!=NULL) {
            RS_Graphic g(NULL);
            g.setOwner(false);

           g.clearLayers();
           g.addLayer(b->getLayer());
            for (RS_Entity* e=b->firstEntity(RS2::ResolveNone);
                 e!=NULL;
                 e = b->nextEntity(RS2::ResolveNone)) {
                g.addEntity(e);
            }
//           std::cout<<__FILE__<<" : "<<__FUNCTION__<<" : line: "<<__LINE__<<std::endl;
//           std::cout<<"add layer name="<<qPrintable(b->getLayer()->getName())<<std::endl;

            RS2::FormatType t = RS2::FormatDXF;

            QG_FileDialog dlg(appWindow->getMDIWindow());
            QString&& fn = dlg.getSaveFile(&t);
            QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
//            g.setModified(true);
            g.saveAs(fn, t);
            QApplication::restoreOverrideCursor();
        }else{
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->commandMessage(tr("No block activated to save"));
            }
        }
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

