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

#include "rs_actiondrawcircleinscribe.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_preview.h"

struct RS_ActionDrawCircleInscribe::ActionData {
    RS_CircleData cData;
    RS_Vector coord;
    std::vector<RS_Line *> lines;
};

// fixme - cleanup, optoins

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleInscribe::RS_ActionDrawCircleInscribe(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Draw circle inscribed",actionContext, RS2::ActionDrawCircleInscribe), m_actionData(std::make_unique<ActionData>()), m_valid(false){
}

RS_ActionDrawCircleInscribe::~RS_ActionDrawCircleInscribe() = default;

void RS_ActionDrawCircleInscribe::clearLines(bool checkStatus){
    while (!m_actionData->lines.empty()) {
        if (checkStatus && (int) m_actionData->lines.size() <= getStatus())
            break;
        m_actionData->lines.pop_back();
    }
}

void RS_ActionDrawCircleInscribe::drawSnapper() {
    // disable snapper
}

void RS_ActionDrawCircleInscribe::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }
    clearLines(true);
}

void RS_ActionDrawCircleInscribe::finish(bool updateTB){
    clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleInscribe::doTrigger() {
    auto *circle = new RS_Circle(m_container, m_actionData->cData);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }
    undoCycleAdd(circle);

    clearLines(false);
    setStatus(SetLine1);
    RS_DEBUG->print("RS_ActionDrawCircle4Line::trigger(): entity added: %lu", circle->getId());
}

void RS_ActionDrawCircleInscribe::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    for(RS_AtomicEntity* const pc: m_actionData->lines) { // highlight already selected
        highlightSelected(pc);
    }
    auto en = catchModifiableAndDescribe(e, RS2::EntityLine);  // fixme - check whether snap is used for entity selection?  Ensure free snap?

    if (en != nullptr){
        auto *line = dynamic_cast<RS_Line *>(en);
        switch (status) {
            case SetLine1: {
                highlightHover(en);
                break;
            }
            case SetLine2: {
                if (en != m_actionData->lines[SetLine1]){
                    highlightHover(en);
                }
                break;
            }
            case SetLine3: {
                if (m_actionData->lines[SetLine1] != line && m_actionData->lines[SetLine2] != line){
                    m_actionData->coord = e->graphPoint;
                    if (preparePreview(line)){
                        highlightHover(en);
                        previewToCreateCircle(m_actionData->cData);
                        if (m_showRefEntitiesOnPreview) {
                            RS_Vector &center = m_actionData->cData.center;
                            previewRefPoint(m_actionData->lines[SetLine1]->getNearestPointOnEntity(center, false));
                            previewRefPoint(m_actionData->lines[SetLine2]->getNearestPointOnEntity(center, false));
                            previewRefPoint(m_actionData->lines[SetLine3]->getNearestPointOnEntity(center, false));
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

void RS_ActionDrawCircleInscribe::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);
    if (!en) return;
    if (!(en->isVisible() && isLine(en))) return;
    for (int i = 0; i < status; i++) {
        if (en->getId() == m_actionData->lines[i]->getId()) return; //do not pull in the same line again
    }

    m_actionData->coord = e->graphPoint;
    auto *line = dynamic_cast<RS_Line *>(en);

    switch (status) {
        case SetLine1:{
            m_actionData->lines.push_back(line);
            setStatus(SetLine2);
            break;
        }
        case SetLine2:
            m_actionData->lines.push_back(line);
            setStatus(SetLine3);
            break;
        case SetLine3:
            if (preparePreview(line)){
                trigger();
            }
            break;
        default:
            break;
    }
}

void RS_ActionDrawCircleInscribe::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    if (status > 0){
        m_actionData->lines.pop_back();
        deletePreview();
    }
    initPrevious(status);
}

bool RS_ActionDrawCircleInscribe::preparePreview(RS_Line* en){
    m_valid = false;
    if (getStatus() == SetLine3){
        if (en != nullptr){
          m_actionData->lines.push_back(en);
        }
        RS_Circle c(m_preview.get(), m_actionData->cData);
        m_valid = c.createInscribe(m_actionData->coord, m_actionData->lines);
        if (m_valid){
            m_actionData->cData = c.getData();
        }
    }
    if (en != nullptr){
        m_actionData->lines.pop_back();
    }
    return m_valid;
}

void RS_ActionDrawCircleInscribe::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Specify the first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRBack(tr("Specify the second line"));
            break;
        case SetLine3:
            updateMouseWidgetTRBack(tr("Specify the third line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawCircleInscribe::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
