/****************************************************************************
**
* Angle between 3 points action

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
#include "rs_math.h"
#include "rs_graphic.h"
#include "rs_units.h"
#include "lc_actioninfo3pointsangle.h"

LC_ActionInfo3PointsAngle::LC_ActionInfo3PointsAngle(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("InfoAngle3Points", container, graphicView) {
    actionType = RS2::ActionInfoDistEntity2Point;
}

LC_ActionInfo3PointsAngle::~LC_ActionInfo3PointsAngle() = default;

void LC_ActionInfo3PointsAngle::trigger() {

    double angle1 = point2.angleTo(point1);
    double angle2 = point2.angleTo(point3);

    double angle = RS_Math::correctAngle(angle1-angle2);

    RS2::AngleFormat angleFormat = graphic->getAngleFormat();
    int anglePrec = graphic->getAnglePrecision();
    QString angleStr = RS_Units::formatAngle(angle, angleFormat, anglePrec);

    RS2::LinearFormat linearFormat = graphic->getLinearFormat();
    RS2::Unit linearUnit = graphic->getUnit();
    int linearPrecision = graphic->getLinearPrecision();

    QString p1X = RS_Units::formatLinear(point1.x, linearUnit,linearFormat, linearPrecision);
    QString p1Y = RS_Units::formatLinear(point1.y, linearUnit,linearFormat, linearPrecision);
    QString p2X = RS_Units::formatLinear(point2.x, linearUnit,linearFormat, linearPrecision);
    QString p2Y = RS_Units::formatLinear(point2.y, linearUnit,linearFormat, linearPrecision);
    QString p3X = RS_Units::formatLinear(point3.x, linearUnit,linearFormat, linearPrecision);
    QString p3Y = RS_Units::formatLinear(point3.y, linearUnit,linearFormat, linearPrecision);


    QString altAngleStr = RS_Units::formatAngle(RS_Math::correctAngle(2 * M_PI - angle), angleFormat, anglePrec);


    const QString &msgTemplate = tr("Angle: %1 (%2)\n Start Edge Point: (%3 , %4)\n Intersection Point :(%5, %6)\n End Edge Point: (%7 , %8)");
    const QString &msg = msgTemplate.arg(angleStr, altAngleStr, p1X, p1Y, p2X, p2Y, p3X, p3Y);
    commandMessage("---");
    commandMessage(msg);

    setStatus(SetPoint3);
}

void LC_ActionInfo3PointsAngle::mouseMoveEvent(QMouseEvent *e) {
    int status = getStatus();
    deletePreview();
    RS_Vector mouse = snapPoint(e);
    switch (status){
        case SetPoint1:{
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2:{
            if (!trySnapToRelZeroCoordinateEvent(e)) {
                mouse = getSnapAngleAwarePoint(e, point1, mouse, true, isControl(e));
                previewRefPoint(point1);
                if (showRefEntitiesOnPreview) {
                    previewRefSelectablePoint(mouse);
                    previewRefLine(point1, mouse);
                }
            }
            break;
        }
        case SetPoint3:{
            if (!trySnapToRelZeroCoordinateEvent(e)) {
                mouse = getSnapAngleAwarePoint(e, point2, mouse, true, isControl(e));

                previewRefPoint(point1);
                previewRefPoint(point2);
                previewRefSelectablePoint(mouse);

                if (showRefEntitiesOnPreview) {
                    previewRefLine(point1, point2);
                    previewRefLine(point2, mouse);

                    double distance1 = point2.distanceTo(point1);
                    double distance2 = point2.distanceTo(mouse);
                    if (distance2 < distance1) {
                        previewRefArc(point2, mouse, point1, true);
                    } else {
                        previewRefArc(point2, point1, mouse, true);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    drawPreview();
}

void LC_ActionInfo3PointsAngle::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            point1 = pos;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            point2 = pos;
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3:{
            point3 = pos;
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfo3PointsAngle::updateMouseButtonHints() {
    int status = getStatus();
    switch (status){
        case SetPoint1:{
            updateMouseWidgetTRCancel(tr("Select first edge point of angle"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetPoint2:{
            updateMouseWidgetTRCancel(tr("Select second (intersection) point of angle"), MOD_SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_REL_ZERO, LC_ModifiersInfo::MSG_ANGLE_SNAP));
            break;
        }
        case SetPoint3:{
            updateMouseWidgetTRCancel(tr("Select second edge point of angle"), MOD_SHIFT_AND_CTRL(LC_ModifiersInfo::MSG_REL_ZERO, LC_ModifiersInfo::MSG_ANGLE_SNAP));
            break;
        }
        default:
            updateMouseWidget();
    }
}

void LC_ActionInfo3PointsAngle::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    switch (status){
        case SetPoint1:{
            break;
        }
        case SetPoint2: {
            snapped = getSnapAngleAwarePoint(e, point1, snapped, false,isControl(e));
            break;
        }
        case SetPoint3:{
            snapped = getSnapAngleAwarePoint(e, point2, snapped, false, isControl(e));
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionInfo3PointsAngle::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    setStatus(getStatus() - 1);
}

RS2::CursorType LC_ActionInfo3PointsAngle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}
