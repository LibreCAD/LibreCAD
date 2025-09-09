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

#include "rs_actiondrawellipseaxis.h"

#include "lc_ellipsearcoptions.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_line.h"

struct RS_ActionDrawEllipseAxis::ActionData {
	/** Center of ellipse */
	RS_Vector center;
	/** Endpoint of major axis */
	RS_Vector m_vMajorP;
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
 * @param isArc true if this action will produce an ellipse arc.
 *              false if it will produce a full ellipse.
 */
RS_ActionDrawEllipseAxis::RS_ActionDrawEllipseAxis(LC_ActionContext *actionContext, bool isArc)
	:LC_ActionDrawCircleBase("Draw ellipse with axis",actionContext, isArc ? RS2::ActionDrawEllipseArcAxis : RS2::ActionDrawEllipseAxis)
    ,m_actionData(std::make_unique<ActionData>()){
    m_actionData->isArc = isArc;
    m_actionData->angle2 = isArc ? 2. * M_PI : 0.;
}

RS_ActionDrawEllipseAxis::~RS_ActionDrawEllipseAxis() = default;

void RS_ActionDrawEllipseAxis::init(int status){
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter){
        m_actionData->center = {};
    }
    if (status <= SetMajor){
        m_actionData->m_vMajorP = {};
    }
    if (status <= SetMinor){
        m_actionData->ratio = 0.5;
    }
    if (status <= SetAngle1){
        m_actionData->angle1 = 0.0;
    }
    if (status <= SetAngle2){
        m_actionData->angle2 = 0.0;
    }
}

