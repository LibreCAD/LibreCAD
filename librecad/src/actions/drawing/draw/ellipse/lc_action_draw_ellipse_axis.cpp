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

#include "lc_action_draw_ellipse_axis.h"

#include "lc_ellipse_arc_options_filler.h"
#include "lc_ellipse_arc_options_widget.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_line.h"

struct RS_ActionDrawEllipseAxis::ActionData {
    /** Center of ellipse */
    RS_Vector center;
    /** Endpoint of major axis */
    RS_Vector vMajorP;
    /** Ratio major / minor */
    double ratio = 1.;
    /** Start angle */
    double angle1 = 0.;
    /** End angle */
    double angle2 = 0.;
    /** Do we produce an arc (true) or full ellipse (false) */
    bool isArc = false;
    // is arc reversed?
    bool reversed = false;
};

/**
 * Constructor.
 *
 * @param actionContext
 * @param isArc true if this action will produce an ellipse arc.
 *              false if it will produce a full ellipse.
 */
RS_ActionDrawEllipseAxis::RS_ActionDrawEllipseAxis(LC_ActionContext* actionContext, const bool isArc)
    : LC_ActionDrawCircleBase(isArc ? "ActionDrawEllipseArcAxis" : "ActionDrawEllipseAxis", actionContext, isArc ? RS2::ActionDrawEllipseArcAxis : RS2::ActionDrawEllipseAxis),
      m_actionData(std::make_unique<ActionData>()) {
    m_actionData->isArc = isArc;
    m_actionData->angle2 = isArc ? 2. * M_PI : 0.;
}

RS_ActionDrawEllipseAxis::~RS_ActionDrawEllipseAxis() = default;

void RS_ActionDrawEllipseAxis::doSaveOptions() {
    if (m_actionData->isArc) {
        save("Reversed", isReversed());
    }
}

void RS_ActionDrawEllipseAxis::doLoadOptions() {
    if (m_actionData->isArc) {
        const bool reversed = loadBool("Reversed",  false);
        setReversed(reversed);
    }
}

bool RS_ActionDrawEllipseAxis::isInVisualSnapStatus(int status) {
    return (status == SetCenter) || (status == SetMajor) || (status == SetMinor) || (status == SetAngle1) || (status == SetAngle2);
}

void RS_ActionDrawEllipseAxis::init(const int status) {
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter) {
        m_actionData->center = {};
    }
    if (status <= SetMajor) {
        m_actionData->vMajorP = {};
    }
    if (status <= SetMinor) {
        m_actionData->ratio = 0.5;
    }
    if (status <= SetAngle1) {
        m_actionData->angle1 = 0.0;
    }
    if (status <= SetAngle2) {
        m_actionData->angle2 = 0.0;
    }
}

RS_Entity* RS_ActionDrawEllipseAxis::doTriggerCreateEntity() {
    auto* ellipse = new RS_Ellipse{
        m_document,
        {
            m_actionData->center,
            m_actionData->vMajorP,
            m_actionData->ratio,
            m_actionData->angle1,
            m_actionData->angle2,
            m_actionData->reversed
        }
    };
    if (m_actionData->ratio > 1.) {
        ellipse->switchMajorMinor();
    }

    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(ellipse->getCenter());
    }
    return ellipse;
}

void RS_ActionDrawEllipseAxis::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCenter);
}

