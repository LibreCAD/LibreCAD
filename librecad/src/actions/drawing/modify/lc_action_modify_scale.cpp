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

#include "lc_action_modify_scale.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_scale_options_filler.h"
#include "lc_scale_options_widget.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct LC_ActionModifyScale::ScaleActionData {
    RS_ScaleData data;
    RS_Vector sourcePoint;
    RS_Vector targetPoint;
};

namespace {

/**
 * @brief getFactor find the factor to scale source to target around reference
 *        target = reference + (source - reference) * factor
 * @param reference - the reference
 * @param source - the source
 * @param target - the target
 * @return double - factor
 */
    double getFactor(const double reference, const double source, const double target) {
        const double dxOld = source - reference;
        const double dxNew = target - reference;
        return (std::abs(dxOld) > RS_TOLERANCE && std::abs(dxNew) > RS_TOLERANCE) ? dxNew/dxOld : 1.;
    }
}

LC_ActionModifyScale::LC_ActionModifyScale(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("ActionModifyScale", actionContext, RS2::ActionModifyScale)
    , m_actionData(std::make_unique<ScaleActionData>()){
}

LC_ActionModifyScale::~LC_ActionModifyScale() = default;

void LC_ActionModifyScale::doSaveOptions() {
    save("UseCurrentLayer",isUseCurrentLayer());
    save("UseCurrentAttributes",isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());

    save("FactorX", getFactorX());
    save("FactorY", getFactorY());
    save("Isotropic",     isIsotropicScaling());
    save("ExplicitFactor", isExplicitFactor());
}

void LC_ActionModifyScale::doLoadOptions() {
    const bool curLayer = loadBool("UseCurrentLayer", true);
    setUseCurrentLayer(curLayer);
    const bool curAttrs = loadBool("UseCurrentAttributes", true);
    setUseCurrentAttributes(curAttrs);
    const bool keepOriginals = loadBool("KeepOriginals", false);
    setKeepOriginals(keepOriginals);
    const bool multiCopy = loadBool("MultipleCopies", false);
    setUseMultipleCopies(multiCopy);
    const int copiesNum = loadInt("Copies", 1);
    setCopiesNumber(copiesNum);

    const bool factorX = loadDouble("FactorX", 1.0);
    setFactorX(factorX);

    const bool factorY = loadDouble("FactorY", 1.0);
    setFactorX(factorY);

    const bool isotrophic = loadBool("Isotropic", true);
    setIsotropicScaling(isotrophic);

    const bool explicitFactor = loadBool("ExplicitFactor", false);
    setExplicitFactor(explicitFactor);
}

bool LC_ActionModifyScale::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint) || (status == SetTargetPoint) || (status == SetSourcePoint);
}

void LC_ActionModifyScale::init(const int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

bool LC_ActionModifyScale::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    deletePreview();
    auto scaleData = m_actionData->data;
    if (scaleData.isotropicScaling){
        scaleData.factor.y = scaleData.factor.x;
    }
    RS_Modification::scale(scaleData, m_selectedEntities, false, ctx);
    ctx.setActiveLayerAndPen(scaleData.useCurrentLayer, scaleData.useCurrentAttributes);
    return true;
}

void LC_ActionModifyScale::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (m_actionData->data.keepOriginals) {
        unselect(m_selectedEntities);
    }
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyScale::doTriggerCompletion([[maybe_unused]]bool success) {
}

#define DRAW_TRIANGLES_ON_PREVIEW_NO

