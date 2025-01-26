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
#include <QMouseEvent>

#include "lc_actioninfo3pointsangle.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_units.h"

LC_ActionInfo3PointsAngle::LC_ActionInfo3PointsAngle(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("InfoAngle3Points", container, graphicView) {
    actionType = RS2::ActionInfoDistEntity2Point;
}

LC_ActionInfo3PointsAngle::~LC_ActionInfo3PointsAngle() = default;

void LC_ActionInfo3PointsAngle::doTrigger() {
    double angle1 = point2.angleTo(point1);
    double angle2 = point2.angleTo(point3);

    double angle = RS_Math::correctAngle(angle1-angle2);
    QString angleStr = formatAngle(angle);

    QString p1X = formatLinear(point1.x);
    QString p1Y = formatLinear(point1.y);
    QString p2X = formatLinear(point2.x);
    QString p2Y = formatLinear(point2.y);
    QString p3X = formatLinear(point3.x);
    QString p3Y = formatLinear(point3.y);

    QString altAngleStr = formatAngle(RS_Math::correctAngle(2 * M_PI - angle));

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
                updateInfoCursor(mouse, point1);
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
                updateInfoCursor(mouse, point2, point1);
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
            updateMouseWidgetTRCancel(tr("Select second (intersection) point of angle"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, MSG_ANGLE_SNAP));
            break;
        }
        case SetPoint3:{
            updateMouseWidgetTRCancel(tr("Select second edge point of angle"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO, MSG_ANGLE_SNAP));
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

void LC_ActionInfo3PointsAngle::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (infoCursorOverlayPrefs->enabled){
        double distance = startPoint.distanceTo(mouse);
        LC_InfoMessageBuilder msg(tr("Info"));
        msg.add(tr("Distance:"),formatLinear(distance));
        msg.add(tr("Angle:"),formatAngle(startPoint.angleTo(mouse)));
        msg.add(tr("From:"),formatVector(startPoint));
        msg.add(tr("To:"),formatVector(mouse));
        appendInfoCursorZoneMessage(msg.toString(), 2, false);
    }
}

void LC_ActionInfo3PointsAngle::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &point2, const RS_Vector &startPoint) {
    if (infoCursorOverlayPrefs->enabled) {

        double angle1 = point2.angleTo(point1);
        double angle2 = point2.angleTo(mouse);

        double angle = RS_Math::correctAngle(angle1 - angle2);

        double distance = point2.distanceTo(startPoint);
        double distance2 = point2.distanceTo(mouse);
        LC_InfoMessageBuilder msg(tr("Info"));
        msg.add(tr("Angle:"), formatAngle(angle));
        msg.add(tr("Angle (Alt):"), formatAngle(RS_Math::correctAngle(2 * M_PI - angle)));
        msg.add(tr("From:"), formatVector(startPoint));
        msg.add(tr("Intersection:"), formatVector(point2));
        msg.add(tr("To:"), formatVector(mouse));
        msg.add(tr("Distance1:"), formatLinear(distance));
        msg.add(tr("Distance2:"), formatLinear(distance2));
        msg.add(tr("Angle 1:"), formatAngle(point2.angleTo(startPoint)));
        msg.add(tr("Angle 2:"), formatAngle(point2.angleTo(mouse)));
        appendInfoCursorZoneMessage(msg.toString(), 2, false);
    }
}
