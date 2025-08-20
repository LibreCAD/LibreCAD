/****************************************************************************
**
* Abstract base class for linear dimensions

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
**********************************************************************/

#include "lc_actiondimlinearbase.h"

#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_dimaligned.h"
#include "rs_dimension.h"
#include "rs_dimlinear.h"
#include "rs_entity.h"
#include "rs_polyline.h"
#include "rs_preview.h"

LC_ActionDimLinearBase::LC_ActionDimLinearBase(const char *name, LC_ActionContext *actionContext,  RS2::EntityType dimType, RS2::ActionType actionType):
   RS_ActionDimension(name, actionContext, dimType, actionType){
}

LC_ActionDimLinearBase::~LC_ActionDimLinearBase() = default;

#define DEBUG_DIM_SNAP_NO

namespace {
    //list of entity types supported by current action - line, arc, circle
    const auto dimEntityTypes = EntityTypeList{RS2::EntityDimLinear, RS2::EntityDimAligned};
}

void LC_ActionDimLinearBase::doTrigger() {
    preparePreview(m_alternateDimDirection);
    auto *dim = createDim(m_container);
    setPenAndLayerToActive(dim);
    dim->update();
    undoCycleAdd(dim);
    RS_DEBUG->print("LC_ActionDimLinearBase::trigger(): dim added: %lu", dim->getId());
}

void LC_ActionDimLinearBase::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        setExtensionPoint1(entity->getStartpoint());
        setExtensionPoint2(entity->getEndpoint());
        setStatus(SetDefPoint);
    }
}

void LC_ActionDimLinearBase::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;

    bool ctrlPressed = e->isControl;
    switch (status) {
        case SetExtPoint1: {
            if (ctrlPressed) {
                auto entity = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
                if (isLine(entity)) {
                    highlightHover(entity);
                }
            }
            else {
                trySnapToRelZeroCoordinateEvent(e);
            }
            break;
        }
        case SetExtPoint2: {
            const RS_Vector &extPoint1 = getExtensionPoint1();
            if (extPoint1.valid){
                mouse = getSnapAngleAwarePoint(e, extPoint1, mouse, true);

                m_dimensionData->definitionPoint = mouse;
                setExtensionPoint2(mouse);

                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(extPoint1, mouse);
                    previewRefSelectablePoint(mouse);
                }
                if (m_previewShowsFullDimension) {
                    preparePreview(ctrlPressed);
                    m_alternateDimDirection = ctrlPressed;
                    RS_Entity *dim = createDim(m_preview.get());
                    dim->update();
                    previewEntity(dim);
                }
                else if (m_showRefEntitiesOnPreview) {
                    previewRefLine(extPoint1, mouse);
                }
                else{
                    previewLine(extPoint1, mouse);
                }
            }
            break;
        }
        case SetDefPoint:{
            const RS_Vector &extPoint1 = getExtensionPoint1();
            const RS_Vector &extPoint2 = getExtensionPoint2();
            if (extPoint1.valid && extPoint2.valid){
                // less restrictive snap
                mouse = getFreeSnapAwarePoint(e, mouse);
                m_alternateDimDirection = ctrlPressed;
                mouse = adjustByAdjacentDim(mouse, true, ctrlPressed);
                m_dimensionData->definitionPoint = mouse;
                preparePreview(ctrlPressed);
                RS_Entity* dim = createDim(m_preview.get());
                dim->update();
                previewEntity(dim);
#ifdef DEBUG_DIM_SNAP
                RS_Vector p1;
                RS_Vector p2;
                dim->getDimPoints(p1, p2);

                addReferencePointToPreview(p1);
                addReferencePointToPreview(p2);
#endif
                break;
            }
        }
        default:
            break;
    }
}

void LC_ActionDimLinearBase::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetExtPoint1: {
            if (e->isControl) {
                auto entity = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
                if (isLine(entity)) {
                    setExtensionPoint1(entity->getStartpoint());
                    setExtensionPoint2(entity->getEndpoint());
                    setStatus(SetDefPoint);
                }
                else {
                    fireCoordinateEvent(snap);
                }
            }
            else {
                fireCoordinateEvent(snap);
            }
            break;
        }
        case SetExtPoint2: {
            m_alternateDimDirection = e->isControl;
            snap = getSnapAngleAwarePoint(e, getExtensionPoint1(), snap);
            fireCoordinateEvent(snap);
            break;
        }
        case SetDefPoint: {
            // less restrictive snap
            snap = getFreeSnapAwarePoint(e, snap);
            m_alternateDimDirection = e->isControl;
            snap = adjustByAdjacentDim(snap, false, m_alternateDimDirection);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            fireCoordinateEvent(snap);
            break;
    }
}

void LC_ActionDimLinearBase::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDimLinearBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetExtPoint1: {
            setExtensionPoint1(pos);
            moveRelativeZero(pos);
            setStatus(SetExtPoint2);
            break;
        }
        case SetExtPoint2: {
            setExtensionPoint2(pos);
            moveRelativeZero(pos);
            setStatus(SetDefPoint);
            break;
        }
        case SetDefPoint: {
            m_dimensionData->definitionPoint = pos;
            trigger();
            reset();
            setStatus(SetExtPoint1);
            break;
        }
        default:
            break;
    }
}

