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

#include <QMouseEvent>

#include "lc_actiondrawparabola4points.h"

#include "lc_parabola.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

struct LC_ActionDrawParabola4Points::Points {
    RS_VectorSolutions points;
    std::vector<LC_ParabolaData> pData;
    LC_ParabolaData data;
    bool valid = false;
};

/**
 * Constructor.
 *
 */
LC_ActionDrawParabola4Points::LC_ActionDrawParabola4Points(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw parabola from 4 points", container,
                               graphicView,
                               RS2::ActionDrawParabola4Points),
     pPoints(std::make_unique<Points>()){
}

LC_ActionDrawParabola4Points::~LC_ActionDrawParabola4Points() = default;

void LC_ActionDrawParabola4Points::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status == SetPoint1)
        pPoints->points.clear();
}

void LC_ActionDrawParabola4Points::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();
    if(pPoints->valid){
        auto* en = new LC_Parabola{container, pPoints->data};
        container->addEntity(en);
        addToDocumentUndoable(en);
    }
    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    moveRelativeZero(rz);
    setStatus(SetPoint1);
}

void LC_ActionDrawParabola4Points::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    int status = getStatus();
    pPoints->points.set(status, mouse);
    if (showRefEntitiesOnPreview) {
        for (int i = 0; i < status;i++) {
            previewRefPoint(pPoints->points.at(i));
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
            RS_Vector m0 = toGraph(e);
            preparePreview(m0,false);
            break;
        }
        default:
            break;
    }
    drawPreview();
}

bool LC_ActionDrawParabola4Points::preparePreview(const RS_Vector& mouse, bool rebuild){
    pPoints->valid = false;
    if (rebuild|| pPoints->pData.empty()) {
        pPoints->pData = LC_ParabolaData::From4Points({pPoints->points.begin(), pPoints->points.end()});
    }
    if (!pPoints->pData.empty()) {
        double ds = RS_MAXDOUBLE;
        for(const auto& pd: pPoints->pData) {
            if (pd.valid) {
                const RS_LineData &axis = pd.GetAxis();
                auto *l = previewRefLine(axis.startpoint, axis.endpoint);
                double ds0 = RS_MAXDOUBLE;
                l->getNearestPointOnEntity(mouse, false, &ds0);
                if (ds0 < ds) {
                    pPoints->data = pd;
                    ds = ds0;
                    pPoints->valid = true;
                }
            }
        }
        auto* pl = new LC_Parabola{preview.get(), pPoints->data};
        previewEntity(pl);
    }
    return pPoints->valid;
}

void LC_ActionDrawParabola4Points::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    // Proceed to next status
    const RS_Vector &coord = getStatus() != SetAxis ? snapPoint(e) : snapFree(e);
    fireCoordinateEvent(coord);
}

void LC_ActionDrawParabola4Points::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    // Return to last status:
    deletePreview();
    initPrevious(status);
    status = getStatus();
    pPoints->points.resize(status+1);
    if (!pPoints->points.empty()) {
        moveRelativeZero(pPoints->points.at(status));
    }
}

void LC_ActionDrawParabola4Points::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    int nextStatus = status + 1;
    pPoints->points.resize(nextStatus);
    pPoints->points.set(status,mouse);

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
            if ((pPoints->points.at(SetPoint4) - pPoints->points.at(SetPoint3)).magnitude() < RS_TOLERANCE){
                break;
            }
            auto pData = LC_ParabolaData::From4Points({pPoints->points.begin(), pPoints->points.end()});
            if (!pData.empty()) {
                pPoints->pData.clear();
                std::copy_if(pData.cbegin(), pData.cend(), std::back_inserter(pPoints->pData), [](const LC_ParabolaData& data){
                    return data.valid;
                });
                if (pPoints->pData.size() == 1) { // shortcut for single solution
                    pPoints->data = pPoints->pData.front();
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
