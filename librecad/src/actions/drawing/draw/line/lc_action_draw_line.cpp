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

#include "lc_action_draw_line.h"

#include "lc_line_options_filler.h"
#include "lc_line_options_widget.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_line.h"

struct LC_ActionDrawLine::History{
    History() = default;

    explicit History(const HistoryAction action,
                     const RS_Vector& previous,
                     const RS_Vector& current,
                     const int start) :
        historicAction( action),
        previousPoint( previous),
        currentPoint( current),
        startOffset( start) {}

    HistoryAction    historicAction = HA_SetStartpoint;    ///< action to undo/redo
    RS_Vector       previousPoint;     ///< previous coordinate
    RS_Vector       currentPoint;     ///< current coordinate
    int             startOffset = 0;///< offset to start point for close method
};

struct LC_ActionDrawLine::ActionData{
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
    size_t index(int offset = 0) const;
};

size_t LC_ActionDrawLine::ActionData::index(const int offset /*= 0*/) const {
    return static_cast<size_t>( std::max<int>( 0, historyIndex + offset));
}

LC_ActionDrawLine::LC_ActionDrawLine(LC_ActionContext *actionContext) :
    LC_SingleEntityCreationAction("ActionDrawLine", actionContext, RS2::ActionDrawLine),
    m_actionData(std::make_unique<ActionData>()){
}

LC_ActionDrawLine::~LC_ActionDrawLine() = default;

void LC_ActionDrawLine::doSaveOptions() {
    // nothing to save
}

void LC_ActionDrawLine::doLoadOptions() {
    // nothing to load
}

bool LC_ActionDrawLine::isInVisualSnapStatus(int status) {
    return status == SetStartpoint || status == SetEndpoint;
}

void LC_ActionDrawLine::reset(){
    RS_DEBUG->print("RS_ActionDrawLine::reset");
    m_actionData = std::make_unique<ActionData>();
}

void LC_ActionDrawLine::init(const int status){
    RS_DEBUG->print("RS_ActionDrawLine::init");
    RS_PreviewActionInterface::init(status);

    reset();
    drawSnapper();
}

RS_Entity* LC_ActionDrawLine::doTriggerCreateEntity() {
    auto* line = new RS_Line(m_document, m_actionData->data);
    moveRelativeZero(m_actionData->history.at(m_actionData->index()).currentPoint);
    return line;
}

void LC_ActionDrawLine::doTriggerCompletion([[maybe_unused]]bool success) {
    // setStatus(SetStartpoint);
}

