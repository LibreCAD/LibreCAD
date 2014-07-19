/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pavel Krejcir (pavel@pamsoft.cz)

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


#ifndef LC_SPLINEPOINTS_H
#define LC_SPLINEPOINTS_H

#include <QList>
#include "rs_atomicentity.h"
#include "rs_linetypepattern.h"


/**
 * Holds the data that defines a line.
 */
class LC_SplinePointsData
{
public:
	/**
	* Default constructor. Leaves the data object uninitialized.
	*/
    LC_SplinePointsData() = default;

    LC_SplinePointsData(bool closed)
	{
		this->closed = closed;
	}

    friend std::ostream& operator << (std::ostream& os, const LC_SplinePointsData& ld)
	{
		os << "( closed: " << ld.closed << ")";
		return os;
	}
public:
	bool closed;
	/** points on the spline. */
	QList<RS_Vector> splinePoints;
	QList<RS_Vector> controlPoints;
};


/**
 * Class for a spline entity.
 *
 * @author Pavel Krejcir
 */
class LC_SplinePoints : public RS_AtomicEntity // RS_EntityContainer
{
private:
	void drawPattern(RS_Painter* painter, RS_GraphicView* view,
		int iPoints, double& patternOffset, RS_LineTypePattern* pat);
	void drawSimple(RS_Painter* painter, RS_GraphicView* view, int iPoints);
	void UpdateControlPoints();
	void UpdateQuadExtent(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
	int GetNearestQuad(const RS_Vector& coord, double* dist, double* dt) const;
	RS_Vector GetSplinePointAtDist(double dDist, int iStartSeg, double dStartT,
		int *piSeg, double *pdt) const;
	bool GetQuadPoints(int iSeg, RS_Vector *pvStart, RS_Vector *pvControl,
		RS_Vector *pvEnd) const;
public:
    LC_SplinePointsData data;
public:
    LC_SplinePoints(RS_EntityContainer* parent, const LC_SplinePointsData& d);
    virtual ~LC_SplinePoints();
	virtual RS_Entity* clone();

	/**	@return RS2::EntitySpline */
	virtual RS2::EntityType rtti() const
	{
		return RS2::EntitySplinePoints;
	}

	/** @return false */
	virtual bool isEdge() const
	{
		return true;
	}

	/** @return Copy of data that defines the spline. */
    LC_SplinePointsData getData() const
	{
		return data;
	}

	/** @return Number of control points. */
	int getNumberOfPoints()
	{
		return data.splinePoints.count();
	}

	/**
	* @retval true if the spline is closed.
	* @retval false otherwise.
	*/
	bool isClosed()
	{
		return data.closed;
	}

	/**
	* Sets the closed falg of this spline.
	*/
	void setClosed(bool c)
	{
		data.closed = c;
		update();
	}
	
	virtual RS_VectorSolutions getRefPoints();
	virtual RS_Vector getNearestRef(const RS_Vector& coord,
		double* dist = NULL);
	virtual RS_Vector getNearestSelectedRef(const RS_Vector& coord,
		double* dist = NULL);

	/** @return Start point of the entity */
	virtual RS_Vector getStartPoint() const;

	/** @return End point of the entity */
	virtual RS_Vector getEndPoint() const;

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

    virtual double getDirection1() const;
    virtual double getDirection2() const;

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
	virtual double getLength() const;

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

	virtual RS_VectorSolutions getTangentPoint(const RS_Vector& point) const;
	virtual RS_Vector getTangentDirection(const RS_Vector& point) const;

	virtual RS_Vector getNearestEndpoint(const RS_Vector& coord,
		double* dist = NULL) const;
	virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
		bool onEntity = true, double* dist = NULL, RS_Entity** entity = NULL) const;
	virtual RS_Vector getNearestCenter(const RS_Vector& coord,
		double* dist = NULL);
	virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
		double* dist = NULL, int middlePoints = 1) const;
	virtual RS_Vector getNearestDist(double distance,
		const RS_Vector& coord, double* dist = NULL);
	//virtual RS_Vector getNearestRef(const RS_Vector& coord,
	//                                 double* dist = NULL);
	virtual double getDistanceToPoint(const RS_Vector& coord,
		RS_Entity** entity = NULL, RS2::ResolveLevel level = RS2::ResolveNone,
		double solidDist = RS_MAXDOUBLE) const;

	bool addPoint(const RS_Vector& v);
	void removeLastPoint();

	virtual void move(const RS_Vector& offset);
	virtual void rotate(const RS_Vector& center, const double& angle);
	virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
	virtual void scale(const RS_Vector& center, const RS_Vector& factor);
	virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

	virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
	virtual void revertDirection();

	virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
	QList<RS_Vector> getPoints();

    friend std::ostream& operator << (std::ostream& os, const LC_SplinePoints& l);

	virtual void calculateBorders();

    virtual bool offset(const RS_Vector& coord, const double& distance);
    virtual QVector<RS_Entity*> offsetTwoSides(const double& distance) const;

	static RS_VectorSolutions getIntersection(RS_Entity* e1, RS_Entity* e2);
	RS_VectorSolutions getLineIntersect(RS_Line* l1);
	RS_VectorSolutions getSplinePointsIntersect(LC_SplinePoints* l1);
	RS_VectorSolutions getQuadraticIntersect(RS_Entity* e1);
};

#endif

