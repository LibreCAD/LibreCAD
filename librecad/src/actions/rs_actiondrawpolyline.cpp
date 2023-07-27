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

#include <cmath>
#include <QAction>
#include <QMouseEvent>

#include "rs_actiondrawpolyline.h"

#include "rs_arc.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_preview.h"

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
		,m_reversed(1)
		, pPoints(std::make_unique<Points>())
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
    RS_DEBUG->print("RS_ActionDrawLinePolyline::trigger(): polyline added: %lu",
					pPoints->polyline->getId());

	pPoints->polyline = nullptr;
}


void RS_ActionDrawPolyline::mouseMoveEvent(QMouseEvent* e)
{
    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    double bulge=solveBulge(mouse);

	if (getStatus()==SetNextPoint && pPoints->point.valid) {
        deletePreview();
        // clearPreview();

                //RS_Polyline* p = polyline->clone();
                //p->reparent(preview);
                //preview->addEntity(p);
        if (fabs(bulge)<RS_TOLERANCE || m_mode==Line) {
			preview->addEntity(new RS_Line{preview.get(), pPoints->point, mouse});
        } else
			preview->addEntity(new RS_Arc(preview.get(), pPoints->arc_data));
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLinePolyline::mouseMoveEvent end");
}


void RS_ActionDrawPolyline::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        if (equationSettingOn   || stepSizeSettingOn) return;

        if (startPointSettingOn || endPointSettingOn)
        {
            QString pointNumberString(QString::number(snapPoint(e).x));

            if (e->modifiers() == Qt::ControlModifier)
            {
                pointNumberString = QString::number(snapPoint(e).x - graphicView->getRelativeZero().x).prepend("@@");
            }

            RS_CommandEvent equationCommandEventObject(pointNumberString);
            commandEvent(&equationCommandEventObject);
            return;
        }

        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton)
    {
        if (equationSettingOn || startPointSettingOn || endPointSettingOn || stepSizeSettingOn)
        {
            equationSettingOn   = false;
            startPointSettingOn = false;
            endPointSettingOn   = false;
            stepSizeSettingOn   = false;
            return;
        }

        if (getStatus() == SetNextPoint) trigger();
        deletePreview();
        deleteSnapper();
        init(getStatus()-1);
    }
}


double RS_ActionDrawPolyline::solveBulge(RS_Vector mouse) {

    double b(0.);
    bool suc = false;
	RS_Arc arc{};
	RS_Line line{};
	double direction;
    RS_AtomicEntity* lastentity = nullptr;
    m_calculatedSegment=false;

    switch (m_mode){
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
            if( std::abs(std::remainder(delta,M_PI))>RS_TOLERANCE_ANGLE ) {
                b = std::tan(delta/2);
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
//                b = std::tan((direction2-direction)/2);
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
                direction,m_radius);
            if (suc){
				pPoints->arc_data = arc.getData();
                b=arc.getBulge();
				pPoints->calculatedEndpoint = arc.getEndpoint();
                m_calculatedSegment=true;

            }
//            else
//                b=0;
        }
//        else
//          b=0;
        break;
/*     case TanAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;
     case TanRadAng:
        b = std::tan(Reversed*m_angle*M_PI/720.0);
        break;*/
    case Ang:
		b = std::tan(m_reversed*m_angle*M_PI/720.0);
		suc = arc.createFrom2PBulge(pPoints->point,mouse,b);
        if (suc)
			pPoints->arc_data = arc.getData();
		else
            b=0;
        break;
    default:
        break;
        /*     case RadAngEndp:
        b=tan(Reversed*m_angle*M_PI/720.0);
        break;
     case RadAngCenp:
        b=tan(Reversed*m_angle*M_PI/720.0);*/
    }
    return b;
}

void RS_ActionDrawPolyline::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    double bulge=solveBulge(mouse);
    if (m_calculatedSegment)
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
	m_mode=m;
}

int RS_ActionDrawPolyline::getMode() const{
	return m_mode;
}

void RS_ActionDrawPolyline::setRadius(double r) {
	m_radius=r;
}

double RS_ActionDrawPolyline::getRadius() const{
	return m_radius;
}

void RS_ActionDrawPolyline::setAngle(double a) {
	m_angle=a;
}

double RS_ActionDrawPolyline::getAngle() const{
	return m_angle;
}

void RS_ActionDrawPolyline::setReversed( bool c) {
	m_reversed=c?-1:1;
}

bool RS_ActionDrawPolyline::isReversed() const{
	return m_reversed==-1;
}


