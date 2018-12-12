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

	RS_Entity* clone() const override;

    /**	@return RS2::EntityEllipse */
	RS2::EntityType rtti() const override{
        return RS2::EntityEllipse;
    }

    /**
     * @return Start point of the entity.
     */
	RS_Vector getStartpoint() const override;
	RS_VectorSolutions getFoci() const;

    /**
     * @return End point of the entity.
     */
	RS_Vector getEndpoint() const override;
	RS_Vector getEllipsePoint(const double& a) const; //find the point according to ellipse angle

	void moveStartpoint(const RS_Vector& pos) override;
	void moveEndpoint(const RS_Vector& pos) override;
	double getLength() const override;

    /**
    //Ellipse must have ratio<1, and not reversed
    *@ x1, ellipse angle
    *@ x2, ellipse angle
    //@return the arc length between ellipse angle x1, x2
    **/
    double getEllipseLength(double a1, double a2) const;
	double getEllipseLength(double a2) const;
	RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;//find the tangential points seeing from given point
	RS_Vector getTangentDirection(const RS_Vector& point)const override;
	RS2::Ending getTrimPoint(const RS_Vector& trimCoord,
									 const RS_Vector& trimPoint) override;

	RS_Vector prepareTrim(const RS_Vector& trimCoord,
								  const RS_VectorSolutions& trimSol) override;

    double getEllipseAngle (const RS_Vector& pos) const;

    /** @return Copy of data that defines the ellipse. **/
	const RS_EllipseData& getData() const;

	RS_VectorSolutions getRefPoints() const override;

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
	RS_Vector getCenter() const override;
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
	double getAngleLength() const;

    /** @return The major radius of this ellipse. Same as getRadius() */
	double getMajorRadius() const;

    /** @return the point by major minor radius directions */
	RS_Vector getMajorPoint() const;
	RS_Vector getMinorPoint() const;

    /** @return The minor radius of this ellipse */
	double getMinorRadius() const;
	//! \brief isEllipticArc the ellipse an Arc, if angle1/angle2 are not both 0
	bool isEllipticArc() const;
	bool isEdge() const override{
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
	RS_Vector getMiddlePoint(void)const override;
	RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr) const override;
	RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
			bool onEntity = true, double* dist = nullptr, RS_Entity** entity=nullptr) const override;
	RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = nullptr)const override;
	RS_Vector getNearestMiddle(const RS_Vector& coord,
									   double* dist = nullptr,
                                       int middlePoints = 1
									   )const override;
	RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const override;
	RS_Vector getNearestOrthTan(const RS_Vector& coord,
                                    const RS_Line& normal,
									 bool onEntity = false) const override;
    bool switchMajorMinor(void); //switch major minor axes to keep major the longer ellipse radius
	void correctAngles();//make sure angleLength() is not more than 2*M_PI
	bool isPointOnEntity(const RS_Vector& coord,
								 double tolerance=RS_TOLERANCE) const override;

	void move(const RS_Vector& offset) override;
	void rotate(const double& angle);
	void rotate(const RS_Vector& angleVector);
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angle) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
	void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    /** whether the entity's bounding box intersects with visible portion of graphic view
    */
	bool isVisibleInWindow(RS_GraphicView* view) const override;
	//! \{ \brief find visible segments of entity and draw only those visible portion
	void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;
	void drawVisible(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
	//! \}

    friend std::ostream& operator << (std::ostream& os, const RS_Ellipse& a);

	//void calculateEndpoints() override;
	void calculateBorders() override;

    //direction of tangent at endpoints
	double getDirection1() const override;
	double getDirection2() const override;

	/** \brief return the equation of the entity
	a quadratic contains coefficients for quadratic:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

    for linear:
    m0 x + m1 y + m2 =0
    **/
	LC_Quadratic getQuadratic() const override;
	/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 * \oint x dy = Cx y + \frac{1}{4}((a^{2}+b^{2})sin(2a)cos^{2}(t)-ab(2sin^{2}(a)sin(2t)-2t-sin(2t)))
 */
	double areaLineIntegral() const override;

protected:
    RS_EllipseData data;
};

#endif
//EOF
