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
#include "rs_arc.h"
#include "rs_math.h"
#include "rs_settings.h"

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
    switch (status) {
        case SetStartPoint:
        case SetOpeningWidth:
        case SetWindowWidth:
        case SetDoorWidth:
            return;

        case SetOpeningAim: {
            double depth = 4.0;
            {
                LC_GROUP_GUARD("/Defaults");
                bool ok = false;
                double d = RS_SETTINGS->readStr("/OpeningDepth", "4").toDouble(&ok);
                if (ok && d > 0.0) depth = d;
            }
            RS_Vector wallDir = RS_Vector::polar(1.0, m_openingWallAngle);
            RS_Vector toMouse = snap - m_openingStart;
            double cross = wallDir.x * toMouse.y - wallDir.y * toMouse.x;
            double perpAngle = m_openingWallAngle + (cross >= 0.0 ? M_PI / 2.0 : -M_PI / 2.0);
            RS_Vector perpVec = RS_Vector::polar(depth, perpAngle);
            RS_Vector openEnd = m_openingStart + RS_Vector::polar(m_openingWidth, m_openingWallAngle);
            list << new RS_Line(m_openingStart, m_openingStart + perpVec);
            list << new RS_Line(m_openingStart, openEnd);
            list << new RS_Line(openEnd, openEnd + perpVec);
            return;
        }

        case SetWindowAim: {
            if (!LC_LineMath::isNonZeroLineLength(m_windowStart, snap)) return;
            double offset = 1.5;
            {
                LC_GROUP_GUARD("/Defaults");
                bool ok = false;
                double d = RS_SETTINGS->readStr("/WindowOffsetWidth", "1.5").toDouble(&ok);
                if (ok && d > 0.0) offset = d;
            }
            double windowAngle = (snap - m_windowStart).angle();
            RS_Vector windowEnd = m_windowStart + RS_Vector::polar(m_windowWidth, windowAngle);
            RS_Vector perpVec  = RS_Vector::polar(offset, windowAngle + M_PI / 2.0);
            list << new RS_Line(m_windowStart, windowEnd);
            list << new RS_Line(m_windowStart + perpVec, windowEnd + perpVec);
            list << new RS_Line(m_windowStart - perpVec, windowEnd - perpVec);
            list << new RS_Line(m_windowStart + perpVec, m_windowStart - perpVec);
            list << new RS_Line(windowEnd + perpVec, windowEnd - perpVec);
            return;
        }

        case SetDoorSwingSide: {
            RS_Vector doorEnd = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
            RS_Vector wallDir = RS_Vector::polar(1.0, m_doorWallAngle);
            RS_Vector toMouse = snap - m_doorStart;
            double cross     = wallDir.x * toMouse.y - wallDir.y * toMouse.x;
            double swingSign = (cross >= 0.0) ? 1.0 : -1.0;
            // Infer provisional hinge from mouse proximity
            bool hingeAtStart = snap.distanceTo(m_doorStart) <= snap.distanceTo(doorEnd);
            appendDoorPreview(list, swingSign, hingeAtStart);
            return;
        }

        case SetDoorHingeSide: {
            RS_Vector doorEnd  = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
            bool hingeAtStart  = snap.distanceTo(m_doorStart) <= snap.distanceTo(doorEnd);
            appendDoorPreview(list, m_doorSwingSign, hingeAtStart);
            return;
        }

        default:
            break;
    }

    if (!m_actionData->data.startpoint.valid) return;
    list << new RS_Line(m_actionData->data.startpoint, snap);
    if (m_showRefEntitiesOnPreview) {
        createRefPoint(m_actionData->data.startpoint, list);
        createRefSelectablePoint(snap, list);
    }
}

