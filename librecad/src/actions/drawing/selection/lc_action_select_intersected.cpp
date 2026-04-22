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

#include "lc_action_select_intersected.h"

#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_selection.h"

struct LC_ActionSelectIntersected::ActionData {
    RS_Vector v1;
    RS_Vector v2;
    RS_Entity* entityToIntersect{nullptr};
};

/**
 * Constructor.
 *
 * @param actionContext
 * @param select true: select window. false: deselect window
 */
LC_ActionSelectIntersected::LC_ActionSelectIntersected(LC_ActionContext* actionContext, const bool select)
    : RS_ActionSelectBase(select ? "ActionSelectIntersected" : "ActionDeselectIntersected", actionContext,
                          select ? RS2::ActionSelectIntersected : RS2::ActionDeselectIntersected),
      m_actionData(std::make_unique<ActionData>()), m_performSelect(select) {
}

LC_ActionSelectIntersected::~LC_ActionSelectIntersected() = default;

void LC_ActionSelectIntersected::reset() {
    m_actionData->entityToIntersect = nullptr;
    m_actionData->v1.valid = false;
    m_actionData->v2.valid = false;
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
}

void LC_ActionSelectIntersected::init(const int status) {
    reset();
    RS_PreviewActionInterface::init(status);
}

void LC_ActionSelectIntersected::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    if (isValidEntityForSelection(contextEntity)) {
        m_actionData->entityToIntersect = contextEntity;
        trigger();
    }
}

void LC_ActionSelectIntersected::doTrigger() {
    if (m_actionData->entityToIntersect != nullptr) {
        // do intersection with entity
        const RS_Selection s(m_document, m_viewport);
        s.selectIntersected(m_actionData->entityToIntersect, m_performSelect);
        reset();
        setStatus(SetPoint1);
    }
    else if (m_actionData->v1.valid && m_actionData->v2.valid) {
        // do intersection with point
        if (toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 10) {
            const RS_Selection s(m_document, m_viewport);
            s.selectIntersected(m_actionData->v1, m_actionData->v2, m_performSelect);
            reset();
            setStatus(SetPoint1);
        }
    }
}

void LC_ActionSelectIntersected::selectionFinishedByKey([[maybe_unused]] QKeyEvent* e, [[maybe_unused]] bool escape) {
    finish();
}

void LC_ActionSelectIntersected::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status == SetPoint1) {
        if (e->isShift) {
            const RS_Entity* entity = nullptr;
            if (e->isControl) {
                entity = catchAndDescribe(e, RS2::ResolveAll);
            }
            else {
                entity = catchAndDescribe(e, RS2::ResolveNone);
            }

            if (entity != nullptr) {
                const bool entityTypeValidForSelection = isValidEntityForSelection(entity);
                if (entityTypeValidForSelection) {
                    highlightHover(entity);
                }
            }
        }
    }
    else if (status == SetPoint2 && m_actionData->v1.valid) {
        m_actionData->v2 = e->snapPoint;
        previewLine(m_actionData->v1, m_actionData->v2);
        // todo - of course, ideally it will be also to highlight entities that will be selected...
        // however, calculating of intersections as it is currently is may be quite costly operation for mouse move
        // todo - review preview for selected entities after indexing
    }
}

void LC_ActionSelectIntersected::onMouseLeftButtonPress(const int status, const LC_MouseEvent* e) {
    m_mouseIsPressed = true;
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

void LC_ActionSelectIntersected::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");

    if (status == SetPoint1) {
        if (e->isShift) {
            RS_Entity* entity = nullptr;
            if (e->isControl) {
                entity = catchAndDescribe(e, RS2::ResolveAll);
            }
            else {
                entity = catchAndDescribe(e, RS2::ResolveNone);
            }
            if (entity != nullptr) {
                const bool entityTypeValidForSelection = isValidEntityForSelection(entity);
                if (entityTypeValidForSelection) {
                    m_actionData->entityToIntersect = entity;
                    trigger();
                }
            }
            else {
                if (e->isControl) {
                    finish();
                }
            }
        }
    }
    else if (status == SetPoint2) {
        m_actionData->v2 = e->snapPoint;
        trigger();
        if (e->isControl) {
            finish();
        }
    }
    m_mouseIsPressed = false;
}

bool LC_ActionSelectIntersected::isValidEntityForSelection(const RS_Entity* entity) {
    const RS2::EntityType entityType = entity->rtti();
    const bool entityTypeValidForSelection = !entity->isParentIgnoredOnModifications() && entityType != RS2::EntitySpline;
    return entityTypeValidForSelection;
}

void LC_ActionSelectIntersected::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (getStatus() == SetPoint2) {
        deletePreview();
    }
    initPrevious(status);
}

void LC_ActionSelectIntersected::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint1:
            updatePromptTRCancel(tr("Choose first point of intersection line") + " " + getSelectionCompletionHintMsg(),
                                 MOD_SHIFT_AND_CTRL(tr("Select intersecting entity"), tr("Select child entities")));
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Choose second point of intersection line"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionSelectIntersected::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
