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

#include "rs_actionmodifymirror.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_align.h"
#include "lc_modifymirroroptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"


struct RS_ActionModifyMirror::MirrorActionData {
    RS_MirrorData data;
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};

RS_ActionModifyMirror::RS_ActionModifyMirror(LC_ActionContext *actionContext)
        :LC_ActionModifyBase("Mirror Entities", actionContext, RS2::ActionModifyMirror)
        , m_actionData(std::make_unique<MirrorActionData>()){
    m_mirrorToExistingLine = false;
}

RS_ActionModifyMirror::~RS_ActionModifyMirror() = default;

void RS_ActionModifyMirror::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyMirror::trigger()");
    RS_Modification m(*m_container, m_viewport);
    m.mirror(m_actionData->data, m_selectedEntities, false, keepSelected);
}

void RS_ActionModifyMirror::onMouseMoveEventSelected(int status, LC_MouseEvent *e){
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetAxisPoint1: {
            if (m_mirrorToExistingLine){
//                deleteSnapper();
                if (e->isShift){ // flip vertically
                    RS_Vector start = RS_Vector();
                    RS_Vector end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, true);
                    previewMirror(start, end);
                }
                else if (e->isControl) {// flip horizontally
                    RS_Vector start = RS_Vector();
                    RS_Vector end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, false);
                    previewMirror(start, end);
                }
                else {
                    RS_Entity *en = catchEntityByEvent(e, RS2::EntityLine, RS2::ResolveAll);
                    if (en != nullptr) {
                        auto line = dynamic_cast<RS_Line *>(en);
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

void RS_ActionModifyMirror::previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2){
    RS_MirrorData tmpData;
    tmpData.axisPoint1 = mirrorLinePoint1;
    tmpData.axisPoint2 = mirrorLinePoint2;

    RS_Modification m(*m_preview, m_viewport, false);
    m.mirror(tmpData, m_selectedEntities, true, false);
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

void RS_ActionModifyMirror::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    if (m_mirrorToExistingLine && status == SetAxisPoint1){
        if (e->isShift){ // flip vertically
            RS_Vector start = RS_Vector();
            RS_Vector end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, true);
            m_actionData->axisPoint1 = start;
            m_actionData->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else if (e->isControl) {// flip horizontally
            RS_Vector start = RS_Vector();
            RS_Vector end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, false);
            m_actionData->axisPoint1 = start;
            m_actionData->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else {
            RS_Entity *en = catchEntityByEvent(e, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr) {
                auto line = dynamic_cast<RS_Line *>(en);
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

void RS_ActionModifyMirror::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
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

void RS_ActionModifyMirror::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    if (!m_selectionComplete){
        return;
    }
    switch (status) {
        case SetAxisPoint1: {
            m_actionData->axisPoint1 = mouse;
            setStatus(SetAxisPoint2);
            moveRelativeZero(mouse);
            break;
        }
        case SetAxisPoint2: {
            m_actionData->axisPoint2 = mouse;
            setStatus(ShowDialog);
            moveRelativeZero(mouse);
            showOptionsAndTrigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMirror::showOptionsAndTrigger(){
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestMirrorDialog(m_actionData->data)) {
            updateOptions();
            doPerformTrigger();
        } else {
            if (m_mirrorToExistingLine) {
                setStatus(SetAxisPoint1);
            } else {
                setStatus(SetAxisPoint2);
            }
        }
    }
    else{
        doPerformTrigger();
    }
}

void RS_ActionModifyMirror::doPerformTrigger() {
    m_actionData->data.axisPoint1 = m_actionData->axisPoint1;
    m_actionData->data.axisPoint2 = m_actionData->axisPoint2;
    deletePreview();
    trigger();
    finish(false);
}

void RS_ActionModifyMirror::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to mirror (Enter to complete)"), MOD_SHIFT_LC(tr("Select contour")));
}

void RS_ActionModifyMirror::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetAxisPoint1: {
            if (m_mirrorToExistingLine){
                updateMouseWidgetTRCancel(tr("Specify mirror line"), MOD_SHIFT_AND_CTRL(tr("Flip Vertically"), tr("Flip Horizontally")));
            }
            else{
                updateMouseWidgetTRCancel(tr("Specify first point of mirror line"), MOD_SHIFT_RELATIVE_ZERO);
            }
            break;
        }
        case SetAxisPoint2: {
            updateMouseWidgetTRBack(tr("Specify second point of mirror line"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default: {
            updateMouseWidget();
            break;
        }
    }
}

void RS_ActionModifyMirror::setMirrorToExistingLine(bool value){
    m_mirrorToExistingLine = value;
    setStatus(SetAxisPoint1);
}

RS2::CursorType RS_ActionModifyMirror::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *RS_ActionModifyMirror::getModifyOperationFlags()  {
    return &m_actionData->data;
}

LC_ActionOptionsWidget* RS_ActionModifyMirror::createOptionsWidget(){
    return new LC_ModifyMirrorOptions();
}

void RS_ActionModifyMirror::obtainFlipLineCoordinates(RS_Vector *start, RS_Vector *end, bool verticalLine) {
    RS_Vector selectionMin;
    RS_Vector selectionMax;
    LC_Align::collectSelectionBounds(m_selectedEntities, selectionMin, selectionMax);

    if (verticalLine){
        double x = (selectionMin.x + selectionMax.x) / 2;
        start->x = x;
        end->x = x;

        start->y = selectionMin.y;
        end->y = selectionMax.y;
    }
    else{
        double y = (selectionMin.y + selectionMax.y) / 2;
        start->y = y;
        end->y = y;

        start->x = selectionMin.x;
        end->x = selectionMax.x;
    }
    start->valid = true;
    end->valid = true;
}
