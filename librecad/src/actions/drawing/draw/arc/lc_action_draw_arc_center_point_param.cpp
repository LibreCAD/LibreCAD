/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_action_draw_arc_center_point_param.h"

#include "lc_arc_center_point_param_options_filler.h"
#include "lc_arc_center_point_param_options_widget.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_line.h"

// fixme - sand -  expand actions options widget to support all possible settings (like angle, radius, start angle, end/total angle, chordlen)
LC_ActionDrawArcCenterPointParam::LC_ActionDrawArcCenterPointParam(LC_ActionContext* actionContext, const RS2::ActionType actionType)
    : LC_ActionDrawCircleBase("", actionContext, actionType), m_arcData(std::make_unique<RS_ArcData>()) {
    switch (actionType) {
        case RS2::ActionDrawArc: {
            m_optionsSettingsGroupName = "ActionDrawArc";
            break;
        }
        case RS2::ActionDrawArcAngleLen: {
            m_optionsSettingsGroupName = "ActionDrawArcAngleLen";
            break;
        }
        case RS2::ActionDrawArcChord: {
            m_optionsSettingsGroupName = "ActionDrawArcChord";
            break;
        }
        default: Q_ASSERT_X(false, "LC_ActionDrawArcCenterPointParam", "Invalid action type");
    }
    LC_ActionDrawArcCenterPointParam::reset();
}

LC_ActionDrawArcCenterPointParam::~LC_ActionDrawArcCenterPointParam() = default;

void LC_ActionDrawArcCenterPointParam::doSaveOptions() {
    save("Reversed", isReversed());
}

void LC_ActionDrawArcCenterPointParam::doLoadOptions() {
    const bool reversed = loadBool("Reversed", false);
    setReversed(reversed);
}

bool LC_ActionDrawArcCenterPointParam::isInVisualSnapStatus(int status) {
    return (status == SetCenter) || (status == SetRadius) || (status == SetAngle1) || (status == SetAngle2);
}

void LC_ActionDrawArcCenterPointParam::reset() {
    double angleMin = 0.;
    double angleMax = 2. * M_PI;
    if (m_alternateArcDirection) {
        m_arcData->reversed = !m_arcData->reversed;
    }
    if (m_arcData->reversed) {
        std::swap(angleMin, angleMax);
    }
    *m_arcData = {{}, 0., angleMin, angleMax, m_arcData->reversed};
    m_alternateArcDirection = false;
}

void LC_ActionDrawArcCenterPointParam::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    reset();
}

RS_Entity* LC_ActionDrawArcCenterPointParam::doTriggerCreateEntity() {
    if (m_alternateArcDirection) {
        m_arcData->reversed = !m_arcData->reversed;
    }
    const auto arc = new RS_Arc(m_document, *m_arcData);
    moveRelativeZero(arc->getCenter());
    return arc;
}

void LC_ActionDrawArcCenterPointParam::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCenter);
    reset();
}

