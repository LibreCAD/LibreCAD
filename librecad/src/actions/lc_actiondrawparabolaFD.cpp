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

#include <cmath>

#include <QAction>
#include <QMouseEvent>

#include "lc_actiondrawparabolaFD.h"

#include "lc_parabola.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

struct LC_ActionDrawParabolaFD::Points {
    RS_Vector focus, startPoint, endPoint, projection;
    RS_Vector axis, vertex;
    RS_Line* directrix = nullptr;
    LC_ParabolaData data;
    double h = 0.;
    bool valid = false;
    double getX(const RS_Vector& point) {
        auto vp = point - vertex;
        return vp.rotate(M_PI/2 - axis.angle()).x;
    }

    std::pair<RS_Vector, RS_Vector> fromX(double x) {
        return {RS_Vector{x, x*x/(4.*h)}.rotate(axis.angle() - M_PI/2) + vertex,
                    RS_Vector{2.*h, x}.rotate(axis.angle() - M_PI/2)};
    }
    bool SetEnd(const RS_Vector& point) {
        endPoint = point;
        double x0 = getX(startPoint);
        const auto& [p0, t0] = fromX(x0);
        double x1 = getX(point);
        const auto& [p1, t1] = fromX(x1);
        data = LC_ParabolaData::FromEndPointsTangents({p0, p1}, {t0, t1});
        return data.valid;
    }

    bool SetStart(const RS_Vector& point) {
        startPoint = point;
        double x = getX(startPoint);
        if (std::abs(x) > RS_TOLERANCE) {
            const auto& [p0, t0] = fromX(x);
            const auto& [p1, t1] = fromX(-x);
            data = LC_ParabolaData::FromEndPointsTangents({p0, p1}, {t0, t1});
        }
        return true;
    }

    bool SetDirectrix(RS_Line& line) {
        double dist=RS_MAXDOUBLE;
        projection = line.getNearestPointOnEntity(focus, false, &dist);
        vertex = (focus + projection) * 0.5;
        axis = focus - vertex;
        h = axis.magnitude();
        valid = h > RS_TOLERANCE;
        if (valid)
            directrix = &line;
        return valid;
    }
};

/**
 * Constructor.
 *
 */
LC_ActionDrawParabolaFD::LC_ActionDrawParabolaFD(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw parabola by focus and directrix", container,
                               graphicView,
                               RS2::ActionDrawParabolaFD)
    , pPoints(std::make_unique<Points>())
{
}

LC_ActionDrawParabolaFD::~LC_ActionDrawParabolaFD() = default;

void LC_ActionDrawParabolaFD::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (pPoints->directrix != nullptr) {
        switch(getStatus()) {
        case SetFocus:
        case SetDirectrix:
            pPoints->directrix->setHighlighted(false);
            graphicView->drawEntity(pPoints->directrix);
            pPoints->directrix = nullptr;
        }
    }
}

void LC_ActionDrawParabolaFD::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();
    if(pPoints->data.valid){
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
    init(SetFocus);
}

void LC_ActionDrawParabolaFD::mouseMoveEvent(QMouseEvent* e) {
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetFocus:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetDirectrix:
            mouse = snapFree(e);
            drawSnapper();
            break;
        case SetStartPoint:
            pPoints->SetStart(mouse);
            preparePreview();
            break;
        case SetEndPoint:
            pPoints->SetEnd(mouse);
            preparePreview();
            break;
        default:
            break;
    }
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
}

bool LC_ActionDrawParabolaFD::preparePreview()
{
    deletePreview();
    if (pPoints->data.valid) {
        auto* pl = new LC_Parabola{preview.get(), pPoints->data};
        preview->addEntity(pl);
        drawPreview();
    }
    return pPoints->valid;
}

void LC_ActionDrawParabolaFD::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        if (getStatus() == SetDirectrix) {
            snapFree(e);
            RS_Entity* entity = catchEntity(e, RS2::EntityLine);
            if (entity != nullptr) {
                if (pPoints->SetDirectrix(*static_cast<RS_Line*>(entity))) {
                    pPoints->directrix->setHighlighted(true);
                    graphicView->drawEntity(pPoints->directrix);
                    setStatus(getStatus()+1);
                }
            }
        }else{
            RS_CoordinateEvent ce{snapPoint(e)};
            coordinateEvent(&ce);
        }
    }

    // Return to last status:
    else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}


void LC_ActionDrawParabolaFD::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) {
        return;
    }
    RS_Vector mouse = e->getCoordinate();

    graphicView->moveRelativeZero(mouse);
    switch (getStatus()) {
    case SetFocus:
        pPoints->focus = mouse;
        setStatus(getStatus()+1);
        break;
    case SetStartPoint:
        pPoints->SetStart(mouse);
        preparePreview();
        setStatus(getStatus()+1);
        break;
    case SetEndPoint:
        if(pPoints->SetEnd(mouse))
            trigger();
        break;

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


QStringList LC_ActionDrawParabolaFD::getAvailableCommands() {
    return {};
}

void LC_ActionDrawParabolaFD::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetFocus:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the focus of parabola"),
                                            tr("Cancel"));
        break;

    case SetDirectrix:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the directrix of parabola"),
                                            tr("Back"));
        break;

    case SetStartPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the start point on parabola"),
                                            tr("Back"));
        break;

    case SetEndPoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the end point on parabola"),
                                            tr("Back"));
        break;


    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}



void LC_ActionDrawParabolaFD::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
