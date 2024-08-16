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
#include "rs_actionmodifymove.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "lc_moveoptions.h"

struct RS_ActionModifyMove::Points {
	RS_MoveData data;
	RS_Vector referencePoint;
	RS_Vector targetPoint;
};

RS_ActionModifyMove::RS_ActionModifyMove(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :LC_ActionModifyBase("Move Entities",container, graphicView)
        ,pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionModifyMove;
}

RS_ActionModifyMove::~RS_ActionModifyMove() = default;

void RS_ActionModifyMove::trigger(){

    RS_DEBUG->print("RS_ActionModifyMove::trigger()");

    RS_Modification m(*container, graphicView);
    m.move(pPoints->data, selectedEntities, false, false);

    updateSelectionWidget();
    finish(false);
}

void RS_ActionModifyMove::mouseMoveEventSelected(QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionModifyMove::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetReferencePoint: {
            pPoints->referencePoint = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTargetPoint: {
            if (pPoints->referencePoint.valid){
                deletePreview();

                mouse = getSnapAngleAwarePoint(e, pPoints->referencePoint, mouse, true);
                pPoints->targetPoint = mouse;

                const RS_Vector &offset = pPoints->targetPoint - pPoints->referencePoint;
                pPoints->data.offset = offset;

                RS_Modification m(*preview, graphicView, false);
                m.move(pPoints->data, selectedEntities, true, false);

                if (isShift(e)){
                    previewLine(pPoints->referencePoint, mouse);
                }
                if (showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewRefPoint(pPoints->referencePoint);
                    previewRefLine(pPoints->referencePoint, mouse);

                    if (pPoints->data.multipleCopies) {
                        int numCopies = pPoints->data.number;
                        if (numCopies > 1) {
                            for (int i = 2; i <= numCopies; i++) {
                                previewRefPoint(pPoints->referencePoint + offset * i);
                            }
                        }
                    }
                }
                drawPreview();
            }
            break;
        }
        case ShowDialog:
            break;
        default:
            break;
    }    

    RS_DEBUG->print("RS_ActionModifyMove::mouseMoveEvent end");
}

void RS_ActionModifyMove::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    if (status == SetTargetPoint){
        snapped = getSnapAngleAwarePoint(e, pPoints->referencePoint, snapped);
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionModifyMove::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    if (status == SetReferencePoint){
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

void RS_ActionModifyMove::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    if (!selectionComplete){
        return;
    }
    switch (status) {
        case SetReferencePoint: {
            pPoints->referencePoint = pos;
            moveRelativeZero(pPoints->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            pPoints->targetPoint = pos;
            if (isShowModifyActionDialog()){
                setStatus(ShowDialog); // todo - hm... what for?
                if (RS_DIALOGFACTORY->requestMoveDialog(pPoints->data)){
                    updateOptions();
                    pPoints->data.offset = pPoints->targetPoint - pPoints->referencePoint;
                    trigger();
                    moveRelativeZero(pPoints->targetPoint);
                }
                else{
                    setStatus(SetTargetPoint);
                }
            }
            else{
                pPoints->data.offset = pPoints->targetPoint - pPoints->referencePoint;
                trigger();
                moveRelativeZero(pPoints->targetPoint);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyMove::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyMove::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to move"), MOD_CTRL(tr("Move immediately after selection")));
}

RS2::CursorType RS_ActionModifyMove::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionModifyMove::createOptionsWidget() {
    return new LC_MoveOptions();
}

LC_ModifyOperationFlags *RS_ActionModifyMove::getModifyOperationFlags() {
    return &pPoints->data;
}
