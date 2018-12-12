/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include <iostream>
#include <cmath>
#include <algorithm>
#include "rs_vector.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "lc_rect.h"

#ifdef EMU_C99
#include "emu_c99.h" /* remainder() */
#endif

/**
 * Constructor for a point with given coordinates.
 */
RS_Vector::RS_Vector(double vx, double vy, double vz):
	x(vx)
  ,y(vy)
  ,z(vz)
  ,valid(true)
{
}

/**
 * Constructor for a unit vector with given angle
 */
RS_Vector::RS_Vector(double angle):
	x(cos(angle))
  ,y(sin(angle))
  ,valid(true)
{
}

/**
 * Constructor for a point with given valid flag.
 *
 * @param valid true: a valid vector with default coordinates is created.
 *              false: an invalid vector is created
 */
RS_Vector::RS_Vector(bool valid):
	valid(valid)
{
}

RS_Vector::operator bool() const
{
	return valid;
}

/**
 * Sets to a unit vector by the direction angle
 */
void RS_Vector::set(double angle) {
    x = cos(angle);
    y = sin(angle);
    z = 0.;
    valid = true;
}

/**
 * Sets a new position for the vector.
 */
void RS_Vector::set(double vx, double vy, double vz) {
    x = vx;
    y = vy;
    z = vz;
    valid = true;
}

/**
 * Sets a new position for the vector in polar coordinates.
 */
void RS_Vector::setPolar(double radius, double angle) {
    x = radius * cos(angle);
    y = radius * sin(angle);
    z = 0.0;
    valid = true;
}

RS_Vector RS_Vector::polar(double rho, double theta){
	return {rho*cos(theta), rho*sin(theta), 0.};
}

/**
 * @return The angle from zero to this vector (in rad).
 */
double RS_Vector::angle() const {
	return RS_Math::correctAngle(atan2(y,x));
}

/**
 * @return The angle from this and the given coordinate (in rad).
 */
double RS_Vector::angleTo(const RS_Vector& v) const {
	if (!valid || !v.valid) return 0.0;
	return (v-(*this)).angle();
}

/**
 * @return The angle from between two vectors using the current vector as the center
 * return 0, if the angle is not well defined
 */
double RS_Vector::angleBetween(const RS_Vector& v1, const RS_Vector& v2) const {
	if (!valid || !v1.valid || !v2.valid) return 0.0;
	RS_Vector const vStart(v1 - (*this));
	RS_Vector const vEnd(v2 - (*this));
	return RS_Math::correctAngle( atan2( vStart.x*vEnd.y-vStart.y*vEnd.x, vStart.x*vEnd.x+vStart.y*vEnd.y));
}

/**
 * @return Magnitude (length) of the vector.
 */
double RS_Vector::magnitude() const {
    double ret(0.0);
    // Note that the z coordinate is also needed for 2d
    //   (due to definition of crossP())
	if (valid)
		ret = hypot(hypot(x, y), z);

    return ret;
}

/**
  * @return square of vector length
  */
double RS_Vector::squared() const {
    // Note that the z coordinate is also needed for 2d
    //   (due to definition of crossP())
	if (valid)
        return x*x + y*y + z*z;
    return RS_MAXDOUBLE;
}

/**
  * @return square of vector length
  */
double RS_Vector::squaredTo(const RS_Vector& v1) const
{
    if (valid && v1.valid) {
        return  (*this - v1).squared();
    }
    return RS_MAXDOUBLE;
}

/**
 *
 */
RS_Vector RS_Vector::lerp(const RS_Vector& v, double t) const {
	return {x+(v.x-x)*t, y+(v.y-y)*t};
}

/**
 * @return The distance between this and the given coordinate.
 */
double RS_Vector::distanceTo(const RS_Vector& v) const {
    if (!valid || !v.valid) {
        return RS_MAXDOUBLE;
    }
    else {
        return (*this-v).magnitude();
    }
}

/**
 * @return true is this vector is within the given range.
 */
