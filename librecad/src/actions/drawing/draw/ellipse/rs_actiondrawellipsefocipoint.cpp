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

#include "rs_document.h"
#include "rs_ellipse.h"

struct RS_ActionDrawEllipseFociPoint::ActionData {
    // Foci of ellipse
    RS_Vector focus1;
    RS_Vector focus2;
    // A point on ellipse
    RS_Vector point;
    RS_Vector center;
    RS_Vector major;
    double halfDistanceFoci = 0.; //hold half of distance between foci
    double halfDistance = 0.; //hold half of distance
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseFociPoint::RS_ActionDrawEllipseFociPoint(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("Draw ellipse by foci and a point", actionContext, RS2::ActionDrawEllipseFociPoint),
      m_actionData(std::make_unique<ActionData>()) {
}

RS_ActionDrawEllipseFociPoint::~RS_ActionDrawEllipseFociPoint() = default;

void RS_ActionDrawEllipseFociPoint::init(const int status) {
    LC_ActionDrawCircleBase::init(status);

    if (status == SetFocus1) {
        m_actionData->focus1.valid = false;
    }
}

double RS_ActionDrawEllipseFociPoint::findRatio() const {
    return std::sqrt(
            m_actionData->halfDistance * m_actionData->halfDistance - m_actionData->halfDistanceFoci * m_actionData->halfDistanceFoci) /
        m_actionData->halfDistance;
}

RS_Entity* RS_ActionDrawEllipseFociPoint::doTriggerCreateEntity() {
    auto* ellipse = new RS_Ellipse{
        m_document,
        {m_actionData->center, m_actionData->major * m_actionData->halfDistance, findRatio(), 0., 0., false}
    };

    moveRelativeZero(ellipse->getCenter());
    return ellipse;
}

void RS_ActionDrawEllipseFociPoint::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetFocus1);
}

bool RS_ActionDrawEllipseFociPoint::isInVisualSnapStatus(int status) {
    return (status == SetFocus1) || (status == SetFocus2) || (status == SetPoint);
}

void RS_ActionDrawEllipseFociPoint::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
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
            m_actionData->halfDistance = 0.5 * (m_actionData->focus1.distanceTo(m_actionData->point) + m_actionData->focus2.distanceTo(
                m_actionData->point));
            if (m_actionData->halfDistance > m_actionData->halfDistanceFoci + RS_TOLERANCE) {
                const auto ellipse = previewToCreateEllipse({
                    m_actionData->center,
                    m_actionData->major * m_actionData->halfDistance,
                    findRatio(),
                    0.,
                    0.,
                    false
                });
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

void RS_ActionDrawEllipseFociPoint::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetFocus2) {
        snap = getSnapAngleAwarePoint(e, m_actionData->focus1, snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipseFociPoint::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseFociPoint::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetFocus1: {
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            m_actionData->focus1 = coord;
            setStatus(SetFocus2);
            break;
        }
        case SetFocus2: {
            m_actionData->halfDistanceFoci = 0.5 * m_actionData->focus1.distanceTo(coord);
            if (m_actionData->halfDistanceFoci > RS_TOLERANCE) {
                m_actionData->focus2 = coord;
                m_actionData->center = (m_actionData->focus1 + m_actionData->focus2) * 0.5;
                m_actionData->major = m_actionData->focus1 - m_actionData->center;
                m_actionData->major /= m_actionData->halfDistanceFoci;
                addSnappedPointToVisualSnap(coord);
                moveRelativeZero(m_actionData->center);
                setStatus(SetPoint);
            }
            break;
        }
        case SetPoint: {
            m_actionData->point = coord;
            m_actionData->halfDistance = 0.5 * (m_actionData->focus1.distanceTo(m_actionData->point) + m_actionData->focus2.distanceTo(
                m_actionData->point));
            if (m_actionData->halfDistance > m_actionData->halfDistanceFoci + RS_TOLERANCE) {
                addSnappedPointToVisualSnap(coord);
                moveRelativeZero(coord);
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

bool RS_ActionDrawEllipseFociPoint::doProcessCommand(const int status, const QString& command) {
    bool accept = false;

    if (status == SetPoint) {
        bool ok;
        const double a = RS_Math::eval(command, &ok);
        if (ok) {
            m_actionData->halfDistance = 0.5 * fabs(a);
            accept = true;
            if (m_actionData->halfDistance > m_actionData->halfDistanceFoci + RS_TOLERANCE) {
                trigger();
            }
            else {
                commandMessage(tr("Total distance %1 is smaller than distance between foci").arg(fabs(a)));
            }
        }
        else {
            commandMessage(tr("Not a valid expression"));
        }
    }
    return accept;
}

QStringList RS_ActionDrawEllipseFociPoint::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipseFociPoint::updateActionPrompt() {
    switch (getStatus()) {
        case SetFocus1:
            updatePromptTRCancel(tr("Specify first focus of ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetFocus2:
            updatePromptTRBack(tr("Specify second focus of ellipse"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint:
            updatePromptTRBack(tr("Specify a point on ellipse or total distance to foci"));
            break;
        default:
            updatePrompt();
            break;
    }
}
