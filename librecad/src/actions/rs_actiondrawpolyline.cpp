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
#include<cmath>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawpolyline.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_debug.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

struct RS_ActionDrawPolyline::Points {

	/**
	 * Line data defined so far.
	 */
	RS_PolylineData data;
	RS_ArcData arc_data;
	/**
	 * Polyline entity we're working on.
	 */
	RS_Polyline* polyline;

	/**
	 * last point.
	 */
	RS_Vector point;
	RS_Vector calculatedEndpoint;
	/**
	 * Start point of the series of lines. Used for close function.
	 */
	RS_Vector start;

	/**
	 * Point history (for undo)
	 */
		QList<RS_Vector> history;

	/**
	 * Bulge history (for undo)
	 */
		QList<double> bHistory;
};

RS_ActionDrawPolyline::RS_ActionDrawPolyline(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw polylines",
						   container, graphicView)
		,m_Reversed(1)
		, pPoints(new Points{})
{
	actionType=RS2::ActionDrawPolyline;
    reset();
}



RS_ActionDrawPolyline::~RS_ActionDrawPolyline() = default;


void RS_ActionDrawPolyline::reset() {
		pPoints->polyline = nullptr;
	pPoints->data = { {}, {}, false};
	pPoints->start = {};
	pPoints->history.clear();
	pPoints->bHistory.clear();
}



void RS_ActionDrawPolyline::init(int status) {
    reset();
    RS_PreviewActionInterface::init(status);

}



void RS_ActionDrawPolyline::trigger() {
    RS_PreviewActionInterface::trigger();

	if (!pPoints->polyline) return;

        // add the entity
    //RS_Polyline* polyline = new RS_Polyline(container, data);
    //polyline->setLayerToActive();
    //polyline->setPenToActive();
    //container->addEntity(polyline);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
		document->addUndoable(pPoints->polyline);
        document->endUndoCycle();
    }

        // upd view
    deleteSnapper();
	graphicView->moveRelativeZero({0.,0.});
	graphicView->drawEntity(pPoints->polyline);
	graphicView->moveRelativeZero(pPoints->polyline->getEndpoint());
    drawSnapper();
    RS_DEBUG->print("RS_ActionDrawLinePolyline::trigger(): polyline added: %d",
					pPoints->polyline->getId());

	pPoints->polyline = nullptr;
}



void RS_ActionDrawPolyline::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    double bulge=solveBulge(mouse);
	if (getStatus()==SetNextPoint && pPoints->point.valid) {
        deletePreview();
        // clearPreview();

                //RS_Polyline* p = polyline->clone();
                //p->reparent(preview);
                //preview->addEntity(p);
        if (fabs(bulge)<RS_TOLERANCE || Mode==Line) {
			preview->addEntity(new RS_Line{preview.get(), pPoints->point, mouse});
        } else
			preview->addEntity(new RS_Arc(preview.get(), pPoints->arc_data));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent end");
}



void RS_ActionDrawPolyline::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
                if (getStatus()==SetNextPoint) {
                        trigger();
                }
        deletePreview();
        // clearPreview();
        deleteSnapper();
        init(getStatus()-1);
    }
}

double RS_ActionDrawPolyline::solveBulge(RS_Vector mouse) {

    double b(0.);
    bool suc;
	RS_Arc arc{};
	RS_Line line{};
	double direction;
    RS_AtomicEntity* lastentity;
    calculatedSegment=false;

    switch (Mode){
//     case Line:
//        b=0.0;
//        break;
     case Tangential:
		if (pPoints->polyline){
			lastentity = static_cast<RS_AtomicEntity*>(pPoints->polyline->lastEntity());
            direction = RS_Math::correctAngle(
                lastentity->getDirection2()+M_PI);
			line.setStartpoint(pPoints->point);
            line.setEndpoint(mouse);
			double const direction2=RS_Math::correctAngle(line.getDirection2()+M_PI);
			double const delta=direction2-direction;
            if( fabs(remainder(delta,M_PI))>RS_TOLERANCE_ANGLE ) {
                b=tan(delta/2);
				suc = arc.createFrom2PBulge(pPoints->point,mouse,b);
                if (suc)
					pPoints->arc_data = arc.getData();
                else
                    b=0;
            }
            break;
//            if(delta<RS_TOLERANCE_ANGLE ||
//                (delta<M_PI+RS_TOLERANCE_ANGLE &&
//                delta>M_PI-RS_TOLERANCE_ANGLE))
//                b=0;
//            else{
//                b=tan((direction2-direction)/2);
//                suc = arc.createFrom2PBulge(point,mouse,b);
//                if (suc)
//                    arc_data = arc.getData();
//                else
//                    b=0;
//            }
        }
//        else
//            b=0;
//        break;
        // fall-through
     case TanRad:
		if (pPoints->polyline){
			lastentity = static_cast<RS_AtomicEntity*>(pPoints->polyline->lastEntity());
            direction = RS_Math::correctAngle(
                lastentity->getDirection2()+M_PI);
			suc = arc.createFrom2PDirectionRadius(pPoints->point, mouse,
                direction,Radius);
            if (suc){
				pPoints->arc_data = arc.getData();
                b=arc.getBulge();
				pPoints->calculatedEndpoint = arc.getEndpoint();
                calculatedSegment=true;

            }
//            else
//                b=0;
        }
//        else
//          b=0;
        break;
/*     case TanAng:
        b=tan(Reversed*Angle*M_PI/720.0);
        break;
     case TanRadAng:
        b=tan(Reversed*Angle*M_PI/720.0);
        break;*/
    case Ang:
		b=tan(m_Reversed*Angle*M_PI/720.0);
		suc = arc.createFrom2PBulge(pPoints->point,mouse,b);
        if (suc)
			pPoints->arc_data = arc.getData();
		else
            b=0;
        break;
    default:
        break;
        /*     case RadAngEndp:
        b=tan(Reversed*Angle*M_PI/720.0);
        break;
     case RadAngCenp:
        b=tan(Reversed*Angle*M_PI/720.0);*/
    }
    return b;
}

