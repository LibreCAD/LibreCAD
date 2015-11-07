/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011-2015 Dongxu Li (dongxuli2011@gmail.com)
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


#ifndef RS_ELLIPSE_H
#define RS_ELLIPSE_H

#include "rs_atomicentity.h"

class LC_Quadratic;

/**
 * Holds the data that defines an ellipse.
 * angle1=angle2=0.0 is reserved for whole ellipses
 * add 2*M_PI to angle1 or angle2 to make whole range ellipse arcs
 */
struct RS_EllipseData {
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

std::ostream& operator << (std::ostream& os, const RS_EllipseData& ed);

/**
 * Class for an ellipse entity. All angles are in Rad.
 *
 * @author Andrew Mustun
 */
class RS_Ellipse : public RS_AtomicEntity {
public:
	RS_Ellipse()=default;
	RS_Ellipse(RS_EntityContainer* parent, const RS_EllipseData& d);

	virtual RS_Entity* clone() const;

    /**	@return RS2::EntityEllipse */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityEllipse;
    }

    /**
     * @return Start point of the entity.
     */
    virtual RS_Vector getStartpoint() const;
    virtual RS_VectorSolutions getFoci() const;

    /**
     * @return End point of the entity.
     */
    virtual RS_Vector getEndpoint() const;
    virtual RS_Vector getEllipsePoint(const double& a) const; //find the point according to ellipse angle

    virtual void moveStartpoint(const RS_Vector& pos);
    virtual void moveEndpoint(const RS_Vector& pos);
#ifdef  HAS_BOOST
    virtual double getLength() const;

    /**
    //Ellipse must have ratio<1, and not reversed
    *@ x1, ellipse angle
    *@ x2, ellipse angle
    //@return the arc length between ellipse angle x1, x2
    **/
    double getEllipseLength(double a1, double a2) const;
    double getEllipseLength(double a2) const;
#else
    virtual double getLength() const{
        return -1.;
   }
#endif
    virtual RS_VectorSolutions getTangentPoint(const RS_Vector& point) const;//find the tangential points seeing from given point
    virtual RS_Vector getTangentDirection(const RS_Vector& point)const;
    virtual RS2::Ending getTrimPoint(const RS_Vector& trimCoord,
                                     const RS_Vector& trimPoint);

    virtual RS_Vector prepareTrim(const RS_Vector& trimCoord,
                                  const RS_VectorSolutions& trimSol);

    double getEllipseAngle (const RS_Vector& pos) const;

    /** @return Copy of data that defines the ellipse. **/
	const RS_EllipseData& getData() const;

	virtual RS_VectorSolutions getRefPoints() const;

    /**
     * @retval true if the arc is reversed (clockwise),
     * @retval false otherwise
     */
	bool isReversed() const;
    /** sets the reversed status. */
	void setReversed(bool r);

    /** @return The rotation angle of this ellipse */
	double getAngle() const;

    /** @return The start angle of this arc */
	double getAngle1() const;
    /** Sets new start angle. */
	void setAngle1(double a1);
    /** @return The end angle of this arc */
	double getAngle2() const;
    /** Sets new end angle. */
	void setAngle2(double a2);


    /** @return The center point (x) of this arc */
	virtual RS_Vector getCenter() const;
    /** Sets new center. */
	void setCenter(const RS_Vector& c);

    /** @return The endpoint of the major axis (relative to center). */
	const RS_Vector& getMajorP() const;
    /** Sets new major point (relative to center). */
	void setMajorP(const RS_Vector& p);

    /** @return The ratio of minor to major axis */
	double getRatio() const;
    /** Sets new ratio. */
	void setRatio(double r);

    /**
     * @return Angle length in rad.
     */
    virtual double getAngleLength() const;

    /** @return The major radius of this ellipse. Same as getRadius() */
	double getMajorRadius() const;

    /** @return the point by major minor radius directions */
	RS_Vector getMajorPoint() const;
	RS_Vector getMinorPoint() const;

    /** @return The minor radius of this ellipse */
	double getMinorRadius() const;
	//! \brief isEllipticArc the ellipse an Arc, if angle1/angle2 are not both 0
	virtual bool isEllipticArc() const;
    virtual bool isEdge() const {
        return true;
    }
    bool createFrom4P(const RS_VectorSolutions& sol);
    bool createFromCenter3Points(const RS_VectorSolutions& sol);
	//! \{ \brief from quadratic form
	/** : dn[0] x^2 + dn[1] xy + dn[2] y^2 =1 */
	bool createFromQuadratic(const std::vector<double>& dn);
	/** : generic quadratic: A x^2 + C xy + B y^2 + D x + E y + F =0 */
	bool createFromQuadratic(const LC_Quadratic& q);
	//! \}
	bool createInscribeQuadrilateral(const std::vector<RS_Line*>& lines);
    virtual RS_Vector getMiddlePoint(void)const;
    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr) const;
    virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
			bool onEntity = true, double* dist = nullptr, RS_Entity** entity=nullptr) const;
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = nullptr)const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
									   double* dist = nullptr,
                                       int middlePoints = 1
                                       )const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const;
    virtual RS_Vector getNearestOrthTan(const RS_Vector& coord,
                                    const RS_Line& normal,
									 bool onEntity = false) const;
    bool switchMajorMinor(void); //switch major minor axes to keep major the longer ellipse radius
    virtual void correctAngles();//make sure angleLength() is not more than 2*M_PI
    virtual bool isPointOnEntity(const RS_Vector& coord,
                                 double tolerance=RS_TOLERANCE) const;

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const double& angle);
    virtual void rotate(const RS_Vector& angleVector);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angle);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    /** whether the entity's bounding box intersects with visible portion of graphic view
    */
    bool isVisibleInWindow(RS_GraphicView* view) const;
	//! \{ \brief find visible segments of entity and draw only those visible portion
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
    virtual void drawVisible(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
	//! \}

    friend std::ostream& operator << (std::ostream& os, const RS_Ellipse& a);

    //virtual void calculateEndpoints();
    virtual void calculateBorders();

    //direction of tangent at endpoints
    virtual double getDirection1() const;
    virtual double getDirection2() const;

	/** \brief return the equation of the entity
	a quadratic contains coefficients for quadratic:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

    for linear:
    m0 x + m1 y + m2 =0
    **/
    virtual LC_Quadratic getQuadratic() const;
	/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 * \oint x dy = Cx y + \frac{1}{4}((a^{2}+b^{2})sin(2a)cos^{2}(t)-ab(2sin^{2}(a)sin(2t)-2t-sin(2t)))
 */
	virtual double areaLineIntegral() const;

protected:
    RS_EllipseData data;
};

#endif
//EOF
