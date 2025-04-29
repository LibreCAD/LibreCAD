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

#include "rs_actiondrawarc.h"

#include "lc_linemath.h"
#include "qg_arcoptions.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_line.h"

// fixme - sand -  expand actions options widget to support all possible settings (like angle, radius, start angle, end/total angle, chordlen)
RS_ActionDrawArc::RS_ActionDrawArc(LC_ActionContext *actionContext,RS2::ActionType ownActionType)
    :LC_ActionDrawCircleBase("Draw arcs",actionContext, ownActionType), m_arcData(std::make_unique<RS_ArcData>()){
    RS_ActionDrawArc::reset();
}

RS_ActionDrawArc::~RS_ActionDrawArc() = default;

void RS_ActionDrawArc::reset(){
    double angleMin = 0.;
    double angleMax = 2. * M_PI;
    if (m_alternateArcDirection){
        m_arcData->reversed = !m_arcData->reversed;
    }
    if (m_arcData->reversed)
        std::swap(angleMin, angleMax);
    *m_arcData = {{}, 0., angleMin, angleMax, m_arcData->reversed};
    m_alternateArcDirection = false;
}

void RS_ActionDrawArc::init(int status){
    LC_ActionDrawCircleBase::init(status);
    reset();
}

void RS_ActionDrawArc::doTrigger() {
    if (m_alternateArcDirection){
        m_arcData->reversed = !m_arcData->reversed;
    }
    auto arc = new RS_Arc(m_container, *m_arcData);
    setPenAndLayerToActive(arc);
    moveRelativeZero(arc->getCenter());
    undoCycleAdd(arc);
    setStatus(SetCenter);
    reset();
    RS_DEBUG->print("RS_ActionDrawArc::trigger(): arc added: %lu", arc->getId());
}