bool RS_Vector::isInWindow(const RS_Vector& firstCorner,
						   const RS_Vector& secondCorner) const {
	if (!valid) return false;
	return LC_Rect{firstCorner, secondCorner}.inArea(*this);
}

/**
 * @return true is this vector is within the given range
 * of ordered vectors
 */
bool RS_Vector::isInWindowOrdered(const RS_Vector& vLow,
								  const RS_Vector& vHigh) const {
	if(!valid) return false;
	return (x>=vLow.x && x<=vHigh.x && y>=vLow.y && y<=vHigh.y);
}

/**
 * move to the closest integer point
 */
RS_Vector RS_Vector::toInteger() {
	x = rint(x);
	y = rint(y);
    return *this;
}

/**
 * Moves this vector by the given offset. Equal to the operator +=.
 */
RS_Vector RS_Vector::move(const RS_Vector& offset) {
    *this+=offset;
    return *this;
}

/**
 * Rotates this vector around 0/0 by the given angle.
 */
RS_Vector RS_Vector::rotate(double ang) {
	rotate(RS_Vector{ang});
    return *this;
}

/**
 * Rotates this vector around 0/0 by the given vector
 * if the vector is a unit, then, it's the same as rotating around
 * 0/0 by the angle of the vector
 */
RS_Vector RS_Vector::rotate(const RS_Vector& angleVector) {
	double x0 = x * angleVector.x - y * angleVector.y;
	y = x * angleVector.y + y * angleVector.x;
	x = x0;

	return *this;
}

/**
 * Rotates this vector around the given center by the given angle.
 */
RS_Vector RS_Vector::rotate(const RS_Vector& center, double ang) {
    *this = center + (*this-center).rotate(ang);
    return *this;
}
RS_Vector RS_Vector::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    *this = center + (*this-center).rotate(angleVector);
    return *this;
}

/**
 * Scales this vector by the given factors with 0/0 as center.
 */
RS_Vector RS_Vector::scale(double factor) {
    x *= factor;
    y *= factor;
    return *this;
}

/**
 * Scales this vector by the given factors with 0/0 as center.
 */
RS_Vector RS_Vector::scale(const RS_Vector& factor) {
    x *= factor.x;
    y *= factor.y;
    return *this;
}

RS_Vector RS_Vector::scale(const RS_Vector& factor) const{
	return {x*factor.x, y*factor.y};
}

/**
 * Scales this vector by the given factors with the given center.
 */
RS_Vector RS_Vector::scale(const RS_Vector& center, const RS_Vector& factor) {
    *this = center + (*this-center).scale(factor);
    return *this;
}



/**
 * Mirrors this vector at the given axis, defined by two points on axis.
 */
RS_Vector RS_Vector::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {

    RS_Vector direction(axisPoint2-axisPoint1);
    double a= direction.squared();
    RS_Vector ret(false);
    if(a<RS_TOLERANCE2) {
        return ret;
    }
    ret= axisPoint1 + direction* dotP(*this - axisPoint1,direction)/a; //projection point
    *this = ret + ret - *this;

    return *this;
}

/**
 * Streams the vector components to stdout. e.g.: "1/4/0"
 */
std::ostream& operator << (std::ostream& os, const RS_Vector& v) {
    if(v.valid) {
        os << v.x << "/" << v.y << "/" << v.z;
    } else {
        os << "invalid vector";
    }
    return os;
}



/**
 * binary + operator.
 */
RS_Vector RS_Vector::operator + (const RS_Vector& v) const {
	return {x + v.x, y + v.y, z + v.z};
}



/**
 * binary - operator.
 */
RS_Vector RS_Vector::operator - (const RS_Vector& v) const {
	return {x - v.x, y - v.y, z - v.z};
}

RS_Vector RS_Vector::operator + (double d) const {
	return {x + d, y + d, z + d};
}

RS_Vector RS_Vector::operator - (double d) const {
	return {x - d, y - d, z - d};
}

RS_Vector RS_Vector::operator * (const RS_Vector& v) const {
	return {x * v.x, y * v.y, z * v.z};
}

