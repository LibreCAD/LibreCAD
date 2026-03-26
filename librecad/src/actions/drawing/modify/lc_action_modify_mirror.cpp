/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "lc_action_modify_mirror.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_align.h"
#include "lc_mirror_options_filler.h"
#include "lc_mirror_options_widget.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"


struct LC_ActionModifyMirror::MirrorActionData {
    RS_MirrorData data;
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};

LC_ActionModifyMirror::LC_ActionModifyMirror(LC_ActionContext *actionContext)
        :LC_ActionModifyBase("ActionModifyMirror", actionContext, RS2::ActionModifyMirror)
        , m_actionData(std::make_unique<MirrorActionData>()){
    m_mirrorToExistingLine = false;
}

LC_ActionModifyMirror::~LC_ActionModifyMirror() = default;

void LC_ActionModifyMirror::doSaveOptions() {
    save("ToLine", m_mirrorToExistingLine);
    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
}

void LC_ActionModifyMirror::doLoadOptions() {
    m_mirrorToExistingLine = loadBool("ToLine", false);
    const bool useCurrentLayer = loadBool("UseCurrentLayer", true);
    setUseCurrentLayer(useCurrentLayer);
    const bool currentAttrs = loadBool("UseCurrentAttributes", true);
    setUseCurrentAttributes(currentAttrs);
    const bool keepOriginals = loadBool("KeepOriginals", false);
    setKeepOriginals(keepOriginals);
}

bool LC_ActionModifyMirror::isInVisualSnapStatus(int status) {
    return (!m_mirrorToExistingLine) && ((status == SetAxisPoint1) || (status == SetAxisPoint2));
}

bool LC_ActionModifyMirror::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
        ctx.setActiveLayerAndPen(m_actionData->data.useCurrentLayer, m_actionData->data.useCurrentAttributes);
        return RS_Modification::mirror(m_actionData->data, m_selectedEntities, false, ctx);
    }

    void LC_ActionModifyMirror::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
        if (m_actionData->data.keepOriginals) {
            unselect(m_selectedEntities);
        }
        if (keepSelected) {
            select(ctx.entitiesToAdd);
        }
    }

