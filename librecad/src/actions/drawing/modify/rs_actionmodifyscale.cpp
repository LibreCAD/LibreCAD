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

#include "rs_actionmodifyscale.h"
#include "lc_actioninfomessagebuilder.h"
#include "lc_modifyscaleoptions.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"

struct RS_ActionModifyScale::ScaleActionData {
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
    double getFactor(double reference, double source, double target) {
        const double dxOld = source - reference;
        const double dxNew = target - reference;
        return (std::abs(dxOld) > RS_TOLERANCE && std::abs(dxNew) > RS_TOLERANCE) ? dxNew/dxOld : 1.;
    }
}

RS_ActionModifyScale::RS_ActionModifyScale(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("Scale Entities", actionContext, RS2::ActionModifyScale)
    , m_actionData(std::make_unique<ScaleActionData>()){
}

RS_ActionModifyScale::~RS_ActionModifyScale() = default;

void RS_ActionModifyScale::init(int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

void RS_ActionModifyScale::doTrigger(bool keepSelected) {
    RS_DEBUG->print("RS_ActionModifyScale::trigger()");
    deletePreview();
    if (m_actionData->data.isotropicScaling){
        m_actionData->data.factor.y = m_actionData->data.factor.x;
    }
    RS_Modification m(*m_container, m_viewport);
    m.scale(m_actionData->data, m_selectedEntities, false, keepSelected);
}

#define DRAW_TRIANGLES_ON_PREVIEW_NO

void RS_ActionModifyScale::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = mouse;
            RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
            RS_Vector selectionCenter = boundingForSelected.getCenter();
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
                RS_Vector centerPoint =  m_actionData->data.referencePoint;
                RS_Vector offset = m_actionData->sourcePoint - mouse;
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

RS_Vector RS_ActionModifyScale::getTargetPoint(LC_MouseEvent* e){
    if (m_actionData->data.isotropicScaling) {
        RS_Vector mouse = e->snapPoint;
        if (e->isControl){
            mouse = e->graphPoint;
        }
        // project mouse to the line (center, source)
        RS_Line centerSourceLine{nullptr, {m_actionData->data.referencePoint, m_actionData->sourcePoint}};
        RS_Vector projected = centerSourceLine.getNearestPointOnEntity(mouse, false);
        snapPoint(projected, true);
        return projected;
    } else {
        return e->snapPoint;
    }
}

void RS_ActionModifyScale::showPreview(){
    findFactor();
    RS_ScaleData &previewData = m_actionData->data;
    showPreview(previewData);
    updateOptionsUI(0);
}

void RS_ActionModifyScale::showPreview(RS_ScaleData &previewData) {
    RS_Modification m(*m_preview, m_viewport, false);
    m.scale(previewData, m_selectedEntities, true, false);

    if (m_showRefEntitiesOnPreview) {
        int numberOfCopies = previewData.obtainNumberOfCopies();

        if (numberOfCopies > 1) {
            for (int i = 1; i <= numberOfCopies; i++) {
                RS_Vector scaledSource = m_actionData->sourcePoint;
                scaledSource.scale(previewData.referencePoint, RS_Math::pow(previewData.factor, i));
                previewRefPoint(scaledSource);
            }
        }
    }
}

void RS_ActionModifyScale::onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    switch (status){
        case SetReferencePoint: {
            if (e->isControl){
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
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

void RS_ActionModifyScale::onMouseRightButtonReleaseSelected(int status, [[maybe_unused]]LC_MouseEvent *e) {
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
bool RS_ActionModifyScale::doProcessCommand(int status, const QString &c) {
    bool accepted = false;
    switch (status){
        case SetReferencePoint: {
            if (checkCommand("center",c)) {
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(m_selectedEntities);
                RS_Vector centerPoint = boundingForSelected.getCenter();
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

QStringList RS_ActionModifyScale::getAvailableCommands() {
    if (getStatus() == SetReferencePoint){
        return {command("center")};
    }
    return {};
}

void RS_ActionModifyScale::tryTrigger() {
    if (m_actionData->data.toFindFactor) {
        setStatus(SetSourcePoint);
    } else {
        trigger();
        finish();
    }
}

void RS_ActionModifyScale::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch(status) {
        case SetReferencePoint: {
            m_actionData->data.referencePoint = mouse;
            moveRelativeZero(mouse);
            if (isShowModifyActionDialog()) {
                if (RS_DIALOGFACTORY->requestScaleDialog(m_actionData->data)) {
                    tryTrigger();
                }
            }
            else{
                tryTrigger();
            }
            break;
        }
        case SetSourcePoint: {
            previewRefPoint(m_actionData->data.referencePoint);
            if (mouse.squaredTo(m_actionData->data.referencePoint) > RS_TOLERANCE2) {
                m_actionData->sourcePoint = mouse;
                setStatus(SetTargetPoint);
            }
            break;
        }
        case SetTargetPoint: {
            if (mouse.squaredTo(m_actionData->data.referencePoint) > RS_TOLERANCE2) {
                m_actionData->targetPoint = mouse;
                trigger();
                finish();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyScale::findFactor(){
    auto& reference = m_actionData->data.referencePoint;
    auto& source = m_actionData->sourcePoint;
    auto& target = m_actionData->targetPoint;
    determineScaleFactor(m_actionData->data, reference, source, target);
}

void RS_ActionModifyScale::determineScaleFactor(RS_ScaleData& data,
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

void RS_ActionModifyScale::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            updateMouseWidgetTRCancel(tr( "Specify scale center"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
            // Find the scale factors to scale the pPoints->sourcePoint to pPoints->targetPoint
        case SetSourcePoint:
            updateMouseWidgetTRCancel(tr("Specify source point"), m_actionData->data.isotropicScaling ? MOD_NONE: MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRCancel(tr("Specify target point"), m_actionData->data.isotropicScaling ? MOD_CTRL(tr("Free snap")): MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyScale::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to scale  (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Scale immediately after selection")));
}

RS2::CursorType RS_ActionModifyScale::doGetMouseCursorSelected([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *RS_ActionModifyScale::getModifyOperationFlags() {
    return &m_actionData->data;
}

bool RS_ActionModifyScale::isIsotropicScaling(){
    return m_actionData->data.isotropicScaling;
}

void RS_ActionModifyScale::setIsotropicScaling(bool enable){
    m_actionData->data.isotropicScaling = enable;
}

double RS_ActionModifyScale::getFactorX() {
    return m_actionData->data.factor.x;
}

void RS_ActionModifyScale::setFactorX(double val) {
    m_actionData->data.factor.x = val;
}

double RS_ActionModifyScale::getFactorY() {
    return m_actionData->data.factor.y;
}

void RS_ActionModifyScale::setFactorY(double val) {
    m_actionData->data.factor.y = val;
}

bool RS_ActionModifyScale::isExplicitFactor() {
    return m_actionData->data.toFindFactor;
}

void RS_ActionModifyScale::setExplicitFactor(bool val) {
    if (!val){
        int status = getStatus();
        if (status == SetTargetPoint || status == SetSourcePoint){
            setStatus(SetReferencePoint);
        }
    }
    m_actionData->data.toFindFactor = val;
}

LC_ActionOptionsWidget *RS_ActionModifyScale::createOptionsWidget() {
    return new LC_ModifyScaleOptions();
}
