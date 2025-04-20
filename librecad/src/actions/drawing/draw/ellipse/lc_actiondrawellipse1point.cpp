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

#include "lc_actiondrawellipse1point.h"

#include "lc_ellipse1pointoptions.h"
#include "lc_linemath.h"
#include "rs_ellipse.h"

struct LC_ActionDrawEllipse1Point::ActionData {
/** Center of ellipse */
    RS_Vector center = RS_Vector(false);
    double  majorRadius = 1.0;
    double minorRadius = 1.0;
    bool hasAngle = false;
    bool freeAngle = false;
    double ucsBasicMajorRadiusAngle = 0.0;
    double angle1 = 0.;
    double angle2 = 0.;
    bool isArc = false;
    bool reversed = false;

    double getMajorAngle(){
        return hasAngle ? ucsBasicMajorRadiusAngle : 0.0;
    }

    double getRatio(){
        return minorRadius / majorRadius;
    }
};

LC_ActionDrawEllipse1Point::LC_ActionDrawEllipse1Point(LC_ActionContext *actionContext,  bool isArc)
    :LC_ActionDrawCircleBase("Draw ellipse by 1 point", actionContext, isArc ? RS2::ActionDrawEllipseArc1Point : RS2::ActionDrawEllipse1Point),
    m_ActionData(std::make_unique<ActionData>()){
    m_ActionData->isArc = isArc;
    m_ActionData->angle2 = isArc ? 2. * M_PI : 0.;
}

void LC_ActionDrawEllipse1Point::init(int status) {
    LC_ActionDrawCircleBase::init(status);
}

LC_ActionDrawEllipse1Point::~LC_ActionDrawEllipse1Point() = default;

void LC_ActionDrawEllipse1Point::doTrigger() {
    double ratio = m_ActionData->getRatio();
    auto *ellipse = new RS_Ellipse{m_container,
                                   {m_ActionData->center, getMajorP(), ratio,
                                    m_ActionData->angle1, m_ActionData->angle2, m_ActionData->reversed}
    };
    // todo - code belos is similar to DrawEllipseAxis action.. should we make it common for all ellipse actions?
    if   (ratio > 1.){
        ellipse->switchMajorMinor();
    }
    setPenAndLayerToActive(ellipse);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(ellipse->getCenter());
    }

    undoCycleAdd(ellipse);

    setStatus(SetPoint);
}

RS_Vector LC_ActionDrawEllipse1Point::getMajorP(){
    return RS_Vector::polar(m_ActionData->majorRadius, toWorldAngleFromUCSBasis(m_ActionData->getMajorAngle()));
}

