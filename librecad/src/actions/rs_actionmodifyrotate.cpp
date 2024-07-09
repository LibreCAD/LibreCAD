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
#include <QApplication>

#include "rs_actionmodifyrotate.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_arc.h"
#include "rs_previewactioninterface.h"


// fixme - options for point selection mode
// fixme - get rid of dialog?
// fixme - options widget
// fixme - coordinates widget with reference point support?

RS_ActionModifyRotate::RS_ActionModifyRotate(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Rotate Entities",
							   container, graphicView)
	,data(new RS_RotateData())
{
	actionType=RS2::ActionModifyRotate;
}

RS_ActionModifyRotate::~RS_ActionModifyRotate() = default;

void RS_ActionModifyRotate::init(int status) {
    RS_ActionInterface::init(status);
    if (status == SelectEntity){
        selectedCount = document->countSelected();
        finishSelection(false);
    }
}

void RS_ActionModifyRotate::finishSelection(bool referenceFirst){
    if (selectedCount > 0){
        if (referenceFirst){
            setStatus(SetReferencePoint);
        }
        else if (selectRefPointFirst){
            setStatus(SetReferencePoint);
        }
        else{
            setStatus(SetCenterPoint);
        }
    }
}

void RS_ActionModifyRotate::trigger(){

    RS_DEBUG->print("RS_ActionModifyRotate::trigger()");

    RS_Modification m(*container, graphicView);
    m.rotate(*data);

    updateSelectionWidget();
}

void RS_ActionModifyRotate::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    switch (getStatus()) {
        case SelectEntity: {
            auto en = catchEntity(e);
            deleteHighlights();
            if (en != nullptr){
                highlightHover(en);
                drawHighlights();
            }
            break;
        }
        case SetReferencePoint: {
            if (selectRefPointFirst){
                deletePreview();
                previewRefPoint(mouse);
                drawPreview();
            } else {
                preview->addSelectionFrom(*container);
                RS_Vector center = (preview->getMin() + preview->getMax()) / 2;
                previewRefLine(data->center, mouse);
                previewRefLine(data->center, center);
                previewRefPoint(center);
                deletePreview();
                drawPreview();
            }
            break;
        }
        case SetCenterPoint: {
            if (selectRefPointFirst){
                previewRefLine(originalReferencePoint, mouse);
                previewRefPoint(originalReferencePoint);
                drawPreview();
            } else {
                trySnapToRelZeroCoordinateEvent(e);
                preview->addSelectionFrom(*container);
                RS_Vector center = (preview->getMin() + preview->getMax()) / 2;
                deletePreview();
                previewRefLine(center, mouse);
                drawPreview();
            }
            break;
        }
        case SetTargetPoint:
            if (!mouse.valid) return;
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);
            preview->addSelectionFrom(*container);
            double rotationAngle = RS_Math::correctAngle((mouse - data->center).angle() - data->angle);
            preview->rotate(data->center, rotationAngle);

            RS_Vector newReferencePoint = originalReferencePoint;
            newReferencePoint.rotate(data->center, rotationAngle);
            previewRefPoint(originalReferencePoint);
            previewRefSelectablePoint(newReferencePoint);
            previewRefPoint(data->center);
            previewRefLine(originalReferencePoint, data->center);
            previewRefLine(mouse, data->center);

            if (originalReferencePoint.valid){
                //previewRefArc(data->center, originalReferencePoint, mouse, true);
                double radius = data->center.distanceTo((originalReferencePoint));
                // fixme - temporarily add copy of circle to the document. If it is there, intersection snap for target reference point will work.
                previewRefCircle(data->center, radius);
            }
            drawPreview();
    }

    RS_DEBUG->print("RS_ActionModifyRotate::mouseMoveEvent end");
}

