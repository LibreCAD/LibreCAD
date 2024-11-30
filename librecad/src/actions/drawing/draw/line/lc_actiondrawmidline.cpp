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

#include "lc_actiondrawmidline.h"
#include "lc_linemath.h"
#include "lc_midlineoptions.h"

namespace {

    //list of entity types supported by current action - only lines so far
    const auto enTypeList = EntityTypeList{RS2::EntityLine};
}

LC_ActionDrawMidLine::LC_ActionDrawMidLine(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("DrawMidLine", container, graphicView) {
    actionType = RS2::ActionDrawLineMiddle;
}

void LC_ActionDrawMidLine::trigger() {
    RS_PreviewActionInterface::trigger();
}

void LC_ActionDrawMidLine::init(int status) {
    RS_PreviewActionInterface::init(status);
    mainStatus = SetEntity1;
}

void LC_ActionDrawMidLine::mouseMoveEvent(QMouseEvent *e) {
    snapPoint(e);
    deletePreview();
    deleteHighlights();
    switch (getStatus()){
        case SetEntity1: {
            RS_Entity* ent = catchEntity(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
            if (ent != nullptr){
                highlightHover(ent);
            }
            break;
        }
        case SetEntity2:{
            highlightSelected(firstEntity);
            RS_Entity* ent = catchEntity(e, enTypeList, RS2::ResolveLevel::ResolveAllButTextImage);
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
                    previewEntity(lineInfo.line);
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

    if (alternate){
        std::swap(start2, end2);
    }

    int count = 2;

    RS_Vector mid1 = (start1 + start2) / count;
    RS_Vector mid2 = (end1 + end2) / count;

    if (LC_LineMath::isNonZeroLineLength(mid1, mid2)){
        info.middlePoint1 = mid1;
        info.middlePoint2 = mid2;

        double angle = mid1.angleTo(mid2);

        RS_Vector start = mid1.relative(-offset, angle);
        RS_Vector end = mid2.relative(offset, angle);

        auto* line = new RS_Line(nullptr, start, end);
        info.line = line;

        info.start1 = start1;
        info.start2 = start2;
        info.end1 = end1;
        info.end2 = end2;
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
    return RS_ActionInterface::getAvailableCommands();
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
    return RS_ActionInterface::doProcessCommand(status, command);
}

void LC_ActionDrawMidLine::onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) {
    RS_ActionInterface::onCoordinateEvent(status, isZero, pos);
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
