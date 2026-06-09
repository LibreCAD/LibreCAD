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

#include "lc_action_draw_bounding_box.h"

#include "lc_align.h"
#include "lc_draw_bounding_box_options_filler.h"
#include "lc_draw_bounding_box_options_widget.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_polyline.h"

LC_ActionDrawBoundingBox::LC_ActionDrawBoundingBox(LC_ActionContext* actionContext)
    : LC_ActionPreSelectionAwareBase("ActionDrawBoundingBox", actionContext, RS2::ActionDrawBoundingBox) {
}

void LC_ActionDrawBoundingBox::doSaveOptions() {
    save("AsGroup", m_selectionAsGroup);
    save("CornerPoints", m_cornerPointsOnly);
    save("Polyline", m_createPolyline);
    save("Offset", m_offset);
}

void LC_ActionDrawBoundingBox::doLoadOptions() {
    m_selectionAsGroup = loadBool("AsGroup", true);
    m_cornerPointsOnly = loadBool("CornerPoints", false);
    m_createPolyline = loadBool("Polyline", false);
    m_offset = loadDouble("Offset", 0.0);
}

void LC_ActionDrawBoundingBox::init(const int status) {
    showOptions();
    LC_ActionPreSelectionAwareBase::init(status);
}

bool LC_ActionDrawBoundingBox::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_selectionAsGroup) {
        RS_Vector selectionMin;
        RS_Vector selectionMax;
        LC_Align::collectSelectionBounds(m_selectedEntities, selectionMin, selectionMax);

        if (m_cornerPointsOnly) {
            createCornerPoints(selectionMin - m_offset, selectionMax + m_offset, ctx.entitiesToAdd);
        }
        else {
            if (m_createPolyline) {
                createBoxPolyline(selectionMin - m_offset, selectionMax + m_offset, ctx.entitiesToAdd);
            }
            else {
                createBoxLines(selectionMin - m_offset, selectionMax + m_offset, ctx.entitiesToAdd);
            }
        }
    }
    else {
        for (const auto e : std::as_const(m_selectedEntities)) {
            if (m_cornerPointsOnly) {
                createCornerPoints(e->getMin() - m_offset, e->getMax() + m_offset, ctx.entitiesToAdd);
            }
            else {
                if (m_createPolyline) {
                    createBoxPolyline(e->getMin() - m_offset, e->getMax() + m_offset, ctx.entitiesToAdd);
                }
                else {
                    createBoxLines(e->getMin() - m_offset, e->getMax() + m_offset, ctx.entitiesToAdd);
                }
            }
        }
    }

    // todo - sand - ucs - rework later as bounding box for entities will support ucs
    if (m_viewport->hasUCS()) {
        if (LC_LineMath::isMeaningfulAngle(m_viewport->getXAxisAngle())) {
            // ucs is rotated and resulting bounding box will be actually for wcs.
            // warn the user
            commandMessage(tr("Note: Bounding box was created for world coordinate system."));
        }
    }
    return true;
}

void LC_ActionDrawBoundingBox::doTriggerCompletion([[maybe_unused]]bool success) {
    m_selectedEntities.clear();
    finish();
}

void LC_ActionDrawBoundingBox::doTriggerSelectionUpdate(const bool keepSelected, [[maybe_unused]]const LC_DocumentModificationBatch& ctx) {
    if (!keepSelected) {
        unselect(m_selectedEntities);
    }
}

void LC_ActionDrawBoundingBox::createBoxPolyline(const RS_Vector& selectionMin, const RS_Vector& selectionMax, QList<RS_Entity*> &entitiesList) const {
    const auto e = new RS_Polyline(m_document);
    e->addVertex({selectionMin.x, selectionMax.y});
    e->addVertex({selectionMin.x, selectionMin.y});
    e->addVertex({selectionMax.x, selectionMin.y});
    e->addVertex({selectionMax.x, selectionMax.y});
    e->addVertex({selectionMin.x, selectionMax.y});
    entitiesList.append(e);
}

void LC_ActionDrawBoundingBox::createBoxLines(const RS_Vector& selectionMin, const RS_Vector& selectionMax, QList<RS_Entity*> &entitiesList) const {
    createLine(selectionMin.x, selectionMax.y, selectionMin.x, selectionMin.y, entitiesList);
    createLine(selectionMin.x, selectionMin.y, selectionMax.x, selectionMin.y, entitiesList);
    createLine(selectionMax.x, selectionMin.y, selectionMax.x, selectionMax.y, entitiesList);
    createLine(selectionMax.x, selectionMax.y, selectionMin.x, selectionMax.y, entitiesList);
}

void LC_ActionDrawBoundingBox::createCornerPoints(const RS_Vector& selectionMin, const RS_Vector& selectionMax, QList<RS_Entity*> &entitiesList) const {
    createPoint(selectionMin.x, selectionMax.y, entitiesList);
    createPoint(selectionMin.x, selectionMin.y, entitiesList);
    createPoint(selectionMax.x, selectionMax.y, entitiesList);
    createPoint(selectionMax.x, selectionMin.y, entitiesList);
}

void LC_ActionDrawBoundingBox::createPoint(double x, double y, QList<RS_Entity*> &entitiesList ) const {
    const auto e = new RS_Point(m_document, RS_PointData{{x, y}});
    entitiesList.append(e);
}

void LC_ActionDrawBoundingBox::createLine(double x1, double y1, double x2, double y2,QList<RS_Entity*> &entitiesList ) const {
    const auto e = new RS_Line(m_document, {{x1, y1}, {x2, y2}});
    entitiesList.append(e);
}

bool LC_ActionDrawBoundingBox::isAllowTriggerOnEmptySelection() {
    return false;
}

bool LC_ActionDrawBoundingBox::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "offset") {
        setOffset(distance);
        return true;
    }
    return false;
}

void LC_ActionDrawBoundingBox::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select entities for bounding box") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Select and draw")));
}


LC_ActionOptionsWidget* LC_ActionDrawBoundingBox::createOptionsWidget() {
    return new LC_DrawBoundingBoxOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawBoundingBox::createOptionsFiller() {
    return new LC_DrawBoundingBoxOptionsFiller();
}