void LC_ActionDrawLine::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetStartpoint: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetEndpoint: {
            const RS_Vector &startPoint = m_actionData->data.startpoint;
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

void LC_ActionDrawLine::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    // Snapping to angle(15*) if shift key is pressed
    if (status == SetEndpoint ) {
        snapped = getSnapAngleAwarePoint(e,  m_actionData->data.startpoint, snapped);
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionDrawLine::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
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

void LC_ActionDrawLine::setStartPoint(const RS_Vector& v) {
    m_actionData->data.startpoint = v;
    addSnappedPointToVisualSnap(v);
}

void LC_ActionDrawLine::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent");

    if (!m_actionData->data.startpoint.valid && status == SetEndpoint) {
        setStatus(SetStartpoint);
        m_actionData->startOffset = 0;
    }
    switch (status) {
        case SetStartpoint: {
            m_actionData->startOffset = 0;
            addHistory( HA_SetStartpoint, getRelativeZero(), coord, m_actionData->startOffset);
            setStatus(SetEndpoint);
            setStartPoint(coord);
            moveRelativeZero(coord);
            updateActionPrompt();
            updateOptions();
            break;
        }
        case SetEndpoint: {
            if ((coord-m_actionData->data.startpoint).squared() > RS_TOLERANCE2) {
                // refuse zero length lines
                m_actionData->data.endpoint = coord;
                ++m_actionData->startOffset;
                addHistory( HA_SetEndpoint, m_actionData->data.startpoint, coord, m_actionData->startOffset);
                trigger();
                setStartPoint(m_actionData->data.endpoint);
                if (m_actionData->history.size() >= 2) {
                    updateActionPrompt();
                    updateOptions();
                }
            }
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLine::coordinateEvent: OK");
}

bool LC_ActionDrawLine::doProcessCommand(const int status, const QString &command) {
    RS_DEBUG->print("RS_ActionDrawLine::commandEvent");

    bool accept = false;

    if (checkCommand( "redo", command)) {
        redo();
        accept = true;
        updateActionPrompt();
    }
    else {
        switch (status) {
            case SetStartpoint:
                break;
            case SetEndpoint: {
                if (checkCommand("close", command)) {
                    close();
                    updateActionPrompt();
                    accept = true;
                } else if (checkCommand("undo", command)) {
                    undo();
                    updateActionPrompt();
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

QStringList LC_ActionDrawLine::getAvailableCommands(){
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

void LC_ActionDrawLine::updateActionPrompt(){
    switch (getStatus()) {
        case SetStartpoint:
            updatePromptTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
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
                updatePromptTRBack(tr("Specify next point or [%1]").arg(msg), MOD_SHIFT_ANGLE_SNAP);
            } else {
                updatePromptTRBack(tr("Specify next point"), MOD_SHIFT_ANGLE_SNAP);
            }
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawLine::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

bool LC_ActionDrawLine::mayClose() const {
    return  SetEndpoint == getStatus()  &&  (1 < m_actionData->startOffset  && 0 <= m_actionData->historyIndex - m_actionData->startOffset);
}

void LC_ActionDrawLine::close(){
    if (SetEndpoint != getStatus()) {
        return;
    }
    if (1 < m_actionData->startOffset  && 0 <= m_actionData->historyIndex - m_actionData->startOffset) {
        const History h(m_actionData->history.at( m_actionData->index( -m_actionData->startOffset)));
        if ((m_actionData->data.startpoint - h.currentPoint).squared() > RS_TOLERANCE2) {
            m_actionData->data.endpoint = h.currentPoint;
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

void LC_ActionDrawLine::next(){
    addHistory( HA_Next, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
    setStatus(SetStartpoint);
}

void LC_ActionDrawLine::addHistory(HistoryAction action, const RS_Vector& previous, const RS_Vector& current, const int start) const {
    m_actionData->historyIndex = std::max<int>(m_actionData->historyIndex, -1);

    const size_t offset = m_actionData->historyIndex + 1;
    assert(offset <= m_actionData->history.size());
    m_actionData->history.resize(offset);

    m_actionData->history.emplace_back(action, previous, current, start);
    m_actionData->historyIndex = static_cast<int>(m_actionData->history.size() - 1);
}

bool LC_ActionDrawLine::mayUndo() const {
    return m_actionData != nullptr && 0 <= m_actionData->historyIndex;
}

bool LC_ActionDrawLine::mayRedo() const {
    return m_actionData->history.size() > m_actionData->index() + 1;
}

void LC_ActionDrawLine::undo(){
    if (m_actionData != nullptr && 0 <= m_actionData->historyIndex) {
        History h( m_actionData->history.at( m_actionData->index()));

        --m_actionData->historyIndex;
        deletePreview();
        moveRelativeZero(h.previousPoint);

        switch (h.historicAction) {
            case HA_SetStartpoint: {
                setStatus(SetStartpoint);
                break;
            }
            case HA_SetEndpoint:
            case HA_Close: {
                m_document->undo();
                m_graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetEndpoint);
                setStartPoint(h.previousPoint);
                break;
            }
            case HA_Next: {
                setStatus(SetEndpoint);
                setStartPoint(h.previousPoint);
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

void LC_ActionDrawLine::redo(){
    if (m_actionData->history.size() > (m_actionData->index() + 1)) {
        ++m_actionData->historyIndex;
        const History h( m_actionData->history.at( m_actionData->index()));
        deletePreview();
        const auto startPoint = h.currentPoint;
        moveRelativeZero(startPoint);
        m_actionData->startOffset = h.startOffset;
        switch (h.historicAction) {
            case HA_SetStartpoint: {
                setStatus(SetEndpoint);
                break;
            }
            case HA_SetEndpoint: {
                // Issue #2608: re-add the entity directly via the document undo
                // stack. Do NOT use switchToAction(RS2::ActionEditRedo): the redo
                // action is a one-shot that finishes inside init(), which makes
                // LC_EventHandler drop back to the default action and destroy
                // *this* while redo() is still on the stack (use-after-free).
                // Mirrors how undo() above performs m_document->undo() directly.
                m_document->redo();
                m_graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetEndpoint);
                break;
            }
            case HA_Close: {
                m_document->redo();
                m_graphicView->redraw(RS2::RedrawDrawing);
                setStatus(SetStartpoint);
                break;
            }
            case HA_Next: {
                setStatus(SetStartpoint);
                break;
            }
        }
        setStartPoint(startPoint);
    }
    else {
        commandMessage(tr("Cannot redo: End of history reached"));
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLine::createOptionsWidget(){
    return new LC_LineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLine::createOptionsFiller() {
    return new LC_LineOptionsFiller();
}
