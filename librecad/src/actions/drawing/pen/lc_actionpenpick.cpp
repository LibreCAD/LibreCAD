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
#include "lc_actionpenpick.h"

#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "rs_entity.h"
#include "rs_graphicview.h"
#include "rs_layer.h"

LC_ActionPenPick::LC_ActionPenPick(LC_ActionContext *actionContext, const bool resolve)
    :RS_PreviewActionInterface(resolve? "PenPick" : "PenPickApply", actionContext,
        resolve?  RS2::ActionPenPickResolved : RS2::ActionPenPick), m_resolveMode{resolve}{
}

void LC_ActionPenPick::init(const int status){
    RS_PreviewActionInterface::init(status);
}

void LC_ActionPenPick::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    pickPen(contextEntity);
}

/**
 * Cleanup that is needed if action was finished by escape
 */
void LC_ActionPenPick::finish(){
    RS_PreviewActionInterface::finish();
}

void LC_ActionPenPick::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status == SelectEntity){
        const RS_Entity *en = catchAndDescribe(e, RS2::ResolveNone);
        deleteHighlights();
        if (en != nullptr){
            highlightHover(en);
        }
    }
}

void LC_ActionPenPick::pickPen(const RS_Entity* en) {
    applyPenToPenToolBar(en);
    init(getStatus() - 1);
    finish();
    redraw();
    m_graphicView->back(Qt::KeyboardModifier::NoModifier);
}

void LC_ActionPenPick::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    if (status == SelectEntity){
        // Well, actually, it's possible to use Shift modifier for determining whether pen should be
        // resolved or not.  However, in UI there are two separate actions for consistency of
        // UIX with Pen Palette Widget
        const RS_Entity *en = catchEntityByEvent(e, RS2::ResolveNone);
        if (en != nullptr){
            pickPen(en);
        }
    }
}

void LC_ActionPenPick::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    finish();
    initPrevious(status);
    redraw();
}

void LC_ActionPenPick::updateActionPrompt(){
    updatePromptTRCancel(tr("Specify entity to pick the pen"));
}
RS2::CursorType LC_ActionPenPick::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

/**
 * Set pen picked from entity as active in pen toolbar
 * @param entity entity from which pen will be picked
 */
void LC_ActionPenPick::applyPenToPenToolBar(const RS_Entity* entity) const {
    QG_PenToolBar* penToolBar  = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
    if (penToolBar != nullptr){
            const RS_Layer *layer = entity->getLayer(true);
            if (layer != nullptr){
                const RS_Pen pen = entity->getPen(m_resolveMode);
                const RS_Pen layerPen = layer->getPen();
                const RS2::LineType lineType = pen.getLineType();
                const RS2::LineWidth width = pen.getWidth();
                const RS_Color &color = pen.getColor();
                // todo - processing of "By Block" - is it needed there?
                if (lineType == RS2::LineType::LineByLayer){
                    penToolBar->setLayerLineType(layerPen.getLineType(), true);
                } else {
                    penToolBar->setLineType(lineType);
                }

                if (width == RS2::LineWidth::WidthByLayer){
                    penToolBar->setLayerWidth(layerPen.getWidth(), true);
                }
                else{
                    penToolBar->setWidth(width);
                }

                if (color.isByLayer()){
                    penToolBar->setLayerColor(layerPen.getColor(), true);
                }
                else{
                    penToolBar->setColor(color);
                }
            }
      }
}
