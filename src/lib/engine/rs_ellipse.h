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


#ifndef RS_ELLIPSE_H
#define RS_ELLIPSE_H

#include "rs_atomicentity.h"

/**
 * Holds the data that defines an ellipse.
 */
class RS_EllipseData {
public:
    RS_EllipseData(const RS_Vector& center,
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

    friend class RS_Ellipse;

    friend std::ostream& operator << (std::ostream& os, const RS_EllipseData& ed) {
        os << "(" << ed.center <<
           "/" << ed.majorP <<
           " " << ed.ratio <<
           " " << ed.angle1 <<
           "," << ed.angle2 <<
           ")";
        return os;
    }

private:
    //! Ellipse center
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
 * Class for an ellipse entity. All angles are in Rad.
 *
 * @author Andrew Mustun
 */
class RS_Ellipse : public RS_AtomicEntity {
public:
    RS_Ellipse(RS_EntityContainer* parent,
               const RS_EllipseData& d);
    virtual ~RS_Ellipse() {}

    virtual RS_Entity* clone() {
        RS_Ellipse* e = new RS_Ellipse(*this);
        e->initId();
        return e;
    }

    /**	@return RS2::EntityEllipse */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityEllipse;
    }


    /**
     * @return Start point of the entity.
     */
    virtual RS_Vector getStartpoint() const {
        RS_Vector p;
        p.set(data.center.x + cos(data.angle1) * getMajorRadius(),
              data.center.y + sin(data.angle1) * getMinorRadius());
        p.rotate(data.center, getAngle());
        return p;
    }
    /**
     * @return End point of the entity.
     */
    virtual RS_Vector getEndpoint() const {
        RS_Vector p;
        p.set(data.center.x + cos(data.angle2) * getMajorRadius(),
              data.center.y + sin(data.angle2) * getMinorRadius());
        p.rotate(data.center, getAngle());
        return p;
    }

    virtual void moveStartpoint(const RS_Vector& pos);
    virtual void moveEndpoint(const RS_Vector& pos);

    virtual RS2::Ending getTrimPoint(const RS_Vector& trimCoord,
                                     const RS_Vector& trimPoint);

    virtual RS_Vector prepareTrim(const RS_Vector& trimCoord,
                                  const RS_VectorSolutions& trimSol);

    double getEllipseAngle(const RS_Vector& pos);

    /** @return Copy of data that defines the ellipse. **/
    RS_EllipseData getData() {
        return data;
    }

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

    /** @return The rotation angle of this ellipse */
    double getAngle() const {
        return data.majorP.angle();
    }

    /** @return The start angle of this arc */
    double getAngle1() {
        return data.angle1;
    }
    /** Sets new start angle. */
    void setAngle1(double a1) {
        data.angle1 = a1;
    }
    /** @return The end angle of this arc */
    double getAngle2() {
        return data.angle2;
    }
    /** Sets new end angle. */
    void setAngle2(double a2) {
        data.angle2 = a2;
    }


    /** @return The center point (x) of this arc */
    RS_Vector getCenter() {
        return data.center;
    }
    /** Sets new center. */
    void setCenter(const RS_Vector& c) {
        data.center = c;
    }

    /** @return The endpoint of the major axis (relative to center). */
    RS_Vector getMajorP() {
        return data.majorP;
    }
    /** Sets new major point (relative to center). */
    void setMajorP(const RS_Vector& p) {
        data.majorP = p;
    }

    /** @return The ratio of minor to major axis */
    double getRatio() {
        return data.ratio;
    }
    /** Sets new ratio. */
    void setRatio(double r) {
        data.ratio = r;
    }


    /**
     * @return Angle length in rad.
     */
    virtual double getAngleLength() const {
        if (isReversed()) {
            return data.angle1-data.angle2;
        } else {
            return data.angle2-data.angle1;
        }
    }

    /** @return The major radius of this ellipse. Same as getRadius() */
    double getMajorRadius() const {
        return data.majorP.magnitude();
    }

    /** @return The minor radius of this ellipse */
    double getMinorRadius() const {
        return data.majorP.magnitude()*data.ratio;
    }

    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                         double* dist = NULL);
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
            bool onEntity = true, double* dist = NULL, RS_Entity** entity=NULL);
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
                                       double* dist = NULL);
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
                                       double* dist = NULL);
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
                                     double* dist = NULL);
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity=NULL,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                      double solidDist = RS_MAXDOUBLE);
    bool switchMajorMinor(void); //switch major minor axes to keep major the longer ellipse radius
    virtual bool isPointOnEntity(const RS_Vector& coord,
                                 double tolerance=RS_TOLERANCE);

    virtual void move(RS_Vector offset);
    virtual void rotate(double angle);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double patternOffset=0.0);

    friend std::ostream& operator << (std::ostream& os, const RS_Ellipse& a);

    //virtual void calculateEndpoints();
    virtual void calculateBorders();

protected:
    RS_EllipseData data;
};

#endif
