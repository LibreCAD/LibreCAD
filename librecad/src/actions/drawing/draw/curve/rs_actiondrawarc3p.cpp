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
#include "rs_actiondrawarc3p.h"

#include "rs_arc.h"

/**
 * Arc data defined so far.
 */
struct RS_ActionDrawArc3P::ActionData {
    RS_ArcData data;
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
    /**
     * 3rd point.
     */
    RS_Vector point3;
};

RS_ActionDrawArc3P::RS_ActionDrawArc3P(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Draw arcs 3P", actionContext,  RS2::ActionDrawArc3P)
    , m_actionData{std::make_unique<RS_ActionDrawArc3P::ActionData>()}{
}

RS_ActionDrawArc3P::~RS_ActionDrawArc3P() = default;

void RS_ActionDrawArc3P::reset() {
}

void RS_ActionDrawArc3P::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    //reset();
}

void RS_ActionDrawArc3P::doTrigger() {
    preparePreview(m_alternatedPoints);
    if (m_actionData->data.isValid()){
        auto *arc = new RS_Arc{m_container, m_actionData->data};

        setPenAndLayerToActive(arc);

        RS_Vector rz = arc->getEndpoint();
        if (m_moveRelPointAtCenterAfterTrigger){
            rz = arc->getCenter();
        }
        moveRelativeZero(rz);

        undoCycleAdd(arc);

        m_alternatedPoints = false;
        setStatus(SetPoint1);
        reset();
    } else {
        //RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid arc data."));
        commandMessage(tr("Invalid arc data."));
    }
}

void RS_ActionDrawArc3P::preparePreview(bool alternatePoints){
    if (m_actionData->point1.valid && m_actionData->point2.valid && m_actionData->point3.valid){
        RS_Arc arc(nullptr, m_actionData->data);
        RS_Vector &middlePoint = m_actionData->point2;
        RS_Vector &startPoint = m_actionData->point1;
        RS_Vector &endPoint = m_actionData->point3;
        bool suc;
        if (alternatePoints){
            suc = arc.createFrom3P(startPoint, endPoint, middlePoint);
        }
        else {
            suc = arc.createFrom3P(startPoint, middlePoint, endPoint);
        }
        if (suc){
            m_actionData->data = arc.getData();
        }
    }
}

void RS_ActionDrawArc3P::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;

    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            m_actionData->point2 = mouse;
            if (m_actionData->point1.valid) { // todo - redundant check
                previewLine(m_actionData->point1, m_actionData->point2);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->point1);
                    previewRefSelectablePoint(m_actionData->point2);
                }
            }
            break;
        }
        case SetPoint3: {
            // todo - which point (1 or 2) is more suitable there for snap?
            mouse = getSnapAngleAwarePoint(e,m_actionData->point1, mouse, true);
            m_actionData->point3 = mouse;
            bool alternatePoints = e->isControl || m_alternatedPoints;
            preparePreview(alternatePoints);
            if (m_actionData->data.isValid()){
                previewToCreateArc(m_actionData->data);

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->data.center);
                    previewRefPoint(m_actionData->point1);
                    previewRefPoint(m_actionData->point2);
                    previewRefSelectablePoint(m_actionData->point3);

                    if (alternatePoints){
                        previewRefLine(m_actionData->point1, m_actionData->point2);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawArc3P::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetPoint2:{
            snap = getSnapAngleAwarePoint(e, m_actionData->point1, snap);
            break;
        }
        case SetPoint3:{
            snap = getSnapAngleAwarePoint(e, m_actionData->point1, snap);
            if (e->isControl){
               m_alternatedPoints = true;
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawArc3P::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    setStatus(status-1);
}

void RS_ActionDrawArc3P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            m_actionData->point2 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3: {
            m_actionData->point3 = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawArc3P::doProcessCommand([[maybe_unused]]int status, const QString &c) {
    bool accept = false;
    if (checkCommand("center", c, rtti())) {
        accept = true;
        finish(false);
        switchToAction(RS2::ActionDrawArc);
    }
    // fixme - sand - add these to commands
    else if (checkCommand("altpoint", c, rtti())){
        accept = true;
        m_alternatedPoints = true;
    }
    else if (checkCommand("normpoint", c, rtti())){
        accept = true;
        m_alternatedPoints = false;
    }
    return accept;
}

QStringList RS_ActionDrawArc3P::getAvailableCommands() {
    return {{"center", "altpoint", "normpoint"}};
}

void RS_ActionDrawArc3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        updateMouseWidgetTRCancel(tr("Specify startpoint or [center]"), MOD_SHIFT_RELATIVE_ZERO);
        break;
    case SetPoint2:
        updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
        break;
    case SetPoint3:
        updateMouseWidgetTRBack(tr("Specify third point"), MOD_SHIFT_AND_CTRL_ANGLE("Second point was endpoint"));
        break;
    default:
        updateMouseWidget();
        break;
    }
}
