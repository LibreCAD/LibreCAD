/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 A. Stebich (librecad@mail.lordofbikes.de)
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
#include <vector>

#include <QAction>
#include <QMouseEvent>

#include "rs_actiondrawline.h"
#include "rs_actioneditundo.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLine::History
{
    explicit History(RS_ActionDrawLine::HistoryAction a,
                     const RS_Vector& p,
                     const RS_Vector& c,
                     const int s) :
        histAct( a),
        prevPt( p),
        currPt( c),
        startOffset( s) {}

    explicit History(const History& h) :
        histAct( h.histAct),
        prevPt( h.prevPt),
        currPt( h.currPt),
        startOffset( h.startOffset) {}

    History& operator=(const History& rho) {
        histAct     = rho.histAct;
        prevPt      = rho.prevPt;
        currPt      = rho.currPt;
        startOffset = rho.startOffset;
        return *this;
    }

    RS_ActionDrawLine::HistoryAction    histAct;    ///< action to undo/redo
    RS_Vector       prevPt;     ///< previous coordinate
    RS_Vector       currPt;     ///< current coordinate
    int             startOffset;///< offset to start point for close method
};

struct RS_ActionDrawLine::Points
{
    /// Line data defined so far
    RS_LineData data;
    /// Point history (undo/redo pointer)
    int  historyIndex {-1};
    /// start point offset for close method
    int  startOffset {0};

    /// Point history (undo/redo buffer)
    std::vector<History> history;

    /// wrapper for historyIndex to avoid 'signedness' warnings where std::vector-methods expect size_t
    /// also, offset helps in close method to find starting point
    size_t index(const int offset = 0);
};

size_t RS_ActionDrawLine::Points::index(const int offset /*= 0*/)
{
    return static_cast<size_t>( std::max( 0, historyIndex + offset));
}

RS_ActionDrawLine::RS_ActionDrawLine(RS_EntityContainer& container,
                                     RS_GraphicView& graphicView) :
    RS_PreviewActionInterface( "Draw lines", container, graphicView),
    pPoints( new Points{})
{
    RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine");
    actionType=RS2::ActionDrawLine;
}

RS_ActionDrawLine::~RS_ActionDrawLine() = default;

void RS_ActionDrawLine::reset()
{
    RS_DEBUG->print("RS_ActionDrawLine::reset");
    pPoints.reset(new Points{});
}

void RS_ActionDrawLine::init(int status)
{
    RS_DEBUG->print("RS_ActionDrawLine::init");
    RS_PreviewActionInterface::init(status);

    reset();
    drawSnapper();
}

void RS_ActionDrawLine::trigger()
{
    RS_PreviewActionInterface::trigger();

    RS_Line* line = new RS_Line(container, pPoints->data);
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // update undo list
    if (document) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }

    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(pPoints->history.at(pPoints->index()).currPt);
    RS_DEBUG->print("RS_ActionDrawLine::trigger(): line added: %lu",
                    line->getId());
}

void RS_ActionDrawLine::mouseMoveEvent(QMouseEvent* e)
{
    RS_Vector mouse = snapPoint(e);
    if (getStatus() == SetEndpoint && pPoints->data.startpoint.valid) {
        // Snapping to angle(15*) if shift key is pressed
        if (e->modifiers() & Qt::ShiftModifier) {
            mouse = snapToAngle(mouse, pPoints->data.startpoint, 15.);
        }

        deletePreview();
        RS_Line *line = new RS_Line(pPoints->data.startpoint, mouse);
        preview->addEntity(line);
        line->setLayerToActive();
        line->setPenToActive();
        drawPreview();
    }
}

void RS_ActionDrawLine::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        RS_Vector snapped = snapPoint(e);

        // Snapping to angle(15*) if shift key is pressed
        if ((e->modifiers() & Qt::ShiftModifier)
            && getStatus() == SetEndpoint ) {
            snapped = snapToAngle(snapped, pPoints->data.startpoint, 15.);
        }

        RS_CoordinateEvent ce(snapped);
        coordinateEvent(&ce);
    }
    else if (e->button() == Qt::RightButton) {
        deletePreview();
        switch (getStatus()) {
        default:
        case SetStartpoint:
            init( getStatus() - 1);
            break;

        case SetEndpoint:
            next();
            break;
        }
    }
}