void LC_ActionDrawArcCenterPointParam::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetCenter: {
            m_arcData->center = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetRadius: {
            if (m_arcData->center.valid) {
                if (rtti() == RS2::ActionDrawArc) {
                    mouse = getFreeSnapAwarePoint(e, mouse);
                }
                else {
                    mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse, true);
                }
                m_arcData->radius = m_arcData->center.distanceTo(mouse);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_arcData->center);
                    previewRefPoint(mouse);
                    previewRefLine(m_arcData->center, mouse);
                }
                previewCircle({m_arcData->center, m_arcData->radius});
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse, true);

            m_arcData->angle1 = m_arcData->center.angleTo(mouse);
            if (m_arcData->reversed) {
                m_arcData->angle2 = RS_Math::correctAngle(m_arcData->angle1 - M_PI / 3);
            }
            else {
                m_arcData->angle2 = RS_Math::correctAngle(m_arcData->angle1 + M_PI / 3);
            }
            previewArc(*m_arcData);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_arcData->center);
                RS_Vector startArcPoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
                previewRefSelectablePoint(startArcPoint);
                previewRefLine(m_arcData->center, mouse);
            }
            break;
        }
        case SetAngle2: {
            mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse, true);
            m_arcData->angle2 = m_arcData->center.angleTo(mouse);
            bool alternateDirection = e->isControl;
            RS_ArcData tmpArcData = *m_arcData;
            if (alternateDirection) {
                tmpArcData.reversed = !tmpArcData.reversed;
            }
            auto arc = previewToCreateArc(tmpArcData);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoints({m_arcData->center, arc->getStartpoint()});
                previewRefSelectablePoint(arc->getEndpoint());
                previewRefLine(m_arcData->center, mouse);
            }
            break;
        }
        case SetIncAngle: {
            RS_Vector& center = m_arcData->center;
            mouse = getSnapAngleAwarePoint(e, center, mouse, true);
            double wcsAngleToMouse = center.angleTo(mouse);
            double wcsAngle = toUCSBasisAngle(wcsAngleToMouse);
            m_arcData->angle2 = m_arcData->angle1 + adjustRelativeAngleSignByBasis(wcsAngle);

            bool alternateDirection = e->isControl;
            RS_ArcData tmpArcdata = *m_arcData;
            if (alternateDirection) {
                tmpArcdata.reversed = !tmpArcdata.reversed;
            }
            auto arc = previewToCreateArc(tmpArcdata);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_arcData->center);
                previewRefPoint(arc->getStartpoint());
                previewRefPoint(arc->getEndpoint());
                RS_Vector nearest = arc->getNearestPointOnEntity(mouse, false);
                previewRefSelectablePoint(nearest);

                previewSnapAngleMark(center, mouse);

                double halfRadius = m_arcData->radius / 2;
                RS_Vector horizontalPoint = center.relative(halfRadius, toWorldAngleFromUCSBasis(0));
                previewRefLine(center, mouse);
                previewRefLine(center, horizontalPoint);
                previewRefLine(center, arc->getEndpoint());
                previewRefLine(center, arc->getStartpoint());
                previewRefArc(RS_ArcData(center, halfRadius, toWorldAngleFromUCSBasis(0.0), wcsAngleToMouse, m_arcData->reversed));
                previewRefArc(RS_ArcData(center, halfRadius * 1.1, arc->getAngle1(), arc->getAngle2(), m_arcData->reversed));
            }
            break;
        }
        case SetChordLength: {
            // todo - add  more relaxed snap... to grid etc???
            RS_Vector startpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);

            RS_Vector arcStart;
            RS_Vector halfCircleArcEnd;
            snapMouseToDiameter(mouse, arcStart, halfCircleArcEnd);
            double distanceFromStartToMouse = arcStart.distanceTo(mouse);

            double diameter = m_arcData->radius * 2;
            m_arcData->angle2 = m_arcData->angle1 + (asin(distanceFromStartToMouse / diameter) * 2);

            RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
            RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);

            RS_ArcData arcDataCopy = *m_arcData;
            bool useAlternativeSolution = e->isShift;
            if (useAlternativeSolution) {
                arcDataCopy.angle2 = m_arcData->center.angleTo(alternativePoint);
                arcDataCopy.reversed = !m_arcData->reversed;
            }
            if (LC_LineMath::isMeaningfulDistance(mouse, arcStart)) {
                auto arc = previewToCreateArc(arcDataCopy);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(arc->getEndpoint());
                    previewRefLine(arcStart, mouse);
                    previewRefLine(arc->getStartpoint(), arc->getEndpoint());

                    if (useAlternativeSolution) {
                        previewRefSelectablePoint(endpoint);
                    }
                    else {
                        previewRefSelectablePoint(alternativePoint);
                    }

                    if (LC_LineMath::isMeaningfulDistance(mouse, halfCircleArcEnd)) {
                        previewRefArc(RS_ArcData(arcStart, distanceFromStartToMouse, arcStart.angleTo(m_arcData->center),
                                                 arcStart.angleTo(arc->getEndpoint()), !useAlternativeSolution));
                    }
                }
            }
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(arcStart);
                previewRefPoint(m_arcData->center);
                previewRefSelectablePoint(mouse);
                previewRefPoint(halfCircleArcEnd);
            }

            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawArcCenterPointParam::snapMouseToDiameter(RS_Vector& mouse, RS_Vector& arcStart, RS_Vector& halfCircleArcEnd) const {
    arcStart = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
    halfCircleArcEnd = m_arcData->center - RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
    const auto diameter = RS_Line(nullptr, RS_LineData(arcStart, halfCircleArcEnd));

    // projection of mouse to diameter
    mouse = diameter.getNearestPointOnEntity(mouse, true);
}

