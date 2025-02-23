/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_rectregion.h"

LC_RectRegion::LC_RectRegion() {}

void LC_RectRegion::setCorners(RS_Vector lBottom, RS_Vector lTop, RS_Vector rTop, RS_Vector rBottom){
    leftBottom = lBottom;
    leftTop = lTop;
    rightBottom = rBottom;
    rightTop = rTop;
    updateOther();
}

RS_Vector LC_RectRegion::getRotatedPoint(RS_Vector vect, double angle) {
    RS_Vector result = vect;
    result.rotate(leftBottom, angle);
    return result;
}

LC_RectRegion* LC_RectRegion::getRotated(double angle) {
    LC_RectRegion* result = new LC_RectRegion();
    RS_Vector ltop = leftTop;
    ltop.rotate(leftBottom, angle);
    RS_Vector rtop = rightTop;
    rtop.rotate(leftBottom, angle);
    RS_Vector rbottom = rightBottom;
    rbottom.rotate(leftBottom, angle);

    result->setCorners(leftBottom, ltop, rtop, rbottom);
    return result;
}

void LC_RectRegion::updateOther(){
    midLeft  = (leftBottom + leftTop) * 0.5;
    midRight = (rightBottom + rightTop) * 0.5;
    midBottom = (rightBottom + leftBottom) * 0.5;
    midTop = (rightTop + leftTop) * 0.5;
    center = (rightBottom + leftTop) * 0.5;

    allPoints[0] = leftBottom;
    allPoints[1] = midLeft;
    allPoints[2] = leftTop;
    allPoints[3] = midTop;
    allPoints[4] = rightTop;
    allPoints[5] = midRight;
    allPoints[6] = rightBottom;
    allPoints[7] = midBottom;

    corners[0] = leftBottom;
    corners[1] = leftTop;
    corners[2] = rightTop;
    corners[3] = rightBottom;
}

void LC_RectRegion::rotate(double angle){
    RS_Vector center = leftBottom;
    RS_Vector angleVector{angle};
    leftTop.rotate(center, angleVector);
    rightBottom.rotate(center, angleVector);
    rightTop.rotate(center, angleVector);
    updateOther();
}

void LC_RectRegion::move(RS_Vector offset){
    leftBottom.move(offset);
    leftTop.move(offset);
    rightBottom.move(offset);
    rightTop.move(offset);
    updateOther();
}

void LC_RectRegion::scale(RS_Vector factor){
    RS_Vector center = leftBottom;
    leftTop.scale(center, factor);
    rightBottom.scale(center, factor);
    rightTop.scale(center, factor);
    updateOther();
}

LC_TransformData LC_RectRegion::determineTransformData([[maybe_unused]]RS_Vector ref, [[maybe_unused]] RS_Vector newRef) {
    return LC_TransformData();
}