void RS_ActionDrawPolyline::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    double bulge=solveBulge(mouse);
    if (calculatedSegment)
		mouse=pPoints->calculatedEndpoint;

    switch (getStatus()) {
    case SetStartpoint:
        //	data.startpoint = mouse;
        //printf ("SetStartpoint\n");
		pPoints->point = mouse;
		pPoints->history.clear();
		pPoints->history.append(mouse);
		pPoints->bHistory.clear();
		pPoints->bHistory.append(0.0);
		pPoints->start = pPoints->point;
        setStatus(SetNextPoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetNextPoint:
        graphicView->moveRelativeZero(mouse);
		pPoints->point = mouse;
		pPoints->history.append(mouse);
		pPoints->bHistory.append(bulge);
				if (!pPoints->polyline) {
						pPoints->polyline = new RS_Polyline(container, pPoints->data);
						pPoints->polyline->addVertex(pPoints->start, 0.0);
                }
				if (pPoints->polyline) {
						pPoints->polyline->setNextBulge(bulge);
						pPoints->polyline->addVertex(mouse, 0.0);
						pPoints->polyline->setEndpoint(mouse);
						if (pPoints->polyline->count()==1) {
						pPoints->polyline->setLayerToActive();
						pPoints->polyline->setPenToActive();
								container->addEntity(pPoints->polyline);
                        }
                        deletePreview();
                        // clearPreview();
                        deleteSnapper();
						graphicView->drawEntity(pPoints->polyline);
                        drawSnapper();
                }
        //trigger();
        //data.startpoint = data.endpoint;
        updateMouseButtonHints();
        //graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
}

void RS_ActionDrawPolyline::setMode(SegmentMode m) {
	Mode=m;
}

int RS_ActionDrawPolyline::getMode() const{
	return Mode;
}

void RS_ActionDrawPolyline::setRadius(double r) {
	Radius=r;
}

double RS_ActionDrawPolyline::getRadius() const{
	return Radius;
}

void RS_ActionDrawPolyline::setAngle(double a) {
	Angle=a;
}

double RS_ActionDrawPolyline::getAngle() const{
	return Angle;
}

void RS_ActionDrawPolyline::setReversed( bool c) {
	m_Reversed=c?-1:1;
}

bool RS_ActionDrawPolyline::isReversed() const{
	return m_Reversed==-1;
}


void RS_ActionDrawPolyline::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
            return;
        }
        break;

    case SetNextPoint:
        if (checkCommand("close", c)) {
            close();
            e->accept();
            updateMouseButtonHints();
            return;
        }

        if (checkCommand("undo", c)) {
            undo();
            e->accept();
            updateMouseButtonHints();
            return;
        }
        break;

    default:
        break;
    }
}



QStringList RS_ActionDrawPolyline::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetNextPoint:
		if (pPoints->history.size()>=2) {
            cmd += command("undo");
        }
		if (pPoints->history.size()>=3) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}



void RS_ActionDrawPolyline::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetNextPoint: {
            QString msg = "";

			if (pPoints->history.size()>=3) {
                msg += RS_COMMANDS->command("close");
                msg += "/";
            }
			if (pPoints->history.size()>=2) {
                msg += RS_COMMANDS->command("undo");
            }

			if (pPoints->history.size()>=2) {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point or [%1]").arg(msg),
                    tr("Back"));
            } else {
                RS_DIALOGFACTORY->updateMouseWidget(
                    tr("Specify next point"),
                    tr("Back"));
            }
        }
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}


void RS_ActionDrawPolyline::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionDrawPolyline::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawPolyline::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

void RS_ActionDrawPolyline::close() {
	if (pPoints->history.size()>2 && pPoints->start.valid) {
		//data.endpoint = start;
		//trigger();
		if (pPoints->polyline) {
			if (Mode==TanRad)
				Mode=Line;
			RS_CoordinateEvent e(pPoints->polyline->getStartpoint());
			coordinateEvent(&e);
			pPoints->polyline->setClosed(true);
		}
		trigger();
        setStatus(SetStartpoint);
		graphicView->moveRelativeZero(pPoints->start);
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot close sequence of lines: "
               "Not enough entities defined yet."));
    }
}

void RS_ActionDrawPolyline::undo() {
	if (pPoints->history.size()>1) {
		pPoints->history.removeLast();
		pPoints->bHistory.removeLast();
        deletePreview();
		pPoints->point = pPoints->history.last();

		if(pPoints->history.size()==1){
			graphicView->moveRelativeZero(pPoints->history.front());
            //remove polyline from container,
            //container calls delete over polyline
			container->removeEntity(pPoints->polyline);
			pPoints->polyline = nullptr;
			graphicView->drawEntity(pPoints->polyline);
        }
		if (pPoints->polyline) {
			pPoints->polyline->removeLastVertex();
			graphicView->moveRelativeZero(pPoints->polyline->getEndpoint());
			graphicView->drawEntity(pPoints->polyline);
        }
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot undo: "
               "Not enough entities defined yet."));
    }
}

// EOF
