/****************************************************************************
**
* Action that draws a line from selected line by specified angle (absolute or
* relating with given length.

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

#include "lc_action_draw_line_angle_rel.h"

#include "lc_line_angle_rel_options_filler.h"
#include "lc_line_angle_rel_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_polyline.h"

namespace {
    //list of entity types supported by current action - only lines so far
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse*/};
}

QString LC_ActionDrawLineAngleRel::getSettingsGroupName(){
    return m_fixedAngle ? "ActionDrawLineOrthogonalRel" : "ActionDrawLineAngleRel";
}

void LC_ActionDrawLineAngleRel::doSaveOptions(){
    save("Length", m_tickLength);
    if (!m_fixedAngle){
        save("Angle", m_tickAngleDegrees);
        save("AngleIsRelative", m_relativeAngle);
    }
    save("LengthIsFree", m_lengthIsFree);
    save("Offset", m_tickOffset);
    save("LineSnapMode", m_lineSnapMode);
    save("TickSnapMode", m_tickSnapMode);
    save("DoDivide", m_divideLine);
    save("SnapDistance", m_intersectionPointOffsetDistance);
}

void LC_ActionDrawLineAngleRel::doLoadOptions() {
    m_tickLength = loadDouble("Length", 1.0);
    if (!m_fixedAngle){
        m_tickAngleDegrees = loadDouble("Angle", 0);
        m_relativeAngle = loadBool("AngleIsRelative", false);
    }
    m_lengthIsFree = loadBool("LengthIsFree", false);
    m_tickOffset = loadDouble("Offset", 0.0);
    m_lineSnapMode = loadInt("LineSnapMode", LC_AbstractActionWithPreview::LINE_SNAP_START);
    m_tickSnapMode = loadInt("TickSnapMode", TICK_SNAP_END);
    m_divideLine = loadBool("DoDivide", false);
    m_intersectionPointOffsetDistance = loadDouble("SnapDistance", 0.0);
}

LC_ActionDrawLineAngleRel::LC_ActionDrawLineAngleRel(LC_ActionContext* actionContext, const double angle, const bool fixedAngle)
    : LC_AbstractActionWithPreview(fixedAngle ? "ActionDrawLineAngleRel" : "ActionDrawLineOrthogonalRel", actionContext), m_fixedAngle{fixedAngle} {
    // the same action may be used for drawing orthogonal lines and lines with specified angles
    if (fixedAngle && RS_Math::getAngleDifference(RS_Math::deg2rad(angle), M_PI_2) < RS_TOLERANCE_ANGLE) {
        m_actionType = RS2::ActionDrawLineOrthogonalRel;
        m_relativeAngle = true;
        m_tickAngleDegrees = angle;
    }
    else {
        m_actionType = RS2::ActionDrawLineAngleRel;
    }
}

LC_ActionDrawLineAngleRel::~LC_ActionDrawLineAngleRel() = default;

void LC_ActionDrawLineAngleRel::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        showOptions();
        setLine(entity, clickPos);
    }
}

bool LC_ActionDrawLineAngleRel::doCheckMayTrigger() {
    return m_tickData != nullptr;
}

bool LC_ActionDrawLineAngleRel::isSetActivePenAndLayerOnTrigger() {
    return false; // we control this by action as there might be divide operation
}

/**
 * Create entities for trigger operations
 * @param ctx
 */
bool LC_ActionDrawLineAngleRel::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    auto* en = new RS_Line{m_document, m_tickData->tickLineData};
    setPenAndLayerToActive(en);
    ctx += en;

    // optionally, try to divide original line if needed
    if (m_divideLine) {
        // divide requested, so first check whether original line may be expanded (not part of polyline)
        const bool mayDivide = checkMayExpandEntity(m_tickData->line, "Line");
        if (mayDivide) {
            // do divide original line
            divideOriginalLine(m_tickData, ctx.entitiesToAdd);
        }
    }

    if (m_tickData->deleteOriginalLine) {
        // removing original line from drawing
        ctx -= m_tickData->line;
    }
    return true;
}

/**
 * Tries to divide original line into to segments if tick line is between endpoints of line
 * @param data tick data
 * @param list list to which created line segments should be added
 */
