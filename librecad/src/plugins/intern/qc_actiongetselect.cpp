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
#include "qc_actiongetselect.h"

#include <QKeyEvent>

#include "doc_plugin_interface.h"
#include "lc_action_select_single.h"
#include "rs_graphicview.h"


class Plugin_Entity;

QC_ActionGetSelect::QC_ActionGetSelect(LC_ActionContext* actionContext)
    :RS_ActionInterface("Get Select", actionContext, RS2::ActionGetSelect), m_message(std::make_unique<QString>(tr("Select objects:"))){
}

QC_ActionGetSelect::QC_ActionGetSelect(const RS2::EntityType typeToSelect, LC_ActionContext* actionContext)
    :RS_ActionInterface("Get Select", actionContext, RS2::ActionGetSelect), m_message(std::make_unique<QString>(tr("Select objects:"))),
     m_entityTypeToSelect(typeToSelect){
}

QC_ActionGetSelect::~QC_ActionGetSelect() = default;

void QC_ActionGetSelect::updateActionPrompt() {
    switch (getStatus()) {
        case Select:
            updatePrompt(*m_message, tr("Cancel"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType QC_ActionGetSelect::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void QC_ActionGetSelect::setMessage(QString msg) const {
    *m_message = std::move(msg);
}

void QC_ActionGetSelect::init(const int status) {
        RS_ActionInterface::init(status);
        m_graphicView->setCurrentAction(
                std::make_shared<LC_ActionSelectSingle>(m_entityTypeToSelect,  m_actionContext, this));
}

void QC_ActionGetSelect::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        m_completed = true;
        updatePrompt();
        finish();
    }
}

void QC_ActionGetSelect::keyPressEvent(QKeyEvent* e){
    if (e->key()==Qt::Key_Escape || e->key()==Qt::Key_Enter){
        updatePrompt();
        finish();
        m_completed = true;
    }
}

/**
 * Adds all selected entities from 'container' to the selection.
 */
void QC_ActionGetSelect::getSelected(QList<Plug_Entity *> *se, Doc_plugin_interface *d) const{
    QList<RS_Entity*> selection;
    m_document->collectSelected(selection);
    for (const auto e: selection) {
        const auto pe = new Plugin_Entity(e, d);
        se->append(reinterpret_cast<Plug_Entity*>(pe));
    }
}

[[deprecated]]
void QC_ActionGetSelect::unselectEntities() const { // fixme - rework by inlinining
    unselectAll();
}