RS_Vector LC_ActionDimLinearBase::getAdjacentDimDimSnapPoint(const RS_Vector &ownDimPointToCheck, double snapRange){
    auto result = RS_Vector(false);
    auto dimCandidate = RS_Snapper::catchEntity(ownDimPointToCheck, dimEntityTypes, RS2::ResolveNone);
    if (dimCandidate != nullptr){
        int rtti = dimCandidate->rtti();
        bool dimFound = true;
        RS_Vector candidateDim1;
        RS_Vector candidateDim2;
        switch (rtti){
            case RS2::EntityDimAligned:{
                auto* startAligned = dynamic_cast<RS_DimAligned *>(dimCandidate);
                startAligned->getDimPoints(candidateDim1, candidateDim2);
                break;
            }
            case RS2::EntityDimLinear: {
                auto *startLin = dynamic_cast<RS_DimLinear *>(dimCandidate);
                startLin->getDimPoints(candidateDim1, candidateDim2);
                break;
            }
            default: {
                dimFound = false;
            }
        }
        if (dimFound){
            if (candidateDim1.distanceTo(ownDimPointToCheck) < snapRange){
                result = candidateDim1;
            } else if (candidateDim2.distanceTo(ownDimPointToCheck) < snapRange){
                result = candidateDim2;
            }
        }
    }
    return result;
}

RS_Vector LC_ActionDimLinearBase::adjustDefPointByAdjacentDims(const RS_Vector &mouse, const RS_Vector &extPoint1, const RS_Vector &extPoint2, double ownDimLineAngle, bool forPreview){
    RS_Vector result = mouse;
    RS_Vector dirDim = RS_Vector::polar(100.0, ownDimLineAngle);
    // construction line for dimension line
    RS_ConstructionLine dimLine(nullptr, RS_ConstructionLineData(mouse,mouse + dirDim));

    // tangent line from extensionPoint2
    RS_Vector dirTan = RS_Vector::polar(100, M_PI_2 + ownDimLineAngle);
    RS_ConstructionLine dimTangentLine(nullptr, RS_ConstructionLineData(extPoint2,
                                                                        extPoint2 + dirTan));

    // check whether dim points may be aligned with neighbors
    RS_Vector ownDimP2 = dimLine.getNearestPointOnEntity(extPoint2);
    double snapRange = getSnapRange();

    // first, check for end point (closest to mouse)
    RS_Vector endSnapPoint = getAdjacentDimDimSnapPoint(ownDimP2, snapRange);
    if (endSnapPoint.valid){
        result = dimTangentLine.getNearestPointOnEntity(endSnapPoint);
        if (forPreview && m_showRefEntitiesOnPreview){
            previewRefPoint(endSnapPoint);
        }
    }
    else {
        // checking for first point (start point of this dim)
        RS_Vector ownDimP1 = dimLine.getNearestPointOnEntity(extPoint1);
        RS_Vector startSnapPoint = getAdjacentDimDimSnapPoint(ownDimP1, snapRange);
        if (startSnapPoint.valid){
            result = dimTangentLine.getNearestPointOnEntity(startSnapPoint);
            if (forPreview && m_showRefEntitiesOnPreview){
                previewRefPoint(startSnapPoint);
            }
        }
    }

#ifdef DEBUG_DIM_SNAP
    if (forPreview){
            addReferenceLineToPreview(dimTangentLine.getStartpoint(), dimTangentLine.getEndpoint());
            addReferencePointToPreview(dimLine.getStartpoint());
            addReferencePointToPreview(dimLine.getEndpoint());
            addReferenceLineToPreview(dimLine.getStartpoint(), dimLine.getEndpoint());
            RS_Vector ownDimP1 = dimLine.getNearestPointOnEntity(edata->extensionPoint1); // intentional duplicate
            addReferencePointToPreview(ownDimP1);
            addReferencePointToPreview(ownDimP2);
        }
#endif
    return result;
}

RS_Vector LC_ActionDimLinearBase::adjustByAdjacentDim(RS_Vector mouse, bool forPreview, bool altDirection){
    return adjustDefPointByAdjacentDims(mouse, getExtensionPoint1(), getExtensionPoint2(), getDimAngle(altDirection), forPreview);
}

void LC_ActionDimLinearBase::updateMouseButtonHintForExtPoint2() {
    updateMouseWidgetTRBack(tr("Specify second extension line origin"), MOD_SHIFT_ANGLE_SNAP);
}

void LC_ActionDimLinearBase::updateMouseButtonHintForDefPoint() {
    updateMouseWidgetTRBack(tr("Specify dimension line location"), MOD_SHIFT_LC(tr("Snap to Adjacent Dim")));
}

void LC_ActionDimLinearBase::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetExtPoint1:
            updateMouseWidgetTRCancel(tr("Specify first extension line origin"),MOD_SHIFT_AND_CTRL( MSG_REL_ZERO, "Select Line for origin points"));
            break;
        case SetExtPoint2:
            updateMouseButtonHintForExtPoint2();
            break;
        case SetDefPoint:
            updateMouseButtonHintForDefPoint();
            break;
        case SetText:
            updateMouseWidget(tr("Enter dimension text:"));
            break;
        case SetAngle:
            updateMouseWidget(tr("Enter dimension line angle:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
