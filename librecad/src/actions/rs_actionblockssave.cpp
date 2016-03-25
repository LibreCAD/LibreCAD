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
#include "rs_insert.h"
#include "rs_coordinateevent.h"
#include "qc_mdiwindow.h"
#include "rs_debug.h"



RS_ActionBlocksSave::RS_ActionBlocksSave(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Block", container, graphicView) {}

/*recursive add blocks in graphic*/
void RS_ActionBlocksSave::addBlock(RS_Insert* in, RS_Graphic* g) {

	for(auto e: *in){

        if (e->rtti() == RS2::EntityInsert) {
			RS_Insert * in=static_cast<RS_Insert *>(e);
			addBlock(in,g);
			g->addBlock(in->getBlockForInsert());
        }
    }
}

void RS_ActionBlocksSave::trigger() {
    RS_DEBUG->print("save block to file");
    QC_ApplicationWindow* appWindow = QC_ApplicationWindow::getAppWindow();
	if(!appWindow) {
        finish(false);
        return;
    }
    RS_BlockList* bList = appWindow->getBlockWidget() -> getBlockList();
    if (bList) {
        auto b=bList->getActive();
        if(b) {
			RS_Graphic g(nullptr);
            g.setOwner(false);

           g.clearLayers();
//           g.addLayer(b->getLayer());
            for (RS_Entity* e=b->firstEntity(RS2::ResolveNone);
                 e;
                 e = b->nextEntity(RS2::ResolveNone)) {
                g.addEntity(e);
                if (e->rtti() == RS2::EntityInsert) {
					RS_Insert *in = static_cast<RS_Insert *>(e);
                    g.addBlock(in->getBlockForInsert());
					addBlock(in,&g);
                }
//           std::cout<<__FILE__<<" : "<<__func__<<" : line: "<<__LINE__<<" : "<<e->rtti()<<std::endl;
//                g.addLayer(e->getLayer());
//           std::cout<<__FILE__<<" : "<<__func__<<" : line: "<<__LINE__<<" : "<<e->rtti()<<std::endl;
            }
//           std::cout<<__FILE__<<" : "<<__func__<<" : line: "<<__LINE__<<std::endl;
//           std::cout<<"add layer name="<<qPrintable(b->getLayer()->getName())<<std::endl;

            RS2::FormatType t = RS2::FormatDXFRW;

            QG_FileDialog dlg(appWindow->getMDIWindow(),0, QG_FileDialog::BlockFile);
			QString const& fn = dlg.getSaveFile(&t);
            QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
//            g.setModified(true);
            g.saveAs(fn, t);
            QApplication::restoreOverrideCursor();
		} else
			RS_DIALOGFACTORY->commandMessage(tr("No block activated to save"));
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

