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
#include "lc_actioninfo3pointsangle.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_linemath.h"

LC_ActionInfo3PointsAngle::LC_ActionInfo3PointsAngle(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("InfoAngle3Points", actionContext, RS2::ActionInfoAngle3Points) {
}

LC_ActionInfo3PointsAngle::~LC_ActionInfo3PointsAngle() = default;

void LC_ActionInfo3PointsAngle::doTrigger() {
    double angle = LC_LineMath::angleFor3Points(m_point1, m_point2, m_point3);
    QString angleStr = formatAngleRaw(angle);

    RS_Vector ucsPoint1 = toUCS(m_point1);
    RS_Vector ucsPoint2 = toUCS(m_point2);
    RS_Vector ucsPoint3 = toUCS(m_point3);

    QString p1X = formatLinear(ucsPoint1.x);
    QString p1Y = formatLinear(ucsPoint1.y);
    QString p2X = formatLinear(ucsPoint2.x);
    QString p2Y = formatLinear(ucsPoint2.y);
    QString p3X = formatLinear(ucsPoint3.x);
    QString p3Y = formatLinear(ucsPoint3.y);

    double angleComplementary, angleSupplementary, alt;
    RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary,  alt);

    QString strAngle = formatAngleRaw(angle);
    QString complimenatryStr = formatAngleRaw(angleComplementary);
    QString supplimentaryStr = formatAngleRaw(angleSupplementary);
    QString altStr = formatAngleRaw(alt);

    const QString &msgTemplate = tr("Angle: %1\nComplementary: %2\nSupplementary: %3\nAlternative: %4\nStart Edge Point: (%5 , %6)\nIntersection Point :(%7, %8)\nEnd Edge Point: (%9 , %10)");
    const QString &msg = msgTemplate.arg(strAngle,complimenatryStr, supplimentaryStr, altStr, p1X, p1Y, p2X, p2Y, p3X, p3Y);
    commandMessage("---");
    commandMessage(msg);

    if (m_restartFromNewPoint) {
        m_restartFromNewPoint = false;
        setStatus(SetPoint1);
    }
    else {
        setStatus(SetPoint3);
    }
}

void LC_ActionInfo3PointsAngle::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetPoint1:{
            previewRefSelectablePoint(mouse);
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, m_point1, mouse, true, e->isShift);
            previewRefPoint(m_point1);
            updateInfoCursor(mouse, m_point1);
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
                previewRefLine(m_point1, mouse);
            }
            break;
        }
        case SetPoint3:{
            mouse = getSnapAngleAwarePoint(e, m_point2, mouse, true, e->isShift);
            previewRefPoint(m_point1);
            previewRefPoint(m_point2);
            previewRefSelectablePoint(mouse);
            updateInfoCursor(mouse, m_point2, m_point1);
            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_point1, m_point2);
                previewRefLine(m_point2, mouse);

                double distance1 = m_point2.distanceTo(m_point1);
                double distance2 = m_point2.distanceTo(mouse);
                if (distance2 < distance1) {
                    previewRefArc(m_point2, mouse, m_point1, true);
                }
                else {
                    previewRefArc(m_point2, m_point1, mouse, true);
                }
                // previewRefArc(false, m_point2, m_point1, mouse);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInfo3PointsAngle::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            m_point1 = pos;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            m_point2 = pos;
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3:{
            m_point3 = pos;
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
            updateMouseWidgetTRCancel(tr("Select first edge point of angle"));
            break;
        }
        case SetPoint2:{
            updateMouseWidgetTRCancel(tr("Select second (intersection) point of angle"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetPoint3:{
            updateMouseWidgetTRCancel(tr("Select second edge point of angle"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Restart with first edge point selection")));
            break;
        }
        default:
            updateMouseWidget();
    }
}

void LC_ActionInfo3PointsAngle::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snapped = e->snapPoint;
    switch (status){
        case SetPoint1:{
            break;
        }
        case SetPoint2: {
            snapped = getSnapAngleAwarePoint(e, m_point1, snapped, false,e->isShift);
            break;
        }
        case SetPoint3:{
            snapped = getSnapAngleAwarePoint(e, m_point2, snapped, false, e->isShift);
            m_restartFromNewPoint = e->isControl;
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void LC_ActionInfo3PointsAngle::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    setStatus(getStatus() - 1);
}

RS2::CursorType LC_ActionInfo3PointsAngle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionInfo3PointsAngle::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (m_infoCursorOverlayPrefs->enabled){
        msg(tr("Angle Info"))
            .linear(tr("Distance:"), startPoint.distanceTo(mouse))
            .wcsAngle(tr("Angle:"), startPoint.angleTo(mouse))
            .vector(tr("From:"), startPoint)
            .vector(tr("To:"), mouse)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInfo3PointsAngle::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &point2, const RS_Vector &startPoint) {
    if (m_infoCursorOverlayPrefs->enabled) {
        double angle = LC_LineMath::angleFor3Points(m_point1, point2, mouse);

        double angleComplementary, angleSupplementary, angleAlt;
        RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary, angleAlt);

        msg(tr("Angle Info"))
            .rawAngle(tr("Angle:"), angle)
            .rawAngle(tr("Complementary:"), angleComplementary)
            .rawAngle(tr("Supplementary:"), angleSupplementary)
            .rawAngle(tr("Alternative: "), angleAlt)
            .vector(tr("From:"), startPoint)
            .vector(tr("Intersection:"), point2)
            .vector(tr("To:"), mouse)
            .linear(tr("Distance1:"), point2.distanceTo(startPoint))
            .linear(tr("Distance2:"), point2.distanceTo(mouse))
            .wcsAngle(tr("Angle 1:"), point2.angleTo(startPoint))
            .wcsAngle(tr("Angle 2:"), point2.angleTo(mouse))
            .toInfoCursorZone2(false);
    }
}
