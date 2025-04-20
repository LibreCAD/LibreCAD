/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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

#include "rs_actiondrawellipsefocipoint.h"

#include "rs_debug.h"
#include "rs_ellipse.h"

struct RS_ActionDrawEllipseFociPoint::ActionData {
	// Foci of ellipse
	RS_Vector focus1,focus2;
	// A point on ellipse
	RS_Vector point;
	RS_Vector center,major;
    double c = 0.; //hold half of distance between foci
    double d = 0.; //hold half of distance
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseFociPoint::RS_ActionDrawEllipseFociPoint(LC_ActionContext *actionContext)
        :LC_ActionDrawCircleBase("Draw ellipse by foci and a point", actionContext,RS2::ActionDrawEllipseFociPoint)
    , m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawEllipseFociPoint::~RS_ActionDrawEllipseFociPoint() = default;

void RS_ActionDrawEllipseFociPoint::init(int status){
    LC_ActionDrawCircleBase::init(status);

    if (status == SetFocus1){
        m_actionData->focus1.valid = false;
    }
}

double RS_ActionDrawEllipseFociPoint::findRatio() const{
    return std::sqrt(m_actionData->d*m_actionData->d-m_actionData->c*m_actionData->c)/m_actionData->d;
}

void RS_ActionDrawEllipseFociPoint::doTrigger() {
    auto* ellipse = new RS_Ellipse{m_container,
                                   {m_actionData->center,
                                    m_actionData->major*m_actionData->d,
                                    findRatio(),
                                    0., 0.,false}
    };
    setPenAndLayerToActive(ellipse);

    moveRelativeZero(ellipse->getCenter());
    undoCycleAdd(ellipse);
    setStatus(SetFocus1);

    RS_DEBUG->print("RS_ActionDrawEllipseFociPoint::trigger():entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseFociPoint::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;

    switch (status) {
        case SetFocus1:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetFocus2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->focus1, mouse, true);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->focus1);
                previewRefSelectablePoint(mouse);
                previewLine(m_actionData->focus1, mouse);
                previewRefLine(m_actionData->focus1, mouse);
            }
            break;
        }
        case SetPoint: {
            m_actionData->point = mouse;
            m_actionData->d = 0.5 * (m_actionData->focus1.distanceTo(m_actionData->point) +
                                m_actionData->focus2.distanceTo(m_actionData->point));
            if (m_actionData->d > m_actionData->c + RS_TOLERANCE){
                auto ellipse = previewToCreateEllipse({m_actionData->center, m_actionData->major * m_actionData->d, findRatio(), 0., 0., false});
                previewEllipseReferencePoints(ellipse, true, false, mouse);
            }

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->focus1);
                previewRefPoint(m_actionData->focus2);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawEllipseFociPoint::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetFocus2){
        snap = getSnapAngleAwarePoint(e, m_actionData->focus1, snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipseFociPoint::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseFociPoint::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetFocus1: {
            moveRelativeZero(mouse);
            m_actionData->focus1 = mouse;
            setStatus(SetFocus2);
            break;
        }
        case SetFocus2: {
            m_actionData->c = 0.5 * m_actionData->focus1.distanceTo(mouse);
            if (m_actionData->c > RS_TOLERANCE){
                m_actionData->focus2 = mouse;
                m_actionData->center = (m_actionData->focus1 + m_actionData->focus2) * 0.5;
                m_actionData->major = m_actionData->focus1 - m_actionData->center;
                m_actionData->major /= m_actionData->c;
                moveRelativeZero(m_actionData->center);
                setStatus(SetPoint);
            }
            break;
        }
        case SetPoint: {
            m_actionData->point = mouse;
            m_actionData->d = 0.5 * (m_actionData->focus1.distanceTo(m_actionData->point) + m_actionData->focus2.distanceTo(m_actionData->point));
            if (m_actionData->d > m_actionData->c + RS_TOLERANCE){
                moveRelativeZero(mouse);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

QString RS_ActionDrawEllipseFociPoint::getAdditionalHelpMessage() {
    return tr("specify a point on ellipse, or total distance to foci");
}

bool RS_ActionDrawEllipseFociPoint::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    if (status == SetPoint){
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok){
            m_actionData->d = 0.5 * fabs(a);
            accept = true;
            if (m_actionData->d > m_actionData->c + RS_TOLERANCE){
                trigger();
            } else
                commandMessage(tr("Total distance %1 is smaller than distance between foci").arg(fabs(a)));
        } else
            commandMessage(tr("Not a valid expression"));
    }
    return accept;
}

QStringList RS_ActionDrawEllipseFociPoint::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipseFociPoint::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetFocus1:
            updateMouseWidgetTRCancel(tr("Specify first focus of ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetFocus2:
            updateMouseWidgetTRBack(tr("Specify second focus of ellipse"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint:
            updateMouseWidgetTRBack(tr("Specify a point on ellipse or total distance to foci"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
