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
#include "lc_action_edit_paste_transform.h"

#include "lc_copyutils.h"
#include "lc_paste_transform_options_filler.h"
#include "lc_paste_transform_options_widget.h"
#include "rs_clipboard.h"
#include "rs_graphic.h"
#include "rs_preview.h"
#include "rs_units.h"

// fixme - sand - ucs - Check for support of UCS!

LC_ActionEditPasteTransform::LC_ActionEditPasteTransform(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionEditPasteTransform", actionContext, RS2::ActionEditPasteTransform),
      m_referencePoint{new RS_Vector(false)}, m_pasteData{std::make_unique<PasteData>()} {
}

void LC_ActionEditPasteTransform::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (RS_CLIPBOARD->count() == 0) {
        commandMessage(tr("Clipboard is empty"));
        finish();
    }
}

void LC_ActionEditPasteTransform::doSaveOptions() {
    save("Angle", getAngle());
    save("ScaleFactor", getFactor());
    save("IsArray", isArrayCreated());
    save("ArrayXCount", getArrayXCount());
    save("ArrayYCount", getArrayYCount());
    save("ArrayXSpacing", getArraySpacingX());
    save("ArrayYSpacing", getArraySpacingY());
    save("ArrayAngle", getArrayAngle());
    save("SameAngles", isSameAngles());
}

void LC_ActionEditPasteTransform::doLoadOptions() {
    double angle = loadDouble("Angle", 0.0);
    setAngle(angle);

    double factor = loadDouble("ScaleFactor", 1.0);
    setFactor(factor);

    bool array = loadBool("IsArray", false);
    setArrayCreated(array);

    int xCount = loadInt("ArrayXCount", 1.0);
    setArrayXCount(xCount);

    int yCount = loadInt("ArrayYCount", 1.0);
    setArrayYCount(yCount);

    int xSpacing = loadDouble("ArrayXSpacing", 10.0);
    setArraySpacingX(xSpacing);

    int ySpacing = loadDouble("ArrayYSpacing", 10.0);
    setArraySpacingY(ySpacing);

    double arrayAngle = loadDouble("ArrayAngle", 0.0);
    setArrayAngle(arrayAngle);

    bool sameAngles = loadBool("SameAngles", true);
    setSameAngles(sameAngles);
}

bool LC_ActionEditPasteTransform::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint);
}

bool LC_ActionEditPasteTransform::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    int numX = m_pasteData->arrayXCount;
    int numY = m_pasteData->arrayYCount;

    RS_Vector xArrayVector;
    RS_Vector yArrayVector;
    if (m_pasteData->arrayCreated) {
        const double arrayAngle = m_pasteData->arrayAngle;
        xArrayVector = RS_Vector::polar(m_pasteData->arraySpacing.x, arrayAngle);
        yArrayVector = RS_Vector::polar(m_pasteData->arraySpacing.y, arrayAngle + M_PI_2);
    }
    else {
        numX = 1;
        numY = 1;
    }
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            const RS_Vector currentPoint = *m_referencePoint + xArrayVector * x + yArrayVector * y;
            const auto pasteData = LC_CopyUtils::RS_PasteData(currentPoint, m_pasteData->factor, m_pasteData->angle);
            LC_CopyUtils::paste(pasteData, m_graphic, ctx);
            // fixme - some progress is needed there, ++++ speed improvement for paste operation!!
            //            LC_ERR << "Paste: " << x+y;
        }
    }
    ctx.dontSetActiveLayerAndPen();
    return true;
}

void LC_ActionEditPasteTransform::doTriggerCompletion([[maybe_unused]] bool success) {
    if (!m_invokedWithControl) {
        finish();
    }
}

void LC_ActionEditPasteTransform::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    if (status == SetReferencePoint) {
        *m_referencePoint = e->snapPoint;
        const auto clipboardGraphics = RS_CLIPBOARD->getGraphic();
        m_preview->addAllFrom(*clipboardGraphics, m_viewport);
        m_preview->move(*m_referencePoint);

        if (m_graphic) {
            const RS_Vector scaleFactor = LC_CopyUtils::getInterGraphicsScaleFactor(m_pasteData->factor, clipboardGraphics, m_graphic);
            m_preview->scale(*m_referencePoint, scaleFactor);
            m_preview->rotate(*m_referencePoint, m_pasteData->angle);

            if (m_showRefEntitiesOnPreview) {
                previewMultipleReferencePoints();
            }
        }
    }
    else {
        deleteSnapper();
    }
}

