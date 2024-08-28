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

#include "rs_dimaligned.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "rs_dimlinear.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_coordinateevent.h"
#include "rs_constructionline.h"
#include <QMouseEvent>
#include "lc_actiondimlinearbase.h"
#include "rs_settings.h"
#include "rs_actiondimension.h"

LC_ActionDimLinearBase::LC_ActionDimLinearBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView):
   RS_ActionDimension(name, container,  graphicView){
}

LC_ActionDimLinearBase::~LC_ActionDimLinearBase() = default;

#define DEBUG_DIM_SNAP_NO

namespace {
    //list of entity types supported by current action - line, arc, circle
    const auto dimEntityTypes = EntityTypeList{RS2::EntityDimLinear, RS2::EntityDimAligned};
}

void LC_ActionDimLinearBase::trigger(){
    RS_ActionDimension::trigger();

    preparePreview();
    auto *dim = createDim(container);
    dim->setLayerToActive();
    dim->setPenToActive();
    dim->update();
    container->addEntity(dim);

    addToDocumentUndoable(dim);

    graphicView->redraw(RS2::RedrawDrawing);

    RS_DEBUG->print("LC_ActionDimLinearBase::trigger():"
                    " dim added: %lu", dim->getId());
}

void LC_ActionDimLinearBase::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetExtPoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetExtPoint2: {
            const RS_Vector &extPoint1 = getExtensionPoint1();
            if (extPoint1.valid){
                deletePreview();
                mouse = getSnapAngleAwarePoint(e, extPoint1, mouse, true);

                data->definitionPoint = mouse;
                setExtensionPoint2(mouse);

                if (showRefEntitiesOnPreview) {
                    previewRefLine(extPoint1, mouse);
                    previewRefSelectablePoint(mouse);
                }
                if (previewShowsFullDimension) {
                    preparePreview();
                    RS_Entity *dim = createDim(preview.get());
                    dim->update();
                    previewEntity(dim);
                }
                else if (showRefEntitiesOnPreview) {
                    previewRefLine(extPoint1, mouse);
                }
                else{
                    previewLine(extPoint1, mouse);
                }

                drawPreview();
            }
            break;
        }
        case SetDefPoint:{
            const RS_Vector &extPoint1 = getExtensionPoint1();
            const RS_Vector &extPoint2 = getExtensionPoint2();
            if (extPoint1.valid && extPoint2.valid){
                deletePreview();
                // less restrictive snap
                mouse = getFreeSnapAwarePoint(e, mouse);
                mouse = adjustByAdjacentDim(mouse, true);
                data->definitionPoint = mouse;
                preparePreview();
                RS_Entity* dim = createDim(preview.get());
                dim->update();
                previewEntity(dim);
#ifdef DEBUG_DIM_SNAP
                RS_Vector p1;
                RS_Vector p2;
                dim->getDimPoints(p1, p2);

                addReferencePointToPreview(p1);
                addReferencePointToPreview(p2);
#endif
                drawPreview();
                break;
            }
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDimLinear::mouseMoveEvent end");
}

void LC_ActionDimLinearBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    if (status == SetExtPoint2){
        snap = getSnapAngleAwarePoint(e, getExtensionPoint1(), snap);
    }
    else if (status == SetDefPoint){
        // less restrictive snap
        snap = getFreeSnapAwarePoint(e, snap);
        snap = adjustByAdjacentDim(snap, false);
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDimLinearBase::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDimLinearBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetExtPoint1:
            setExtensionPoint1(pos);
            moveRelativeZero(pos);
            setStatus(SetExtPoint2);
            break;

        case SetExtPoint2:
            setExtensionPoint2(pos);
            moveRelativeZero(pos);
            setStatus(SetDefPoint);
            break;

        case SetDefPoint:
            data->definitionPoint = pos;
            trigger();
            reset();
            setStatus(SetExtPoint1);
            break;

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
        if (forPreview && showRefEntitiesOnPreview){
            previewRefPoint(endSnapPoint);
        }
    }
    else {
        // checking for first point (start point of this dim)
        RS_Vector ownDimP1 = dimLine.getNearestPointOnEntity(extPoint1);
        RS_Vector startSnapPoint = getAdjacentDimDimSnapPoint(ownDimP1, snapRange);
        if (startSnapPoint.valid){
            result = dimTangentLine.getNearestPointOnEntity(startSnapPoint);
            if (forPreview && showRefEntitiesOnPreview){
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

RS_Vector LC_ActionDimLinearBase::adjustByAdjacentDim(RS_Vector mouse, bool forPreview){
    return adjustDefPointByAdjacentDims(mouse, getExtensionPoint1(), getExtensionPoint2(), getDimAngle(), forPreview);
}

void LC_ActionDimLinearBase::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetExtPoint1:
            updateMouseWidgetTRCancel(tr("Specify first extension line origin"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetExtPoint2:
            updateMouseWidgetTRBack(tr("Specify second extension line origin"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetDefPoint:
            updateMouseWidgetTRBack(tr("Specify dimension line location"), MOD_SHIFT(tr("Snap to Adjacent Dim")));
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
