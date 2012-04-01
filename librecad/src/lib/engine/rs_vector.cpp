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


#include "rs_vector.h"

//#include <values.h>

#include "rs_debug.h"
#include "rs_math.h"
#include "rs_constructionline.h"

#ifdef EMU_C99
#include "emu_c99.h" /* remainder() */
#endif

/**
 * Constructor for a point with default coordinates.
 */
RS_Vector::RS_Vector() {
    //RS_DEBUG->print("RS_Vector::RS_Vector");

#ifdef  RS_VECTOR2D
    set(0.0, 0.0);
#else
    set(0.0, 0.0, 0.0);
#endif
}



/**
 * Constructor for a point with given coordinates.
 */
#ifdef  RS_VECTOR2D
RS_Vector::RS_Vector(double vx, double vy) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    set(vx, vy);
}
#else
RS_Vector::RS_Vector(double vx, double vy, double vz) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    set(vx, vy, vz);
}
#endif

/**
 * Constructor for a unit vector with given angle
 */
RS_Vector::RS_Vector(double angle) {
    //RS_DEBUG->print("RS_Vector::RS_Vector");
    x = cos(angle);
    y = sin(angle);
#ifndef RS_VECTOR2D
    z = 0.0;
#endif
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
#ifdef  RS_VECTOR2D
    set(0.0, 0.0);
#else
    set(0.0, 0.0, 0.0);
#endif
    this->valid = valid;
}


/**
 * Destructor.
 */
RS_Vector::~RS_Vector() {
    //RS_DEBUG->print("RS_Vector::~RS_Vector");
}


/**
 * Sets to a unit vector by the direction angle
 */
void RS_Vector::set(double angle) {
    x = cos(angle);
    y = sin(angle);
#ifndef RS_VECTOR2D
    z = 0.;
#endif
    valid = true;
}


/**
 * Sets a new position for the vector.
 */