void RS_ActionDrawEllipseAxis::doTrigger() {
    auto *ellipse = new RS_Ellipse{m_container,
                                   {m_actionData->center, m_actionData->m_vMajorP, m_actionData->ratio,
                                    m_actionData->angle1, m_actionData->angle2, m_actionData->reversed}
    };
    if (m_actionData->ratio > 1.){
        ellipse->switchMajorMinor();
    }
    setPenAndLayerToActive(ellipse);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(ellipse->getCenter());
    }

    undoCycleAdd(ellipse);
    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawEllipseAxis::trigger():entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseAxis::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (getStatus()) {
        case SetCenter: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetMajor: {
            if (m_actionData->center.valid){
                mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);

                previewToCreateEllipse({m_actionData->center, mouse - m_actionData->center, 0.5, 0.0,
                                m_actionData->isArc ? 2. * M_PI : 0., false});

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->center);
                    previewRefLine(m_actionData->center, mouse);
                    previewRefSelectablePoint(mouse);
                }
            }
            break;
        }
        case SetMinor: {
            if (m_actionData->center.valid && m_actionData->m_vMajorP.valid){
                RS_Vector &center = m_actionData->center;
                const RS_Vector &major1Point = center - m_actionData->m_vMajorP;
                const RS_Vector &major2Point = center + m_actionData->m_vMajorP;
                RS_Line line{major1Point, major2Point};
                double d = line.getDistanceToPoint(mouse);
                m_actionData->ratio = d / (line.getLength() / 2);
                auto ellipse = previewToCreateEllipse({center, m_actionData->m_vMajorP, m_actionData->ratio,
                                               0., m_actionData->isArc ? 2. * M_PI : 0., false});

                if (m_showRefEntitiesOnPreview) {
                    previewEllipseReferencePoints(ellipse, true, false, mouse);
                }
            }
            break;
        }
        case SetAngle1: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);
            if (m_actionData->center.valid && m_actionData->m_vMajorP.valid){
                //angle1 = center.angleTo(mouse);
                RS_Vector m = mouse;
                m.rotate(m_actionData->center, -m_actionData->m_vMajorP.angle());
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle1 = v.angle(); // + m_vMajorP.angle();

                previewRefLine(m_actionData->center, mouse);

                auto ellipse = previewToCreateEllipse({m_actionData->center, m_actionData->m_vMajorP, m_actionData->ratio,
                                               m_actionData->angle1, m_actionData->angle1 + 1.0, m_actionData->reversed});

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->center);
                    previewRefSelectablePoint(ellipse->getStartpoint());
                    previewEllipseReferencePoints(ellipse, false, true, mouse);
                }
            }
            break;
        }
        case SetAngle2: {
            if (m_actionData->center.valid && m_actionData->m_vMajorP.valid){ // todo - redundant check
                //angle2 = center.angleTo(mouse);
                mouse = getSnapAngleAwarePoint(e, m_actionData->center, mouse, true);

                RS_Vector m = mouse;
                m.rotate(m_actionData->center, -m_actionData->m_vMajorP.angle());
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle2 = v.angle(); // + m_vMajorP.angle();

                auto ellipse = previewToCreateEllipse({m_actionData->center, m_actionData->m_vMajorP,
                    m_actionData->ratio, m_actionData->angle1, m_actionData->angle2, m_actionData->reversed});

                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(m_actionData->center, mouse);
                    previewRefPoint(m_actionData->center);
                    auto point = m_actionData->center + RS_Vector{m_actionData->angle1}.scale(
                        {ellipse->getMajorRadius(), /*-*/ellipse->getMinorRadius()});
                    point.rotate(m_actionData->center, /*-*/ m_actionData->m_vMajorP.angle());
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

void RS_ActionDrawEllipseAxis::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetMajor:
        case SetAngle1:
        case SetAngle2:{
            snap = getSnapAngleAwarePoint(e, m_actionData->center, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipseAxis::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseAxis::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCenter: {
            m_actionData->center = mouse;
            moveRelativeZero(mouse);
            setStatus(SetMajor);
            break;
        }
        case SetMajor: {
            m_actionData->m_vMajorP = mouse - m_actionData->center;
            setStatus(SetMinor);
            break;
        }
        case SetMinor: {
            RS_Line line{m_actionData->center - m_actionData->m_vMajorP, m_actionData->center + m_actionData->m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            m_actionData->ratio = d / (line.getLength() / 2);
            if (!m_actionData->isArc){
                trigger();
                setStatus(SetCenter);
            } else {
                setStatus(SetAngle1);
            }
            break;
        }
        case SetAngle1: {
            if (isZero){
                m_actionData->angle1 = 0;
            }
            else {
                RS_Vector m = mouse;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->m_vMajorP.angle()));
                RS_Vector v = m - m_actionData->center;
                v.y /= m_actionData->ratio;
                m_actionData->angle1 = v.angle();
            }
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            if (isZero){
                m_actionData->angle2 = 0;
            }
            else {
                RS_Vector m = mouse;
                m.rotate(m_actionData->center, toUCSAngle(-m_actionData->m_vMajorP.angle()));
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

void RS_ActionDrawEllipseAxis::setReversed(bool b) const {
    m_actionData->reversed = b;
}

// fixme - sand - complete support of command line!
bool RS_ActionDrawEllipseAxis::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetMinor: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                m_actionData->ratio = m / m_actionData->m_vMajorP.magnitude();
                if (!m_actionData->isArc){
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else
                commandMessage(tr("Not a valid expression"));
            break;
        }
        case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                m_actionData->angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else
                commandMessage(tr("Not a valid expression"));
            break;
        }
        case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                m_actionData->angle2 = RS_Math::deg2rad(a);
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

void RS_ActionDrawEllipseAxis::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel(tr("Specify ellipse center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetMajor:
            updateMouseWidgetTRBack(tr("Specify endpoint of major axis"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetMinor:
            updateMouseWidgetTRBack(tr("Specify endpoint or length of minor axis:"));
            break;
        case SetAngle1:
            updateMouseWidgetTRBack(tr("Specify start angle"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle2:
            updateMouseWidgetTRBack(tr("Specify end angle"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget *RS_ActionDrawEllipseAxis::createOptionsWidget() {
    if (m_actionData->isArc) {
        return new LC_EllipseArcOptions();
    }
    return nullptr;
}
