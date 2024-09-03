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

#include "rs_actionmodifyrotate2.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "lc_rotate2options.h"

RS_ActionModifyRotate2::RS_ActionModifyRotate2(RS_EntityContainer& container, RS_GraphicView& graphicView)
    :LC_ActionModifyBase("Rotate Entities around two centers",container, graphicView)
    ,data(new RS_Rotate2Data()){
    actionType=RS2::ActionModifyRotate2;
}

// fixme - The logic of Rotate2 action should be deeply reviewed... this is very old implementation, and for
// non-mirrored angles it delivery really uncontrolled and messy results.. So the generic purpose of this action
// is cumbersome. While it may by used for creation of entities moved over the circle and spiral, the overall
// practical use of the action seems to be not obvious...

RS_ActionModifyRotate2::~RS_ActionModifyRotate2() = default;

void RS_ActionModifyRotate2::init(int status) {
    LC_ActionModifyBase::init(status);
}

void RS_ActionModifyRotate2::trigger(){
    RS_DEBUG->print("RS_ActionModifyRotate2::trigger()");

    RS_Modification m(*container, graphicView);
    m.rotate2(*data, selectedEntities,false);

    finish(false);

    updateSelectionWidget();
}

void RS_ActionModifyRotate2::mouseMoveEventSelected(QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionModifyRotate2::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    deletePreview();
    switch (status) {
        case SetReferencePoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetReferencePoint2: {
            if (data->center1.valid){
                mouse = getSnapAngleAwarePoint(e, data->center1, mouse, true);
                data->center2 = mouse;
                RS_Modification m(*preview, graphicView, false);
                m.rotate2(*data, selectedEntities, true);

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(data->center1);
                    previewRefLine(data->center1, mouse);
                    previewRefPointsForMultipleCopies(mouse);
                }
            }
            break;
        }
        default:
            break;
    }
    drawPreview();

    RS_DEBUG->print("RS_ActionModifyRotate2::mouseMoveEvent end");
}

void RS_ActionModifyRotate2::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status){
        case SetReferencePoint2: {
            snap = getSnapAngleAwarePoint(e, data->center1, snap, false);
            break;
        }
    }
    fireCoordinateEvent(snap);
}

void RS_ActionModifyRotate2::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    switch (status) {
        case SetReferencePoint2: {
            setStatus(SetReferencePoint1);
            break;
        }
        case SetReferencePoint1: {
            selectionComplete = false;
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate2::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetReferencePoint1: {
            data->center1 = pos;
            moveRelativeZero(pos);
            setStatus(SetReferencePoint2);
            break;
        }
        case SetReferencePoint2: {
            data->center2 = pos;
//            setStatus(ShowDialog);
            doTrigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRotate2::doTrigger() {
    if (isShowModifyActionDialog()) {
        if (RS_DIALOGFACTORY->requestRotate2Dialog(*data)) {
            updateOptions();
            trigger();
        }
        else{
            setStatus(SetReferencePoint2);
        }
    }
    else{
        trigger();
    }
}

void RS_ActionModifyRotate2::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select for two axis rotation"), MOD_CTRL(tr("Rotate 2 Axis immediately after selection")));
}

void RS_ActionModifyRotate2::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint1:
            updateMouseWidgetTRCancel(tr("Specify absolute reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetReferencePoint2:
            updateMouseWidgetTRBack(tr("Specify relative reference point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyRotate2::doGetMouseCursorSelected([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void RS_ActionModifyRotate2::previewRefPointsForMultipleCopies( [[maybe_unused]]const RS_Vector &mouse) {
    int numPoints = data->number;
    if (!data->multipleCopies) {
        numPoints = 1;
    }
    for (int i = 1; i <= numPoints; i++) {
        double angle1ForCopy = /*data->sameAngle1ForCopies ?  data->angle1 :*/ data->angle1 * i;
        double angle2ForCopy = data->sameAngle2ForCopies ? data->angle2 : data->angle2 * i;
        RS_Vector center2 = data->center2;
        center2.rotate(data->center1, angle1ForCopy);

        double angleSum = angle1ForCopy + angle2ForCopy;
        previewSnapAngleMark(center2, angleSum);
        if (i == 1) {
            previewRefLine(data->center1, center2);
            previewRefPoint(center2);
        }
    }
}

LC_ActionOptionsWidget *RS_ActionModifyRotate2::createOptionsWidget() {
    return new LC_Rotate2Options();
}

void RS_ActionModifyRotate2::setAngle2(double d) {
    data->angle2 = d;
}

void RS_ActionModifyRotate2::setAngle1(double d) {
    data->angle1 = d;
}

double RS_ActionModifyRotate2::getAngle1() {
    return data->angle1;
}

double RS_ActionModifyRotate2::getAngle2() {
    return data->angle2;
}

void RS_ActionModifyRotate2::setUseSameAngle2ForCopies(bool b) {
    data->sameAngle2ForCopies = b;
}

bool RS_ActionModifyRotate2::isUseSameAngle2ForCopies() {
    return data->sameAngle2ForCopies;
}

void RS_ActionModifyRotate2::setMirrorAngles(bool b) {
    data->mirrorAngles = b;
}

bool RS_ActionModifyRotate2::isMirrorAngles() {
    return data->mirrorAngles;
}

LC_ModifyOperationFlags *RS_ActionModifyRotate2::getModifyOperationFlags() {
    return data.get();
}

bool RS_ActionModifyRotate2::isAllowTriggerOnEmptySelection() {
    return false;
}
