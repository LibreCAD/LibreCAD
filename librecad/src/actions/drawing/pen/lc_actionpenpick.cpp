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
#include "qg_pentoolbar.h"
#include "qc_applicationwindow.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include <QMouseEvent>

LC_ActionPenPick::LC_ActionPenPick(RS_EntityContainer &container, RS_GraphicView &graphicView, bool resolve)
    :RS_PreviewActionInterface(resolve? "PenPick" : "PenPickApply", container, graphicView){
    resolveMode  = resolve;
    if (resolve){
        actionType = RS2::ActionPenPickResolved;
    }
    else{
        actionType = RS2::ActionPenPick;
    }
}

void LC_ActionPenPick::init(int status){
    RS_PreviewActionInterface::init(status);
}

void LC_ActionPenPick::trigger(){
    RS_PreviewActionInterface::trigger();
    // do nothing, processing is performed on mouse click
}

/**
 * Cleanup that is needed if action was finished by escape
 * @param updateTB
 */
void LC_ActionPenPick::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionPenPick::mouseMoveEvent(QMouseEvent *e){
    if (getStatus() == SelectEntity){
        RS_Entity *en = catchEntity(e, RS2::ResolveNone);
        deleteHighlights();
        if (en != nullptr){
            highlightHover(en);
        }
        drawHighlights();
    }
}

void LC_ActionPenPick::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    if (status == SelectEntity){
        // Well, actually, it's possible to use Shift modifier for determining whether pen should be
        // resolved or not.  However, in UI there are two separate actions for consistency of
        // UIX with Pen Palette Widget
        RS_Entity *en = catchEntity(e, RS2::ResolveNone);
        if (en != nullptr){
            applyPenToPenToolBar(en);
            init( getStatus() - 1);
            finish(true);
            graphicView->back();
        }
    }
    graphicView->redraw();
}

void LC_ActionPenPick::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    finish(true);
    initPrevious(status);
    graphicView->redraw();
}

void LC_ActionPenPick::updateMouseButtonHints(){
    updateMouseWidgetTRCancel(tr("Specify entity to pick the pen"));
}
RS2::CursorType LC_ActionPenPick::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

/**
 * Set pen picked from entity as active in pen toolbar
 * @param entity entity from which pen will be picked
 */
void LC_ActionPenPick::applyPenToPenToolBar(RS_Entity* entity){
    QG_PenToolBar* penToolBar  = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
    if (penToolBar != nullptr){
            RS_Layer *layer = entity->getLayer(true);
            if (layer != nullptr){
                RS_Pen pen = entity->getPen(resolveMode);
                RS_Pen layerPen = layer->getPen();
                RS2::LineType lineType = pen.getLineType();
                RS2::LineWidth width = pen.getWidth();
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
