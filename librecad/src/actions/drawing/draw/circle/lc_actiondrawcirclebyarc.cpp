/****************************************************************************
**
* Action that creates a circle by given arc or ellipse by ellipse arc

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
#include <QMouseEvent>
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "lc_linemath.h"
#include "lc_actiondrawcirclebyarc.h"
#include "lc_circlebyarcoptions.h"

// Command line support - potentially, it's possible to use coordinates for selection of arc - yet it seems it is overkill,
// selection by mouse is more convenient so do nothing there

LC_ActionDrawCircleByArc::LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView):
    LC_AbstractActionWithPreview("Circle By Arc", container, graphicView){
    actionType = RS2::ActionDrawCircleByArc;
}

LC_ActionDrawCircleByArc::~LC_ActionDrawCircleByArc() = default;

// support of trigger on init functions (so on invocation, we'll check for selection and create circles for selected arcs)
bool LC_ActionDrawCircleByArc::doCheckMayTriggerOnInit(int status){
    return status == SetArc;
}

bool LC_ActionDrawCircleByArc::isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity){
    // here we'll accept only selected arcs or ellipse arcs
    int rtti = pEntity->rtti();
    bool result = rtti == RS2::EntityArc;
    if (!result){
        if (rtti == RS2::EntityEllipse){
            auto* ellipse = dynamic_cast<RS_Ellipse*>(pEntity);
            result = ellipse->isEllipticArc();
        }
    }
    return result;
}

void LC_ActionDrawCircleByArc::doPerformOriginalEntitiesDeletionOnInitTrigger(QList<RS_Entity *> &list){
    if (replaceArcByCircle){
        for (auto e: list){
            deleteOriginalArcOrEllipse(e);
        }
    }
}

// trigger support
bool LC_ActionDrawCircleByArc::doCheckMayTrigger(){
    return entity != nullptr;
}

RS_Vector LC_ActionDrawCircleByArc::doGetRelativeZeroAfterTrigger(){
    // for normal trigger, we'll position relative zero to center point
    return entity->getCenter();
}

void LC_ActionDrawCircleByArc::doAfterTrigger(){
    entity = nullptr;
    setStatus(SetArc);
}

void LC_ActionDrawCircleByArc::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    doCreateEntitiesOnTrigger(entity, list);
}

bool LC_ActionDrawCircleByArc::isSetActivePenAndLayerOnTrigger(){
    return false; // custom implementation in this action will take care of pen and layer
}

// creation of circle or ellipse for given entity
void LC_ActionDrawCircleByArc::doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list) {
    int rtti = en->rtti();
    switch (rtti) {
        case RS2::EntityArc: {
            // prepare data for cycle based on arc
            auto* arc = dynamic_cast<RS_Arc*>(en);
            RS_CircleData circleData = createCircleData(arc);
            // setup new circle
            RS_Entity* circle = new RS_Circle(container, circleData);
            // apply attributes
            applyPenAndLayerBySourceEntity(arc, circle, penMode, layerMode);
            list << circle;
            break;
        }
        case RS2::EntityEllipse: {
            // prepare data for cycle based on arc
            auto* ellipseArc = dynamic_cast<RS_Ellipse*>(en);
            RS_EllipseData ellipseData = createEllipseData(ellipseArc);
            // setup new circle
            auto ellipse = new RS_Ellipse(container, ellipseData);
            // apply attributes
            applyPenAndLayerBySourceEntity(ellipseArc, ellipse, penMode, layerMode);
            list << ellipse;
            break;
        }
        default:
            break;
    }
}

// deletion of original entities, if needed
void LC_ActionDrawCircleByArc::performTriggerDeletions(){
    // check whether we need to delete original arc
    if (replaceArcByCircle){
        deleteOriginalArcOrEllipse(entity);
    }
}

void LC_ActionDrawCircleByArc::deleteOriginalArcOrEllipse(RS_Entity *en){
    if (checkMayExpandEntity(en,  en->is(RS2::EntityArc) ? "Arc":"Ellipse")){
        deleteEntityUndoable(en);
    }
}

RS_CircleData LC_ActionDrawCircleByArc::createCircleData(RS_Arc* arc){
    RS_Vector center = arc->getCenter();
    double radius = arc->getRadius();
    if (!replaceArcByCircle){
        // adjust radius to specified shift if necessary
        if (LC_LineMath::isMeaningful(radiusShift)){
            radius = radius + radiusShift;
        }
    }
    RS_CircleData result = RS_CircleData(center, radius);
    return result;
}

RS_EllipseData LC_ActionDrawCircleByArc::createEllipseData(RS_Ellipse *ellipseArc){
    RS_EllipseData originalData = ellipseArc->getData();

    RS_EllipseData result;
    result.center = originalData.center;
    result.reversed = originalData.reversed;

    RS_Vector majorP = originalData.majorP;

    if (!replaceArcByCircle){
        // adjust major axis of ellipse for necessary shift
        if (LC_LineMath::isMeaningful(radiusShift)){
            majorP = majorP.relative(radiusShift, ellipseArc->getAngle());
        }
    }

    result.majorP = majorP;
    result.ratio = originalData.ratio;
    result.angle1 = 0.0;
    result.angle2 = 0.0;
    return result;
}

bool LC_ActionDrawCircleByArc::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status == SetArc;
}

void LC_ActionDrawCircleByArc::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    RS_Entity *en = catchEntity(e, circleType, RS2::ResolveAll);
    if (en != nullptr){
        highlightHover(en);
        int rtti = en->rtti();
        bool isArc = (en->isArc() && rtti == RS2::EntityArc);
        if (isArc){
            auto *arc = dynamic_cast<RS_Arc *>(en);

            RS_CircleData circleData = createCircleData(arc);
            RS_Entity *circle = new RS_Circle(container, circleData);
            list << circle;

            if (showRefEntitiesOnPreview) {
                createRefPoint(circleData.center, list);
            };

            entity = arc;
        } else {
            if (rtti == RS2::EntityEllipse){
                auto *ellipseArc = dynamic_cast<RS_Ellipse *>(en);

                if (ellipseArc->isEllipticArc()){
                    RS_EllipseData ellipseData = createEllipseData(ellipseArc);
                    auto ellipse = new RS_Ellipse(container, ellipseData);
                    list << ellipse;

                    if (showRefEntitiesOnPreview) {
                        createRefPoint(ellipse->getCenter(), list);
                    }
                    entity = ellipseArc;
                }
            }
        }
    } else {
        entity = nullptr;
    }
}

void LC_ActionDrawCircleByArc::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status,[[maybe_unused]] const RS_Vector &snapPoint){
    // just trigger on entity selection
    if (status == SetArc){
        trigger();
    }
}


void LC_ActionDrawCircleByArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetArc:
            updateMouseWidgetTRCancel(tr("Select arc or ellipse arc"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleByArc::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawCircleByArc::createOptionsWidget(){
    return new LC_CircleByArcOptions();
}

void LC_ActionDrawCircleByArc::setReplaceArcByCircle(bool value){
    replaceArcByCircle = value;
}