void LC_ActionDrawEllipse1Point::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;

    switch (status){
        case SetPoint:{
            if (!trySnapToRelZeroCoordinateEvent(e)){
                auto *ellipse = previewToCreateEllipse({mouse, getMajorP(), m_ActionData->getRatio(), 0.0,
                                                      m_ActionData->isArc ? 2. * M_PI : 0., false});
                if (m_showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewEllipseReferencePoints(ellipse, true, true, mouse);
                }
            }
            break;
        }
        case SetMajorAngle: {
            mouse = getSnapAngleAwarePoint(e, m_ActionData->center, mouse, true);
            m_ActionData->ucsBasicMajorRadiusAngle = toUCSBasisAngle(m_ActionData->center.angleTo(mouse));
            auto ellipse = previewToCreateEllipse({m_ActionData->center, getMajorP(), m_ActionData->getRatio(), 0.0,
                            m_ActionData->isArc ? 2. * M_PI : 0., false});
            if (m_showRefEntitiesOnPreview){
                previewRefSelectablePoint(mouse);
                previewRefPoint(m_ActionData->center);
                previewRefLine(m_ActionData->center, mouse);
                previewEllipseReferencePoints(ellipse, true, false, mouse);
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_ActionData->center, mouse, true);

            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(m_ActionData->center, -toWorldAngleFromUCSBasis(m_ActionData->ucsBasicMajorRadiusAngle));
            }
            RS_Vector v = m - m_ActionData->center;
            v.y /= m_ActionData->getRatio();
            double angle = v.angle();
            m_ActionData->angle1 = angle;

            previewRefLine(m_ActionData->center, mouse);

            auto ellipse = previewToCreateEllipse({m_ActionData->center, getMajorP(), m_ActionData->getRatio(),
                                           m_ActionData->angle1, m_ActionData->angle1 + 1.0, m_ActionData->reversed});

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_ActionData->center);
                previewRefSelectablePoint(ellipse->getStartpoint());
                previewEllipseReferencePoints(ellipse, false, true, mouse);
            }
            break;
        }
        case SetAngle2: {
            mouse = getSnapAngleAwarePoint(e, m_ActionData->center, mouse, true);
            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(m_ActionData->center, -toWorldAngle(m_ActionData->getMajorAngle()));
            }

            RS_Vector v = m - m_ActionData->center;
            v.y /= m_ActionData->getRatio();
            m_ActionData->angle2 = v.angle();

            auto ellipse = previewToCreateEllipse({m_ActionData->center, getMajorP(), m_ActionData->getRatio(),
                                           m_ActionData->angle1, m_ActionData->angle2, m_ActionData->reversed});

            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_ActionData->center, mouse);
                previewRefPoint(m_ActionData->center);
                auto point = m_ActionData->center + RS_Vector{m_ActionData->angle1}.scale(
                    {ellipse->getMajorRadius(), /*-*/ellipse->getMinorRadius()});
                point.rotate(m_ActionData->center, /*-*/ m_ActionData->getMajorAngle());
                previewRefPoint(point);
                previewRefSelectablePoint(ellipse->getEndpoint());
                previewEllipseReferencePoints(ellipse, false,true, mouse);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetMajorAngle:
        case SetAngle1:
        case SetAngle2:{
            snap = getSnapAngleAwarePoint(e, m_ActionData->center, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawEllipse1Point::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawEllipse1Point::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint:{
            m_ActionData->center = pos;
            moveRelativeZero(pos);
            if (m_ActionData->hasAngle && m_ActionData->freeAngle){
                setStatus(SetMajorAngle);
            }
            else{
                if (m_ActionData->isArc){
                    setStatus(SetAngle1);
                }
                else {
                    trigger();
                }
            }
            break;
        }
        case SetMajorAngle: {
            if (isZero){
                m_ActionData->ucsBasicMajorRadiusAngle = toUCSBasisAngle(0);
                if (m_ActionData->isArc) {
                    setStatus(SetAngle1);
                }
                else{
                    trigger();
                }
            }
            else if (LC_LineMath::isMeaningfulDistance(m_ActionData->center, pos)){
                double majorAngle = toUCSBasisAngle(m_ActionData->center.angleTo(pos));
                m_ActionData->ucsBasicMajorRadiusAngle = majorAngle;
                if (m_ActionData->isArc){
                    setStatus(SetAngle1);
                }
                else{
                    trigger();
                }
            }
            break;
        }
        case SetAngle1: {
            if (isZero){
                m_ActionData->angle1 = 0;
            }
            else {
                RS_Vector m = pos;
                m.rotate(m_ActionData->center, toUCSAngle(-m_ActionData->getMajorAngle()));
                RS_Vector v = m - m_ActionData->center;
                v.y /= m_ActionData->getRatio();
                m_ActionData->angle1 = v.angle();
            }
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            if (isZero){
                m_ActionData->angle2 = 0;
            }
            else {
                RS_Vector m = pos;
                m.rotate(m_ActionData->center, toUCSAngle(-m_ActionData->getMajorAngle()));
                RS_Vector v = m - m_ActionData->center;
                v.y /= m_ActionData->getRatio();
                m_ActionData->angle2 = v.angle();
            }
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
    if (checkCommand("angle", command)){
        accept = true;
        setStatus(SetMajorAngle);
    }
    else if (checkCommand("angle1", command)){
        accept = true;
        setStatus(SetAngle1);
    }
    else if (checkCommand("angle2", command)){
        accept = true;
        setStatus(SetAngle2);
    }
    else {
        switch (status) {
            case SetMajorAngle: {
                double wcsAngle;
                bool ok = parseToUCSBasisAngle(command, wcsAngle);
                if (ok) {
                    accept = true;
                    m_ActionData->ucsBasicMajorRadiusAngle = wcsAngle;
                    if (m_ActionData->isArc) {
                        setStatus(SetAngle2);
                    } else {
                        trigger();
                    }
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            case SetAngle1: {
                bool ok;
                double ucsAngleDegrees = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    m_ActionData->angle1 = RS_Math::deg2rad(ucsAngleDegrees);
                    setStatus(SetAngle2);
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            case SetAngle2: {
                bool ok;
                double ucsAngleDegrees = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    m_ActionData->angle2 = RS_Math::deg2rad(ucsAngleDegrees);
                    trigger();
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            default:
                break;
        }
    }
    return accept;
}

QStringList LC_ActionDrawEllipse1Point::getAvailableCommands() {
    if (m_actionType == RS2::ActionDrawEllipseArc1Point) {
        return {command("angle"), command("angle1"),command("angle2")};
    }
    else{
        return {command("angle")};
    }
}


double LC_ActionDrawEllipse1Point::getMajorRadius() {
    return m_ActionData->majorRadius;
}

double LC_ActionDrawEllipse1Point::getMinorRadius() {
    return m_ActionData->minorRadius;
}

bool LC_ActionDrawEllipse1Point::isAngleFree() {
    return m_ActionData->freeAngle;
}

void LC_ActionDrawEllipse1Point::setMajorRadius(double val) {
    m_ActionData->majorRadius = val;
}

void LC_ActionDrawEllipse1Point::setMinorRadius(double val) {
    m_ActionData->minorRadius = val;
}

double LC_ActionDrawEllipse1Point::getUcsMajorAngleDegrees() {
    return RS_Math::rad2deg(m_ActionData->ucsBasicMajorRadiusAngle);
}

void LC_ActionDrawEllipse1Point::setUcsMajorAngleDegrees(double ucsBasisAngleDegrees) {
    m_ActionData->ucsBasicMajorRadiusAngle = RS_Math::deg2rad(ucsBasisAngleDegrees);
}

void LC_ActionDrawEllipse1Point::setHasAngle(bool val) {
    m_ActionData->hasAngle = val;
    toSetPointStatus();
}

void LC_ActionDrawEllipse1Point::setAngleFree(bool val) {
    m_ActionData->freeAngle = val;
    toSetPointStatus();
}

bool LC_ActionDrawEllipse1Point::hasAngle() {
    return m_ActionData->hasAngle;
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
   m_ActionData->reversed = b;
}

bool LC_ActionDrawEllipse1Point::isReversed() const{
    return m_ActionData->reversed;
}