RS_Vector RS_Vector::operator / (const RS_Vector& v) const {
	if(fabs(v.x)> RS_TOLERANCE && fabs(v.y)>RS_TOLERANCE)
		return {x / v.x, y / v.y, std::isnormal(v.z)?z / v.z:z};

	return *this;
}

/**
 * binary * operator.
 */
RS_Vector RS_Vector::operator * (double s) const {
	return {x * s, y * s, z * s};
}

/**
 * binary / operator.
 */
RS_Vector RS_Vector::operator / (double s) const {
	if(fabs(s)> RS_TOLERANCE)
		return {x / s, y / s, z / s};

	return *this;
}

/**
 * unary - operator.
 */
RS_Vector RS_Vector::operator - () const {
	return {-x, -y, -z};
}

/**
 * Scalarproduct (dot product).
 */
double RS_Vector::dotP(const RS_Vector& v1) const
{
    return x*v1.x+y*v1.y;
}

double RS_Vector::dotP(const RS_Vector& v1, const RS_Vector& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

/**
 * Get position of \p pos in line \p start -> \p end,
 * as a factor of line length.
 *
 * @param start Start point of the line
 * @param end End point of the line
 * @param pos Point to calculate
 * @return double factor of line length,
 *         factor == 0.0 : \p pos is same as start point
 *         factor == 1.0 : \p pos is same as end point
 *         factor < 0.0 : \p pos is in opposite direction
 *         factor > 1.0 : \p pos is behind end point
 *         factor > 0.0 and < 1.0 : \p pos is somewhere between start and end
 */
double RS_Vector::posInLine(const RS_Vector& start,
                            const RS_Vector& end,
                            const RS_Vector& pos)
{
    RS_Vector dirEnd {end - start};
    RS_Vector dirPos {pos - start};
    double lenSquared {dirEnd.squared()};

    if( RS_TOLERANCE2 > lenSquared ) {
        // line too short
        return start.distanceTo( pos);
    }

    return dotP( dirPos, dirEnd) / lenSquared;
}

/** switch x,y for all vectors */
RS_Vector RS_Vector::flipXY(void) const{
	return {y, x};
}

/**
 * += operator. Assert: both vectors must be valid.
 */
RS_Vector RS_Vector::operator += (const RS_Vector& v) {
    x += v.x;
    y += v.y;
    z += v.z;
	return *this;
}

/**
 * -= operator
 */
RS_Vector RS_Vector::operator -= (const RS_Vector& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
	return *this;
}

RS_Vector RS_Vector::operator *= (const RS_Vector& v) {
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

RS_Vector RS_Vector::operator /= (const RS_Vector& v) {
	if (fabs(v.x)> RS_TOLERANCE && fabs(v.y)>RS_TOLERANCE){
		x /= v.x;
		y /= v.y;
		if (std::isnormal(v.z))
			z /= v.z;
	}
	return *this;
}

/**
 * *= operator
 */
RS_Vector RS_Vector::operator *= (double s) {
    x *= s;
    y *= s;
    z *= s;
	return *this;
}
/**
 * /= operator
 */
RS_Vector RS_Vector::operator /= (double s) {
	if(fabs(s)>RS_TOLERANCE) {
		x /= s;
		y /= s;
		z /= s;
	}
	return *this;
}

/**
 * == operator
 */
bool RS_Vector::operator == (const RS_Vector& v) const {
	return (x==v.x && y==v.y && z==v.z && valid && v.valid);
}

bool RS_Vector::operator == (bool valid) const
{
	return this->valid == valid;
}

bool RS_Vector::operator != (bool valid) const
{
	return this->valid != valid;
}

/**
 * @return A vector with the minimum components from the vectors v1 and v2.
 * These might be mixed components from both vectors.
 */
RS_Vector RS_Vector::minimum (const RS_Vector& v1, const RS_Vector& v2) {
	if (!v2) return v1;
	if (!v1) return v2;
	return {std::min(v1.x, v2.x),
				std::min(v1.y, v2.y),
				std::min(v1.z, v2.z)
	};
}

/**
 * @return A vector with the maximum values from the vectors v1 and v2
 */
RS_Vector RS_Vector::maximum (const RS_Vector& v1, const RS_Vector& v2) {
	if (!v2) return v1;
	if (!v1) return v2;
	return {std::max(v1.x, v2.x),
				std::max(v1.y, v2.y),
				std::max(v1.z, v2.z)
	};
}

/**
 * @return Cross product of two vectors.
 *  we don't need cross product for 2D vectors
 */
RS_Vector RS_Vector::crossP(const RS_Vector& v1, const RS_Vector& v2) {
	return {v1.y*v2.z - v1.z*v2.y,
				v1.z*v2.x - v1.x*v2.z,
				v1.x*v2.y - v1.y*v2.x};
}

/**
 * Constructor for no solution.
 */
RS_VectorSolutions::RS_VectorSolutions():
	vector(0)
  ,tangent(false)
{
}

RS_VectorSolutions::RS_VectorSolutions(const std::vector<RS_Vector>& l):
	vector( l.begin(), l.end())
  ,tangent(false)
{
}

/**
 * Constructor for num solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(int num):
	vector(num, RS_Vector(false))
  ,tangent(false)
{
}

RS_VectorSolutions::RS_VectorSolutions(std::initializer_list<RS_Vector> const& l):
	vector(l)
  ,tangent(false)
{
}

/**
 * Allocates 'num' vectors.
 */
void RS_VectorSolutions::alloc(size_t num) {
	if(num<=vector.size()){
		vector.resize(num);
	}else{
		const std::vector<RS_Vector> v(num - vector.size());
		vector.insert(vector.end(), v.begin(), v.end());
	}
}

RS_Vector RS_VectorSolutions::get(size_t i) const
{
	if(i<vector.size())
		return vector.at(i);
	return {};
}

const RS_Vector&  RS_VectorSolutions::operator [] (const size_t i) const
{
	return vector[i];
}

RS_Vector&  RS_VectorSolutions::operator [] (const size_t i)
{
	return vector[i];
}

size_t RS_VectorSolutions::size() const
{
	return vector.size();
}
/**
 * Deletes vector array and resets everything.
 */
void RS_VectorSolutions::clear() {
    vector.clear();
    tangent = false;
}

/**
 * @return vector solution number i or an invalid vector if there
 * are less solutions.
 */
const RS_Vector& RS_VectorSolutions::at(size_t i) const {
		return vector.at(i);
}

/**
 * @return Number of solutions available.
 */
size_t RS_VectorSolutions::getNumber() const {
    return vector.size();
}

/**
 * @retval true There's at least one valid solution.
 * @retval false There's no valid solution.
 */
bool RS_VectorSolutions::hasValid() const {
	for(const RS_Vector& v: vector)
		if (v.valid)  return true;

    return false;
}

void RS_VectorSolutions::resize(size_t n){
    vector.resize(n);
}

const std::vector<RS_Vector>& RS_VectorSolutions::getVector() const {
    return vector;
}

std::vector<RS_Vector>::const_iterator RS_VectorSolutions::begin() const
{
	return vector.begin();
}

std::vector<RS_Vector>::const_iterator RS_VectorSolutions::end() const
{
	return vector.end();
}

std::vector<RS_Vector>::iterator RS_VectorSolutions::begin()
{
	return vector.begin();
}

std::vector<RS_Vector>::iterator RS_VectorSolutions::end()
{
	return vector.end();
}

void RS_VectorSolutions::push_back(const RS_Vector& v) {
        vector.push_back(v);
}

void RS_VectorSolutions::removeAt(const size_t i){
	if (vector.size()> i)
		vector.erase(vector.begin()+i);
}

RS_VectorSolutions& RS_VectorSolutions::push_back(const RS_VectorSolutions& v) {
	vector.insert(vector.end(), v.begin(), v.end());
    return *this;
}

/**
 * Sets the solution i to the given vector.
 * If i is greater than the current number of solutions available,
 * nothing happens.
 */
void RS_VectorSolutions::set(size_t i, const RS_Vector& v) {
    if (i<vector.size()) {
        vector[i] = v;
    }else{
//            RS_DEBUG->print(RS_Debug::D_ERROR, "set member in vector in RS_VectorSolutions: out of range, %d to size of %d", i,vector.size());
		for(size_t j=vector.size();j<=i;++j)
            vector.push_back(v);
    }
}

/**
 * Sets the tangent flag.
 */
void RS_VectorSolutions::setTangent(bool t) {
    tangent = t;
}

/**
 * @return true if at least one of the solutions is a double solution
 * (tangent).
 */
bool RS_VectorSolutions::isTangent() const {
    return tangent;
}

/**
 * Rotates all vectors around (0,0) by the given angle.
 */
void RS_VectorSolutions::rotate(double ang) {
    RS_Vector angleVector(ang);
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.rotate(angleVector);
		}
	}
}

