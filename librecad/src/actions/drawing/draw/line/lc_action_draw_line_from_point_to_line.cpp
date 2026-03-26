/****************************************************************************
**
* Action that draws a line from given point to selected line.
* Created line may be either orthogonal to selected line, or be with
* some specified angle.

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
**********************************************************************/
#include "lc_action_draw_line_from_point_to_line.h"

#include "lc_line_from_point_to_line_options_filler.h"
#include "lc_line_from_point_to_line_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"

// fixme - sand - add support of line and circle!

LC_ActionDrawLineFromPointToLine::LC_ActionDrawLineFromPointToLine(LC_ActionContext* actionContext)
    : LC_AbstractActionWithPreview("ActionDrawLineFromPointToLine", actionContext, RS2::ActionDrawLineFromPointToLine) {
}

void LC_ActionDrawLineFromPointToLine::doSaveOptions() {
    save("Orthogonal", m_orthogonalMode);
    save("Angle", m_angleDegrees);
    save("Length", m_length);
    save("SnapMode", m_lineSnapMode);
    save("SizeMode", m_sizeMode);
    save("EndOffset", m_endOffset);
}

void LC_ActionDrawLineFromPointToLine::doLoadOptions() {
    m_orthogonalMode = loadBool("Orthogonal", true);
    m_angleDegrees = loadDouble("Angle", 90);
    m_length = loadDouble("Length", 1.0);
    m_lineSnapMode = loadInt("SnapMode", 0);
    m_sizeMode = loadInt("SizeMode", 0);
    m_endOffset = loadDouble("EndOffset", 0.0);
}

void LC_ActionDrawLineFromPointToLine::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        m_targetLine = static_cast<RS_Line*>(entity);
        m_selectLineFirst = true;
    }
}

/*
 * check that we're fine to trigger
 */
bool LC_ActionDrawLineFromPointToLine::doCheckMayTrigger() {
    if (m_selectLineFirst) {
        return m_startPoint.valid && m_targetLine != nullptr;
    }
    return m_targetLine != nullptr;
}

/**
 * just create line according to given parameters
 */
bool LC_ActionDrawLineFromPointToLine::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    RS_Vector intersection;
    RS_Line* line = createLineFromPointToTarget(m_targetLine, m_startPoint, intersection);
    ctx += line;
    return true;
}

/*
 * do post trigger cleanup and go to point selection state
 */
void LC_ActionDrawLineFromPointToLine::doAfterTrigger() {
    if (m_selectLineFirst) {
        m_startPoint = RS_Vector(false);
    }
    else {
        m_targetLine = nullptr;
        m_startPoint = RS_Vector(false);
    }
    restoreSnapMode();
    setStatus(SetPoint);
}

void LC_ActionDrawLineFromPointToLine::doBack(const LC_MouseEvent* e, const int status) {
    if (status == SelectLine) {
        restoreSnapMode();
    }
    LC_AbstractActionWithPreview::doBack(e, status);
}

void LC_ActionDrawLineFromPointToLine::doFinish() {
    const int status = getStatus();
    if (m_selectLineFirst) {
        if (status == SetPoint) {
            restoreSnapMode();
        }
    }
    else {
        if (status == SelectLine) {
            restoreSnapMode();
        }
    }
}

/**
 * support of snapping to relative point on mouse move
 * @return
 */
int LC_ActionDrawLineFromPointToLine::doGetStatusForInitialSnapToRelativeZero() {
    return SetPoint;
}

/**
 * rely on relative zero for first point
 * @param zero
 */
void LC_ActionDrawLineFromPointToLine::doInitialSnapToRelativeZero(const RS_Vector& zero) {
    // we'll use current relative point as starting point on pre-snap (mouse move + SHIFT)
    m_startPoint = zero;
    setStatus(SelectLine);
    setFreeSnap();
}

/**
 * left mouse clicks processing
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawLineFromPointToLine::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status,
                                                                  const RS_Vector& snapPoint) {
    switch (status) {
        case SetPoint: {
            onCoordinateEvent(status, false, snapPoint);
            break;
        }
        case SelectLine: {
            // try to select target line
            RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr) {
                m_targetLine = dynamic_cast<RS_Line*>(en);
                if (!m_selectLineFirst) {
                    trigger();
                    invalidateSnapSpot();
                }
            }
            break;
        }
        default:
            break;
    }
}

/**
 * need preview if point was selected only
 * @param event
 * @param status
 * @return
 */
