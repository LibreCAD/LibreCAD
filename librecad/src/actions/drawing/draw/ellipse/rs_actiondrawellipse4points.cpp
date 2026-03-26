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

#include "rs_actiondrawellipse4points.h"

#include "lc_creation_circle.h"
#include "lc_creation_ellipse.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_preview.h"

struct RS_ActionDrawEllipse4Points::ActionData {
    RS_VectorSolutions points;
    RS_CircleData circleData;
    RS_EllipseData ellipseData;
    bool valid = false;
    bool evalid = false;
    bool bUniqueEllipse{false}; //a message of non-unique ellipse is shown
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipse4Points::RS_ActionDrawEllipse4Points(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("Draw ellipse from 4 points", actionContext, RS2::ActionDrawEllipse4Points),
      m_actionData(std::make_unique<ActionData>()) {
}

RS_ActionDrawEllipse4Points::~RS_ActionDrawEllipse4Points() = default;

void RS_ActionDrawEllipse4Points::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    if (getStatus() == SetPoint1) {
        m_actionData->points.clear();
    }
}

RS_Entity* RS_ActionDrawEllipse4Points::doTriggerCreateEntity() {
    RS_Entity* en;
    if (getStatus() == SetPoint4 && m_actionData->evalid) {
        en = new RS_Ellipse(m_document, m_actionData->ellipseData);
    }
    else {
        en = new RS_Circle(m_document, m_actionData->circleData);
    }
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(en->getCenter());
    }
    return en;
}

bool RS_ActionDrawEllipse4Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2) || (status == SetPoint3) || (status == SetPoint4);
}

void RS_ActionDrawEllipse4Points::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetPoint1);
}

void RS_ActionDrawEllipse4Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    if (status == SetPoint1) {
        trySnapToRelZeroCoordinateEvent(e);
    }

    if (m_showRefEntitiesOnPreview) {
        for (int i = SetPoint2; i <= status; i++) {
            previewRefPoint(m_actionData->points.at(i - 1));
        }
    }

    if (status == SetPoint2) {
        mouse = getSnapAngleAwarePoint(e, m_actionData->points.at(SetPoint1), mouse, true);
    }

    m_actionData->points.set(status, mouse);
    if (preparePreview()) {
        switch (status) {
            case SetPoint2: {
                break;
            }
            case SetPoint3: {
                if (m_actionData->valid) {
                    previewToCreateCircle(m_actionData->circleData);

                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->circleData.center);
                    }
                }
                break;
            }
            case SetPoint4: {
                if (m_actionData->evalid) {
                    const auto ellipse = previewToCreateEllipse(m_actionData->ellipseData);
                    if (m_showRefEntitiesOnPreview) {
                        previewEllipseReferencePoints(ellipse, true);
                        previewRefSelectablePoint(mouse);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void RS_ActionDrawEllipse4Points::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetPoint2) {
        snap = getSnapAngleAwarePoint(e, m_actionData->points.at(SetPoint1), snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipse4Points::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionDrawEllipse4Points::preparePreview() const {
    m_actionData->valid = false;
    switch (getStatus()) {
        case SetPoint2:
        case SetPoint3: {
            m_actionData->valid = LC_CreationCircle::createFrom3P(m_actionData->points, m_actionData->circleData);
            if (!m_actionData->valid) {
                m_actionData->circleData = RS_CircleData{};
            }
            break;
        }
        case SetPoint4: {
            m_actionData->evalid = false;
            if ((m_actionData->points.get(SetPoint4) - m_actionData->points.get(SetPoint3)).squared() < RS_TOLERANCE15) {
                m_actionData->valid = LC_CreationCircle::createFrom3P(m_actionData->points,m_actionData->circleData);
                if (!m_actionData->valid) {
                    m_actionData->circleData = RS_CircleData{};
                }
            }
            else {
                m_actionData->valid = LC_CreationEllipse::createEllipseFrom4P(m_actionData->points, m_actionData->ellipseData);
                if (m_actionData->valid) {
                    m_actionData->evalid = m_actionData->valid;
                    m_actionData->bUniqueEllipse = false;
                }
                else {
                    m_actionData->evalid = false;
                    if (m_actionData->bUniqueEllipse == false) {
                        commandMessage(tr("Can not determine uniquely an ellipse"));
                        m_actionData->bUniqueEllipse = true;
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return m_actionData->valid;
}

void RS_ActionDrawEllipse4Points::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    m_actionData->points.alloc(status + 1);
    m_actionData->points.set(status, coord);

    switch (status) {
        case SetPoint1: {
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:
        case SetPoint3:
        case SetPoint4: {
            if (preparePreview()) {
                addSnappedPointToVisualSnap(coord);
                moveRelativeZero(coord);
                if (status == SetPoint4 || (m_actionData->points.get(status) - m_actionData->points.get(status - 1)).squared() <
                    RS_TOLERANCE15) {
                    //also draw the entity, if clicked on the same point twice
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
void RS_ActionDrawEllipse4Point::commandEvent(RS_CommandEvent* e) {
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
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    default:
        break;
    }
}
*/

QStringList RS_ActionDrawEllipse4Points::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipse4Points::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint1:
            updatePromptTRCancel(tr("Specify the first point on ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify the second point on ellipse"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint3:
            updatePromptTRBack(tr("Specify the third point on ellipse"));
            break;
        case SetPoint4:
            updatePromptTRBack(tr("Specify the fourth point on ellipse"));
            break;
        default:
            updatePrompt();
            break;
    }
}
