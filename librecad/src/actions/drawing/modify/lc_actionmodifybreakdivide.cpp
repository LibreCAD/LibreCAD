/****************************************************************************
**
* Action that breaks line, arc or circle to segments by points of
* intersection with other entities.

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

#include <cfloat>
#include "lc_actionmodifybreakdivide.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_containertraverser.h"
#include "lc_division.h"
#include "lc_linemath.h"
#include "lc_modifybreakdivideoptions.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_pen.h"

namespace {
    //list of entity types supported by current action - line, arc, circle
    const auto g_enTypeList = EntityTypeList{RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle/*,RS2::EntityEllipse*/};
}

LC_ActionModifyBreakDivide::LC_ActionModifyBreakDivide(LC_ActionContext *actionContext)
   :LC_AbstractActionWithPreview("Break Out",actionContext, RS2::ActionModifyBreakDivide){
}

bool LC_ActionModifyBreakDivide::doCheckMayDrawPreview([[maybe_unused]]LC_MouseEvent *event, int status){
    return status == SetLine;
}

/**
 * Creating preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionModifyBreakDivide::doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SetLine){
        deleteSnapper();
        RS_Entity *en = catchModifiableAndDescribe(e, g_enTypeList);
        if (en != nullptr){
            int rtti = en->rtti();
            switch (rtti) {
                case RS2::EntityLine: { // process line
                    auto *line = dynamic_cast<RS_Line *>(en);
                    createEntitiesForLine(line, snap, list, true);
                    break;
                }
                case RS2::EntityCircle: { //process circle
                    auto *circle = dynamic_cast<RS_Circle *>(en);
                    createEntitiesForCircle(circle, snap, list, true);
                    break;
                }
                case RS2::EntityArc: { // process arc
                    auto *arc = dynamic_cast<RS_Arc *>(en);
                    createEntitiesForArc(arc, snap, list, true);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void LC_ActionModifyBreakDivide::doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapPoint){
    if (status == SetLine){
        RS_Entity *en = catchModifiableEntity(e, g_enTypeList);
        if (en != nullptr){
            int rtti = en->rtti();
            switch (rtti) {
                case RS2::EntityLine:
                case RS2::EntityCircle:
                case RS2::EntityArc:
                    // store information about entity and snap point and pass to trigger()
                    m_triggerData = new TriggerData();
                    m_triggerData->entity = en;
                    m_triggerData->snapPoint = snapPoint;
                    trigger();
                    break;
                default:
                    break;
            }
        }
        invalidateSnapSpot();
    }
}

bool LC_ActionModifyBreakDivide::doCheckMayTrigger(){
    bool result = false;
    if (m_triggerData != nullptr){
        RS_Entity* en = m_triggerData->entity;
        RS_Vector snap = m_triggerData->snapPoint;
        if (en != nullptr){
            int rtti = en->rtti();
            // do processing of individual entity types
            switch (rtti) {
                case RS2::EntityLine: {
                    auto *line = dynamic_cast<RS_Line *>(en);
                    createEntitiesForLine(line, snap, m_triggerData->entitiesToCreate, false);
                    break;
                }
                case RS2::EntityCircle: {
                    auto *circle = dynamic_cast<RS_Circle *>(en);
                    createEntitiesForCircle(circle, snap, m_triggerData->entitiesToCreate, false);
                    break;
                }
                case RS2::EntityArc: {
                    auto *arc = dynamic_cast<RS_Arc *>(en);
                    createEntitiesForArc(arc, snap, m_triggerData->entitiesToCreate, false);
                    break;
                }
                default:
                    break;
            }
        }
        if (m_triggerData->entitiesToCreate.isEmpty()){
            commandMessage(tr("Invalid entity selected - no segments between intersections to break/divide."));
        }
        else {
            result = true;
        }
    }
    return result;
}

bool LC_ActionModifyBreakDivide::isSetActivePenAndLayerOnTrigger(){
    return false; // action will handle attributes
}

void LC_ActionModifyBreakDivide::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    list.append(m_triggerData->entitiesToCreate);
}

void LC_ActionModifyBreakDivide::performTriggerDeletions(){
    // delete original entity as we'll expand it and create segment entities
    undoableDeleteEntity(m_triggerData->entity);
}

void LC_ActionModifyBreakDivide::doAfterTrigger(){
    m_triggerData->entitiesToCreate.clear();
    delete m_triggerData;
    m_triggerData = nullptr;
}

void LC_ActionModifyBreakDivide::doFinish([[maybe_unused]]bool updateTB){
    if (m_triggerData != nullptr){
        delete m_triggerData;
        m_triggerData = nullptr;
    }
}

/**
 * creating segment entities for preview or trigger for line entity
 * @param line selected line
 * @param snap snap point
 * @param list list to which entities should be added
 * @param preview true if entities for preview
 */
