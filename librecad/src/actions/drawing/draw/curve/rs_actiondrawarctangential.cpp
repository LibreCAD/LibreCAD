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

#include<cmath>

#include <QMouseEvent>

#include "rs_actiondrawarctangential.h"
#include "rs_arc.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"
#include "qg_arctangentialoptions.h"

RS_ActionDrawArcTangential::RS_ActionDrawArcTangential(RS_EntityContainer& container,
                                                       RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw arcs tangential",
                               container, graphicView)
    , point(false)
    , data(std::make_unique<RS_ArcData>()){
    actionType=RS2::ActionDrawArcTangential;
}

RS_ActionDrawArcTangential::~RS_ActionDrawArcTangential() = default;

void RS_ActionDrawArcTangential::reset() {
    baseEntity = nullptr;
    isStartPoint = false;
    point = RS_Vector(false);
    data = std::make_unique<RS_ArcData>();
}

void RS_ActionDrawArcTangential::init(int status) {
    RS_PreviewActionInterface::init(status);
    //reset();
}

void RS_ActionDrawArcTangential::trigger() {
    RS_PreviewActionInterface::trigger();

    if (!(point.valid && baseEntity)) {
        RS_DEBUG->print("RS_ActionDrawArcTangential::trigger: "
                        "conditions not met");
        return;
    }

    preparePreview();
    auto* arc = new RS_Arc(container, *data);
    container->addEntity(arc);
    arc->setLayerToActive();
    arc->setPenToActive();

    addToDocumentUndoable(arc);

    graphicView->redraw(RS2::RedrawDrawing);
    moveRelativeZero(arc->getCenter());

    setStatus(SetBaseEntity);
    reset();
}

void RS_ActionDrawArcTangential::preparePreview() {
    if (baseEntity && point.valid) {
        RS_Vector startPoint;
        double direction;
        if (isStartPoint) {
            startPoint = baseEntity->getStartpoint();
            direction = RS_Math::correctAngle(baseEntity->getDirection1()+M_PI);
        } else {
            startPoint = baseEntity->getEndpoint();
            direction = RS_Math::correctAngle(baseEntity->getDirection2()+M_PI);
        }

        RS_Arc arc(nullptr, RS_ArcData());
        bool suc;
        if (byRadius) {
            suc = arc.createFrom2PDirectionRadius(startPoint, point, direction, data->radius);
        } else {
            suc = arc.createFrom2PDirectionAngle(startPoint, point, direction, angleLength);
        }
        if (suc) {
            data.reset(new RS_ArcData(arc.getData()));
            if (byRadius){
                updateOptionsRadius(arc.getRadius());
            }
            else {
                updateOptionsAngle(RS_Math::rad2deg(arc.getAngleLength()));
            }
        }
    }
}

