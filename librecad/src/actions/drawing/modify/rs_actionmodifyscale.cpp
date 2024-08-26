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
#include <QMouseEvent>

#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "lc_modifyscaleoptions.h"
#include "rs_math.h"

struct RS_ActionModifyScale::Points {
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

RS_ActionModifyScale::RS_ActionModifyScale(RS_EntityContainer& container, RS_GraphicView& graphicView)
    :LC_ActionModifyBase("Scale Entities",container, graphicView)
    , pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionModifyScale;
}

RS_ActionModifyScale::~RS_ActionModifyScale() = default;

void RS_ActionModifyScale::init(int status) {
    LC_ActionPreSelectionAwareBase::init(status);
}

void RS_ActionModifyScale::trigger() {
    RS_DEBUG->print("RS_ActionModifyScale::trigger()");
    deletePreview();
    if (pPoints->data.isotropicScaling){
        pPoints->data.factor.y = pPoints->data.factor.x;
    }
    RS_Modification m(*container, graphicView);
    m.scale(pPoints->data, selectedEntities, false);
    updateSelectionWidget();
}

#define DRAW_TRIANGLES_ON_PREVIEW_NO

void RS_ActionModifyScale::mouseMoveEventSelected(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            pPoints->data.referencePoint = mouse;
            RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
            RS_Vector selectionCenter = boundingForSelected.getCenter();
            pPoints->sourcePoint = selectionCenter;
            if (!trySnapToRelZeroCoordinateEvent(e)){
                if (isControl(e)){
                    mouse = selectionCenter;
                }
                RS_ScaleData previewData = pPoints->data;
                previewData.referencePoint = mouse;
                if (previewData.isotropicScaling){
                    previewData.factor.y = previewData.factor.x;
                }
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(selectionCenter);
                }
                showPreview(previewData);
            }
            break;
        }
        case SetSourcePoint: {
            mouse = getSnapAngleAwarePoint(e, pPoints->data.referencePoint, mouse, true);
            previewRefPoint(pPoints->data.referencePoint);
            pPoints->sourcePoint = mouse;
            RS_ScaleData previewData = pPoints->data;
            if (showRefEntitiesOnPreview) {
                determineScaleFactor(previewData, pPoints->data.referencePoint, mouse, mouse);
                previewRefLine(mouse, pPoints->data.referencePoint);
                previewRefSelectablePoint(mouse);
            }
            showPreview(previewData);
            break;
        }
        case SetTargetPoint: {
            if (pPoints->data.isotropicScaling){
               mouse = getTargetPoint(e);
            }
            else{
                mouse = getSnapAngleAwarePoint(e, pPoints->sourcePoint, mouse, true); // todo - review whether it's necessary
            }

            if (showRefEntitiesOnPreview) {
                // control points
                previewRefSelectablePoint(pPoints->sourcePoint);
                previewRefSelectablePoint(mouse);
                previewRefPoint(pPoints->data.referencePoint);

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
                previewRefLine(pPoints->sourcePoint, pPoints->data.referencePoint);
                // target point triangle
                previewRefLine(mouse, pPoints->data.referencePoint);
                // source to target
                previewRefSelectableLine(mouse, pPoints->sourcePoint);
            }
            RS_ScaleData previewData = pPoints->data;
            determineScaleFactor(previewData, pPoints->data.referencePoint, pPoints->sourcePoint, mouse);
            pPoints->targetPoint = mouse;

            showPreview();
            break;
        }
        default:
            break;
    }
    drawPreview();
    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent end");
}

RS_Vector RS_ActionModifyScale::getTargetPoint(QMouseEvent* e){
    if (pPoints->data.isotropicScaling) {
        RS_Vector mouse = snapPoint(e);
        if (isControl(e)){
            mouse = toGraph(e);
        }
        // project mouse to the line (center, source)
        RS_Line centerSourceLine{nullptr, {pPoints->data.referencePoint, pPoints->sourcePoint}};
        RS_Vector projected = centerSourceLine.getNearestPointOnEntity(mouse, false);
        snapPoint(projected, true);
        return projected;
    } else {
        return snapPoint(e);
    }
}

void RS_ActionModifyScale::showPreview(){
    findFactor();
    RS_ScaleData &previewData = pPoints->data;
    showPreview(previewData);
    updateOptionsUI(0);
}

