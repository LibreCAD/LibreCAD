/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011-2015 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_actiondrawellipsecenter3points.h"

#include "lc_creation_circle.h"
#include "lc_creation_ellipse.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_preview.h"

struct RS_ActionDrawEllipseCenter3Points::ActionData {
    RS_VectorSolutions points;
    RS_CircleData circleData;
    RS_EllipseData ellipseData;
    bool valid{false};
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseCenter3Points::RS_ActionDrawEllipseCenter3Points(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("Draw ellipse by center and 3 points", actionContext, RS2::ActionDrawEllipseCenter3Points),
      m_actionData(std::make_unique<ActionData>()) {
}

RS_ActionDrawEllipseCenter3Points::~RS_ActionDrawEllipseCenter3Points() = default;

void RS_ActionDrawEllipseCenter3Points::init(const int status) {
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter) {
        m_actionData->points.clear();
    }
    drawSnapper();
}

RS_Entity* RS_ActionDrawEllipseCenter3Points::doTriggerCreateEntity() {
    auto* ellipse = new RS_Ellipse(m_document, m_actionData->ellipseData);
    moveRelativeZero(ellipse->getCenter());
    return ellipse;
}

bool RS_ActionDrawEllipseCenter3Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2) || (status == SetPoint3) || (status == SetCenter);
}

void RS_ActionDrawEllipseCenter3Points::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCenter);
}

void RS_ActionDrawEllipseCenter3Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    if (status == SetCenter) {
        trySnapToRelZeroCoordinateEvent(e);
        return;
    }
    m_actionData->points.resize(status);
    m_actionData->points.push_back(mouse);

    if (m_showRefEntitiesOnPreview) {
        for (int i = SetPoint1; i <= status; i++) {
            previewRefPoint(m_actionData->points.at(i - 1));
        }
    }

    previewRefSelectablePoint(mouse);

    if (preparePreview()) {
        switch (status) {
            case SetPoint1: {
                previewToCreateCircle(m_actionData->circleData);
                break;
            }
            case SetPoint2: {
                const auto ellipse = previewToCreateEllipse(m_actionData->ellipseData);
                previewEllipseReferencePoints(ellipse, true, false);
                break;
            }
            case SetPoint3: {
                const auto ellipse = previewToCreateEllipse(m_actionData->ellipseData);
                previewEllipseReferencePoints(ellipse, true, false);
                break;
            }
            default:
                break;
        }
    }
}

bool RS_ActionDrawEllipseCenter3Points::preparePreview() const {
    m_actionData->valid = false;
    switch (getStatus()) {
        case SetPoint1: {
            m_actionData->valid = LC_CreationCircle::createFromCR(m_actionData->points.at(0),
                                                 m_actionData->points.get(0).distanceTo(m_actionData->points.get(1)), m_actionData->circleData);

            if (!m_actionData->valid) {
                m_actionData->circleData = RS_CircleData{};
            }
            break;
        }
        case SetPoint2:
        case SetPoint3: {
            m_actionData->valid = LC_CreationEllipse::createEllipseFromCenter3Points(m_actionData->points, m_actionData->ellipseData);
            break;
        }
        default:
            break;
    }
    return m_actionData->valid;
}

void RS_ActionDrawEllipseCenter3Points::onMouseLeftButtonRelease([[maybe_unused]] int status, const LC_MouseEvent* e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawEllipseCenter3Points::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseCenter3Points::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    m_actionData->points.alloc(status + 1);
    m_actionData->points.set(status, coord);

    switch (getStatus()) {
        case SetCenter: {
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint1);
            break;
        }
        case SetPoint1:
        case SetPoint2:
            for (int i = 0; i < status - 1; i++) {
                if ((coord - m_actionData->points.get(i)).squared() < RS_TOLERANCE15) {
                    return; //refuse to accept points already chosen
                }
            }
            //                setStatus(getStatus()+1);
            //                break;
            // fall-through
            [[fallthrough]];
        case SetPoint3: {
            addSnappedPointToVisualSnap(coord);
            if (preparePreview()) {
                if (status == SetPoint3) {
                    trigger();
                }
                else {
                    setStatus(status + 1);
                }
            }
            break;
        }
        default:
            break;
    }
}

//fixme, support command line

/*
void RS_ActionDrawEllipseCenter3Points::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    default:
        break;
    }
}
*/

QStringList RS_ActionDrawEllipseCenter3Points::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipseCenter3Points::updateActionPrompt() {
    switch (getStatus()) {
        case SetCenter:
            updatePromptTRCancel(tr("Specify the center of ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint1:
            updatePromptTRCancel(tr("Specify the first point on ellipse"));
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify the second point on ellipse"));
            break;
        case SetPoint3:
            updatePromptTRBack(tr("Specify the third point on ellipse"));
            break;
        default:
            updatePrompt();
            break;
    }
}
