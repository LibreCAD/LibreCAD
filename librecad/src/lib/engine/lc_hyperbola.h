/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

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


#ifndef LC_HYPERBOLA_H
#define LC_HYPERBOLA_H

#include "rs_atomicentity.h"

class RS_Circle;
class LC_Quadratic;

/**
 * Holds the data that defines one branch of a hyperbola.
 * majorP is the vector from center to the vertex
 * ratio is the ratio between semi-major and semi-minor axis

 */
struct LC_HyperbolaData {
	LC_HyperbolaData() = default;
	LC_HyperbolaData(const RS_Vector& center,
					 const RS_Vector& majorP,
					 double ratio,
					 double angle1, double angle2,
					 bool reversed);
	~LC_HyperbolaData() = default;
	/** create data based on foci and a point on hyperbola */
	LC_HyperbolaData(const RS_Vector& focus0,
					 const RS_Vector& focus1,
					 const RS_Vector& point);

	//! Hyperbola center
	RS_Vector center;
	//! Endpoint of major axis relative to center.
	RS_Vector majorP;
	//! Ratio of minor axis to major axis.
	double ratio;
	//! Start angle
	double angle1;
	//! End angle
	double angle2;
	//! Reversed (cw) flag
	bool reversed;
};

std::ostream& operator << (std::ostream& os, const LC_HyperbolaData& ed);


/**
 * Class for an hyperbola entity.
 *
 * @author Dongxu Li
 */
class LC_Hyperbola : public RS_AtomicEntity {
public:
	LC_Hyperbola() = default;
    LC_Hyperbola(RS_EntityContainer* parent,
               const LC_HyperbolaData& d);

    /** create data based on foci and a point on hyperbola */
    LC_Hyperbola(const RS_Vector& focus0,
                     const RS_Vector& focus1,
                     const RS_Vector& point);

	RS_Entity* clone() const override;

    /**	@return RS2::EntityHyperbola */
	RS2::EntityType rtti() const  override{
        return RS2::EntityHyperbola;
    }
	bool isValid() const{
        return m_bValid;
    }



//    double getLength() const;

//    /**
//    //Hyperbola must have ratio<1, and not reversed
//    *@ x1, hyperbola angle
//    *@ x2, hyperbola angle
//    //@return the arc length between hyperbola angle x1, x2
//    **/
//    double getHyperbolaLength(double a1, double a2) const;
//    double getHyperbolaLength(double a2) const;


    /** @return Copy of data that defines the hyperbola. **/
    LC_HyperbolaData getData() const {
        return data;
    }
	RS_VectorSolutions getFoci() const;
	RS_VectorSolutions getRefPoints() const override;

    /**
     * @retval true if the arc is reversed (clockwise),
     * @retval false otherwise
     */
	bool isReversed() const{
        return data.reversed;
    }
    /** sets the reversed status. */
	void setReversed(bool r){
        data.reversed = r;
    }

    /** @return The rotation angle of this hyperbola */
	double getAngle() const {
        return data.majorP.angle();
    }

    /** @return The start angle of this arc */
    double getAngle1() const {
        return data.angle1;
    }
    /** Sets new start angle. */
    void setAngle1(double a1) {
        data.angle1 = a1;
    }
    /** @return The end angle of this arc */
    double getAngle2() const {
        return data.angle2;
    }
    /** Sets new end angle. */
    void setAngle2(double a2) {
        data.angle2 = a2;
    }


    /** @return The center point (x) of this arc */
	RS_Vector getCenter() const override{
        return data.center;
    }
    /** Sets new center. */
    void setCenter(const RS_Vector& c) {
        data.center = c;
    }

    /** @return The endpoint of the major axis (relative to center). */
    RS_Vector getMajorP() const {
        return data.majorP;
    }
    /** Sets new major point (relative to center). */
    void setMajorP(const RS_Vector& p) {
        data.majorP = p;
    }

    /** @return The ratio of minor to major axis */
    double getRatio() const {
        return data.ratio;
    }
    /** Sets new ratio. */
    void setRatio(double r) {
        data.ratio = r;
    }


    /** @return The major radius of this hyperbola. Same as getRadius() */
    double getMajorRadius() const {
        return data.majorP.magnitude();
    }

    /** @return The minor radius of this hyperbola */
    double getMinorRadius() const {
        return data.majorP.magnitude()*data.ratio;
    }

	void calculateBorders() override{}

	RS_Vector getMiddlePoint(void)const override{return RS_Vector(false);}
	RS_Vector getNearestEndpoint(const RS_Vector& /*coord*/,
										 double*/* dist = NULL*/) const override
    {return RS_Vector(false);}
	RS_Vector getNearestPointOnEntity(const RS_Vector& /*coord*/,
			bool /*onEntity = true*/, double*/* dist = NULL*/, RS_Entity**/* entity=NULL*/) const override
    {return RS_Vector(false);}
	RS_Vector getNearestCenter(const RS_Vector& /*coord*/,
									   double*/* dist = NULL*/) const override
   {return RS_Vector(false);}
	RS_Vector getNearestMiddle(const RS_Vector& /*coord*/,
                                       double*/* dist = NULL*/,
                                       int/* middlePoints = 1*/
									   )const override
   {return RS_Vector(false);}
	RS_Vector getNearestDist(double /*distance*/,
                                     const RS_Vector&/* coord*/,
									 double*/* dist = NULL*/) const override
    {return RS_Vector(false);}
	RS_Vector getNearestOrthTan(const RS_Vector& /*coord*/,
                                    const RS_Line& /*normal*/,
									 bool /*onEntity = false*/) const override
    {return RS_Vector(false);}
	double getDistanceToPoint(const RS_Vector& /*coord*/,
                                      RS_Entity** /*entity=NULL*/,
                                      RS2::ResolveLevel/* level=RS2::ResolveNone*/,
									  double /*solidDist = RS_MAXDOUBLE*/) const override
    {return RS_MAXDOUBLE;}
	bool isPointOnEntity(const RS_Vector& /*coord*/,
								 double /*tolerance=RS_TOLERANCE*/) const override;

	void move(const RS_Vector& /*offset*/) override{}
	void rotate(const double& /*angle*/) {}
	void rotate(const RS_Vector& /*angleVector*/){}
	void rotate(const RS_Vector& /*center*/, const double& /*angle*/) override{}
	void rotate(const RS_Vector& /*center*/, const RS_Vector& /*angle*/)override{}
	void scale(const RS_Vector& /*center*/, const RS_Vector& /*factor*/)override{}
	void mirror(const RS_Vector& /*axisPoint1*/, const RS_Vector& /*axisPoint2*/)override{}
	void moveRef(const RS_Vector& /*ref*/, const RS_Vector& /*offset*/)override{}

	void draw(RS_Painter* /*painter*/, RS_GraphicView* /*view*/, double& /*patternOffset*/)override{}

    friend std::ostream& operator << (std::ostream& os, const LC_Hyperbola& a);

	//void calculateEndpoints();
//    void calculateBorders();

    //direction of tangent at endpoints
	double getDirection1() const override{return 0.;}
	double getDirection2() const override{return 0.;}
    /** return the equation of the entity
    for quadratic,

    return a vector contains:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

    for linear:
    m0 x + m1 y + m2 =0
    **/
	LC_Quadratic getQuadratic() const override;

protected:
    LC_HyperbolaData data;
    bool m_bValid;

};



#endif
//EOF
