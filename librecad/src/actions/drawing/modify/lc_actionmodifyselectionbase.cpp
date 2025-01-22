/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/

#include <QKeyEvent>

#include "lc_actionmodifyselectionbase.h"
#include "rs_entitycontainer.h"


LC_ActionModifySelectionBase::LC_ActionModifySelectionBase(const char *name,
                                                           RS_EntityContainer &container,
                                                           RS_GraphicView &graphicView, RS2::ActionType actionType)
                                                           :RS_PreviewActionInterface(name, container, graphicView,actionType){}

void LC_ActionModifySelectionBase::mouseMoveEvent(QMouseEvent *event){
    RS_ActionInterface::mouseMoveEvent(event);
}

void LC_ActionModifySelectionBase::mouseReleaseEvent(QMouseEvent *event){
    RS_ActionInterface::mouseReleaseEvent(event);
}

int LC_ActionModifySelectionBase::countSelected() {
    unsigned int ret=container->countSelected();
    // fixme - ensure that this is correct place for method
    if (ret==0) {
        commandMessage(tr("No entity selected!"));
    }
    return ret;
}

void LC_ActionModifySelectionBase::keyPressEvent(QKeyEvent *e){
    if (e->key()==Qt::Key_Enter && countSelected() > 0){
        selectionFinished = true;
    }
}
