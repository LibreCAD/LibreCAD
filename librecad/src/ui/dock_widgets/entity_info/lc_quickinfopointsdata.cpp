/****************************************************************************
*
* Class that holds the list of collected coordinates

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

#include "lc_quickinfopointsdata.h"
#include "lc_graphicviewport.h"

LC_QuickInfoPointsData::LC_QuickInfoPointsData():LC_QuickInfoBaseData(){}

LC_QuickInfoPointsData::~LC_QuickInfoPointsData(){
    clear();
}

/**
 * Check whether there are some coordinates
 * @return
 */
bool LC_QuickInfoPointsData::hasData() const{
    return !m_collectedPoints.isEmpty();
}

/**
 * Removes coordinate with given index
 * @param index
 * @return
 */
bool LC_QuickInfoPointsData::removeCoordinate(int index){
    int size = m_collectedPoints.size();
    if (index >= 0 && index < size){
        m_collectedPoints.remove(index);
        doUpdatePointsAttributes();
        return true;
    }
    return false;
}

/**
 * Adds coordinate to the list, taking into consideration coordinates view mode
 * @param wcsPoint
 */
void LC_QuickInfoPointsData::processCoordinate(const RS_Vector &wcsPoint){

    int pointsCount = m_collectedPoints.size();
    PointInfo *pointInfo = nullptr;
    int index = pointsCount + 1;
    QString idxValue;
    idxValue.setNum(index);

    RS_Vector relZero = getRelativeZero();

    switch (m_coordinatesMode){
        case COORD_ABSOLUTE:
            // just use the vector itself
            pointInfo = createPointInfo(wcsPoint, wcsPoint, idxValue, false);
            break;
        case COORD_RELATIVE:{
            // check relative zero position
            if (relZero.valid){
                // calculate relative vector
                RS_Vector wcsRelative = wcsPoint - relZero;
                pointInfo = createPointInfo(wcsPoint, wcsRelative, idxValue, true);
            }
            else{
                pointInfo = createPointInfo(wcsPoint, wcsPoint, idxValue, false);
            }
            break;
        }
        case COORD_RELATIVE_TO_FIRST:{
            if (pointsCount == 0){
                // for first item, use zero at view
                RS_Vector dummyWCSZero = m_viewport->toWorld(RS_Vector(0, 0, 0));
                pointInfo = createPointInfo(wcsPoint, dummyWCSZero, idxValue, false);
            }
            else{
                // get first vector
                RS_Vector firstPoint = m_collectedPoints.at(0)->data;
                // calculate relative vector from this vector to the first vector in the list
                RS_Vector wcsRelative = wcsPoint - firstPoint;
                pointInfo = createPointInfo(wcsPoint, wcsRelative, idxValue, true);
            }
            break;
        }
        case COORD_RELATIVE_TO_PREVIOUS:{
            if (pointsCount == 0){
                RS_Vector dummyWCSZero = m_viewport->toWorld(RS_Vector(0, 0, 0));
                pointInfo = createPointInfo(wcsPoint, dummyWCSZero, idxValue, false);
            }
            else{
                // get previous vector in the list
                RS_Vector prevPoint = m_collectedPoints.at(pointsCount - 1)->data;
                // calculate relative vector
                RS_Vector wcsRelative = wcsPoint - prevPoint;
                pointInfo = createPointInfo(wcsPoint, wcsRelative, idxValue, true);
            }
            break;
        }
        default:
            break;
    }

    // check where we should add point
    if (m_collectedPointsInsertionIndex < 0){
        // just add to the end of the list
        m_collectedPoints << pointInfo;
    }
    else{
        // insert into specified index
        m_collectedPoints.insert(m_collectedPointsInsertionIndex, pointInfo);
        m_collectedPointsInsertionIndex++;
    }

    // if insertion is not at the end of the list, it's necessary to update indexes for collected points.
    // and also distance and angle if coordinates are relative to previous point.
    // so doing cleanup pass
    if (m_collectedPointsInsertionIndex >= 0){
        int size = m_collectedPoints.size();
        RS_Vector ucsViewCoordinate; // vector used for visual presentation of coordinate
        for (int i = 0; i < size; i++) {
            PointInfo *info = m_collectedPoints.at(i);
            int pointIndex = i + 1;
            QString indexValue;
            indexValue.setNum(pointIndex);
            info->label = indexValue;
            if (m_coordinatesMode == COORD_RELATIVE_TO_PREVIOUS){
                QString angleValue;
                QString lengthValue;
                if (i == 0){
                    ucsViewCoordinate = RS_Vector(0, 0, 0);
                    angleValue = "";
                    lengthValue = "";
                }
                else{
                    RS_Vector prevPoint = m_collectedPoints.at(i - 1)->data;
                    ucsViewCoordinate = wcsPoint - prevPoint;
                    ucsViewCoordinate = m_viewport->toUCSDelta(ucsViewCoordinate);
                    double ucsAngleToPrevious = ucsViewCoordinate.angle();
                    double lenToPrevious = ucsViewCoordinate.magnitude();

                    angleValue = formatUCSAngle(ucsAngleToPrevious);
                    lengthValue = formatLinear(lenToPrevious);
                }
                pointInfo->angle = angleValue;
                pointInfo->distance = lengthValue;

            }
        }
    }
}

