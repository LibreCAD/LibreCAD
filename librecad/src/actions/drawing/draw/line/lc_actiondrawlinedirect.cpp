/**
 * Draw Fast: draw sequential segments by typed distance and mouse aim direction.
 *
 * Usage:
 *   1. Click to set the start point.
 *   2. Type a distance and press Enter; aim the mouse to choose direction, or
 *      simply click the desired endpoint.
 *   3. Each confirmed point advances the start automatically.
 *   4. Right-click or Escape to finish.  Type "undo" to remove the last segment.
 *
 * Commands: df, dfast
 */
#include "lc_actiondrawlinedirect.h"

#include <QMouseEvent>

#include "lc_linemath.h"
#include "rs_math.h"

LC_ActionDrawLineDirect::LC_ActionDrawLineDirect(LC_ActionContext *actionContext)
    : LC_AbstractActionDrawLine("Draw Fast", actionContext, RS2::ActionDrawLineDirect)
    , m_actionData(new ActionData{}) {
    m_primaryDirection = DIRECTION_POINT;
    m_direction = DIRECTION_POINT;
}

LC_ActionDrawLineDirect::~LC_ActionDrawLineDirect() = default;

// ── start point ──────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::doSetStartPoint(RS_Vector start) {
    m_actionData->data.startpoint = start;
    m_actionData->prevPoints.clear();
    setStatus(SetPoint);
    moveRelativeZero(start);
    updateMouseButtonHints();
}

bool LC_ActionDrawLineDirect::isStartPointValid() const {
    return m_actionData->data.startpoint.valid;
}

bool LC_ActionDrawLineDirect::mayStart() {
    return true;
}

const RS_Vector &LC_ActionDrawLineDirect::getStartPointForAngleSnap() const {
    return m_actionData->data.startpoint;
}

// ── preview ───────────────────────────────────────────────────────────────────

bool LC_ActionDrawLineDirect::doCheckMayDrawPreview([[maybe_unused]] LC_MouseEvent *pEvent,
                                                     [[maybe_unused]] int status) {
    return true;
}

void LC_ActionDrawLineDirect::doPreparePreviewEntities([[maybe_unused]] LC_MouseEvent *e,
                                                        RS_Vector &snap,
                                                        QList<RS_Entity *> &list,
                                                        int status) {
    if (status == SetStartPoint) return;
    if (!m_actionData->data.startpoint.valid) return;

    list << new RS_Line(m_actionData->data.startpoint, snap);
    if (m_showRefEntitiesOnPreview) {
        createRefPoint(m_actionData->data.startpoint, list);
        createRefSelectablePoint(snap, list);
    }
}

// ── trigger ───────────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::doPrepareTriggerEntities(QList<RS_Entity *> &list) {
    list << new RS_Line(m_container, m_actionData->data);
}

RS_Vector LC_ActionDrawLineDirect::doGetRelativeZeroAfterTrigger() {
    return m_actionData->data.endpoint;
}

// ── segment completion ────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::completeLineSegment() {
    m_actionData->prevPoints.push_back(m_actionData->data.startpoint);
    trigger();
    m_actionData->data.startpoint = m_actionData->data.endpoint;
    moveRelativeZero(m_actionData->data.startpoint);
    setStatus(SetPoint);
    updateMouseButtonHints();
}

// ── coordinate / command input ────────────────────────────────────────────────

void LC_ActionDrawLineDirect::onCoordinateEvent(int status, [[maybe_unused]] bool isZero,
                                                 const RS_Vector &pos) {
    switch (status) {
        case SetStartPoint:
            doSetStartPoint(pos);
            break;
        case SetPoint:
            if (LC_LineMath::isNonZeroLineLength(m_actionData->data.startpoint, pos)) {
                m_actionData->data.endpoint = pos;
                completeLineSegment();
            }
            break;
        default:
            break;
    }
}

bool LC_ActionDrawLineDirect::doProcessCommandValue(int status, const QString &c) {
    if (status == SetPoint) {
        bool ok = false;
        double distance = RS_Math::eval(c, &ok);
        if (ok && LC_LineMath::isMeaningful(distance)) {
            RS_Vector dir = m_lastSnapPoint - m_actionData->data.startpoint;
            if (dir.valid && dir.magnitude() > RS_TOLERANCE) {
                m_actionData->data.endpoint = m_actionData->data.startpoint + dir.normalized() * distance;
                completeLineSegment();
                return true;
            }
        }
        return false;
    }
    return false;
}

bool LC_ActionDrawLineDirect::doProceedCommand([[maybe_unused]] int status, const QString &c) {
    if (checkCommand("undo", c)) {
        undo();
        updateMouseButtonHints();
        return true;
    }
    return false;
}

// ── undo ──────────────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::undo() {
    if (mayUndo()) {
        RS_Vector prev = m_actionData->prevPoints.back();
        m_actionData->prevPoints.pop_back();
        m_actionData->data.startpoint = prev;
        deletePreview();
        moveRelativeZero(prev);
        switchToAction(RS2::ActionEditUndo);
        setStatus(SetPoint);
    } else {
        commandMessage(tr("Cannot undo: Begin of history reached"));
    }
}

bool LC_ActionDrawLineDirect::mayUndo() const {
    return !m_actionData->prevPoints.empty();
}

// ── back / cancel ──────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::doBack(LC_MouseEvent *e, int status) {
    e->originalEvent->accept();
    if (status == SetStartPoint) {
        finishAction();
    } else {
        setStatus(SetStartPoint);
        m_actionData->data.startpoint = RS_Vector(false);
        m_actionData->prevPoints.clear();
    }
}

// ── commands / hints ──────────────────────────────────────────────────────────

QStringList LC_ActionDrawLineDirect::getAvailableCommands() {
    QStringList cmd;
    if (mayUndo()) {
        cmd += command("undo");
    }
    return cmd;
}

void LC_ActionDrawLineDirect::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint:
            if (mayUndo()) {
                updateMouseWidgetTRBack(
                    tr("Specify next point or type distance [%1]").arg(command("undo")),
                    MOD_SHIFT_ANGLE_SNAP);
            } else {
                updateMouseWidgetTRBack(tr("Specify next point or type distance"),
                                        MOD_SHIFT_ANGLE_SNAP);
            }
            break;
        default:
            updateMouseWidget();
            break;
    }
}
