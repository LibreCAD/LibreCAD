/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
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

#include "qc_actiongetent.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include "doc_plugin_interface.h"
#include "rs_dialogfactory.h"
#include "rs_selection.h"
#include "rs_snapper.h"
#include "rs_debug.h"
// #include <QDebug>


QC_ActionGetEnt::QC_ActionGetEnt(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView)
        :RS_ActionInterface("Get Entity", container, graphicView) {
    completed = false;
    mesage = tr("Select object:");
    en = nullptr;
}



void QC_ActionGetEnt::updateMouseButtonHints() {
    if (!completed)
        RS_DIALOGFACTORY->updateMouseWidget(mesage, tr("Cancel"));
    else
        RS_DIALOGFACTORY->updateMouseWidget();
}


void QC_ActionGetEnt::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

void QC_ActionGetEnt::setMesage(QString msg){
    mesage = msg;
}

void QC_ActionGetEnt::trigger() {
    if (en) {
        RS_Selection s(*container, graphicView);
        s.selectSingle(en);
        completed = true;
        updateMouseButtonHints();
    } else {
        RS_DEBUG->print("QC_ActionGetEnt::trigger: Entity is NULL\n");
    }
}

void QC_ActionGetEnt::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        completed = true;
        updateMouseButtonHints();
        finish();
    } else {
        en = catchEntity(e);
        trigger();
    }
}

void QC_ActionGetEnt::keyPressEvent(QKeyEvent* e)
{
    // qDebug() << "QC_ActionGetEnt::keyPressEvent";
    if (e->key()==Qt::Key_Escape)
    {
		RS_DIALOGFACTORY->updateMouseWidget();
        completed = true;
        // qDebug() << "escape QC_ActionGetEnt";
    }
}

/**
 * Add selected entity from 'container' to the selection.
 */
Plugin_Entity *QC_ActionGetEnt::getSelected(Doc_plugin_interface* d) {
    Plugin_Entity *pe = en ? new Plugin_Entity(en, d) : nullptr;
    return pe;
}

// EOF
