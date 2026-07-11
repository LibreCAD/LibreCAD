/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_action_draw_points_line.h"

#include "lc_linemath.h"
#include "lc_points_line_options_filler.h"
#include "lc_points_line_options_widget.h"
#include "rs_document.h"
#include "rs_point.h"

LC_ActionDrawPointsLine::LC_ActionDrawPointsLine(LC_ActionContext* actionContext, const bool drawMiddle)
    : LC_AbstractActionDrawLine(drawMiddle ? "ActionDrawPointsMiddle" : "ActionDrawPointsLine", actionContext,
                                drawMiddle ? RS2::ActionDrawPointsMiddle : RS2::ActionDrawPointsLine) {
    if (drawMiddle) {
        m_edgePointsMode = DRAW_EDGE_NONE;
    }
}

LC_ActionDrawPointsLine::~LC_ActionDrawPointsLine() = default;

void LC_ActionDrawPointsLine::doSaveOptions() {
    save("Count", m_pointsCount);
    if (rtti() == RS2::ActionDrawPointsLine) {
        save("EdgeMode", m_edgePointsMode);
        save("UseFixedDistance", m_fixedDistanceMode);
        save("FitToLine", m_withinLineMode);
        save("PointsDistance", m_fixedDistance);
        save("Angle", m_angleDegrees);
    }
}

void LC_ActionDrawPointsLine::doLoadOptions() {
    m_pointsCount = loadInt("Count", 1);
    if (rtti() == RS2::ActionDrawPointsLine) {
        m_edgePointsMode = loadInt("EdgeMode", 1);
        m_fixedDistanceMode = loadBool("UseFixedDistance", false);
        m_withinLineMode = loadBool("FitToLine", true);
        m_fixedDistance = loadDouble("PointsDistance", 1.0);
        m_angleDegrees = loadDouble("Angle", 0.0);
    }
}

bool LC_ActionDrawPointsLine::isInVisualSnapStatus(int status) {
    return (status == SetStartPoint) || (status == SetPoint) || (status == SetDistance) || (status == SetAngle);
}

/**
 * just cleanup if needed
 * @param status new status
 */
void LC_ActionDrawPointsLine::init(const int status) {
    LC_AbstractActionWithPreview::init(status);
    if (status == 0) {
        m_point1Set = false;
        m_startpoint = RS_Vector(false);
        m_endpoint = RS_Vector(false);
    }
}

void LC_ActionDrawPointsLine::doSetStartPoint(const RS_Vector& vector) {
    // pre-snap to relative zero
    m_startpoint = vector;
    m_point1Set = true;
    if (m_direction == DIRECTION_POINT || m_direction == DIRECTION_NONE) {
        setStatus(SetPoint);
    }
    else {
        setStatus(SetDistance);
    }
}

bool LC_ActionDrawPointsLine::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    // prepare points data
    createPoints(m_endpoint, ctx.entitiesToAdd);
    return true;
}

bool LC_ActionDrawPointsLine::doCheckMayTrigger() {
    const bool result = m_point1Set && m_startpoint.valid && m_endpoint.valid;
    return result;
}

RS_Vector LC_ActionDrawPointsLine::doGetRelativeZeroAfterTrigger() {
    return m_endpoint;
}

void LC_ActionDrawPointsLine::doAfterTrigger() {
    LC_AbstractActionWithPreview::doAfterTrigger();
    if (m_direction == DIRECTION_X || m_direction == DIRECTION_Y) {
        m_direction = DIRECTION_POINT;
    }
    init(SetStartPoint);
}