/**
 * Rotates all vectors around (0,0) by the given angleVector.
 */
void RS_VectorSolutions::rotate(const RS_Vector& angleVector) {
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.rotate(angleVector);
		}
	}
}

/**
 * Rotates all vectors around the given center by the given angle.
 */
void RS_VectorSolutions::rotate(const RS_Vector& center, double ang) {
	const RS_Vector angleVector(ang);
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.rotate(center,angleVector);
		}
	}
}

void RS_VectorSolutions::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.rotate(center, angleVector);
		}
	}
}

/**
 * Move all vectors around the given center by the given vector.
 */
void RS_VectorSolutions::move(const RS_Vector& vp) {
	for (RS_Vector& v: vector) {
		if (v.valid) {
			v.move(vp);
		}
	}
}

/**
 * Scales all vectors by the given factors with the given center.
 */
void RS_VectorSolutions::scale(const RS_Vector& center, const RS_Vector& factor) {
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.scale(center, factor);
		}
	}
}

void RS_VectorSolutions::scale( const RS_Vector& factor) {
	for (auto& vp: vector) {
		if (vp.valid) {
			vp.scale(factor);
        }
    }
}

/**
 * @return vector solution which is the closest to the given coordinate.
 * dist will contain the distance if it doesn't point to NULL (default).
 */
