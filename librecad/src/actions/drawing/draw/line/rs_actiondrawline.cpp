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


#include "rs_actiondrawline.h"

#include "qg_lineoptions.h"
#include "rs_debug.h"
#include "rs_line.h"

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

struct RS_ActionDrawLine::ActionData
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

size_t RS_ActionDrawLine::ActionData::index(const int offset /*= 0*/)
{
    return static_cast<size_t>( std::max( 0, historyIndex + offset));
}

RS_ActionDrawLine::RS_ActionDrawLine(LC_ActionContext *actionContext) :
    RS_PreviewActionInterface( "Draw lines", actionContext, RS2::ActionDrawLine),
    m_actionData( new ActionData{}){
    RS_DEBUG->print("RS_ActionDrawLine::RS_ActionDrawLine");
}

RS_ActionDrawLine::~RS_ActionDrawLine() = default;

void RS_ActionDrawLine::reset(){
    RS_DEBUG->print("RS_ActionDrawLine::reset");
    m_actionData.reset(new ActionData{});
}

void RS_ActionDrawLine::init(int status){
    RS_DEBUG->print("RS_ActionDrawLine::init");
    RS_PreviewActionInterface::init(status);

    reset();
    drawSnapper();
}

void RS_ActionDrawLine::doTrigger() {
    auto* line = new RS_Line(m_container, m_actionData->data);
    setPenAndLayerToActive(line);
    moveRelativeZero(m_actionData->history.at(m_actionData->index()).currPt);
    undoCycleAdd(line);
    RS_DEBUG->print("RS_ActionDrawLine::trigger(): line added: %lu",line->getId());
}