void LC_ActionDrawArcCenterPointParam::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    bool shouldFireCoordinateEvent = true;
    switch (status) {
        case SetRadius: {
            if (rtti() == RS2::ActionDrawArc) {
                mouse = getFreeSnapAwarePoint(e, mouse);
            }
            else {
                mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse, true);
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse);
            break;
        }
        case SetIncAngle:
        case SetAngle2: {
            mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse);
            m_alternateArcDirection = e->isControl;
            break;
        }
        case SetChordLength: {
            RS_Vector arcStart;
            RS_Vector halfCircleArcEnd;
            m_alternateArcDirection = e->isShift;
            snapMouseToDiameter(mouse, arcStart, halfCircleArcEnd);
            shouldFireCoordinateEvent = LC_LineMath::isMeaningfulDistance(mouse, arcStart);
            if (!shouldFireCoordinateEvent) {
                commandMessage(tr("Length of chord should be non-zero"));
            }
            break;
        }
        default:
            break;
    }
    if (shouldFireCoordinateEvent) {
        fireCoordinateEvent(mouse);
    }
}

void LC_ActionDrawArcCenterPointParam::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    switch (status) {
        case SetChordLength: {
            moveRelativeZero(m_arcData->center);
            if (m_actionType == RS2::ActionDrawArc) {
                setStatus(SetAngle2);
            }
            else {
                setStatus(SetRadius);
            }
            break;
        }
        case SetIncAngle: {
            if (m_actionType == RS2::ActionDrawArc) {
                setStatus(SetAngle2);
            }
            else {
                setStatus(SetRadius);
            }
            break;
        }
        default: {
            setStatus(status - 1);
        }
    }
}