void RS_ActionModifyScale::showPreview(RS_ScaleData &previewData) {
    RS_Modification m(*preview, graphicView, false);
    m.scale(previewData, selectedEntities, true);

    if (showRefEntitiesOnPreview) {
        int numberOfCopies = previewData.obtainNumberOfCopies();

        if (numberOfCopies > 1) {
            for (int i = 1; i <= numberOfCopies; i++) {
                RS_Vector scaledSource = pPoints->sourcePoint;
                scaledSource.scale(previewData.referencePoint, RS_Math::pow(previewData.factor, i));
                previewRefPoint(scaledSource);
            }
        }
    }
}

void RS_ActionModifyScale::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    switch (status){
        case SetReferencePoint: {
            if (isControl(e)){
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
                snapped = boundingForSelected.getCenter();
            }
            break;
        }
        case SetSourcePoint: {
            snapped = getSnapAngleAwarePoint(e, pPoints->data.referencePoint, snapped);
            break;
        }
        case SetTargetPoint: {
            if (pPoints->data.isotropicScaling){
                snapped = getTargetPoint(e);
            }
            else{
                snapped = getSnapAngleAwarePoint(e, pPoints->sourcePoint, snapped, true); // todo - review whether it's necessary
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionModifyScale::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    switch (status) {
        case SetReferencePoint: {
            selectionComplete = false;
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
                RS_BoundData boundingForSelected = RS_Modification::getBoundingRect(selectedEntities);
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
    if (pPoints->data.toFindFactor) {
        setStatus(SetSourcePoint);
    } else {
        trigger();
        finish();
    }
}

void RS_ActionModifyScale::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch(status) {
        case SetReferencePoint: {
            pPoints->data.referencePoint = mouse;
            graphicView->setRelativeZero(mouse);
            if (isShowModifyActionDialog()) {
                if (RS_DIALOGFACTORY->requestScaleDialog(pPoints->data)) {
                    tryTrigger();
                }
            }
            else{
                tryTrigger();
            }
            break;
        }
        case SetSourcePoint: {
            previewRefPoint(pPoints->data.referencePoint);
            if (mouse.squaredTo(pPoints->data.referencePoint) > RS_TOLERANCE2) {
                pPoints->sourcePoint = mouse;
                setStatus(SetTargetPoint);
            }
            break;
        }
        case SetTargetPoint: {
            if (mouse.squaredTo(pPoints->data.referencePoint) > RS_TOLERANCE2) {
                pPoints->targetPoint = mouse;
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
    auto& reference = pPoints->data.referencePoint;
    auto& source = pPoints->sourcePoint;
    auto& target = pPoints->targetPoint;
    determineScaleFactor(pPoints->data, reference, source, target);
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
            updateMouseWidgetTRCancel(tr( "Specify scale center"), MOD_SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_REL_ZERO, tr("Snap to center of selection")));
            break;
            // Find the scale factors to scale the pPoints->sourcePoint to pPoints->targetPoint
        case SetSourcePoint:
            updateMouseWidgetTRCancel(tr("Specify source point"), pPoints->data.isotropicScaling ? MOD_NONE: MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetTargetPoint:
            updateMouseWidgetTRCancel(tr("Specify target point"), pPoints->data.isotropicScaling ? MOD_CTRL(tr("Free snap")): MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyScale::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to scale"), MOD_CTRL(tr("Scale immediately after selection")));
}

RS2::CursorType RS_ActionModifyScale::doGetMouseCursorSelected([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ModifyOperationFlags *RS_ActionModifyScale::getModifyOperationFlags() {
    return &pPoints->data;
}

bool RS_ActionModifyScale::isIsotropicScaling(){
    return pPoints->data.isotropicScaling;
}

void RS_ActionModifyScale::setIsotropicScaling(bool enable){
    pPoints->data.isotropicScaling = enable;
}

double RS_ActionModifyScale::getFactorX() {
    return pPoints->data.factor.x;
}

void RS_ActionModifyScale::setFactorX(double val) {
    pPoints->data.factor.x = val;
}

double RS_ActionModifyScale::getFactorY() {
    return pPoints->data.factor.y;
}

void RS_ActionModifyScale::setFactorY(double val) {
    pPoints->data.factor.y = val;
}

bool RS_ActionModifyScale::isExplicitFactor() {
    return pPoints->data.toFindFactor;
}

void RS_ActionModifyScale::setExplicitFactor(bool val) {
    if (!val){
        int status = getStatus();
        if (status == SetTargetPoint || status == SetSourcePoint){
            setStatus(SetReferencePoint);
        }
    }
    pPoints->data.toFindFactor = val;
}

LC_ActionOptionsWidget *RS_ActionModifyScale::createOptionsWidget() {
    return new LC_ModifyScaleOptions();
}
