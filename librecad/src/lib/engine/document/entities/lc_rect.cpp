/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2015 librecad.org (www.librecad.org)

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

#include "lc_rect.h"

#include <QDebug>
#include<iostream>
#include <qlogging.h>

#define INTERT_TEST(s) qDebug()<<"\ntesting " #s; \
 assert(s); \
 qDebug()<<"Passed";

using namespace lc::geo;

Coordinate LC_Rect::Vector(const Coordinate& p, const Coordinate& q) {
    return {q.x - p.x, q.y - p.y};
}

/**
  * Create a new Area. The coordinates coordA and coordB will be ordered so that minP will always be < maxP
  * The coordinates are not allowed to describe a volume
  *
  * @param coordA First coordinate of an area
  * @param coordB Second coordinate of an area
  */
LC_Rect::Area(const Coordinate& coordA, const Coordinate& coordB) : m_minP{std::min(coordA.x, coordB.x), std::min(coordA.y, coordB.y)},
                                                                    m_maxP{std::max(coordA.x, coordB.x), std::max(coordA.y, coordB.y)} {
}

LC_Rect::Area() : m_minP{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
                  m_maxP{std::numeric_limits<double>::min(), std::numeric_limits<double>::min()} {
}

/**
 * @brief Area
 * given at a coordinate with a given width and height
 * @param coord
 * @param width
 * @param height
 */
LC_Rect::Area(const Coordinate& coord, const double width, const double height) : Area(coord, {coord.x + width, coord.y + height}) {
}

/**
  * Return the smallest corner (smallest xy-coordinates)
  */
const Coordinate& LC_Rect::minP() const {
    return m_minP;
}

/**
  * Return the highest corner
  */
const Coordinate& LC_Rect::maxP() const {
    return m_maxP;
}

/**
 * @brief topLeftCorner return the upperLeftCorner coordinates
 * _minP is considered lowerLeft, _maxP is the upperRight
 * @return {_minP.x, _maxP.y}
 */
Coordinate LC_Rect::upperLeftCorner() const {
    return {m_minP.x, m_maxP.y};
}

Coordinate LC_Rect::upperRightCorner() const {
    return m_maxP;
}

/**
 * @brief lowerRightCorner return the lowerRight coordinates
 * _minP is considered lowerLeft, _maxP is the upperRight
 * @return {_maxP.x, _minP.y}
 */
Coordinate LC_Rect::lowerRightCorner() const {
    return {m_maxP.x, m_minP.y};
}

Coordinate LC_Rect::lowerLeftCorner() const {
    return m_minP;
}

/**
 * @brief width
 * Returns the wid th of this area
 * @return
 */
double LC_Rect::width() const {
    return m_maxP.x - m_minP.x;
}

/**
 * @brief height
 * Returns the height f this area
 * @return
 */
double LC_Rect::height() const {
    return m_maxP.y - m_minP.y;
}

bool LC_Rect::isEmpty(double tolerance) const {
          return std::min(width(), height()) < tolerance;
        }

/**
  * @brief Test of a specific point lies within an area
  * @param point Point to test against
  * @param tolerance
  * @return boolean true of the point is within the area
  */
bool LC_Rect::inArea(const Coordinate& point, const double tolerance) const {
    return point.x >= m_minP.x - tolerance && point.x <= m_maxP.x + tolerance && point.y >= m_minP.y - tolerance && point.y <= m_maxP.y +
        tolerance;
}

/**
 * @brief inArea
 * test if this object's fit's fully in area
 * @param area
 * @return
 */
bool LC_Rect::inArea(const Area& area) const {
    return m_minP.x >= area.m_minP.x && m_minP.y >= area.m_minP.y && m_maxP.x <= area.m_maxP.x && m_maxP.y <= area.m_maxP.y;
}

/**
 * @brief overlaps
 * returns true if any overlap is happening between the two area's, even if otherArea fit's within this area
 * @param otherArea
 * @return
 */
bool LC_Rect::overlaps(const Area& otherArea) const {
    return intersects(otherArea);
}

/**
 * @brief numCornersInside
 * count the number of corners this object has in otherArea
 * @param otherArea
 * @return
 */
short LC_Rect::numCornersInside(const Area& otherArea) const {
    short pointsInside = 0;

    if (otherArea.inArea(m_minP)) {
        pointsInside++;
    }

    if (otherArea.inArea(m_maxP)) {
        pointsInside++;
    }

    if (otherArea.inArea(upperLeftCorner())) {
        pointsInside++;
    }

    if (otherArea.inArea(lowerRightCorner())) {
        pointsInside++;
    }

    return pointsInside;
}

/**
 * @brief merge
 * two area's and expand if required to largest containing area
 * @param other
 * @return
 */
Area LC_Rect::merge(const Area& other) const {
    return {
        {std::min(other.minP().x, this->minP().x), std::min(other.minP().y, this->minP().y)},
        {std::max(other.maxP().x, this->maxP().x), std::max(other.maxP().y, this->maxP().y)}
    };
}

/**
 * @brief merge
 * two area's and expand if required to largest containing area
 * @param other
 * @return
 */
Area LC_Rect::merge(const Coordinate& other) const {
    return {
        {std::min(other.x, this->minP().x), std::min(other.y, this->minP().y)},
        {std::max(other.x, this->maxP().x), std::max(other.y, this->maxP().y)}
    };
}

/**
 * @brief merge
 * two area's and expand if required to largest containing area
 * @param other
 * @param tolerance, tolerance to detect zero size intersection
 * @return
 */
Area LC_Rect::intersection(const Area& other, const double tolerance) const {
    const Area ret{
        {std::max(other.minP().x, this->minP().x), std::max(other.minP().y, this->minP().y)},
        {std::min(other.maxP().x, this->maxP().x), std::min(other.maxP().y, this->maxP().y)}
    };
    if (ret.width() < tolerance || ret.height() < tolerance) {
        return {};
    }
    return ret;
}

bool LC_Rect::intersects(const Area& rhs, const double tolerance) const {
    return maxP().x + tolerance >= rhs.minP().x && maxP().y + tolerance >= rhs.minP().y && rhs.maxP().x + tolerance >= minP().x && rhs.
        maxP().y + tolerance >= minP().y;
}

/**
 * @brief top
 * vector of this area
 * @return
 */
Coordinate LC_Rect::top() const {
    return Vector(upperLeftCorner(), m_maxP);
}

/**
 * @brief bottom
 * vector of this area
 * @return
 */
Coordinate LC_Rect::bottom() const {
    return Vector(m_minP, lowerRightCorner());
}

/**
 * @brief left
 * vector for this area
 * @return
 */
Coordinate LC_Rect::left() const {
    return Vector(m_minP, upperLeftCorner());
}

/**
 * @brief right
 * vector of this area
 * @return
 */
Coordinate LC_Rect::right() const {
    return Vector(lowerRightCorner(), m_maxP);
}

/**
 * Increase the area on each side by increaseBy
 */
Area LC_Rect::increaseBy(const double increaseBy) const {
    return {m_minP - increaseBy, m_maxP + increaseBy};
}

std::array<Coordinate, 4> LC_Rect::vertices() const {
    return {{lowerLeftCorner(), lowerRightCorner(), upperRightCorner(), upperLeftCorner()}};
}

std::ostream& operator<<(std::ostream& os, const Area& area) {
    os << "Area(" << area.minP() << " " << area.maxP() << ")";
    return os;
}

// fixme - sand - move to tests
/*void LC_Rect::unitTest() {
 LC_Rect const rect0{{0., 0.}, {1., 1.}};
 LC_Rect const rect1{{0.5, 0.5}, {1.5, 1.5}};
 LC_Rect const rect2{{1.5, 1.5}, {2.5, 2.5}};
 LC_Rect const rect3{{0.0, 1.}, {1.5, 1.5}};

 //intersects() tests
 INTERT_TEST(rect0.intersects(rect1))
 INTERT_TEST(rect1.intersects(rect0))
 INTERT_TEST(rect1.intersects(rect2))
 INTERT_TEST(rect2.intersects(rect1))

 INTERT_TEST(!rect0.intersects(rect2))
 INTERT_TEST(!rect2.intersects(rect0))

 INTERT_TEST(rect2.intersects(rect3))
 INTERT_TEST(rect3.intersects(rect2))

 // inArea() tests
 INTERT_TEST(rect0.inArea({0.1, 0.1}))
 INTERT_TEST(rect0.inArea({0.5, 0.5}))

 INTERT_TEST(!rect0.inArea({1.1, 1.1}))
 INTERT_TEST(!rect0.inArea({-1.1, -1.1}))

}*/