void LC_ActionDrawArcCenterPointParam::onCoordinateEvent(const int status, [[maybe_unused]] const bool isZero, const RS_Vector& pos) {
    switch (status) {
        case SetCenter: {
            m_arcData->center = pos;
            addSnappedPointToVisualSnap(pos);
            moveRelativeZero(pos);
            setStatus(SetRadius);
            break;
        }
        case SetRadius: {
            if (m_arcData->center.valid) {
                m_arcData->radius = m_arcData->center.distanceTo(pos);
            }
            switch (m_actionType) {
                case RS2::ActionDrawArc: {
                    setStatus(SetAngle1);
                    break;
                }
                case RS2::ActionDrawArcChord: {
                    m_arcData->angle1 = m_arcData->center.angleTo(pos);
                    setStatus(SetChordLength);
                    break;
                }
                case RS2::ActionDrawArcAngleLen: {
                    m_arcData->angle1 = m_arcData->center.angleTo(pos);
                    setStatus(SetIncAngle);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SetAngle1: {
            if (isZero) {
                m_arcData->angle1 = toWorldAngleFromUCSBasisDegrees(0);
            }
            else {
                m_arcData->angle1 = m_arcData->center.angleTo(pos);
            }
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            if (isZero) {
                m_arcData->angle2 = toWorldAngleFromUCSBasisDegrees(0);
            }
            else {
                m_arcData->angle2 = m_arcData->center.angleTo(pos);
            }
            trigger();
            break;
        }
        case SetIncAngle: {
            const double wcsAngle = m_arcData->center.angleTo(pos);
            const double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
            const double innerAngle = adjustRelativeAngleSignByBasis(rotationAngle);
            m_arcData->angle2 = m_arcData->angle1 + innerAngle;
            trigger();
            break;
        }
        case SetChordLength: {
            // todo - double calculation of arc start - store it for later use?
            const RS_Vector startpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
            const double distanceFromStartToMouse = startpoint.distanceTo(pos);
            const double diameter = 2 * m_arcData->radius;
            if (fabs(distanceFromStartToMouse / diameter) <= 1.0) {
                m_arcData->angle2 = m_arcData->angle1 + (asin(distanceFromStartToMouse / diameter) * 2);

                if (m_alternateArcDirection) {
                    RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
                    const RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);
                    m_arcData->angle2 = m_arcData->center.angleTo(alternativePoint);
                    //                    m_arcData->reversed = !m_arcData->reversed ;
                }
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawArcCenterPointParam::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    if (checkCommand("reversed", command)) {
        accept = true;
        setReversed(!isReversed());
        updateOptions();
    }
    else {
        switch (status) {
            case SetRadius: {
                bool ok = false;
                const double r = RS_Math::eval(command, &ok);
                if (ok) {
                    m_arcData->radius = r;
                    setStatus(SetAngle1);
                    accept = true;
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                break;
            }
            case SetAngle1: {
                double wcsAngle;
                const bool ok = parseToWCSAngle(command, wcsAngle);
                if (ok) {
                    m_arcData->angle1 = wcsAngle;
                    accept = true;
                    setStatus(SetAngle2);
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                break;
            }
            case SetAngle2: {
                if (checkCommand("angle", command)) {
                    setStatus(SetIncAngle);
                    accept = true;
                }
                else if (checkCommand("chordlen", command)) {
                    const RS_Vector arcStart = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
                    addSnappedPointToVisualSnap(arcStart);
                    moveRelativeZero(arcStart);
                    setStatus(SetChordLength);
                    accept = true;
                }
                else {
                    double wcsAngle;
                    const bool ok = parseToWCSAngle(command, wcsAngle);
                    if (ok) {
                        m_arcData->angle2 = wcsAngle;
                        accept = true;
                        trigger();
                    }
                    else {
                        commandMessage(tr("Not a valid expression"));
                    }
                }
                break;
            }
            case SetIncAngle: {
                double relativeAngleRad;
                const bool ok = parseToRelativeAngle(command, relativeAngleRad);
                if (ok) {
                    m_arcData->angle2 = m_arcData->angle1 + relativeAngleRad;
                    accept = true;
                    trigger();
                }
                else {
                    commandMessage(tr("Not a valid expression"));
                }
                break;
            }
            case SetChordLength: {
                bool ok = false;
                const double l = RS_Math::eval(command, &ok);
                if (ok) {
                    accept = true;
                    if (fabs(l / (2 * m_arcData->radius)) <= 1.0) {
                        m_arcData->angle2 = m_arcData->angle1 + asin(l / (2 * m_arcData->radius)) * 2;
                        if (l < 0) {
                            // using alternative solution (if negative value is entered)
                            const RS_Vector startpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
                            RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
                            const RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);

                            m_arcData->angle2 = m_arcData->center.angleTo(alternativePoint);
                            m_arcData->reversed = !m_arcData->reversed;
                        }
                        trigger();
                    }
                    else {
                        commandMessage(tr("Not a valid chord length"));
                    }
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

QStringList LC_ActionDrawArcCenterPointParam::getAvailableCommands() {
    if (m_actionType == RS2::ActionDrawArc) {
        return {command("angle"), command("chordlen"), command("reversed")};
    }
    return {command("reversed")};
}

void LC_ActionDrawArcCenterPointParam::updateActionPrompt() {
    switch (getStatus()) {
        case SetCenter:
            updatePromptTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            if (m_actionType == RS2::ActionDrawArc) {
                updatePromptTRBack(tr("Specify radius"), MOD_SHIFT_FREE_SNAP);
            }
            else {
                updatePromptTRBack(tr("Specify start point"), MOD_SHIFT_FREE_SNAP);
            }
            break;
        case SetAngle1:
            updatePromptTRBack(tr("Specify start angle:"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle2:
            updatePromptTRBack(tr("Specify end angle or [angle/chordlen]"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternative Arc")));
            break;
        case SetIncAngle:
            updatePromptTRBack(tr("Specify included angle:"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternative Arc")));
            break;
        case SetChordLength:
            updatePromptTRBack(tr("Specify chord length (negative for alt point):"), MOD_SHIFT_LC(tr("Use alternative arc point")));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawArcCenterPointParam::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

bool LC_ActionDrawArcCenterPointParam::isReversed() const {
    return m_arcData->reversed;
}

void LC_ActionDrawArcCenterPointParam::setReversed(const bool r) const {
    m_arcData->reversed = r;
}

LC_ActionOptionsWidget* LC_ActionDrawArcCenterPointParam::createOptionsWidget() {
    return new LC_ArcCenterPointParamOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawArcCenterPointParam::createOptionsFiller() {
    return new LC_ArcCenterPointParamOptionsFiller();
}