void LC_ActionModifyBreakDivide::createEntitiesForLine(RS_Line *line, RS_Vector &snap, QList<RS_Entity*> &list, bool preview){
    // check whether selection entity may be expanded
    if (checkMayExpandEntity(line, "")){
        // determine snap point projection on line
        RS_Vector nearestPoint = LC_LineMath::getNearestPointOnLine(line, snap, false);
        RS_Vector start = line->getStartpoint();
        RS_Vector end = line->getEndpoint();

        // create segments only if tick snap point is between of original lines endpoints
        if (nearestPoint != start && nearestPoint != end){
            // calculate segments data
            LC_Division division(m_container);
            bool allowEntireLineAsSegment = m_alternativeActionMode && m_removeSegments;
            LC_Division::LineSegmentData *data = division.findLineSegmentBetweenIntersections(line, snap, allowEntireLineAsSegment);
            if (data != nullptr){
                if (preview){
                    highlightHover(line);
                }

                // determine which segments should be created
                bool createSnapSegment = !preview;
                bool createNonSnapSegments = !preview;

                if (m_removeSegments){
                    if (preview){
                        createSnapSegment = m_removeSelected;
                        createNonSnapSegments = !m_removeSelected;
                    }
                    else{
                        createSnapSegment = !m_removeSelected;
                        createNonSnapSegments = m_removeSelected;
                    }
                }

                // attributes of original entity
                RS_Pen pen = line->getPen(false);
                RS_Layer* layer = line->getLayer(true);

                // creating snap segment (where snap was performed)
                if (createSnapSegment){
                    createLineEntity(preview, data->snapSegmentStart, data->snapSegmentEnd, pen, layer, list);
                }
                int segmentDisposition = data->segmentDisposition;

                // in preview mode provide visual indication of division/break points
                if (preview){
                    // check that start of segment is not line endpoint
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_START){
                        createRefSelectablePoint(data->snapSegmentStart, list);
                    }

                    // check that end of segment is not line endpoint
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_END){
                        createRefSelectablePoint(data->snapSegmentEnd, list);
                    }

                    if (isInfoCursorForModificationEnabled()) {
                        msg(tr("Break/Divide Line"))
                            .vector(tr("Point 1:"), data->snapSegmentStart)
                            .vector(tr("Point 2:"), data->snapSegmentEnd)
                            .toInfoCursorZone2(false);
                    }
                }

                // create segments of line that are outside of segment selected by the user
                if (createNonSnapSegments){
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_START){
                        // we don't need this segment if snap is between line start point and intersection point
                        createLineEntity(preview, line->getStartpoint(), data->snapSegmentStart, pen, layer, list);
                    }
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_END){
                        // we don't need this segment if snap is between line end point and intersection point
                        createLineEntity(preview, data->snapSegmentEnd, line->getEndpoint(), pen, layer, list);
                    }
                }
            }
            // don't need temporary data we used anymore, so delete it
            delete data;
        }
    }
}
/**
 * Creates individual line for given coordinates. If not for preview, also assigns provided pen and layer attributes
 * @param preview
 * @param start
 * @param end
 * @param pen
 * @param layer
 * @param list
 */
