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

#ifndef LC_RECT_H
#define LC_RECT_H
#include "rs_vector.h"
#include <array>

//ported from LibreCAD V3
namespace lc {
namespace geo {

typedef RS_Vector Coordinate;

/**
		  * Class that describes an area or window.
		  */
class Area {

public:
	Area();
	/**
		  * Create a new Area. The coordinates coordA and coordB will be ordered so that minP will always be < maxP
		  * The coordinates are not allowed to describe a volume
		  *
		  * @param CoordA First coordinate of an area
		  * @param CoordB Second coordinate of an area
		  */
	Area(const Coordinate& coordA, const Coordinate& coordB);


	/**
		 * @brief Area
		 * given at a coordinate with a given width and height
		 * @param coordA
		 * @param width
		 * @param height
		 */
	explicit Area(const Coordinate& coord, double width, double height);

	/**
		  * Return the smallest corner (closest to (0,0,0) )
		  */
	const Coordinate& minP() const;

	/**
		  * Return the highest corner
		  */
	const Coordinate& maxP() const;
	/**
		 * @brief topLeftCorner return the upperLeftCorner coordinates
		 * _minP is considered lowerLeft, _maxP is the upperRight
		 * @return {_minP.x, _maxP.y}
		 */
	Coordinate upperLeftCorner() const;
	Coordinate upperRightCorner() const;
	/**
		 * @brief lowerRightCorner return the lowerRight coordinates
		 * _minP is considered lowerLeft, _maxP is the upperRight
		 * @return {_maxP.x, _minP.y}
		 */
	Coordinate lowerLeftCorner() const;
	Coordinate lowerRightCorner() const;

	/**
		 * @brief width
		 * Returns the width of this area
		 * @return
		 */
	double width() const;

	/**
		 * @brief height
		 * Returns the height of this area
		 * @return
		 */
	double height() const;

	/**
		  * @brief Test of a specific point lies within an area
		  * @param point Point to test against
		  * @return boolean true of the point is within the area
		  */
	bool inArea(const Coordinate& point, double tolerance = 0.) const;

	/**
		 * @brief inArea
		 * test if this object's fit's fully in area
		 * @param area
		 * @return
		 */
	bool inArea(const Area& area) const;

	/**
		 * @brief overlaps
		 * returns true if any overlap is happening between the two area's, even if otherArea fit's within this area
		 * @param other
		 * @return
		 */
	bool overlaps(const Area& otherArea) const;

	/**
		 * @brief numCornersInside
		 * count the number of corners this object has in otherArea
		 * @param other
		 * @return
		 */
	short numCornersInside(const Area& otherArea) const;

	/**
		 * @brief merge
		 * two area's and expand if required to largest containing area
		 * @param other
		 * @return
		 */
	Area merge(const Area& other) const;

	/**
		  * @brief merge
		  * two area's and expand if required to largest containing area
		  * @param other
		  * @return
		  */
	Area merge(const Coordinate& other) const;

	/**
		 * @brief merge
		 * two area's and expand if required to largest containing area
		 * @param other
		 * @param tolerance, tolerance to detect zero size intersection
		 * @return
		 */
	Area intersection(const Area& other, double tolerance = 0.) const;

	/**
	 * @brief intersects whether two rectangular area
	 * if the closest distance between two Areas is smaller than tolerance, they
	 * are considered to have intersection
	 * @param rhs the other rect
	 * @return true if closest distance is smaller than or equal to tolerance
	 */
	bool intersects(Area const& rhs, double tolerance = 0.) const;

	/**
		 * @brief top
		 * vector of this area
		 * @return
		 */
	Coordinate top() const;

	/**
		 * @brief bottom
		 * vector of this area
		 * @return
		 */
	Coordinate bottom() const;

	/**
		 * @brief left
		 * vector for this area
		 * @return
		 */
	Coordinate left() const;

	/**
		 * @brief right
		 * vector of this area
		 * @return
		 */
	Coordinate right() const;

	/**
		 * Increase the area on each side by increaseBy
		 */
	Area increaseBy (double increaseBy) const;
	/**
	 * @brief vertices generate vertices of the rectangular area
	 * @return array of vertices by the order {ll, lr, ur, rl} starting with
	 * lower-left corner, i.e. _minP
	 */
	std::array<Coordinate, 4> vertices() const;

	static void unitTest();

private:
	static Coordinate Vector(Coordinate const& p, Coordinate const& q);
	friend std::ostream& operator<<(std::ostream& os, const Area& area);

private:
	Coordinate _minP;
	Coordinate _maxP;
};
}
}

using LC_Rect = lc::geo::Area;
#endif // LC_RECT_H