void RS_ActionDrawLine::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetStartpoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetEndpoint: {
            RS_Vector &startPoint = m_actionData->data.startpoint;
            if (startPoint.valid){
                mouse = getSnapAngleAwarePoint(e, startPoint, mouse, true);
                previewToCreateLine(startPoint, mouse);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(startPoint);
                    previewRefSelectablePoint(mouse);
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLine::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    // Snapping to angle(15*) if shift key is pressed
    if (status == SetEndpoint ) {
        snapped = getSnapAngleAwarePoint(e,  m_actionData->data.startpoint, snapped);
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionDrawLine::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    switch (status) {
        default:
        case SetStartpoint:
            initPrevious(status);
            break;
        case SetEndpoint:
            next();
            break;
    }
}

void RS_ActionDrawLine::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent");

    if (m_actionData->data.startpoint.valid == false && status == SetEndpoint) {
        setStatus(SetStartpoint);
        m_actionData->startOffset = 0;
    }
    switch (status) {
        case SetStartpoint: {
            m_actionData->data.startpoint = mouse;
            m_actionData->startOffset = 0;
            addHistory( HA_SetStartpoint, getRelativeZero(), mouse, m_actionData->startOffset);
            setStatus(SetEndpoint);
            moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        }
        case SetEndpoint: {
            if ((mouse-m_actionData->data.startpoint).squared() > RS_TOLERANCE2) {
                // refuse zero length lines
                m_actionData->data.endpoint = mouse;
                ++m_actionData->startOffset;
                addHistory( HA_SetEndpoint, m_actionData->data.startpoint, mouse, m_actionData->startOffset);
                trigger();
                m_actionData->data.startpoint = m_actionData->data.endpoint;
                if (m_actionData->history.size() >= 2) {
                    updateMouseButtonHints();
                }
            }
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: OK");
}

bool RS_ActionDrawLine::doProcessCommand(int status, const QString &c) {
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent");

    bool accept = false;

    if (checkCommand( "redo", c)) {
        redo();
        accept = true;
        updateMouseButtonHints();
    }
    else {
        switch (status) {
            case SetStartpoint:
                break;
            case SetEndpoint: {
                if (checkCommand("close", c)) {
                    close();
                    updateMouseButtonHints();
                    accept = true;
                } else if (checkCommand("undo", c)) {
                    undo();
                    updateMouseButtonHints();
                    accept = true;
                }
                break;
            }
            default:
                break;
        }
    }
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent: OK");
    return accept;
}

QStringList RS_ActionDrawLine::getAvailableCommands(){
    QStringList cmd;
    if (m_actionData->index() + 1 < m_actionData->history.size()) {
        cmd += command("redo");
    }

    switch (getStatus()) {
        case SetStartpoint:
            break;
        case SetEndpoint: {
            if (m_actionData->historyIndex >= 1) {
                cmd += command("undo");
            }
            if (m_actionData->startOffset >= 2) {
                cmd += command("close");
            }
            break;
        }
        default:
            break;
    }

    return cmd;
}

void RS_ActionDrawLine::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartpoint:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetEndpoint: {
            QString msg = "";

            if (m_actionData->startOffset >= 2) {
                msg += command("close");
            }
            if (m_actionData->index() + 1 < m_actionData->history.size()) {
                if (msg.size() > 0) {
                    msg += "/";
                }
                msg += command("redo");
            }
            if (m_actionData->historyIndex >= 1) {
                if (msg.size() > 0) {
                    msg += "/";
                }
                msg += command("undo");
            }

            if (m_actionData->historyIndex >= 1) {
                updateMouseWidgetTRBack(tr("Specify next point or [%1]").arg(msg), MOD_SHIFT_ANGLE_SNAP);
            } else {
                updateMouseWidgetTRBack(tr("Specify next point"), MOD_SHIFT_ANGLE_SNAP);
            }
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLine::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void RS_ActionDrawLine::close(){
    if (SetEndpoint != getStatus()) {
        return;
    }
    if (1 < m_actionData->startOffset  && 0 <= m_actionData->historyIndex - m_actionData->startOffset) {
        History h(m_actionData->history.at( m_actionData->index( -m_actionData->startOffset)));
        if ((m_actionData->data.startpoint - h.currPt).squared() > RS_TOLERANCE2) {
            m_actionData->data.endpoint = h.currPt;
            addHistory( HA_Close, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
            trigger();
            setStatus(SetStartpoint);
        }
    }
    else {
        commandMessage(tr("Cannot close sequence of lines: "
                          "Not enough entities defined yet, or already closed."));
    }
}

void RS_ActionDrawLine::next(){
    addHistory( HA_Next, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
    setStatus(SetStartpoint);
}

void RS_ActionDrawLine::addHistory(RS_ActionDrawLine::HistoryAction a, const RS_Vector& p, const RS_Vector& c, const int s){
    if (m_actionData->historyIndex < -1) {
        m_actionData->historyIndex = -1;
    }

    auto offset = m_actionData->historyIndex + 1;
    if (offset == 0) {
        // this is for MSVC implementation of vector, it does not like 0 offset
        m_actionData->history.erase(m_actionData->history.begin(), m_actionData->history.end());
    }
    else {
        m_actionData->history.erase(m_actionData->history.begin() + offset, m_actionData->history.end());
    }

    m_actionData->history.push_back( History( a, p, c, s));
    m_actionData->historyIndex = static_cast<int>(m_actionData->history.size() - 1);
}

void RS_ActionDrawLine::undo(){
    if (0 <= m_actionData->historyIndex) {
        History h( m_actionData->history.at( m_actionData->index()));

        --m_actionData->historyIndex;
        deletePreview();
        moveRelativeZero(h.prevPt);

        switch (h.histAct) {
            case HA_SetStartpoint: {
                setStatus(SetStartpoint);
                break;
            }
            case HA_SetEndpoint:
            case HA_Close: {
                switchToAction(RS2::ActionEditUndo);
                m_actionData->data.startpoint = h.prevPt;
                setStatus(SetEndpoint);
                break;
            }
            case HA_Next: {
                m_actionData->data.startpoint = h.prevPt;
                setStatus(SetEndpoint);
                break;
            }
        }

        // get index for close from new current history
        h = m_actionData->history.at( m_actionData->index());
        m_actionData->startOffset = h.startOffset;
    }
    else {
        commandMessage(tr("Cannot undo: Begin of history reached"));
    }
}

void RS_ActionDrawLine::redo(){
    if (m_actionData->history.size() > (m_actionData->index() + 1)) {
        ++m_actionData->historyIndex;
        History h( m_actionData->history.at( m_actionData->index()));
        deletePreview();
        moveRelativeZero(h.currPt);
        m_actionData->data.startpoint = h.currPt;
        m_actionData->startOffset = h.startOffset;
        switch (h.histAct) {
            case HA_SetStartpoint: {
                setStatus(SetEndpoint);
                break;
            }
            case HA_SetEndpoint: {
                switchToAction(RS2::ActionEditRedo);
                setStatus(SetEndpoint);
                break;
            }
            case HA_Close: {
                switchToAction(RS2::ActionEditRedo);
                setStatus(SetStartpoint);
                break;
            }
            case HA_Next: {
                setStatus(SetStartpoint);
                break;
            }
        }
    }
    else {
        commandMessage(tr("Cannot redo: End of history reached"));
    }
}

LC_ActionOptionsWidget* RS_ActionDrawLine::createOptionsWidget(){
    return new QG_LineOptions();
}
