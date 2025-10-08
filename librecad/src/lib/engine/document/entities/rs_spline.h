/**************************************************************************
*                                                                        *
*  Copyright (C) 2001-2003 RibbonSoft. All rights reserved.              *
*                                                                        *
*  Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)                 *
*  Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)                 *
*                                                                        *
*  This program is free software; you can redistribute it and/or         *
*  modify it under the terms of the GNU General Public License           *
*  as published by the Free Software Foundation; either version 2        *
*  of the License, or (at your option) any later version.                *
*                                                                        *
*  This program is distributed in the hope that it will be useful,       *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*  GNU General Public License for more details.                          *
*                                                                        *
*  You should have received a copy of the GNU General Public License     *
*  along with this program; if not, write to the Free Software           *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
*  02110-1301, USA.                                                      *
*                                                                        *
*  http://www.ribbonsoft.com                                             *
*                                                                        *
*  http://librecad.org                                                   *
**************************************************************************/

#ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include <iosfwd>
#include <vector>

#include "rs_entitycontainer.h"
#include "rs_painter.h"
#include "rs_vector.h"

/**
 * Data structure for spline.
 */
struct RS_SplineData {
    /** Default constructor. */
    RS_SplineData() = default;

    /** Constructor with initialisation. */
    RS_SplineData(int _degree, bool _closed);

    /** Friend operator for output. */
    friend std::ostream& operator << (std::ostream& os, const RS_SplineData& ld);

    /** Control points. */
    std::vector<RS_Vector> controlPoints;

    /** Knot vector. */
    std::vector<double> knotslist;

    /** Weights for control points (rational splines). */
    std::vector<double> weights;

    /** Degree (1-3). */
    int degree = 3;

    /** Closed flag. */
    bool closed = false;

    /** Wrapped flag (for closed splines). */
    bool wrapped = false;
};

/**
 * Spline entity class. Supports B-splines and rational B-splines.
 * Inherits from RS_EntityContainer.
 */
class RS_Spline : public RS_EntityContainer {
public:
    /** Constructor with parent and data. */
    RS_Spline(RS_EntityContainer* parent, const RS_SplineData& d);

    /** Entity type. */
    RS2::EntityType rtti() const override
    {
        return RS2::EntitySpline;
    }

    /** @return false */
    bool isEdge() const override
    {
        return false;
    }

    /** Clone the spline. */
    RS_Entity* clone() const override;

    /** Get spline data reference. */
    RS_SplineData& getData();

    /** Get const spline data reference. */
    const RS_SplineData& getData() const;

    /** Get unwrapped control points. */
    std::vector<RS_Vector> getUnwrappedControlPoints() const;

    /** Get unwrapped weights. */
    std::vector<double> getUnwrappedWeights() const;

    /** Get unwrapped knot vector. */
    std::vector<double> getUnwrappedKnotVector() const;

    /** Calculate borders. */
    void calculateBorders() override;

    /** Set degree (1-3). */
    void setDegree(int degree);

    /** Get degree. */
    int getDegree() const;

    /** Get number of control points. */
    size_t getNumberOfControlPoints() const;

    /** Get number of knots. */
    size_t getNumberOfKnots() const;

    /** Check if closed. */
    bool isClosed() const;

    /** Set closed flag. */
    void setClosed(bool c);

    /** Get reference points. */
    RS_VectorSolutions getRefPoints() const override;

    /** Nearest reference point. */
    RS_Vector getNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;

    /** Nearest selected reference. */
    RS_Vector getNearestSelectedRef(const RS_Vector& coord, double* dist = nullptr) const override;

    /** Update internal state. */
    void update() override;

    /** Fill stroke points. */
    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points);

    /** Get start point. */
    RS_Vector getStartpoint() const override;

    /** Get end point. */
    RS_Vector getEndpoint() const override;

    /** Nearest endpoint. */
    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;

    /** Nearest center. */
    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist) const override;

    /** Nearest middle point. */
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;

    /** Nearest point at distance. */
    RS_Vector getNearestDist(double distance, const RS_Vector& coord, double* dist) const override;

    /** Move by offset. */
    void move(const RS_Vector& offset) override;

    /** Rotate by angle. */
    void rotate(const RS_Vector& center, double angle) override;

    /** Rotate by angle vector. */
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;

    /** Scale by factor. */
    void scale(const RS_Vector& center, const RS_Vector& factor) override;

    /** Shear by factor. */
    RS_Entity& shear(double k) override;

    /** Mirror across axis. */
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    /** Move reference point. */
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    /** Revert direction. */
    void revertDirection() override;

    /** Draw with painter. */
    void draw(RS_Painter* painter) override;

    /** Get control points. */
    std::vector<RS_Vector> getControlPoints() const;

    /** Get weights. */
    std::vector<double> getWeights() const;

    /** Get weight at index. */
    double getWeight(size_t index) const;

    /** Add control point. */
    void addControlPoint(const RS_Vector& v, double w = 1.0);

    /** Add raw control point. */
    void addControlPointRaw(const RS_Vector& v, double w = 1.0);

    /** Remove last control point. */
    void removeLastControlPoint();

    /** Set weight at index. */
    void setWeight(size_t index, double w);

    /** Set all weights. */
    void setWeights(const std::vector<double>& weights);

    /** Set control point at index. */
    void setControlPoint(size_t index, const RS_Vector& v);

    /** Set knot at index. */
    void setKnot(size_t index, double k);

    /** Insert control point. */
    void insertControlPoint(size_t index, const RS_Vector& v, double w);

    /** Remove control point. */
    void removeControlPoint(size_t index);

    /** Get knot vector. */
    std::vector<double> getKnotVector() const;

    /** Set knot vector. */
    void setKnotVector(const std::vector<double>& knots);

    /** Check if wrapped. */
    bool hasWrappedControlPoints() const;

    /** Output operator. */
    friend std::ostream& operator << (std::ostream& os, const RS_Spline& l);

private:

    /** Remove wrapping. */
    void removeWrapping();

    /** Add wrapping. */
    void addWrapping();

    /** Update control/weight wrapping. */
    void updateControlAndWeightWrapping();

    /** Update knot wrapping. */
    void updateKnotWrapping();

    /** Get unwrapped size. */
    size_t getUnwrappedSize() const;

    /** Generate standard knot vector. */
    std::vector<double> knot(size_t num, size_t order) const;

    /** Generate uniform knot vector. */
    std::vector<double> knotu(size_t num, size_t order) const;

    /** Compute rational B-spline points. */
    void rbspline(size_t npts, size_t k, size_t p1, const std::vector<RS_Vector>& b, const std::vector<double>& h, std::vector<RS_Vector>& p) const;

    /** Compute rational B-spline points (uniform). */
    void rbsplinu(size_t npts, size_t k, size_t p1, const std::vector<RS_Vector>& b, const std::vector<double>& h, std::vector<RS_Vector>& p) const;

    /** Internal spline data. */
    RS_SplineData data;
};

#endif