void LC_ActionModifyMirror::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e){
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetAxisPoint1: {
            if (m_mirrorToExistingLine){
//                deleteSnapper();
                if (e->isShift){ // flip vertically
                    auto start = RS_Vector();
                    auto end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, true);
                    previewMirror(start, end);
                }
                else if (e->isControl) {// flip horizontally
                    auto start = RS_Vector();
                    auto end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, false);
                    previewMirror(start, end);
                }
                else {
                    RS_Entity *en = catchEntityByEvent(e, RS2::EntityLine, RS2::ResolveAll);
                    if (en != nullptr) {
                        const auto line = static_cast<RS_Line *>(en);
                        previewMirror(line->getStartpoint(), line->getEndpoint());
                    }
                }
            }
            else {
                trySnapToRelZeroCoordinateEvent(e);
            }
            break;
        }
        case SetAxisPoint2: {
            if (m_actionData->axisPoint1.valid){
                mouse = getSnapAngleAwarePoint(e, m_actionData->axisPoint1, mouse, true);
                previewMirror(m_actionData->axisPoint1, mouse);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyMirror::previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2) const {
    RS_MirrorData tmpData;
    tmpData.axisPoint1 = mirrorLinePoint1;
    tmpData.axisPoint2 = mirrorLinePoint2;

    LC_DocumentModificationBatch ctx;
    RS_Modification::mirror(tmpData, m_selectedEntities, true, ctx);
    previewEntitiesToAdd(ctx);

    previewLine(mirrorLinePoint1, mirrorLinePoint2);

    if (m_showRefEntitiesOnPreview) {
        previewRefLine(mirrorLinePoint1, mirrorLinePoint2);
        previewRefPoint(mirrorLinePoint1);
        previewRefSelectablePoint(mirrorLinePoint2);
    }

    if (isInfoCursorForModificationEnabled()){
        msg(tr("Mirror"))
            .wcsAngle(tr("Angle:"), mirrorLinePoint1.angleTo(mirrorLinePoint2))
            .wcsAngle(tr("Angle (alt):"), mirrorLinePoint2.angleTo(mirrorLinePoint1))
            .vector(tr("Line From:"), mirrorLinePoint1)
            .vector(tr("Line To:"), mirrorLinePoint2)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionModifyMirror::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    if (m_mirrorToExistingLine && status == SetAxisPoint1){
        if (e->isShift){ // flip vertically
            auto start = RS_Vector();
            auto end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, true);
            m_actionData->axisPoint1 = start;
            m_actionData->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else if (e->isControl) {// flip horizontally
            auto start = RS_Vector();
            auto end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, false);
            m_actionData->axisPoint1 = start;
            m_actionData->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else {
            RS_Entity *en = catchEntityByEvent(e, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr) {
                const auto line = static_cast<RS_Line *>(en);
                m_actionData->axisPoint1 = line->getStartpoint();
                m_actionData->axisPoint2 = line->getEndpoint();
                setStatus(ShowDialog);
                showOptionsAndTrigger();
            }
        }
        invalidateSnapSpot();
    }
    else {
        RS_Vector snapped = e->snapPoint;
        if (status == SetAxisPoint2){
            snapped = getSnapAngleAwarePoint(e, m_actionData->axisPoint1, snapped);
        }
        fireCoordinateEvent(snapped);
    }
}

void LC_ActionModifyMirror::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    if (status == SetAxisPoint1){
        if (m_selectionComplete) {
            m_selectionComplete = false;
        }
        else{
            initPrevious(status);
        }
    }
    else{
        initPrevious(status);
    }
}

void LC_ActionModifyMirror::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    if (!m_selectionComplete){
        return;
    }
    switch (status) {
        case SetAxisPoint1: {
            m_actionData->axisPoint1 = coord;
            setStatus(SetAxisPoint2);
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            break;
        }
        case SetAxisPoint2: {
            m_actionData->axisPoint2 = coord;
            setStatus(ShowDialog);
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            showOptionsAndTrigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyMirror::showOptionsAndTrigger(){
   doPerformTrigger();
}

void LC_ActionModifyMirror::doPerformTrigger() {
    m_actionData->data.axisPoint1 = m_actionData->axisPoint1;
    m_actionData->data.axisPoint2 = m_actionData->axisPoint2;
    deletePreview();
    trigger();
    finish();
}

void LC_ActionModifyMirror::updateMouseButtonHintsForSelection() {
    updatePromptTRCancel(tr("Select to mirror") + getSelectionCompletionHintMsg(), MOD_SHIFT_LC(tr("Select contour")));
}

void LC_ActionModifyMirror::updateMouseButtonHintsForSelected(const int status) {
    switch (status) {
        case SetAxisPoint1: {
            if (m_mirrorToExistingLine){
                updatePromptTRCancel(tr("Specify mirror line"), MOD_SHIFT_AND_CTRL(tr("Flip Vertically"), tr("Flip Horizontally")));
            }
            else{
                updatePromptTRCancel(tr("Specify first point of mirror line"), MOD_SHIFT_RELATIVE_ZERO);
            }
            break;
        }
        case SetAxisPoint2: {
            updatePromptTRBack(tr("Specify second point of mirror line"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default: {
            updatePrompt();
            break;
        }
    }
}

void LC_ActionModifyMirror::setMirrorToExistingLine(const bool value){
    m_mirrorToExistingLine = value;
    setStatus(SetAxisPoint1);
}

RS2::CursorType LC_ActionModifyMirror::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *LC_ActionModifyMirror::getModifyOperationFlags()  {
    return &m_actionData->data;
}

LC_ActionOptionsWidget* LC_ActionModifyMirror::createOptionsWidget(){
    return new LC_MirrorOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyMirror::createOptionsFiller() {
    return new LC_MirrorOptionsFiller();
}

void LC_ActionModifyMirror::obtainFlipLineCoordinates(RS_Vector *start, RS_Vector *end, const bool verticalLine) const {
    RS_Vector selectionMin;
    RS_Vector selectionMax;
    LC_Align::collectSelectionBounds(m_selectedEntities, selectionMin, selectionMax);

    if (verticalLine){
        const double x = (selectionMin.x + selectionMax.x) / 2;
        start->x = x;
        end->x = x;

        start->y = selectionMin.y;
        end->y = selectionMax.y;
    }
    else{
        const double y = (selectionMin.y + selectionMax.y) / 2;
        start->y = y;
        end->y = y;

        start->x = selectionMin.x;
        end->x = selectionMax.x;
    }
    start->valid = true;
    end->valid = true;
}
