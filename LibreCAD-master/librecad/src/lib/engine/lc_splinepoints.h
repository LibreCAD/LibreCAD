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

class QPolygonF;
struct RS_LineTypePattern;

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
	RS_Entity* clone() const override;

	/**	@return RS2::EntitySpline */
	RS2::EntityType rtti() const override;

	/** @return false */
	bool isEdge() const override;

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
	
	void update() override;

	RS_VectorSolutions getRefPoints() const override;

	/** @return Start point of the entity */
	RS_Vector getStartpoint() const override;

	/** @return End point of the entity */
	RS_Vector getEndpoint() const override;

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

	double getDirection1() const override;
	double getDirection2() const override;

	//void moveStartpoint(const RS_Vector& pos) override;
	//void moveEndpoint(const RS_Vector& pos) override;
	//RS2::Ending getTrimPoint(const RS_Vector& coord,
	//          const RS_Vector& trimPoint);
	//void reverse() override;
	/** @return the center point of the line. */
	//RS_Vector getMiddlePoint() {
	//    return (data.startpoint + data.endpoint)/2.0;
	//}
	//bool hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) override;

	/**
	* @return The length of the line.
	*/
	double getLength() const override;

	/**
	* @return The angle of the line (from start to endpoint).
	*/
	//double getAngle1() {
	//    return data.startpoint.angleTo(data.endpoint);
	//}

	/**
	* @return The angle of the line (from end to startpoint).
	*/
	//double getAngle2() {
	//    return data.endpoint.angleTo(data.startpoint);
	//}

	RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;
	RS_Vector getTangentDirection(const RS_Vector& point) const override;

	RS_Vector getNearestEndpoint(const RS_Vector& coord,
		double* dist = nullptr) const override;
    /**
     * @brief getNearestPointOnEntity
     * @param coord
     * @param onEntity, unused, because current implementation finds the nearest point on the spline
     * @param dist
     * @param entity
     * @return
     */
	RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
		bool onEntity = true, double* dist = nullptr, RS_Entity** entity = nullptr) const override;
//	RS_Vector getNearestCenter(const RS_Vector& coord,
//		double* dist = nullptr) const;
	RS_Vector getNearestMiddle(const RS_Vector& coord,
		double* dist = nullptr, int middlePoints = 1) const override;
	RS_Vector getNearestDist(double distance,
		const RS_Vector& coord, double* dist = nullptr) const override;
	//RS_Vector getNearestRef(const RS_Vector& coord,
	//                                 double* dist = nullptr);
	double getDistanceToPoint(const RS_Vector& coord,
		RS_Entity** entity = nullptr, RS2::ResolveLevel level = RS2::ResolveNone,
		double solidDist = RS_MAXDOUBLE) const override;

	bool addPoint(const RS_Vector& v);
	void removeLastPoint();
	void addControlPoint(const RS_Vector& v);

	void move(const RS_Vector& offset) override;
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

	void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
	void revertDirection() override;

	void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;
    std::vector<RS_Vector> const& getPoints() const;
    std::vector<RS_Vector> const& getControlPoints() const;
    std::vector<RS_Vector> getStrokePoints() const;

    friend std::ostream& operator << (std::ostream& os, const LC_SplinePoints& l);

	void calculateBorders() override;

	bool offset(const RS_Vector& coord, const double& distance) override;
	std::vector<RS_Entity*> offsetTwoSides(const double& distance) const override;

	static RS_VectorSolutions getIntersection(RS_Entity const* e1, RS_Entity const* e2);
	RS_VectorSolutions getLineIntersect(const RS_Vector& x1, const RS_Vector& x2);
	void addQuadIntersect(RS_VectorSolutions *pVS, const RS_Vector& x1,
		const RS_Vector& c1, const RS_Vector& x2);
	RS_VectorSolutions getSplinePointsIntersect(LC_SplinePoints* l1);
	RS_VectorSolutions getQuadraticIntersect(RS_Entity const* e1);

	// we will not enable trimming, maybe in the future
	//void trimStartpoint(const RS_Vector& pos) override;
	//void trimEndpoint(const RS_Vector& pos) override;

    LC_SplinePoints* cut(const RS_Vector& pos);
    //! \{ getBoundingRect find bounding rectangle for the bezier segment
    //! \param x1,c1,x2 first/center/last control points
    //! \return rectangle as a polygon
    static QPolygonF getBoundingRect(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
    //! \}
};

#endif

