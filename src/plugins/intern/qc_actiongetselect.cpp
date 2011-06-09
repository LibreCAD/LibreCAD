/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
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

#include "doc_plugin_interface.h"
#include "qc_actiongetselect.h"
#include "rs_actionselectsingle.h"

#include "rs_snapper.h"


QC_ActionGetSelect::QC_ActionGetSelect(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView)
        :RS_ActionInterface("Get Select", container, graphicView) {
    completed = false;
    mesage = tr("Select objects:");
}


void QC_ActionGetSelect::updateMouseButtonHints() {
    switch (getStatus()) {
    case Select:
        RS_DIALOGFACTORY->updateMouseWidget(mesage, tr("Cancel"));
            break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}


void QC_ActionGetSelect::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

void QC_ActionGetSelect::setMesage(QString msg){
    mesage = msg;
}


void QC_ActionGetSelect::init(int status) {
        RS_ActionInterface::init(status);
        graphicView->setCurrentAction(
                new RS_ActionSelectSingle(*container, *graphicView));
}



void QC_ActionGetSelect::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        completed = true;
    }
}



void QC_ActionGetSelect::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            RS_DIALOGFACTORY->requestToolBarSelect(this, RS2::ActionDefault);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            completed = true;
        }
    }
}

/**
 * Adds all selected entities from 'container' to the selection.
 */
void QC_ActionGetSelect::getSelected(QList<Plug_Entity *> *se) {
    for (RS_Entity* e= container->firstEntity();
            e!=NULL; e= container->nextEntity()) {

        if (e->isSelected()) {
            Plugin_Entity *pe = new Plugin_Entity(e);
            se->append(reinterpret_cast<Plug_Entity*>(pe));
        }
    }
}


// EOF
