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

#include <cmath>

#include <QMouseEvent>
#include "rs_line.h"
#include "rs_layer.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_lineanglereloptions.h"
#include "lc_actiondrawlineanglerel.h"

namespace {

    //list of entity types supported by current action - only lines so far
    const auto enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse*/};
}


LC_ActionDrawLineAngleRel:: LC_ActionDrawLineAngleRel(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    double angle,
    bool fixedAngle)
    :LC_AbstractActionWithPreview("Draw Line with angle",
                               container, graphicView),
     tickAngle(angle),
     tickData{nullptr}{

    // the same action may be used for drawing orthogonal lines and lines with specified angles
    if( fixedAngle &&
        RS_Math::getAngleDifference(RS_Math::deg2rad(angle), M_PI_2) < RS_TOLERANCE_ANGLE){
        actionType = RS2::ActionDrawLineOrthogonalRel;
        relativeAngle = true;
    }
    else
        actionType =  RS2::ActionDrawLineAngleRel;
}

LC_ActionDrawLineAngleRel::~LC_ActionDrawLineAngleRel() = default;


bool LC_ActionDrawLineAngleRel::doCheckMayTrigger(){
    return tickData != nullptr;
}

bool LC_ActionDrawLineAngleRel::isSetActivePenAndLayerOnTrigger(){
    return false; // we control this by action as there might be divide operation
}

/**
 * Create entities for trigger operations
 * @param list
 */
void LC_ActionDrawLineAngleRel::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    auto* en = new RS_Line{container, tickData->tickLineData};
    en->setPenToActive();
    en->setLayerToActive();
    list<<en;

    // optionally, try to divide original line if needed
    if (divideLine){
        // divide requested, so first check whether original line may be expanded (not part of polyline)
        bool mayDivide = checkMayExpandEntity(tickData->line, "Line");
        if (mayDivide){
            // do divide original line
            divideOriginalLine(tickData, list);
        }
    }
}
/**
 * Tries to divide original line into to segments if tick line is between endpoints of line
 * @param data tick data
 * @param list list to which created line segments should be added
 */
