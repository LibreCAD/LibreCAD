/****************************************************************************
**
Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)

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

#include "lc_actiondrawparabola4points.h"

#include "lc_parabola.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_preview.h"

struct LC_ActionDrawParabola4Points::ActionData {
    RS_VectorSolutions points;
    std::vector<LC_ParabolaData> pData;
    LC_ParabolaData data;
    bool valid = false;
};

/**
 * Constructor.
 *
 */
LC_ActionDrawParabola4Points::LC_ActionDrawParabola4Points(LC_ActionContext *actionContext)
    :LC_SingleEntityCreationAction("ActionDrawParabola4Points", actionContext,RS2::ActionDrawParabola4Points),
     m_actionData(std::make_unique<ActionData>()){
}

LC_ActionDrawParabola4Points::~LC_ActionDrawParabola4Points() = default;

void LC_ActionDrawParabola4Points::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (status == SetPoint1) {
        m_actionData->points.clear();
    }
}

RS_Entity* LC_ActionDrawParabola4Points::doTriggerCreateEntity() {
    if(m_actionData->valid){
        auto* en = new LC_Parabola{m_document, m_actionData->data};
        return en;
    }
    return nullptr;
}

void LC_ActionDrawParabola4Points::doTriggerCompletion([[maybe_unused]]bool success) {
    setStatus(SetPoint1);
}

void LC_ActionDrawParabola4Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    m_actionData->points.set(status, mouse);
    if (m_showRefEntitiesOnPreview) {
        for (int i = 0; i < status;i++) {
            previewRefPoint(m_actionData->points.at(i));
        }
    }
    switch(status) {
        case SetPoint1:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetPoint2:
        case SetPoint3:
            break;
        case SetPoint4:
            preparePreview(mouse, true);
            break;
        case SetAxis:{
            const RS_Vector m0 = e->graphPoint;
            preparePreview(m0,false);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawParabola4Points::preparePreview(const RS_Vector& mouse, const bool rebuild) const {
    m_actionData->valid = false;
    if (rebuild|| m_actionData->pData.empty()) {
        m_actionData->pData = LC_ParabolaData::From4Points({m_actionData->points.begin(), m_actionData->points.end()});
    }
    if (!m_actionData->pData.empty()) {
        double ds = RS_MAXDOUBLE;
        for(const auto& pd: m_actionData->pData) {
            if (pd.valid) {
                const RS_LineData &axis = pd.getAxis();
                const auto *l = obtainPreviewRefLine(axis.startpoint, axis.endpoint);
                double ds0 = RS_MAXDOUBLE;
                l->getNearestPointOnEntity(mouse, false, &ds0);
                if (ds0 < ds) {
                    m_actionData->data = pd;
                    ds = ds0;
                    m_actionData->valid = true;
                }
            }
        }
        const auto* pl = new LC_Parabola{m_preview.get(), m_actionData->data};
        previewEntity(pl);
    }
}

void LC_ActionDrawParabola4Points::onMouseLeftButtonRelease([[maybe_unused]]int status, const LC_MouseEvent* e) {
    // Proceed to next status
    const RS_Vector &coord = getStatus() != SetAxis ? e->snapPoint : e->graphPoint;
    fireCoordinateEvent(coord);
}

void LC_ActionDrawParabola4Points::onMouseRightButtonRelease(int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    deletePreview();
    initPrevious(status);
    status = getStatus();
    m_actionData->points.resize(status+1);
    if (!m_actionData->points.empty()) {
        moveRelativeZero(m_actionData->points.at(status));
    }
}

bool LC_ActionDrawParabola4Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2) || (status == SetPoint3) || (status == SetPoint4);
}

void LC_ActionDrawParabola4Points::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    const int nextStatus = status + 1;
    m_actionData->points.resize(nextStatus);
    m_actionData->points.set(status,coord);

    switch (status) {
        case SetPoint1:
        case SetPoint2:
        case SetPoint3: {
            moveRelativeZero(coord);
            setStatus(nextStatus);
            break;
        }
        case SetPoint4:    {
            // reject the same point
            if ((m_actionData->points.at(SetPoint4) - m_actionData->points.at(SetPoint3)).magnitude() < RS_TOLERANCE){
                break;
            }
            const auto pData = LC_ParabolaData::From4Points({m_actionData->points.begin(), m_actionData->points.end()});
            if (!pData.empty()) {
                m_actionData->pData.clear();
                std::copy_if(pData.cbegin(), pData.cend(), std::back_inserter(m_actionData->pData), [](const LC_ParabolaData& data){
                    return data.valid;
                });
                if (m_actionData->pData.size() == 1) { // shortcut for single solution
                    m_actionData->data = m_actionData->pData.front();
                    trigger();
                } else {
                    setStatus(nextStatus);
                }
            }
            break;
        }
        case SetAxis: {
            trigger();
            break;
        }
        default:
            break;
    }
}

//fixme, support command line

QStringList LC_ActionDrawParabola4Points::getAvailableCommands() {
    return {};
}

void LC_ActionDrawParabola4Points::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint1:
            updatePromptTRCancel(tr("Specify the first point on parabola"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify the second point on parabola"));
            break;
        case SetPoint3:
            updatePromptTRBack(tr("Specify the third point on parabola"));
            break;
        case SetPoint4:
            updatePromptTRBack(tr("Specify the fourth point on parabola"));
            break;
        case SetAxis:
            updatePromptTRBack(tr("Specify the Axis on parabola"));
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionDrawParabola4Points::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
