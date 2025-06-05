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

#include "lc_actiondrawsplinepoints.h"

#include "lc_splinepoints.h"
#include "rs_debug.h"
#include "rs_document.h"

struct LC_ActionDrawSplinePoints::ActionData {
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

LC_ActionDrawSplinePoints::LC_ActionDrawSplinePoints(LC_ActionContext *actionContext)
    :RS_ActionDrawSpline(actionContext, RS2::ActionDrawSplinePoints)
    , m_actionData(std::make_unique<ActionData>())
{
    setName("DrawSplinePoints");
}

LC_ActionDrawSplinePoints::~LC_ActionDrawSplinePoints() = default;

void LC_ActionDrawSplinePoints::reset(){
    m_actionData->spline.reset();
    m_actionData->undoBuffer.clear();
    setStatus(SetStartPoint);
}

void LC_ActionDrawSplinePoints::init(int status){
    RS_ActionDrawSpline::init(status);
    reset(); // fixme - review reset, try to make it common with base action
}

void LC_ActionDrawSplinePoints::doTrigger() {
    if (m_actionData->spline.get() != nullptr) {
        setPenAndLayerToActive(m_actionData->spline.get());
        auto spline = std::make_unique<LC_SplinePoints>(m_container, m_actionData->spline->getData());
        setPenAndLayerToActive(spline.get());
        undoCycleAdd(spline.get());

        LC_LOG<<"RS_ActionDrawSplinePoints::trigger(): spline added: "<<spline->getId();
        spline.release();
        reset();
    }
}

void LC_ActionDrawSplinePoints::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartPoint:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetNextPoint: {
            auto *sp = dynamic_cast<LC_SplinePoints *>(m_actionData->spline->clone());

            if (m_showRefEntitiesOnPreview) {
                for (auto const &v: sp->getPoints()) {
                    previewRefPoint(v);
                }
                previewRefSelectablePoint(mouse);
            }

            sp->addPoint(mouse);
            sp->update();
            previewEntity(sp);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawSplinePoints::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawSplinePoints::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if (status == SetNextPoint && m_actionData->spline.get()){
        trigger();
    }
    initPrevious(status);
}

void LC_ActionDrawSplinePoints::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetStartPoint: {
            m_actionData->undoBuffer.clear();
            if (m_actionData->spline.get() == nullptr){
                m_actionData->spline = std::make_unique<LC_SplinePoints>(m_container, m_actionData->data);
                m_actionData->spline->addPoint(mouse);

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(mouse);
                }
            }
            setStatus(SetNextPoint);
            moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        }
        case SetNextPoint: {
            moveRelativeZero(mouse);
            if (m_actionData->spline.get() != nullptr){
                m_actionData->spline->addPoint(mouse);
                m_actionData->spline->update();
                drawPreview();
                drawSnapper();
            }
            updateMouseButtonHints();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawSplinePoints::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetStartPoint:
            break;
        case SetNextPoint: {
            if (checkCommand("undo", c)){
                undo();
                updateMouseButtonHints();
                accept = true;
            }
            else if (checkCommand("redo", c)){
                redo();
                updateMouseButtonHints();
                accept = true;
            }
            else if (checkCommand("close", c)) {
                setClosed(!isClosed());
                accept = true;
            }
            else if (checkCommand("kill", c)) {
                trigger();
                accept = true;
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionDrawSplinePoints::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetStartPoint:
            break;
        case SetNextPoint:
            if (!m_actionData->data.splinePoints.empty()){
                cmd += "undo";
            }
            if (!m_actionData->undoBuffer.empty()){
                cmd += "redo";
            }
            if (m_actionData->spline->getData().splinePoints.size() > 2){
                cmd += "close";
                cmd += "kill";
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
            updateMouseWidgetTRCancel(tr("Specify first control point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetNextPoint: {
            QString msg = "";

            if (m_actionData->data.splinePoints.size() > 2){
                msg += command("close");
                msg += "/";
            }
            if (!m_actionData->data.splinePoints.empty()){
                msg += command("undo");
            }
            if (!m_actionData->undoBuffer.empty()){
                msg += command("redo");
            }

            if (!m_actionData->data.splinePoints.empty()){
                updateMouseWidget(tr("Specify next control point or [%1]").arg(msg),tr("Back"));
            } else {
                updateMouseWidgetTRBack(tr("Specify next control point"));
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
    if (!m_actionData->spline.get()){
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
        return;
    }

    auto &splinePts = m_actionData->spline->getData().splinePoints;
    size_t nPoints = splinePts.size();
    if (nPoints > 1){
        RS_Vector v = splinePts.back();
        m_actionData->undoBuffer.push_back(v);
        m_actionData->spline->removeLastPoint();

        if (splinePts.empty()) {
            setStatus(SetStartPoint);
        }
        else {
            v = splinePts.back();
            moveRelativeZero(v);
        }
        redrawDrawing();
        drawPreview();
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}

void LC_ActionDrawSplinePoints::redo(){
    int iBufLen = m_actionData->undoBuffer.size();
    if (iBufLen > 1){
        RS_Vector v = m_actionData->undoBuffer.back();
        m_actionData->spline->addPoint(v);
        m_actionData->undoBuffer.pop_back();

        setStatus(SetNextPoint);
        v = m_actionData->data.splinePoints.back();
        moveRelativeZero(v);
        redrawDrawing();
    } else {
        commandMessage(tr("Cannot undo: Nothing could be redone."));
    }
}

void LC_ActionDrawSplinePoints::setClosed(bool c){
    m_actionData->data.closed = c;
    if (m_actionData->spline.get() != nullptr){
        m_actionData->spline->setClosed(c);
    }
}

bool LC_ActionDrawSplinePoints::isClosed(){
    return m_actionData->data.closed;
}