// ── trigger ───────────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::doPrepareTriggerEntities(QList<RS_Entity *> &list) {
    if (!m_pendingOpening.isEmpty() || !m_pendingArcs.isEmpty()) {
        for (const RS_LineData &d : std::as_const(m_pendingOpening))
            list << new RS_Line(m_container, d);
        m_pendingOpening.clear();
        for (const RS_ArcData &d : std::as_const(m_pendingArcs))
            list << new RS_Arc(m_container, d);
        m_pendingArcs.clear();
    } else {
        list << new RS_Line(m_container, m_actionData->data);
    }
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
        case SetOpeningAim:
            completeOpening(pos);
            break;
        case SetWindowAim:
            completeWindow(pos);
            break;
        case SetDoorSwingSide: {
            // Single click decides both swing side and hinge side.
            RS_Vector wallDir = RS_Vector::polar(1.0, m_doorWallAngle);
            RS_Vector toMouse = pos - m_doorStart;
            double cross = wallDir.x * toMouse.y - wallDir.y * toMouse.x;
            m_doorSwingSign = (cross >= 0.0) ? 1.0 : -1.0;
            RS_Vector doorEnd = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
            m_doorHingeAtStart = pos.distanceTo(m_doorStart) <= pos.distanceTo(doorEnd);
            completeDoor();
            break;
        }
        case SetDoorHingeSide: {
            // No longer used in normal flow; kept for safety.
            RS_Vector doorEnd = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
            m_doorHingeAtStart = pos.distanceTo(m_doorStart) <= pos.distanceTo(doorEnd);
            completeDoor();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawLineDirect::doProcessCommandValue(int status, const QString &c) {
    bool ok = false;
    double distance = RS_Math::eval(c, &ok);
    switch (status) {
        case SetPoint:
            if (ok && LC_LineMath::isMeaningful(distance)) {
                RS_Vector dir = m_lastSnapPoint - m_actionData->data.startpoint;
                if (dir.valid && dir.magnitude() > RS_TOLERANCE) {
                    m_actionData->data.endpoint = m_actionData->data.startpoint + dir.normalized() * distance;
                    completeLineSegment();
                    return true;
                }
            }
            return false;
        case SetOpeningWidth:
            if (ok && LC_LineMath::isMeaningful(distance)) {
                m_openingWidth = distance;
                setStatus(SetOpeningAim);
                updateMouseButtonHints();
                return true;
            }
            return false;
        case SetWindowWidth:
            if (ok && LC_LineMath::isMeaningful(distance)) {
                m_windowWidth = distance;
                completeWindowAlongWall();
                return true;
            }
            return false;
        case SetDoorWidth:
            if (ok && LC_LineMath::isMeaningful(distance)) {
                m_doorWidth = distance;
                setStatus(SetDoorSwingSide);
                updateMouseButtonHints();
                return true;
            }
            return false;
        default:
            return false;
    }
}

bool LC_ActionDrawLineDirect::doProceedCommand([[maybe_unused]] int status, const QString &c) {
    if (checkCommand("undo", c)) {
        undo();
        updateMouseButtonHints();
        return true;
    }
    // Action-local subcommands are not in the global command translation table;
    // compare directly.  Handle exact matches first.
    if (c == QLatin1String("o") || c == QLatin1String("opening")) {
        startOpeningMode();
        return true;
    }
    if (c == QLatin1String("w") || c == QLatin1String("window")) {
        startWindowMode();
        return true;
    }
    if (c == QLatin1String("d") || c == QLatin1String("door")) {
        startDoorMode();
        return true;
    }

    // Inline forms: "o30", "o 30", "opening30", "opening 30", etc.
    // Only match while in SetPoint state with a valid start; exact matches
    // are already handled above, so any startsWith match here has a non-empty suffix.
    if (getStatus() != SetPoint) return false;

    // Lambda: extract suffix after a given prefix (returns "" if not a prefix match)
    auto suffix = [&](const QString &prefix) -> QString {
        if (c.startsWith(prefix))
            return c.mid(prefix.length()).trimmed();
        return {};
    };

    // Shared distance parser used by all three inline paths
    auto parseWidth = [&](const QString &s, double &out) -> bool {
        bool ok = false;
        double v = RS_Math::eval(s, &ok);
        if (ok && LC_LineMath::isMeaningful(v)) { out = v; return true; }
        return false;
    };

    // Opening: "opening<w>" or "o<w>"
    {
        QString s = suffix(QLatin1String("opening"));
        if (s.isEmpty() && c.length() > 1 && c.front() == QLatin1Char('o'))
            s = c.mid(1).trimmed();
        if (!s.isEmpty()) {
            double w = 0.0;
            if (!parseWidth(s, w)) return false;
            if (m_actionData->prevPoints.empty()) {
                commandMessage(tr("Cannot open: draw at least one wall segment first."));
                return true;
            }
            m_openingStart     = m_actionData->data.startpoint;
            m_openingWallAngle = getLastWallAngle();
            m_openingWidth     = w;
            setStatus(SetOpeningAim);
            updateMouseButtonHints();
            return true;
        }
    }

    // Window: "window<w>" or "w<w>"
    {
        QString s = suffix(QLatin1String("window"));
        if (s.isEmpty() && c.length() > 1 && c.front() == QLatin1Char('w'))
            s = c.mid(1).trimmed();
        if (!s.isEmpty()) {
            double w = 0.0;
            if (!parseWidth(s, w)) return false;
            if (m_actionData->prevPoints.empty()) {
                commandMessage(tr("Cannot place window: draw at least one wall segment first."));
                return true;
            }
            m_windowStart = m_actionData->data.startpoint;
            m_windowWidth = w;
            completeWindowAlongWall();
            return true;
        }
    }

    // Door: "door<w>" or "d<w>"
    {
        QString s = suffix(QLatin1String("door"));
        if (s.isEmpty() && c.length() > 1 && c.front() == QLatin1Char('d'))
            s = c.mid(1).trimmed();
        if (!s.isEmpty()) {
            double w = 0.0;
            if (!parseWidth(s, w)) return false;
            if (m_actionData->prevPoints.empty()) {
                commandMessage(tr("Cannot place door: draw at least one wall segment first."));
                return true;
            }
            m_doorStart     = m_actionData->data.startpoint;
            m_doorWallAngle = getLastWallAngle();
            m_doorWidth     = w;
            setStatus(SetDoorSwingSide);
            updateMouseButtonHints();
            return true;
        }
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
    switch (status) {
        case SetStartPoint:
            finishAction();
            break;
        case SetOpeningAim:
            setStatus(SetOpeningWidth);
            updateMouseButtonHints();
            break;
        case SetOpeningWidth:
            setStatus(SetPoint);
            updateMouseButtonHints();
            break;
        case SetWindowAim:
            setStatus(SetWindowWidth);
            updateMouseButtonHints();
            break;
        case SetWindowWidth:
            setStatus(SetPoint);
            updateMouseButtonHints();
            break;
        case SetDoorSwingSide:
            setStatus(SetDoorWidth);
            updateMouseButtonHints();
            break;
        case SetDoorHingeSide:
            setStatus(SetDoorSwingSide);
            updateMouseButtonHints();
            break;
        case SetDoorWidth:
            setStatus(SetPoint);
            updateMouseButtonHints();
            break;
        default:
            setStatus(SetStartPoint);
            m_actionData->data.startpoint = RS_Vector(false);
            m_actionData->prevPoints.clear();
            break;
    }
}

// ── commands / hints ──────────────────────────────────────────────────────────

QStringList LC_ActionDrawLineDirect::getAvailableCommands() {
    QStringList cmd;
    if (mayUndo()) {
        cmd += command("undo");
    }
    if (!m_actionData->prevPoints.empty()) {
        // wall-dependent subcommands
        cmd += QStringLiteral("o");
        cmd += QStringLiteral("opening");
        cmd += QStringLiteral("w");
        cmd += QStringLiteral("window");
        cmd += QStringLiteral("d");
        cmd += QStringLiteral("door");
    }
    return cmd;
}

// ── sub-mode helpers ─────────────────────────────────────────────────────────

double LC_ActionDrawLineDirect::getLastWallAngle() const {
    if (m_actionData->prevPoints.empty()) return 0.0;
    // prevPoints.back() is the start of the last drawn segment;
    // data.startpoint is its endpoint (our current position).
    return (m_actionData->data.startpoint - m_actionData->prevPoints.back()).angle();
}

void LC_ActionDrawLineDirect::startOpeningMode() {
    if (getStatus() != SetPoint) return;
    if (m_actionData->prevPoints.empty()) {
        commandMessage(tr("Cannot open: draw at least one wall segment first."));
        return;
    }
    m_openingStart     = m_actionData->data.startpoint;
    m_openingWallAngle = getLastWallAngle();
    setStatus(SetOpeningWidth);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::completeOpening(const RS_Vector &snap) {
    double depth = 4.0;
    {
        LC_GROUP_GUARD("/Defaults");
        bool ok = false;
        double d = RS_SETTINGS->readStr("/OpeningDepth", "4").toDouble(&ok);
        if (ok && d > 0.0) depth = d;
    }
    RS_Vector wallDir = RS_Vector::polar(1.0, m_openingWallAngle);
    RS_Vector toMouse = snap - m_openingStart;
    double cross      = wallDir.x * toMouse.y - wallDir.y * toMouse.x;
    double perpAngle  = m_openingWallAngle + (cross >= 0.0 ? M_PI / 2.0 : -M_PI / 2.0);
    RS_Vector perpVec = RS_Vector::polar(depth, perpAngle);
    RS_Vector openEnd = m_openingStart + RS_Vector::polar(m_openingWidth, m_openingWallAngle);

    m_pendingOpening.clear();
    m_pendingOpening << RS_LineData(m_openingStart, m_openingStart + perpVec);
    m_pendingOpening << RS_LineData(m_openingStart, openEnd);
    m_pendingOpening << RS_LineData(openEnd, openEnd + perpVec);

    m_actionData->prevPoints.push_back(m_openingStart);
    m_actionData->data.startpoint = m_openingStart;
    m_actionData->data.endpoint   = openEnd;
    trigger();
    m_actionData->data.startpoint = openEnd;
    moveRelativeZero(openEnd);
    setStatus(SetPoint);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::startWindowMode() {
    if (getStatus() != SetPoint) return;
    if (m_actionData->prevPoints.empty()) {
        commandMessage(tr("Cannot place window: draw at least one wall segment first."));
        return;
    }
    m_windowStart = m_actionData->data.startpoint;
    setStatus(SetWindowWidth);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::completeWindow(const RS_Vector &snap) {
    if (!LC_LineMath::isNonZeroLineLength(m_windowStart, snap)) return;
    double offset = 1.5;
    {
        LC_GROUP_GUARD("/Defaults");
        bool ok = false;
        double d = RS_SETTINGS->readStr("/WindowOffsetWidth", "1.5").toDouble(&ok);
        if (ok && d > 0.0) offset = d;
    }
    double windowAngle = (snap - m_windowStart).angle();
    RS_Vector windowEnd = m_windowStart + RS_Vector::polar(m_windowWidth, windowAngle);
    RS_Vector perpVec   = RS_Vector::polar(offset, windowAngle + M_PI / 2.0);

    m_pendingOpening.clear();
    m_pendingOpening << RS_LineData(m_windowStart, windowEnd);
    m_pendingOpening << RS_LineData(m_windowStart + perpVec, windowEnd + perpVec);
    m_pendingOpening << RS_LineData(m_windowStart - perpVec, windowEnd - perpVec);
    m_pendingOpening << RS_LineData(m_windowStart + perpVec, m_windowStart - perpVec);
    m_pendingOpening << RS_LineData(windowEnd + perpVec,    windowEnd - perpVec);

    m_actionData->prevPoints.push_back(m_windowStart);
    m_actionData->data.startpoint = m_windowStart;
    m_actionData->data.endpoint   = windowEnd;
    trigger();
    m_actionData->data.startpoint = windowEnd;
    moveRelativeZero(windowEnd);
    setStatus(SetPoint);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::completeWindowAlongWall() {
    double angle = getLastWallAngle();
    double offset = 1.5;
    {
        LC_GROUP_GUARD("/Defaults");
        bool ok = false;
        double d = RS_SETTINGS->readStr("/WindowOffsetWidth", "1.5").toDouble(&ok);
        if (ok && d > 0.0) offset = d;
    }
    RS_Vector windowEnd = m_windowStart + RS_Vector::polar(m_windowWidth, angle);
    RS_Vector perpVec   = RS_Vector::polar(offset, angle + M_PI / 2.0);

    m_pendingOpening.clear();
    m_pendingOpening << RS_LineData(m_windowStart, windowEnd);
    m_pendingOpening << RS_LineData(m_windowStart + perpVec, windowEnd + perpVec);
    m_pendingOpening << RS_LineData(m_windowStart - perpVec, windowEnd - perpVec);
    m_pendingOpening << RS_LineData(m_windowStart + perpVec, m_windowStart - perpVec);
    m_pendingOpening << RS_LineData(windowEnd + perpVec,    windowEnd - perpVec);

    m_actionData->prevPoints.push_back(m_windowStart);
    m_actionData->data.startpoint = m_windowStart;
    m_actionData->data.endpoint   = windowEnd;
    trigger();
    m_actionData->data.startpoint = windowEnd;
    moveRelativeZero(windowEnd);
    setStatus(SetPoint);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::startDoorMode() {
    if (getStatus() != SetPoint) return;
    if (m_actionData->prevPoints.empty()) {
        commandMessage(tr("Cannot place door: draw at least one wall segment first."));
        return;
    }
    m_doorStart     = m_actionData->data.startpoint;
    m_doorWallAngle = getLastWallAngle();
    setStatus(SetDoorWidth);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::completeDoor() {
    RS_Vector doorEnd    = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
    RS_Vector hingePoint = m_doorHingeAtStart ? m_doorStart : doorEnd;
    double leafAngle     = m_doorWallAngle + m_doorSwingSign * M_PI / 2.0;
    RS_Vector leafEnd    = hingePoint + RS_Vector::polar(m_doorWidth, leafAngle);
    double wallEndAngle  = m_doorHingeAtStart ? m_doorWallAngle : m_doorWallAngle + M_PI;
    bool reversed        = (m_doorHingeAtStart == (m_doorSwingSign > 0.0));

    m_pendingOpening.clear();
    m_pendingArcs.clear();
    m_pendingOpening << RS_LineData(m_doorStart, doorEnd);
    m_pendingOpening << RS_LineData(hingePoint, leafEnd);
    m_pendingArcs    << RS_ArcData(hingePoint, m_doorWidth, leafAngle, wallEndAngle, reversed);

    m_actionData->prevPoints.push_back(m_doorStart);
    m_actionData->data.startpoint = m_doorStart;
    m_actionData->data.endpoint   = doorEnd;
    trigger();
    m_actionData->data.startpoint = doorEnd;
    moveRelativeZero(doorEnd);
    setStatus(SetPoint);
    updateMouseButtonHints();
}

void LC_ActionDrawLineDirect::appendDoorPreview(QList<RS_Entity *> &list,
                                                 double swingSign,
                                                 bool hingeAtStart) const {
    RS_Vector doorEnd    = m_doorStart + RS_Vector::polar(m_doorWidth, m_doorWallAngle);
    double leafAngle     = m_doorWallAngle + swingSign * M_PI / 2.0;
    RS_Vector hingePoint = hingeAtStart ? m_doorStart : doorEnd;
    RS_Vector leafEnd    = hingePoint + RS_Vector::polar(m_doorWidth, leafAngle);
    double wallEndAngle  = hingeAtStart ? m_doorWallAngle : m_doorWallAngle + M_PI;
    bool reversed        = (hingeAtStart == (swingSign > 0.0));
    list << new RS_Line(m_doorStart, doorEnd);
    list << new RS_Line(hingePoint, leafEnd);
    list << new RS_Arc(RS_ArcData(hingePoint, m_doorWidth, leafAngle, wallEndAngle, reversed));
}

// ─────────────────────────────────────────────────────────────────────────────

void LC_ActionDrawLineDirect::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint: {
            bool hasWall = !m_actionData->prevPoints.empty();
            QString cmds;
            if (mayUndo()) cmds += command("undo") + "/";
            if (hasWall)   cmds += "o/w/d";
            updateMouseWidgetTRBack(
                tr("Specify next point or type distance [%1]").arg(cmds),
                MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetOpeningWidth:
            updateMouseWidgetTRBack(tr("Type opening width"));
            break;
        case SetOpeningAim:
            updateMouseWidgetTRBack(tr("Click to choose marker side"));
            break;
        case SetWindowWidth:
            updateMouseWidgetTRBack(tr("Type window width"));
            break;
        case SetWindowAim:
            updateMouseWidgetTRBack(tr("Click to choose window direction"));
            break;
        case SetDoorWidth:
            updateMouseWidgetTRBack(tr("Type door width"));
            break;
        case SetDoorSwingSide:
            updateMouseWidgetTRBack(tr("Move mouse to preview orientation, click to confirm"));
            break;
        case SetDoorHingeSide:
            updateMouseWidgetTRBack(tr("Click near hinge end"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