void RS_ActionDrawEllipseAxis::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (getStatus()) {
        case SetCenter: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetMajor: {
            if (m_actionData->center.valid) {
                mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);

                previewToCreateEllipse({
                    m_actionData->center,
                    mouse - m_actionData->center,
                    0.5,
                    0.0,
                    m_actionData->isArc ? 2. * M_PI : 0.,
                    false
                });

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->center);
                    previewRefLine(m_actionData->center, mouse);
                    previewRefSelectablePoint(mouse);
                }
            }
            break;
        }
        case SetMinor: {
            if (m_actionData->center.valid && m_actionData->vMajorP.valid) {
                const RS_Vector& center = m_actionData->center;
                const RS_Vector& major1Point = center - m_actionData->vMajorP;
                const RS_Vector& major2Point = center + m_actionData->vMajorP;
                const RS_Line line{major1Point, major2Point};
                const double d = line.getDistanceToPoint(mouse, nullptr, RS2::ResolveNone, RS_MAXDOUBLE);
                m_actionData->ratio = d / (line.getLength() / 2);
                const auto ellipse = previewToCreateEllipse({
                    center,
                    m_actionData->vMajorP,
                    m_actionData->ratio,
                    0.,
                    m_actionData->isArc ? 2. * M_PI : 0.,
                    false
                });

                if (m_showRefEntitiesOnPreview) {
                    previewEllipseReferencePoints(ellipse, true, false, mouse);
                }
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);
            if (m_actionData->center.valid && m_actionData->vMajorP.valid) {
                //angle1 = center.angleTo(mouse);
                RS_Vector m = mouse;
                m.rotate(m_actionData->center, -m_actionData->vMajorP.angle());
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle1 = v.angle(); // + m_vMajorP.angle();

                previewRefLine(m_actionData->center, mouse);

                const auto ellipse = previewToCreateEllipse({
                    m_actionData->center,
                    m_actionData->vMajorP,
                    m_actionData->ratio,
                    m_actionData->angle1,
                    m_actionData->angle1 + 1.0,
                    m_actionData->reversed
                });

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->center);
                    previewRefSelectablePoint(ellipse->getStartpoint());
                    previewEllipseReferencePoints(ellipse, false, true, mouse);
                }
            }
            break;
        }
        case SetAngle2: {
            if (m_actionData->center.valid && m_actionData->vMajorP.valid) {
                // todo - redundant check
                //angle2 = center.angleTo(mouse);
                mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);

                RS_Vector m = mouse;
                m.rotate(m_actionData->center, -m_actionData->vMajorP.angle());
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle2 = v.angle(); // + m_vMajorP.angle();

                const auto ellipse = previewToCreateEllipse({
                    m_actionData->center,
                    m_actionData->vMajorP,
                    m_actionData->ratio,
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
                    point.rotate(m_actionData->center, /*-*/ m_actionData->vMajorP.angle());
                    previewRefPoint(point);
                    previewRefSelectablePoint(ellipse->getEndpoint());
                    previewEllipseReferencePoints(ellipse, false, true, mouse);
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawEllipseAxis::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetMajor:
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

void RS_ActionDrawEllipseAxis::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseAxis::onCoordinateEvent(const int status, [[maybe_unused]] const bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetCenter: {
            m_actionData->center = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetMajor);
            break;
        }
        case SetMajor: {
            m_actionData->vMajorP = coord - m_actionData->center;
            setStatus(SetMinor);
            break;
        }
        case SetMinor: {
            const RS_Line line{m_actionData->center - m_actionData->vMajorP, m_actionData->center + m_actionData->vMajorP};
            const double d = line.getDistanceToPoint(coord);
            m_actionData->ratio = d / (line.getLength() / 2);
            if (!m_actionData->isArc) {
                trigger();
                setStatus(SetCenter);
            }
            else {
                setStatus(SetAngle1);
            }
            break;
        }
        case SetAngle1: {
            if (isZero) {
                m_actionData->angle1 = 0;
            }
            else {
                RS_Vector m = coord;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->vMajorP.angle()));
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
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
                RS_Vector m = coord;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->vMajorP.angle()));
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle2 = v.angle();
            }
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawEllipseAxis::isReversed() const {
    return m_actionData->reversed;
}

void RS_ActionDrawEllipseAxis::setReversed(const bool b) const {
    m_actionData->reversed = b;
}

// fixme - sand - complete support of command line!
bool RS_ActionDrawEllipseAxis::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetMinor: {
            bool ok = false;
            const double m = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                m_actionData->ratio = m / m_actionData->vMajorP.magnitude();
                if (!m_actionData->isArc) {
                    trigger();
                }
                else {
                    setStatus(SetAngle1);
                }
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            break;
        }
        case SetAngle1: {
            bool ok = false;
            const double a = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                m_actionData->angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            break;
        }
        case SetAngle2: {
            bool ok = false;
            const double a = RS_Math::eval(command, &ok);
            if (ok) {
                accept = true;
                m_actionData->angle2 = RS_Math::deg2rad(a);
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
    return accept;
}

void RS_ActionDrawEllipseAxis::updateActionPrompt() {
    switch (getStatus()) {
        case SetCenter:
            updatePromptTRCancel(tr("Specify ellipse center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetMajor:
            updatePromptTRBack(tr("Specify endpoint of major axis"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetMinor:
            updatePromptTRBack(tr("Specify endpoint or length of minor axis:"));
            break;
        case SetAngle1:
            updatePromptTRBack(tr("Specify start angle"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle2:
            updatePromptTRBack(tr("Specify end angle"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}

LC_ActionOptionsWidget* RS_ActionDrawEllipseAxis::createOptionsWidget() {
    if (m_actionData->isArc) {
        return new LC_EllipseArcOptionsWidget();
    }
    return nullptr;
}

LC_ActionOptionsPropertiesFiller* RS_ActionDrawEllipseAxis::createOptionsFiller() {
    if (m_actionData->isArc) {
        return new LC_EllipseArcOptionsFiller();
    }
    return nullptr;
}