void RS_ActionModifyRotate::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr){
        return;
    }

    RS_Vector pos = e->getCoordinate();
    if (!pos.valid){
        return;
    }
    switch (getStatus()) {
        case SetReferencePoint: {
            if (selectRefPointFirst){
                originalReferencePoint = e->getCoordinate();
                moveRelativeZero(originalReferencePoint);
                setStatus(SetCenterPoint);
            }
            else{
                pos -= data->center;
                originalReferencePoint = e->getCoordinate();
                deletePreview();
                if (pos.squared() < RS_TOLERANCE2){
                    data->angle = 0.;//angle not well defined, go direct to dialog
                    setStatus(ShowDialog);
                    if (RS_DIALOGFACTORY->requestRotateDialog(*data)){
                        trigger();
                        finish(false);
                    }
                } else {
                    data->angle = pos.angle();
                    setStatus(SetTargetPoint);
                }
                break;
            }
            break;
        }
        case SetCenterPoint:{
            if (selectRefPointFirst){
                RS_Vector radius = pos - data->center;
                if (radius.squared() > RS_TOLERANCE2){
                    data->center = pos;
                    data->angle = (originalReferencePoint - pos).angle();
                    moveRelativeZero(data->center);
                    setStatus(SetTargetPoint);
                } else {
                   commandMessageTR("Rotation center is too close to reference point.");
                }
                break;
            }
            else{
                data->center = pos;
                moveRelativeZero(data->center);
                setStatus(SetReferencePoint);
                break;
            }
        }
        case SetTargetPoint:

            pos -= data->center;
            if (pos.squared() < RS_TOLERANCE2){
                data->angle = 0.;//angle not well defined
            } else {
                data->angle = RS_Math::correctAngle(pos.angle() - data->angle);
            }
            setStatus(ShowDialog);
            if (RS_DIALOGFACTORY->requestRotateDialog(*data)){
                trigger();
                finish(false);
            }
            break;

        default:
            break;
    }
}

void RS_ActionModifyRotate::mouseReleaseEvent(QMouseEvent *e){
    int status = getStatus();
    if (e->button() == Qt::LeftButton){
        RS_Vector snap = snapPoint(e);
        if (status == SelectEntity){
            auto en = catchEntity(e);
            if (en != nullptr){
                bool selectedOld = en->isSelected();
                bool selected = !selectedOld;
                en->setSelected(selected);
                graphicView->drawEntity(en);
                if (selected){
                    selectedCount++;
                }
                else{
                    selectedCount--;
                }
            }
        }
        else if (status == SetTargetPoint){
            snap = getSnapAngleAwarePoint(e, data->center, snap);
        }
        fireCoordinateEvent(snap);
    } else if (e->button() == Qt::RightButton){
        deletePreview();
        int newStatus = -1;
        switch (status)
        {
            case SelectEntity:
                newStatus = -1;
                break;
            case SetReferencePoint:
                newStatus = selectRefPointFirst ? SelectEntity: SetCenterPoint;
                break;
            case SetCenterPoint:
                newStatus = selectRefPointFirst ? SetReferencePoint: SelectEntity;
                break;
            case SetTargetPoint:
                newStatus = selectRefPointFirst ? SetCenterPoint: SetReferencePoint;
                break;
            default:
                break;
        }
        setStatus(newStatus);
    }
}

void RS_ActionModifyRotate::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionModifyRotate::keyPressEvent(QKeyEvent* e)
{
    if (getStatus() == SelectEntity){
        bool shiftPressed = QApplication::keyboardModifiers()/*e->modifiers()*/ & Qt::ShiftModifier;
        if (e->key() == Qt::Key_Enter){
            finishSelection(shiftPressed);
        } else {
//        e->ignore();
        }
    }
}

void RS_ActionModifyRotate::keyReleaseEvent(QKeyEvent *e){
    bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
    if (e->key()==Qt::Key_Enter){
        finishSelection(shiftPressed);
    }
}

void RS_ActionModifyRotate::updateMouseButtonHints(){
    switch (getStatus()) {
        case SelectEntity:
            updateMouseWidgetTRCancel("Select to rotate");
            break;
        case SetCenterPoint:
            updateMouseWidgetTRBack("Specify rotation center", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetReferencePoint:
            updateMouseWidgetTRBack("Specify reference point");
            break;
        case SetTargetPoint:
            updateMouseWidgetTRBack("Specify target point to rotate to", MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyRotate::updateMouseCursor(){
    if (graphicView){
        if (getStatus() == SelectEntity){
            setMouseCursor(RS2::SelectCursor);
        } else {
            setMouseCursor(RS2::CadCursor);
        }
    }
}

// EOF
