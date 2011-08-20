/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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


#include "rs_vector.h"

//#include <values.h>

#include "rs_debug.h"
#include "rs_math.h"
#include "rs_constructionline.h"

/**
 * Constructor for a point with default coordinates.
 */
RS_Vector::RS_Vector() {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    set(0.0, 0.0, 0.0);
}



/**
 * Constructor for a point with given coordinates.
 */
RS_Vector::RS_Vector(double vx, double vy, double vz) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    set(vx, vy, vz);
}

/**
 * Constructor for a unit vector with given angle
 */
RS_Vector::RS_Vector(double angle) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    x = cos(angle);
    y = sin(angle);
    z = 0.0;
    valid = true;
}


/**
 * Constructor for a point with given coordinates in an array
 * or three doubles.
 */
//RS_Vector::RS_Vector(double v[]) {
//    set(v[0], v[1], v[2]);
//}


/**
 * Constructor for a point with given valid flag.
 *
 * @param valid true: a valid vector with default coordinates is created.
 *              false: an invalid vector is created
 */
RS_Vector::RS_Vector(bool valid) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    set(0.0, 0.0, 0.0);
    this->valid = valid;
}


/**
 * Destructor.
 */
RS_Vector::~RS_Vector() {
    //RS_DEBUG->print("RS_Vector::~RS_Vector");
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



/**
 * @return The angle from zero to this vector (in rad).
 */
double RS_Vector::angle() const {
    return RS_Math::correctAngle(atan2(y,x));
//    double ret = 0.0;
//    double m = magnitude();
//
//    if (m>1.0e-6) {
//		double dp = dotP(*this, RS_Vector(1.0, 0.0));
//		RS_DEBUG->print("RS_Vector::angle: dp/m: %f/%f", dp, m);
//		if (dp/m>=1.0) {
//			ret = 0.0;
//		}
//		else if (dp/m<-1.0) {
//			ret = M_PI;
//		}
//		else {
//        	ret = acos( dp / m);
//		}
//        if (y<0.0) {
//            ret = 2*M_PI - ret;
//        }
//    }
//    //std::cout<<"New angle="<<fmod(2*M_PI+atan2(y,x),2*M_PI)<<"\tatan2("<<y<<','<<x<<")"<<atan2(y,x)<<std::endl;
//
//    return ret;
}



/**
 * @return The angle from this and the given coordinate (in rad).
 */
double RS_Vector::angleTo(const RS_Vector& v) const {
    if (!valid || !v.valid) {
        return 0.0;
    }
    else {
        return (v-(*this)).angle();
    }
}



/**
 * @return Magnitude (length) of the vector.
 */
double RS_Vector::magnitude() const {
    double ret(0.0);
    // Note that the z coordinate is also needed for 2d
    //   (due to definition of crossP())
    if (valid) {
        ret = sqrt(x*x + y*y + z*z);
    }

    return ret;
}


/**
 *
 */
RS_Vector RS_Vector::lerp(const RS_Vector& v, double t) const {
    return RS_Vector(x+(v.x-x)*t, y+(v.y-y)*t);
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
                           const RS_Vector& secondCorner) {

    double minX = std::min(firstCorner.x, secondCorner.x);
    double maxX = std::max(firstCorner.x, secondCorner.x);
    double minY = std::min(firstCorner.y, secondCorner.y);
    double maxY = std::max(firstCorner.y, secondCorner.y);

    return (x>=minX && x<=maxX && y>=minY && y<=maxY);
}



/**
 * Moves this vector by the given offset. Equal to the operator +=.
 */
RS_Vector RS_Vector::move(RS_Vector offset) {
    *this+=offset;
    return *this;
}



/**
 * Rotates this vector around 0/0 by the given angle.
 */
RS_Vector RS_Vector::rotate(double ang) {
    RS_DEBUG->print("RS_Vector::rotate: angle: %f", ang);

    double r = magnitude();

    RS_DEBUG->print("RS_Vector::rotate: r: %f", r);

    double a = angle() + ang;

    RS_DEBUG->print("RS_Vector::rotate: a: %f", a);

    x = cos(a) * r;
    y = sin(a) * r;

    RS_DEBUG->print("RS_Vector::rotate: x/y: %f/%f", x, y);

    return *this;
}

/**
 * Rotates this vector around 0/0 by the given vector
 * if the vector is a unit, then, it's the same as rotating around 
 * 0/0 by the angle of the vector
 */
RS_Vector RS_Vector::rotate(RS_Vector angleVector) {
        if( angleVector.valid) {
    RS_DEBUG->print("RS_Vector::rotate: rotating Vecotr: %g/%g", x,y);
    RS_DEBUG->print("RS_Vector::rotate: rotating by Vecotr: %g/%g", angleVector.x,angleVector.y);
    x = x * angleVector.x - y * angleVector.y;
    y = x * angleVector.y + y * angleVector.x;

    RS_DEBUG->print("RS_Vector::rotate: rotated x/y: %f/%f", x, y);
        } else {
    RS_DEBUG->print("RS_Vector::rotate: rotating by invalid RS_Vector");
        }

    return *this;
}


/**
 * Rotates this vector around the given center by the given angle.
 */
RS_Vector RS_Vector::rotate(RS_Vector center, double ang) {
    *this = center + (*this-center).rotate(ang);
    return *this;
}


/**
 * Scales this vector by the given factors with 0/0 as center.
 */
RS_Vector RS_Vector::scale(RS_Vector factor) {
    x *= factor.x;
    y *= factor.y;
    return *this;
}



/**
 * Scales this vector by the given factors with the given center.
 */
RS_Vector RS_Vector::scale(RS_Vector center, RS_Vector factor) {
    *this = center + (*this-center).scale(factor);
    return *this;
}



/**
 * Mirrors this vector at the given axis.
 */
RS_Vector RS_Vector::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    /*
    RS_ConstructionLine axis(NULL,
    	RS_ConstructionLineData(axisPoint1, axisPoint2));

    RS_Vector xp = axis.getNearestPointOnEntity(*this);
    xp = xp - (*this);
    (*this) += (xp*2);
    */

    double phi1 = axisPoint1.angleTo(*this);
    double phi2 = axisPoint1.angleTo(axisPoint2) - phi1;
    double r1 = axisPoint1.distanceTo(*this);
    double r2 = axisPoint2.distanceTo(*this);

    if (r1<1.0e-6 || r2<1.0e-6) {
        // point touches one axis point
    }
    else {
        setPolar(r1, phi1 + 2*phi2);
        (*this) += axisPoint1;
    }

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
    return RS_Vector(x + v.x, y + v.y, z + v.z);
}