bool LC_ActionDrawLineFromPointToLine::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, const int status) {
    if (m_selectLineFirst) {
        return status == SetPoint;
    }
    return status != SetPoint; // draw preview if we're selecting the line only
}

/**
 * snap entity, check whether it's line and build line from start point to that line
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::doPreparePreviewEntities([[maybe_unused]] const LC_MouseEvent* e, RS_Vector& snap,
                                                                QList<RS_Entity*>& list, const int status) {
    if (m_selectLineFirst) {
        if (status == SetPoint) {
            deleteSnapper();

            highlightHover(m_targetLine);
            auto intersectionPoint = RS_Vector(false);
            const auto line = createLineFromPointToTarget(m_targetLine, snap, intersectionPoint);
            if (m_showRefEntitiesOnPreview) {
                createRefPoint(line->getEndpoint(), list);
                if (m_sizeMode == SIZE_INTERSECTION && LC_LineMath::isMeaningful(m_endOffset)) {
                    createRefPoint(intersectionPoint, list);
                }
            }

            previewEntityToCreate(line, false);
            if (m_showRefEntitiesOnPreview) {
                createRefSelectablePoint(snap, list);
                createRefPoint(m_startPoint, list);
            }
            list << line;
        }
    }
    else if (status == SelectLine) {
        deleteSnapper();
        RS_Entity* en = catchModifiableAndDescribe(e, RS2::EntityLine);
        RS_Line* line;
        if (en != nullptr) {
            const auto potentialLine = dynamic_cast<RS_Line*>(en);
            highlightHover(potentialLine);
            auto intersectionPoint = RS_Vector(false);
            line = createLineFromPointToTarget(potentialLine, m_startPoint, intersectionPoint);
            if (m_showRefEntitiesOnPreview) {
                createRefPoint(line->getEndpoint(), list);
                if (m_sizeMode == SIZE_INTERSECTION && LC_LineMath::isMeaningful(m_endOffset)) {
                    createRefPoint(intersectionPoint, list);
                }
            }
        }
        else {
            line = new RS_Line(m_startPoint, snap);
        }
        previewEntityToCreate(line, false);
        if (m_showRefEntitiesOnPreview) {
            createRefSelectablePoint(snap, list);
            createRefPoint(m_startPoint, list);
        }
        list << line;
    }
}

bool LC_ActionDrawLineFromPointToLine::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawLineFromPointToLine::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "offset") {
        setEndOffset(distance);
        return true;
    }
    if (tag == "length") {
        setLength(distance);
        return true;
    }
    return false;
}

bool LC_ActionDrawLineFromPointToLine::isInVisualSnapStatus(int status) {
    return (status == SetPoint);
}

/**
 * processing of coordinates for start point via mouse click or command widget
 * @param coord
 * @param isZero
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    if (m_selectLineFirst) {
        m_startPoint = coord;
        addSnappedPointToVisualSnap(coord);
        trigger();
        invalidateSnapSpot();
    }
    else if (status == SetPoint) {
        m_startPoint = coord;
        // for simplicity of line selection, remove snap restrictions until we'll select a line
        setFreeSnap();
        setStatus(SelectLine);
        // relative zero will remain in starting point
        addSnappedPointToVisualSnap(coord);
        moveRelativeZero(coord);
    }
}

/**
 * Central method for building the line. First, for the simplicity of calculations, we'll rotate coordinates of target line around start point,
 * so the line will be parallel to x axis.
 * Than, calculate proper angles for the vector direction.
 * That direction vector is positioned to start point, and based on the size policy we build the line in the direction of the vector
 * either to the point of intersection of target line and ray defined by the direction vector, or just build the line of given length.
 * As soon as all calculations are performed, perform backward rotation.
 *
 * @param line
 * @param point
 * @param intersection
 * @return
 */
