/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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

#include "lc_action_draw_circle_inscribe.h"

#include "lc_creation_circle.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"

class RS_Polyline;

struct LC_ActionDrawCircleInscribe::ActionData {
    RS_CircleData circleData;
    RS_Vector coord;
    std::vector<RS_Line*> lines;
};

// fixme - cleanup, optoins

/**
 * Constructor.
 *
 */
LC_ActionDrawCircleInscribe::LC_ActionDrawCircleInscribe(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleInscribe", actionContext, RS2::ActionDrawCircleInscribe),
      m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionDrawCircleInscribe::~LC_ActionDrawCircleInscribe() = default;

void LC_ActionDrawCircleInscribe::clearLines(const bool checkStatus) const {
    while (!m_actionData->lines.empty()) {
        if (checkStatus && static_cast<int>(m_actionData->lines.size()) <= getStatus()) {
            break;
        }
        m_actionData->lines.pop_back();
    }
}

void LC_ActionDrawCircleInscribe::drawSnapper() {
    // disable snapper
}

void LC_ActionDrawCircleInscribe::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        setLine1(static_cast<RS_Line*>(entity));
    }
}

void LC_ActionDrawCircleInscribe::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    if (getStatus() >= 0) {
        RS_PreviewActionInterface::suspend();
    }
    clearLines(true);
}

void LC_ActionDrawCircleInscribe::finish() {
    clearLines();
    RS_PreviewActionInterface::finish();
}

RS_Entity* LC_ActionDrawCircleInscribe::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, m_actionData->circleData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

void LC_ActionDrawCircleInscribe::doTriggerCompletion([[maybe_unused]] bool success) {
    clearLines(false);
    setStatus(SetLine1);
}

void LC_ActionDrawCircleInscribe::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    for (RS_AtomicEntity* const pc : m_actionData->lines) {
        // highlight already selected
        highlightSelected(pc);
    }
    const auto en = catchModifiableAndDescribe(e, RS2::EntityLine);
    // fixme - check whether snap is used for entity selection?  Ensure free snap?

    if (en != nullptr) {
        auto* line = static_cast<RS_Line*>(en);
        switch (status) {
            case SetLine1: {
                highlightHover(en);
                break;
            }
            case SetLine2: {
                if (en != m_actionData->lines[SetLine1]) {
                    highlightHover(en);
                }
                break;
            }
            case SetLine3: {
                if (m_actionData->lines[SetLine1] != line && m_actionData->lines[SetLine2] != line) {
                    m_actionData->coord = e->graphPoint;
                    if (preparePreview(line)) {
                        highlightHover(en);
                        previewToCreateCircle(m_actionData->circleData);
                        if (m_showRefEntitiesOnPreview) {
                            const RS_Vector& center = m_actionData->circleData.center;
                            previewRefPoint(m_actionData->lines[SetLine1]->getNearestPointOnEntity(center, false));
                            previewRefPoint(m_actionData->lines[SetLine2]->getNearestPointOnEntity(center, false));
                            previewRefPoint(line->getNearestPointOnEntity(center, false));
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void LC_ActionDrawCircleInscribe::setLine1(RS_Line* line) {
    m_actionData->lines.push_back(line);
    setStatus(SetLine2);
}

void LC_ActionDrawCircleInscribe::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine); // fixme - support of polyline
    if (en == nullptr) {
        return;
    }
    if (!(en->isVisible() && isLine(en))) {
        return;
    }
    for (int i = 0; i < status; i++) {
        if (en->getId() == m_actionData->lines[i]->getId()) {
            return; //do not pull in the same line again
        }
    }

    m_actionData->coord = e->graphPoint;
    auto* line = dynamic_cast<RS_Line*>(en);

    switch (status) {
        case SetLine1: {
            setLine1(line);
            break;
        }
        case SetLine2:
            m_actionData->lines.push_back(line);
            setStatus(SetLine3);
            break;
        case SetLine3:
            if (preparePreview(line)) {
                trigger();
            }
            break;
        default:
            break;
    }
}

void LC_ActionDrawCircleInscribe::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    if (status > 0) {
        m_actionData->lines.pop_back();
        deletePreview();
    }
    initPrevious(status);
}

bool LC_ActionDrawCircleInscribe::preparePreview(RS_Line* en) {
    m_valid = false;
    if (getStatus() == SetLine3) {
        if (en != nullptr) {
            m_actionData->lines.push_back(en);
        }
        m_valid = LC_CreationCircle::createInscribe(m_actionData->coord, m_actionData->lines, m_actionData->circleData);
        if (!m_valid) {
            m_actionData->circleData = RS_CircleData();
        }
    }
    if (en != nullptr) {
        m_actionData->lines.pop_back();
    }
    return m_valid;
}

void LC_ActionDrawCircleInscribe::updateActionPrompt() {
    switch (getStatus()) {
        case SetLine1:
            updatePromptTRCancel(tr("Specify the first line"));
            break;
        case SetLine2:
            updatePromptTRBack(tr("Specify the second line"));
            break;
        case SetLine3:
            updatePromptTRBack(tr("Specify the third line"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleInscribe::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
