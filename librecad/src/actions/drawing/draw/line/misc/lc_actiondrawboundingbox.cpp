/****************************************************************************
**
* Action that draws a cross in the center of selected arc, circle,
* ellipse or ellipse arc

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
**********************************************************************/

#include "lc_actiondrawboundingbox.h"

#include "lc_align.h"
#include "lc_drawboundingboxoptions.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_polyline.h"

LC_ActionDrawBoundingBox::LC_ActionDrawBoundingBox(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("DrawBoundingBox", actionContext, RS2::ActionDrawBoundingBox){
}

void LC_ActionDrawBoundingBox::init(int status) {
    showOptions();
    LC_ActionPreSelectionAwareBase::init(status);
}

void LC_ActionDrawBoundingBox::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select entities for bounding box (Enter to complete)"), MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Select and draw")));
}

void LC_ActionDrawBoundingBox::doTrigger([[maybe_unused]]bool keepSelected) {
    if (m_document != nullptr) {
        RS_Graphic* graphic = m_graphicView->getGraphic();
        RS_Layer* activeLayer = graphic->getActiveLayer();
        RS_Pen pen = m_document->getActivePen();
        undoCycleStart();
        if (m_selectionAsGroup) {
            RS_Vector selectionMin;
            RS_Vector selectionMax;
            LC_Align::collectSelectionBounds(m_selectedEntities, selectionMin, selectionMax);

            if (m_cornerPointsOnly){
                createCornerPoints(activeLayer, pen, selectionMin-m_offset, selectionMax+m_offset);
            }
            else{
                if (m_createPolyline){
                    createBoxPolyline(activeLayer, pen, selectionMin-m_offset, selectionMax+m_offset);
                }
                else {
                    createBoxLines(activeLayer, pen, selectionMin-m_offset, selectionMax+m_offset);
                }
            }
        } else {
            for (auto e: m_selectedEntities){
                if (m_cornerPointsOnly){
                    createCornerPoints(activeLayer, pen, e->getMin()-m_offset, e->getMax()+m_offset);
                }
                else{
                    if (m_createPolyline) {
                        createBoxPolyline(activeLayer, pen, e->getMin()-m_offset, e->getMax()+m_offset);
                    }
                    else{
                        createBoxLines(activeLayer, pen, e->getMin()-m_offset, e->getMax()+m_offset);
                    }
                }
            }
        }
        undoCycleEnd();
    }
    // todo - sand - ucs - rework later as bounding box for entities will support ucs
    if (m_viewport->hasUCS()){
        if (LC_LineMath::isMeaningfulAngle(m_viewport->getXAxisAngle())){
            // ucs is rotated and resulting bounding box will be actually for wcs.
            // warn the user
            commandMessage(tr("Note: Bounding box was created for world coordinate system."));
        }
    }
    m_selectedEntities.clear();
    finish(false);
}

void LC_ActionDrawBoundingBox::createBoxPolyline(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax) {
    auto e = new RS_Polyline(m_container);
    e->setLayer(activeLayer);
    e->setPen(pen);

    e->addVertex({selectionMin.x, selectionMax.y});
    e->addVertex({selectionMin.x, selectionMin.y});
    e->addVertex({selectionMax.x, selectionMin.y});
    e->addVertex({selectionMax.x, selectionMax.y});
    e->addVertex({selectionMin.x, selectionMax.y});

    m_container->addEntity(e);
    m_document->addUndoable(e);
}

void LC_ActionDrawBoundingBox::createBoxLines(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax) {
    createLine(activeLayer, pen, selectionMin.x, selectionMax.y, selectionMin.x, selectionMin.y);
    createLine(activeLayer, pen, selectionMin.x, selectionMin.y, selectionMax.x, selectionMin.y);
    createLine(activeLayer, pen, selectionMax.x, selectionMin.y, selectionMax.x, selectionMax.y);
    createLine(activeLayer, pen, selectionMax.x, selectionMax.y, selectionMin.x, selectionMax.y);
}

void LC_ActionDrawBoundingBox::createCornerPoints(RS_Layer *activeLayer, const RS_Pen &pen, const RS_Vector &selectionMin, const RS_Vector &selectionMax) {
    createPoint(activeLayer, pen, selectionMin.x, selectionMax.y);
    createPoint(activeLayer, pen, selectionMin.x, selectionMin.y);
    createPoint(activeLayer, pen, selectionMax.x, selectionMax.y);
    createPoint(activeLayer, pen, selectionMax.x, selectionMin.y);
}

void LC_ActionDrawBoundingBox::createPoint(RS_Layer *activeLayer, const RS_Pen &pen, double x, double y) {
    auto e = new RS_Point(m_container, {{x, y}});
    e->setLayer(activeLayer);
    e->setPen(pen);
    m_container->addEntity(e);
    m_document->addUndoable(e);
}

void LC_ActionDrawBoundingBox::createLine(RS_Layer *activeLayer, const RS_Pen &pen, double x1, double y1, double x2, double y2) {
    auto e = new RS_Line(m_container, {{x1, y1}, {x2, y2}});
    e->setLayer(activeLayer);
    e->setPen(pen);
    m_container->addEntity(e);
    undoableAdd(e);
}

bool LC_ActionDrawBoundingBox::isAllowTriggerOnEmptySelection() {
    return false;
}

LC_ActionOptionsWidget *LC_ActionDrawBoundingBox::createOptionsWidget() {
    return new LC_DrawBoundingBoxOptions();
}
