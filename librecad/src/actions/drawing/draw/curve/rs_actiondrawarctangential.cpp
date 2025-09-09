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
#include "rs_actiondrawarctangential.h"

#include "lc_actioncontext.h"
#include "qg_arctangentialoptions.h"
#include "rs_arc.h"
#include "rs_debug.h"

RS_ActionDrawArcTangential::RS_ActionDrawArcTangential(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw arcs tangential",actionContext, RS2::ActionDrawArcTangential)
    , m_point(false)
    , m_arcData(std::make_unique<RS_ArcData>()){
}

RS_ActionDrawArcTangential::~RS_ActionDrawArcTangential() = default;

void RS_ActionDrawArcTangential::reset() {
    m_baseEntity = nullptr;
    m_isStartPoint = false;
    m_point = RS_Vector(false);
    m_arcData = std::make_unique<RS_ArcData>();
    m_alternateArc = false;
}

void RS_ActionDrawArcTangential::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPosition) {
    setBaseEntity(contextEntity, m_actionContext->getContextMenuActionClickPosition());
}

void RS_ActionDrawArcTangential::doTrigger() {
    if (!(m_point.valid && m_baseEntity)) {
        RS_DEBUG->print("RS_ActionDrawArcTangential::trigger: conditions not met");
        return;
    }

    preparePreview();
    if (m_alternateArc){
        m_arcData->reversed = !m_arcData->reversed;
    }
    auto* arc = new RS_Arc(m_container, *m_arcData);

    setPenAndLayerToActive(arc);
    moveRelativeZero(arc->getCenter());

    undoCycleAdd(arc);

    setStatus(SetBaseEntity);
    double oldRadius = m_arcData->radius;

    reset();
    if (m_byRadius) {
        m_arcData->radius = oldRadius;
    }
}