void LC_ActionModifyBreakDivide::createLineEntity(bool preview, const RS_Vector &start, const RS_Vector &end,
                                                  const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const{
    if (preview){
        createRefLine(start, end, list);
        createLine(start, end, list);
    }
    else{
        auto *createdLine = createLine(start, end, list);
        createdLine->setPen(pen);
        createdLine->setLayer(layer);
    }
}

/**
 * Creates arc segment entities for provided circle and snap point
 * @param circle
 * @param snap
 * @param list
 * @param preview
 */
void LC_ActionModifyBreakDivide::createEntitiesForCircle(RS_Circle *circle, RS_Vector &snap, QList<RS_Entity *> &list, bool preview){
    // check that we may expand the circle
    if (checkMayExpandEntity(circle, "")){
        // determine snap point projection on entity
        RS_Vector nearestPoint = circle->getNearestPointOnEntity(snap, true);

        // compute segment data
        LC_Division division(m_container);
        bool allowEntireCircleAsSegment = m_alternativeActionMode && m_removeSegments;
        LC_Division::CircleSegmentData *data = division.findCircleSegmentBetweenIntersections(circle, nearestPoint, allowEntireCircleAsSegment);
        if (data != nullptr){

            if (preview){
                highlightHover(circle);
            }

            const RS_Vector &center = circle->getCenter();
            double radius = circle->getRadius();

            // basic arc info
            RS_ArcData arcData;
            arcData.radius = radius;
            arcData.center = center;
            arcData.angle1 = data->snapSegmentStartAngle;
            arcData.angle2 = data->snapSegmentEndAngle;
            arcData.reversed = false;

            // determine which segments should be created of circle
            bool createSnapSegment = !preview;
            bool createNonSnapSegments = !preview;

            if (m_removeSegments){
                if (preview){
                    createSnapSegment = m_removeSelected;
                    createNonSnapSegments = !m_removeSelected;
                }
                else{
                    createSnapSegment = !m_removeSelected;
                    createNonSnapSegments = m_removeSelected;
                }
            }

            // attributes of original circle
            RS_Pen pen = circle->getPen(false);
            RS_Layer* layer = circle->getLayer();

            // create snap arc segment
            if (createSnapSegment){
                createArcEntity(arcData, preview, pen, layer, list);
            }

            // create complimentary non-snap arc segment, if needed
            if (createNonSnapSegments){
                RS_ArcData arcData1 = arcData;
                arcData1.reversed = !arcData.reversed; // that ark wil be in same points, yet reversed
                createArcEntity(arcData1, preview, pen, layer, list);
             }

            // for circle divide mode, add visual indication of divide points if we're creating preview
            if (preview){
                // todo - ignore refpoints visibility for divide?
                 RS_Vector dividePoint1 = center.relative(radius, data->snapSegmentStartAngle);
                 createRefSelectablePoint(dividePoint1, list);

                 RS_Vector dividePoint2 = center.relative(radius, data->snapSegmentEndAngle);
                 createRefSelectablePoint(dividePoint2, list);

                if (isInfoCursorForModificationEnabled()){
                    msg(tr("Break/Divide Circle"))
                        .wcsAngle(tr("Angle 1:"), data->snapSegmentStartAngle)
                        .vector(tr("Point 1:"), dividePoint1)
                        .wcsAngle(tr("Angle 2:"), data->snapSegmentEndAngle)
                        .vector(tr("Point 2:"), dividePoint2)
                        .toInfoCursorZone2(false);
                }
            }
        }
        // don't need temporary data, so delete it
        delete data;
    }
}
/**
 * Creates segment entities for provided arc
 * @param arc original arc
 * @param snap snap point (where the user selected arc)
 * @param list list of entities to add
 * @param preview true if we generate entities for preview, false otherwise
 */
void LC_ActionModifyBreakDivide::createEntitiesForArc(RS_Arc *arc, RS_Vector &snap, QList<RS_Entity *> &list, bool preview){
    // check that arc is expandable
    if (checkMayExpandEntity(arc, "")){
        // determine snap point
        RS_Vector nearestPoint = arc->getNearestPointOnEntity(snap, true);        
        RS_Vector start = arc->getStartpoint();
        RS_Vector end = arc->getEndpoint();

        // create segments only if tick snap point is between of original lines endpoints
        if (nearestPoint != start && nearestPoint != end){
            // determine snap segment coordinates
            LC_Division division(m_container);
            bool allowEntireArcAsSegment = m_alternativeActionMode && m_removeSegments;
            LC_Division::ArcSegmentData *data = division.findArcSegmentBetweenIntersections(arc, snap, allowEntireArcAsSegment);
            if (data != nullptr){
                if (preview){
                    highlightHover(arc);
                }

                // determine which segment entities should be created
                bool createSnapSegment = !preview;
                bool createNonSnapSegments = !preview;

                if (m_removeSegments){
                    if (preview){
                        createSnapSegment = m_removeSelected;
                        createNonSnapSegments = !m_removeSelected;
                    } else {
                        createSnapSegment = !m_removeSelected;
                        createNonSnapSegments = m_removeSelected;
                    }
                }

                // current arc attributes
                RS_Pen pen = arc->getPen(false);
                RS_Layer* layer = arc->getLayer(true);

                // create segment where arc was selected, if necessary
                if (createSnapSegment){
                    RS_ArcData arcData = arc->getData();
                    arcData.angle1 = data->snapSegmentStartAngle;
                    arcData.angle2 = data->snapSegmentEndAngle;
                    createArcEntity(arcData, preview, pen, layer, list);
                }

                // for preview and arc break/divide mode, add points that highlights places where arc will be divided/broken
                int segmentDisposition = data->segmentDisposition;

                // create non-snap segments, if necessary
                if (createNonSnapSegments){
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_START){
                        RS_ArcData arcData1 = arc->getData();
                        arcData1.angle1 = arc->getAngle1();
                        arcData1.angle2 = data->snapSegmentStartAngle;
                        createArcEntity(arcData1, preview, pen, layer, list);
                    }

                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_END){
                        RS_ArcData arcData2 = arc->getData();
                        arcData2.angle1 = data->snapSegmentEndAngle;
                        arcData2.angle2 = arc->getAngle2();
                        createArcEntity(arcData2, preview, pen, layer, list);
                    }
                }

                if (preview){
                    double radius = arc->getRadius();
                    RS_Vector center = arc->getCenter();
                    RS_Vector segmentStart = center.relative(radius, data->snapSegmentStartAngle);
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_START){
                        createRefSelectablePoint(segmentStart, list);
                    }

                    RS_Vector segmentEnd = center.relative(radius, data->snapSegmentEndAngle);
                    if (segmentDisposition != LC_Division::SegmentDisposition::SEGMENT_TO_END){
                        createRefSelectablePoint(segmentEnd, list);
                    }

                    if (isInfoCursorForModificationEnabled()){
                        msg(tr("Break/Divide Arc"))
                            .wcsAngle(tr("Angle 1:"), data->snapSegmentStartAngle)
                            .vector(tr("Point 1:"), segmentStart)
                            .wcsAngle(tr("Angle 2:"), data->snapSegmentEndAngle)
                            .vector(tr("Point 2:"), segmentEnd)
                            .toInfoCursorZone2(false);
                    }
                }
            }
            // don't need temporary data, so delete it
            delete data;
        }
    }
}

