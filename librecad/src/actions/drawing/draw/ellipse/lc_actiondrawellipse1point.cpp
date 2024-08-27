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

#include "rs_graphicview.h"
#include "rs_math.h"
#include "lc_actiondrawellipse1point.h"
#include "lc_ellipse1pointoptions.h"
#include "lc_linemath.h"

struct LC_ActionDrawEllipse1Point::Points {
/** Center of ellipse */
    RS_Vector center = RS_Vector(false);
    double  majorRadius = 1.0;
    double minorRadius = 1.0;
    bool hasAngle = false;
    bool freeAngle = false;
    double majorRadiusAngle = 0.0;
    double angle1 = 0.;
    double angle2 = 0.;
    bool isArc = false;
    bool reversed = false;

    double getMajorAngle(){
        return hasAngle ? majorRadiusAngle : 0.0;
    }

    double getRatio(){
        return minorRadius / majorRadius;
    }

    RS_Vector getMajorP(){
        return RS_Vector::polar(majorRadius, getMajorAngle());
    }
};

LC_ActionDrawEllipse1Point::LC_ActionDrawEllipse1Point(RS_EntityContainer &container, RS_GraphicView &graphicView,   bool isArc)
    :LC_ActionDrawCircleBase("Draw ellipse by 1 point", container, graphicView),
    pPoints(std::make_unique<Points>()){
    pPoints->isArc = isArc;
    pPoints->angle2 = isArc ? 2. * M_PI : 0.;
    actionType = isArc ? RS2::ActionDrawEllipseArc1Point : RS2::ActionDrawEllipse1Point;
}

void LC_ActionDrawEllipse1Point::init(int status) {
    LC_ActionDrawCircleBase::init(status);
}

LC_ActionDrawEllipse1Point::~LC_ActionDrawEllipse1Point() = default;

void LC_ActionDrawEllipse1Point::trigger() {

    double ratio = pPoints->getRatio();
    auto *ellipse = new RS_Ellipse{container,
                                   {pPoints->center, pPoints->getMajorP(), ratio,
                                    pPoints->angle1, pPoints->angle2, pPoints->reversed}
    };
    // todo - code belos is similar to DrawEllipseAxis action.. should we make it common for all ellipse actions?
    if (ratio > 1.){
        ellipse->switchMajorMinor();
    }
    ellipse->setLayerToActive();
    ellipse->setPenToActive();

    container->addEntity(ellipse);

    addToDocumentUndoable(ellipse);

    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        rz = ellipse->getCenter();
    }
    moveRelativeZero(rz);
    drawSnapper();

    setStatus(SetPoint);

}

void LC_ActionDrawEllipse1Point::mouseMoveEvent(QMouseEvent *e) {
    int status = getStatus();
    RS_Vector mouse = snapPoint(e);
    deletePreview();
    switch (status){
        case SetPoint:{
            if (!trySnapToRelZeroCoordinateEvent(e)){
                auto *ellipse = previewEllipse({mouse, pPoints->getMajorP(), pPoints->getRatio(), 0.0,
                                                      pPoints->isArc ? 2. * M_PI : 0., false});
                if (showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewEllipseReferencePoints(ellipse, true, true, mouse);
                }
            }
            break;
        }
        case SetMajorAngle: {
            mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);
            pPoints->majorRadiusAngle = pPoints->center.angleTo(mouse);
            auto ellipse = previewEllipse({pPoints->center, pPoints->getMajorP(), pPoints->getRatio(), 0.0,
                            pPoints->isArc ? 2. * M_PI : 0., false});
            if (showRefEntitiesOnPreview){
                previewRefSelectablePoint(mouse);
                previewRefPoint(pPoints->center);
                previewRefLine(pPoints->center, mouse);
                previewEllipseReferencePoints(ellipse, true, false, mouse);
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);

            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(pPoints->center, -pPoints->majorRadiusAngle);
            }
            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->getRatio();
            pPoints->angle1 = v.angle();

            previewRefLine(pPoints->center, mouse);

            auto ellipse = previewEllipse({pPoints->center, pPoints->getMajorP(), pPoints->getRatio(),
                                           pPoints->angle1, pPoints->angle1 + 1.0, pPoints->reversed});

            if (showRefEntitiesOnPreview) {
                previewRefPoint(pPoints->center);
                previewRefSelectablePoint(ellipse->getStartpoint());
                previewEllipseReferencePoints(ellipse, false, true, mouse);
            }
            break;
        }
        case SetAngle2: {
            //angle2 = center.angleTo(mouse);
            mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);

            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(pPoints->center, -pPoints->getMajorAngle());
            }

            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->getRatio();
            pPoints->angle2 = v.angle(); // + m_vMajorP.angle();

            auto ellipse = previewEllipse({pPoints->center, pPoints->getMajorP(), pPoints->getRatio(),
                                           pPoints->angle1, pPoints->angle2, pPoints->reversed});

            if (showRefEntitiesOnPreview) {
                previewRefLine(pPoints->center, mouse);
                previewRefPoint(pPoints->center);
                auto point = pPoints->center + RS_Vector{pPoints->angle1}.scale(
                    {ellipse->getMajorRadius(), /*-*/ellipse->getMinorRadius()});
                point.rotate(pPoints->center, /*-*/ pPoints->getMajorAngle());
                previewRefPoint(point);
                previewRefSelectablePoint(ellipse->getEndpoint());
                previewEllipseReferencePoints(ellipse, false,true, mouse);
            }
            break;
        }
        default:
            break;
    }
    drawPreview();
}

