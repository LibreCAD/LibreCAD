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

#include "lc_actiondrawdimbaseline.h"
#include "rs_dimlinear.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_debug.h"
#include "rs_dimaligned.h"
#include "qg_dimoptions.h"
#include "rs_actiondimension.h"

// some functions are duplicated with DimLiner action, however, that's intentional as later we can support angular dimensions in additional to linear ones
LC_ActionDrawDimBaseline::LC_ActionDrawDimBaseline(RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType type)
       :LC_ActionDimLinearBase(type == RS2::ActionDimBaseline? "Draw Dim Baseline": "Draw Dim Continue", container, graphicView),
       edata(std::make_unique<RS_DimLinearData>(RS_Vector(0., 0.), RS_Vector(0., 0.), 0, 0.)){
    actionType = type;
}

namespace {
    //list of entity types supported by current action - line, arc, circle
    const auto dimEntityTypes = EntityTypeList{RS2::EntityDimLinear, RS2::EntityDimAligned};
}

void LC_ActionDrawDimBaseline::reset(){
    RS_ActionDimension::reset();

    double oldAngle = edata->angle; // keep selected angle
    *edata = {{}, {}, oldAngle, 0.0};
}

void LC_ActionDrawDimBaseline::trigger() {
    RS_ActionDimension::trigger();

    preparePreview();
    auto *dim = createDim(container);
    dim->setLayerToActive();
    dim->setPenToActive();
    dim->update();
    container->addEntity(dim);

    addToDocumentUndoable(dim);

    graphicView->redraw(RS2::RedrawDrawing);

    if (isBaseline()) {
        moveRelativeZero(edata->extensionPoint1);
        prevExtensionPointEnd = edata->extensionPoint2;
        // test is just in case
        auto* dimLinear = dynamic_cast<RS_DimLinear*>(dim);
        if (dimLinear != nullptr) {
            baseDefPoint = dimLinear->getDefinitionPoint();
        }
    }
    else{
        moveRelativeZero(edata->extensionPoint2);
        edata->extensionPoint1 = edata->extensionPoint2;
        prevExtensionPointEnd = edata->extensionPoint1; // todo - check whether this is necessary. Potentially - for ordnance continued
    }
    baseDefPoint = data->definitionPoint;

}

RS_Entity *LC_ActionDrawDimBaseline::createDim(RS_EntityContainer* parent){
    auto *dim = new RS_DimLinear(parent, *data, *edata);
    return dim;
}

bool LC_ActionDrawDimBaseline::isBaseline(){
    return actionType == RS2::ActionDimBaseline;
}