void LC_ActionEditPasteTransform::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    m_invokedWithControl = e->isControl;
    fireCoordinateEventForSnap(e);
}

void LC_ActionEditPasteTransform::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

void LC_ActionEditPasteTransform::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero, const RS_Vector& pos) {
    *m_referencePoint = pos;
    trigger();
}

RS2::CursorType LC_ActionEditPasteTransform::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionEditPasteTransform::updateActionPrompt() {
    switch (getStatus()) {
        case SetReferencePoint:
            updatePromptTRCancel(tr("Set paste reference point"), MOD_CTRL(tr("Paste Multiple")));
            break;

        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionEditPasteTransform::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    if (tag == "arrayAngle") {
        setArrayAngle(angle);
        return true;
    }
    return false;
}

bool LC_ActionEditPasteTransform::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "spacingX") {
        setArraySpacingX(distance);
        return true;
    }
    if (tag == "spacingY") {
        setArraySpacingY(distance);
        return true;
    }
    return false;
}

double LC_ActionEditPasteTransform::getAngle() const {
    return m_pasteData->angle;
}

void LC_ActionEditPasteTransform::setAngle(const double angle) const {
    m_pasteData->angle = angle;
}

double LC_ActionEditPasteTransform::getFactor() const {
    return m_pasteData->factor;
}

void LC_ActionEditPasteTransform::setFactor(const double factor) const {
    m_pasteData->factor = factor;
}

bool LC_ActionEditPasteTransform::isArrayCreated() const {
    return m_pasteData->arrayCreated;
}

void LC_ActionEditPasteTransform::setArrayCreated(const bool arrayCreated) const {
    m_pasteData->arrayCreated = arrayCreated;
}

int LC_ActionEditPasteTransform::getArrayXCount() const {
    return m_pasteData->arrayXCount;
}

void LC_ActionEditPasteTransform::setArrayXCount(const int arrayXCount) const {
    m_pasteData->arrayXCount = arrayXCount;
}

int LC_ActionEditPasteTransform::getArrayYCount() const {
    return m_pasteData->arrayYCount;
}

void LC_ActionEditPasteTransform::setArrayYCount(const int arrayYCount) const {
    m_pasteData->arrayYCount = arrayYCount;
}

double LC_ActionEditPasteTransform::getArraySpacingX() const {
    return m_pasteData->arraySpacing.x;
}

void LC_ActionEditPasteTransform::setArraySpacingX(const double arraySpacing) const {
    m_pasteData->arraySpacing.x = arraySpacing;
}

double LC_ActionEditPasteTransform::getArraySpacingY() const {
    return m_pasteData->arraySpacing.y;
}

void LC_ActionEditPasteTransform::setArraySpacingY(const double arraySpacing) const {
    m_pasteData->arraySpacing.y = arraySpacing;
}

double LC_ActionEditPasteTransform::getArrayAngle() const {
    return m_pasteData->arrayAngle;
}

void LC_ActionEditPasteTransform::setArrayAngle(const double arrayAngle) const {
    m_pasteData->arrayAngle = arrayAngle;
}

LC_ActionOptionsWidget* LC_ActionEditPasteTransform::createOptionsWidget() {
    return new LC_PasteTransformOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionEditPasteTransform::createOptionsFiller() {
    return new LC_PasteTransformOptionsFiller();
}

void LC_ActionEditPasteTransform::previewMultipleReferencePoints() const {
    int numX = m_pasteData->arrayXCount;
    int numY = m_pasteData->arrayYCount;

    RS_Vector xArrayVector;
    RS_Vector yArrayVector;
    if (m_pasteData->arrayCreated) {
        const double arrayAngle = m_pasteData->arrayAngle;
        xArrayVector = RS_Vector::polar(m_pasteData->arraySpacing.x, arrayAngle);
        yArrayVector = RS_Vector::polar(m_pasteData->arraySpacing.y, arrayAngle + M_PI_2);
    }
    else {
        xArrayVector = RS_Vector(0, 0, 0);
        yArrayVector = RS_Vector(0, 0, 0);
        numX = 1;
        numY = 1;
    }

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            RS_Vector currentPoint = *m_referencePoint + xArrayVector * x + yArrayVector * y;
            /*   const RS_PasteData &pasteData = RS_PasteData(currentPoint, data->factor , data->angle,
                                                            false, "");*/
            //            m.paste(pasteData);
            previewRefPoint(currentPoint);
        }
    }
}