void LC_ActionDrawEllipse1Point::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status){
        case SetMajorAngle:
        case SetAngle1:
        case SetAngle2:{
            snap = getSnapAngleAwarePoint(e, pPoints->center, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawEllipse1Point::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawEllipse1Point::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint:{
            pPoints->center = pos;
            moveRelativeZero(pos);
            if (pPoints->hasAngle && pPoints->freeAngle){
                setStatus(SetMajorAngle);
            }
            else{
                if (pPoints->isArc){
                    setStatus(SetAngle1);
                }
                else {
                    trigger();
                }
            }
            break;
        }
        case SetMajorAngle: {
            if (LC_LineMath::isMeaningfulDistance(pPoints->center, pos)){
                double majorAngle = pPoints->center.angleTo(pos);
                pPoints->majorRadiusAngle = majorAngle;
                if (pPoints->isArc){
                    setStatus(SetAngle1);
                }
                else{
                    trigger();
                }
            }
            break;
        }
        case SetAngle1: {
            RS_Vector m = pos;
            m.rotate(pPoints->center, -pPoints->getMajorAngle());
            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->getRatio();
            pPoints->angle1 = v.angle();
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            RS_Vector m = pos;
            m.rotate(pPoints->center, -pPoints->getMajorAngle());
            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->getRatio();
            pPoints->angle2 = v.angle();
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::updateMouseButtonHints() {
    int status = getStatus();
    switch (status){
        case SetPoint:{
            updateMouseWidgetTRCancel(tr("Specify ellipse center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetMajorAngle: {
            updateMouseWidgetTRCancel(tr("Specify angle for major axis"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetAngle1: {
            updateMouseWidgetTRCancel(tr("Set start angle of arc"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetAngle2: {
            updateMouseWidgetTRCancel(tr("Set end angle of arc"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawEllipse1Point::doProcessCommand(int status, const QString &command) {
    bool accept = false;
    switch (status) {
        case SetMajorAngle: {
            bool ok;
            double a = RS_Math::eval(command, &ok);
            if (ok){
                accept = true;
                pPoints->majorRadiusAngle = RS_Math::deg2rad(a);
                if (pPoints->isArc) {
                    setStatus(SetAngle2);
                }
                else{
                    trigger();
                }
            } else
                commandMessage(tr("Not a valid expression"));
            break;
        }
        case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(command, &ok);
            if (ok){
                accept = true;
                pPoints->angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else
                commandMessage(tr("Not a valid expression"));
            break;
        }
        case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(command, &ok);
            if (ok){
                accept = true;
                pPoints->angle2 = RS_Math::deg2rad(a);
                trigger();
            } else
                commandMessage(tr("Not a valid expression"));
            break;
        }
        default:
            break;
    }
    return accept;
}

double LC_ActionDrawEllipse1Point::getMajorRadius() {
    return pPoints->majorRadius;
}

double LC_ActionDrawEllipse1Point::getMinorRadius() {
    return pPoints->minorRadius;
}

double LC_ActionDrawEllipse1Point::getAngle() {
    return pPoints->majorRadiusAngle;
}

bool LC_ActionDrawEllipse1Point::hasAngle() {
    return pPoints->hasAngle;
}

bool LC_ActionDrawEllipse1Point::isAngleFree() {
    return pPoints->freeAngle;
}

void LC_ActionDrawEllipse1Point::setMajorRadius(double val) {
    pPoints->majorRadius = val;
}

void LC_ActionDrawEllipse1Point::setMinorRadius(double val) {
    pPoints->minorRadius = val;
}

void LC_ActionDrawEllipse1Point::setAngle(double val) {
    pPoints->majorRadiusAngle = val;
}

void LC_ActionDrawEllipse1Point::setHasAngle(bool val) {
    pPoints->hasAngle = val;
    toSetPointStatus();
}

void LC_ActionDrawEllipse1Point::setAngleFree(bool val) {
    pPoints->freeAngle = val;
    toSetPointStatus();
}

void LC_ActionDrawEllipse1Point::toSetPointStatus() {
    int status = this->getStatus();
    if (status > SetPoint){
        this->setStatus(SetPoint);
    }
}

LC_ActionOptionsWidget *LC_ActionDrawEllipse1Point::createOptionsWidget() {
    return new LC_Ellipse1PointOptions();
}

void LC_ActionDrawEllipse1Point::setReversed(bool b) const{
   pPoints->reversed = b;
}

bool LC_ActionDrawEllipse1Point::isReversed() const{
    return pPoints->reversed;
}