void RS_ActionDrawArc::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetCenter: {
            m_arcData->center = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetRadius: {
            if (m_arcData->center.valid){
                if (rtti() == RS2::ActionDrawArc) {
                    mouse = getFreeSnapAwarePoint(e, mouse);
                }
                else{
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
            if (m_arcData->reversed){
                m_arcData->angle2 = RS_Math::correctAngle(m_arcData->angle1 - M_PI / 3);
            } else {
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
            RS_ArcData tmpm_arcData = *m_arcData;
            if (alternateDirection) {
                tmpm_arcData.reversed = !tmpm_arcData.reversed;
            }
            auto arc = previewToCreateArc(tmpm_arcData);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoints({m_arcData->center, arc->getStartpoint()});
                previewRefSelectablePoint(arc->getEndpoint());
                previewRefLine(m_arcData->center, mouse);
            }
            break;
        }
        case SetIncAngle: {

            RS_Vector &center = m_arcData->center;
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
            m_arcData->angle2 = m_arcData->angle1 + asin(distanceFromStartToMouse / diameter) * 2;

            RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
            RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);

            RS_ArcData arcDataCopy = *m_arcData;
            bool useAlternativeSolution = e->isShift;
            if (useAlternativeSolution){
                arcDataCopy.angle2 = m_arcData->center.angleTo(alternativePoint);
                arcDataCopy.reversed = !m_arcData->reversed ;
            }
            if (LC_LineMath::isMeaningfulDistance(mouse, arcStart)) {
                auto arc = previewToCreateArc(arcDataCopy);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(arc->getEndpoint());
                    previewRefLine(arcStart, mouse);
                    previewRefLine(arc->getStartpoint(), arc->getEndpoint());

                    if (useAlternativeSolution){
                        previewRefSelectablePoint(endpoint);
                    }
                    else{
                        previewRefSelectablePoint(alternativePoint);
                    }

                    if (LC_LineMath::isMeaningfulDistance(mouse, halfCircleArcEnd)) {
                        previewRefArc(
                            RS_ArcData(arcStart, distanceFromStartToMouse, arcStart.angleTo(m_arcData->center),
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

void RS_ActionDrawArc::snapMouseToDiameter(RS_Vector &mouse, RS_Vector &arcStart, RS_Vector &halfCircleArcEnd) const{
    arcStart= m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
    halfCircleArcEnd= m_arcData->center - RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
    RS_Line diameter = RS_Line(nullptr, RS_LineData(arcStart, halfCircleArcEnd));

    // projection of mouse to diameter
    mouse = diameter.getNearestPointOnEntity(mouse, true);
}

void RS_ActionDrawArc::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    bool shouldFireCoordinateEvent = true;
    switch (status) {
        case SetRadius: {
        if (rtti() == RS2::ActionDrawArc) {
                mouse = getFreeSnapAwarePoint(e, mouse);
            }
            else{
                mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse, true);
            }
            break;
        }
        case SetAngle1:{
            mouse = getSnapAngleAwarePoint(e, m_arcData->center, mouse);
            break;
        }
        case SetIncAngle:
        case SetAngle2:{
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
            if (!shouldFireCoordinateEvent){
                commandMessage(tr("Length of chord should be non-zero"));
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

void RS_ActionDrawArc::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    switch (status) {
        case SetChordLength:{
            moveRelativeZero(m_arcData->center);
            if (m_actionType == RS2::ActionDrawArc) {
                setStatus(SetAngle2);
            }
            else{
                setStatus(SetRadius);
            }
            break;
        }
        case SetIncAngle:{
            if (m_actionType == RS2::ActionDrawArc) {
                setStatus(SetAngle2);
            }
            else{
                setStatus(SetRadius);
            }
            break;
        }
        default: {
            setStatus(status - 1);
        }
    }
}

void RS_ActionDrawArc::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCenter: {
            m_arcData->center = mouse;
            moveRelativeZero(mouse);
            setStatus(SetRadius);
            break;
        }
        case SetRadius: {
            if (m_arcData->center.valid){
                m_arcData->radius = m_arcData->center.distanceTo(mouse);
            }
            switch (m_actionType){
                case RS2::ActionDrawArc:  {
                    setStatus(SetAngle1);
                    break;
                }
                case RS2::ActionDrawArcChord: {
                    m_arcData->angle1 = m_arcData->center.angleTo(mouse);
                    setStatus(SetChordLength);
                    break;
                }
                case RS2::ActionDrawArcAngleLen: {
                    m_arcData->angle1 = m_arcData->center.angleTo(mouse);
                    setStatus(SetIncAngle);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SetAngle1: {
            if (isZero){
                m_arcData->angle1 = toWorldAngleFromUCSBasisDegrees(0);
            }
            else {
                m_arcData->angle1 = m_arcData->center.angleTo(mouse);
            }
            setStatus(SetAngle2);
            break;
        }
        case SetAngle2: {
            if (isZero) {
                m_arcData->angle2 = toWorldAngleFromUCSBasisDegrees(0);
            }
            else {
                m_arcData->angle2 = m_arcData->center.angleTo(mouse);
            }
            trigger();
            break;
        }
        case SetIncAngle: {
            double wcsAngle = m_arcData->center.angleTo(mouse);
            double rotationAngle = RS_Math::correctAngle(toUCSBasisAngle(wcsAngle));
            double innerAngle = adjustRelativeAngleSignByBasis(rotationAngle);
            m_arcData->angle2 = m_arcData->angle1 + innerAngle;
            trigger();
            break;
        }
        case SetChordLength: {
            // todo - double calculation of arc start - store it for later use?
            RS_Vector startpoint= m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
            double distanceFromStartToMouse = startpoint.distanceTo(mouse);
            double diameter = 2 * m_arcData->radius;
            if (fabs(distanceFromStartToMouse / diameter) <= 1.0){
                m_arcData->angle2 = m_arcData->angle1 + asin(distanceFromStartToMouse / diameter) * 2;

                if (m_alternateArcDirection){
                    RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
                    RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);
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

bool RS_ActionDrawArc::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    if (checkCommand("reversed", c)){
        accept = true;
        setReversed(!isReversed());
        updateOptions();
    }
    else {
        switch (status) {
            case SetRadius: {
                bool ok;
                double r = RS_Math::eval(c, &ok);
                if (ok) {
                    m_arcData->radius = r;
                    setStatus(SetAngle1);
                    accept = true;
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            case SetAngle1: {
                double wcsAngle;
                bool ok = parseToWCSAngle(c, wcsAngle);
                if (ok) {
                    m_arcData->angle1 = wcsAngle;
                    accept = true;
                    setStatus(SetAngle2);
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            case SetAngle2: {
                if (checkCommand("angle", c)) {
                    setStatus(SetIncAngle);
                    accept = true;
                } else if (checkCommand("chordlen", c)) {
                    RS_Vector arcStart = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
                    moveRelativeZero(arcStart);
                    setStatus(SetChordLength);
                    accept = true;
                } else {
                    double wcsAngle;
                    bool ok = parseToWCSAngle(c, wcsAngle);
                    if (ok) {
                        m_arcData->angle2 = wcsAngle;
                        accept = true;
                        trigger();
                    } else {
                        commandMessage(tr("Not a valid expression"));
                    }
                }
                break;
            }
            case SetIncAngle: {
                double relativeAngleRad;
                bool ok = parseToRelativeAngle(c, relativeAngleRad);
                if (ok) {
                    m_arcData->angle2 = m_arcData->angle1 + relativeAngleRad;
                    accept = true;
                    trigger();
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            case SetChordLength: {
                bool ok = false;
                double l = RS_Math::eval(c, &ok);
                if (ok) {
                    accept = true;
                    if (fabs(l / (2 * m_arcData->radius)) <= 1.0) {
                        m_arcData->angle2 = m_arcData->angle1 + asin(l / (2 * m_arcData->radius)) * 2;
                        if (l < 0){ // using alternative solution (if negative value is entered)
                            RS_Vector startpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle1);
                            RS_Vector endpoint = m_arcData->center + RS_Vector::polar(m_arcData->radius, m_arcData->angle2);
                            RS_Vector alternativePoint = endpoint.mirror(m_arcData->center, startpoint);

                            m_arcData->angle2 = m_arcData->center.angleTo(alternativePoint);
                            m_arcData->reversed = !m_arcData->reversed;
                        }
                        trigger();
                    } else
                        commandMessage(tr("Not a valid chord length"));
                } else
                    commandMessage(tr("Not a valid expression"));
                break;
            }
            default:
                break;
        }
    }
    return accept;
}

QStringList RS_ActionDrawArc::getAvailableCommands() {
    if (m_actionType == RS2::ActionDrawArc) {
        return {command("angle"), command("chordlen"),command("reversed")};
    }
    else{
        return {command("reversed")};
    }
}

void RS_ActionDrawArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            if (m_actionType == RS2::ActionDrawArc) {
                updateMouseWidgetTRBack(tr("Specify radius"), MOD_SHIFT_FREE_SNAP);
            }
            else{
                updateMouseWidgetTRBack(tr("Specify start point"), MOD_SHIFT_FREE_SNAP);
            }
            break;
        case SetAngle1:
            updateMouseWidgetTRBack(tr("Specify start angle:"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle2:
            updateMouseWidgetTRBack(tr("Specify end angle or [angle/chordlen]"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternative Arc")));
            break;
        case SetIncAngle:
            updateMouseWidgetTRBack(tr("Specify included angle:"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternative Arc")));
            break;
        case SetChordLength:
            updateMouseWidgetTRBack(tr("Specify chord length (negative for alt point):"), MOD_SHIFT_LC(tr("Use alternative arc point")));
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
    return m_arcData->reversed;
}

void RS_ActionDrawArc::setReversed(bool r) const{
    m_arcData->reversed = r;
}

LC_ActionOptionsWidget* RS_ActionDrawArc::createOptionsWidget(){
    return new QG_ArcOptions();
}