void LC_ActionModifyScale::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = mouse;
            const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
            const RS_Vector selectionCenter = boundingForSelected.getCenter();
            m_actionData->sourcePoint = selectionCenter;
            if (!trySnapToRelZeroCoordinateEvent(e)){
                if (e->isControl){
                    mouse = selectionCenter;
                }
                RS_ScaleData previewData = m_actionData->data;
                previewData.referencePoint = mouse;
                if (previewData.isotropicScaling){
                    previewData.factor.y = previewData.factor.x;
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(selectionCenter);
                }
                showPreview(previewData);
            }
            break;
        }
        case SetSourcePoint: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->data.referencePoint, mouse, true);
            previewRefPoint(m_actionData->data.referencePoint);
            m_actionData->sourcePoint = mouse;
            RS_ScaleData previewData = m_actionData->data;
            if (m_showRefEntitiesOnPreview) {
                determineScaleFactor(previewData, m_actionData->data.referencePoint, mouse, mouse);
                previewRefLine(mouse, m_actionData->data.referencePoint);
                previewRefSelectablePoint(mouse);
            }
            showPreview(previewData);
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->data.isotropicScaling){
               mouse = getTargetPoint(e);
            }
            else{
                mouse = getSnapAngleAwarePoint(e, m_actionData->sourcePoint, mouse, true); // todo - review whether it's necessary
            }

            if (m_showRefEntitiesOnPreview) {
                // control points
                previewRefSelectablePoint(m_actionData->sourcePoint);
                previewRefSelectablePoint(mouse);
                previewRefPoint(m_actionData->data.referencePoint);

#ifdef DRAW_TRIANGLES_ON_PREVIEW
                RS_Vector intersectionPoint = RS_Vector(mouse.x, pPoints->data.referencePoint.y);
                previewRefSelectablePoint(intersectionPoint);

                // projections of source points to axis
                RS_Vector sourceProjectionY = RS_Vector(mouse.x, pPoints->sourcePoint.y);
                RS_Vector sourceProjectionX = RS_Vector(pPoints->sourcePoint.x, pPoints->data.referencePoint.y);
                previewRefPoint(sourceProjectionY);
                previewRefPoint(sourceProjectionX);

                previewRefSelectableLine(intersectionPoint, sourceProjectionX);
                previewRefSelectableLine(intersectionPoint, sourceProjectionY);

                previewRefLine(intersectionPoint, mouse);
                previewRefLine(pPoints->data.referencePoint, intersectionPoint);

                previewRefLine(pPoints->sourcePoint, sourceProjectionX);
#endif

                // source point triangle
                previewRefLine(m_actionData->sourcePoint, m_actionData->data.referencePoint);
                // target point triangle
                previewRefLine(mouse, m_actionData->data.referencePoint);
                // source to target
                previewRefSelectableLine(mouse, m_actionData->sourcePoint);
            }
            RS_ScaleData previewData = m_actionData->data;
            determineScaleFactor(previewData, m_actionData->data.referencePoint, m_actionData->sourcePoint, mouse);
            m_actionData->targetPoint = mouse;

            showPreview();

            if (isInfoCursorForModificationEnabled()) {
                const RS_Vector centerPoint =  m_actionData->data.referencePoint;
                const RS_Vector offset = m_actionData->sourcePoint - mouse;
                msg(tr("Scale"))
                    .vector(tr("Center:"), centerPoint)
                    .vector(tr("Source Point:"), m_actionData->sourcePoint)
                    .vector(tr("Target Point:"), mouse)
                    .string(tr("Offset:"))
                    .relative(offset)
                    .relativePolar(offset)
                    .linear(tr("Scale by X:"), m_actionData->data.factor.x)
                    .linear(tr("Scale by Y:"), m_actionData->data.factor.y)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

RS_Vector LC_ActionModifyScale::getTargetPoint(const LC_MouseEvent* e){
    if (m_actionData->data.isotropicScaling) {
        RS_Vector mouse = e->snapPoint;
        if (e->isControl){
            mouse = e->graphPoint;
        }
        // project mouse to the line (center, source)
        const RS_Line centerSourceLine{nullptr, {m_actionData->data.referencePoint, m_actionData->sourcePoint}};
        const RS_Vector projected = centerSourceLine.getNearestPointOnEntity(mouse, false);
        snapPoint(projected, true);
        return projected;
    }
    return e->snapPoint;
}

void LC_ActionModifyScale::showPreview(){
    findFactor();
    const RS_ScaleData &previewData = m_actionData->data;
    showPreview(previewData);
    updateOptionsUI(0);
}

void LC_ActionModifyScale::showPreview(const RS_ScaleData &previewData) const {
    LC_DocumentModificationBatch ctx;
    RS_Modification::scale(previewData, m_selectedEntities, true, ctx);
    previewEntitiesToAdd(ctx);

    if (m_showRefEntitiesOnPreview) {
        const int numberOfCopies = previewData.obtainNumberOfCopies();

        if (numberOfCopies > 1) {
            for (int i = 1; i <= numberOfCopies; i++) {
                RS_Vector scaledSource = m_actionData->sourcePoint;
                scaledSource.scale(previewData.referencePoint, RS_Math::pow(previewData.factor, i));
                previewRefPoint(scaledSource);
            }
        }
    }
}

void LC_ActionModifyScale::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    switch (status){
        case SetReferencePoint: {
            if (e->isControl){
                const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                snapped = boundingForSelected.getCenter();
            }
            break;
        }
        case SetSourcePoint: {
            snapped = getSnapAngleAwarePoint(e, m_actionData->data.referencePoint, snapped);
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->data.isotropicScaling){
                snapped = getTargetPoint(e);
            }
            else{
                snapped = getSnapAngleAwarePoint(e, m_actionData->sourcePoint, snapped, true); // todo - review whether it's necessary
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionModifyScale::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            m_selectionComplete = false;
            break;
        }
        default: {
            initPrevious(status);
            break;
        }
    }
}
// fixme - is it necessary to add other commands support? Fixed, scale factors etc.
bool LC_ActionModifyScale::doProcessCommand(const int status, const QString &command) {
    bool accepted = false;
    switch (status){
        case SetReferencePoint: {
            if (checkCommand("center",command)) {
                const RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                const RS_Vector centerPoint = boundingForSelected.getCenter();
                fireCoordinateEvent(centerPoint);
                accepted = true;
            }
            break;
        }
        default:
            break;
    }
    return accepted;
}

