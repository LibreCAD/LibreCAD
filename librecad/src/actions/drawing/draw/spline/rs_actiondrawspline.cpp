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

#include "rs_actiondrawspline.h"

#include "qg_splineoptions.h"
#include "rs_debug.h"
#include "rs_spline.h"

struct RS_ActionDrawSpline::ActionData {

/**
 * Spline data defined so far.
 */
    RS_SplineData data;
/**
 * Polyline entity we're working on.
 */
    RS_Spline* spline{nullptr};
/**
 * Point history (for undo)
 */
    QList<RS_Vector> history;

/**
 * Bulge history (for undo)
 */
//QList<double> bHistory;
};

RS_ActionDrawSpline::RS_ActionDrawSpline(LC_ActionContext *actionContext, RS2::ActionType actionType)
    :RS_PreviewActionInterface("Draw splines",actionContext, actionType)
    , m_actionData(std::make_unique<ActionData>())
{
    reset();
}

RS_ActionDrawSpline::~RS_ActionDrawSpline() = default;

void RS_ActionDrawSpline::reset(){
    m_actionData->spline = nullptr;
    m_actionData->history.clear();
}

void RS_ActionDrawSpline::init(int status){
    RS_PreviewActionInterface::init(status);
    reset();
}

void RS_ActionDrawSpline::doTrigger() {
    if (!m_actionData->spline){
        return;
    }

    // add the entity
    //RS_Spline* spline = new RS_Spline(container, data);
    setPenAndLayerToActive(m_actionData->spline);
    m_actionData->spline->update();
    undoCycleAdd(m_actionData->spline);

    RS_DEBUG->print("RS_ActionDrawSpline::trigger(): spline added: %lu",
                    m_actionData->spline->getId());

    m_actionData->spline = nullptr;
    //history.clear();
}

void RS_ActionDrawSpline::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartPoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            if (m_actionData->spline /*&& point.valid*/){
                auto *tmpSpline = dynamic_cast<RS_Spline *>(m_actionData->spline->clone());
                tmpSpline->addControlPoint(mouse);
                tmpSpline->update();
                previewEntity(tmpSpline);

                if (m_showRefEntitiesOnPreview) {
                    auto cpts = tmpSpline->getControlPoints();
                    for (size_t i = 0; i < cpts.size() - 1; i++) {
                        const RS_Vector &vp = cpts[i];
                        previewRefPoint(vp);
                    }
                    previewRefSelectablePoint(mouse);
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawSpline::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawSpline::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    if (status == SetNextPoint && m_actionData->spline){
        const size_t nPoints = m_actionData->spline->getNumberOfControlPoints();
        bool isClosed = m_actionData->spline->isClosed();
        // Issue #1689: allow closed splines by 3 control points
        if (nPoints > size_t(m_actionData->spline->getDegree()) || (isClosed && nPoints == 3))
            trigger();
    }
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawSpline::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetStartPoint: {
            m_actionData->history.clear();
            m_actionData->history.append(mouse);
            if (!m_actionData->spline){
                m_actionData->spline = new RS_Spline(m_container, m_actionData->data);
                m_actionData->spline->addControlPoint(mouse);
            }
            setStatus(SetNextPoint);
            moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        }
        case SetNextPoint: {
            moveRelativeZero(mouse);
            m_actionData->history.append(mouse);
            if (m_actionData->spline){
                m_actionData->spline->addControlPoint(mouse);
                deletePreview();
                drawSnapper();
            }
            updateMouseButtonHints();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawSpline::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetStartPoint: {
            break;
        }
        case SetNextPoint: {
            /*if (checkCommand("close", c)) {
                close();
                updateMouseButtonHints();
                return;
            }*/
            if (checkCommand("undo", c)){
                undo();
                updateMouseButtonHints();
                accept = true;
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawSpline::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetStartPoint:
            break;
        case SetNextPoint: {
            if (m_actionData->history.size() >= 2){
                cmd += command("undo");
            } else if (m_actionData->history.size() >= 3){
                cmd += command("close");
            }
            break;
        }
        default:
            break;
    }
    return cmd;
}

void RS_ActionDrawSpline::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartPoint: {
            updateMouseWidgetTRCancel(tr("Specify first control point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetNextPoint: {
            QString msg = "";

            if (m_actionData->history.size() >= 3){
                msg += command("close");
                msg += "/";
            }
            if (m_actionData->history.size() >= 2){
                msg += command("undo");
                updateMouseWidgetTRBack(tr("Specify next control point or [%1]").arg(msg));
            } else {
                updateMouseWidgetTRBack(tr("Specify next control point"));
            }
        }
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawSpline::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

/*
void RS_ActionDrawSpline::close() {
    if (history.count()>2 && start.valid) {
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

void RS_ActionDrawSpline::undo(){
    if (m_actionData->history.size() > 1){
        m_actionData->history.removeLast();
        //bHistory.removeLast();
        deletePreview();
        //graphicView->setCurrentAction(
        //    new RS_ActionEditUndo(true, *container, *graphicView));
        if (!m_actionData->history.isEmpty()){
            //point = *history.last();
        }
        if (m_actionData->spline){
            m_actionData->spline->removeLastControlPoint();
            if (!m_actionData->history.isEmpty()){
                RS_Vector v = m_actionData->history.last();
                moveRelativeZero(v);
            }
            redrawDrawing();

        }
    } else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}

void RS_ActionDrawSpline::setDegree(int deg){
    m_actionData->data.degree = deg;
    if (m_actionData->spline){
        m_actionData->spline->setDegree(deg);
    }
}

int RS_ActionDrawSpline::getDegree(){
    return m_actionData->data.degree;
}

void RS_ActionDrawSpline::setClosed(bool c){
    m_actionData->data.closed = c;
    if (m_actionData->spline){
        m_actionData->spline->setClosed(c);
    }
}

bool RS_ActionDrawSpline::isClosed(){
    return m_actionData->data.closed;
}

LC_ActionOptionsWidget* RS_ActionDrawSpline::createOptionsWidget(){
    return new QG_SplineOptions();
}