void LC_ActionDrawPointsLine::doPreparePreviewEntities([[maybe_unused]] const LC_MouseEvent* e, RS_Vector& snap, QList<RS_Entity*>& list,
                                                       const int status) {
    // determine candidate for possible end point
    RS_Vector possibleEndPoint;
    switch (status) {
        case SetStartPoint:
        case SetEdge:
        case SetFixDistance:
        case SetPointsCount:
            return;
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
        case SetAngle:
            // calculate point with given angle from start point to snap
            possibleEndPoint = getPossibleEndPointForAngle(snap);
            break;
        case SetDistance:
            switch (m_direction) {
                case DIRECTION_X: // use only x coordinate from snap
                    possibleEndPoint = restrictHorizontal(m_startpoint, snap);
                    break;
                case DIRECTION_Y: // use only y coordinate from snap
                    possibleEndPoint = restrictVertical(m_startpoint, snap);
                    break;
                case DIRECTION_POINT:
                    possibleEndPoint = snap;
                    break;
                case DIRECTION_ANGLE:
                    // calculate point with given angle from start point to snap
                    possibleEndPoint = getPossibleEndPointForAngle(snap);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    // draw preview if this is non-zero line
    if (isNonZeroLine(possibleEndPoint)) {
        createPoints(possibleEndPoint, list);
        if (m_actionType == RS2::ActionDrawPointsMiddle && list.size() == 1) {
            RS_Entity* ep = list.at(0);
            auto* point = dynamic_cast<RS_Point*>(ep);
            if (point != nullptr) {
                previewEntityToCreate(point, false);
            }
        }
        if (m_showRefEntitiesOnPreview) {
            createRefSelectablePoint(possibleEndPoint, list);
            createRefPoint(m_startpoint, list);

            createRefLine(m_startpoint, possibleEndPoint, list);
        }
    }
}

RS_Vector LC_ActionDrawPointsLine::getPossibleEndPointForAngle(const RS_Vector& snap) const {
    // if shift is pressed, we'll use alternative mirrored angle for direction
    double angleToUse = m_angleDegrees;
    if (m_alternativeActionMode) {
        angleToUse = 180 - m_angleDegrees;
    }
    const double wcsAngle = toWorldAngleFromUCSBasisDegrees(angleToUse);
    return LC_LineMath::calculateEndpointForAngleDirection(wcsAngle, m_startpoint, snap);
}

/**
 * Calculate positions of points specified on line between start point and given point
 * @param potentialEndPoint coordinates of end point *
 * @param entitiesList
 */
void LC_ActionDrawPointsLine::createPoints(const RS_Vector& potentialEndPoint, QList<RS_Entity*>& entitiesList) {
    // determine angle of line
    const double segmentAngle = m_startpoint.angleTo(potentialEndPoint);

    // calculate distance of line
    const double distanceAll = m_startpoint.distanceTo(potentialEndPoint);

    // calculate length of single segment between points
    double segmentLength;

    int numberOfPoints = m_pointsCount;

    if (m_fixedDistanceMode) {
        segmentLength = m_fixedDistance;
        if (m_withinLineMode) {
            // calculate required number of points dynamically based on length of line and distance
            numberOfPoints = std::ceil(distanceAll / segmentLength);
        }
    }
    else {
        segmentLength = distanceAll / (m_pointsCount + 1);
    }

    // handle point for start edge of line
    const bool includeStartPoint = m_edgePointsMode == DRAW_EDGE_START || m_edgePointsMode == DRAW_EDGE_BOTH;
    if (includeStartPoint) {
        createPoint(m_startpoint, entitiesList);
    }

    bool lineExceeds = false;

    // proceed with intermediate points
    for (int i = 1; i <= numberOfPoints; i++) {
        // calc distance from start point to intermediate point
        const double distanceFromStart = segmentLength * i;

        if (m_fixedDistanceMode && m_withinLineMode) {
            // check whether we are still within line if we in fixed distance mode and should fit points into the line
            lineExceeds = distanceFromStart > distanceAll;
            if (lineExceeds) {
                break;
            }
        }

        // define point with needed distance and angle of line from start point
        RS_Vector point = m_startpoint.relative(distanceFromStart, segmentAngle);

        createPoint(point, entitiesList);
    }

    // handle point for end edge of line
    if (!lineExceeds) {
        const bool includeEndPoint = m_edgePointsMode == DRAW_EDGE_END || m_edgePointsMode == DRAW_EDGE_BOTH;
        if (includeEndPoint) {
            RS_Vector actualEndPoint;
            if (m_fixedDistanceMode) {
                const double endDistance = (numberOfPoints + 1) * m_fixedDistance;
                actualEndPoint = m_startpoint.relative(endDistance, segmentAngle);
            }
            else {
                actualEndPoint = potentialEndPoint;
            }
            createPoint(actualEndPoint, entitiesList);
            // endpoint field will be used for setting related zero after trigger, so
            // we'll correct it there
            m_endpoint = actualEndPoint;
        }
    }
}

/**
 * check whether start is set (and so it is valid)
 * @return
 */
bool LC_ActionDrawPointsLine::isStartPointValid() const {
    return m_startpoint.valid;
}

const RS_Vector& LC_ActionDrawPointsLine::getStartPointForAngleSnap() const {
    return m_startpoint;
}

void LC_ActionDrawPointsLine::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetDistance:
            switch (m_direction) {
                case DIRECTION_X: {
                    // calculate  point on X axis
                    const RS_Vector possiblePoint = restrictHorizontal(m_startpoint, coord);
                    if (isNonZeroLine(possiblePoint)) {
                        m_endpoint = possiblePoint;
                        addSnappedPointToVisualSnap(possiblePoint);
                        trigger();
                    }
                    break;
                }
                case DIRECTION_Y: {
                    // calculate  point on y axis
                    const RS_Vector possiblePoint = restrictVertical(m_startpoint, coord);
                    if (isNonZeroLine(possiblePoint)) {
                        addSnappedPointToVisualSnap(possiblePoint);
                        m_endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                case DIRECTION_ANGLE: {
                    // calculate end point in given angle direction
                    const RS_Vector possiblePoint = getPossibleEndPointForAngle(coord);
                    if (isNonZeroLine(possiblePoint)) {
                        addSnappedPointToVisualSnap(possiblePoint);
                        m_endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case SetAngle: {
            const RS_Vector possiblePoint = getPossibleEndPointForAngle(coord);
            if (isNonZeroLine(possiblePoint)) {
                addSnappedPointToVisualSnap(possiblePoint);
                m_endpoint = possiblePoint;
                trigger();
            }
            break;
        }
        case SetDirection:
        case SetPoint: {
            // set end to provided point
            if (isNonZeroLine(coord)) {
                // refuse zero length lines
                addSnappedPointToVisualSnap(coord);
                m_endpoint = coord;
                trigger();
            }
            break;
        }
        case SetStartPoint: {
            // setup start point of line
            m_startpoint = coord;
            m_point1Set = true;
            if (m_direction != DIRECTION_POINT && m_direction != DIRECTION_NONE) {
                setStatus(SetDistance);
            }
            else {
                setStatus(SetPoint);
            }
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            break;
        }
        case SetFixDistance:
            break;
        default:
            break;
    }
}

/**
 * Check whether distance between start point and given point is non-zero
 * @param possiblePoint second point
 * @return true if non-zero
 */
bool LC_ActionDrawPointsLine::isNonZeroLine(const RS_Vector& possiblePoint) const {
    return LC_LineMath::isNonZeroLineLength(m_startpoint, possiblePoint);
}

/**
 * Proceed commands from command widget
 * @param status
 * @param command command
 * @return true if command is processed, false - if additional processing is needed
 */
bool LC_ActionDrawPointsLine::doProceedCommand([[maybe_unused]] const int status, const QString& command) {
    bool result = true;
    bool shouldProcess = false;
    if (m_actionType == RS2::ActionDrawPointsLine) {
        const bool edgeStatus = status == SetEdge;
        if (checkCommand("edge_none", command)) {
            //specifies no points in line edges
            updateEdgePointsMode(DRAW_EDGE_NONE);
        }
        else if (checkCommand("edge_start", command)) {
            // point will be created in start point edge
            updateEdgePointsMode(DRAW_EDGE_START);
        }
        else if (checkCommand("edge_end", command)) {
            // point will be created in end point edge
            updateEdgePointsMode(DRAW_EDGE_END);
        }
        else if (checkCommand("edge_both", command)) {
            // points will be created in start and end points of line
            updateEdgePointsMode(DRAW_EDGE_BOTH);
        }
        else if (edgeStatus && checkCommand("start", command)) {
            // point will be created in start point edge
            updateEdgePointsMode(DRAW_EDGE_START);
            m_edgePointsMode = DRAW_EDGE_START;
            setMajorStatus();
        }
        else if (edgeStatus && checkCommand("end", command)) {
            updateEdgePointsMode(DRAW_EDGE_END);
            m_edgePointsMode = DRAW_EDGE_END;
            setMajorStatus();
        }
        else if (edgeStatus && checkCommand("none", command)) {
            m_edgePointsMode = DRAW_EDGE_NONE;
            setMajorStatus();
        }
        else if (edgeStatus && checkCommand("both", command)) {
            m_edgePointsMode = DRAW_EDGE_BOTH;
            setMajorStatus();
        }
        else if (checkCommand("edges", command)) {
            // initiates edge entering mode
            setStatus(SetEdge);
        }
        else if (checkCommand("dist_fixed", command)) {
            // switches to fixed distance mode
            m_fixedDistanceMode = true;
            updateOptions();
        }
        else if (checkCommand("dist_flex", command)) {
            // switches to flexible distance mode
            m_fixedDistanceMode = false;
            updateOptions();
        }
        else if (checkCommand("nofit", command)) {
            // for fixed distance mode, allows creation points outside of line
            m_withinLineMode = false;
            updateOptions();
        }
        else if (checkCommand("fit", command)) {
            //for fixed distance mode, ensures that all point are within the line
            m_withinLineMode = true;
            updateOptions();
        }
        else if (checkCommand("distance", command)) {
            // initiates entering distance between points (for fixed mode)
            setStatus(SetFixDistance);
        }
        else {
            shouldProcess = true;
        }
    }
    else {
        shouldProcess = true;
    }
    if (shouldProcess) {
        if (checkCommand("number", command)) {
            // initiates entering of points numbers (edges are not counted!)
            setStatus(SetPointsCount);
            updateOptions();
        }
        else {
            result = false;
        }
    }
    return result;
}

/**
 * processing of individual value
 * @param status
 * @param c
 * @return
 */
bool LC_ActionDrawPointsLine::doProcessCommandValue(const int status, const QString& c) {
    bool result = true;
    switch (status) {
        case SetDirection:
            // processed earlier
            break;
        case SetFixDistance: {
            bool ok = false;
            const double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)) {
                // non-zero distance is provided
                m_fixedDistance = distance;
                m_fixedDistanceMode = true;
                setMajorStatus();
            }
            else {
                result = false;
            }
            break;
        }
        case SetDistance: {
            bool ok = false;
            const double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)) {
                // non-zero distance is provided
                switch (m_direction) {
                    case DIRECTION_X: // calculate x on x axis
                        m_endpoint.x = m_startpoint.x + distance;
                        m_endpoint.y = m_startpoint.y;
                        m_endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_Y: // calculate y on y axis
                        m_endpoint.x = m_startpoint.x;
                        m_endpoint.y = m_startpoint.y + distance;
                        m_endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_ANGLE: {
                        // calculate endpoint coordinate by previously set angle and distance
                        m_endpoint = LC_LineMath::getEndOfLineSegment(m_startpoint, m_angleDegrees, distance);
                        m_endpoint.valid = true;
                        trigger();
                        break;
                    }
                    default:
                        break;
                }
            }
            else {
                result = false; // invalid value
            }
            break;
        }
        case SetPointsCount: {
            // set amount of points
            bool ok = false;
            const int count = RS_Math::eval(c, &ok);
            if (ok && count > 0) {
                // at least 1 point should be present
                updatePointsCount(count);
                setMajorStatus();
            }
            else {
                result = false; // in valid value
            }
            break;
        }
        case SetAngle: {
            // process angle value
            processAngleValueInput(c);
            break;
        }
        default:
            break;
    }
    return result;
}

QStringList LC_ActionDrawPointsLine::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetEdge:
        case SetDistance:
        case SetDirection:
        case SetPointsCount:
        case SetPoint:
        case SetAngle:
            if (m_actionType == RS2::ActionDrawPointsLine) {
                cmd += command("x");
                cmd += command("y");
                cmd += command("p");
                cmd += command("angle");
                cmd += command("edges");
                cmd += command("distance");
                cmd += command("dist_fixed");
                cmd += command("dist_flex");
                cmd += command("fit");
                cmd += command("nofit");
                cmd += command("fix");
                cmd += command("nofix");
                cmd += command("edge_none");
                cmd += command("edge_start");
                cmd += command("edge_end");
                cmd += command("edge_both");
            }
            cmd += command("number");

            break;
        default:
            break;
    }
    return cmd;
}

