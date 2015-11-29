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

#include <vector>
#include "rs_atomicentity.h"
#include "rs_linetypepattern.h"

class QPolygonF;

/**
 * Holds the data that defines a line.
 * Few notes about implementation:
 * When drawing, the spline is defined via splinePoints collection.
 * However, since we want to allow trimming/cutting the spline,
 * we cannot guarantee that the shape would stay unchanged after
 * a part of the spline would be cut off. This would espetially be
 * obvious after cutting closed splines. So we introduce the "cut"
 * state. After that, all splinePoints will be deleted except start
 * and end points, and the controlPoints become the reference points
 * of that shape. It will be further possible to modify the spline,
 * but the control points will serve as handles then. 
 */
struct LC_SplinePointsData
{
	/**
	* Default constructor. Leaves the data object uninitialized.
	*/
    LC_SplinePointsData() = default;
	~LC_SplinePointsData() = default;

	LC_SplinePointsData(bool closed, bool cut);

	bool closed;
	bool cut;
	/** points on the spline. */
	std::vector<RS_Vector> splinePoints;
	std::vector<RS_Vector> controlPoints;
};

std::ostream& operator << (std::ostream& os, const LC_SplinePointsData& ld);

/**
 * Class for a spline entity.
 *
 * @author Pavel Krejcir
 */
class LC_SplinePoints : public RS_AtomicEntity // RS_EntityContainer
{
private:
	void drawPattern(RS_Painter* painter, RS_GraphicView* view,
        double& patternOffset, const RS_LineTypePattern* pat);
	void drawSimple(RS_Painter* painter, RS_GraphicView* view);
	void UpdateControlPoints();
	void UpdateQuadExtent(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
	int GetNearestQuad(const RS_Vector& coord, double* dist, double* dt) const;
	RS_Vector GetSplinePointAtDist(double dDist, int iStartSeg, double dStartT,
		int *piSeg, double *pdt) const;
	int GetQuadPoints(int iSeg, RS_Vector *pvStart, RS_Vector *pvControl,
		RS_Vector *pvEnd) const;

    bool offsetCut(const RS_Vector& coord, const double& distance);
    bool offsetSpline(const RS_Vector& coord, const double& distance);
	std::vector<RS_Entity*> offsetTwoSidesSpline(const double& distance) const;
	std::vector<RS_Entity*> offsetTwoSidesCut(const double& distance) const;
    LC_SplinePointsData data;

public:
    LC_SplinePoints(RS_EntityContainer* parent, const LC_SplinePointsData& d);
	virtual ~LC_SplinePoints() = default;
	virtual RS_Entity* clone() const;

	/**	@return RS2::EntitySpline */
	virtual RS2::EntityType rtti() const;

	/** @return false */
	virtual bool isEdge() const;

	/** @return Copy of data that defines the spline. */
	LC_SplinePointsData const& getData() const;
	LC_SplinePointsData& getData();

	/** @return Number of control points. */
	size_t getNumberOfControlPoints() const;

	/**
	* @retval true if the spline is closed.
	* @retval false otherwise.
	*/
	bool isClosed() const;

	/**
    * Sets the closed flag of this spline.
	*/
	void setClosed(bool c);
	
    virtual void update();

	virtual RS_VectorSolutions getRefPoints() const;

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
		double* dist = nullptr) const;
    /**
     * @brief getNearestPointOnEntity
     * @param coord
     * @param onEntity, unused, because current implementation finds the nearest point on the spline
     * @param dist
     * @param entity
     * @return
     */
	virtual RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
		bool onEntity = true, double* dist = nullptr, RS_Entity** entity = nullptr) const;
//	virtual RS_Vector getNearestCenter(const RS_Vector& coord,
//		double* dist = nullptr) const;
	virtual RS_Vector getNearestMiddle(const RS_Vector& coord,
		double* dist = nullptr, int middlePoints = 1) const;
	virtual RS_Vector getNearestDist(double distance,
		const RS_Vector& coord, double* dist = nullptr) const;
	//virtual RS_Vector getNearestRef(const RS_Vector& coord,
	//                                 double* dist = nullptr);
	virtual double getDistanceToPoint(const RS_Vector& coord,
		RS_Entity** entity = nullptr, RS2::ResolveLevel level = RS2::ResolveNone,
		double solidDist = RS_MAXDOUBLE) const;

	bool addPoint(const RS_Vector& v);
	void removeLastPoint();
	void addControlPoint(const RS_Vector& v);

	virtual void move(const RS_Vector& offset);
	virtual void rotate(const RS_Vector& center, const double& angle);
	virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
	virtual void scale(const RS_Vector& center, const RS_Vector& factor);
	virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);

	virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
	virtual void revertDirection();

	virtual void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset);
    std::vector<RS_Vector> const& getPoints() const;
    std::vector<RS_Vector> const& getControlPoints() const;
    std::vector<RS_Vector> getStrokePoints() const;

    friend std::ostream& operator << (std::ostream& os, const LC_SplinePoints& l);

	virtual void calculateBorders();

    virtual bool offset(const RS_Vector& coord, const double& distance);
	virtual std::vector<RS_Entity*> offsetTwoSides(const double& distance) const;

	static RS_VectorSolutions getIntersection(RS_Entity const* e1, RS_Entity const* e2);
	RS_VectorSolutions getLineIntersect(const RS_Vector& x1, const RS_Vector& x2);
	void addQuadIntersect(RS_VectorSolutions *pVS, const RS_Vector& x1,
		const RS_Vector& c1, const RS_Vector& x2);
	RS_VectorSolutions getSplinePointsIntersect(LC_SplinePoints* l1);
	RS_VectorSolutions getQuadraticIntersect(RS_Entity const* e1);

	// we will not enable trimming, maybe in the future
	//virtual void trimStartpoint(const RS_Vector& pos);
	//virtual void trimEndpoint(const RS_Vector& pos);

    LC_SplinePoints* cut(const RS_Vector& pos);
    //! \{ getBoundingRect find bounding rectangle for the bezier segment
    //! \param x1,c1,x2 first/center/last control points
    //! \return rectangle as a polygon
    static QPolygonF getBoundingRect(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
    //! \}
};

#endif

