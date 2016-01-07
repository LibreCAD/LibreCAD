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


#ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include <vector>
#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a line.
 */
struct RS_SplineData {
	/**
	 * Default constructor. Leaves the data object uninitialized.
	 */
	RS_SplineData() = default;

	RS_SplineData(int degree, bool closed);


	/** Degree of the spline (1, 2, 3) */
	size_t degree;
	/** Closed flag. */
	bool closed;
	/** Control points of the spline. */
	std::vector<RS_Vector> controlPoints;
};

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld);

/**
 * Class for a spline entity.
 *
 * @author Andrew Mustun
 */
class RS_Spline : public RS_EntityContainer {
public:
    RS_Spline(RS_EntityContainer* parent,
            const RS_SplineData& d);
	virtual ~RS_Spline() = default;

	virtual RS_Entity* clone() const;


    /**	@return RS2::EntitySpline */
    virtual RS2::EntityType rtti() const {
        return RS2::EntitySpline;
    }
    /** @return false */
    virtual bool isEdge() const {
        return false;
    }

	/** @return Copy of data that defines the spline. */
	const RS_SplineData& getData() const {
		return data;
	}

	/** Sets the splines degree (1-3). */
	void setDegree(size_t deg);

	/** @return Degree of this spline curve (1-3).*/
	size_t getDegree() const;

	/** @return 0. */
	int getNumberOfKnots() {
		return 0;
	}

	/** @return Number of control points. */
	size_t getNumberOfControlPoints() const;

	/**
		 * @retval true if the spline is closed.
		 * @retval false otherwise.
		 */
	bool isClosed() const;

	/**
		 * Sets the closed falg of this spline.
		 */
	void setClosed(bool c);

	virtual RS_VectorSolutions getRefPoints() const;
    virtual RS_Vector getNearestRef( const RS_Vector& coord, double* dist = nullptr) const;
    virtual RS_Vector getNearestSelectedRef( const RS_Vector& coord, double* dist = nullptr) const;

    /** @return Start point of the entity */
    virtual RS_Vector getStartpoint() const ;
    //    return data.startpoint;
    //}
    /** @return End point of the entity */
    virtual RS_Vector getEndpoint() const ;
    //    return data.endpoint;
    //}
    /** Sets the startpoint */
    //void setStartpoint(RS_Vector s) {
    //    data.startpoint = s;
    //    calculateBorders();
    //}
    /** Sets the endpoint */
    //void setEndpoint(RS_Vector e) {
    //    data.endpoint = e;
    //    calculateBorders();
    //}

        void update();

        //virtual void moveStartpoint(const RS_Vector& pos);
        //virtual void moveEndpoint(const RS_Vector& pos);
        //virtual RS2::Ending getTrimPoint(const RS_Vector& coord,
        //          const RS_Vector& trimPoint);
        //virtual void reverse();
    /** @return the center point of the line. */
    //RS_Vector getMiddlePoint() {
    //    return (data.startpoint + data.endpoint)/2.0;
    //}
        //virtual bool hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2);

    /**
     * @return The length of the line.
     */
    //virtual double getLength() const {
    //    return data.startpoint.distanceTo(data.endpoint);
    //}

    /**
     * @return The angle of the line (from start to endpoint).
     */
    //virtual double getAngle1() {
    //    return data.startpoint.angleTo(data.endpoint);
    //}

    /**
     * @return The angle of the line (from end to startpoint).
     */
    //virtual double getAngle2() {
    //    return data.endpoint.angleTo(data.startpoint);
    //}

    virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr)const;
    //virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
	//        bool onEntity=true, double* dist = nullptr, RS_Entity** entity=nullptr);
    virtual RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = nullptr)const;
    virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
									   double* dist = nullptr,
                                       int middlePoints = 1)const;
    virtual RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const;
        //virtual RS_Vector getNearestRef(const RS_Vector& coord,
		//                                 double* dist = nullptr);
        /*virtual double getDistanceToPoint(const RS_Vector& coord,
									  RS_Entity** entity=nullptr,
                                      RS2::ResolveLevel level=RS2::ResolveNone,
                                                                          double solidDist = RS_MAXDOUBLE);*/

        virtual void addControlPoint(const RS_Vector& v);
        virtual void removeLastControlPoint();

        virtual void move(const RS_Vector& offset);
        virtual void rotate(const RS_Vector& center, const double& angle);
        virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
        virtual void scale(const RS_Vector& center, const RS_Vector& factor);
		virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

        virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
		virtual void revertDirection();

        virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
		const std::vector<RS_Vector>& getControlPoints() const;

        friend std::ostream& operator << (std::ostream& os, const RS_Spline& l);

        virtual void calculateBorders();

private:
		static void rbasis(int c, double t, int npts, const std::vector<int>& x, const std::vector<double>& h, std::vector<double>& r);

		static void knot(int num, int order, std::vector<int>& knotVector);
		static void rbspline(size_t npts, size_t k, size_t p1,
							 const std::vector<double>& b, const std::vector<double>& h, std::vector<double>& p);

		static void knotu(int num, int order, std::vector<int>& knotVector);
        static void rbsplinu(int npts, int k, int p1,
							 const std::vector<double>& b, const std::vector<double>& h, std::vector<double>& p);

protected:
		RS_SplineData data;
}
;

#endif
