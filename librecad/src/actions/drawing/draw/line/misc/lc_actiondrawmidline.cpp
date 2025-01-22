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

#include <QMouseEvent>

#include "lc_actiondrawmidline.h"
#include "lc_linemath.h"
#include "lc_midlineoptions.h"
#include "rs_graphicview.h"
#include "rs_document.h"
#include "rs_math.h"
#include "rs_information.h"

namespace {

    //list of entity types supported by current action - only lines so far
    const auto enTypeList = EntityTypeList{RS2::EntityLine};
}

LC_ActionDrawMidLine::LC_ActionDrawMidLine(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("DrawMidLine", container, graphicView) {
    actionType = RS2::ActionDrawLineMiddle;
}

void LC_ActionDrawMidLine::init(int status) {
    RS_PreviewActionInterface::init(status);
    mainStatus = SetEntity1;
}

void LC_ActionDrawMidLine::doTrigger() {
    if (document != nullptr) {
        LineInfo lineInfo;
        prepareLine(lineInfo, secondEntity, alternateEndpoints);
        RS_Line *lineToCreate = lineInfo.line;
        if (lineToCreate != nullptr) {
            lineToCreate->reparent(container);
            setPenAndLayerToActive(lineToCreate);
            undoCycleAdd(lineToCreate);
        }
    }
    setStatus(SetEntity1);
}

void LC_ActionDrawMidLine::mouseMoveEvent(QMouseEvent *e) {
    deletePreview();
    deleteHighlights();
    snapPoint(e);
    switch (getStatus()){
        case SetEntity1: {
            RS_Entity* ent = catchEntityOnPreview(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
            if (ent != nullptr){
                highlightHover(ent);
            }
            break;
        }
        case SetEntity2:{
            highlightSelected(firstEntity);
            RS_Entity* ent = catchEntityOnPreview(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
            if (ent != nullptr){
                highlightHover(ent);
                bool alternate = isShift(e);
                LineInfo lineInfo;
                prepareLine(lineInfo, ent, alternate);
                if (lineInfo.line != nullptr){
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(lineInfo.start1, lineInfo.start2);
                        previewRefLine(lineInfo.end1, lineInfo.end2);
                        previewRefPoint(lineInfo.start1);
                        previewRefPoint(lineInfo.start2);
                        previewRefPoint(lineInfo.end1);
                        previewRefPoint(lineInfo.end2);
                        previewRefPoint(lineInfo.middlePoint1);
                        previewRefPoint(lineInfo.middlePoint2);
                    }
                    previewEntityToCreate(lineInfo.line);
                }
            }
            break;
        }
        default:
            break;
    }
    drawPreview();
    drawHighlights();
}


// fixme - more division points?
void LC_ActionDrawMidLine::prepareLine(LC_ActionDrawMidLine::LineInfo &info, RS_Entity *ent, bool alternate) {
    RS_Vector start1 = firstEntity->getStartpoint();
    RS_Vector end1 = firstEntity->getEndpoint();

    RS_Vector start2 = ent->getStartpoint();
    RS_Vector end2 = ent->getEndpoint();
//
//    if (alternate){
//        std::swap(start2, end2);
//    }

    int count = 2;

    RS_Vector mid1 = (start1 + start2) / count;
    RS_Vector mid2 = (end1 + end2) / count;

    RS_Vector altMid1 = (start1 + end2) / count;
    RS_Vector altMid2 = (start2 + end1) / count;

    double dist = mid1.distanceTo(mid2);
    double altDist = altMid1.distanceTo(altMid2);

    bool useAlt = altDist > dist;

    if (alternate){
        useAlt = !useAlt;
    }

    RS_Vector point1 = useAlt ? altMid1 : mid1;
    RS_Vector point2 = useAlt ? altMid2 : mid2;

    bool mayProceed = false;
    if (LC_LineMath::isNonZeroLineLength(point1, point2)){
        mayProceed = true;
    }
    else { // check for parallel lines. In this case - just draw line between centers of lines.
        RS_VectorSolutions const &sol = RS_Information::getIntersection(firstEntity, ent, false);
        if (!sol.hasValid()) {
            point1  = (start1 + end1) / count;
            point2 = (start2 + end2) / count;
            mayProceed = LC_LineMath::isNonZeroLineLength(point1, point2);
        }
    }

    if (mayProceed){
        info.middlePoint1 = point1;
        info.middlePoint2 = point2;

        double angle = point1.angleTo(point2);

        RS_Vector start = point1.relative(-offset, angle);
        RS_Vector end = point2.relative(offset, angle);

        auto* line = new RS_Line(nullptr, start, end);
        info.line = line;

        info.start1 = start1;
        info.start2 = useAlt ? end2 : start2;
        info.end1 = end1;
        info.end2 = useAlt ? start2 :end2;
    }
    else{
        info.line = nullptr;
    }
}

void LC_ActionDrawMidLine::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetEntity1: {
            RS_Entity* ent = catchEntity(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
            if (ent != nullptr){
                firstEntity = ent;
                setStatus(SetEntity2);
                mainStatus = SetEntity2;
            }
            break;
        }
        case SetEntity2:{
            highlightSelected(firstEntity);
            RS_Entity* ent = catchEntity(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
            if (ent != nullptr){
                secondEntity = ent;
            }
            alternateEndpoints = isShift(e);
            trigger();
            break;
        }
        default:
            break;
    }
}

QStringList LC_ActionDrawMidLine::getAvailableCommands() {
    QStringList cmd;
    cmd << command("offset");
    return cmd;
}

void LC_ActionDrawMidLine::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    switch (getStatus()){
        case SetEntity1:{
            setStatus(-1);
            break;
        }
        case SetEntity2:{
            setStatus(SetEntity1);
            break;
        }
        case SetOffset:{
            restoreMainStatus();
            break;
        }
        default:
            updateMouseWidget();
    }
}

bool LC_ActionDrawMidLine::doProcessCommand(int status, const QString &command) {
    bool accept = false;
    if (checkCommand(command, "offset")){
        mainStatus = status;
        setStatus(SetOffset);
    }
    else{
        bool ok;
        double a = RS_Math::eval(command, &ok);
        if (LC_LineMath::isNotMeaningful(a)){
            a = 0.0;
        }
        if (ok){
            accept = true;
            offset = a;
        } else {
            commandMessage(tr("Not a valid expression"));
        }
        updateOptions();
        restoreMainStatus();
    }
    return accept;
}

void LC_ActionDrawMidLine::onCoordinateEvent(int status, bool isZero, [[maybe_unused]]const RS_Vector &pos) {
    if (status == SetOffset){
        if (isZero){
            setOffset(0.0);
            restoreMainStatus();
        }
    }
}

void LC_ActionDrawMidLine::updateMouseButtonHints() {
    switch (getStatus()){
        case SetEntity1:{
            updateMouseWidgetTRCancel(tr("Select first entity"));
            break;
        }
        case SetEntity2:{
            updateMouseWidgetTRBack(tr("Select second entity"), MOD_SHIFT_LC(tr("Alternate endpoints")));
            break;
        }
        case SetOffset:{
            updateMouseWidgetTRBack(tr("Enter offset value"));
            break;
        }
        default:
            updateMouseWidget();
    }
}

RS2::CursorType LC_ActionDrawMidLine::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

double LC_ActionDrawMidLine::getOffset() const {
    return offset;
}

void LC_ActionDrawMidLine::setOffset(double o) {
    offset = o;
}

LC_ActionOptionsWidget *LC_ActionDrawMidLine::createOptionsWidget() {
    return new LC_MidLineOptions();
}