/**
 * Utility method that creates arc entity. If is not for preview, also applies provided attributes to  created entity
 * @param arcData data that describes arc
 * @param preview true if entity is created for preview
 * @param pen pen
 * @param layer layer
 * @param list list of entities to add created entity
 */
void LC_ActionModifyBreakDivide::createArcEntity(const RS_ArcData &arcData, bool preview, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const{
    if (preview){
        createRefArc(arcData, list);
        auto arc = new RS_Arc(m_container, arcData);
        list << arc;
    }
    else{
        auto createdArc = new RS_Arc(m_container, arcData);
        createdArc->setPen(pen);
        createdArc->setLayer(layer);
        list << createdArc;
    }

}

/**
 * For selection of entities, we'll rely on free snap mode, so point of selection is not affected by intersection points or endpoints.
 * @param e
 * @return
 */
RS_Vector LC_ActionModifyBreakDivide::doGetMouseSnapPoint(LC_MouseEvent *e){
    return e->graphPoint;
}

RS2::CursorType LC_ActionModifyBreakDivide::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

void LC_ActionModifyBreakDivide::updateMouseButtonHints(){
    updateMouseWidgetTRCancel(tr("Select line, arc or circle"), m_removeSegments ? MOD_SHIFT_LC(tr("Proceed even if no intersections")):MOD_NONE);
}

LC_ActionOptionsWidget* LC_ActionModifyBreakDivide::createOptionsWidget(){
    return new LC_ModifyBreakDivideOptions();
}
