/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)
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


#ifndef LC_Parabola_H
#define LC_Parabola_H

#include <array>
#include <memory>
#include <vector>

#include "lc_splinepoints.h"

class RS_Line;
struct RS_LineData;

/**
 * Holds the data that defines a parabola.
 * @author Dongxu Li
 */
struct LC_ParabolaData
{
    /**
    * Default constructor. Leaves the data object uninitialized.
    */
    static std::vector<LC_ParabolaData> From4Points(const std::vector<RS_Vector>& points);
    static LC_ParabolaData FromEndPointsTangents(
            const std::array<RS_Vector, 2>& endPoints,
            const std::array<RS_Vector, 2>& endTangents);
    LC_ParabolaData() = default;
    LC_ParabolaData(std::array<RS_Vector, 3> controlPoints);
    RS_LineData GetAxis() const;
    RS_LineData GetDirectrix() const;
    RS_Vector GetFocus() const;

    /** \brief return the equation of the entity
    a quadratic contains coefficients for quadratic:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0
    **/
    LC_Quadratic getQuadratic() const;
    double FindX(const RS_Vector& point) const;
    RS_Vector FromX(double x) const;
    /**
     * @brief FromXWithTangent find point on curve and tangent at the given x
     * @param x - the coordinate along the x-direction
     * @return std::array<RS_Vector, 2> - point and tangent
     */
    std::array<RS_Vector, 2> FromXWithTangent(double x) const;

    // The three control points, and all other properties are calculated from control points
    std::array<RS_Vector, 3> controlPoints;
    void CalculatePrimitives();
    // properties should be calculated from control points
    RS_Vector focus;
    // a vector from the vertex to focus
    RS_Vector axis;
    RS_Vector vertex;
    // whether valid for a parabola
    bool valid = false;
};

std::ostream& operator << (std::ostream& os, const LC_ParabolaData& ld);

/**
 * Class for a parabola entity.
 *
 * @author Dongxu Li
 */
class LC_Parabola : public LC_SplinePoints // RS_EntityContainer
{
private:

    // bool offsetCut(const RS_Vector& coord, const double& distance);
    // bool offsetSpline(const RS_Vector& coord, const double& distance);
    // std::vector<RS_Entity*> offsetTwoSidesSpline(const double& distance) const;
    // std::vector<RS_Entity*> offsetTwoSidesCut(const double& distance) const;
    LC_ParabolaData data;

public:
    LC_Parabola(RS_EntityContainer* parent, const LC_ParabolaData& d);
    //RS_Entity* clone() const override;

    RS_Entity* clone() const override;

    /**	@return RS2::EntityParabola */
    RS2::EntityType rtti() const override;

    /** @return false */
    bool isEdge() const override
    {
        return true;
    }

    /** @return Copy of data that defines the spline. */
    LC_ParabolaData const& getData() const
    {
        return data;
    }
    LC_ParabolaData& getData()
    {
        return data;
    }

    /** \brief return the equation of the entity
    a quadratic contains coefficients for quadratic:
    m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

    for linear:
    m0 x + m1 y + m2 =0
    **/
    LC_Quadratic getQuadratic() const override;
    RS_VectorSolutions getRefPoints() const override;

    RS_Vector getTangentDirection(const RS_Vector& point)const override;
    //find the tangential points seeing from given point
    RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;

    /**
      * get the tangential point of a tangential line orthogonal to a given line
      *@ normal, the given line
      *@ onEntity, should the tangential be required to on entity of the elliptic arc
      *@ coord, current cursor position
      *
      *@author: Dongxu Li
      */
    RS_Vector getNearestOrthTan(const RS_Vector& coord,
                                            const RS_Line& normal,
                                            bool onEntity ) const override;

    RS_Vector dualLineTangentPoint(const RS_Vector& line) const override;
    RS2::Ending getTrimPoint(const RS_Vector& trimCoord,
                             const RS_Vector& trimPoint) override;
    RS_Vector prepareTrim(const RS_Vector& trimCoord,
                          const RS_VectorSolutions& trimSol) override;

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

    void moveStartpoint(const RS_Vector& pos) override;
    void moveEndpoint(const RS_Vector& pos) override;

    void update() override;

    //void reverse() override;
    /** @return the center point of the line. */
    //RS_Vector getMiddlePoint() {
    //    return (data.startpoint + data.endpoint)/2.0;
    //}
    //bool hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) override;

    /**
    * @return The length of the line.
    */
    // double getLength() const override;

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

    // RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;
    // RS_Vector getTangentDirection(const RS_Vector& point) const override;

    // RS_Vector getNearestEndpoint(const RS_Vector& coord,
    //                              double* dist = nullptr) const override;
    /**
     * @brief getNearestPointOnEntity
     * @param coord
     * @param onEntity, unused, because current implementation finds the nearest point on the spline
     * @param dist
     * @param entity
     * @return
     */
    // RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
    //                                   bool onEntity = true, double* dist = nullptr, RS_Entity** entity = nullptr) const override;
    // //	RS_Vector getNearestCenter(const RS_Vector& coord,
    // //		double* dist = nullptr) const;
    // RS_Vector getNearestMiddle(const RS_Vector& coord,
    //                            double* dist = nullptr, int middlePoints = 1) const override;
    // RS_Vector getNearestDist(double distance,
    //                          const RS_Vector& coord, double* dist = nullptr) const override;
    // //RS_Vector getNearestRef(const RS_Vector& coord,
    // //                                 double* dist = nullptr);
    // double getDistanceToPoint(const RS_Vector& coord,
    //                           RS_Entity** entity = nullptr, RS2::ResolveLevel level = RS2::ResolveNone,
    //                           double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;

    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    /**
     * @brief approximateOffset - approximate offset by a parabola
     * @param dist - the approximate distance for offset, positive direction is when the new parabola appears to be
     *               outside of the original
     * @return an approximate offset
     */
    std::unique_ptr<LC_Parabola> approximateOffset(double dist) const;

    // void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;
    // std::vector<RS_Vector> const& getPoints() const;
    // std::vector<RS_Vector> const& getControlPoints() const;
    // std::vector<RS_Vector> fillStrokePoints() const;

    // friend std::ostream& operator << (std::ostream& os, const LC_SplinePoints& l);

    // void calculateBorders() override;

    // bool offset(const RS_Vector& coord, const double& distance) override;
    // std::vector<RS_Entity*> offsetTwoSides(const double& distance) const override;
    // void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

private:
    // rotate a point around the parabola vertex so, the parabola is y= ax^2 + bx + c, with a > 0 after the
    // same rotation
    RS_Vector rotateToQuadratic(RS_Vector vp) const;
};

#endif
