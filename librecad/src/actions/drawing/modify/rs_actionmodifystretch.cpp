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

#include "rs_actionmodifystretch.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "lc_modifystretchoptions.h"

struct RS_ActionModifyStretch::Points {
    RS_Vector firstCorner;
    RS_Vector secondCorner;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
};

RS_ActionModifyStretch::RS_ActionModifyStretch(RS_EntityContainer& container,
											   RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Stretch Entities",
							   container, graphicView)
	, pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionModifyStretch;
}

void RS_ActionModifyStretch::init(int status) {
    RS_PreviewActionInterface::init(status);
}

RS_ActionModifyStretch::~RS_ActionModifyStretch() = default;

void RS_ActionModifyStretch::trigger(){

    RS_DEBUG->print("RS_ActionModifyStretch::trigger()");

    deletePreview();

    RS_Modification m(*container, graphicView);
    m.stretch(pPoints->firstCorner,
              pPoints->secondCorner,
              pPoints->targetPoint - pPoints->referencePoint, removeOriginals);
    if (removeOriginals) {
        setStatus(SetFirstCorner);
    }
    else{
        setStatus(SetTargetPoint);
    }

    updateSelectionWidget();
}

void RS_ActionModifyStretch::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetFirstCorner:{
            break;
        }
        case SetSecondCorner: {
            if (pPoints->firstCorner.valid) {
                pPoints->secondCorner = snapPoint(e);
                deletePreview();
                previewStretchRect(false);
                drawPreview();
            }
            break;
        }
        case SetReferencePoint: {
            deletePreview();
            previewStretchRect(true);
            trySnapToRelZeroCoordinateEvent(e);
            drawPreview();
            break;
        }
        case SetTargetPoint: {
            if (pPoints->referencePoint.valid) {
                deletePreview();
                mouse= getSnapAngleAwarePoint(e, pPoints->referencePoint, mouse, true);
                pPoints->targetPoint = mouse;
                // fixme - isn't it more reliable to rely on RS_Modification::stretch there?
                preview->addStretchablesFrom(*container, pPoints->firstCorner, pPoints->secondCorner);
                //preview->move(targetPoint-referencePoint);
                preview->stretch(pPoints->firstCorner, pPoints->secondCorner,
                                 pPoints->targetPoint - pPoints->referencePoint);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->referencePoint);
                    previewRefSelectablePoint(pPoints->targetPoint);
                    previewRefLine(pPoints->referencePoint, pPoints->targetPoint);
                }
                drawPreview();
            }
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionModifyStretch::mouseMoveEvent end");
}

void RS_ActionModifyStretch::previewStretchRect(bool selected) {
    if (showRefEntitiesOnPreview){
        RS_Vector v0 = pPoints->firstCorner;
        RS_Vector v1 = pPoints->secondCorner;
        previewRefLine(v0, {v1.x, v0.y});
        previewRefLine({v1.x, v0.y}, v1);
        previewRefLine(v1, {v0.x, v1.y});
        previewRefLine({v0.x, v1.y}, v0);
        previewRefPoint(v0);
        if (selected){
            previewRefPoint(v1);
        }
        else{
            previewRefSelectablePoint(v1);
        }
    }
    else{
        preview->addRectangle(pPoints->firstCorner, pPoints->secondCorner);
    }
}

void RS_ActionModifyStretch::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    if (status == SetTargetPoint){
        RS_Vector mouse= getSnapAngleAwarePoint(e, pPoints->referencePoint, snapPoint(e));
        fireCoordinateEvent(mouse);
    }
    else {
        fireCoordinateEventForSnap(e);
    }
}
// fixme - default back - remove as well as from other actions and rely to parent class
void RS_ActionModifyStretch::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionModifyStretch::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetFirstCorner: {
            pPoints->firstCorner = mouse;
            setStatus(SetSecondCorner);
            break;
        }
        case SetSecondCorner: {
            pPoints->secondCorner = mouse;
            deletePreview();
            setStatus(SetReferencePoint);
            break;
        }
        case SetReferencePoint: {
            pPoints->referencePoint = mouse;
            moveRelativeZero(pPoints->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            pPoints->targetPoint = mouse;
            moveRelativeZero(pPoints->targetPoint);
            trigger();
            //finish();
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyStretch::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetFirstCorner:
            updateMouseWidgetTRCancel(tr("Specify first corner"));
            break;
        case SetSecondCorner:
            updateMouseWidgetTRBack(tr("Specify second corner"));
            break;
        case SetReferencePoint:
            updateMouseWidgetTRBack(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack(tr("Specify target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyStretch::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* RS_ActionModifyStretch::createOptionsWidget() {
    return new LC_ModifyStretchOptions();
}