#ifdef  RS_VECTOR2D
void RS_Vector::set(double vx, double vy) {
#else
void RS_Vector::set(double vx, double vy, double vz) {
#endif
    x = vx;
    y = vy;
#ifndef RS_VECTOR2D
    z = vz;
#endif
    valid = true;
}


/**
 * Sets a new position for the vector in polar coordinates.
 */
void RS_Vector::setPolar(double radius, double angle) {
    x = radius * cos(angle);
    y = radius * sin(angle);
#ifndef RS_VECTOR2D
    z = 0.0;
#endif
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
 * @return The angle from between two vectors using the current vector as the center
 * return 0, if the angle is not well defined
 */
double RS_Vector::angleBetween(const RS_Vector& v1, const RS_Vector& v2) const {
    if (!valid || !v1.valid || !v2.valid) {
        return 0.0;
    }
    else {
        RS_Vector vStart(v1- (*this));
        RS_Vector vEnd(v2- (*this));
        //        if( vStart.magnitude() < RS_TOLERANCE
        //                || vEnd.magnitude() < RS_TOLERANCE) return 0.0;
        return RS_Math::correctAngle( atan2( vStart.x*vEnd.y-vStart.y*vEnd.x, vStart.x*vEnd.x+vStart.y*vEnd.y));

        //         std::cout<<"old algorithm:: "<<RS_Math::correctAngle(vEnd.angle() - vStart.angle())<<std::endl;
//        return RS_Math::correctAngle(vEnd.angle() - vStart.angle());
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
#ifdef  RS_VECTOR2D
        ret = sqrt(x*x + y*y);
#else
        ret = sqrt(x*x + y*y + z*z);
#endif
    }

    return ret;
}

/**
  * @return square of vector length
  */
double RS_Vector::squared() const {
    // Note that the z coordinate is also needed for 2d
    //   (due to definition of crossP())
    if (valid) {
#ifdef  RS_VECTOR2D
        return x*x + y*y;
#else
        return x*x + y*y + z*z;
#endif
    }
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
                           const RS_Vector& secondCorner) const {
    RS_Vector vLow( std::min(firstCorner.x, secondCorner.x), std::min(firstCorner.y, secondCorner.y));
    RS_Vector vHigh( std::max(firstCorner.x, secondCorner.x), std::max(firstCorner.y, secondCorner.y));

    return isInWindowOrdered(vLow,vHigh);
}

/**
 * @return true is this vector is within the given range
 * of ordered vectors
 */
bool RS_Vector::isInWindowOrdered(const RS_Vector& vLow,
                           const RS_Vector& vHigh) const {

    return (x>=vLow.x && x<=vHigh.x && y>=vLow.y && y<=vHigh.y);
}

/**
 * move to the closest integer point
 */
RS_Vector RS_Vector::toInteger() {
    x= x - remainder(x,1.);
    y= y - remainder(y,1.);
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
RS_Vector RS_Vector::rotate(const double& ang) {
//    RS_DEBUG->print("RS_Vector::rotate: angle: %f", ang);

//    double r = magnitude();

//    RS_DEBUG->print("RS_Vector::rotate: r: %f", r);

//    double a = angle() + ang;

//    RS_DEBUG->print("RS_Vector::rotate: a: %f", a);

    //    x = cos(a) * r;
    //    y = sin(a) * r;

    //    RS_DEBUG->print("RS_Vector::rotate: x/y: %f/%f", x, y);
    // rotate by direction vector
    rotate(RS_Vector(ang));
    return *this;
}

/**
 * Rotates this vector around 0/0 by the given vector
 * if the vector is a unit, then, it's the same as rotating around
 * 0/0 by the angle of the vector
 */
RS_Vector RS_Vector::rotate(const RS_Vector& angleVector) {
//        RS_DEBUG->print("RS_Vector::rotate: rotating Vecotr: %g/%g", x,y);
//        RS_DEBUG->print("RS_Vector::rotate: rotating by Vecotr: %g/%g", angleVector.x,angleVector.y);
        double x0 = x * angleVector.x - y * angleVector.y;
        y = x * angleVector.y + y * angleVector.x;
        x = x0;

//        RS_DEBUG->print("RS_Vector::rotate: rotated x/y: %f/%f", x, y);

    return *this;
}


/**
 * Rotates this vector around the given center by the given angle.
 */
RS_Vector RS_Vector::rotate(const RS_Vector& center, const double& ang) {
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
RS_Vector RS_Vector::scale(const double& factor) {
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
    /*
    RS_ConstructionLine axis(NULL,
        RS_ConstructionLineData(axisPoint1, axisPoint2));

    RS_Vector xp = axis.getNearestPointOnEntity(*this);
    xp = xp - (*this);
    (*this) += (xp*2);
    */
    RS_Vector direction(axisPoint2-axisPoint1);
    double a= direction.squared();
    RS_Vector ret(false);
    if(a<RS_TOLERANCE*RS_TOLERANCE) {
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
#ifdef  RS_VECTOR2D
        os << v.x << "/" << v.y ;
#else
        os << v.x << "/" << v.y << "/" << v.z;
#endif
    } else {
        os << "invalid vector";
    }
    return os;
}



/**
 * binary + operator.
 */
RS_Vector RS_Vector::operator + (const RS_Vector& v) const {
#ifdef  RS_VECTOR2D
    return RS_Vector(x + v.x, y + v.y);
#else
    return RS_Vector(x + v.x, y + v.y, z + v.z);
#endif
}



/**
 * binary - operator.
 */
RS_Vector RS_Vector::operator - (const RS_Vector& v) const {
#ifdef  RS_VECTOR2D
    return RS_Vector(x - v.x, y - v.y);
#else
    return RS_Vector(x - v.x, y - v.y, z - v.z);
#endif
}


/**
 * binary * operator.
 */
RS_Vector RS_Vector::operator * (const double& s) const {
#ifdef  RS_VECTOR2D
    return RS_Vector(x * s, y * s);
#else
    return RS_Vector(x * s, y * s, z * s);
#endif
}



/**
 * binary / operator.
 */
RS_Vector RS_Vector::operator / (const double& s) const {
#ifdef  RS_VECTOR2D
    return RS_Vector(x / s, y / s);
#else
    return RS_Vector(x / s, y / s, z / s);
#endif
}



/**
 * unary - operator.
 */
RS_Vector RS_Vector::operator - () const {
#ifdef  RS_VECTOR2D
    return RS_Vector(-x, -y);
#else
    return RS_Vector(-x, -y, -z);
#endif
}



/**
 * Scalarproduct (dot product).
 */
double RS_Vector::dotP(const RS_Vector& v1, const RS_Vector& v2) {
#ifdef  RS_VECTOR2D
    return v1.x * v2.x + v1.y * v2.y;
#else
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
#endif
}



/**
 * += operator. Assert: both vectors must be valid.
 */
void RS_Vector::operator += (const RS_Vector& v) {
    x += v.x;
    y += v.y;
#ifndef RS_VECTOR2D
    z += v.z;
#endif
}


/**
 * -= operator
 */
void RS_Vector::operator -= (const RS_Vector& v) {
    x -= v.x;
    y -= v.y;
#ifndef RS_VECTOR2D
    z -= v.z;
#endif
}



/**
 * *= operator
 */
void RS_Vector::operator *= (const double& s) {
    x *= s;
    y *= s;
#ifndef RS_VECTOR2D
    z *= s;
#endif
}
/**
 * /= operator
 */
void RS_Vector::operator /= (const double& s) {
    if(fabs(s)>RS_TOLERANCE) {
    x /= s;
    y /= s;
#ifndef RS_VECTOR2D
    z /= s;
#endif
    }
}



/**
 * == operator
 */
bool RS_Vector::operator == (const RS_Vector& v) const {
#ifdef  RS_VECTOR2D
    return (x==v.x && y==v.y && valid==v.valid);
#else
    return (x==v.x && y==v.y && z==v.z && valid==v.valid);
#endif
}



/**
 * @return A vector with the minimum components from the vectors v1 and v2.
 * These might be mixed components from both vectors.
 */
RS_Vector RS_Vector::minimum (const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector (std::min(v1.x, v2.x),
                      std::min(v1.y, v2.y)
#ifndef RS_VECTOR2D
                      , std::min(v1.z, v2.z)
#endif
                      );
}



/**
 * @return A vector with the maximum values from the vectors v1 and v2
 */
RS_Vector RS_Vector::maximum (const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector (std::max(v1.x, v2.x),
                      std::max(v1.y, v2.y)
#ifndef RS_VECTOR2D
                      , std::max(v1.z, v2.z)
#endif
                      );
}



/**
 * @return Cross product of two vectors.
 *  we don't need cross product for 2D vectors
 */
#ifndef RS_VECTOR2D
RS_Vector RS_Vector::crossP(const RS_Vector& v1, const RS_Vector& v2) {
    return RS_Vector(v1.y*v2.z - v1.z*v2.y,
                     v1.z*v2.x - v1.x*v2.z,
                     v1.x*v2.y - v1.y*v2.x);
}
#endif


/**
 * Constructor for no solution.
 */
RS_VectorSolutions::RS_VectorSolutions() {
    vector.clear();
    tangent=false;
}



/**
 * Copy constructor
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_VectorSolutions& s)
    {
    setTangent(s.isTangent());
    vector=s.vector;
}



/**
 * Constructor for num solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(int num) {
#if QT_VERSION >= 0x040700
    vector.reserve(num);
#else
        for(int i=vector.size();i<num;i++){
            vector.push_back(RS_Vector(false));
        }
#endif
}



/**
 * Constructor for one solution.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1) {
    vector.push_back(v1);
    tangent = false;
}



/**
 * Constructor for two solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2) {
    vector.push_back(v1);
    vector.push_back(v2);
    tangent = false;
}



/**
 * Constructor for three solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2,
                                       const RS_Vector& v3) {
    vector.push_back(v1);
    vector.push_back(v2);
    vector.push_back(v3);
    tangent = false;
}


/**
 * Constructor for four solutions.
 */
RS_VectorSolutions::RS_VectorSolutions(const RS_Vector& v1,
                                       const RS_Vector& v2,
                                       const RS_Vector& v3,
                                       const RS_Vector& v4) {
    vector.push_back(v1);
    vector.push_back(v2);
    vector.push_back(v3);
    vector.push_back(v4);
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
    vector.push_back(v1);
    vector.push_back(v2);
    vector.push_back(v3);
    vector.push_back(v4);
    vector.push_back(v5);
    tangent = false;
}



/**
 * Destructor.
 */
RS_VectorSolutions::~RS_VectorSolutions() {
    vector.clear();
}


/**
 * Allocates 'num' vectors.
 */
void RS_VectorSolutions::alloc(int num) {
#if QT_VERSION >= 0x040700
    vector.reserve(num);
#else
    if(vector.size() <num) {
        for(int i=vector.size();i<num;i++){
            vector.push_back(RS_Vector(false));
        }
    }
#endif
    if(vector.size()>num) {
        vector.erase(vector.begin()+num, vector.end());
    }
   // tangent = false;
}

/**
 * Deletes vector array and resets everything.
 */
void RS_VectorSolutions::clean() {
    vector.clear();
    tangent = false;
}



/**
 * @return vector solution number i or an invalid vector if there
 * are less solutions.
 */
RS_Vector RS_VectorSolutions::get(int i) const {
    if (i<vector.size()) {
        return vector[i];
    } else {
        return RS_Vector(false);
    }
}



/**
 * @return Number of solutions available.
 */
int RS_VectorSolutions::getNumber() const {
    return vector.size();
}



/**
 * @retval true There's at least one valid solution.
 * @retval false There's no valid solution.
 */
bool RS_VectorSolutions::hasValid() const {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            return true;
        }
    }

    return false;
}

QList<RS_Vector> RS_VectorSolutions::getList() const {
    return vector;
}

void RS_VectorSolutions::push_back(const RS_Vector& v) {
        vector.push_back(v);
}
RS_VectorSolutions RS_VectorSolutions::appendTo(const RS_VectorSolutions& v) {
    vector += v.getList();
    return *this;
}

/**
 * Sets the solution i to the given vector.
 * If i is greater than the current number of solutions available,
 * nothing happens.
 */
void RS_VectorSolutions::set(int i, const RS_Vector& v) {
    if (i<vector.size()) {
        vector[i] = v;
    }else{
//            RS_DEBUG->print(RS_Debug::D_ERROR, "set member in vector in RS_VectorSolutions: out of range, %d to size of %d", i,vector.size());
        for(int j=vector.size();j<=i;j++)
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
void RS_VectorSolutions::rotate(const double& ang) {
    RS_Vector angleVector(ang);
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            vector[i].rotate(angleVector);
        }
    }
}

/**
 * Rotates all vectors around (0,0) by the given angleVector.
 */
void RS_VectorSolutions::rotate(const RS_Vector& angleVector) {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
                //To be a real rotation, the determinant of angleVector has to be 1 for a pure rotation
            vector[i].rotate(angleVector);
        }
    }
}