RS_Vector RS_VectorSolutions::getClosest(const RS_Vector& coord,
										 double* dist, size_t* index) const {

	double curDist{0.};
    double minDist = RS_MAXDOUBLE;
	RS_Vector closestPoint{false};
    int pos(0);

	for (size_t i=0; i<vector.size(); i++) {
		if (vector[i].valid) {
			curDist = (coord - vector[i]).squared();

            if (curDist<minDist) {
				closestPoint = vector[i];
                minDist = curDist;
                pos=i;
            }
        }
    }
	if (dist) {
        *dist = sqrt(minDist);
    }
	if (index) {
        *index = pos;
    }
    return closestPoint;
}

/**
  *@ return the closest distance from the first counts rs_vectors
  *@coord, distance to this point
  *@counts, only consider this many points within solution
  */
double RS_VectorSolutions::getClosestDistance(const RS_Vector& coord,
											  int counts)
{
    double ret=RS_MAXDOUBLE*RS_MAXDOUBLE;
    int i=vector.size();
	if (counts < i && counts >= 0) i=counts;
	std::for_each(vector.begin(), vector.begin() + i,
				  [&ret, &coord](RS_Vector const& vp) {
		if(vp.valid) {
			double d=(coord - vp).squared();
			if(d<ret) ret=d;
		}
	}
	);

	return sqrt(ret);
}

/** switch x,y for all vectors */
RS_VectorSolutions RS_VectorSolutions::flipXY(void) const
{
        RS_VectorSolutions ret;
		for(const auto& vp: vector)
			ret.push_back(vp.flipXY());
        return ret;
}

RS_VectorSolutions RS_VectorSolutions::operator = (const RS_VectorSolutions& s) {
    setTangent(s.isTangent());
    vector=s.vector;

    return *this;
}

std::ostream& operator << (std::ostream& os,
                           const RS_VectorSolutions& s) {
	for (const RS_Vector& vp: s){
		os << "(" << vp << ")\n";
    }
    os << " tangent: " << (int)s.isTangent() << "\n";
    return os;
}
