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

#include <QMouseEvent>

#include "rs_actionmodifymirror.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "lc_modifymirroroptions.h"
#include "lc_align.h"

struct RS_ActionModifyMirror::Points {
    RS_MirrorData data;
    RS_Vector axisPoint1;
    RS_Vector axisPoint2;
};

RS_ActionModifyMirror::RS_ActionModifyMirror(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :LC_ActionModifyBase("Mirror Entities",container, graphicView)
        , pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionModifyMirror;
    mirrorToExistingLine = false;
}
RS_ActionModifyMirror::~RS_ActionModifyMirror() = default;


void RS_ActionModifyMirror::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyMirror::trigger()");
    RS_Modification m(*container, graphicView);
    m.mirror(pPoints->data, selectedEntities, false, keepSelected);
}

void RS_ActionModifyMirror::mouseMoveEventSelected(QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    deleteHighlights();
    switch (getStatus()) {
        case SetAxisPoint1: {
            if (mirrorToExistingLine){
//                deleteSnapper();
                if (isShift(e)){ // flip vertically
                    RS_Vector start = RS_Vector();
                    RS_Vector end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, true);
                    previewMirror(start, end);
                }
                else if (isControl(e)) {// flip horizontally
                    RS_Vector start = RS_Vector();
                    RS_Vector end = RS_Vector();
                    obtainFlipLineCoordinates(&start, &end, false);
                    previewMirror(start, end);
                }
                else {
                    RS_Entity *en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
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
            if (pPoints->axisPoint1.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->axisPoint1, mouse, true);
                previewMirror(pPoints->axisPoint1, mouse);
            }
            break;
        }
        default:
            break;
    }
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent end");
    drawHighlights();
    drawPreview();
}

void RS_ActionModifyMirror::previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2){
    RS_MirrorData tmpData;
    tmpData.axisPoint1 = mirrorLinePoint1;
    tmpData.axisPoint2 = mirrorLinePoint2;

    RS_Modification m(*preview, graphicView, false);
    m.mirror(tmpData, selectedEntities, true, false);
    previewLine(mirrorLinePoint1, mirrorLinePoint2);

    if (showRefEntitiesOnPreview) {
        previewRefLine(mirrorLinePoint1, mirrorLinePoint2);
        previewRefPoint(mirrorLinePoint1);
        previewRefSelectablePoint(mirrorLinePoint2);
    }

    if (isInfoCursorForModificationEnabled()){
        LC_InfoMessageBuilder msg(tr("Mirror"));
        msg.add(tr("Angle:"),formatAngle(mirrorLinePoint1.angleTo(mirrorLinePoint2)));
        msg.add(tr("Angle (alt):"),formatAngle(mirrorLinePoint2.angleTo(mirrorLinePoint1)));
        msg.add(tr("Line From:"), formatVector(mirrorLinePoint1));
        msg.add(tr("Line To:"),formatVector(mirrorLinePoint2));
        appendInfoCursorZoneMessage(msg.toString(), 2, false);
    }
}

void RS_ActionModifyMirror::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    if (mirrorToExistingLine && status == SetAxisPoint1){
        if (isShift(e)){ // flip vertically
            RS_Vector start = RS_Vector();
            RS_Vector end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, true);
            pPoints->axisPoint1 = start;
            pPoints->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else if (isControl(e)) {// flip horizontally
            RS_Vector start = RS_Vector();
            RS_Vector end = RS_Vector();
            obtainFlipLineCoordinates(&start, &end, false);
            pPoints->axisPoint1 = start;
            pPoints->axisPoint2 = end;
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
        else {
            RS_Entity *en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr) {
                auto line = dynamic_cast<RS_Line *>(en);
                pPoints->axisPoint1 = line->getStartpoint();
                pPoints->axisPoint2 = line->getEndpoint();
                setStatus(ShowDialog);
                showOptionsAndTrigger();
            }
        }
        invalidateSnapSpot();
    }
    else {
        RS_Vector snapped = snapPoint(e);
        if (status == SetAxisPoint2){
            snapped = getSnapAngleAwarePoint(e, pPoints->axisPoint1, snapped);
        }
        fireCoordinateEvent(snapped);
    }
}

void RS_ActionModifyMirror::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    if (status == SetAxisPoint1){
        if (selectionComplete) {
            selectionComplete = false;
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
    if (!selectionComplete){
        return;
    }
    switch (status) {
        case SetAxisPoint1: {
            pPoints->axisPoint1 = mouse;
            setStatus(SetAxisPoint2);
            moveRelativeZero(mouse);
            break;
        }
        case SetAxisPoint2: {
            pPoints->axisPoint2 = mouse;
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
        if (RS_DIALOGFACTORY->requestMirrorDialog(pPoints->data)) {
            updateOptions();
            doPerformTrigger();
        } else {
            if (mirrorToExistingLine) {
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
    pPoints->data.axisPoint1 = pPoints->axisPoint1;
    pPoints->data.axisPoint2 = pPoints->axisPoint2;
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
            if (mirrorToExistingLine){
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
    mirrorToExistingLine = value;
    setStatus(SetAxisPoint1);
}

RS2::CursorType RS_ActionModifyMirror::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *RS_ActionModifyMirror::getModifyOperationFlags()  {
    return &pPoints->data;
}

LC_ActionOptionsWidget* RS_ActionModifyMirror::createOptionsWidget(){
    return new LC_ModifyMirrorOptions();
}

void RS_ActionModifyMirror::obtainFlipLineCoordinates(RS_Vector *start, RS_Vector *end, bool verticalLine) {
    RS_Vector selectionMin;
    RS_Vector selectionMax;
    LC_Align::collectSelectionBounds(selectedEntities, selectionMin, selectionMax);

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
