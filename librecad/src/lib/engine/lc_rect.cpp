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
#include<iostream>
#include <QDebug>
#include <cassert>
#include "lc_rect.h"

#define INTERT_TEST(s) qDebug()<<"\ntesting " #s; \
	assert(s); \
	qDebug()<<"Passed";

using namespace lc::geo;

Coordinate LC_Rect::Vector(Coordinate const& p, Coordinate const& q) {
	return {q.x - p.x, q.y - p.y};
}

	/**
	  * Create a new Area. The coordinates coordA and coordB will be ordered so that minP will always be < maxP
	  * The coordinates are not allowed to describe a volume
	  *
	  * @param CoordA First coordinate of an area
	  * @param CoordB Second coordinate of an area
	  */
LC_Rect::Area(const Coordinate& coordA, const Coordinate& coordB) :
_minP{std::min(coordA.x, coordB.x), std::min(coordA.y, coordB.y)},
_maxP{std::max(coordA.x, coordB.x), std::max(coordA.y, coordB.y)}
{
}

LC_Rect::Area() : _minP{0., 0.}, _maxP{0., 0.} {}

	/**
	 * @brief Area
	 * given at a coordinate with a given width and height
	 * @param coordA
	 * @param width
	 * @param height
	 */
	LC_Rect::Area(const Coordinate& coord, double width, double height):
Area(coord, {coord.x + width, coord.y + height})
{}

	/**
	  * Return the smallest corner (closest to (0,0,0) )
	  */
	const Coordinate& LC_Rect::minP() const {
		return _minP;
	}

	/**
	  * Return the highest corner
	  */
	const Coordinate& LC_Rect::maxP() const {
		return _maxP;
	}
	/**
	 * @brief topLeftCorner return the upperLeftCorner coordinates
	 * _minP is considered lowerLeft, _maxP is the upperRight
	 * @return {_minP.x, _maxP.y}
	 */
	Coordinate LC_Rect::upperLeftCorner() const {
	  return {_minP.x, _maxP.y};
	}
	Coordinate LC_Rect::upperRightCorner() const {
	  return _maxP;
	}

	/**
	 * @brief lowerRightCorner return the lowerRight coordinates
	 * _minP is considered lowerLeft, _maxP is the upperRight
	 * @return {_maxP.x, _minP.y}
	 */
	Coordinate LC_Rect::lowerRightCorner() const {
	  return {_maxP.x, _minP.y};
	}

	Coordinate LC_Rect::lowerLeftCorner() const {
	  return _minP;
	}

	/**
	 * @brief width
	 * Returns the wid th of this area
	 * @return
	 */
	double LC_Rect::width() const {
		return _maxP.x - _minP.x;
	}

	/**
	 * @brief height
	 * Returns the height f this area
	 * @return
	 */
	double LC_Rect::height() const {
		return _maxP.y - _minP.y;
	}

	/**
	  * @brief Test of a specific point lies within an area
	  * @param point Point to test against
	  * @return boolean true of the point is within the area
	  */
	bool LC_Rect::inArea(const Coordinate& point, double tolerance) const {
		return (point.x >= _minP.x - tolerance && point.x <= _maxP.x + tolerance && point.y >= _minP.y - tolerance && point.y <= _maxP.y + tolerance);
	}

	/**
	 * @brief inArea
	 * test if this object's fit's fully in area
	 * @param area
	 * @return
	 */
	bool LC_Rect::inArea(const Area& area) const {
		return _minP.x >= area._minP.x && _minP.y >= area._minP.y && _maxP.x <= area._maxP.x && _maxP.y <= area._maxP.y;
	}

	/**
	 * @brief overlaps
	 * returns true if any overlap is happening between the two area's, even if otherArea fit's within this area
	 * @param other
	 * @return
	 */
	bool LC_Rect::overlaps(const Area& otherArea) const {
		return intersects(otherArea);
	}

	/**
	 * @brief numCornersInside
	 * count the number of corners this object has in otherArea
	 * @param other
	 * @return
	 */
	short LC_Rect::numCornersInside(const Area& otherArea) const  {
		short pointsInside = 0;

		if (otherArea.inArea(_minP)) {
			pointsInside++;
		}

		if (otherArea.inArea(_maxP)) {
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
	Area LC_Rect::intersection(const Area& other, double tolerance) const {

	  Area const ret{
		  {std::max(other.minP().x, this->minP().x), std::max(other.minP().y, this->minP().y)},
		  {std::min(other.maxP().x, this->maxP().x), std::min(other.maxP().y, this->maxP().y)}
		};
		if (ret.width() < tolerance || ret.height() < tolerance) {
			return {};
		}
		return ret;
	}

	bool LC_Rect::intersects(Area const& rhs, double tolerance) const {
		return maxP().x + tolerance >= rhs.minP().x &&
				maxP().y + tolerance >= rhs.minP().y &&
				rhs.maxP().x + tolerance >= minP().x &&
				rhs.maxP().y + tolerance >= minP().y;
	}

	/**
	 * @brief top
	 * vector of this area
	 * @return
	 */
	Coordinate LC_Rect::top() const {
		return Vector(upperLeftCorner(), _maxP);
	}

	/**
	 * @brief bottom
	 * vector of this area
	 * @return
	 */
	Coordinate LC_Rect::bottom() const {
		return Vector(_minP, lowerRightCorner());
	}

	/**
	 * @brief left
	 * vector for this area
	 * @return
	 */
	Coordinate LC_Rect::left() const {
		return Vector(_minP, upperLeftCorner());
	}

	/**
	 * @brief right
	 * vector of this area
	 * @return
	 */
	Coordinate LC_Rect::right() const {
		return Vector(lowerRightCorner(), _maxP);
	}

	/**
	 * Increase the area on each side by increaseBy
	 */
	Area LC_Rect::increaseBy(double increaseBy) const {
		return {_minP - increaseBy, _maxP + increaseBy};
	}

	std::array<Coordinate, 4> LC_Rect::vertices() const
	{
		return {{lowerLeftCorner(), lowerRightCorner(),
					upperRightCorner(), upperLeftCorner()}};
	}

	std::ostream& operator<<(std::ostream& os, const Area& area) {
		os << "Area(" << area.minP() << " " << area.maxP() << ")";
		return os;
	}

void LC_Rect::unitTest() {
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

}

