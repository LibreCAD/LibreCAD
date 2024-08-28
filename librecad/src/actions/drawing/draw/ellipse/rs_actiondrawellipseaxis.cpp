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

#include <QMouseEvent>

#include "rs_actiondrawellipseaxis.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "qg_arcoptions.h"
#include "lc_ellipsearcoptions.h"

struct RS_ActionDrawEllipseAxis::Points {
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
RS_ActionDrawEllipseAxis::RS_ActionDrawEllipseAxis(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView,
		bool isArc)
	:LC_ActionDrawCircleBase("Draw ellipse with axis",
                               container, graphicView)
    ,pPoints(std::make_unique<Points>()){
    pPoints->isArc = isArc;
    pPoints->angle2 = isArc ? 2. * M_PI : 0.;
    this->actionType = isArc ? RS2::ActionDrawEllipseArcAxis : RS2::ActionDrawEllipseAxis;
}

RS_ActionDrawEllipseAxis::~RS_ActionDrawEllipseAxis() = default;

void RS_ActionDrawEllipseAxis::init(int status){
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter){
        pPoints->center = {};
    }
    if (status <= SetMajor){
        pPoints->m_vMajorP = {};
    }
    if (status <= SetMinor){
        pPoints->ratio = 0.5;
    }
    if (status <= SetAngle1){
        pPoints->angle1 = 0.0;
    }
    if (status <= SetAngle2){
        pPoints->angle2 = 0.0;
    }
}

void RS_ActionDrawEllipseAxis::trigger(){
    RS_PreviewActionInterface::trigger();

    auto *ellipse = new RS_Ellipse{container,
                                   {pPoints->center, pPoints->m_vMajorP, pPoints->ratio,
                                    pPoints->angle1, pPoints->angle2, pPoints->reversed}
    };
    if (pPoints->ratio > 1.){
        ellipse->switchMajorMinor();
    }
    ellipse->setLayerToActive();
    ellipse->setPenToActive();

    container->addEntity(ellipse);

    addToDocumentUndoable(ellipse);

    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        rz = ellipse->getCenter();
    }
    moveRelativeZero(rz);
    drawSnapper();

    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawEllipseAxis::trigger():"
                    " entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseAxis::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawEllipseAxis::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetCenter: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetMajor: {
            deletePreview();
            if (pPoints->center.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);

                previewEllipse({pPoints->center, mouse - pPoints->center, 0.5, 0.0,
                                pPoints->isArc ? 2. * M_PI : 0., false});

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->center);
                    previewRefLine(pPoints->center, mouse);
                    previewRefSelectablePoint(mouse);
                }
            }
            drawPreview();
            break;
        }
        case SetMinor: {
            if (pPoints->center.valid && pPoints->m_vMajorP.valid){
                deletePreview();
                RS_Vector &center = pPoints->center;
                const RS_Vector &major1Point = center - pPoints->m_vMajorP;
                const RS_Vector &major2Point = center + pPoints->m_vMajorP;
                RS_Line line{major1Point, major2Point};
                double d = line.getDistanceToPoint(mouse);
                pPoints->ratio = d / (line.getLength() / 2);
                auto ellipse = previewEllipse({center, pPoints->m_vMajorP, pPoints->ratio,
                                               0., pPoints->isArc ? 2. * M_PI : 0., false});

                if (showRefEntitiesOnPreview) {
                    previewEllipseReferencePoints(ellipse, true, false, mouse);
                }
                drawPreview();
            }
            break;
        }
        case SetAngle1: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);
            if (pPoints->center.valid && pPoints->m_vMajorP.valid){

                //angle1 = center.angleTo(mouse);

                RS_Vector m = mouse;
                m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
                RS_Vector v = m - pPoints->center;
                v.y /= pPoints->ratio;
                pPoints->angle1 = v.angle(); // + m_vMajorP.angle();

                previewRefLine(pPoints->center, mouse);

                auto ellipse = previewEllipse({pPoints->center, pPoints->m_vMajorP, pPoints->ratio,
                                               pPoints->angle1, pPoints->angle1 + 1.0, pPoints->reversed});

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->center);
                    previewRefSelectablePoint(ellipse->getStartpoint());
                    previewEllipseReferencePoints(ellipse, false, true, mouse);
                }
            }
            drawPreview();
            break;
        }
        case SetAngle2: {
            deletePreview();
            if (pPoints->center.valid && pPoints->m_vMajorP.valid){ // todo - redundant check
                //angle2 = center.angleTo(mouse);
                mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);

                RS_Vector m = mouse;
                m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
                RS_Vector v = m - pPoints->center;
                v.y /= pPoints->ratio;
                pPoints->angle2 = v.angle(); // + m_vMajorP.angle();

                auto ellipse = previewEllipse({pPoints->center, pPoints->m_vMajorP, pPoints->ratio, pPoints->angle1, pPoints->angle2, pPoints->reversed});

                if (showRefEntitiesOnPreview) {
                    previewRefLine(pPoints->center, mouse);
                    previewRefPoint(pPoints->center);
                    auto point = pPoints->center + RS_Vector{pPoints->angle1}.scale(
                        {ellipse->getMajorRadius(), /*-*/ellipse->getMinorRadius()});
                    point.rotate(pPoints->center, /*-*/ pPoints->m_vMajorP.angle());
                    previewRefPoint(point);
                    previewRefSelectablePoint(ellipse->getEndpoint());
                    previewEllipseReferencePoints(ellipse, false, true, mouse);
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawEllipseAxis::mouseMoveEvent end");
}

void RS_ActionDrawEllipseAxis::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status){
        case SetMajor:
        case SetAngle1:
        case SetAngle2:{
            snap = getSnapAngleAwarePoint(e, pPoints->center, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipseAxis::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseAxis::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCenter: {
            pPoints->center = mouse;
            moveRelativeZero(mouse);
            setStatus(SetMajor);
            break;
        }
        case SetMajor: {
            pPoints->m_vMajorP = mouse - pPoints->center;
            setStatus(SetMinor);
            break;
        }
        case SetMinor: {
            RS_Line line{pPoints->center - pPoints->m_vMajorP, pPoints->center + pPoints->m_vMajorP};
            double d = line.getDistanceToPoint(mouse);
            pPoints->ratio = d / (line.getLength() / 2);
            if (!pPoints->isArc){
                trigger();
                setStatus(SetCenter);
            } else {
                setStatus(SetAngle1);
            }
            break;
        }
        case SetAngle1: {
            //angle1 = center.angleTo(mouse);
            RS_Vector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->angle1 = v.angle();
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            //angle2 = center.angleTo(mouse);
            RS_Vector m = mouse;
            m.rotate(pPoints->center, -pPoints->m_vMajorP.angle());
            RS_Vector v = m - pPoints->center;
            v.y /= pPoints->ratio;
            pPoints->angle2 = v.angle();
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawEllipseAxis::isReversed() const {
    return pPoints->reversed;
}

void RS_ActionDrawEllipseAxis::setReversed(bool b) const {
    pPoints->reversed = b;
}

bool RS_ActionDrawEllipseAxis::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetMinor: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->ratio = m / pPoints->m_vMajorP.magnitude();
                if (!pPoints->isArc){
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
                pPoints->angle1 = RS_Math::deg2rad(a);
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
                pPoints->angle2 = RS_Math::deg2rad(a);
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
    if (pPoints->isArc) {
        return new LC_EllipseArcOptions();
    }
    return nullptr;
}
