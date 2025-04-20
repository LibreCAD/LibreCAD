/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/
#include "lc_actiondrawpointslattice.h"

#include "lc_linemath.h"
#include "lc_pointslatticeoptions.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_pen.h"
#include "rs_point.h"

class RS_Layer;

LC_ActionDrawPointsLattice::LC_ActionDrawPointsLattice(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Points Lattice", actionContext,RS2::ActionDrawPointsLattice) {
}

void LC_ActionDrawPointsLattice::doTrigger() {
    QVector<RS_Vector> pointsToCreate;
    createPointsLattice(m_point4, pointsToCreate);

    qsizetype pointsCount = pointsToCreate.size();
    if (pointsCount > 0) {
        RS_Layer *layerToSet  = m_graphicView->getGraphic()->getActiveLayer();
        RS_Pen penToUse = m_graphicView->getGraphic()->getActivePen();
        undoCycleStart();
        for (unsigned i = 0; i < pointsCount; i++) {
            auto *point = new RS_Point(m_container, pointsToCreate.at(i));
            point->setLayer(layerToSet);
            point->setPen(penToUse);
            point->setParent(m_container);
            m_container->addEntity(point);
            undoableAdd(point);
        }
        undoCycleEnd();
    }

    setStatus(SetPoint1);
}

void LC_ActionDrawPointsLattice::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    QVector<RS_Vector> pointsToCreate;
    switch (status){
        case SetPoint1:{
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2:{
            RS_Vector pos = getSnapAngleAwarePoint(e, m_point1, mouse, true);
            createPointsLine(m_point1, pos, m_pointsAmountByX, pointsToCreate);
            if (m_showRefEntitiesOnPreview){
                previewRefPoint(m_point1);
                previewRefSelectablePoint(pos);
            }
            break;
        }
        case SetPoint3:{
            RS_Vector pos = getSnapAngleAwarePoint(e, m_point2, mouse, true);
            createPointsLine(m_point1, m_point2, m_pointsAmountByX, pointsToCreate);
            createPointsLine(m_point2, pos, m_pointsAmountByY, pointsToCreate);
            if (m_showRefEntitiesOnPreview){
                previewRefPoint(m_point1);
                previewRefPoint(m_point2);
                previewRefSelectablePoint(pos);
            }
            break;
        }
        case SetPoint4:{
            RS_Vector pos = getSnapAngleAwarePoint(e, m_point3, mouse , true);
            bool alternateLastPointAdjustment = e->isControl;
            pos = getLastPointPosition(pos, alternateLastPointAdjustment);
            createPointsLattice(pos, pointsToCreate);
            if (m_showRefEntitiesOnPreview){
                previewRefPoint(m_point1);
                previewRefPoint(m_point2);
                previewRefPoint(m_point3);
                previewRefSelectablePoint(pos);
            }
            break;
        }
    }
    qsizetype pointsCount = pointsToCreate.size();
    for (unsigned i = 0; i < pointsCount; i++){
        previewPoint(pointsToCreate.at(i));
    }
}

void LC_ActionDrawPointsLattice::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector pos = e->snapPoint;
    switch (status){
        case SetPoint1:{
            pos = getRelZeroAwarePoint(e, pos);
            break;
        }
        case SetPoint2:{
            pos = getSnapAngleAwarePoint(e, m_point1, pos, false);
            break;
        }
        case SetPoint3:{
            pos = getSnapAngleAwarePoint(e, m_point2, pos, false);
            break;
        }
        case SetPoint4:{
            pos = getSnapAngleAwarePoint(e, m_point3, pos, false);
            bool alternateLastPointAdjustment = e->isControl;
            pos = getLastPointPosition(pos, alternateLastPointAdjustment);
            break;
        }
    }
    fireCoordinateEvent(pos);
}

RS_Vector LC_ActionDrawPointsLattice::getLastPointPosition(RS_Vector &pos, bool alternateLastPointAdjustment) const {
    bool doAdjustPoint4 = m_adjustLastPointToFirst;
    if (alternateLastPointAdjustment){
        doAdjustPoint4 = !doAdjustPoint4;
    }
    if (doAdjustPoint4) {
         RS_Vector firstSideDelta = m_point2 - m_point1;
         pos = m_point3 - firstSideDelta;
    }
    return pos;
}

void LC_ActionDrawPointsLattice::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    switch (status){
        case SetPoint1:
        case SetPoint2:
        case SetPoint3:
        case SetPoint4:{
            setStatus(status - 1);
            break;
        }
        case SetNumXPoints:
        case SetNumYPoints:{
            setStatus(m_majorStatus);
            break;
        }
    }
}

