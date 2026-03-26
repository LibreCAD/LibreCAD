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

#include "lc_action_draw_spline.h"

#include "lc_spline_options_filler.h"
#include "lc_spline_options_widget.h"
#include "rs_document.h"
#include "rs_spline.h"

struct LC_ActionDrawSpline::ActionData {
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

LC_ActionDrawSpline::LC_ActionDrawSpline(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawSpline", actionContext,  RS2::ActionDrawSpline), m_actionData(std::make_unique<ActionData>()) {
    reset();
}

LC_ActionDrawSpline::LC_ActionDrawSpline(const QString& actionName, LC_ActionContext* actionContext, const RS2::ActionType actionType)
    : LC_SingleEntityCreationAction(actionName, actionContext, actionType), m_actionData(std::make_unique<ActionData>()) {
    reset();
}

LC_ActionDrawSpline::~LC_ActionDrawSpline() = default;

void LC_ActionDrawSpline::doSaveOptions() {
    const bool drawSplineAction = rtti() == RS2::ActionDrawSpline;
    if (drawSplineAction){
        save("Degree", getDegree());
    }
    save("Closed", isClosed());
}

void LC_ActionDrawSpline::doLoadOptions() {
    const bool drawSplineAction = rtti() == RS2::ActionDrawSpline;
    if (drawSplineAction){
        const int degree = loadInt("Degree", 3);
        setDegree(degree);
    }
    bool closed = loadBool("Closed", false);
    setClosed(closed);
}

bool LC_ActionDrawSpline::isInVisualSnapStatus(int status) {
    return (status == SetStartPoint) || (status == SetNextPoint);
}

void LC_ActionDrawSpline::reset() const {
    m_actionData->spline = nullptr;
    m_actionData->history.clear();
}

void LC_ActionDrawSpline::init(const int status) {
    RS_PreviewActionInterface::init(status);
    reset();
}

RS_Entity* LC_ActionDrawSpline::doTriggerCreateEntity() {
    if (!m_actionData->spline) {
        return nullptr;
    }

    m_actionData->spline->update();
    return m_actionData->spline;
}

void LC_ActionDrawSpline::doTriggerCompletion([[maybe_unused]] bool success) {
    m_actionData->spline = nullptr;
}

void LC_ActionDrawSpline::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetStartPoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetNextPoint: {
            if (m_actionData->spline /*&& point.valid*/) {
                auto* tmpSplineClone = dynamic_cast<RS_Spline*>(m_actionData->spline->clone());
                tmpSplineClone->addControlPoint(mouse);
                tmpSplineClone->update();
                previewEntity(tmpSplineClone);

                if (m_showRefEntitiesOnPreview) {
                    const auto cpts = tmpSplineClone->getControlPoints();
                    for (size_t i = 0; i < cpts.size() - 1; i++) {
                        const RS_Vector& vp = cpts[i];
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

void LC_ActionDrawSpline::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawSpline::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    const auto spline = m_actionData->spline;
    if (status == SetNextPoint && spline != nullptr) {
        const size_t nPoints = spline->getNumberOfControlPoints();
        const bool isClosed = spline->isClosed();
        // Issue #1689: allow closed splines by 3 control points
        if (nPoints > static_cast<size_t>(spline->getDegree()) || (isClosed && nPoints == 3)) {
            trigger();
        }
    }
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawSpline::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetStartPoint: {
            m_actionData->history.clear();
            m_actionData->history.append(coord);
            if (!m_actionData->spline) {
                m_actionData->spline = new RS_Spline(m_document, m_actionData->data);
                m_actionData->spline->addControlPoint(coord);
            }
            setStatus(SetNextPoint);
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            updateActionPrompt();
            break;
        }
        case SetNextPoint: {
            moveRelativeZero(coord);
            addSnappedPointToVisualSnap(coord);
            m_actionData->history.append(coord);
            if (m_actionData->spline) {
                m_actionData->spline->addControlPoint(coord);
                drawPreview();
                drawSnapper();
            }
            updateActionPrompt();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawSpline::doProcessCommand(const int status, const QString& command) {
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
            if (checkCommand("undo", command)) {
                undo();
                updateActionPrompt();
                accept = true;
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionDrawSpline::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetStartPoint:
            break;
        case SetNextPoint: {
            if (m_actionData->history.size() >= 2) {
                cmd += command("undo");
            }
            else if (m_actionData->history.size() >= 3) {
                cmd += command("close");
            }
            break;
        }
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawSpline::updateActionPrompt() {
    switch (getStatus()) {
        case SetStartPoint: {
            updatePromptTRCancel(tr("Specify first control point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetNextPoint: {
            QString msg = "";

            if (m_actionData->history.size() >= 3) {
                msg += command("close");
                msg += "/";
            }
            if (m_actionData->history.size() >= 2) {
                msg += command("undo");
                updatePromptTRBack(tr("Specify next control point or [%1]").arg(msg));
            }
            else {
                updatePromptTRBack(tr("Specify next control point"));
            }
        }
        break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawSpline::doGetMouseCursor([[maybe_unused]] int status) {
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

void LC_ActionDrawSpline::undo() {
    if (m_actionData->history.size() > 1) {
        m_actionData->history.removeLast();
        //bHistory.removeLast();
        deletePreview();
        //graphicView->setCurrentAction(
        //    new RS_ActionEditUndo(true, *container, *graphicView));
        if (!m_actionData->history.isEmpty()) {
            //point = *history.last();
        }
        if (m_actionData->spline) {
            m_actionData->spline->removeLastControlPoint();
            if (!m_actionData->history.isEmpty()) {
                const RS_Vector v = m_actionData->history.last();
                moveRelativeZero(v);
            }
            redrawDrawing();
        }
    }
    else {
        commandMessage(tr("Cannot undo: Not enough entities defined yet."));
    }
}

void LC_ActionDrawSpline::setDegree(const int deg) {
    m_actionData->data.degree = deg;
    if (m_actionData->spline) {
        m_actionData->spline->setDegree(deg);
    }
}

int LC_ActionDrawSpline::getDegree() const {
    return m_actionData->data.degree;
}

void LC_ActionDrawSpline::setClosed(const bool c) {
    m_actionData->data.setClosed(c);
    if (m_actionData->spline) {
        m_actionData->spline->setClosed(c);
    }
}

bool LC_ActionDrawSpline::isClosed() {
    return m_actionData->data.isClosed();
}

LC_ActionOptionsWidget* LC_ActionDrawSpline::createOptionsWidget() {
    return new LC_SplineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawSpline::createOptionsFiller() {
    return new LC_SplineOptionsFiller();
}
