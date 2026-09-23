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

#include "lc_cachedlengthentity.h"
#include "lc_curvejet.h"
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
struct LC_SplinePointsData {
    /**
    * Default constructor. Leaves the data object uninitialized.
    */
    LC_SplinePointsData() = default;

    LC_SplinePointsData(bool closed, bool cut);

    bool closed = false;
    bool cut = false;
    // directly use control points from data, instead of generating control points from splinePoints
    bool useControlPoints = false;
    /** points on the spline. */
    std::vector<RS_Vector> splinePoints;
    std::vector<RS_Vector> controlPoints;
};

std::ostream& operator <<(std::ostream& os, const LC_SplinePointsData& ld);

/**
 * One piece of an LC_SplinePoints, as it is drawn: the spline is a chain of
 * quadratic Bezier segments, except that one control point is a point and two
 * are a line segment.
 */
struct LC_SplinePointsSegment {
    enum class Kind {
        Point,
        Line,
        Quadratic
    };
    Kind kind = Kind::Point;
    RS_Vector start{false};
    /** Quadratic segments only. */
    RS_Vector control{false};
    /** Line and quadratic segments only. */
    RS_Vector end{false};
};

/**
 * Class for a spline entity.
 *
 * @author Pavel Krejcir
 */
class LC_SplinePoints : public LC_CachedLengthEntity{

public:
    LC_SplinePoints(RS_EntityContainer* parent, LC_SplinePointsData d);
    RS_Entity* clone() const override;

    /**	@return RS2::EntitySpline */
    RS2::EntityType rtti() const override;

    /** @return false */
    bool isEdge() const override;

    /** @return Copy of data that defines the spline. */
    const LC_SplinePointsData& getData() const;
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

    //	RS_Vector getNearestCenter(const RS_Vector& coord,
    //		double* dist = nullptr) const;
    //RS_Vector getNearestRef(const RS_Vector& coord,
    //                                 double* dist = nullptr);


    bool addPoint(const RS_Vector& v);
    void removeLastPoint();
    void addControlPoint(const RS_Vector& v);

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;

    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;

    const std::vector<RS_Vector>& getPoints() const;
    const std::vector<RS_Vector>& getControlPoints() const;
    std::vector<RS_Vector> getStrokePoints() const;

    friend std::ostream& operator <<(std::ostream& os, const LC_SplinePoints& l);

    void calculateBorders() override;

    /**
     * Offsets this spline in place, keeping its type. Kept for compatibility: an
     * offset of a spline is generally not a spline of the same kind, so general
     * callers use createOffset(). A failure leaves the spline unchanged.
     */
    bool offset(const RS_Vector& coord, double distance) override;
    /** The offset through @p coord at |@p distance|, from the offset engine:
     *  several cubic RS_Spline pieces, or nothing on failure. */
    std::vector<RS_Entity*> createOffset(const RS_Vector& coord, const double& distance) const override;
    /** Both offsets at |@p distance|, or nothing unless both succeed. */
    std::vector<RS_Entity*> offsetTwoSides(double distance) const override;

    static RS_VectorSolutions getIntersection(const RS_Entity* e1, const RS_Entity* e2);
    RS_VectorSolutions getLineIntersect(const RS_Vector& x1, const RS_Vector& x2) const;
    void addQuadIntersect(RS_VectorSolutions* sol, const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2) const;
    RS_VectorSolutions getSplinePointsIntersect(LC_SplinePoints* l1) const;
    RS_VectorSolutions getQuadraticIntersect(const RS_Entity* e1) const;

    // we will not enable trimming, maybe in the future
    //void trimStartpoint(const RS_Vector& pos) override;
    //void trimEndpoint(const RS_Vector& pos) override;

    LC_SplinePoints* cut(const RS_Vector& pos);
    //! \{ getBoundingRect find bounding rectangle for the bezier segment
    //! \param x1,c1,x2 first/center/last control points
    //! \return rectangle as a polygon
    static QPolygonF getBoundingRect(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
    //! \}
    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points) const;

    /**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area = \oint x dy
 * @return line integral \oint x dy along the spline entity
 * @author Dongxu Li
 */
    double areaLineIntegral() const override;
    /**
     * @brief secondMomentLineIntegral - second-moment line integrals for a
     * quadratic B-spline, computed per segment via 5-point Gauss-Legendre
     * quadrature. Exact for each quadratic Bézier segment's polynomial integrand.
     */
    LC_SecondMoment secondMomentLineIntegral() const override;

    int getQuadPoints(int iSeg, RS_Vector* pvStart, RS_Vector* pvControl, RS_Vector* pvEnd) const;

    /**
     * Number of segments the spline consists of, built from its control points:
     * 0 without geometry, 1 for a single point, a line segment or one quadratic,
     * and otherwise one quadratic per interior control point of an open spline
     * or per control point of a closed one.
     */
    size_t getSegmentCount() const;
    /**
     * The segment with 0-based @p index, tagged with its kind.
     * @return false, leaving @p segment a point with invalid coordinates, if the
     *         index is out of range or a control point it uses is not finite.
     */
    bool tryGetSegment(size_t index, LC_SplinePointsSegment& segment) const;
    /**
     * Checked evaluation of the point and its first and second derivatives at
     * parameter @p t in [0, getSegmentCount()]: segment k covers [k, k+1], with
     * t = k + u for the segment's own Bezier parameter u. Segment joins take the
     * limit chosen by @p side. A point segment has zero derivatives, which is its
     * geometry rather than a failure. A closed spline is not wrapped around.
     * @return false, leaving @p jet invalid, for a parameter outside the domain,
     *         a limit that does not exist, or a result that is not finite.
     */
    bool tryEvaluateJet(double t, LC_CurveEvaluationSide side, LC_CurveJet& jet) const;
    /**
     * Conservative enclosures of the point and its first and second derivatives
     * over the parameter box [a, b], which must lie inside one segment. They are
     * formed exactly from the segment's Bezier control points over the box, with
     * outward-rounded arithmetic; so, with @p products, are |C'|^2 and
     * C' x C'', from the Bezier coefficients of the products
     * (speedSquaredProduct, crossProduct).
     * @return false for a box outside the domain or across a join.
     */
    bool tryBoundJet(double a, double b, LC_CurveJetBounds& bounds, bool products = false) const;
protected:
    /**
* @return The length of the line.
*/
    void updateLength() override;
    /**
     * @brief getNearestPointOnEntity
     * @param coord
     * @param onEntity
     * @param dist
     * @param entity
     * @return
     */
    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
private:
    void updateControlPointsUI();
    void updateQuadExtentUI(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2);
    int getNearestQuad(const RS_Vector& coord, double* dist, double* dt) const;
    RS_Vector getSplinePointAtDist(double dDist, int iStartSeg, double dStartT, int* piSeg, double* pdt) const;

    bool offsetCut(const RS_Vector& coord, const double& distance);
    bool offsetSpline(const RS_Vector& coord, const double& distance);
    LC_SplinePointsData m_data;

};

#endif