void LC_ActionDrawPointsLattice::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint1:{
            m_point1 = pos;
            moveRelativeZero(pos);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            if (LC_LineMath::isNotMeaningfulDistance(pos, m_point1)){
                commandMessage(tr("Second point is too close to the first one"));
            }
            else{
                m_point2 = pos;
                moveRelativeZero(pos);
                setStatus(SetPoint3);
            }
            break;
        }
        case SetPoint3:{
            if (LC_LineMath::isNotMeaningfulDistance(pos, m_point2)){
                commandMessage(tr("Third point is too close to the second one"));
            }
            else{
                m_point3 = pos;
                moveRelativeZero(pos);
                setStatus(SetPoint4);
            }
            break;
        }
        case SetPoint4:{
            if (LC_LineMath::isNotMeaningfulDistance(pos, m_point3)){
                commandMessage(tr("Third point is too close to the second one"));
            }
            else{
                m_point4 = pos;
                moveRelativeZero(pos);
                trigger();
            }
            break;
        }
    }
}

bool LC_ActionDrawPointsLattice::doProcessCommand(int status, const QString &command) {
    bool accept;
    if (checkCommand("cols", command)){
        m_majorStatus = status;
        setStatus(SetNumXPoints);
        accept = true;
    }
    else if (checkCommand("rows", command)){
        m_majorStatus = status;
        setStatus(SetNumYPoints);
        accept = true;
    }
    else if (checkCommand("p4off", command)){
        m_adjustLastPointToFirst = true;
        accept = true;
    }
    else if (checkCommand("p4on", command)){
        m_adjustLastPointToFirst = false;
        accept = true;
    }
    else if (status == SetNumYPoints || status == SetNumXPoints){
        bool ok = false;
        int count = RS_Math::eval(command, &ok);
        if (ok && count > 0){ // at least 1 point should be present
            if (status == SetNumXPoints){
                m_pointsAmountByX = count;
            }
            else{
                m_pointsAmountByY = count;
            }
            updateOptions();
            setStatus(m_majorStatus);
            accept = true;
        } else {
            commandMessage(tr("Invalid value provided"));
            accept = false;
        }
    }
    else{
        accept = false;
    }
    return accept;
}

QStringList LC_ActionDrawPointsLattice::getAvailableCommands() {
    return {command("cols"), command("rows"),command("p4off"), command("p4on")};
}

void LC_ActionDrawPointsLattice::updateMouseButtonHints() {
    int state = getStatus();
    switch (state){
        case SetPoint1: {
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        }
        case SetPoint2: {
            updateMouseWidgetTRCancel(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetPoint3: {
            updateMouseWidgetTRCancel(tr("Specify third point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case SetPoint4: {
            updateMouseWidgetTRCancel(tr("Specify fourth point"), m_adjustLastPointToFirst ? MOD_SHIFT_AND_CTRL_ANGLE(tr("Last point position is un-adjusted")):
                                                                     MOD_SHIFT_AND_CTRL_ANGLE(tr("Last point position is adjusted to first")));
            break;
        }
        case SetNumXPoints: {
            updateMouseWidgetTRCancel(tr("Enter number of points by X"));
            break;
        }
        case SetNumYPoints: {
            updateMouseWidgetTRCancel(tr("Enter number of points by Y"));
            break;
        }
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}

void LC_ActionDrawPointsLattice::createPointsLine(RS_Vector start, RS_Vector end, int count, QVector<RS_Vector> &points) {
    double segmentAngle = start.angleTo(end);
    double distance = start.distanceTo(end);
    double singleSegmentLen = distance/(count-1);
    RS_Vector pos = start;
    for (int i = 0; i < count; i++){
        points << pos;
        pos = pos.relative(singleSegmentLen, segmentAngle);
    }
}

void LC_ActionDrawPointsLattice::createPointsLattice(RS_Vector lastPoint, QVector<RS_Vector> &pointsToCreate) {

    createPointsLine(m_point1, m_point2, m_pointsAmountByX, pointsToCreate);
    int numByY = m_pointsAmountByY - 1;
    RS_Vector dv1 = (m_point1 - lastPoint) / numByY;
    RS_Vector dv2 = (m_point2 - m_point3) / numByY;
    RS_Vector v1 = m_point1;
    RS_Vector v2 = m_point2;
    for (int i = 1; i < numByY; ++i) {
        createPointsLine(v1 - dv1*i,
                         v2 - dv2*i,m_pointsAmountByX,pointsToCreate);
    }
    createPointsLine(m_point3, lastPoint,  m_pointsAmountByX, pointsToCreate);
}

int LC_ActionDrawPointsLattice::getColumnPointsCount() const {
    return m_pointsAmountByX;
}

void LC_ActionDrawPointsLattice::setColumnPointsCount(int count) {
    m_pointsAmountByX = count;
}

int LC_ActionDrawPointsLattice::getRowPointsCount() const {
    return m_pointsAmountByY;
}

void LC_ActionDrawPointsLattice::setRowPointsCount(int count) {
    m_pointsAmountByY = count;
}

bool LC_ActionDrawPointsLattice::isAdjustLastPointToFirst() const {
    return m_adjustLastPointToFirst;
}

void LC_ActionDrawPointsLattice::setAdjustLastPointToFirst(bool val) {
    m_adjustLastPointToFirst = val;
}

RS2::CursorType LC_ActionDrawPointsLattice::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ActionOptionsWidget *LC_ActionDrawPointsLattice::createOptionsWidget() {
    return new LC_PointsLatticeOptions();
}
