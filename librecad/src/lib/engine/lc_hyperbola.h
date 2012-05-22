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

class LC_Quadratic;

/**
 * Holds the data that defines one branch of a hyperbola.
 * majorP is the vector from center to the vertex
 * ratio is the ratio between semi-major and semi-minor axis

 */
class LC_HyperbolaData {
public:
    LC_HyperbolaData(const RS_Vector& center,
                   const RS_Vector& majorP,
                   double ratio,
                   double angle1, double angle2,
                   bool reversed) {

        this->center = center;
        this->majorP = majorP;
        this->ratio = ratio;
        this->angle1 = angle1;
        this->angle2 = angle2;
        this->reversed = reversed;
    }

    friend class LC_Hyperbola;

    friend std::ostream& operator << (std::ostream& os, const LC_HyperbolaData& ed) {
        os << "(" << ed.center <<
           "/" << ed.majorP <<
           " " << ed.ratio <<
           " " << ed.angle1 <<
           "," << ed.angle2 <<
           ")";
        return os;
    }

private:
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




/**
 * Class for an hyperbola entity.
 *
 * @author Dongxu Li
 */
class LC_Hyperbola : public RS_AtomicEntity {
public:
    LC_Hyperbola(RS_EntityContainer* parent,
               const LC_HyperbolaData& d);
    virtual ~LC_Hyperbola() {}

    virtual RS_Entity* clone() {
        LC_Hyperbola* e = new LC_Hyperbola(*this);
        e->initId();
        return e;
    }

    /**	@return RS2::EntityHyperbola */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityHyperbola;
    }



//    virtual double getLength() const;

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
    RS_VectorSolutions getFoci() const ;
    virtual RS_VectorSolutions getRefPoints();

    /**
     * @retval true if the arc is reversed (clockwise),
     * @retval false otherwise
     */
    bool isReversed() const {
        return data.reversed;
    }
    /** sets the reversed status. */
    void setReversed(bool r) {
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
    virtual RS_Vector getCenter() const {
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

    virtual void calculateBorders(){}

    virtual RS_Vector getMiddlePoint(void)const {return RS_Vector(false);}
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL) const
    {return RS_Vector(false);}
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = NULL, RS_Entity** entity=NULL) const
    {return RS_Vector(false);}
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = NULL)
   {return RS_Vector(false);}
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL,
                                       int middlePoints = 1
                                       )const
   {return RS_Vector(false);}
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = NULL)
    {return RS_Vector(false);}
    virtual RS_Vector getNearestOrthTan(const RS_Vector& coord,
                                    const RS_Line& normal,
                                     bool onEntity = false)
    {return RS_Vector(false);}
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                      double solidDist = RS_MAXDOUBLE) const
    {return RS_MAXDOUBLE;}
    virtual bool isPointOnEntity(const RS_Vector& coord,
                                 double tolerance=RS_TOLERANCE) const;

    virtual void move(const RS_Vector& offset){}
    virtual void rotate(const double& angle){}
    virtual void rotate(const RS_Vector& angleVector){}
    virtual void rotate(const RS_Vector& center, const double& angle){}
    virtual void rotate(const RS_Vector& center, const RS_Vector& angle){}
    virtual void scale(const RS_Vector& center, const RS_Vector& factor){}
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2){}
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset){}

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset){}

    friend std::ostream& operator << (std::ostream& os, const LC_Hyperbola& a);

    //virtual void calculateEndpoints();
//    virtual void calculateBorders();

    //direction of tangent at endpoints
    virtual double getDirection1() const{return 0.;}
    virtual double getDirection2() const{return 0.;}
    /** return the equation of the entity
    for quadratic,

    return a vector contains:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

    for linear:
    m0 x + m1 y + m2 =0
    **/
    LC_Quadratic getQuadratic() const;

protected:
    LC_HyperbolaData data;

};



#endif
//EOF
