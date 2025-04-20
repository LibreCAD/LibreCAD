/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 sand1024

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
#include "lc_actionpenapply.h"

#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "rs_entity.h"
#include "rs_modification.h"
#include "rs_pen.h"

class QG_PenToolBar;

LC_ActionPenApply::LC_ActionPenApply(LC_ActionContext *actionContext, bool copy):
    RS_PreviewActionInterface(copy? "PenCopy" : "PenApply", actionContext, copy? RS2::ActionPenCopy :  RS2::ActionPenApply),
    m_copyMode{copy}{
}

void LC_ActionPenApply::init(int status){
    if (status == SelectEntity && !m_copyMode){
        status = ApplyToEntity;
    }
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        m_srcEntity = nullptr;
    }
}

void LC_ActionPenApply::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status){
        case SelectEntity:
        case ApplyToEntity:{
            RS_Entity* en = catchAndDescribe(e, RS2::ResolveNone);
            if (en != nullptr && en != m_srcEntity){ // exclude entity we use as source, if any
                highlightHover(en);
            }
            break;
        }
        default:
            break;
    }
}

/**
 * this one is called if back by escape is involved, so perform cleanup there too
 * @param updateTB
 */
void LC_ActionPenApply::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
    m_srcEntity = nullptr;
}

void LC_ActionPenApply::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Entity* en= catchEntityByEvent(e, RS2::ResolveNone);

    if(en != nullptr){
        switch (getStatus()){
            case SelectEntity:{
                // selection of entity that will be used as source for pen
                m_srcEntity = en;
                setStatus(ApplyToEntity);
                break;
            }
            case ApplyToEntity:{
                if (!en->isLocked() && en != m_srcEntity){
                    RS_Pen penToApply;
                    if (m_copyMode){
                        // we apply pen from source entity, if Shift is pressed - resolved pen is used.
                        bool resolvePen = e->isShift;
                        penToApply = m_srcEntity->getPen(resolvePen);

                    } else {
                        // we apply active pen from pen toolbar
                        QG_PenToolBar *penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
                        if (penToolBar != nullptr){
                            penToApply = penToolBar->getPen();
                        }
                    }

                    // do actual modifications
                    RS_AttributesData data;
                    data.pen = penToApply;
                    data.layer = "0";
                    data.changeColor = true;
                    data.changeLineType = true;
                    data.changeWidth = true;
                    data.changeLayer = false;

                    // this is temporary selection, it is needed as RS_Modification relies on selected entities.
                    // fixme - sand - replace by version with explicitly provided entity rather than one that relies on selection
                    en->setSelected(true);

                    RS_Modification m(*m_container, m_viewport);
                    m.changeAttributes(data, false);
                }
                break;
            }
        }
    }
    redraw();
}

void LC_ActionPenApply::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    switch (status){
        case SelectEntity:{
            init(-1);
            break;
        }
        case ApplyToEntity:{
            if (m_copyMode){
                setStatus(SelectEntity);
                m_srcEntity = nullptr;
            }
            else{
                init(-1);
            }
            break;
        }
        default:
            break;
    }
    redraw();
}

void LC_ActionPenApply::updateMouseButtonHints(){
    switch (getStatus()) {
        case (SelectEntity):
            updateMouseWidgetTRCancel(tr("Specify entity to pick the pen"));
            break;
        case ApplyToEntity:
            updateMouseWidgetTRCancel(tr("Specify entity to apply pen"), m_copyMode? MOD_SHIFT_LC(tr("Apply Resolved Pen")) : MOD_NONE);
            break;
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}
RS2::CursorType LC_ActionPenApply::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