// FIXME - SAND - REVIEW command PROMPTS!!!

void LC_ActionDrawPointsLine::updateActionPrompt() {
    switch (getStatus()) {
        case SetStartPoint:
            updatePromptTRCancel(tr("Specify First Point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint:
            if (m_actionType == RS2::ActionDrawPointsLine) {
                updatePromptTRBack(tr("Specify Second Point\nor [number|x|y|angle|p|edges|distance]"), MOD_SHIFT_ANGLE_SNAP);
            }
            else {
                updatePromptTRBack(tr("Specify Second Point\nor [number]"), MOD_SHIFT_ANGLE_SNAP);
            }
            break;
        case SetDirection:
            updatePromptTRBack(tr("Specify line direction\n[x|y|angle|p|distance]"));
            break;
        case SetAngle:
            updatePromptTRBack(tr("Specify line direction angle\nor [x|y|p|number|edges|distance]"));
            break;
        case SetEdge:
            updatePromptTRBack(tr("Specify edge points mode\n[none|start|end|both|distance]"));
            break;
        case SetFixDistance:
            updatePromptTRBack(tr("Specify fixed distance between points\nor[x|y|p|number|edges]"));
            break;
        case SetDistance: {
            const bool toX = m_direction == DIRECTION_X;
            const bool toY = m_direction == DIRECTION_Y;
            QString msg = command("number") + "|";
            msg += command("angle") + "|";
            msg += command("p") + "|";
            msg += command("edges");
            if (toX) {
                msg += "|" + command("y");
                updatePrompt(tr("Specify distance (%1)\nor [%2]").arg(tr("X"), msg), tr("Back"));
            }
            else if (toY) {
                msg += "|" + command("x");
                updatePrompt(tr("Specify distance (%1)\nor [%2]").arg(tr("Y"), msg), tr("Back"));
            }
            else if (m_direction == DIRECTION_ANGLE) {
                msg += "|" + command("x");
                msg += "|" + command("y");
                QString angleStr = RS_Math::doubleToString(m_angleDegrees, 1);
                updatePrompt(tr("Specify  distance (angle %1 deg)\nor [%2]").arg(angleStr, msg), tr("Back"), MOD_SHIFT_MIRROR_ANGLE);
            }
            break;
        }
        case SetPointsCount:
            updatePromptTRBack(tr("Specify points count"));
            break;
        default:
            LC_AbstractActionDrawLine::updateActionPrompt();
            break;
    }
}

/**
 * Right mouse button processing - doing back for specific state
 * @param e original event
 * @param status current status
 */
void LC_ActionDrawPointsLine::doBack([[maybe_unused]] const LC_MouseEvent* e, const int status) {
    if (status == SetStartPoint) {
        // complete action
        finishAction();
    }
    else {
        // return to set start point state
        // restore relative point to start point
        moveRelativeZero(m_startpoint);
        init(SetStartPoint);
    }
}

/**
 * updates of edges mode
 * @param mode
 */
void LC_ActionDrawPointsLine::updateEdgePointsMode(const int mode) {
    m_edgePointsMode = mode;
    updateOptions();
}

void LC_ActionDrawPointsLine::setEdgePointsMode(const int value) {
    m_edgePointsMode = value;
}

void LC_ActionDrawPointsLine::updatePointsCount(const int count) {
    m_pointsCount = count;
    updateOptions();
}

void LC_ActionDrawPointsLine::setMajorStatus() {
    updateOptions();
    if (m_point1Set) {
        setStatus(SetPoint);
    }
    else {
        setStatus(SetStartPoint);
    }
}

bool LC_ActionDrawPointsLine::isAllowDirectionCommands() {
    return m_actionType == RS2::ActionDrawPointsLine;
}

bool LC_ActionDrawPointsLine::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawPointsLine::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "distance") {
        setPointsDistance(distance);
        return true;
    }
    return false;
}

/**
 * options for the action
 */
LC_ActionOptionsWidget* LC_ActionDrawPointsLine::createOptionsWidget() {
    return new LC_LinePointsOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawPointsLine::createOptionsFiller() {
    return new LC_PointsLineOptionsFiller();
}
