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

#include <QMouseEvent>
#include <QKeyEvent>

#include "doc_plugin_interface.h"
#include "qc_actionentsel.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_snapper.h"


QC_ActionEntSel::QC_ActionEntSel(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView)
        :RS_ActionInterface("Get Entity", container, graphicView)
        , canceled(false)
        , completed{false}
{
    message = tr("Select object:");
    en = nullptr;
    targetPoint = RS_Vector(0,0);
}

void QC_ActionEntSel::updateMouseButtonHints() {
    if (!completed)
        updateMouseWidget(message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionEntSel::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void QC_ActionEntSel::setMessage(QString msg){
    message = msg;
}

void QC_ActionEntSel::trigger() {
    if (en) {
        completed = true;
        updateMouseButtonHints();
    } else {
        RS_DEBUG->print("QC_ActionEntSel::trigger: Entity is NULL\n");
    }
}

void QC_ActionEntSel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("QC_ActionEntSel::mouseMoveEvent begin");

    targetPoint = snapFree(e);

    RS_DEBUG->print("QC_ActionEntSel::mouseMoveEvent end");
}

void QC_ActionEntSel::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e) {
    en = catchEntity(e);
    targetPoint = snapFree(e);
    trigger();
}
void QC_ActionEntSel::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e){
    completed = true;
    canceled = true;
    updateMouseButtonHints();
    finish();
}

void QC_ActionEntSel::keyPressEvent(QKeyEvent *e){
    if (e->key() == Qt::Key_Escape) {
        updateMouseWidget();
        completed = true;
        canceled = true;
    }
}

int QC_ActionEntSel::getEntityId()
{
    return (en != nullptr ? en->getId() : 0);
}

/**
 * Add selected entity from 'container' to the selection.
 */
Plugin_Entity *QC_ActionEntSel::getSelected(Doc_plugin_interface* d) {
    Plugin_Entity *pe = en ? new Plugin_Entity(en, d) : nullptr;
    return pe;
}
