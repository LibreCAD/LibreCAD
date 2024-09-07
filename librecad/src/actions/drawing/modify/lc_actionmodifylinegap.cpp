/****************************************************************************
**
* Action that creates a gap in selected line and so creates two line segments

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

#include "lc_linemath.h"
#include "lc_actionmodifylinegap.h"
#include "lc_modifygapoptions.h"
#include "lc_abstractactionwithpreview.h"

namespace {
    //list of entity types supported by current action - line
    const auto enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::Entity,CircleRS2::EntityEllipse*/};
}

LC_ActionModifyLineGap::LC_ActionModifyLineGap(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Line Gap",
                                  container,
                                  graphicView){
    actionType = RS2::ActionModifyLineGap;
}



void LC_ActionModifyLineGap::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    switch (status){
        case (SetEntity):{ // selecting the line
            // finding line entity
            RS_Entity* en = catchModifiableEntity(e, enTypeList);
            if (en != nullptr){
                auto *line = dynamic_cast<RS_Line *>(en);
                // check that line may be expanded
                if (checkMayExpandEntity(line, "")){

                    // determine snap point
                    RS_Vector nearestPoint = line->getNearestPointOnEntity(snap);

                    // determine start point for gap
                    RS_Vector gapStartPosition = obtainLineSnapPointForMode(line, nearestPoint);

                    highlightHover(line);

                    // calculate gap temporary data
                    GapData *data = prepareGapData(line, snap, gapStartPosition);
                    createPreviewEntities(data, list, false);

                    // don't need temporary data, so delete it
                    delete data;
                }
            }
            break;
        }
        case SetGapEndPoint:{ // free gap size mode

            RS_Line *line = gapData->originalLine;

            highlightSelected(line);

            // gap end is projection of snap point to previously selected line
            RS_Vector nearestPoint = line->getNearestPointOnEntity(snap);
            gapData->endPoint = nearestPoint;

            createPreviewEntities(gapData, list, true);

            break;
        }
        default:
            break;
    }
}

/**
 * Utility method that creates entities used on preview and adds them to provided list. Entities are the line for gap location and boundary points for the gap
 * @param data data for gap
 * @param list list to add entities
 * @param startPointNoSelected - if true, first entity is not selected
 */
void LC_ActionModifyLineGap::createPreviewEntities(LC_ActionModifyLineGap::GapData *data, QList<RS_Entity *> &list, bool startPointNoSelected) const{
    RS_Vector &startPoint = data->startPoint;
    RS_Vector &endPoint = data->endPoint;

    // create preview line
    createLine(startPoint, endPoint, list);
    createRefLine(startPoint, endPoint, list);

    // create boundary points for gap for better visibility on preview
    if (startPointNoSelected){
        createRefPoint(startPoint, list);
    }
    else {
        createRefSelectablePoint(startPoint, list);
    }
    createRefSelectablePoint(endPoint, list);
}

