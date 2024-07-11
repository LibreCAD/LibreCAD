/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pavel Krejcir (pavel@pamsoft.cz)
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

#include <QAction>
#include <QMouseEvent>
#include "lc_actiondrawsplinepoints.h"

#include "lc_splinepoints.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_point.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_actioninterface.h"

struct LC_ActionDrawSplinePoints::Points {
	/**
	* Spline data defined so far.
	*/
	LC_SplinePointsData data;

	/**
	* Spline used.
	*/
	std::unique_ptr<LC_SplinePoints> spline;

	/**
	* Point history (for undo)
	*/
	std::vector<RS_Vector> undoBuffer;
};

LC_ActionDrawSplinePoints::LC_ActionDrawSplinePoints(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView):
    RS_ActionDrawSpline(container, graphicView), pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionDrawSplinePoints;
    setName("DrawSplinePoints");
}

LC_ActionDrawSplinePoints::~LC_ActionDrawSplinePoints() = default;

void LC_ActionDrawSplinePoints::reset(){
    pPoints->spline.reset();
    pPoints->undoBuffer.clear();
}

void LC_ActionDrawSplinePoints::init(int status){
    RS_PreviewActionInterface::init(status);
    reset(); // fixme - review reset, try to make it common with base action
}

void LC_ActionDrawSplinePoints::trigger(){
    if (!pPoints->spline.get()) return;

    pPoints->spline->setLayerToActive();
    pPoints->spline->setPenToActive();
    pPoints->spline->update();
    RS_Entity *s = pPoints->spline->clone();
    container->addEntity(s);

    addToDocumentUndoable(s);

// upd view
    RS_Vector r = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    moveRelativeZero(r);
    RS_DEBUG->print("RS_ActionDrawSplinePoints::trigger(): spline added: %lu", s->getId());
    reset();
}

void LC_ActionDrawSplinePoints::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawSplinePoints::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status) {
        case SetStartPoint:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetNextPoint: {
            auto *sp = dynamic_cast<LC_SplinePoints *>(pPoints->spline->clone());
            deletePreview();

            for (auto const &v: sp->getPoints()) {
                previewRefPoint(v);
            }

            sp->addPoint(mouse);
            previewRefSelectablePoint(mouse);
            previewEntity(sp);
            drawPreview();
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawSplinePoints::mouseMoveEvent end");
}

void LC_ActionDrawSplinePoints::mouseLeftButtonReleaseEvent([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawSplinePoints::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    if (status == SetNextPoint && pPoints->spline.get()){
        trigger();
    }
    init(status - 1);
}


void LC_ActionDrawSplinePoints::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
        case SetStartPoint:
            pPoints->undoBuffer.clear();
            if (!pPoints->spline.get()){
                pPoints->spline.reset(new LC_SplinePoints(container, pPoints->data));
                pPoints->spline->addPoint(mouse);
                previewRefPoint(mouse);
            }
            setStatus(SetNextPoint);
            moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        case SetNextPoint:
            moveRelativeZero(mouse);
            if (pPoints->spline.get()){
                pPoints->spline->addPoint(mouse);
                drawPreview();
                drawSnapper();
            }
            updateMouseButtonHints();
            break;
        default:
            break;
    }
}

void LC_ActionDrawSplinePoints::commandEvent(RS_CommandEvent *e){
    QString c = e->getCommand().toLower();

    switch (getStatus()) {
        case SetStartPoint:
            if (checkCommand("help", c)){
                commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
                return;
            }
            break;
        case SetNextPoint:
            if (checkCommand("undo", c)){
                undo();
                updateMouseButtonHints();
                return;
            }
            if (checkCommand("redo", c)){
                redo();
                updateMouseButtonHints();
                return;
            }
            break;
        default:
            break;
    }
}

QStringList LC_ActionDrawSplinePoints::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetStartPoint:
            break;
        case SetNextPoint:
            if (pPoints->data.splinePoints.size() > 0){
                cmd += command("undo");
            }
            if (pPoints->undoBuffer.size() > 0){
                cmd += command("redo");
            }
            if (pPoints->data.splinePoints.size() > 2){
                cmd += command("close");
            }
            break;
        default:
            break;
    }

    return cmd;
}

void LC_ActionDrawSplinePoints::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel("Specify first control point", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetNextPoint: {
            QString msg = "";

            if (pPoints->data.splinePoints.size() > 2){
                msg += command("close");
                msg += "/";
            }
            if (pPoints->data.splinePoints.size() > 0){
                msg += command("undo");
            }
            if (pPoints->undoBuffer.size() > 0){
                msg += command("redo");
            }

            if (pPoints->data.splinePoints.size() > 0){
                updateMouseWidget(tr("Specify next control point or [%1]").arg(msg),tr("Back"));
            } else {
                updateMouseWidgetTRBack("Specify next control point");
            }
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

/*
void RS_ActionDrawSplinePoints::close() {
	if (history.size()>2 && start.valid) {
        //data.endpoint = start;
        //trigger();
                if (spline) {
                        RS_CoordinateEvent e(spline->getStartpoint());
                        coordinateEvent(&e);
                }
                trigger();
        setStatus(SetStartpoint);
        graphicView->moveRelativeZero(start);
    } else {
        RS_DIALOGFACTORY->commandMessage(
            tr("Cannot close sequence of lines: "
               "Not enough entities defined yet."));
    }
}
*/

void LC_ActionDrawSplinePoints::undo(){
    if (!pPoints->spline.get()){
        commandMessageTR("Cannot undo: Not enough entities defined yet.");
        return;
    }

    auto &splinePts = pPoints->spline->getData().splinePoints;
    size_t nPoints = splinePts.size();
    if (nPoints > 1){
        RS_Vector v = splinePts.back();
        pPoints->undoBuffer.push_back(v);
        pPoints->spline->removeLastPoint();

        if (!splinePts.size()) setStatus(SetStartPoint);
        else {
            v = splinePts.back();
            graphicView->moveRelativeZero(v);
        }
        graphicView->redraw(RS2::RedrawDrawing);
        drawPreview();
    } else {
        commandMessageTR("Cannot undo: Not enough entities defined yet.");
    }
}

void LC_ActionDrawSplinePoints::redo(){
    int iBufLen = pPoints->undoBuffer.size();
    if (iBufLen > 1){
        RS_Vector v = pPoints->undoBuffer.back();
        pPoints->spline->addPoint(v);
        pPoints->undoBuffer.pop_back();

        setStatus(SetNextPoint);
        v = pPoints->data.splinePoints.back();
        moveRelativeZero(v);
        graphicView->redraw(RS2::RedrawDrawing);
    } else {
        commandMessageTR("Cannot undo: Nothing could be redone.");
    }
}

void LC_ActionDrawSplinePoints::setClosed(bool c){
    pPoints->data.closed = c;
    if (pPoints->spline.get()){
        pPoints->spline->setClosed(c);
    }
}

bool LC_ActionDrawSplinePoints::isClosed(){
    return pPoints->data.closed;
}

// EOF