LC_QuickInfoPointsData::PointInfo* LC_QuickInfoPointsData::createPointInfo(const RS_Vector &point, const RS_Vector &viewCoordinate, const QString &idxValue, bool relative) const {
    QString value;
    RS_Vector viewCoord;
    QString angleVal;
    if (relative){
        viewCoord = m_viewport->toUCSDelta(viewCoordinate);
        value = formatWCSDeltaVector(viewCoordinate);
        double ucsAngle = viewCoord.angle();
        angleVal = formatUCSAngle(ucsAngle);
    }
    else{
        viewCoord = viewCoordinate;
        value = formatWCSVector(viewCoordinate);
        double angle = viewCoord.angle();
        angleVal = formatWCSAngle(angle);
    }

    // hold information about vector
    auto* pointInfo = new PointInfo(point, idxValue, value);

    double len = viewCoord.magnitude();

    QString lengthVal = formatLinear(len);

    pointInfo->angle = angleVal;
    pointInfo->distance = lengthVal;
    return pointInfo;
}

/**
 * Create HTML that contains information about collected coordinates.
 * @param showDistanceAndAngle
 * @param forceUpdate
 * @return
 */
QString LC_QuickInfoPointsData::generateView(bool showDistanceAndAngle, bool forceUpdate){

    int pointsCount = m_collectedPoints.size();
    QString data = "<body>";
    if (pointsCount > 0){
        if (forceUpdate){
            doUpdatePointsAttributes();
        }
        // first, include information about point that is used as zero
        PointInfo *firstPoint = m_collectedPoints.at(0);
        RS_Vector zero = RS_Vector(0,0,0);
        switch (m_coordinatesMode) {
            case COORD_ABSOLUTE:{
                data.append(tr("Zero")).append(": <b>").append(formatUCSVector(zero)).append("</b><hr>");
                break;
            }
            case COORD_RELATIVE:{
                RS_Vector wcsRelZero = getRelativeZero();
                QString vectorStr;
                if (wcsRelZero.valid) {
                    vectorStr = formatWCSVector(wcsRelZero);
                } else {
                    vectorStr = formatUCSVector(zero);
                }
                data.append(tr("Relative Zero")).append(": <b>").append(vectorStr).append("</b><hr>");
                break;
            }
            case COORD_RELATIVE_TO_FIRST:
            case COORD_RELATIVE_TO_PREVIOUS: {
                QString label = tr("First point").append(": ");
                QString value = formatWCSVector(firstPoint->data);
                createLink(data, "zero", 0, tr("Set Relative Zero"), label);
                data.append(" <b>");
                createLink(data, "coord", 0, tr("To Cmd"), value);
                data.append(" </b>");
                data.append("<hr>");
                break;
            }
            default:
                break;
        }
        // total distance of points path
        double totalDistance = 0.0;

        // iterate over all collected coordinates
        RS_Vector prevPoint = firstPoint->data;
        for (int i = 0; i < pointsCount; i++){
            PointInfo* pointInfo = m_collectedPoints.at(i);
            createLink(data, "zero", i, tr("Set Relative Zero"), pointInfo->label);
            data.append(" | &nbsp;");
            createLink(data, "coord", i, tr("To Cmd"), pointInfo->value);
            if (showDistanceAndAngle){ // show them based on options
                data.append(" | &nbsp;");
                data.append(pointInfo->distance).append(", &lt; ").append(pointInfo->angle);
            }

            if (i < (pointsCount-1)){
                data.append("<br>");
            }
            RS_Vector coord = pointInfo->data;
            // update distance
            double distance = coord.distanceTo(prevPoint);
            totalDistance += distance;

            prevPoint = coord;

        }
        // Output total length for points path
        data.append(" <hr>").append(tr("Points Path Distance:")).append(" <b>").append(formatLinear(totalDistance)).append("</b>");

    }
    else{
        data = tr("No data - select coordinates first...");
    }
    data.append("</body>");
    return data;
}