void RS_ActionDrawLine::coordinateEvent(RS_CoordinateEvent* e)
{
    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent");

    if (nullptr == e) {
        RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: event was nullptr");
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    if (pPoints->data.startpoint.valid == false
        && getStatus() == SetEndpoint) {
        setStatus(SetStartpoint);
        pPoints->startOffset = 0;
    }

    switch (getStatus()) {
    case SetStartpoint:
        pPoints->data.startpoint = mouse;
        pPoints->startOffset = 0;
        addHistory( HA_SetStartpoint, graphicView->getRelativeZero(), mouse, pPoints->startOffset);
        setStatus(SetEndpoint);
        graphicView->moveRelativeZero(mouse);
        updateMouseButtonHints();
        break;

    case SetEndpoint:
        if ((mouse-pPoints->data.startpoint).squared() > RS_TOLERANCE2) {
            // refuse zero length lines
            pPoints->data.endpoint = mouse;
            ++pPoints->startOffset;
            addHistory( HA_SetEndpoint, pPoints->data.startpoint, mouse, pPoints->startOffset);
            trigger();
            pPoints->data.startpoint = pPoints->data.endpoint;
            if (pPoints->history.size() >= 2) {
                updateMouseButtonHints();
            }
        }
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: OK");
}

void RS_ActionDrawLine::commandEvent(RS_CommandEvent* e)
{
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent");

    QString c = e->getCommand().toLower();

    switch (getStatus()) {
    case SetStartpoint:
        if (checkCommand( "help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
            e->accept();
            return;
        }
        break;

    case SetEndpoint:
        if (checkCommand( "close", c)) {
            close();
            e->accept();
            updateMouseButtonHints();
            return;
        }

        if (checkCommand( "undo", c)) {
            undo();
            e->accept();
            updateMouseButtonHints();
            return;
        }
        break;

    default:
        return;
    }

    if (checkCommand( "redo", c)) {
        redo();
        e->accept();
        updateMouseButtonHints();
    }
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent: OK");
}

QStringList RS_ActionDrawLine::getAvailableCommands()
{
    QStringList cmd;
    if (pPoints->index() + 1 < pPoints->history.size()) {
        cmd += command("redo");
    }

    switch (getStatus()) {
    case SetStartpoint:
        break;
    case SetEndpoint:
        if (pPoints->historyIndex >= 1) {
            cmd += command("undo");
        }
        if (pPoints->startOffset >= 2) {
            cmd += command("close");
        }
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawLine::updateMouseButtonHints()
{
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetEndpoint: {
        QString msg = "";

        if (pPoints->startOffset >= 2) {
            msg += RS_COMMANDS->command("close");
        }
        if (pPoints->index() + 1 < pPoints->history.size()) {
            if (msg.size() > 0) {
                msg += "/";
            }
            msg += RS_COMMANDS->command("redo");
        }
        if (pPoints->historyIndex >= 1) {
            if (msg.size() > 0) {
                msg += "/";
            }
            msg += RS_COMMANDS->command("undo");
        }

        if (pPoints->historyIndex >= 1) {
            RS_DIALOGFACTORY->updateMouseWidget(
                        tr("Specify next point or [%1]").arg(msg),
                        tr("Back"));
        } else {
            RS_DIALOGFACTORY->updateMouseWidget(
                        tr("Specify next point"),
                        tr("Back"));
        }
        break;
    }
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionDrawLine::showOptions()
{
    RS_DEBUG->print("RS_ActionDrawLine::showOptions");
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionDrawLine::hideOptions()
{
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}


void RS_ActionDrawLine::updateMouseCursor()
{
    graphicView->setMouseCursor(RS2::CadCursor);
}

void RS_ActionDrawLine::close()
{
    if (SetEndpoint != getStatus()) {
        return;
    }
    if (1 < pPoints->startOffset
        && 0 <= pPoints->historyIndex - pPoints->startOffset) {
        History h(pPoints->history.at( pPoints->index( -pPoints->startOffset)));
        if ((pPoints->data.startpoint - h.currPt).squared() > RS_TOLERANCE2) {
            pPoints->data.endpoint = h.currPt;
            addHistory( HA_Close, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
            trigger();
            setStatus(SetStartpoint);
        }
    }
    else {
        RS_DIALOGFACTORY->commandMessage(tr("Cannot close sequence of lines: "
                                            "Not enough entities defined yet, or already closed."));
    }
}

void RS_ActionDrawLine::next()
{
    addHistory( HA_Next, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    setStatus(SetStartpoint);
}

void RS_ActionDrawLine::addHistory(RS_ActionDrawLine::HistoryAction a, const RS_Vector& p, const RS_Vector& c, const int s)
{
    if (pPoints->historyIndex < -1) {
        pPoints->historyIndex = -1;
    }

    pPoints->history.erase(pPoints->history.begin() + pPoints->historyIndex + 1, pPoints->history.end());
    pPoints->history.push_back( History( a, p, c, s));
    pPoints->historyIndex = static_cast<int>(pPoints->history.size() - 1);
}

void RS_ActionDrawLine::undo()
{
    if (0 <= pPoints->historyIndex) {
        History h( pPoints->history.at( pPoints->index()));

        --pPoints->historyIndex;
        deletePreview();
        graphicView->moveRelativeZero(h.prevPt);

        switch (h.histAct) {
        case HA_SetStartpoint:
            setStatus(SetStartpoint);
            break;

        case HA_SetEndpoint:
        case HA_Close:
            graphicView->setCurrentAction( new RS_ActionEditUndo(true, *container, *graphicView));
            pPoints->data.startpoint = h.prevPt;
            setStatus(SetEndpoint);
            break;

        case HA_Next:
            pPoints->data.startpoint = h.prevPt;
            setStatus(SetEndpoint);
            break;
        }

        // get index for close from new current history
        h = pPoints->history.at( pPoints->index());
        pPoints->startOffset = h.startOffset;
    }
    else {
        RS_DIALOGFACTORY->commandMessage(tr("Cannot undo: "
                                            "Begin of history reached"));
    }
}

void RS_ActionDrawLine::redo()
{
    if (pPoints->history.size() > (pPoints->index() + 1)) {
        ++pPoints->historyIndex;
        History h( pPoints->history.at( pPoints->index()));
        deletePreview();
        graphicView->moveRelativeZero(h.currPt);
        pPoints->data.startpoint = h.currPt;
        pPoints->startOffset = h.startOffset;
        switch (h.histAct) {
        case HA_SetStartpoint:
            setStatus(SetEndpoint);
            break;

        case HA_SetEndpoint:
            graphicView->setCurrentAction( new RS_ActionEditUndo(false, *container, *graphicView));
            setStatus(SetEndpoint);
            break;

        case HA_Close:
            graphicView->setCurrentAction( new RS_ActionEditUndo(false, *container, *graphicView));
            setStatus(SetStartpoint);
            break;

        case HA_Next:
            setStatus(SetStartpoint);
            break;
        }
    }
    else {
        RS_DIALOGFACTORY->commandMessage( tr("Cannot redo: "
                                             "End of history reached"));
    }
}
