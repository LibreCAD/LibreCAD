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

#include "lc_action_draw_ellipse_1point.h"

#include "lc_ellipse_1_point_options_filler.h"
#include "lc_ellipse_1point_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_ellipse.h"

struct LC_ActionDrawEllipse1Point::ActionData {
    /** Center of ellipse */
    RS_Vector center = RS_Vector(false);
    double majorRadius = 1.0;
    double minorRadius = 1.0;
    double ucsBasicMajorRadiusAngle = 0.0;
    double angle1 = 0.;
    double angle2 = 0.;
    bool hasAngle = false;
    bool freeAngle = false;
    bool isArc = false;
    bool reversed = false;

    double getMajorAngle() const {
        return hasAngle ? ucsBasicMajorRadiusAngle : 0.0;
    }

    double getRatio() const {
        return minorRadius / majorRadius;
    }
};

LC_ActionDrawEllipse1Point::LC_ActionDrawEllipse1Point(LC_ActionContext* actionContext, const bool isArc)
    : LC_ActionDrawCircleBase( isArc ? "ActionDrawEllipseArc1Point" : "ActionDrawEllipse1Point", actionContext,
                              isArc ? RS2::ActionDrawEllipseArc1Point : RS2::ActionDrawEllipse1Point),
      m_actionData(std::make_unique<ActionData>()) {
    m_actionData->isArc = isArc;
    m_actionData->angle2 = isArc ? 2. * M_PI : 0.;
}

void LC_ActionDrawEllipse1Point::doSaveOptions() {
    save("MajorRadius", getMajorRadius());
    save("MinorRadius", getMinorRadius());
    save("UseAngle", hasAngle());
    save("Angle", getUcsMajorAngleDegrees());
    save("FreeAngle", isAngleFree());
    if (rtti() == RS2::ActionDrawEllipseArc1Point){
        save("ArcReversed", isReversed());
    }
}

void LC_ActionDrawEllipse1Point::doLoadOptions() {
    double majorRadius = loadDouble("MajorRadius", 10.0);
    setMajorRadius(majorRadius);
    double minorRadius = loadDouble("MinorRadius", 10.0);
    setMinorRadius(minorRadius);
    bool useAngle = loadBool("UseAngle", false);
    setHasAngle(useAngle);
    double angle = loadDouble("Angle", 0.0);
    setUcsMajorAngleDegrees(angle);
    bool freeAngle = loadBool("FreeAngle", false);
    setAngleFree(freeAngle);
    if (rtti() == RS2::ActionDrawEllipseArc1Point){
        bool reversed = loadBool("ArcReversed", false);
        setReversed(reversed);
    }
}

bool LC_ActionDrawEllipse1Point::isInVisualSnapStatus(int status) {
    return (status == SetPoint) || (status == SetMajorAngle) || (status == SetAngle1) || (status == SetAngle2);
}

void LC_ActionDrawEllipse1Point::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
}

LC_ActionDrawEllipse1Point::~LC_ActionDrawEllipse1Point() = default;

RS_Entity* LC_ActionDrawEllipse1Point::doTriggerCreateEntity() {
    const double ratio = m_actionData->getRatio();
    auto* ellipse = new RS_Ellipse{
        m_document,
        {m_actionData->center, getMajorP(), ratio, m_actionData->angle1, m_actionData->angle2, m_actionData->reversed}
    };
    // todo - code below is similar to DrawEllipseAxis action.. should we make it common for all ellipse actions?
    if (ratio > 1.) {
        ellipse->switchMajorMinor();
    }

    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(ellipse->getCenter());
    }
    return ellipse;
}

void LC_ActionDrawEllipse1Point::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetPoint);
}

RS_Vector LC_ActionDrawEllipse1Point::getMajorP() const {
    return RS_Vector::polar(m_actionData->majorRadius, toWorldAngleFromUCSBasis(m_actionData->getMajorAngle()));
}