/**
 * binary - operator.
 */
RS_Vector RS_Vector::operator - (const RS_Vector& v) const {
    return RS_Vector(x - v.x, y - v.y, z - v.z);
}


/**
 * binary * operator.
 */
RS_Vector RS_Vector::operator * (double s) const {
    return RS_Vector(x * s, y * s, z * s);
}



/**
 * binary / operator.
 */
RS_Vector RS_Vector::operator / (double s) const {
    return RS_Vector(x / s, y / s, z / s);
}



/**
 * unary - operator.
 */
RS_Vector RS_Vector::operator - () const {
    return RS_Vector(-x, -y, -z);
}



/**
 * Scalarproduct (dot product).
 */
double RS_Vector::dotP(const RS_Vector& v1, const RS_Vector& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}



/**
 * += operator. Assert: both vectors must be valid.
 */
void RS_Vector::operator += (const RS_Vector& v) {
    x += v.x;
    y += v.y;
    z += v.z;
}


/**
 * -= operator
 */
void RS_Vector::operator -= (const RS_Vector& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
}



/**
 * *= operator
 */
void RS_Vector::operator *= (double s) {
    x *= s;
    y *= s;
    z *= s;
}



/**
 * == operator
 */
bool RS_Vector::operator == (const RS_Vector& v) const {
    return (x==v.x && y==v.y && z==v.z && valid==v.valid);
}



/**
 * @return A vector with the minimum components from the vectors v1 and v2.
 * These might be mixed components from both vectors.
 */
RS_Vector RS_Vector::minimum (const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector (std::min(v1.x, v2.x),
                      std::min(v1.y, v2.y),
                      std::min(v1.z, v2.z));
}



/**
 * @return A vector with the maximum values from the vectors v1 and v2
 */
RS_Vector RS_Vector::maximum (const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector (std::max(v1.x, v2.x),
                      std::max(v1.y, v2.y),
                      std::max(v1.z, v2.z));
}



/**
 * @return Cross product of two vectors.
 */
RS_Vector RS_Vector::crossP(const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector(v1.y*v2.z - v1.z*v2.y,
                     v1.z*v2.x - v1.x*v2.z,
                     v1.x*v2.y - v1.y*v2.x);
}


/**
 * Constructor for no solution.
 */
RS_VectorSolutions::RS_VectorSolutions() : vector(NULL) {
    clean();
}