void RS_ActionDrawPolyline::commandEvent(RS_CommandEvent* e)
{
    QString c = e->getCommand().toLower().replace(" ", "");

    switch (getStatus())
    {
        case SetStartpoint:
            if (checkCommand("help", c))
            {
                RS_DIALOGFACTORY->commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
                return;
            }
            if (checkCommand("close", c))
            {
                e->accept();
                close();
                return;
            }
            break;

        case SetNextPoint:
            if (checkCommand("undo", c))
            {
                undo();
                e->accept();
                updateMouseButtonHints();
                return;
            }
            if (checkCommand("close", c))
            {
                e->accept();
                close();
                return;
            }
            break;

        default:
            break;
    }

    if ((m_mode == Line) && (checkCommand("equation", c)))
    {
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter an equation, f(x)"));
        equationSettingOn = true;
        e->accept();
        return;
    }

    if (equationSettingOn)
    {
        equationSettingOn = false;

        shiftX = false;

        try
        {
            QString cRef = c;

            const QString someRandomNumber = "123.456";

            cRef.replace(tr("x"), someRandomNumber);

            m_muParserObject = std::make_unique<mu::Parser>();
            m_muParserObject->DefineConst(_T("e"),  M_E);
            m_muParserObject->DefineConst(_T("pi"), M_PI);

            m_muParserObject->SetExpr(cRef.toStdString());

            const double parseTestValue = m_muParserObject->Eval();

            if (parseTestValue) { /* This is to counter the 'unused variable' warning. */ }

            RS_DIALOGFACTORY->updateMouseWidget(tr("Enter the start point"));

            startPointSettingOn = true;

            polyEquation = c;
        }
        catch (...)
        {
            RS_DIALOGFACTORY->commandMessage(tr("The equation entered is invalid."));
            updateMouseButtonHints();
        }

        e->accept();
        return;
    }

    if (startPointSettingOn)
    {
        startPointSettingOn = false;

        try
        {
            bool isRelative = false;

            if (c.startsWith("@")) isRelative = true;

            if (c.startsWith("@@")) shiftX = true;

            m_muParserObject->SetExpr(c.remove("@").toStdString());
            startPoint = m_muParserObject->Eval();

            if (isRelative) startPoint += graphicView->getRelativeZero().x;

            RS_DIALOGFACTORY->updateMouseWidget(tr("Enter the end point"));

            endPointSettingOn = true;
        }
        catch (...)
        {
            RS_DIALOGFACTORY->commandMessage(tr("The start point entered is invalid."));
            updateMouseButtonHints();
        }

        e->accept();
        return;
    }

    if (endPointSettingOn)
    {
        endPointSettingOn = false;

        try
        {
            bool isRelative = false;

            if (c.startsWith("@")) isRelative = true;

            if (c.startsWith("@@")) shiftX = true;

            m_muParserObject->SetExpr(c.remove("@").toStdString());
            endPoint = m_muParserObject->Eval();

            if (isRelative) endPoint += graphicView->getRelativeZero().x;

            if (endPoint == startPoint) throw -1;

            RS_DIALOGFACTORY->updateMouseWidget(tr("Enter the number of polylines"));

            stepSizeSettingOn = true;
        }
        catch (...)
        {
            RS_DIALOGFACTORY->commandMessage(tr("The end point entered is invalid."));
            updateMouseButtonHints();
        }

        e->accept();
        return;
    }

    if (stepSizeSettingOn)
    {
        stepSizeSettingOn = false;

        int numberOfPolylines;

        try
        {
            m_muParserObject->SetExpr(c.toStdString());
            numberOfPolylines = (int) trunc(m_muParserObject->Eval());

            if (numberOfPolylines <= 0) throw -1;
        }
        catch (...)
        {
            RS_DIALOGFACTORY->commandMessage(tr("The step size entered is invalid."));
            updateMouseButtonHints();

            e->accept();
            return;
        }

        deleteSnapper();

        const double stepSize = (endPoint - startPoint) / (double) numberOfPolylines;

        const int direction = (stepSize < 0) ? -1 : 1;

        double equation_xTerm = 0.0;
        m_muParserObject->DefineVar(_T("x"), &equation_xTerm);

        m_muParserObject->SetExpr(polyEquation.toStdString());

        double plotting_xTerm = startPoint;

        equation_xTerm = startPoint;

        if (shiftX) equation_xTerm = 0.0;

        if (getStatus() == SetStartpoint)
        {
            pPoints->point = RS_Vector(startPoint, m_muParserObject->Eval());
            pPoints->history.clear();
            pPoints->history.append(pPoints->point);
            pPoints->bHistory.clear();
            pPoints->bHistory.append(0.0);
            pPoints->start = pPoints->point;

            setStatus(SetNextPoint);

            plotting_xTerm += stepSize;
            equation_xTerm += stepSize;
        }

        while (((direction ==  1) && (plotting_xTerm <= endPoint)) 
        ||     ((direction == -1) && (plotting_xTerm >= endPoint)))
        {
            pPoints->point = RS_Vector(plotting_xTerm, m_muParserObject->Eval());
            pPoints->history.append(pPoints->point);

            if (pPoints->polyline == nullptr)
            {
                pPoints->polyline = new RS_Polyline(container, pPoints->data);
                pPoints->polyline->addVertex(pPoints->start, 0.0);
            }

            pPoints->polyline->addVertex(pPoints->point, 0.0);
            pPoints->polyline->setEndpoint(pPoints->point);

            if (pPoints->polyline->count() == 1)
            {
                pPoints->polyline->setLayerToActive();
                pPoints->polyline->setPenToActive();
                container->addEntity(pPoints->polyline);
            }

            deletePreview();
            graphicView->drawEntity(pPoints->polyline);

            plotting_xTerm += stepSize;
            equation_xTerm += stepSize;
        }

        drawSnapper();

        plotting_xTerm -= stepSize;
        equation_xTerm -= stepSize;

        graphicView->moveRelativeZero(RS_Vector(plotting_xTerm, m_muParserObject->Eval()));

        updateMouseButtonHints();

        e->accept();
        return;
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



void RS_ActionDrawPolyline::updateMouseButtonHints()
{
    if (equationSettingOn || startPointSettingOn || endPointSettingOn || stepSizeSettingOn) return;

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
			if (m_mode==TanRad)
				m_mode=Line;
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