void LC_ActionDrawEllipse1Point::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;

    switch (status) {
        case SetPoint: {
            if (!trySnapToRelZeroCoordinateEvent(e)) {
                const auto* ellipse = previewToCreateEllipse({
                    mouse,
                    getMajorP(),
                    m_actionData->getRatio(),
                    0.0,
                    m_actionData->isArc ? 2. * M_PI : 0.,
                    false
                });
                if (m_showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewEllipseReferencePoints(ellipse, true, true, mouse);
                }
            }
            break;
        }
        case SetMajorAngle: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);
            m_actionData->ucsBasicMajorRadiusAngle = toUCSBasisAngle(m_actionData->center.angleTo(mouse));
            const auto ellipse = previewToCreateEllipse({
                m_actionData->center,
                getMajorP(),
                m_actionData->getRatio(),
                0.0,
                m_actionData->isArc ? 2. * M_PI : 0.,
                false
            });
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
                previewRefPoint(m_actionData->center);
                previewRefLine(m_actionData->center, mouse);
                previewEllipseReferencePoints(ellipse, true, false, mouse);
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);

            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(m_actionData->center, -toWorldAngleFromUCSBasis(m_actionData->ucsBasicMajorRadiusAngle));
            }
            RS_Vector v = m - m_actionData->center;
            v.y /= m_actionData->getRatio();
            const double angle = v.angle();
            m_actionData->angle1 = angle;

            previewRefLine(m_actionData->center, mouse);

            const auto ellipse = previewToCreateEllipse({
                m_actionData->center,
                getMajorP(),
                m_actionData->getRatio(),
                m_actionData->angle1,
                m_actionData->angle1 + 1.0,
                m_actionData->reversed
            });

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->center);
                previewRefSelectablePoint(ellipse->getStartpoint());
                previewEllipseReferencePoints(ellipse, false, true, mouse);
            }
            break;
        }
        case SetAngle2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);
            RS_Vector m = mouse;
            if (hasAngle()) {
                m.rotate(m_actionData->center, -toWorldAngle(m_actionData->getMajorAngle()));
            }

            RS_Vector v = m - m_actionData->center;
            v.y /= m_actionData->getRatio();
            m_actionData->angle2 = v.angle();

            const auto ellipse = previewToCreateEllipse({
                m_actionData->center,
                getMajorP(),
                m_actionData->getRatio(),
                m_actionData->angle1,
                m_actionData->angle2,
                m_actionData->reversed
            });

            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_actionData->center, mouse);
                previewRefPoint(m_actionData->center);
                auto point = m_actionData->center + RS_Vector{m_actionData->angle1}.scale({
                    ellipse->getMajorRadius(),
                    /*-*/
                    ellipse->getMinorRadius()
                });
                point.rotate(m_actionData->center, /*-*/ m_actionData->getMajorAngle());
                previewRefPoint(point);
                previewRefSelectablePoint(ellipse->getEndpoint());
                previewEllipseReferencePoints(ellipse, false, true, mouse);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetMajorAngle:
        case SetAngle1:
        case SetAngle2: {
            snap = getSnapAngleAwarePoint(e, m_actionData->center, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawEllipse1Point::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawEllipse1Point::onCoordinateEvent(const int status, [[maybe_unused]] const bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetPoint: {
            m_actionData->center = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            if (m_actionData->hasAngle && m_actionData->freeAngle) {
                setStatus(SetMajorAngle);
            }
            else {
                if (m_actionData->isArc) {
                    setStatus(SetAngle1);
                }
                else {
                    trigger();
                }
            }
            break;
        }
        case SetMajorAngle: {
            if (isZero) {
                m_actionData->ucsBasicMajorRadiusAngle = toUCSBasisAngle(0);
                if (m_actionData->isArc) {
                    setStatus(SetAngle1);
                }
                else {
                    trigger();
                }
            }
            else if (LC_LineMath::isMeaningfulDistance(m_actionData->center, pos)) {
                const double majorAngle = toUCSBasisAngle(m_actionData->center.angleTo(pos));
                m_actionData->ucsBasicMajorRadiusAngle = majorAngle;
                if (m_actionData->isArc) {
                    setStatus(SetAngle1);
                }
                else {
                    trigger();
                }
            }
            break;
        }
        case SetAngle1: {
            if (isZero) {
                m_actionData->angle1 = 0;
            }
            else {
                RS_Vector m = pos;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->getMajorAngle()));
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->getRatio();
                m_actionData->angle1 = v.angle();
            }
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            if (isZero) {
                m_actionData->angle2 = 0;
            }
            else {
                RS_Vector m = pos;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->getMajorAngle()));
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->getRatio();
                m_actionData->angle2 = v.angle();
            }
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::updateActionPrompt() {
    const int status = getStatus();
    switch (status) {
        case SetPoint: {
            updatePromptTRCancel(tr("Specify ellipse center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetMajorAngle: {
            updatePromptTRCancel(tr("Specify angle for major axis"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetAngle1: {
            updatePromptTRCancel(tr("Set start angle of arc"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetAngle2: {
            updatePromptTRCancel(tr("Set end angle of arc"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawEllipse1Point::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    if (checkCommand("angle", command)) {
        accept = true;
        setStatus(SetMajorAngle);
    }
    else if (checkCommand("angle1", command)) {
        accept = true;
        setStatus(SetAngle1);
    }
    else if (checkCommand("angle2", command)) {
        accept = true;
        setStatus(SetAngle2);
    }
    else {
        switch (status) {
            case SetMajorAngle: {
                double wcsAngle;
                const bool ok = parseToUCSBasisAngle(command, wcsAngle);
                if (ok) {
                    accept = true;
                    m_actionData->ucsBasicMajorRadiusAngle = wcsAngle;
                    if (m_actionData->isArc) {
                        setStatus(SetAngle2);
                    }
                    else {
                        trigger();
                    }
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                break;
            }
            case SetAngle1: {
                bool ok;
                const double ucsAngleDegrees = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    m_actionData->angle1 = RS_Math::deg2rad(ucsAngleDegrees);
                    setStatus(SetAngle2);
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                break;
            }
            case SetAngle2: {
                bool ok;
                const double ucsAngleDegrees = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    m_actionData->angle2 = RS_Math::deg2rad(ucsAngleDegrees);
                    trigger();
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
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
        return {command("angle"), command("angle1"), command("angle2")};
    }
    return {command("angle")};
}

double LC_ActionDrawEllipse1Point::getMajorRadius() const {
    return m_actionData->majorRadius;
}

double LC_ActionDrawEllipse1Point::getMinorRadius() const {
    return m_actionData->minorRadius;
}

bool LC_ActionDrawEllipse1Point::isAngleFree() const {
    return m_actionData->freeAngle;
}

void LC_ActionDrawEllipse1Point::setMajorRadius(const double val) const {
    m_actionData->majorRadius = val;
}

void LC_ActionDrawEllipse1Point::setMinorRadius(const double val) const {
    m_actionData->minorRadius = val;
}

double LC_ActionDrawEllipse1Point::getUcsMajorAngleDegrees() const {
    return RS_Math::rad2deg(m_actionData->ucsBasicMajorRadiusAngle);
}

void LC_ActionDrawEllipse1Point::setUcsMajorAngleDegrees(const double ucsBasisAngleDegrees) const {
    m_actionData->ucsBasicMajorRadiusAngle = RS_Math::deg2rad(ucsBasisAngleDegrees);
}

void LC_ActionDrawEllipse1Point::setUcsMajorAngle(const double ucsBasisAngleRad) const {
    m_actionData->ucsBasicMajorRadiusAngle = ucsBasisAngleRad;
}

void LC_ActionDrawEllipse1Point::setHasAngle(const bool val) {
    m_actionData->hasAngle = val;
    toSetPointStatus();
}

void LC_ActionDrawEllipse1Point::setAngleFree(const bool val) {
    m_actionData->freeAngle = val;
    toSetPointStatus();
}

bool LC_ActionDrawEllipse1Point::hasAngle() const {
    return m_actionData->hasAngle;
}

void LC_ActionDrawEllipse1Point::toSetPointStatus() {
    const int status = this->getStatus();
    if (status > SetPoint) {
        this->setStatus(SetPoint);
    }
}

bool LC_ActionDrawEllipse1Point::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setUcsMajorAngle(angleRad);
        return true;
    }
    return false;
}

bool LC_ActionDrawEllipse1Point::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "major") {
        setMajorRadius(distance);
        return true;
    }
    if (tag == "minor") {
        setMinorRadius(distance);
        return true;
    }
    return false;
}

LC_ActionOptionsWidget* LC_ActionDrawEllipse1Point::createOptionsWidget() {
    return new LC_Ellipse1PointOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawEllipse1Point::createOptionsFiller() {
    return new LC_Ellipse1PointOptionsFiller();
}

void LC_ActionDrawEllipse1Point::setReversed(const bool b) const {
    m_actionData->reversed = b;
}

bool LC_ActionDrawEllipse1Point::isReversed() const {
    return m_actionData->reversed;
}