void LC_ActionDrawLineAngleRel::divideOriginalLine(LC_ActionDrawLineAngleRel::TickData* data, QList<RS_Entity *> &list){
    RS_Line* line = data->line;
    RS_Vector snap = data->tickSnapPosition;

    // find nearest point on original line for snap point
    RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, false);

    RS_Vector start = line->getStartpoint();
    RS_Vector end = line->getEndpoint();

    // create segments only if tick snap point is between of original lines endpoints
    if (nearestPoint != start && nearestPoint != end){
        // our snap point is not outside of line
        RS_Pen pen = line->getPen();
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
RS_Vector LC_ActionDrawLineAngleRel::doGetRelativeZeroAfterTrigger(){
    return tickData->tickLineData.endpoint; // just rely on endpoint of created tick
}

/**
 * Removes original line, if needed
 */
void LC_ActionDrawLineAngleRel::performTriggerDeletions(){
    if (tickData->deleteOriginalLine){
        // removing original line from drawing
        deleteEntityUndoable(tickData->line);
    }
}

void LC_ActionDrawLineAngleRel::doAfterTrigger(){
    // just do cleanup
    unHighlightEntity();
    delete tickData;
    tickData = nullptr;
}

void LC_ActionDrawLineAngleRel::doFinish([[maybe_unused]]bool updateTB){
    if (tickData != nullptr){
        delete tickData;
        tickData = nullptr;
    }
}

/**
 * Processing of left mouse click
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawLineAngleRel::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
        switch (status) {
            case SetLine:{ // line selection state
                RS_Entity* en = catchModifiableEntity(e, enTypeList);
                if (en != nullptr) {
                    auto* line = dynamic_cast<RS_Line *>(en);
                    // determine where tick line should be snapped on original line
                    RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snapPoint, true);
                    RS_Vector tickSnapPosition = obtainLineSnapPointForMode(line, nearestPoint);

                    // prepare line data for the snap point
                    auto tickEnd = RS_Vector(false);
                    tickData = prepareLineData(line, tickSnapPosition, tickEnd, alternativeActionMode);

                    // if length is not fixed, we need additional input from the user for the length
                    if (lengthIsFree){
                        setStatus(SetTickLength);
                    }
                    else{
                        // length of tick is fixed, triggering action
                        trigger();
                    }
                }
                break;
            }
            case SetTickLength:{ // tick length selection state

                    TickData* oldData = tickData;
                    // update tick data based on last snap point that is used for calculation of tick length
                    tickData = prepareLineData( tickData->line, tickData->tickSnapPosition, snapPoint, alternativeActionMode);
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

bool LC_ActionDrawLineAngleRel::doCheckMayDrawPreview[[maybe_unused]]([[maybe_unused]]QMouseEvent *event, [[maybe_unused]]int status){
    return true;  // can draw preview in any state
}


/**
 * Creating preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */

//fixme - divide & intersection points (as for line from point to entity)
void LC_ActionDrawLineAngleRel::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    switch (status) {
        case SetLine:{ // line select state
            RS_Entity* en = catchModifiableEntity(e, enTypeList);
            if (en != nullptr){
                auto* line = dynamic_cast<RS_Line *>(en);

                highlightHover(line);

                // determine snap point
                RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, true);
                RS_Vector tickSnapPosition = obtainLineSnapPointForMode(line, nearestPoint);
                auto tickEnd = RS_Vector(false);

                // calculate tick temporary data
                TickData* data = prepareLineData(line, tickSnapPosition, tickEnd, alternativeActionMode);

                // create line and add it to preview
                // todo - createLine()
                auto *previewLine = new RS_Line{container, data->tickLineData};
                list << previewLine;

                if (showRefEntitiesOnPreview) {
                    // add reference points
                    if (lineSnapMode == LINE_SNAP_FREE) {
                        createRefSelectablePoint(data->tickSnapPosition, list);
                    } else {
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
        case SetTickLength: { // tick length setting state

            highlightSelected(tickData->line);
            // create temporary preview tick data
            TickData* data = prepareLineData( tickData->line, tickData->tickSnapPosition, snap, alternativeActionMode);

            // create preview line
            auto *previewLine = new RS_Line{container, data->tickLineData};
            list<< previewLine;

            if (showRefEntitiesOnPreview) {
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
LC_ActionDrawLineAngleRel::TickData* LC_ActionDrawLineAngleRel::prepareLineData(RS_Line* targetLine, const RS_Vector& tickSnapPosition, const RS_Vector& tickEndSnapPosition, bool useAlternateAngle){
    auto* result = new TickData();

    // store position for later use
    result->tickSnapPosition = tickSnapPosition;

    auto const vp = targetLine->getNearestPointOnEntity(tickSnapPosition, false);

    // prepare angle
    double angle = tickAngle;
    if (useAlternateAngle){
        angle = 180 - tickAngle;
    }

    double tickAngleRad = RS_Math::deg2rad(angle);

    // handle relative angle, if needed
    double actualTickAngle = tickAngleRad;
    if (relativeAngle){
        actualTickAngle = actualTickAngle + targetLine->getTangentDirection(vp).angle();
    }

    // handle offset of tick line by preparing offset vector
    RS_Vector vectorOffset(0,0,0);
    if (LC_LineMath::isMeaningful(tickOffset)){
        vectorOffset = RS_Vector::polar(tickOffset, actualTickAngle);
    }

    // handle tick length if length is free
    double actualTickLength = tickLength;
    if (lengthIsFree){
        if (tickEndSnapPosition.valid){

            /// just create vector of length 10 using angle of tick that is used to set direction
            RS_Vector infiniteTickEndPoint = tickSnapPosition.relative(10.0, actualTickAngle);

            // determine end tick position for provided snap point - it will lay on direction vector
            RS_Vector tickEndPosition = LC_LineMath::getNearestPointOnInfiniteLine(tickEndSnapPosition, tickSnapPosition, infiniteTickEndPoint);

            // calculate resulting length of tick
            RS_Vector tickVector = tickEndPosition - tickSnapPosition;
            actualTickLength = tickVector.magnitude();

            actualTickLength = LC_LineMath::getMeaningful(actualTickLength, 1.0);

            // determine sign of length

            RS_Vector point1 = targetLine->getStartpoint();
            RS_Vector point2 = targetLine->getEndpoint();

            int pointPosition = LC_LineMath::getPointPosition(point1,point2, tickEndPosition);
            if (pointPosition == LC_LineMath::RIGHT){
                if (tickSnapMode != TICK_SNAP_END){
                    actualTickLength = -actualTickLength;
                }
            }
            else if (pointPosition == LC_LineMath::LEFT){
                if (tickSnapMode == TICK_SNAP_END){
                    actualTickLength = -actualTickLength;
                }
            }
            if (tickSnapMode == TICK_SNAP_MIDDLE){
                actualTickLength = actualTickLength * 2;
            }
        }
    }

    // prepare correction vector that takes into consideration specified tick snap mode
    RS_Vector vectorOffsetCorrection(0,0,0);
    switch (tickSnapMode){
        case TICK_SNAP_START:
            break;
        case TICK_SNAP_END:
            vectorOffsetCorrection =  RS_Vector::polar(-actualTickLength, actualTickAngle);
            break;
        case TICK_SNAP_MIDDLE:
            vectorOffsetCorrection =  RS_Vector::polar(-actualTickLength/2, actualTickAngle);
            break;
    }

    // determine start point of tick
    const RS_Vector tickStartPoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;
    result -> tickLineData.startpoint = tickStartPoint;

    // determine end point of tick
    result -> tickLineData.endpoint = tickStartPoint.relative(actualTickLength, actualTickAngle);
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
RS_Vector LC_ActionDrawLineAngleRel::obtainLineSnapPointForMode(RS_Line* targetLine, RS_Vector& snap) const{
    RS_Vector snapPoint;

    // angle of target line
    double angle = targetLine->getAngle1();

    // vector will use to move snap point along base original line, if needed
    RS_Vector snapDistanceCorrectionVector = RS_Vector(0, 0, 0);

    if (LC_LineMath::isMeaningful(snapDistance)){
        // if some distance from snap is set, calculate shift for snap
        snapDistanceCorrectionVector = RS_Vector::polar(snapDistance, angle);
    }

    switch (lineSnapMode) {
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
    }
    return snapPoint;
};

RS2::CursorType LC_ActionDrawLineAngleRel::doGetMouseCursor(int status){
    switch (status){
        case SetLine:
            return RS2::SelectCursor;
        case SetTickLength:
            return RS2::CadCursor;
        default:
            return LC_AbstractActionWithPreview::doGetMouseCursor(status);
    }
}

void LC_ActionDrawLineAngleRel::updateMouseButtonHints() {
    bool hasModifiers = (actionType == RS2::ActionDrawLineAngleRel);
    switch (getStatus()) {
        case SetLine:
            updateMouseWidgetTRCancel(tr("Select base line"), hasModifiers ? MOD_SHIFT_MIRROR_ANGLE : MOD_NONE);
            break;
        case SetTickLength:
            updateMouseWidgetTRBack(tr("Specify length"), hasModifiers ? MOD_SHIFT_MIRROR_ANGLE : MOD_NONE);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLineAngleRel::createOptionsWidget(){
    return new LC_LineAngleRelOptions();
}
