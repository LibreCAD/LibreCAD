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

#include <QAction>
#include <QMouseEvent>

#include "lc_actiondrawparabola4points.h"

#include "lc_parabola.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
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
                               RS2::ActionDrawParabola4Points)
    , pPoints(std::make_unique<Points>())
{
}

LC_ActionDrawParabola4Points::~LC_ActionDrawParabola4Points() = default;

void LC_ActionDrawParabola4Points::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (getStatus() == SetPoint1)
        pPoints->points.clear();
}

void LC_ActionDrawParabola4Points::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();
    if(getStatus()==SetAxis && pPoints->valid){
        LC_Parabola* en = new LC_Parabola{container, pPoints->data};
        container->addEntity(en);
        if (document) {
            document->startUndoCycle();
            document->addUndoable(en);
            document->endUndoCycle();
        }
    }
    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(rz);
    drawSnapper();
    setStatus(SetPoint1);
}

void LC_ActionDrawParabola4Points::mouseMoveEvent(QMouseEvent* e) {
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    pPoints->points.set(getStatus(),mouse);
    switch(getStatus()) {
    case SetPoint2:
    case SetPoint3:
        break;
    case SetPoint4:
        preparePreview(mouse);
        break;
    case SetAxis:
    {
        deleteSnapper();
        RS_Vector m0 = snapFree(e);
        drawSnapper();
        preparePreview(m0);
    }
        break;
    default:
        break;
    }
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
}

bool LC_ActionDrawParabola4Points::preparePreview(const RS_Vector& mouse){
    deletePreview();
    pPoints->valid = false;
    if (getStatus() == SetPoint4 || pPoints->pData.empty())
        pPoints->pData = LC_ParabolaData::From4Points({pPoints->points.begin(), pPoints->points.end()});
    if (!pPoints->pData.empty()) {
        double ds = RS_MAXDOUBLE;
        for(const auto& pd: pPoints->pData) {
            if (!pd.valid)
                continue;
            auto* l = new RS_Line{preview.get(), pd.GetAxis()};
            double ds0 = RS_MAXDOUBLE;
            l->getNearestPointOnEntity(mouse, false, &ds0);
            preview->addEntity(l);
            if (ds0 < ds) {
                pPoints->data = pd;
                ds = ds0;
                pPoints->valid = true;
            }
        }
        auto* pl = new LC_Parabola{preview.get(), pPoints->data};
        preview->addEntity(pl);
        drawPreview();
    }
    return pPoints->valid;
}

void LC_ActionDrawParabola4Points::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce{getStatus() != SetAxis ? snapPoint(e) : snapFree(e)};
        coordinateEvent(&ce);
    }

    // Return to last status:
    else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
        pPoints->points.resize(getStatus()+1);
        if (!pPoints->points.empty()) {
            graphicView->moveRelativeZero(pPoints->points.at(getStatus()));
        }
    }
}


void LC_ActionDrawParabola4Points::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();
    pPoints->points.resize(getStatus()+1);
    pPoints->points.set(getStatus(),mouse);

    switch (getStatus()) {
    case SetPoint1:
    case SetPoint2:
    case SetPoint3:
        graphicView->moveRelativeZero(mouse);
        setStatus(getStatus()+1);
        break;
    case SetPoint4:
    {
        // reject the same point
        if ((pPoints->points.at(SetPoint4) - pPoints->points.at(SetPoint3)).magnitude() < RS_TOLERANCE)
            break;
        auto pData = LC_ParabolaData::From4Points({pPoints->points.begin(), pPoints->points.end()});
        if (!pData.empty()) {
            pPoints->pData.clear();
            std::copy_if(pData.cbegin(), pData.cend(), std::back_inserter(pPoints->pData), [](const LC_ParabolaData& data){
                return data.valid;
            });
            if (pPoints->pData.size() == 1) {
                pPoints->data = pPoints->pData.front();
                trigger();
            } else {
                setStatus(getStatus()+1);
            }
        }
    }
        break;

    case SetAxis:
        trigger();

    default:
        break;
    }
}

//fixme, support command line

/*
void RS_ActionDrawEllipse4Point::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    default:
        break;
    }
}
*/


QStringList LC_ActionDrawParabola4Points::getAvailableCommands() {
    return {};
}

void LC_ActionDrawParabola4Points::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first point on parabola"),
                                            tr("Cancel"));
        break;

    case SetPoint2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second point on parabola"),
                                            tr("Back"));
        break;

    case SetPoint3:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the third point on parabola"),
                                            tr("Back"));
        break;

    case SetPoint4:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the fourth point on parabola"),
                                            tr("Back"));
        break;

    case SetAxis:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the Axis on parabola"),
                                            tr("Back"));
        break;

    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void LC_ActionDrawParabola4Points::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