void LC_ActionDrawLineAngleRel::divideOriginalLine(TickData* data, QList<RS_Entity*>& list) const {
    const RS_Line* line = data->line;
    const RS_Vector snap = data->tickSnapPosition;

    // find nearest point on original line for snap point
    const RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, false);

    const RS_Vector start = line->getStartpoint();
    const RS_Vector end = line->getEndpoint();

    // create segments only if tick snap point is between of original lines endpoints
    if (nearestPoint != start && nearestPoint != end) {
        // our snap point is not outside of line
        const RS_Pen pen = line->getPen(false);
        RS_Layer* layer = line->getLayer();

        // create first segment
        auto* line1 = createLine(start, nearestPoint, list);
        line1->setPen(pen);
        line1->setLayer(layer);

        // create second segment
        auto* line2 = createLine(nearestPoint, end, list);
        line2->setPen(pen);
        line2->setLayer(layer);

        // as we're here, notify that original line should be deleted as part of trigger
        data->deleteOriginalLine = true;
    }
}

/**
 * setting relative zero after trigger
 * @return
 */
RS_Vector LC_ActionDrawLineAngleRel::doGetRelativeZeroAfterTrigger() {
    return m_tickData->tickLineData.endpoint; // just rely on endpoint of created tick
}

void LC_ActionDrawLineAngleRel::doAfterTrigger() {
    // just do cleanup
    unHighlightEntity();
    delete m_tickData;
    m_tickData = nullptr;
}

void LC_ActionDrawLineAngleRel::doFinish() {
    if (m_tickData != nullptr) {
        delete m_tickData;
        m_tickData = nullptr;
    }
}

void LC_ActionDrawLineAngleRel::setLine(RS_Entity* en, const RS_Vector& snapPoint) {
    auto* line = dynamic_cast<RS_Line*>(en);
    // determine where tick line should be snapped on original line
    const RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snapPoint, true);
    const RS_Vector tickSnapPosition = obtainLineSnapPointForMode(line, nearestPoint);

    // prepare line data for the snap point
    const auto tickEnd = RS_Vector(false);
    m_tickData = prepareLineData(line, tickSnapPosition, tickEnd, m_alternativeActionMode);

    // if length is not fixed, we need additional input from the user for the length
    if (m_lengthIsFree) {
        setStatus(SetTickLength);
    }
    else {
        // length of tick is fixed, triggering action
        trigger();
    }
}

/**
 * Processing of left mouse click
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawLineAngleRel::doOnLeftMouseButtonRelease(const LC_MouseEvent* e, const int status, const RS_Vector& snapPoint) {
    switch (status) {
        case SetLine: {
            // line selection state
            RS_Entity* en = catchModifiableEntity(e, g_enTypeList);
            if (en != nullptr) {
                setLine(en, snapPoint);
            }
            invalidateSnapSpot();
            break;
        }
        case SetTickLength: {
            // tick length selection state
            const TickData* oldData = m_tickData;
            // update tick data based on last snap point that is used for calculation of tick length
            m_tickData = prepareLineData(m_tickData->line, m_tickData->tickSnapPosition, snapPoint, m_alternativeActionMode);
            delete oldData;
            // we're set now, so triggering action
            trigger();
            setStatus(SetLine);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawLineAngleRel::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event,
                                                                      [[maybe_unused]] int status) {
    return true; // can draw preview in any state
}

/**
 * Creating preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */

//fixme - divide & intersection points (as for line from point to entity)
void LC_ActionDrawLineAngleRel::doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector& snap, QList<RS_Entity*>& list,
                                                         const int status) {
    switch (status) {
        case SetLine: {
            // line select state
            deleteSnapper();
            RS_Entity* en = catchModifiableAndDescribe(e, g_enTypeList);
            if (en != nullptr) {
                auto* line = dynamic_cast<RS_Line*>(en);

                highlightHover(line);

                // determine snap point
                const RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, true);
                const RS_Vector tickSnapPosition = obtainLineSnapPointForMode(line, nearestPoint);
                const auto tickEnd = RS_Vector(false);

                // calculate tick temporary data
                const TickData* data = prepareLineData(line, tickSnapPosition, tickEnd, m_alternativeActionMode);

                // create line and add it to preview
                const auto* previewLine = createLine(data->tickLineData.startpoint, data->tickLineData.endpoint, list);
                previewEntityToCreate(previewLine, false);

                if (m_showRefEntitiesOnPreview) {
                    // add reference points
                    if (m_lineSnapMode == LINE_SNAP_FREE) {
                        createRefSelectablePoint(data->tickSnapPosition, list);
                    }
                    else {
                        createRefPoint(data->tickSnapPosition, list);
                    }
                    createRefPoint(data->tickLineData.endpoint, list);
                    createRefPoint(data->tickLineData.endpoint, list);
                }

                // don't need temporary data, so delete it
                delete data;
            }
            break;
        }
        case SetTickLength: {
            // tick length setting state

            highlightSelected(m_tickData->line);
            // create temporary preview tick data
            const TickData* data = prepareLineData(m_tickData->line, m_tickData->tickSnapPosition, snap, m_alternativeActionMode);

            // create preview line
            const auto* previewLine = createLine(data->tickLineData.startpoint, data->tickLineData.endpoint, list);
            previewEntityToCreate(previewLine, false);

            if (m_showRefEntitiesOnPreview) {
                // add reference points
                createRefPoint(data->tickSnapPosition, list);
                createRefPoint(data->tickLineData.endpoint, list);
                createRefSelectablePoint(data->tickLineData.endpoint, list);
            }

            // delete temporary data
            delete data;
            break;
        }
        default:
            break;
    }
}