/**
 * Rotates all vectors around the given center by the given angle.
 */
void RS_VectorSolutions::rotate(const RS_Vector& center, const double& ang) {
    RS_Vector angleVector(ang);
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            vector[i].rotate(center,angleVector);
        }
    }
}
void RS_VectorSolutions::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            vector[i].rotate(center,angleVector);
        }
    }
}

/**
 * Move all vectors around the given center by the given vector.
 */
void RS_VectorSolutions::move(const RS_Vector& vp) {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) vector[i].move(vp);
    }
}


/**
 * Scales all vectors by the given factors with the given center.
 */
void RS_VectorSolutions::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            vector[i].scale(center, factor);
        }
    }
}
void RS_VectorSolutions::scale( const RS_Vector& factor) {
    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            vector[i].scale(factor);
        }
    }
}


/**
 * @return vector solution which is the closest to the given coordinate.
 * dist will contain the distance if it doesn't point to NULL (default).
 */
RS_Vector RS_VectorSolutions::getClosest(const RS_Vector& coord,
                                         double* dist, int* index) const {

    double curDist(0.);
    double minDist = RS_MAXDOUBLE;
    RS_Vector closestPoint(false);
    int pos(0);

    for (int i=0; i<vector.size(); i++) {
        if (vector[i].valid) {
            curDist = (coord - vector[i]).squared();

            if (curDist<minDist) {
                closestPoint = vector[i];
                minDist = curDist;
                pos=i;
            }
        }
    }
    if (dist!=NULL) {
        *dist = sqrt(minDist);
    }
    if (index!=NULL) {
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
    if (counts<i && counts>=0) i=counts;
    for(int j=0; j<i; j++) {
        if(vector[j].valid) {
            double d=(coord - vector[j]).squared();
            if(d<ret) ret=d;
        }
    }
    return sqrt(ret);
}


RS_VectorSolutions RS_VectorSolutions::operator = (const RS_VectorSolutions& s) {
    setTangent(s.isTangent());
    vector=s.vector;

    return *this;
}


std::ostream& operator << (std::ostream& os,
                           const RS_VectorSolutions& s) {
    for (int i=0; i<s.getNumber(); ++i) {
        os << "(" << s.get(i) << ")\n";
    }
    os << " tangent: " << (int)s.isTangent() << "\n";
    return os;
}