QStringList LC_ActionModifyScale::getAvailableCommands() {
    if (getStatus() == SetReferencePoint){
        return {command("center")};
    }
    return {};
}

void LC_ActionModifyScale::tryTrigger() {
    if (m_actionData->data.toFindFactor) {
        setStatus(SetSourcePoint);
    } else {
        trigger();
        finish();
    }
}

void LC_ActionModifyScale::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    switch(status) {
        case SetReferencePoint: {
            addSnappedPointToVisualSnap(coord);
            m_actionData->data.referencePoint = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            tryTrigger();
            break;
        }
        case SetSourcePoint: {
            previewRefPoint(m_actionData->data.referencePoint);
            if (coord.squaredTo(m_actionData->data.referencePoint) > RS_TOLERANCE2) {
                addSnappedPointToVisualSnap(coord);
                m_actionData->sourcePoint = coord;
                setStatus(SetTargetPoint);
            }
            break;
        }
        case SetTargetPoint: {
            if (coord.squaredTo(m_actionData->data.referencePoint) > RS_TOLERANCE2) {
                addSnappedPointToVisualSnap(coord);
                m_actionData->targetPoint = coord;
                trigger();
                finish();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyScale::findFactor(){
    const auto& reference = m_actionData->data.referencePoint;
    const auto& source = m_actionData->sourcePoint;
    const auto& target = m_actionData->targetPoint;
    determineScaleFactor(m_actionData->data, reference, source, target);
}

void LC_ActionModifyScale::determineScaleFactor(RS_ScaleData& data,
                                                const RS_Vector &reference, const RS_Vector &source, const RS_Vector &target) {
    data.factor.x = getFactor(reference.x, source.x, target.x);
    if (data.isotropicScaling){
        data.factor.y = data.factor.x;
    }
    else {
        data.factor.y = getFactor(reference.y, source.y, target.y);
    }
    data.factor.valid = true;
}

void LC_ActionModifyScale::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            updatePromptTRCancel(tr( "Specify scale center"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
            // Find the scale factors to scale the pPoints->sourcePoint to pPoints->targetPoint
        case SetSourcePoint:
            updatePromptTRCancel(tr("Specify source point"), m_actionData->data.isotropicScaling ? MOD_NONE: MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetTargetPoint:
            updatePromptTRCancel(tr("Specify target point"), m_actionData->data.isotropicScaling ? MOD_CTRL(tr("Free snap")): MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyScale::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to scale") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Scale immediately after selection")));
}

RS2::CursorType LC_ActionModifyScale::doGetMouseCursorSelected([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *LC_ActionModifyScale::getModifyOperationFlags() {
    return &m_actionData->data;
}

bool LC_ActionModifyScale::isIsotropicScaling() const {
    return m_actionData->data.isotropicScaling;
}

void LC_ActionModifyScale::setIsotropicScaling(const bool enable) const {
    m_actionData->data.isotropicScaling = enable;
}

double LC_ActionModifyScale::getFactorX() const {
    return m_actionData->data.factor.x;
}

void LC_ActionModifyScale::setFactorX(const double val) const {
    m_actionData->data.factor.x = val;
}

double LC_ActionModifyScale::getFactorY() const {
    return m_actionData->data.factor.y;
}

void LC_ActionModifyScale::setFactorY(const double val) const {
    m_actionData->data.factor.y = val;
}

bool LC_ActionModifyScale::isExplicitFactor() const {
    return m_actionData->data.toFindFactor;
}

void LC_ActionModifyScale::setExplicitFactor(const bool val) {
    if (!val){
        const int status = getStatus();
        if (status == SetTargetPoint || status == SetSourcePoint){
            setStatus(SetReferencePoint);
        }
    }
    m_actionData->data.toFindFactor = val;
}

LC_ActionOptionsWidget *LC_ActionModifyScale::createOptionsWidget() {
    return new LC_ScaleOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyScale::createOptionsFiller() {
    return new LC_ScaleOptionsFiller();
}