/**
 * Method that calculates position of tick line
 * @param targetLine base line selected by the user
 * @param tickSnapPosition position of tick snap
 * @param tickEndSnapPosition end of tick snap position, if any (if valid)
 * @param useAlternateAngle  should we use original or alternative angle
 * @return tick data
 */
LC_ActionDrawLineAngleRel::TickData* LC_ActionDrawLineAngleRel::prepareLineData(RS_Line* targetLine, const RS_Vector& tickSnapPosition,
                                                                                const RS_Vector& tickEndSnapPosition,
                                                                                const bool useAlternateAngle) const {
    auto* result = new TickData();

    // store position for later use
    result->tickSnapPosition = tickSnapPosition;

    const double tickAngleDegrees = m_tickAngleDegrees;
    // prepare angle
    double angleDegress = tickAngleDegrees;
    if (useAlternateAngle) {
        angleDegress = 180 - tickAngleDegrees;
    }

    const double angleRad = RS_Math::deg2rad(angleDegress);

    // handle relative angle, if needed
    double wcsTickAngle;
    if (m_relativeAngle) {
        const auto vp = targetLine->getNearestPointOnEntity(tickSnapPosition, false);
        const double targetLineOwnAngle = targetLine->getTangentDirection(vp).angle();
        wcsTickAngle = toWorldAngleFromUCSBasis(angleRad + toUCSBasisAngle(targetLineOwnAngle));
    }
    else {
        wcsTickAngle = toWorldAngleFromUCSBasis(angleRad);
    }

    // handle offset of tick line by preparing offset vector
    RS_Vector vectorOffset(0, 0, 0);
    if (LC_LineMath::isMeaningful(m_tickOffset)) {
        vectorOffset = RS_Vector::polar(m_tickOffset, wcsTickAngle);
    }

    // handle tick length if length is free
    double actualTickLength = m_tickLength;
    const int tickSnapMode = m_tickSnapMode;
    if (m_lengthIsFree) {
        if (tickEndSnapPosition.valid) {
            /// just create vector of length 10 using angle of tick that is used to set direction
            const RS_Vector infiniteTickEndPoint = tickSnapPosition.relative(10.0, wcsTickAngle);

            // determine end tick position for provided snap point - it will lay on direction vector
            const RS_Vector tickEndPosition = LC_LineMath::getNearestPointOnInfiniteLine(
                tickEndSnapPosition, tickSnapPosition, infiniteTickEndPoint);

            // calculate resulting length of tick
            const RS_Vector tickVector = tickEndPosition - tickSnapPosition;
            actualTickLength = tickVector.magnitude();

            actualTickLength = LC_LineMath::getMeaningful(actualTickLength, 1.0);

            // determine sign of length

            const RS_Vector point1 = targetLine->getStartpoint();
            const RS_Vector point2 = targetLine->getEndpoint();

            const int pointPosition = LC_LineMath::getPointPosition(point1, point2, tickEndPosition);

            if (pointPosition == LC_LineMath::RIGHT) {
                if (tickSnapMode != TICK_SNAP_END) {
                    actualTickLength = -actualTickLength;
                }
            }
            else if (pointPosition == LC_LineMath::LEFT) {
                if (tickSnapMode == TICK_SNAP_END) {
                    actualTickLength = -actualTickLength;
                }
            }
            if (tickSnapMode == TICK_SNAP_MIDDLE) {
                actualTickLength = actualTickLength * 2;
            }
        }
    }

    // prepare correction vector that takes into consideration specified tick snap mode
    RS_Vector vectorOffsetCorrection(0, 0, 0);
    switch (tickSnapMode) {
        case TICK_SNAP_START:
            break;
        case TICK_SNAP_END:
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength, wcsTickAngle);
            break;
        case TICK_SNAP_MIDDLE:
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength / 2, wcsTickAngle);
            break;
        default:
            break;
    }

    // determine start point of tick
    const RS_Vector tickStartPoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;
    result->tickLineData.startpoint = tickStartPoint;

    // determine end point of tick
    result->tickLineData.endpoint = tickStartPoint.relative(actualTickLength, wcsTickAngle);
    result->line = targetLine;

    return result;
}