RS_Line* LC_ActionDrawLineFromPointToLine::createLineFromPointToTarget(const RS_Line* line, const RS_Vector& point,
                                                                       RS_Vector& intersection) const {
    RS_Vector lineStart = line->getStartpoint();
    RS_Vector lineEnd = line->getEndpoint();

    double targetLineAngle = lineStart.angleTo(lineEnd);

    // rotate line coordinates around lineStart point, so they will be parallel to X axis

    lineStart.rotate(point, -targetLineAngle);
    lineEnd.rotate(point, -targetLineAngle);

    // define angle that should be used
    double vectorAngle;

    if (m_orthogonalMode) {
        vectorAngle = RS_Math::deg2rad(90);
    }
    else {
        // determine which angle should be used - normal or alternate
        // alternative angle allows to simplify angle setting - so in ui the angle is within 0..90 degrees.
        double angleToUse = m_angleDegrees;
        if (m_alternativeActionMode) {
            angleToUse = -m_angleDegrees;
        }
        double resultingAngle = RS_Math::deg2rad(angleToUse);
        vectorAngle = resultingAngle;
        //        vectorAngle = RS_Math::correctAngle3(resultingAngle);
    }

    if (point.y >= lineStart.y) {
        // lineStart point is above
    }
    else {
        vectorAngle = -vectorAngle;
    }

    // create direction vector

    RS_Vector directionVector = RS_Vector::polar(m_length, vectorAngle);

    RS_Vector ortLineStart;
    RS_Vector ortLineEnd;

    switch (m_sizeMode) {
        case SIZE_INTERSECTION: {
            // create line from point to intersection point
            ortLineStart = point; // in this mode, we just build the line from start point

            // calculate end point of direction vector positioned in start point
            RS_Vector vectorEnd = point + directionVector;

            // determine intersection point
            RS_Vector intersectionPoint = LC_LineMath::getIntersectionLineLine(point, vectorEnd, lineStart, lineEnd);

            if (intersectionPoint.valid) {
                // rotate intersection back to return to drawing coordinates
                RS_Vector restoredIntersection = intersectionPoint.rotate(point, targetLineAngle);
                // process end offset from intersection point, if needed
                RS_Vector endPoint = restoredIntersection;
                if (LC_LineMath::isMeaningful(m_endOffset)) {
                    RS_Vector offsetVector = RS_Vector::polar(m_endOffset, point.angleTo(restoredIntersection));
                    endPoint = restoredIntersection + offsetVector;
                }

                intersection = restoredIntersection;

                // end of the line to be build is intersection point
                ortLineEnd = endPoint;
            }
            else {
                // should not be there - if we're here, it means calculation error, since it is always should be possible to create a line from point to line
                ortLineEnd = ortLineStart;
            }
            break;
        }
        case SIZE_FIXED_LENGTH: {
            // create line from point in direction to intersection point yet with given length, taking into consideration snap mode
            switch (m_lineSnapMode) {
                case SNAP_START: {
                    // start point of line will be in initial point
                    // correct start point according to current snap mode
                    ortLineStart = point;
                    // define end point of line
                    if (m_alternativeActionMode) {
                        ortLineEnd = ortLineStart + directionVector;
                    }
                    else {
                        ortLineEnd = ortLineStart - directionVector;
                    }
                    break;
                }
                case SNAP_END: {
                    // end point of the line will be in initial point
                    ortLineEnd = point;
                    if (m_alternativeActionMode) {
                        ortLineStart = ortLineEnd - directionVector;
                    }
                    else {
                        ortLineStart = ortLineEnd + directionVector;
                    }

                    break;
                }
                case SNAP_MIDDLE: {
                    // middle point of line will be in initial point
                    RS_Vector vectorOffsetCorrection = RS_Vector::polar(m_length / 2, vectorAngle);
                    if (m_alternativeActionMode) {
                        ortLineStart = point - vectorOffsetCorrection;
                        ortLineEnd = point + vectorOffsetCorrection;
                    }
                    else {
                        ortLineStart = point + vectorOffsetCorrection;
                        ortLineEnd = point - vectorOffsetCorrection;
                    }
                    break;
                }
                default:
                    break;
            }
            // restore rotated position back to drawing
            ortLineStart.rotate(point, targetLineAngle);
            ortLineEnd.rotate(point, targetLineAngle);
            break;
        }
        default:
            break;
    }
    // resulting line
    auto* result = new RS_Line(m_document, ortLineStart, ortLineEnd);
    return result;
}

void LC_ActionDrawLineFromPointToLine::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint:
            updatePromptTRCancel(tr("Select Initial Point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SelectLine:
            updatePromptTRBack(tr("Select Line"),
                                    (m_orthogonalMode && (m_sizeMode == SIZE_INTERSECTION)) ? MOD_NONE : MOD_SHIFT_MIRROR_ANGLE);
            break;
        default:
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLineFromPointToLine::createOptionsWidget() {
    return new LC_LineFromPointToLineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineFromPointToLine::createOptionsFiller() {
    return new LC_LineFromPointToLineOptionsFiller();
}

RS2::CursorType LC_ActionDrawLineFromPointToLine::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
