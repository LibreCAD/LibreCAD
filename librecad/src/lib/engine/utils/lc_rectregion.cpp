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

LC_RectRegion::LC_RectRegion() = default;

void LC_RectRegion::setCorners(const RS_Vector& lBottom, const RS_Vector& lTop, const RS_Vector& rTop, const RS_Vector& rBottom){
    m_leftBottom = lBottom;
    m_leftTop = lTop;
    m_rightBottom = rBottom;
    m_rightTop = rTop;
    updateOther();
}

RS_Vector LC_RectRegion::getRotatedPoint(const RS_Vector& vect, const double angle) const {
    RS_Vector result = vect;
    result.rotate(m_leftBottom, angle);
    return result;
}

LC_RectRegion* LC_RectRegion::getRotated(const double angle) const {
    const auto result = new LC_RectRegion();
    RS_Vector ltop = m_leftTop;
    ltop.rotate(m_leftBottom, angle);
    RS_Vector rtop = m_rightTop;
    rtop.rotate(m_leftBottom, angle);
    RS_Vector rbottom = m_rightBottom;
    rbottom.rotate(m_leftBottom, angle);

    result->setCorners(m_leftBottom, ltop, rtop, rbottom);
    return result;
}

void LC_RectRegion::updateOther(){
    m_midLeft  = (m_leftBottom + m_leftTop) * 0.5;
    m_midRight = (m_rightBottom + m_rightTop) * 0.5;
    m_midBottom = (m_rightBottom + m_leftBottom) * 0.5;
    m_midTop = (m_rightTop + m_leftTop) * 0.5;
    m_center = (m_rightBottom + m_leftTop) * 0.5;

    m_allPoints[0] = m_leftBottom;
    m_allPoints[1] = m_midLeft;
    m_allPoints[2] = m_leftTop;
    m_allPoints[3] = m_midTop;
    m_allPoints[4] = m_rightTop;
    m_allPoints[5] = m_midRight;
    m_allPoints[6] = m_rightBottom;
    m_allPoints[7] = m_midBottom;

    m_corners[0] = m_leftBottom;
    m_corners[1] = m_leftTop;
    m_corners[2] = m_rightTop;
    m_corners[3] = m_rightBottom;
}

void LC_RectRegion::rotate(const double angle){
    const RS_Vector center = m_leftBottom;
    const RS_Vector angleVector{angle};
    m_leftTop.rotate(center, angleVector);
    m_rightBottom.rotate(center, angleVector);
    m_rightTop.rotate(center, angleVector);
    updateOther();
}

void LC_RectRegion::move(const RS_Vector& offset){
    m_leftBottom.move(offset);
    m_leftTop.move(offset);
    m_rightBottom.move(offset);
    m_rightTop.move(offset);
    updateOther();
}

void LC_RectRegion::scale(const RS_Vector& factor){
    const RS_Vector center = m_leftBottom;
    m_leftTop.scale(center, factor);
    m_rightBottom.scale(center, factor);
    m_rightTop.scale(center, factor);
    updateOther();
}

LC_TransformData LC_RectRegion::determineTransformData([[maybe_unused]]RS_Vector ref, [[maybe_unused]] RS_Vector newRef) {
    return LC_TransformData();
}
