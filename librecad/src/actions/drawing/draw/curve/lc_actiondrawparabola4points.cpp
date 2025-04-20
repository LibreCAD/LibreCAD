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
    :RS_PreviewActionInterface("Draw parabola from 4 points", actionContext,RS2::ActionDrawParabola4Points),
     m_actionData(std::make_unique<ActionData>()){
}

LC_ActionDrawParabola4Points::~LC_ActionDrawParabola4Points() = default;

void LC_ActionDrawParabola4Points::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status == SetPoint1)
        m_actionData->points.clear();
}

void LC_ActionDrawParabola4Points::doTrigger() {
    if(m_actionData->valid){
        auto* en = new LC_Parabola{m_container, m_actionData->data};
        undoCycleAdd(en);
    }
    setStatus(SetPoint1);
}

void LC_ActionDrawParabola4Points::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
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
            RS_Vector m0 = e->graphPoint;
            preparePreview(m0,false);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawParabola4Points::preparePreview(const RS_Vector& mouse, bool rebuild){
    m_actionData->valid = false;
    if (rebuild|| m_actionData->pData.empty()) {
        m_actionData->pData = LC_ParabolaData::From4Points({m_actionData->points.begin(), m_actionData->points.end()});
    }
    if (!m_actionData->pData.empty()) {
        double ds = RS_MAXDOUBLE;
        for(const auto& pd: m_actionData->pData) {
            if (pd.valid) {
                const RS_LineData &axis = pd.GetAxis();
                auto *l = previewRefLine(axis.startpoint, axis.endpoint);
                double ds0 = RS_MAXDOUBLE;
                l->getNearestPointOnEntity(mouse, false, &ds0);
                if (ds0 < ds) {
                    m_actionData->data = pd;
                    ds = ds0;
                    m_actionData->valid = true;
                }
            }
        }
        auto* pl = new LC_Parabola{m_preview.get(), m_actionData->data};
        previewEntity(pl);
    }
    return m_actionData->valid;
}

void LC_ActionDrawParabola4Points::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    // Proceed to next status
    const RS_Vector &coord = getStatus() != SetAxis ? e->snapPoint : e->graphPoint;
    fireCoordinateEvent(coord);
}

void LC_ActionDrawParabola4Points::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    deletePreview();
    initPrevious(status);
    status = getStatus();
    m_actionData->points.resize(status+1);
    if (!m_actionData->points.empty()) {
        moveRelativeZero(m_actionData->points.at(status));
    }
}

void LC_ActionDrawParabola4Points::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    int nextStatus = status + 1;
    m_actionData->points.resize(nextStatus);
    m_actionData->points.set(status,mouse);

    switch (status) {
        case SetPoint1:
        case SetPoint2:
        case SetPoint3: {
            moveRelativeZero(mouse);
            setStatus(nextStatus);
            break;
        }
        case SetPoint4:    {
            // reject the same point
            if ((m_actionData->points.at(SetPoint4) - m_actionData->points.at(SetPoint3)).magnitude() < RS_TOLERANCE){
                break;
            }
            auto pData = LC_ParabolaData::From4Points({m_actionData->points.begin(), m_actionData->points.end()});
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
        }
        default:
            break;
    }
}

//fixme, support command line

QStringList LC_ActionDrawParabola4Points::getAvailableCommands() {
    return {};
}

void LC_ActionDrawParabola4Points::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify the first point on parabola"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify the second point on parabola"));
            break;
        case SetPoint3:
            updateMouseWidgetTRBack(tr("Specify the third point on parabola"));
            break;
        case SetPoint4:
            updateMouseWidgetTRBack(tr("Specify the fourth point on parabola"));
            break;
        case SetAxis:
            updateMouseWidgetTRBack(tr("Specify the Axis on parabola"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType LC_ActionDrawParabola4Points::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