bool RS_ActionDrawArcTangential::doUpdateAngleByInteractiveInput(const QString& tag, double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

bool RS_ActionDrawArcTangential::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void RS_ActionDrawArcTangential::preparePreview() {
    if (m_baseEntity && m_point.valid) {
        RS_Vector startPoint;
        double direction;
        if (m_isStartPoint) {
            startPoint = m_baseEntity->getStartpoint();
            direction = RS_Math::correctAngle(m_baseEntity->getDirection1()+M_PI);
        } else {
            startPoint = m_baseEntity->getEndpoint();
            direction = RS_Math::correctAngle(m_baseEntity->getDirection2()+M_PI);
        }

        RS_Arc arc(nullptr, RS_ArcData());
        bool suc;
        if (m_byRadius) {
            suc = arc.createFrom2PDirectionRadius(startPoint, m_point, direction, m_arcData->radius);
        } else {
            suc = arc.createFrom2PDirectionAngle(startPoint, m_point, direction, m_angleLength);
        }
        if (suc) {
            m_arcData.reset(new RS_ArcData(arc.getData()));
            if (!m_byRadius){
                updateOptionsRadius(arc.getRadius());
            }
            else {
                updateOptionsAngle(RS_Math::rad2deg(arc.getAngleLength()));
            }
        }
    }
}

void RS_ActionDrawArcTangential::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    m_point = e->snapPoint;
    switch (status){
        case SetBaseEntity: {
            deleteSnapper();
            auto entity = catchAndDescribe(e, RS2::ResolveAll);
            if (isAtomic(entity)){
                highlightHover(entity);
            }
            break;
        }
        case SetEndAngle: {
            highlightSelected(m_baseEntity);
            if (m_byRadius){
                if (e->isShift){ // double check for efficiency, eliminate center forecasting calculations if not needed
                    auto center = forecastArcCenter();
                    m_point = getSnapAngleAwarePoint(e, center, m_point, true);
                }
            }
            preparePreview();
            if (m_arcData->isValid()){
                RS_Arc* arc;
                bool alternateArcMode = e->isControl;
                if (alternateArcMode) {
                    auto tmpArcData = *m_arcData;
                    tmpArcData.reversed = !m_arcData->reversed;
                    arc = previewToCreateArc(tmpArcData);
                }
                else{
                    arc = previewToCreateArc(*m_arcData);
                }
                if (m_showRefEntitiesOnPreview) {
                    auto &center = m_arcData->center;
                    const auto &startPoint = arc->getStartpoint();
                    previewRefPoint(center);
                    previewRefPoint(startPoint);
                    if (m_byRadius) {
                        previewRefLine(center, m_point);
                        previewRefSelectablePoint(arc->getEndpoint());
                        previewRefSelectablePoint(center);
                    } else {
                        previewRefLine(center, arc->getEndpoint());
                        previewRefLine(center, startPoint);
                        auto nearest = arc->getNearestPointOnEntity(m_point, false);
                        previewRefLine(center, m_point);
                        previewRefSelectablePoint(nearest);
                        auto circleArcData = arc->getData();
                        std::swap(circleArcData.angle1, circleArcData.angle2);
                        previewRefArc(circleArcData);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

RS_Vector RS_ActionDrawArcTangential::forecastArcCenter() const{
    RS_Vector center;
    double direction;
    if (m_isStartPoint) {
        direction = RS_Math::correctAngle(m_baseEntity->getDirection1() + M_PI);
    } else {
        direction = RS_Math::correctAngle(m_baseEntity->getDirection2() + M_PI);
    }

    RS_Vector ortho = RS_Vector::polar(m_arcData->radius, direction + M_PI_2);
    RS_Vector center1 = m_arcStartPoint + ortho;
    RS_Vector center2 = m_arcStartPoint - ortho;
    if (center1.distanceTo(m_point) < center2.distanceTo(m_point)) {
        center = center1;
    } else {
        center = center2;
    }
    return center;
}

void RS_ActionDrawArcTangential::setBaseEntity(RS_Entity* entity, RS_Vector coord) {
    if (isAtomic(entity)) {
        m_baseEntity = dynamic_cast<RS_AtomicEntity*>(entity);
        const RS_Vector& startPoint = m_baseEntity->getStartpoint();
        const RS_Vector& endPoint = m_baseEntity->getEndpoint();
        if (startPoint.distanceTo(coord) < endPoint.distanceTo(coord)) {
            m_isStartPoint = true;
            m_arcStartPoint = startPoint;
        }
        else {
            m_isStartPoint = false;
            m_arcStartPoint = endPoint;
        }
        setStatus(SetEndAngle);
        updateMouseButtonHints();
    }
}

void RS_ActionDrawArcTangential::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        // set base entity:
        case SetBaseEntity: {
            RS_Vector coord = e->graphPoint;
            RS_Entity *entity = RS_Snapper::catchEntity(coord, RS2::ResolveAll);
            setBaseEntity(entity, coord);
            invalidateSnapSpot();
            break;
        }
        case SetEndAngle: {// set angle (point that defines the angle)
            if (m_byRadius){
                if (e->isShift){ // double check for efficiency, eliminate calculations if not needed
                    RS_Vector center = forecastArcCenter();
                    m_point = getSnapAngleAwarePoint(e, center, m_point);
                }
            }else {
                m_point = getSnapAngleAwarePoint(e, m_arcStartPoint, m_point);
            }
            m_alternateArc = e->isControl;
            fireCoordinateEvent(m_point);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawArcTangential::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

// fixme - more intelligent processing
void RS_ActionDrawArcTangential::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
    case SetBaseEntity:
        break;
    case SetEndAngle:
        m_point = mouse;
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
        if(m_byRadius) {
            updateMouseWidgetTRBack(tr("Specify end angle"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternate arc")));
        } else {
            updateMouseWidgetTRBack(tr("Specify end point"), MOD_CTRL(tr("Alternate Arc")));
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
    m_arcData->radius = std::abs(r);
}

double RS_ActionDrawArcTangential::getRadius() const {
    return m_arcData->radius;
}

bool RS_ActionDrawArcTangential::getByRadius() const{
    return m_byRadius;
}

void RS_ActionDrawArcTangential::setByRadius(bool status) {
    m_byRadius=status;
}

double RS_ActionDrawArcTangential::getAngle() const {
    return m_angleLength;
}

void RS_ActionDrawArcTangential::setAngle(double angle) {
    m_angleLength = angle;
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
