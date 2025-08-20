/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#include "rs_actionselectintersected.h"

#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_selection.h"

struct RS_ActionSelectIntersected::ActionData {
	RS_Vector v1;
	RS_Vector v2;
    RS_Entity* m_entityToIntersect{nullptr};
};

/**
 * Constructor.
 *
 * @param select true: select window. false: deselect window
 */
RS_ActionSelectIntersected::RS_ActionSelectIntersected(LC_ActionContext *actionContext, bool select)
    :RS_PreviewActionInterface("Select Intersected",actionContext, RS2::ActionSelectIntersected),
    m_actionData(std::make_unique<ActionData>()), m_performSelect(select){
}

RS_ActionSelectIntersected::~RS_ActionSelectIntersected() = default;

void RS_ActionSelectIntersected::reset() {
    m_actionData->m_entityToIntersect = nullptr;
    m_actionData->v1.valid = false;
    m_actionData->v2.valid = false;
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionSelectIntersected::init(int status) {
    reset();
    RS_PreviewActionInterface::init(status);
}

void RS_ActionSelectIntersected::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (isValidEntityForSelection(contextEntity)) {
        m_actionData->m_entityToIntersect = contextEntity;
        trigger();
    }
}

void RS_ActionSelectIntersected::doTrigger() {
    if (m_actionData->m_entityToIntersect != nullptr) { // do intersection with entity
        RS_Selection s(*m_container, m_viewport);
        s.selectIntersected(m_actionData->m_entityToIntersect, m_performSelect);
        reset();
        setStatus(SetPoint1);
    }
    else if (m_actionData->v1.valid && m_actionData->v2.valid){ // do intersection with point
        if (toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 10){
            RS_Selection s(*m_container, m_viewport);
            s.selectIntersected(m_actionData->v1, m_actionData->v2, m_performSelect);
            reset();
            setStatus(SetPoint1);
        }
    }
}

void RS_ActionSelectIntersected::onMouseMoveEvent(int status, LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetPoint1) {
        if (e->isShift) {
            RS_Entity* entity;
            if (e->isControl) {
                entity = catchAndDescribe(e, RS2::ResolveAll);
            }
            else {
                entity = catchAndDescribe(e, RS2::ResolveNone);
            }

            if (entity != nullptr) {
                bool entityTypeValidForSelection = isValidEntityForSelection(entity);
                if (entityTypeValidForSelection) {
                    highlightHover(entity);
                }
            }
        }
    }
    else if (status == SetPoint2 && m_actionData->v1.valid) {
        m_actionData->v2 = snap;
        previewLine(m_actionData->v1, m_actionData->v2);
        // todo - of course, ideally it will be also to highlight entities that will be selected...
        // however, calculating of intersections as it is currently is may be quite costly operation for mouse move
        // todo - review preview for selected entities after indexing
    }
}

void RS_ActionSelectIntersected::onMouseLeftButtonPress(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetPoint1:
            if (!e->isShift) {
                m_actionData->v1 = e->snapPoint;
                setStatus(SetPoint2);
            }
            break;
        default:
            break;
    }
}

bool RS_ActionSelectIntersected::isValidEntityForSelection(RS_Entity* entity) {
    RS2::EntityType entityType =  entity->rtti();
    bool entityTypeValidForSelection = !entity->isParentIgnoredOnModifications() && entityType != RS2::EntitySpline;
    return entityTypeValidForSelection;
}

void RS_ActionSelectIntersected::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (status == SetPoint1) {
        if (e->isShift) {
            RS_Entity* entity;
            if (e->isControl) {
                entity = catchAndDescribe(e, RS2::ResolveAll);
            }
            else {
                entity = catchAndDescribe(e, RS2::ResolveNone);
            }
            if (entity != nullptr) {
                bool entityTypeValidForSelection = isValidEntityForSelection(entity);
                if (entityTypeValidForSelection) {
                    m_actionData->m_entityToIntersect = entity;
                    trigger();
                }
            }
        }
    }
    else if (status == SetPoint2){
        m_actionData->v2 = e->snapPoint;
        trigger();
    }
}

void RS_ActionSelectIntersected::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (getStatus() == SetPoint2){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionSelectIntersected::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Choose first point of intersection line"), MOD_SHIFT_AND_CTRL(tr("Select intersecting entity"), tr("Select child entities")));
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Choose second point of intersection line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionSelectIntersected::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
