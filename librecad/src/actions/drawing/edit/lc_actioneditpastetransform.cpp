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

#include "lc_actioneditpastetransform.h"
#include "rs_modification.h"
#include "lc_pastetransformoptions.h"
#include "rs_math.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_clipboard.h"
#include "rs_units.h"

LC_ActionEditPasteTransform::LC_ActionEditPasteTransform(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("PasteTransform",container, graphicView),
    referencePoint{new RS_Vector(false)},
    data{new PasteData()}{
    actionType = RS2::ActionEditPasteTransform;
}

void LC_ActionEditPasteTransform::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (RS_CLIPBOARD->count() == 0){
        commandMessage(tr("Clipboard is empty"));
        finish(false);
    }
}

void LC_ActionEditPasteTransform::trigger() {
    deletePreview();

    RS_Modification m(*container, graphicView, false);

    int numX = data->arrayXCount;
    int numY = data->arrayYCount;


    RS_Vector xArrayVector;
    RS_Vector yArrayVector;
    if (data->arrayCreated){
        double arrayAngle = data->arrayAngle;
        xArrayVector = RS_Vector::polar(data->arraySpacing.x, arrayAngle);
        yArrayVector = RS_Vector::polar(data->arraySpacing.y, arrayAngle + M_PI_2);
    }
    else{
        numX = 1;
        numY = 1;
    }

    document->startUndoCycle();

    for (int x = 0; x < numX; x++){
        for (int y = 0; y < numY; y++){
            RS_Vector currentPoint = *referencePoint + xArrayVector*x + yArrayVector * y;
            const RS_PasteData &pasteData = RS_PasteData(currentPoint, data->factor , data->angle,
                                                         false, "");
            m.paste(pasteData);
        }
    }

    document->endUndoCycle();


    graphicView->redraw(RS2::RedrawDrawing);
    if (!invokedWithControl) {
        finish(false);
    }
}

void LC_ActionEditPasteTransform::mouseMoveEvent(QMouseEvent *e) {
    if (getStatus()==SetReferencePoint) {
        *referencePoint = snapPoint(e);
        deletePreview();
        preview->addAllFrom(*RS_CLIPBOARD->getGraphic());
        preview->move(*referencePoint);

        if (graphic) {
            RS2::Unit sourceUnit = RS_CLIPBOARD->getGraphic()->getUnit();
            RS2::Unit targetUnit = graphic->getUnit();
            double const f = RS_Units::convert(data->factor, sourceUnit, targetUnit);
            preview->scale(*referencePoint, {f, f});
            preview->rotate(*referencePoint, data->angle);

            if (showRefEntitiesOnPreview) {
                previewMultipleReferencePoints();
            }
        }
        drawPreview();
    }
    else
        deleteSnapper();
}

void LC_ActionEditPasteTransform::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    invokedWithControl = isControl(e);
    fireCoordinateEventForSnap(e);
}

void LC_ActionEditPasteTransform::onMouseRightButtonRelease(int status,[[maybe_unused]] QMouseEvent *e) {
    initPrevious(status);
}

void LC_ActionEditPasteTransform::onCoordinateEvent([[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    *referencePoint = pos;
    trigger();
}

RS2::CursorType LC_ActionEditPasteTransform::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionEditPasteTransform::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr("Set paste reference point"), MOD_CTRL(tr("Paste Multiple")));
            break;

        default:
            updateMouseWidget();
            break;
    }
}

double LC_ActionEditPasteTransform::getAngle() const {return data-> angle;}
void LC_ActionEditPasteTransform::setAngle(double angle) {data->angle = angle;}
double LC_ActionEditPasteTransform::getFactor() const {return data->factor;}
void LC_ActionEditPasteTransform::setFactor(double factor) {data->factor = factor;}
bool LC_ActionEditPasteTransform::isArrayCreated() const {return data->arrayCreated;}
void LC_ActionEditPasteTransform::setArrayCreated(bool arrayCreated) {data->arrayCreated = arrayCreated;}
int LC_ActionEditPasteTransform::getArrayXCount() const {return data->arrayXCount;}
void LC_ActionEditPasteTransform::setArrayXCount(int arrayXCount) {data->arrayXCount = arrayXCount;}
int LC_ActionEditPasteTransform::getArrayYCount() const {return data->arrayYCount;}
void LC_ActionEditPasteTransform::setArrayYCount(int arrayYCount) {data->arrayYCount = arrayYCount;}
double LC_ActionEditPasteTransform::getArraySpacingX() const {return data->arraySpacing.x;}
void LC_ActionEditPasteTransform::setArraySpacingX(double arraySpacing) {data->arraySpacing.x = arraySpacing;}
double LC_ActionEditPasteTransform::getArraySpacingY() const {return data->arraySpacing.y;}
void LC_ActionEditPasteTransform::setArraySpacingY(double arraySpacing) {data->arraySpacing.y = arraySpacing;}
double LC_ActionEditPasteTransform::getArrayAngle() const {return data->arrayAngle;}
void LC_ActionEditPasteTransform::setArrayAngle(double arrayAngle) {data->arrayAngle = arrayAngle;}
LC_ActionOptionsWidget *LC_ActionEditPasteTransform::createOptionsWidget() {return new LC_PasteTransformOptions();}

void LC_ActionEditPasteTransform::previewMultipleReferencePoints() {
    int numX = data->arrayXCount;
    int numY = data->arrayYCount;

    RS_Vector xArrayVector;
    RS_Vector yArrayVector;
    if (data->arrayCreated) {
        double arrayAngle = data->arrayAngle;
        xArrayVector = RS_Vector::polar(data->arraySpacing.x, arrayAngle);
        yArrayVector = RS_Vector::polar(data->arraySpacing.y, arrayAngle + M_PI_2);
    }
    else{
        xArrayVector = RS_Vector(0,0,0);
        yArrayVector = RS_Vector(0,0,0);
        numX = 1;
        numY = 1;
    }

    for (int x = 0; x < numX; x++){
        for (int y = 0; y < numY; y++){
            RS_Vector currentPoint = *referencePoint + xArrayVector*x + yArrayVector * y;
         /*   const RS_PasteData &pasteData = RS_PasteData(currentPoint, data->factor , data->angle,
                                                         false, "");*/
//            m.paste(pasteData);
            previewRefPoint(currentPoint);
        }
    }
}
