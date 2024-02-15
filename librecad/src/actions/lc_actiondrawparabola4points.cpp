/****************************************************************************
**
 * Draw ellipse by foci and a point on parabola

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

#include <QAction>
#include <QMouseEvent>

#include "lc_actiondrawparabola4points.h"

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "lc_parabola.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"

struct LC_ActionDrawParabola4Points::Points {
	RS_VectorSolutions points;
	RS_CircleData cData;
    std::vector<LC_ParabolaData> pData;
    LC_ParabolaData data;
    bool valid = false,evalid =false;
	bool m_bUniqueEllipse{false}; //a message of non-unique ellipse is shown
};

/**
 * Constructor.
 *
 */
LC_ActionDrawParabola4Points::LC_ActionDrawParabola4Points(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Draw ellipse from 4 points", container,
                               graphicView)
    , pPoints(std::make_unique<Points>())
{
    actionType=RS2::ActionDrawParabola4Points;
}

LC_ActionDrawParabola4Points::~LC_ActionDrawParabola4Points() = default;

void LC_ActionDrawParabola4Points::init(int status) {
    RS_PreviewActionInterface::init(status);
	if(getStatus() == SetPoint1) pPoints->points.clear();
}

void LC_ActionDrawParabola4Points::trigger() {
    RS_PreviewActionInterface::trigger();

    // update undo list:
    deletePreview();
    if(getStatus()==SetAxis && pPoints->evalid){
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
    if(preparePreview()) {
        switch(getStatus()) {
        case SetPoint2:
        case SetPoint3:
            break;
        case SetPoint4:
        {
           // pPoints->points = {{0., 1.}, {0.5, 0.25}, {1.5, 0.25}, {2., 1.}};
            auto pData = LC_ParabolaData::From4Points({pPoints->points.begin(), pPoints->points.end()});
            if (!pData.empty()) {
                pPoints->pData = std::vector<LC_ParabolaData>{pData.cbegin(), pData.cend()};
                deletePreview();
                double ds = RS_MAXDOUBLE;
                for(const auto& pd: pPoints->pData) {
                    auto* l = new RS_Line{preview.get(), {pd.focus, pd.vertex}};
                    double ds0 = RS_MAXDOUBLE;
                    l->getNearestPointOnEntity(mouse, false, &ds0);
                    preview->addEntity(l);
                    //auto* c0 = new RS_Circle{preview.get(), {pd.vertex, (pd.focus - pd.vertex).magnitude()/2.}};
                    //preview->addEntity(c0);
                    //auto* c1 = new RS_Circle{preview.get(), {pd.p1, (pd.focus - pd.vertex).magnitude()/2.}};
                    //preview->addEntity(c1);
                    // for (const auto& p0: pPoints->points) {
                    //     auto* c0 = new RS_Circle{preview.get(), {p0, (pd.focus - pd.vertex).magnitude()/2.}};
                    //     preview->addEntity(c0);
                    // }
                    if (ds0 < ds) {
                        pPoints->data = pd;
                        ds = ds0;
                        pPoints->evalid = true;
                    }
                }
                auto* pl = new LC_Parabola{preview.get(), pPoints->data};
                preview->addEntity(pl);
                drawPreview();
            }
        }
            break;
        case SetAxis:
        {
            deleteSnapper();
            RS_Vector m0 = snapFree(e);
            drawSnapper();
            pPoints->evalid = false;
            if(!pPoints->points.empty()){
                deletePreview();
                double ds = RS_MAXDOUBLE;
                for(const auto& pd: pPoints->pData) {
                    RS_Line* l = new RS_Line{preview.get(), pd.GetAxis()};
                    double ds0 = RS_MAXDOUBLE;
                    l->getNearestPointOnEntity(m0, false, &ds0);
                    if (ds0 < ds) {
                        pPoints->data = pd;
                        ds = ds0;
                        //LC_ERR<<"ds="<<ds0;
                        //LC_ERR<<"pd "<<l->getTangentDirection({}).angle();
                        pPoints->evalid = true;
                    }
                    preview->addEntity(l);
                }
                if (pPoints->evalid) {
                    LC_Parabola* e=new LC_Parabola{preview.get(), pPoints->data};
                    preview->addEntity(e);
                    const auto& c=pPoints->data.GetCurve();
                    for(size_t i=1; i<c.size(); ++i) {
                        RS_Line* l=new RS_Line{preview.get(), RS_LineData{c[i-1], c[i]}};
                        preview->addEntity(l);
                    }
                }
                drawPreview();
            }
        }
        default:
            break;
        }

    }
//    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
}


bool LC_ActionDrawParabola4Points::preparePreview(){
    return true;
    // pPoints->valid=false;
    switch(getStatus()) {
    case SetPoint2:
    case SetPoint3:
    case SetPoint4:
            break;
    case SetAxis:
    {

    }
        break;
    default:
        break;
    }
    return pPoints->valid;
}

void LC_ActionDrawParabola4Points::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
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
                return (data.startPoint - data.endPoint).magnitude() >= RS_TOLERANCE;
            });
            setStatus(getStatus()+1);
        }
    }
        break;

    case SetAxis:
    {
        deletePreview();
        double ds = RS_MAXDOUBLE;
        int i=0;
        for(const auto& pd: pPoints->pData) {
            RS_Line l{nullptr, pd.GetAxis()};
            // LC_ERR<<"Axis: "<<l.getStartpoint().x<<":"<<l.getStartpoint().y<<" | "
            //      <<l.getEndpoint().x<<":"<<l.getEndpoint().y;

            double ds0 = RS_MAXDOUBLE;
            l.getNearestPointOnEntity(mouse, false, &ds0);
            LC_ERR<<i++<<": ds0="<<ds0;
            if (ds0 < ds) {
                pPoints->data = pd;
                ds = ds0;
                pPoints->evalid = true;
            }
        }
        if (pPoints->evalid)
            trigger();
    }

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
