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

#include "lc_action_modify_line_gap.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_line_gap_options_filler.h"
#include "lc_line_gap_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_line.h"

namespace {
    //list of entity types supported by current action - line
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::Entity,CircleRS2::EntityEllipse*/};
}

LC_ActionModifyLineGap::LC_ActionModifyLineGap(LC_ActionContext *actionContext)
    :LC_AbstractActionWithPreview("ActionModifyLineGap",actionContext, RS2::ActionModifyLineGap){
}

void LC_ActionModifyLineGap::doSaveOptions() {
    save("GapSize", m_gapSize);
    save("GapFree", m_freeGapSize);
    save("LineSnap", m_lineSnapMode);
    save("SnapDistance", m_gapSnapDistance);
    save("GapSnap", m_gapSnapMode);
}

void LC_ActionModifyLineGap::doLoadOptions() {
    m_gapSize = loadDouble("GapSize", 1.0);
    m_freeGapSize = loadBool("GapFree", true);
    m_lineSnapMode = loadInt("LineSnap", 0);
    m_gapSnapDistance  = loadDouble("SnapDistance", 0.0);
    m_gapSnapMode = loadInt("GapSnap", 0);
}

bool LC_ActionModifyLineGap::isInVisualSnapStatus(int status) {
    return (status == SetGapEndPoint);
}

void LC_ActionModifyLineGap::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
   if (isLine(contextEntity) && checkMayExpandEntity(contextEntity, "")) {
       // todo - don't preselect line so far, as it will not let the user to set options. Review flow later...
   }
}