/**
 * Copy constructor
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_VectorSolutions& s)
    : vector(NULL) {
    alloc(s.getNumber());
    setTangent(s.isTangent());
    for (int i=0; i<s.getNumber(); ++i) {
        set(i, s.get(i));
    }
}



/**
 * Constructor for num solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(int num) : vector(NULL) {
    alloc(num);
}



/**
 * Constructor for one solution.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1) {
    num = 1;
    vector = new RS_Vector[num];
    vector[0] = v1;
    tangent = false;
}



/**
 * Constructor for two solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2) {
    num = 2;
    vector = new RS_Vector[num];
    vector[0] = v1;
    vector[1] = v2;
    tangent = false;
}



/**
 * Constructor for three solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2,
                                       const RS_Vector& v3) {
    num = 3;
    vector = new RS_Vector[num];
    vector[0] = v1;
    vector[1] = v2;
    vector[2] = v3;
    tangent = false;
}


/**
 * Constructor for four solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2,
                                       const RS_Vector& v3,
                                       const RS_Vector& v4) {
    num = 4;
    vector = new RS_Vector[num];
    vector[0] = v1;
    vector[1] = v2;
    vector[2] = v3;
    vector[3] = v4;
    tangent = false;
}


/**
 * Constructor for four solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2,
                                       const RS_Vector& v3,
                                       const RS_Vector& v4,
                                       const RS_Vector& v5) {
    num = 5;
    vector = new RS_Vector[num];
    vector[0] = v1;
    vector[1] = v2;
    vector[2] = v3;
    vector[3] = v4;
    vector[4] = v5;
    tangent = false;
}



/**
 * Destructor.
 */
RS_VectorSolutions::~RS_VectorSolutions() {
    clean();
}


/**
 * Allocates 'num' vectors.
 */
void RS_VectorSolutions::alloc(int num) {
    clean();
    this->num = num;
    vector = new RS_Vector[num];
    for (int i=0; i<num; ++i)  {
        vector[i] = RS_Vector(false);
    }
    tangent = false;
}

/**
 * Deletes vector array and resets everything.
 */
void RS_VectorSolutions::clean() {
    if (vector!=NULL) {
        delete[] vector;
    }
    vector = NULL;
    num = 0;
    tangent = false;
}



/**
 * @return vector solution number i or an invalid vector if there
 * are less solutions.
 */
RS_Vector RS_VectorSolutions::get(int i) const {
    if (i<num) {
        return vector[i];
    } else {
        return RS_Vector(false);
    }
}



/**
 * @return Number of solutions available.
 */
int RS_VectorSolutions::getNumber() const {
    return num;
}



/**
 * @retval true There's at least one valid solution.
 * @retval false There's no valid solution.
 */
bool RS_VectorSolutions::hasValid() const {
    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            return true;
        }
    }

    return false;
}



/**
 * Sets the solution i to the given vector.
 * If i is greater than the current number of solutions available,
 * nothing happens.
 */
void RS_VectorSolutions::set(int i, const RS_Vector& v) {
    if (i<num) {
        vector[i] = v;
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
    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            vector[i].rotate(angleVector);
        }
    }
}

/**
 * Rotates all vectors around (0,0) by the given angleVector.
 */
void RS_VectorSolutions::rotate(RS_Vector angleVector) {
    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            vector[i].rotate(angleVector);
        }
    }
}

/**
 * Rotates all vectors around the given center by the given angle.
 */
void RS_VectorSolutions::rotate(RS_Vector center, double ang) {
    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            vector[i].rotate(center, ang);
        }
    }
}

/**
 * Move all vectors around the given center by the given vector.
 */
void RS_VectorSolutions::move(RS_Vector vp) {
    for (int i=0; i<num; i++) {
        if (vector[i].valid) vector[i].move(vp);
    }
}


/**
 * Scales all vectors by the given factors with the given center.
 */
void RS_VectorSolutions::scale(RS_Vector center, RS_Vector factor) {
    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            vector[i].scale(center, factor);
        }
    }
}


/**
 * @return vector solution which is the closest to the given coordinate.
 * dist will contain the distance if it doesn't point to NULL (default).
 */
RS_Vector RS_VectorSolutions::getClosest(const RS_Vector& coord,
        double* dist, int* index) const {

    double curDist;
    double minDist = RS_MAXDOUBLE;
    RS_Vector closestPoint(false);

    for (int i=0; i<num; i++) {
        if (vector[i].valid) {
            curDist = coord.distanceTo(vector[i]);

            if (curDist<minDist) {
                closestPoint = vector[i];
                minDist = curDist;
                if (dist!=NULL) {
                    *dist = curDist;
                }
                if (index!=NULL) {
                    *index = i;
                }
            }
        }
    }

    return closestPoint;
}
double RS_VectorSolutions::getClosestDistance(const RS_Vector& coord,
        int counts)
{
    double ret=RS_MAXDOUBLE;
    int i=getNumber();
    if (counts<i) i=counts;
    if (i<1) return(ret);
    for(int j=0; j<i; j++) {
        double d=coord.distanceTo(get(j));
        if(d<ret) ret=d;
    }
    return ret;
}


RS_VectorSolutions RS_VectorSolutions::operator = (const RS_VectorSolutions& s) {
    alloc(s.getNumber());
    setTangent(s.isTangent());
    for (int i=0; i<s.getNumber(); ++i) {
        set(i, s.get(i));
    }

    return *this;
}


std::ostream& operator << (std::ostream& os,
                           const RS_VectorSolutions& s) {
    for (int i=0; i<s.num; ++i) {
        os << "(" << s.get(i) << ")\n";
    }
    os << " tangent: " << (int)s.isTangent() << "\n";
    return os;
}


