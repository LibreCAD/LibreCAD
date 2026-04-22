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

#include "lc_action_draw_center_line.h"

#include "lc_center_line_options_filler.h"
#include "lc_center_line_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_pen.h"

namespace {
    //list of entity types supported by current action - only lines so far
   const auto g_enTypeList = EntityTypeList{RS2::EntityLine};
}

LC_ActionDrawCenterLine::LC_ActionDrawCenterLine(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionDrawCenterLine", actionContext,RS2::ActionDrawCenterLine) {
}

void LC_ActionDrawCenterLine::doSaveOptions() {
      save("Offset", m_offset);
}

void LC_ActionDrawCenterLine::doLoadOptions() {
    m_offset  = loadDouble("Offset", 0);
}

void LC_ActionDrawCenterLine::init(const int status) {
    m_mainStatus = SetEntity1;
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawCenterLine::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        m_firstEntity = contextEntity;
        setStatus(SetEntity2);
        m_mainStatus = SetEntity2;
    }
}

bool LC_ActionDrawCenterLine::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    LineInfo lineInfo;
    prepareLine(lineInfo, m_secondEntity, m_alternateEndpoints);
    RS_Line* lineToCreate = lineInfo.line;
    if (lineToCreate != nullptr) {
        lineToCreate->reparent(m_document);
        setupCenterlinePenLayer(lineToCreate);
        ctx += lineToCreate;
        ctx.dontSetActiveLayerAndPen();
        return true;
    }
    return false;
}

void LC_ActionDrawCenterLine::doTriggerCompletion([[maybe_unused]]bool success) {
    setStatus(SetEntity1);
}

bool LC_ActionDrawCenterLine::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "offset") {
        setOffset(distance);
        return true;
    }
    return false;
}

void LC_ActionDrawCenterLine::setupCenterlinePenLayer(RS_Line* line) const{
    line->setLayerToActive(); // fixme - sand - change to some annotation layer?
    const RS2::LineType lineType = getLineTypeForCenterLine();
    RS_Pen pen = m_document->getActivePen();
    if (lineType != RS2::LineTypeUnchanged) {
        pen.setLineType(lineType);
    }
    line->setPen(pen);
}

RS2::LineType LC_ActionDrawCenterLine::getLineTypeForCenterLine() const {
    return RS2::CenterLine2; // fixme - retrieve from settings (CENTERLTYPE)
}

void LC_ActionDrawCenterLine::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status){
        case SetEntity1: {
            const RS_Entity* ent = catchAndDescribe(e, g_enTypeList, RS2::ResolveLevel::ResolveAll/*ButTextImage*/);
            if (ent != nullptr){
                highlightHover(ent);
            }
            break;
        }
        case SetEntity2:{
            highlightSelected(m_firstEntity);
            const RS_Entity* ent = catchAndDescribe(e, g_enTypeList, RS2::ResolveLevel::ResolveAll/*ButTextImage*/);
            if (ent != nullptr){
                highlightHover(ent);
                const bool alternate = e->isShift;
                LineInfo lineInfo;
                prepareLine(lineInfo, ent, alternate);
                if (lineInfo.line != nullptr){
                    if (m_showRefEntitiesOnPreview) {
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
}

// fixme - more division points?
void LC_ActionDrawCenterLine::prepareLine(LineInfo &info, const RS_Entity *ent, const bool alternate) const {
    const RS_Vector start1 = m_firstEntity->getStartpoint();
    const RS_Vector end1 = m_firstEntity->getEndpoint();

    const RS_Vector start2 = ent->getStartpoint();
    const RS_Vector end2 = ent->getEndpoint();
//
//    if (alternate){
//        std::swap(start2, end2);
//    }

    constexpr int count = 2;

    const RS_Vector mid1 = (start1 + start2) / count;
    const RS_Vector mid2 = (end1 + end2) / count;

    const RS_Vector altMid1 = (start1 + end2) / count;
    const RS_Vector altMid2 = (start2 + end1) / count;

    const double dist = mid1.distanceTo(mid2);
    const double altDist = altMid1.distanceTo(altMid2);

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
        const RS_VectorSolutions&sol = RS_Information::getIntersection(m_firstEntity, ent, false);
        if (!sol.hasValid()) {
            point1  = (start1 + end1) / count;
            point2 = (start2 + end2) / count;
            mayProceed = LC_LineMath::isNonZeroLineLength(point1, point2);
        }
    }

    if (mayProceed){
        info.middlePoint1 = point1;
        info.middlePoint2 = point2;

        const double angle = point1.angleTo(point2);

        const RS_Vector start = point1.relative(-m_offset, angle);
        const RS_Vector end = point2.relative(m_offset, angle);

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

void LC_ActionDrawCenterLine::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status){
        case SetEntity1: {
            RS_Entity* ent = catchEntityByEvent(e, g_enTypeList, RS2::ResolveLevel::ResolveAll/*ButTextImage*/);
            if (ent != nullptr){
                m_firstEntity = ent;
                setStatus(SetEntity2);
                m_mainStatus = SetEntity2;
            }
            break;
        }
        case SetEntity2:{
            highlightSelected(m_firstEntity);
            RS_Entity* ent = catchEntityByEvent(e, g_enTypeList, RS2::ResolveLevel::ResolveAll/*ButTextImage*/);
            if (ent != nullptr){
                m_secondEntity = ent;
            }
            m_alternateEndpoints = e->isShift;
            trigger();
            break;
        }
        default:
            break;
    }
}

QStringList LC_ActionDrawCenterLine::getAvailableCommands() {
    QStringList cmd;
    cmd << command("offset");
    return cmd;
}

void LC_ActionDrawCenterLine::onMouseRightButtonRelease([[maybe_unused]] const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    switch (status){
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
            updatePrompt();
    }
}

bool LC_ActionDrawCenterLine::doProcessCommand(const int status, const QString &command) {
    bool accept = false;
    if (checkCommand(command, "offset")){
        m_mainStatus = status;
        setStatus(SetOffset);
    }
    else{
        bool ok = false;
        double a = RS_Math::eval(command, &ok);
        if (LC_LineMath::isNotMeaningful(a)){
            a = 0.0;
        }
        if (ok){
            accept = true;
            m_offset = a;
        } else {
            commandMessage(tr("Not a valid expression"));
        }
        updateOptions();
        restoreMainStatus();
    }
    return accept;
}

void LC_ActionDrawCenterLine::onCoordinateEvent(const int status, const bool isZero, [[maybe_unused]]const RS_Vector &pos) {
    if (status == SetOffset){
        if (isZero){
            setOffset(0.0);
            restoreMainStatus();
        }
    }
}

void LC_ActionDrawCenterLine::updateActionPrompt() {
    switch (getStatus()){
        case SetEntity1:{
            updatePromptTRCancel(tr("Select first entity"));
            break;
        }
        case SetEntity2:{
            updatePromptTRBack(tr("Select second entity"), MOD_SHIFT_LC(tr("Alternate endpoints")));
            break;
        }
        case SetOffset:{
            updatePromptTRBack(tr("Enter offset value"));
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawCenterLine::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

double LC_ActionDrawCenterLine::getOffset() const {
    return m_offset;
}

void LC_ActionDrawCenterLine::setOffset(const double offset) {
    m_offset = offset;
}

LC_ActionOptionsWidget *LC_ActionDrawCenterLine::createOptionsWidget() {
    return new LC_CenterLineOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawCenterLine::createOptionsFiller() {
    return new LC_CenterLineOptionsFiller();
}