/**
 * Update points for provided coordinates mode
 * @param mode mode
 * @return true if view should be updated
 */
bool LC_QuickInfoPointsData::updateForCoordinateViewMode(int mode){
    if (m_coordinatesMode != mode){
        m_coordinatesMode = mode;
        doUpdatePointsAttributes();
        return true;
    }
    return false;
}

/**
 * Recalculates points view labels
 */
void LC_QuickInfoPointsData::doUpdatePointsAttributes() const {
    int pointsCount = m_collectedPoints.size();

    RS_Vector relZero = getRelativeZero();

    QString value;
    QString angleVal;
    QString lengthVal;

    for (int i = 0; i < pointsCount; i++) {
        PointInfo *pointInfo = m_collectedPoints.at(i);
        RS_Vector &wcsPoint = pointInfo->data;
        switch (m_coordinatesMode) {
            case COORD_ABSOLUTE:{
                value = formatWCSVector(wcsPoint);
                angleVal = formatWCSAngle(wcsPoint.angle());
                lengthVal = formatLinear(wcsPoint.magnitude());
                break;
            }
            case COORD_RELATIVE: {
                if (relZero.valid){
                    RS_Vector wcsRelative = wcsPoint - relZero;
                    RS_Vector viewCoord = m_viewport->toUCSDelta(wcsRelative);
                    value = formatWCSDeltaVector(viewCoord);
                    double ucsAngle = viewCoord.angle();
                    angleVal = formatUCSAngle(ucsAngle);
                    lengthVal = formatLinear(viewCoord.magnitude());
                } else {
                    value = formatWCSVector(wcsPoint);
                    angleVal = formatWCSAngle(wcsPoint.angle());
                    lengthVal = formatLinear(wcsPoint.magnitude());
                }
                break;
            }
            case COORD_RELATIVE_TO_FIRST: {
                if (pointsCount == 0){
                    value = formatUCSVector(RS_Vector(0, 0, 0));
                    angleVal = "";
                    lengthVal = "";
                } else {
                    RS_Vector firstPoint = m_collectedPoints.at(0)->data;
                    RS_Vector wcsRelative = wcsPoint - firstPoint;
                    RS_Vector viewCoord = m_viewport->toUCSDelta(wcsRelative);
                    value = formatWCSDeltaVector(viewCoord);
                    double ucsAngle = viewCoord.angle();
                    angleVal = formatUCSAngle(ucsAngle);
                    lengthVal = formatLinear(viewCoord.magnitude());
                }
                break;
            }
            case COORD_RELATIVE_TO_PREVIOUS: {
                if (i == 0){
                    value = formatUCSVector(RS_Vector(0, 0, 0));
                    angleVal = "";
                    lengthVal = "";
                } else {
                    RS_Vector prevPoint = m_collectedPoints.at(i - 1)->data;
                    RS_Vector wcsRelative = wcsPoint - prevPoint;
                    RS_Vector viewCoord = m_viewport->toUCSDelta(wcsRelative);
                    value = formatWCSDeltaVector(viewCoord);
                    double ucsAngle = viewCoord.angle();
                    angleVal = formatUCSAngle(ucsAngle);
                    lengthVal = formatLinear(viewCoord.magnitude());
                }
                break;
            }
            default:
                break;
        }

        pointInfo->value = value;
        pointInfo->distance = lengthVal;
        pointInfo->angle = angleVal;

        QString index;
        index.setNum(i+1);
        pointInfo->label = index;
    }
}

/**
 * Return vector for given index
 * @param index
 * @return
 */
RS_Vector LC_QuickInfoPointsData::getVectorForIndex(int index) const{
    auto result = RS_Vector(false);
    if (index < m_collectedPoints.size()){
        result = m_collectedPoints.at(index)->data;
    }
    return result;
}

/**
 * Cleanup
 */
void LC_QuickInfoPointsData::clear(){
    qDeleteAll(m_collectedPoints.begin(), m_collectedPoints.end());
    m_collectedPoints.clear();
}