void LC_ActionDrawDimBaseline::mouseMoveEvent(QMouseEvent *e) {
    int status = getStatus();
    RS_Vector mouse = snapPoint(e); // snap on entity?
    deletePreview();
    switch (status){
        case SetExtPoint1: {
            deleteHighlights();
            auto dimCandidate = RS_Snapper::catchEntity(mouse, dimEntityTypes, RS2::ResolveNone);
            if (dimCandidate != nullptr) {
                highlightHover(dimCandidate);

                RS_Vector extPoint1;
                RS_Vector extPoint2;
                RS_Vector dim1;
                RS_Vector dim2;

                int rtti = dimCandidate->rtti();
                switch (rtti){
                    case RS2::EntityDimLinear:{
                        auto dimLinear = dynamic_cast<RS_DimLinear *>(dimCandidate);
                        extPoint1 = dimLinear->getExtensionPoint1();
                        extPoint2 = dimLinear->getExtensionPoint2();
                        dimLinear->getDimPoints(dim1, dim2);
                        break;
                    }
                    case RS2::EntityDimAligned:{
                        auto dimAligned = dynamic_cast<RS_DimAligned *>(dimCandidate);
                        extPoint1 = dimAligned->getExtensionPoint1();
                        extPoint2 = dimAligned->getExtensionPoint2();
                        dimAligned->getDimPoints(dim1, dim2);
                        break;
                    }
                    default:
                        break;
                }
                // we define which dim point is closer to mouse - based on this, extension 1 or extension 2 point will be selected as start extension point
                double dist1 = mouse.distanceTo(dim1);
                double dist2 = mouse.distanceTo(dim2);

                bool dim1CloserToMouse = dist1 < dist2;
                if (isControl(e)){
                    dim1CloserToMouse = !dim1CloserToMouse;
                }
                if (dim1CloserToMouse){
                    previewRefSelectablePoint(extPoint1);
                }
                else {
                    previewRefSelectablePoint(extPoint2);
                }
            }
            drawHighlights();
            break;
        }
        case SetExtPoint2:{
            const RS_Vector &extPoint1 = getExtensionPoint1();
            if (extPoint1.valid){
                mouse = getSnapAngleAwarePoint(e, extPoint1, mouse, true);

                // make a projection for new def point
                // vector in direction controlled by angle of dimension
                const RS_Vector &dimVector = RS_Vector::polar(100.0, edata->angle);
                // infinite line from extPoint1 to angle direction
                RS_ConstructionLine dimLine(nullptr, RS_ConstructionLineData(extPoint1, extPoint1 + dimVector));
                // projection of mouse on infinite line
                RS_Vector ext2Candidate = dimLine.getNearestPointOnEntity(mouse, false);
                // projection of previous definition point on infinite line
                RS_Vector originalDefPointProjection = dimLine.getNearestPointOnEntity(baseDefPoint, false);
                // distance vector between projection of old definition point projection and mouse projections
                RS_Vector defPointsDistance = baseDefPoint - originalDefPointProjection;
                // coordinate of definition point on base dimension line
                RS_Vector newDefPointInline = ext2Candidate + defPointsDistance;
                RS_Vector newDefPoint;

                // complete calculation of new definition point for dimension
                if (isBaseline()) {
                    // for base line, we need to offset definition point on infinite line that corresponds old dim line to specified distance
                    double dimAngle = dimDirectionAngle; // angle we use for offset vector - it's the same as angle from base dimension
                    if (isControl(e)) { // swap direction of the offset, if needed, so we'll offset in opposite direction
                        dimAngle = M_PI + dimAngle;
                    }
                    double previewDistance = baselineDistance;
                    if (freeBaselineDistance){
                        previewDistance = 16; // of it's better to use zero there? Just a placeholder for preview
                    }
                    // find new definition point for this dimension
                    newDefPoint = newDefPointInline + RS_Vector::polar(previewDistance, dimAngle);
                } else {
                    // for continue mode, new definition point will be on the same line as previous definition point
                    newDefPoint = newDefPointInline;
                }

                setExtensionPoint2(mouse);
                data->definitionPoint = newDefPoint;

                if (previewShowsFullDimension) {
                    auto dim = dynamic_cast<RS_DimLinear *>(createDim(preview.get()));
                    dim->update();
                    previewEntity(dim);
                }

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(extPoint1);
                    previewRefSelectablePoint(newDefPoint);
                    previewRefLine(extPoint1, mouse);
                    previewRefPoint(newDefPoint);
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
                mouse = adjustByAdjacentDim(mouse, true);

                const RS_Vector &dimVector = RS_Vector::polar(100.0, dimDirectionAngle);
                // infinite line in normal direction to dimension angle
                RS_ConstructionLine dimLine(nullptr, RS_ConstructionLineData(extPoint1, extPoint1 + dimVector));

                // projection of old definition point on infinite line
                RS_Vector originalDefPointProjection = dimLine.getNearestPointOnEntity(baseDefPoint, false);
                // projection of mouse on infinite line
                RS_Vector newDefPointProjection = dimLine.getNearestPointOnEntity(mouse, false);

                // distance between projections of old definition point and new one
                currentDistance = originalDefPointProjection.distanceTo(newDefPointProjection);
                updateOptionsUI(QG_DimOptions::UI_UPDATE_BASELINE_DISTANCE);

                data->definitionPoint = mouse;
                preparePreview();
                previewRefSelectablePoint(data->definitionPoint);
                previewRefPoint(extPoint1);
                previewRefPoint(extPoint2);
                RS_Entity* dim = createDim(preview.get());
                previewEntity(dim);
                dim->update();
                break;
            }
            break;
        }
        default:
            break;

    }
    drawPreview();
}

