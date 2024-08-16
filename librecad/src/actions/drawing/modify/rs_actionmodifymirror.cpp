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


void RS_ActionModifyMirror::trigger() {
    RS_DEBUG->print("RS_ActionModifyMirror::trigger()");

    RS_Modification m(*container, graphicView);
    m.mirror(pPoints->data, selectedEntities, false);

    updateSelectionWidget();
}

void RS_ActionModifyMirror::mouseMoveEventSelected(QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    deleteHighlights();
    switch (getStatus()) {
        case SetAxisPoint1: {
            if (mirrorToExistingLine){
                RS_Entity* en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
                if (en != nullptr){
                    auto line = dynamic_cast<RS_Line *>(en);
                    previewMirror(line->getStartpoint(), line->getEndpoint());
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
    drawHighlights();
    drawPreview();
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent end");
}

void RS_ActionModifyMirror::previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2){

    RS_Modification m(*preview, graphicView, false);
    RS_MirrorData tmpData;
    tmpData.axisPoint1 = mirrorLinePoint1;
    tmpData.axisPoint2 = mirrorLinePoint2;
    m.mirror(tmpData, selectedEntities, true);
    previewLine(mirrorLinePoint1, mirrorLinePoint2);

    if (showRefEntitiesOnPreview) {
        previewRefLine(mirrorLinePoint1, mirrorLinePoint2);
        previewRefPoint(mirrorLinePoint1);
        previewRefSelectablePoint(mirrorLinePoint2);
    }
}

void RS_ActionModifyMirror::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    if (mirrorToExistingLine && status == SetAxisPoint1){
        RS_Entity* en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
        if (en != nullptr){
            auto line = dynamic_cast<RS_Line *>(en);
            pPoints->axisPoint1 = line->getStartpoint();
            pPoints->axisPoint2 = line->getEndpoint();
            setStatus(ShowDialog);
            showOptionsAndTrigger();
        }
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
            doTrigger();
        } else {
            if (mirrorToExistingLine) {
                setStatus(SetAxisPoint1);
            } else {
                setStatus(SetAxisPoint2);
            }
        }
    }
    else{
        doTrigger();
    }
}

void RS_ActionModifyMirror::doTrigger() {
    pPoints->data.axisPoint1 = pPoints->axisPoint1;
    pPoints->data.axisPoint2 = pPoints->axisPoint2;
    deletePreview();
    trigger();
    finish(false);
}

void RS_ActionModifyMirror::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to mirror"));
}

void RS_ActionModifyMirror::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetAxisPoint1: {
            if (mirrorToExistingLine){
                updateMouseWidgetTRCancel(tr("Specify mirror line"));
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

LC_ModifyOperationFlags *RS_ActionModifyMirror::getModifyOperationFlags() {
    return &pPoints->data;
}

LC_ActionOptionsWidget* RS_ActionModifyMirror::createOptionsWidget(){
    return new LC_ModifyMirrorOptions();
}