void LC_ActionModifyLineGap::doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, const int status){
    switch (status){
        case SetEntity:{ // selecting the line
            // finding line entity
            deleteSnapper();
            RS_Entity* en = catchModifiableAndDescribe(e, g_enTypeList);
            if (en != nullptr){
                auto *line = static_cast<RS_Line *>(en);
                // check that line may be expanded
                if (checkMayExpandEntity(line, "")){

                    // determine snap point
                    const RS_Vector nearestPoint = line->getNearestPointOnEntity(snap);

                    // determine start point for gap
                    const RS_Vector gapStartPosition = obtainLineSnapPointForMode(line, nearestPoint);

                    highlightHover(line);

                    // calculate gap temporary data
                    const GapData *data = prepareGapData(line, snap, gapStartPosition);
                    createPreviewEntities(data, list, false);

                    if (isInfoCursorForModificationEnabled()){
                        msg(tr("Line Gap"))
                            .vector(tr("Start:"), data->startPoint)
                            .vector(tr("End:"), data->endPoint)
                            .toInfoCursorZone2(false);
                    }

                    // don't need temporary data, so delete it
                    delete data;
                }
            }
            break;
        }
        case SetGapEndPoint:{ // free gap size mode

            RS_Line *line = m_gapData->originalLine;

            highlightSelected(line);

            // gap end is projection of snap point to previously selected line
            const RS_Vector nearestPoint = line->getNearestPointOnEntity(snap);
            m_gapData->endPoint = nearestPoint;

            createPreviewEntities(m_gapData, list, true);

            if (isInfoCursorForModificationEnabled()){
                msg(tr("Line Gap"))
                    .vector(tr("Start:"), m_gapData->startPoint)
                    .vector(tr("End:"), m_gapData->endPoint)
                    .toInfoCursorZone2(false);
            }

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
void LC_ActionModifyLineGap::createPreviewEntities(const GapData *data, QList<RS_Entity *> &list, const bool startPointNoSelected) const{
    const RS_Vector &startPoint = data->startPoint;
    const RS_Vector &endPoint = data->endPoint;

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

void LC_ActionModifyLineGap::doOnLeftMouseButtonRelease(const LC_MouseEvent* e, const int status, const RS_Vector &snapPoint){
    switch (status){
        case SetEntity:{ // entity selection
            // catching the line
            RS_Entity* en = catchModifiableEntity(e, g_enTypeList);
            if (en != nullptr) {
                auto* line = static_cast<RS_Line *>(en);
                // check whether line is expandable
                if (checkMayExpandEntity(line, "")){
                    // determine where gap should be positioned on original line
                    const RS_Vector nearestPoint = line->getNearestPointOnEntity(snapPoint);
                    const RS_Vector gapStartPosition = obtainLineSnapPointForMode(line, nearestPoint);

                    // prepare gap data for the gap
                    m_gapData = prepareGapData(line, snapPoint, gapStartPosition);

                    if (m_freeGapSize){
                        // if length is not fixed, we need additional input from the user for the length, so go to the next step
                        setStatus(SetGapEndPoint);
                    } else {
                        // length of gap is fixed, triggering action
                        trigger();
                    }
                }
            }
            invalidateSnapSpot();
            break;
        }
        case SetGapEndPoint: {
            const RS_Line *line = m_gapData->originalLine;

            // update gap end position as projection of snap point on selected entity
            const RS_Vector nearestPoint = line->getNearestPointOnEntity(snapPoint);
            m_gapData->endPoint = nearestPoint;

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
    return m_gapData != nullptr; // may trigger if we have data for gap
}

bool LC_ActionModifyLineGap::isSetActivePenAndLayerOnTrigger(){
    return false; // we'll pick attributes from original line
}

bool LC_ActionModifyLineGap::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx){
    if (m_gapData == nullptr) {
        return false;
    }

     const RS_Line* originalLine = m_gapData->originalLine;

     const RS_Vector lineStart = originalLine->getStartpoint();
     const RS_Vector lineEnd = originalLine->getEndpoint();

     RS_Vector gapStart = m_gapData->startPoint;
     RS_Vector gapEnd = m_gapData->endPoint;

     const double angle = originalLine->getAngle1();

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
         if (lineStart.x > rotatedGapStart.x) {
             // start of gap is outside of line
             // here we create segment for the gap itself - IF gap is fully outside the line. So it
             // will add segment, actually.
             if (lineStart.x - rotatedGapEnd.x >= 0) {
                 gapOutsideTheLine = true;
                 auto* segment1 = createLine(gapStart, gapEnd, ctx.entitiesToAdd);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
                 // prevent deletion of original line
                 m_gapData->originalLine = nullptr;
             }
         }
         else {
             // gap is not on the edge, actual creation of line segment is needed
             if (rotatedGapStart.x < rotatedLineEnd.x) {
                 // we'll create first segment only if gap start point is not outsider of line (after line endpoint)
                 const auto segment1 = createLine(lineStart, gapStart, ctx.entitiesToAdd);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
             }
         }
     }

     if (!gapOutsideTheLine){
         // check that gap is not in the end of line, so actual creation of line segment is needed
         if (LC_LineMath::isMeaningfulDistance(lineEnd, gapEnd)){
             // gap is not on the edge

             if ((rotatedGapStart.x - rotatedLineEnd.x) >= 0) {
                 // gap is outside of line, so we'll extend the line
                 auto *segment1 = createLine(gapStart, gapEnd, ctx.entitiesToAdd);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment1, PEN_ORIGINAL, LAYER_ORIGINAL);
                 // prevent deletion of original line
                 m_gapData->originalLine = nullptr;
             }
             else {
                 auto *segment2 = createLine(gapEnd, lineEnd, ctx.entitiesToAdd);
                 // apply attributes from original line
                 applyPenAndLayerBySourceEntity(originalLine, segment2, PEN_ORIGINAL, LAYER_ORIGINAL);
             }
         }
     }

    // just deleting original entity as it is replaced by created segments
    RS_Line* line = m_gapData->originalLine;
    if (line != nullptr) {
        ctx -= line;
    }

   return true;
}

void LC_ActionModifyLineGap::doAfterTrigger(){
    delete m_gapData; // just do a cleanup
    m_gapData = nullptr;
}

/**
 * calculating position of gap start based on action options and snap point
 * @param targetLine
 * @param snap
 * @return
 */
RS_Vector LC_ActionModifyLineGap::obtainLineSnapPointForMode(const RS_Line* targetLine, const RS_Vector& snap) const{
    RS_Vector snapPoint;

    // vector will use to move gap snap point along base original line, if needed
    auto snapDistanceCorrectionVector = RS_Vector(0, 0, 0);


    int lineSnap = m_lineSnapMode;
    double distanceForSnap = m_gapSnapDistance;

    // alternating snap point for simpler handling of end segments of line
    // here we actually mirror snapping to another edge of line
    if (m_alternativeActionMode){
        switch (m_lineSnapMode){
            case LINE_SNAP_START:
                lineSnap = LINE_SNAP_END;
                distanceForSnap = -m_gapSnapDistance;
                break;
            case LINE_SNAP_END:
                lineSnap = LINE_SNAP_START;
                distanceForSnap = -m_gapSnapDistance;
                break;
            case LINE_SNAP_MIDDLE:
                distanceForSnap = -m_gapSnapDistance;
                break;
            default:
               break;
        }
    }

    if (LC_LineMath::isMeaningful(m_gapSnapDistance)){
        // angle of target line
        const double angle = targetLine->getAngle1();

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
}


/**
 * Calculates gap data for fixed size gap
 * @param line original line
 * @param snap snap point
 * @param startPoint gap start point
 * @return gap data
 */
LC_ActionModifyLineGap::GapData *LC_ActionModifyLineGap::prepareGapData(RS_Line *line, [[maybe_unused]]const RS_Vector &snap, const RS_Vector &startPoint) const{

    RS_Vector gapStart;
    RS_Vector gapEnd;

    if (!m_freeGapSize){

        const double size = m_gapSize;

        // angle of target line
        const double angle = line->getAngle1();

        // vector that describes gap (from zero point)
        const RS_Vector gapVector = RS_Vector::polar(size, angle);

        // vector that will be used for adjusting start point of gap based on gap snap mode
        RS_Vector snapCorrectionVector;

        int gapSnap = m_gapSnapMode;
        if (m_alternativeActionMode){
            if (m_gapSnapMode == GAP_SNAP_START){
                gapSnap = GAP_SNAP_END;
            }
            else if (m_gapSnapMode == GAP_SNAP_END){
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
        if (m_lineSnapMode == LINE_SNAP_FREE){
            const RS_Vector lineStartPoint = line->getStartpoint();
            const RS_Vector lineEndPoint = line->getEndpoint();

            // check that we're not outside the line, if it so - limit gap by line edge points
            const double distanceToEnd = gapStart.distanceTo(lineEndPoint);
            if (distanceToEnd < m_gapSize){
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

 void LC_ActionModifyLineGap::updateActionPrompt(){
     switch (getStatus()) {
         case SetEntity: {
             updatePromptTRCancel(tr("Select line"), MOD_SHIFT_LC(tr("Use Alternative Line Endpoint")));
             break;
         }
         case SetGapEndPoint: {
             updatePromptTRBack(tr("Select endpoint of gap"));
             break;
         }
         default:
             LC_AbstractActionWithPreview::updateActionPrompt();
             break;
     }
 }

void LC_ActionModifyLineGap::doFinish(){
    if (m_gapData != nullptr){
        delete m_gapData;
        m_gapData = nullptr;
    }
}

LC_ActionOptionsWidget* LC_ActionModifyLineGap::createOptionsWidget(){
    return new LC_LineGapOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyLineGap::createOptionsFiller() {
    return new LC_LineGapOptionsFiller();
}

RS2::CursorType LC_ActionModifyLineGap::doGetMouseCursor(const int status) {
    switch (status) {
        case SetEntity: {
            return RS2::SelectCursor;
        }
        case SetGapEndPoint: {
            return RS2::CadCursor;
        }
        default:
            return RS2::NoCursorChange;
    }
}

bool LC_ActionModifyLineGap::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "size") {
        setGapSize(distance);
        return true;
    }
    if (tag == "snap") {
        setGapSnapDistance(distance);
        return true;
    }
    return false;
}