void RS_ActionDrawArcTangential::mouseMoveEvent(QMouseEvent* e) {
    int status = getStatus();
    point = snapPoint(e);
    deleteHighlights();
    switch (status){
        case SetBaseEntity: {
            RS_Entity *entity = catchEntity(e, RS2::ResolveAll);
            if (entity != nullptr){
                if (entity->isAtomic()){
                    highlightHover(entity);
                }
            }
            break;
        }
        case SetEndAngle: {
            highlightSelected(baseEntity);
            deletePreview();
            RS_Vector center;
            if (byRadius){
                if (isShift(e)){ // double check for efficiency, eliminate center forecasting calculations if not needed
                    center = forecastArcCenter();
                    point = getSnapAngleAwarePoint(e, center, point, true);
                }
            }
            preparePreview();
            if (data->isValid()){
                auto arc = previewArc(*data);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(data->center);
                    previewRefPoint(arc->getStartpoint());
                    if (byRadius) {
                        previewRefLine(data->center, point);
                        previewRefSelectablePoint(arc->getEndpoint());
                        previewRefSelectablePoint(center);
                    } else {
                        previewRefLine(data->center, arc->getEndpoint());
                        previewRefLine(data->center, arc->getStartpoint());
                        RS_Vector nearest = arc->getNearestPointOnEntity(point, false);
                        previewRefLine(data->center, point);
                        previewRefSelectablePoint(nearest);
                        RS_ArcData circleArcData = arc->getData();
                        std::swap(circleArcData.angle1, circleArcData.angle2);
                        previewRefArc(circleArcData);
                    }
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();
}

RS_Vector RS_ActionDrawArcTangential::forecastArcCenter() const{
    RS_Vector center;
    double direction;
    if (isStartPoint) {
        direction = RS_Math::correctAngle(baseEntity->getDirection1() + M_PI);
    } else {
        direction = RS_Math::correctAngle(baseEntity->getDirection2() + M_PI);
    }

    RS_Vector ortho = RS_Vector::polar(data->radius, direction + M_PI_2);
    RS_Vector center1 = arcStartPoint + ortho;
    RS_Vector center2 = arcStartPoint - ortho;
    if (center1.distanceTo(point) < center2.distanceTo(point)) {
        center = center1;
    } else {
        center = center2;
    }
    return center;
}

void RS_ActionDrawArcTangential::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        // set base entity:
        case SetBaseEntity: {
            RS_Vector coord = toGraph(e);
            RS_Entity *entity = catchEntity(coord, RS2::ResolveAll);
            if (entity){
                if (entity->isAtomic()){
                    baseEntity = dynamic_cast<RS_AtomicEntity *>(entity);
                    const RS_Vector &startPoint = baseEntity->getStartpoint();
                    const RS_Vector &endPoint = baseEntity->getEndpoint();
                    if (startPoint.distanceTo(coord) < endPoint.distanceTo(coord)){
                        isStartPoint = true;
                        arcStartPoint = startPoint;

                    } else {
                        isStartPoint = false;
                        arcStartPoint = endPoint;
                    }
                    setStatus(SetEndAngle);
                    updateMouseButtonHints();
                }
            }
            break;
        }
        case SetEndAngle: {// set angle (point that defines the angle)
            if (byRadius){
                if (isShift(e)){ // double check for efficiency, eliminate calculations if not needed
                    RS_Vector center = forecastArcCenter();
                    point = getSnapAngleAwarePoint(e, center, point);
                }
            }else {
                point = getSnapAngleAwarePoint(e, arcStartPoint, point);
            }
            fireCoordinateEvent(point);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawArcTangential::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

// fixme - more intelligent processing
void RS_ActionDrawArcTangential::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
    case SetBaseEntity:
        break;
    case SetEndAngle:
        point = mouse;
        trigger();
        break;
    default:
        break;
    }
}

void RS_ActionDrawArcTangential::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetBaseEntity:
        updateMouseWidgetTRCancel(tr("Specify base entity"));
        break;
    case SetEndAngle:
        if(byRadius) {
            updateMouseWidgetTRBack(tr("Specify end angle"), MOD_SHIFT_ANGLE_SNAP);
        } else {
            updateMouseWidgetTRBack(tr("Specify end point"));
        }
        break;
    default:
        updateMouseWidget();
        break;
    }
}

// fixme - add suport of commands?

RS2::CursorType RS_ActionDrawArcTangential::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionDrawArcTangential::setRadius(double r){
    data->radius = std::abs(r);
}

double RS_ActionDrawArcTangential::getRadius() const {
    return data->radius;
}

bool RS_ActionDrawArcTangential::getByRadius() const{
    return byRadius;
}

void RS_ActionDrawArcTangential::setByRadius(bool status) {
    byRadius=status;
}

double RS_ActionDrawArcTangential::getAngle() const {
    return angleLength;
}

void RS_ActionDrawArcTangential::setAngle(double angle) {
    angleLength = angle;
}

LC_ActionOptionsWidget* RS_ActionDrawArcTangential::createOptionsWidget(){
    return new QG_ArcTangentialOptions();
}

void RS_ActionDrawArcTangential::updateOptionsRadius(double radius){
    if (m_optionWidget != nullptr){
        auto* options = dynamic_cast <QG_ArcTangentialOptions*> (m_optionWidget.get());
        options->updateRadius(radius);
    }
}

void RS_ActionDrawArcTangential::updateOptionsAngle(double angle){
    if (m_optionWidget != nullptr){
        auto* options = dynamic_cast <QG_ArcTangentialOptions*> (m_optionWidget.get());
        options->updateAngle(angle);
    }
}