void LC_ActionModifyLineGap::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
    switch (status){
        case SetEntity:{ // entity selection
            // catching the line
            RS_Entity* en = catchModifiableEntity(e, enTypeList);
            if (en != nullptr) {
                auto* line = dynamic_cast<RS_Line *>(en);
                // check whether line is expandable
                if (checkMayExpandEntity(line, "")){

                    // determine where gap should be positioned on original line
                    RS_Vector nearestPoint = line->getNearestPointOnEntity(snapPoint);
                    RS_Vector gapStartPosition = obtainLineSnapPointForMode(line, nearestPoint);

                    // prepare gap data for the gap
                    gapData = prepareGapData(line, snapPoint, gapStartPosition);

                    if (freeGapSize){
                        // if length is not fixed, we need additional input from the user for the length, so go to the next step
                        setStatus(SetGapEndPoint);
                    } else {
                        // length of gap is fixed, triggering action
                        trigger();
                    }
                }
            }
            break;
        }
        case SetGapEndPoint: {
            RS_Line *line = gapData->originalLine;

            // update gap end position as projection of snap point on selected entity
            RS_Vector nearestPoint = line->getNearestPointOnEntity(snapPoint);
            gapData->endPoint = nearestPoint;

            // invoke trigger
            trigger();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyLineGap::doCheckMayTrigger(){
    return gapData != nullptr; // may trigger if we have data for gap
}

bool LC_ActionModifyLineGap::isSetActivePenAndLayerOnTrigger(){
    return false; // we'll pick attributes from original line
}

void LC_ActionModifyLineGap::doPrepareTriggerEntities(QList<RS_Entity *> &list){
     RS_Line* originalLine = gapData->originalLine;

     RS_Vector lineStart = originalLine->getStartpoint();
     RS_Vector lineEnd = originalLine->getEndpoint();

     RS_Vector gapStart = gapData->startPoint;
     RS_Vector gapEnd = gapData->endPoint;

     double angle = originalLine->getAngle1();

     RS_Vector rotatedLineEnd = lineEnd;
     rotatedLineEnd.rotate(lineStart, -angle);

     RS_Vector rotatedGapStart = gapStart;
     rotatedGapStart = rotatedGapStart.rotate(lineStart, -angle);

     RS_Vector rotatedGapEnd = gapEnd;
     rotatedGapEnd = rotatedGapEnd.rotate(lineStart, -angle);

      // check the direction of gap line (it may be reversed for free mode)
     if (rotatedGapEnd.x < rotatedGapStart.x){
         // reposition gap points, so they will have the same direction as selected line
         std::swap(gapStart, gapEnd);
         std::swap(rotatedGapStart, rotatedGapEnd);
     }

     bool gapOutsideTheLine = false;

     if (LC_LineMath::isMeaningful(lineStart.x - rotatedGapStart.x)){
         // check that gap is not in the start of line
         if (lineStart.x > rotatedGapStart.x){ // start of gap is outside of line
             // here we create segment for the gap itself - IF gap is fully outside the line. So it
             // will add segment, actually.
             if ((lineStart.x - rotatedGapEnd.x)>=0){
                 gapOutsideTheLine = true;
                 auto *segment1 = createLine(gapStart, gapEnd, list);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
                 // prevent deletion of original line
                 gapData->originalLine = nullptr;
             }
         }
         else{
             // gap is not on the edge, actual creation of line segment is needed
             if (rotatedGapStart.x < rotatedLineEnd.x){
                 // we'll create first segment only if gap start point is not outsider of line (after line endpoint)
                 auto segment1 = createLine(lineStart, gapStart, list);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
             }
         }
     }

     if (!gapOutsideTheLine){
         // check that gap is not in the end of line, so actual creation of line segment is needed
         if (LC_LineMath::isMeaningfulDistance(lineEnd, gapEnd)){
             // gap is not on the edge

             if ((rotatedGapStart.x - rotatedLineEnd.x)>=0){
                 // gap is outside of line, so we'll extend the line
                 auto *segment1 = createLine(gapStart, gapEnd, list);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
                 // prevent deletion of original line
                 gapData->originalLine = nullptr;
             }
             else {
                 auto *segment2 = createLine(gapEnd, lineEnd, list);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment2, PEN_ORIGINAL, LAYER_ORIGINAL);
             }
         }
     }

}

void LC_ActionModifyLineGap::doAfterTrigger(){
    delete gapData; // just do a cleanup
    gapData = nullptr;
}

void LC_ActionModifyLineGap::performTriggerDeletions(){
    if (gapData != nullptr){
        // just deleting original entity as it is replaced by created segments
        RS_Line* line = gapData->originalLine;
        if (line != nullptr){
            deleteEntityUndoable(line);
        }
    }
}

/**
 * calculating position of gap start based on action options and snap point
 * @param targetLine
 * @param snap
 * @return
 */
RS_Vector LC_ActionModifyLineGap::obtainLineSnapPointForMode(const RS_Line* targetLine, const RS_Vector& snap) const{
    RS_Vector snapPoint;

    // angle of target line
    double angle = targetLine->getAngle1();

    // vector will use to move gap snap point along base original line, if needed
    RS_Vector snapDistanceCorrectionVector = RS_Vector(0, 0, 0);


    int lineSnap = lineSnapMode;
    double distanceForSnap = snapDistance;

    // alternating snap point for simpler handling of end segments of line
    // here we actually mirror snapping to another edge of line
    if (alternativeActionMode){
        switch (lineSnapMode){
            case LINE_SNAP_START:
                lineSnap = LINE_SNAP_END;
                distanceForSnap = -snapDistance;
                break;
            case LINE_SNAP_END:
                lineSnap = LINE_SNAP_START;
                distanceForSnap = -snapDistance;
                break;
            case LINE_SNAP_MIDDLE:
                distanceForSnap = -snapDistance;
                break;
            default:
               break;
        }
    }

    if (LC_LineMath::isMeaningful(snapDistance)){
        // if some distance from snap is set, calculate shift for snap
        snapDistanceCorrectionVector = RS_Vector::polar(distanceForSnap, angle);
    }

    switch (lineSnap) {
        case LINE_SNAP_FREE:
            // in free snap mode, just used projection of snap point to infinite ray on which original line is located
            snapPoint = snap;
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
};


/**
 * Calculates gap data for fixed size gap
 * @param line original line
 * @param snap snap point
 * @param startPoint gap start point
 * @return gap data
 */
LC_ActionModifyLineGap::GapData *LC_ActionModifyLineGap::prepareGapData(RS_Line *line, [[maybe_unused]]const RS_Vector &snap, const RS_Vector &startPoint) const{
    // angle of target line
    double angle = line->getAngle1();

    RS_Vector lineStartPoint = line->getStartpoint();
    RS_Vector lineEndPoint = line->getEndpoint();

    RS_Vector gapStart;
    RS_Vector gapEnd;

    double size = gapSize;
    if (!freeGapSize){

        // vector that describes gap (from zero point)
        RS_Vector gapVector = RS_Vector::polar(size, angle);

        // vector that will be used for adjusting start point of gap based on gap snap mode
        RS_Vector snapCorrectionVector;

        int gapSnap = gapSnapMode;
        if (alternativeActionMode){
            if (gapSnapMode == GAP_SNAP_START){
                gapSnap = GAP_SNAP_END;
            }
            else if (gapSnapMode == GAP_SNAP_END){
                gapSnap = GAP_SNAP_START;
            }
        }

        switch (gapSnap) {
            case GAP_SNAP_START: { // gap start is snap point, do nothing
                snapCorrectionVector = RS_Vector(0, 0, 0);
                break;
            }
            case GAP_SNAP_MIDDLE: // gap middle point is in snap point, so move to half of gap size
                snapCorrectionVector = RS_Vector::polar(-size / 2, angle);
                break;
            case GAP_SNAP_END: { // gap end point is in snap point, move to the length of gap
                snapCorrectionVector = RS_Vector::polar(-size, angle);
                break;
            }
            default:
                break;
        }

        // update start and end point of gap
        gapStart = startPoint + snapCorrectionVector;
        gapEnd = gapStart + gapVector;

        // check that start of gap is not outside the line
        if (lineSnapMode == LINE_SNAP_FREE){

            // check that we're not outside the line, if it so - limit gap by line edge points
            double distanceToEnd = gapStart.distanceTo(lineEndPoint);
            if (distanceToEnd < gapSize){
                gapEnd = lineEndPoint;
            }

            RS_Vector rotatedGapStart = gapStart;
            rotatedGapStart.rotate(lineStartPoint, -angle);
            if (rotatedGapStart.x < lineStartPoint.x){
                gapStart = lineStartPoint;
            }
        }

    }
    else{
        gapStart = startPoint;
        gapEnd = startPoint;
    }

    // return results
    auto* result = new GapData();
    result->originalLine = line;
    result->startPoint = gapStart;
    result->endPoint = gapEnd;

    return result;
}

 void LC_ActionModifyLineGap::updateMouseButtonHints(){
    switch (getStatus()){
        case SetEntity:{
            updateMouseWidgetTRCancel(tr("Select line"), MOD_SHIFT_LC(tr("Use Alternative Line Endpoint")));
            break;
        }
        case SetGapEndPoint:{
            updateMouseWidgetTRBack(tr("Select endpoint of gap"));
            break;
        }
    default:
        LC_AbstractActionWithPreview::updateMouseButtonHints();
    }
 }

void LC_ActionModifyLineGap::doFinish([[maybe_unused]]bool updateTB){
    if (gapData != nullptr){
        delete gapData;
        gapData = nullptr;
    }
}

LC_ActionOptionsWidget* LC_ActionModifyLineGap::createOptionsWidget(){
    return new LC_ModifyGapOptions();
}
