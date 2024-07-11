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

#include<cmath>

#include <QMouseEvent>

#include "rs_actiondrawarc.h"
#include "rs_arc.h"
#include "rs_line.h"
#include "rs_circle.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "lc_linemath.h"
#include "rs_actioninterface.h"
#include "qg_arcoptions.h"

// fixme - add proper options for the action
RS_ActionDrawArc::RS_ActionDrawArc(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDrawCircleBase("Draw arcs",container, graphicView), data(std::make_unique<RS_ArcData>()){
    actionType = RS2::ActionDrawArc;
    reset();
}

RS_ActionDrawArc::~RS_ActionDrawArc() = default;

void RS_ActionDrawArc::reset(){
    double angleMin = 0.;
    double angleMax = 2. * M_PI;
    if (data->reversed)
        std::swap(angleMin, angleMax);
    *data = {{}, 0., angleMin, angleMax, data->reversed};
}

void RS_ActionDrawArc::init(int status){
    LC_ActionDrawCircleBase::init(status);
    reset();
}

void RS_ActionDrawArc::trigger(){
    RS_PreviewActionInterface::trigger();

    auto arc = new RS_Arc(container,*data);
    arc->setLayerToActive();
    arc->setPenToActive();
    container->addEntity(arc);

    addToDocumentUndoable(arc);

    graphicView->redraw(RS2::RedrawDrawing);
    moveRelativeZero(arc->getCenter());

    setStatus(SetCenter);
    reset();

    RS_DEBUG->print("RS_ActionDrawArc::trigger(): arc added: %lu", arc->getId());
}

void RS_ActionDrawArc::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawArc::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetCenter: {
            data->center = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetRadius: {
            if (data->center.valid){
                mouse = getFreeSnapAwarePoint(e, mouse);
                data->radius = data->center.distanceTo(mouse);
                deletePreview();
                previewRefPoint(data->center);
                previewRefPoint(mouse);
                previewCircle({data->center, data->radius});
                drawPreview();
            }
            break;
        }
        case SetAngle1: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);

            data->angle1 = data->center.angleTo(mouse);
            if (data->reversed){
                data->angle2 = RS_Math::correctAngle(data->angle1 - M_PI / 3);
            } else {
                data->angle2 = RS_Math::correctAngle(data->angle1 + M_PI / 3);
            }
            previewArc(*data);

            previewRefPoint(data->center);
            RS_Vector startArcPoint = data->center + RS_Vector::polar(data->radius, data->angle1);
            previewRefSelectablePoint(startArcPoint);
            previewRefLine(data->center, mouse);

            drawPreview();
            break;
        }
        case SetAngle2: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);
            data->angle2 = data->center.angleTo(mouse);
            auto arc = previewArc(*data);
            previewRefPoints({data->center, arc->getStartpoint()});
            previewRefSelectablePoint(arc->getEndpoint());
            previewRefLine(data->center, mouse);
            drawPreview();
            break;
        }
        case SetIncAngle: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, data->center, mouse, true);
            double angleToMouse = data->center.angleTo(mouse);
            data->angle2 = data->angle1 + angleToMouse;
            auto arc = previewArc(*data);

            previewRefPoint(data->center);
            previewRefPoint(arc->getStartpoint());
            previewRefPoint(arc->getEndpoint());
            RS_Vector nearest = arc->getNearestPointOnEntity(mouse, false);
            previewRefSelectablePoint(nearest);

            double halfRadius = data->radius/2;
            const RS_Vector &horizontalPoint = data->center + RS_Vector(halfRadius, 0, 0);
            previewRefLine(data->center, mouse);
            previewRefLine(data->center, horizontalPoint);
            previewRefLine(data->center, arc->getEndpoint());
            previewRefLine(data->center, arc->getStartpoint());
            previewRefArc(RS_ArcData(data->center, halfRadius, 0, angleToMouse, data->reversed));
            previewRefArc(RS_ArcData(data->center, halfRadius *1.1 , arc->getAngle1(), arc->getAngle2(), data->reversed));

            drawPreview();
            break;
        }
        case SetChordLength: {
            // todo - add  more relaxed snap... to grid etc???
            RS_Vector arcStart;
            RS_Vector halfCircleArcEnd;
            snapMouseToDiameter(mouse, arcStart, halfCircleArcEnd);
            double distanceFromStartToMouse = arcStart.distanceTo(mouse);

            deletePreview();
            double diameter = data->radius * 2;
            data->angle2 = data->angle1 + asin(distanceFromStartToMouse / diameter) * 2;
            if (LC_LineMath::isMeaningfulDistance(mouse, arcStart)){
                auto arc = previewArc(*data);
                previewRefPoint(arc->getEndpoint());
                previewRefLine(arcStart, mouse);
                previewRefLine(arc->getStartpoint(), arc->getEndpoint());
                if (LC_LineMath::isMeaningfulDistance(mouse, halfCircleArcEnd)){
                    previewRefArc(
                        RS_ArcData(arcStart, distanceFromStartToMouse, arcStart.angleTo(data->center), arcStart.angleTo(arc->getEndpoint()), true));
                }
            }
            previewRefPoint(arcStart);
            previewRefPoint(data->center);
            previewRefSelectablePoint(mouse, true);
            previewRefPoint(halfCircleArcEnd);

            drawPreview();
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawArc::mouseMoveEvent end");
}