/**
 * Calculates actual tick snap point for given line and snap point. Based on line snap mode and distance from snap,
 * determines coordinate where tick line will intersect with original line.
 * @param targetLine original base line
 * @param snap  snap coordinates
 * @return  tick snap coordinate
 */
RS_Vector LC_ActionDrawLineAngleRel::obtainLineSnapPointForMode(const RS_Line* targetLine, const RS_Vector& snap) const {
    RS_Vector snapPoint;

    // vector will use to move snap point along base original line, if needed
    auto snapDistanceCorrectionVector = RS_Vector(0, 0, 0);

    if (LC_LineMath::isMeaningful(m_intersectionPointOffsetDistance)) {
        // angle of target line
        const double angle = targetLine->getAngle1();

        // if some distance from snap is set, calculate shift for snap
        snapDistanceCorrectionVector = RS_Vector::polar(m_intersectionPointOffsetDistance, angle);
    }

    switch (m_lineSnapMode) {
        case LINE_SNAP_FREE:
            // in free snap mode, just used projection of snap point to infinite ray on which original line is located
            snapPoint = LC_LineMath::getNearestPointOnInfiniteLine(snap, targetLine->getStartpoint(), targetLine->getEndpoint());
            break;
        case LINE_SNAP_START:
            // for this mode, just perform shift to specified distance from start point of base line
            snapPoint = targetLine->getStartpoint() + snapDistanceCorrectionVector;
            break;
        case LINE_SNAP_END:
            // for this mode, just perform shift to specified distance from end point of base line
            snapPoint = targetLine->getEndpoint() + snapDistanceCorrectionVector;
            break;
        case LINE_SNAP_MIDDLE:
            // for this mode, just perform shift to specified distance from middle point of base line
            snapPoint = targetLine->getMiddlePoint() + snapDistanceCorrectionVector;
            break;
        default:
            break;
    }
    return snapPoint;
}

RS2::CursorType LC_ActionDrawLineAngleRel::doGetMouseCursor(const int status) {
    switch (status) {
        case SetLine:
            return RS2::SelectCursor;
        case SetTickLength:
            return RS2::CadCursor;
        default:
            return LC_AbstractActionWithPreview::doGetMouseCursor(status);
    }
}

void LC_ActionDrawLineAngleRel::updateActionPrompt() {
    const bool hasModifiers = m_actionType == RS2::ActionDrawLineAngleRel;
    switch (getStatus()) {
        case SetLine:
            // fixme - sand - support mirrorring of snap by CTRL?
            updatePromptTRCancel(tr("Select base line"), hasModifiers ? MOD_SHIFT_MIRROR_ANGLE : MOD_NONE);
            break;
        case SetTickLength:
            updatePromptTRBack(tr("Specify length"), hasModifiers ? MOD_SHIFT_MIRROR_ANGLE : MOD_NONE);
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawLineAngleRel::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setTickAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawLineAngleRel::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length") {
        setTickLength(distance);
        return true;
    }
    if (tag == "offset") {
        setTickOffset(distance);
        return true;
    }
    if (tag == "snapDistance") {
        setIntersectionOffsetDistance(distance);
        return true;
    }
    return false;
}


LC_ActionOptionsWidget* LC_ActionDrawLineAngleRel::createOptionsWidget() {
    return new LC_LineAngleRelOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineAngleRel::createOptionsFiller() {
    return new LC_LineAngleRelOptionsFiller();
}

bool LC_ActionDrawLineAngleRel::isInVisualSnapStatus(int status) {
    return (status == SetTickLength);
}
