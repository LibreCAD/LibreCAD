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


#ifndef RS_VECTOR_H
#define RS_VECTOR_H

#include <vector>
#include <iosfwd>
#include "rs.h"

/**
 * Represents a 3d vector (x/y/z)
 *
 * @author Andrew Mustun
 */
class RS_Vector {
public:
	RS_Vector()=default;
	RS_Vector(double vx, double vy, double vz=0.0);
    explicit RS_Vector(double angle);
    //RS_Vector(double v[]);
    explicit RS_Vector(bool valid);
	~RS_Vector()=default;

	//!
	//! \brief operator bool explicit and implicit conversion to bool
	//!
	explicit operator bool() const;

	void set(double angle); // set to unit vector by the direction of angle
    void set(double vx, double vy, double vz=0.0);
    void setPolar(double radius, double angle);
	//! \{
	//! construct by cartesian, or polar coordinates
	static RS_Vector polar(double rho, double theta);
	//! \}

    double distanceTo(const RS_Vector& v) const;
    double angle() const;
    double angleTo(const RS_Vector& v) const;
    double angleBetween(const RS_Vector& v1, const RS_Vector& v2) const;
    double magnitude() const;
    double squared() const; //return square of length
    double squaredTo(const RS_Vector& v1) const; //return square of length
    RS_Vector lerp(const RS_Vector& v, double t) const;

    bool isInWindow(const RS_Vector& firstCorner, const RS_Vector& secondCorner) const;
    bool isInWindowOrdered(const RS_Vector& vLow, const RS_Vector& vHigh) const;

    RS_Vector toInteger();

    RS_Vector move(const RS_Vector& offset);
	RS_Vector rotate(double ang);
    RS_Vector rotate(const RS_Vector& angleVector);
	RS_Vector rotate(const RS_Vector& center, double ang);
    RS_Vector rotate(const RS_Vector& center, const RS_Vector& angleVector);
	RS_Vector scale(double factor);
    RS_Vector scale(const RS_Vector& factor);
	RS_Vector scale(const RS_Vector& factor) const;
	RS_Vector scale(const RS_Vector& center, const RS_Vector& factor);
    RS_Vector mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
	double dotP(const RS_Vector& v1) const;

    RS_Vector operator + (const RS_Vector& v) const;
	RS_Vector operator + (double d) const;
	RS_Vector operator - (const RS_Vector& v) const;
	RS_Vector operator - (double d) const;
	RS_Vector operator * (const RS_Vector& v) const;
	RS_Vector operator / (const RS_Vector& v) const;
	RS_Vector operator * (double s) const;
	RS_Vector operator / (double s) const;
    RS_Vector operator - () const;

	RS_Vector operator += (const RS_Vector& v);
	RS_Vector operator -= (const RS_Vector& v);
	RS_Vector operator *= (const RS_Vector& v);
	RS_Vector operator /= (const RS_Vector& v);
	RS_Vector operator *= (double s);
	RS_Vector operator /= (double s);

    bool operator == (const RS_Vector& v) const;
    bool operator != (const RS_Vector& v) const {
        return !operator==(v);
    }
	//!
	//! \brief operator == comparison of validity with bool
	//! \param valid boolean parameter
	//! \return true is the parameter valid is the same as validity
	//!
	bool operator == (bool valid) const;
	bool operator != (bool valid) const;

    static RS_Vector minimum(const RS_Vector& v1, const RS_Vector& v2);
    static RS_Vector maximum(const RS_Vector& v1, const RS_Vector& v2);
//    crossP only defined for 3D
    static RS_Vector crossP(const RS_Vector& v1, const RS_Vector& v2);
    static double dotP(const RS_Vector& v1, const RS_Vector& v2);
    static double posInLine(const RS_Vector& start,
                            const RS_Vector& end,
                            const RS_Vector& pos);

    /** switch x,y for all vectors */
    RS_Vector flipXY(void) const;

    friend std::ostream& operator << (std::ostream&, const RS_Vector& v);

#ifdef RS_TEST

    static bool test();
#endif

public:
	double x=0.;
	double y=0.;
	double z=0.;
	bool valid=false;
};


/**
 * Represents one to 4 vectors. Typically used to return multiple
 * solutions from a function.
 */
class RS_VectorSolutions {
public:
	typedef RS_Vector value_type;
	RS_VectorSolutions();
	RS_VectorSolutions(const std::vector<RS_Vector>& s);
	RS_VectorSolutions(std::initializer_list<RS_Vector> const& l);
	RS_VectorSolutions(int num);

	~RS_VectorSolutions()=default;

	void alloc(size_t num);
    void clear();
	/**
	 * @brief get range safe method of member access
	 * @param i member index
	 * @return indexed member, or invalid vector, if out of range
	 */
	RS_Vector get(size_t i) const;
	const RS_Vector& at(size_t i) const;
	const RS_Vector&  operator [] (const size_t i) const;
	RS_Vector&  operator [] (const size_t i);
	size_t getNumber() const;
	size_t size() const;
    void resize(size_t n);
    bool hasValid() const;
void set(size_t i, const RS_Vector& v);
    void push_back(const RS_Vector& v);
	void removeAt(const size_t i);
	RS_VectorSolutions& push_back(const RS_VectorSolutions& v);
    void setTangent(bool t);
    bool isTangent() const;
    RS_Vector getClosest(const RS_Vector& coord,
						 double* dist=nullptr, size_t* index=nullptr) const;
    double getClosestDistance(const RS_Vector& coord,
                              int counts = -1); //default to search all
	const std::vector<RS_Vector>& getVector() const;
	std::vector<RS_Vector>::const_iterator begin() const;
	std::vector<RS_Vector>::const_iterator end() const;
	std::vector<RS_Vector>::iterator begin();
	std::vector<RS_Vector>::iterator end();
	void rotate(double ang);
    void rotate(const RS_Vector& angleVector);
	void rotate(const RS_Vector& center, double ang);
    void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    void move(const RS_Vector& vp);
    void scale(const RS_Vector& center, const RS_Vector& factor);
    void scale(const RS_Vector& factor);

    /** switch x,y for all vectors */
    RS_VectorSolutions flipXY(void) const;

    RS_VectorSolutions operator = (const RS_VectorSolutions& s);

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_VectorSolutions& s);

private:
	std::vector<RS_Vector> vector;
    bool tangent;
};

#endif

// EOF