void RS_ActionDrawArc::snapMouseToDiameter(RS_Vector &mouse, RS_Vector &arcStart, RS_Vector &halfCircleArcEnd) const{
    arcStart= data->center + RS_Vector::polar(data->radius, data->angle1);
    halfCircleArcEnd= data->center - RS_Vector::polar(data->radius, data->angle1);
    RS_Line diameter = RS_Line(nullptr, RS_LineData(arcStart, halfCircleArcEnd));

    // projection of mouse to diameter
    mouse = diameter.getNearestPointOnEntity(mouse, true);
}

void RS_ActionDrawArc::mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) {
    RS_Vector mouse = snapPoint(e);
    bool shouldFireCoordinateEvent = true;
    switch (status) {
        case SetRadius: {
            mouse = getFreeSnapAwarePoint(e, mouse);
            break;
        }
        case SetAngle1:
        case SetAngle2:
        case SetIncAngle: {
            mouse = getSnapAngleAwarePoint(e, data->center, mouse);
            break;
        }
        case SetChordLength: {
            RS_Vector arcStart;
            RS_Vector halfCircleArcEnd;
            snapMouseToDiameter(mouse, arcStart, halfCircleArcEnd);
            shouldFireCoordinateEvent = LC_LineMath::isMeaningfulDistance(mouse, arcStart);
            if (!shouldFireCoordinateEvent){
                commandMessageTR("Length of chord should be non-zero");
            }
            break;
        }
        default:
            break;
    }
    if (shouldFireCoordinateEvent){
        fireCoordinateEvent(mouse);
    }
}

void RS_ActionDrawArc::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    if (status == SetChordLength){
        moveRelativeZero(data->center);
        setStatus(SetAngle2);
    }
    else{
        setStatus(status - 1);
    }
}

void RS_ActionDrawArc::coordinateEvent(RS_CoordinateEvent *e){
    if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
        case SetCenter: {
            data->center = mouse;
            moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;
        }
        case SetRadius: {
            if (data->center.valid){
                data->radius = data->center.distanceTo(mouse);
            }
            setStatus(SetAngle1);
            break;
        }
        case SetAngle1: {
            data->angle1 = data->center.angleTo(mouse);
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            data->angle2 = data->center.angleTo(mouse);
            trigger();
            break;
        }
        case SetIncAngle: {
            data->angle2 = data->angle1 + data->center.angleTo(mouse);
            trigger();
            break;
        }
        case SetChordLength: {
            // todo - double calculation of arc start - store it for later use?
            RS_Vector arcStart= data->center + RS_Vector::polar(data->radius, data->angle1);
            double distanceFromStartToMouse = arcStart.distanceTo(mouse);
            double diameter = 2 * data->radius;
            if (fabs(distanceFromStartToMouse / diameter) <= 1.0){
                data->angle2 = data->angle1 + asin(distanceFromStartToMouse / diameter) * 2;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawArc::commandEvent(RS_CommandEvent *e){
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)){
        commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
        return;
    }

    if (checkCommand("reversed", c)){
        e->accept();
        setReversed(!isReversed());

        updateOptions();
        return;
    }

    switch (getStatus()) {

        case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok){
                data->radius = r;
                setStatus(SetAngle1);
                e->accept();
            } else
                commandMessageTR("Not a valid expression");
            break;
        }
        case SetAngle1: {
            bool ok = false;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                data->angle1 = RS_Math::deg2rad(a);
                e->accept();
                setStatus(SetAngle2);
            } else
                commandMessageTR("Not a valid expression");
            break;
        }
        case SetAngle2: {
            if (checkCommand("angle", c)){
                setStatus(SetIncAngle);
            } else if (checkCommand("chordlen", c)){
                RS_Vector arcStart = data->center + RS_Vector::polar(data->radius, data->angle1);
                moveRelativeZero(arcStart);
                setStatus(SetChordLength);
            } else {
                bool ok = false;
                double a = RS_Math::eval(c, &ok);
                if (ok){
                    data->angle2 = RS_Math::deg2rad(a);
                    e->accept();
                    trigger();
                } else
                    commandMessageTR("Not a valid expression");
            }
            break;
        }
        case SetIncAngle: {
            bool ok = false;
            double a = RS_Math::eval(c, &ok);
            if (ok){
                data->angle2 = data->angle1 + RS_Math::deg2rad(a);
                e->accept();
                trigger();
            } else
                commandMessageTR("Not a valid expression");
            break;
        }
        case SetChordLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok){
                if (fabs(l / (2 * data->radius)) <= 1.0){
                    data->angle2 = data->angle1 + asin(l / (2 * data->radius)) * 2;
                    trigger();
                } else
                    commandMessageTR("Not a valid chord length");
                e->accept();
            } else
                commandMessageTR("Not a valid expression");
            break;
        }
        default:
            break;
    }
}

QStringList RS_ActionDrawArc::getAvailableCommands() {
    return {{"reversed"}};
}

void RS_ActionDrawArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel("Specify center", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            updateMouseWidgetTRBack("Specify radius", MOD_SHIFT_FREE_SNAP);
            break;
        case SetAngle1:
            updateMouseWidgetTRBack("Specify start angle:", MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle2:
            updateMouseWidgetTRBack("Specify end angle or [angle/chordlen]", MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetIncAngle:
            updateMouseWidgetTRBack("Specify included angle:", MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetChordLength:
            updateMouseWidgetTRBack("Specify chord length:");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawArc::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

bool RS_ActionDrawArc::isReversed() const{
    return data->reversed;
}

void RS_ActionDrawArc::setReversed(bool r) const{
    data->reversed = r;
}

void RS_ActionDrawArc::createOptionsWidget(){
    m_optionWidget = std::make_unique<QG_ArcOptions>();
}

// EOF

