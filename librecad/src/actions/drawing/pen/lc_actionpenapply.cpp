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
#include "rs_dialogfactory.h"
#include "qg_pentoolbar.h"
#include "qc_applicationwindow.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include <QMouseEvent>

LC_ActionPenApply::LC_ActionPenApply(RS_EntityContainer &container, RS_GraphicView &graphicView, bool copy):
  RS_PreviewActionInterface(copy? "PenCopy" : "PenApply", container, graphicView){
    copyMode  = copy;
    if (copy){
        actionType = RS2::ActionPenCopy;
    }
    else{
        actionType = RS2::ActionPenApply;
    }
    srcEntity = nullptr;
}

void LC_ActionPenApply::init(int status){
    if (status == SelectEntity && !copyMode){
        status = ApplyToEntity;
    }
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        srcEntity = nullptr;
    }
}

void LC_ActionPenApply::mouseMoveEvent(QMouseEvent *e){
    deletePreview();
    deleteHighlights();

    snapPoint(e);

    switch (getStatus()){
        case SelectEntity:
        case ApplyToEntity:
            RS_Entity* en = catchEntityOnPreview(e, RS2::ResolveNone);
            if (en != nullptr && en != srcEntity){ // exclude entity we use as source, if any
                highlightHover(en);
            }
            break;
    }
    drawPreview();
    drawHighlights();
}

/**
 * this one is called if back by escape is involved, so perform cleanup there too
 * @param updateTB
 */
void LC_ActionPenApply::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
    srcEntity = nullptr;
}

void LC_ActionPenApply::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    RS_Entity* en= catchEntity(e, RS2::ResolveNone);

    if(en != nullptr){
        switch (getStatus()){
            case SelectEntity:{
                // selection of entity that will be used as source for pen
                srcEntity = en;
                setStatus(ApplyToEntity);
                break;
            }
            case ApplyToEntity:{
                if (!en->isLocked() && en != srcEntity){
                    RS_Pen penToApply;
                    if (copyMode){
                        // we apply pen from source entity, if Shift is pressed - resolved pen is used.
                        bool resolvePen = e->modifiers() & Qt::ShiftModifier;
                        penToApply = srcEntity->getPen(resolvePen);

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

                    RS_Modification m(*container, graphicView);
                    m.changeAttributes(data, false);
                }
                break;
            }
        }
    }
    graphicView->redraw();
}

void LC_ActionPenApply::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    switch (status){
        case SelectEntity:{
            init(-1);
            break;
        }
        case ApplyToEntity:{
            if (copyMode){
                setStatus(SelectEntity);
                srcEntity = nullptr;
            }
            else{
                init(-1);
            }
            break;
        }
        default:
            break;
    }
    graphicView->redraw();
}

void LC_ActionPenApply::updateMouseButtonHints(){
    switch (getStatus()) {
        case (SelectEntity):
            updateMouseWidgetTRCancel(tr("Specify entity to pick the pen"));
            break;
        case ApplyToEntity:
            updateMouseWidgetTRCancel(tr("Specify entity to apply pen"), copyMode? MOD_SHIFT_LC(tr("Apply Resolved Pen")) : MOD_NONE);
            break;
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}
RS2::CursorType LC_ActionPenApply::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
