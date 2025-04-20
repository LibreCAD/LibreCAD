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

#include <QKeyEvent>

#include "doc_plugin_interface.h"
#include "rs_debug.h"
#include "rs_selection.h"

QC_ActionGetEnt::QC_ActionGetEnt(LC_ActionContext* actionContext)
        :RS_ActionInterface("Get Entity", actionContext,RS2::ActionGetEntity) {
    m_completed = false;
    m_message = tr("Select object:");
    m_entity = nullptr;
}

void QC_ActionGetEnt::updateMouseButtonHints() {
    if (!m_completed)
        updateMouseWidget(m_message, tr("Cancel"));
    else
        updateMouseWidget();
}


RS2::CursorType QC_ActionGetEnt::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void QC_ActionGetEnt::setMessage(QString msg){
    m_message = msg;
}

void QC_ActionGetEnt::trigger() {
    if (m_entity) {
        RS_Selection s(*m_container, m_viewport);
        s.selectSingle(m_entity);
        m_completed = true;
        updateMouseButtonHints();
    } else {
        RS_DEBUG->print("QC_ActionGetEnt::trigger: Entity is NULL\n");
    }
}

void QC_ActionGetEnt::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e) {
    m_entity = catchEntity(e);
    trigger();
}
void QC_ActionGetEnt::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent * e){
    m_completed = true;
    updateMouseButtonHints();
    finish();
}

void QC_ActionGetEnt::keyPressEvent(QKeyEvent *e){
    // qDebug() << "QC_ActionGetEnt::keyPressEvent";
    if (e->key() == Qt::Key_Escape) {
        updateMouseWidget();
        m_completed = true;
        // qDebug() << "escape QC_ActionGetEnt";
    }
}

/**
 * Add selected entity from 'container' to the selection.
 */
Plugin_Entity *QC_ActionGetEnt::getSelected(Doc_plugin_interface* d) {
    Plugin_Entity *pe = m_entity ? new Plugin_Entity(m_entity, d) : nullptr;
    return pe;
}
