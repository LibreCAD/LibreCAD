/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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
#include "qc_actiongetent.h"
#include "rs_selection.h"
#include "rs_snapper.h"


QC_ActionGetEnt::QC_ActionGetEnt(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView)
        :RS_ActionInterface("Get Entity", container, graphicView) {
    completed = false;
    mesage = tr("Select object:");
    en = NULL;
}


void QC_ActionGetEnt::updateMouseButtonHints() {
    if (!completed)
        RS_DIALOGFACTORY->updateMouseWidget(mesage, tr("Cancel"));
    else
        RS_DIALOGFACTORY->updateMouseWidget("", "");
}


void QC_ActionGetEnt::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

void QC_ActionGetEnt::setMesage(QString msg){
    mesage = msg;
}

void QC_ActionGetEnt::trigger() {
    if (en!=NULL) {
        RS_Selection s(*container, graphicView);
        s.selectSingle(en);
        completed = true;
        updateMouseButtonHints();
    } else {
        RS_DEBUG->print("QC_ActionGetEnt::trigger: Entity is NULL\n");
    }
}

void QC_ActionGetEnt::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}

/**
 * Add selected entity from 'container' to the selection.
 */
Plugin_Entity *QC_ActionGetEnt::getSelected() {
    Plugin_Entity *pe = new Plugin_Entity(en);
    return pe;
}

// EOF
