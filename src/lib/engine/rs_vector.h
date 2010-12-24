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


#ifndef RS_VECTOR_H
#define RS_VECTOR_H

#include <iostream>

#include "rs.h"

/**
 * Represents a 3d vector (x/y/z)
 *
 * @author Andrew Mustun
 */
class RS_Vector {
public:
    RS_Vector();
    RS_Vector(double vx, double vy, double vz=0.0);
    //RS_Vector(double v[]);
    explicit RS_Vector(bool valid);
    ~RS_Vector();

    void set(double vx, double vy, double vz=0.0);
    void setPolar(double radius, double angle);

    double distanceTo(const RS_Vector& v) const;
    double angle() const;
    double angleTo(const RS_Vector& v) const;
    double magnitude() const;
    RS_Vector lerp(const RS_Vector& v, double t) const;

	bool isInWindow(const RS_Vector& firstCorner, const RS_Vector& secondCorner);

    RS_Vector move(RS_Vector offset);
    RS_Vector rotate(double ang);
    RS_Vector rotate(RS_Vector center, double ang);
    RS_Vector scale(RS_Vector factor);
    RS_Vector scale(RS_Vector center, RS_Vector factor);
	RS_Vector mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);

    RS_Vector operator + (const RS_Vector& v) const;
    RS_Vector operator - (const RS_Vector& v) const;
    RS_Vector operator * (double s) const;
    RS_Vector operator / (double s) const;
    RS_Vector operator - () const;

    void operator += (const RS_Vector& v);
    void operator -= (const RS_Vector& v);
    void operator *= (double s);

    bool operator == (const RS_Vector& v) const;
    bool operator != (const RS_Vector& v) const {
        return !operator==(v);
    }

    static RS_Vector minimum(const RS_Vector& v1, const RS_Vector& v2);
    static RS_Vector maximum(const RS_Vector& v1, const RS_Vector& v2);
    static RS_Vector crossP(const RS_Vector& v1, const RS_Vector& v2);
    static double dotP(const RS_Vector& v1, const RS_Vector& v2);

    friend std::ostream& operator << (std::ostream&, const RS_Vector& v);

#ifdef RS_TEST

    static bool test();
#endif

public:
    double x;
    double y;
    double z;
    bool valid;
};


/**
 * Represents one to 4 vectors. Typically used to return multiple
 * solutions from a function.
 */
class RS_VectorSolutions {
public:
    RS_VectorSolutions();
    RS_VectorSolutions(const RS_VectorSolutions& s);
    RS_VectorSolutions(int num);
    RS_VectorSolutions(const RS_Vector& v1);
    RS_VectorSolutions(const RS_Vector& v1, const RS_Vector& v2);
    RS_VectorSolutions(const RS_Vector& v1, const RS_Vector& v2,
	                   const RS_Vector& v3);
    RS_VectorSolutions(const RS_Vector& v1, const RS_Vector& v2,
                       const RS_Vector& v3, const RS_Vector& v4);
    RS_VectorSolutions(const RS_Vector& v1, const RS_Vector& v2,
                       const RS_Vector& v3, const RS_Vector& v4,
					   const RS_Vector& v5);
    
	~RS_VectorSolutions();

	void alloc(int num);
    void clean();
    RS_Vector get(int i) const;
	int getNumber() const;
	bool hasValid() const;
	void set(int i, const RS_Vector& v);
	void setTangent(bool t);
    bool isTangent() const;
    RS_Vector getClosest(const RS_Vector& coord, 
			double* dist=NULL, int* index=NULL) const;
    void rotate(RS_Vector center, double ang);
    void scale(RS_Vector center, RS_Vector factor);

	RS_VectorSolutions operator = (const RS_VectorSolutions& s);

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_VectorSolutions& s);

private:
    RS_Vector* vector;
    int num;
    bool tangent;
};

#endif

// EOF