void LC_ActionDrawDimBaseline::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status){
        case SetExtPoint1:
        case SetExtPoint2:{
            alternateMode = isControl(e);
            break;
        }
        case SetDefPoint: {
            snap = getFreeSnapAwarePoint(e, snap);
            snap = adjustByAdjacentDim(snap, false);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawDimBaseline::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status){
        case SetExtPoint1: {
            auto dimCandidate = RS_Snapper::catchEntity(mouse, dimEntityTypes, RS2::ResolveNone);
            if (dimCandidate != nullptr) {
                RS_Vector extPoint1;
                RS_Vector extPoint2;
                RS_Vector dim1;
                RS_Vector dim2;
                double dimAngle;

                int rtti = dimCandidate->rtti();
                switch (rtti){
                    case RS2::EntityDimLinear:{
                        auto dimLinear = dynamic_cast<RS_DimLinear *>(dimCandidate);
                        extPoint1 = dimLinear->getExtensionPoint1();
                        extPoint2 = dimLinear->getExtensionPoint2();
                        dimLinear->getDimPoints(dim1, dim2);
                        baseDefPoint = dimLinear->getDefinitionPoint();
                        dimAngle = dimLinear->getAngle();
                        dimDirectionAngle = extPoint1.angleTo(dim1);
                        break;
                    }
                    case RS2::EntityDimAligned:{
                        auto dimAligned = dynamic_cast<RS_DimAligned *>(dimCandidate);
                        extPoint1 = dimAligned->getExtensionPoint1();
                        extPoint2 = dimAligned->getExtensionPoint2();
                        dimAligned->getDimPoints(dim1, dim2);
                        baseDefPoint = dimAligned->getDefinitionPoint();
                        dimAngle = extPoint1.angleTo(extPoint2);
                        dimDirectionAngle = extPoint2.angleTo(dim2);
                        break;
                    }
                    default:
                        dimAngle = 0.0; // just to avoid warning
                        break;
                }

                double dist1 = mouse.distanceTo(dim1);
                double dist2 = mouse.distanceTo(dim2);

                bool dim1CloserToMouse = dist1 < dist2;
                if (alternateMode){
                    dim1CloserToMouse = !dim1CloserToMouse;
                }
                if (dim1CloserToMouse){
                    edata->extensionPoint1 = extPoint1;
                    prevExtensionPointEnd = extPoint2;
                } else {
                    edata->extensionPoint1 = extPoint2;
                    prevExtensionPointEnd = extPoint1;
                }

                edata->angle =  dimAngle;
                moveRelativeZero(edata->extensionPoint1);
                setStatus(SetExtPoint2);
            }
            break;
        }
        case SetExtPoint2:{
            const RS_Vector &extPoint1 = getExtensionPoint1();
            const RS_Vector &dimVector = RS_Vector::polar(100.0, edata->angle);
            RS_ConstructionLine dimLine(nullptr, RS_ConstructionLineData(extPoint1, extPoint1 + dimVector));
            RS_Vector ext2Candidate = dimLine.getNearestPointOnEntity(mouse, false);
            RS_Vector originalDefPointProjection = dimLine.getNearestPointOnEntity(baseDefPoint, false);
            RS_Vector defPointsDistance = baseDefPoint - originalDefPointProjection;

            RS_Vector newDefPoint = ext2Candidate + defPointsDistance;
            if (isBaseline()) {
                double dimAngle = dimDirectionAngle;

                if (alternateMode){
                    dimAngle = M_PI+dimAngle;
                }
                newDefPoint = newDefPoint +RS_Vector::polar(baselineDistance, dimAngle);
            }
            data->definitionPoint = newDefPoint;

            if (freeBaselineDistance && isBaseline()){
                currentDistance = 0.0;
                setStatus(SetDefPoint);
            }
            else{
                trigger();
                setStatus(SetExtPoint2);
            }
            break;
        }
        case SetDefPoint:{
            data->definitionPoint = mouse;
            trigger();
            setStatus(SetExtPoint2);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawDimBaseline::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetText: {
            setText(c);
            updateOptions();
            graphicView->enableCoordinateInput();
            setStatus(lastStatus);
            accept = true;
            break;
        }
        default:
            lastStatus = (Status) getStatus();
            deletePreview();
            if (checkCommand("text", c)) {
                graphicView->disableCoordinateInput();
                setStatus(SetText);
                accept = true;
            }
            break;
    }
    return accept;
}

QStringList LC_ActionDrawDimBaseline::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetExtPoint1:
        case SetExtPoint2:
        case SetDefPoint: {
            cmd += command("text");
            break;
        }
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawDimBaseline::updateMouseButtonHints() {
    int status = getStatus();
    switch (status) {
        case SetExtPoint1:
            updateMouseWidgetTRCancel(tr("Select base linear/aligned dimension"), MOD_CTRL("Select distant extension point"));
            break;
        case SetExtPoint2:
            updateMouseWidgetTRBack(tr("Specify second extension line origin"), isBaseline() && !freeBaselineDistance ? MOD_CTRL("Mirror offset direction"): MOD_NONE);
            break;
        case SetDefPoint:
            updateMouseWidgetTRBack(tr("Specify dimension line location"), MOD_SHIFT(tr("Snap to Adjacent Dim")));
            break;
        case SetText:
            updateMouseWidget(tr("Enter dimension text:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

bool LC_ActionDrawDimBaseline::isFreeBaselineDistance() const {
    return freeBaselineDistance;
}

void LC_ActionDrawDimBaseline::setFreeBaselineDistance(bool freeDistance) {
    this->freeBaselineDistance = freeDistance;
    if (!freeBaselineDistance && getStatus() == SetDefPoint){
        setStatus(SetExtPoint2);
    }
}

double LC_ActionDrawDimBaseline::getBaselineDistance() const {
    return baselineDistance;
}

void LC_ActionDrawDimBaseline::setBaselineDistance(double distance) {
    LC_ActionDrawDimBaseline::baselineDistance = distance;
}

double LC_ActionDrawDimBaseline::getCurrentBaselineDistance() const {
    return currentDistance;
}

RS_Vector LC_ActionDrawDimBaseline::getExtensionPoint1(){
    return edata->extensionPoint1;
}

RS_Vector LC_ActionDrawDimBaseline::getExtensionPoint2(){
    return edata->extensionPoint2;
}

double LC_ActionDrawDimBaseline::getDimAngle(){
    return edata->angle;
}

void LC_ActionDrawDimBaseline::setExtensionPoint1(RS_Vector p){
    edata->extensionPoint1 = p;
}

void LC_ActionDrawDimBaseline::setExtensionPoint2(RS_Vector p){
    edata->extensionPoint2 = p;
}

void LC_ActionDrawDimBaseline::preparePreview() {
    RS_Vector dirV = RS_Vector::polar(100., edata->angle + M_PI_2);
    RS_ConstructionLine cl(nullptr,RS_ConstructionLineData(edata->extensionPoint2,edata->extensionPoint2 + dirV));
    data->definitionPoint = cl.getNearestPointOnEntity(data->definitionPoint);
}
