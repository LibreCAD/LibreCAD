/**************************************************************************
*                                                                         *
*  Copyright (C) 2001-2003 RibbonSoft. All rights reserved.               *
*                                                                         *
*  Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)                  *
*  Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)                  *
*                                                                         *
*  This program is free software; you can redistribute it and/or          *
*  modify it under the terms of the GNU General Public License            *
*  as published by the Free Software Foundation; either version 2         *
*  of the License, or (at your option) any later version.                 *
*                                                                         *
*  This program is distributed in the hope that it will be useful,        *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*  GNU General Public License for more details.                           *
*                                                                         *
*  You should have received a copy of the GNU General Public License      *
*  along with this program; if not, write to the Free Software            *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA          *
*  02110-1301, USA.                                                       *
*                                                                         *
*  http://www.ribbonsoft.com                                              *
*                                                                         *
*  http://librecad.org                                                    *
**************************************************************************/

#ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include <iosfwd>
#include <vector>

#include "rs_entitycontainer.h"
#include "rs_painter.h"
#include "rs_vector.h"

/**
 * Holds the data that defines a spline.
 */
struct RS_SplineData {
    /** Spline representation types */
    enum class SplineType {
        Standard,      /**< Open uniform (non-clamped) */
        ClampedOpen,   /**< Open clamped (interpolates ends) */
        WrappedClosed  /**< Closed periodic (wrapped) */
    };

    /** Default constructor */
    RS_SplineData() = default;

    /** Constructor with initialization */
    RS_SplineData(int _degree, bool _closed);

    /** Check if closed (wrapped) */
    bool isClosed() const {
        return type == SplineType::WrappedClosed;
    }
    void setClosed(bool closed) {
        type = SplineType::WrappedClosed;
    }

    /** Stream output operator */
    friend std::ostream& operator<<(std::ostream& os, const RS_SplineData& ld);

    /** Control points */
    std::vector<RS_Vector> controlPoints;

    /** Knot vector */
    std::vector<double> knotslist;

    /** Weights for rational splines */
    std::vector<double> weights;

    /** Fit points for interpolation */
    std::vector<RS_Vector> fitPoints;

    /** Degree (1-3) */
    size_t degree = 3;

    /** Spline type */
    SplineType type = SplineType::ClampedOpen;

    /** Saved open type for round-trip */
    SplineType savedOpenType = SplineType::ClampedOpen;

    /** Saved open knots for round-trip */
    std::vector<double> savedOpenKnots;
};

/**
 * Spline entity class. Supports B-splines and rational B-splines.
 * Inherits from RS_EntityContainer.
 */
class RS_Spline : public RS_EntityContainer {
public:
    /** Constructor with parent and data */
    RS_Spline(RS_EntityContainer* parent, const RS_SplineData& d);

    /** Entity type */
    RS2::EntityType rtti() const override {
        return RS2::EntitySpline;
    }

    /** @return false */
    bool isEdge() const override {
        return false;
    }

    /** Clone the spline */
    RS_Entity* clone() const override;

    /** Get spline data reference */
    RS_SplineData& getData();

    /** Get const spline data reference */
    const RS_SplineData& getData() const;

    /** Get unwrapped size (control points without wrapping) */
    size_t getUnwrappedSize() const;

    /** Get unwrapped control points */
    std::vector<RS_Vector> getUnwrappedControlPoints() const;

    /** Get unwrapped weights */
    std::vector<double> getUnwrappedWeights() const;

    /** Get unwrapped knot vector */
    std::vector<double> getUnwrappedKnotVector() const;

    /** Remove wrapping from control points, weights, and knots */
    void removeWrapping();

    /** Add wrapping to control points and weights for closed splines */
    void addWrapping();

    /** Update wrapping for control points and weights */
    void updateControlAndWeightWrapping();

    /** Update knot vector wrapping for closed splines */
    void updateKnotWrapping();

    /** Calculate bounding box from control points */
    void calculateBorders() override;

    /** Set degree (1-3), throws if invalid */
    void setDegree(int degree);

    /** Get degree */
    int getDegree() const;

    /** Get number of control points (unwrapped) */
    size_t getNumberOfControlPoints() const;

    /** Get number of knots */
    size_t getNumberOfKnots() const;

    /** Check if closed */
    bool isClosed() const;

    /** Set closed flag and adjust wrapping/knots */
    void setClosed(bool c);

    /** Adjust knot vector to open clamped form */
    std::vector<double> adjustToOpenClamped(const std::vector<double>& knots,
                                            size_t num_control,
                                            size_t order,
                                            bool is_natural) const;

    /** Get reference points (unwrapped control points) */
    RS_VectorSolutions getRefPoints() const override;

    /** Nearest reference point (overrides container method) */
    RS_Vector getNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;

    /** Nearest selected reference (overrides container method) */
    RS_Vector getNearestSelectedRef(const RS_Vector& coord, double* dist = nullptr) const override;

    /** Update polyline approximation */
    void update() override;

    /** Fill points for spline approximation */
    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points);

    /** Get start point (invalid if closed) */
    RS_Vector getStartpoint() const override;

    /** Get end point (invalid if closed) */
    RS_Vector getEndpoint() const override;

    /** Nearest endpoint or control point */
    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;

    /** Nearest center (invalid) */
    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist) const override;

    /** Nearest middle point (invalid) */
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;

    /** Nearest point at distance (invalid) */
    RS_Vector getNearestDist(double distance, const RS_Vector& coord, double* dist) const override;

    /** Move by offset */
    void move(const RS_Vector& offset) override;

    /** Rotate by angle */
    void rotate(const RS_Vector& center, double angle) override;

    /** Rotate by angle vector */
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;

    /** Scale by factor */
    void scale(const RS_Vector& center, const RS_Vector& factor) override;

    /** Shear by factor */
    RS_Entity& shear(double k) override;

    /** Mirror across axis */
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    /** Move reference point (control point) */
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    /** Revert direction by reversing points, weights, knots */
    void revertDirection() override;

    /** Draw spline with painter */
    void draw(RS_Painter* painter) override;

    /** Get control points (unwrapped) */
    std::vector<RS_Vector> getControlPoints() const;

    /** Get weights (unwrapped) */
    std::vector<double> getWeights() const;

    /** Get weight at index */
    double getWeight(size_t index) const;

    /** Add control point with weight, handling wrapping */
    void addControlPoint(const RS_Vector& v, double w = 1.0);

    /** Add raw control point */
    void addControlPointRaw(const RS_Vector& v, double w = 1.0);

    /** Remove last control point, handling wrapping */
    void removeLastControlPoint();

    /** Set weight at index */
    void setWeight(size_t index, double w);

    /** Set all weights */
    void setWeights(const std::vector<double>& weights);

    /** Set control point at index */
    void setControlPoint(size_t index, const RS_Vector& v);

    /** Set knot at index */
    void setKnot(size_t index, double k);

    /** Insert control point, clear knots if present */
    void insertControlPoint(size_t index, const RS_Vector& v, double w = 1.);

    /** Remove control point, clear knots if present */
    void removeControlPoint(size_t index);

    /** Get knot vector (unwrapped) */
    std::vector<double> getKnotVector() const;

    /** Set knot vector, validate size and monotonicity */
    void setKnotVector(const std::vector<double>& knots);

    /** Generate open knot vector */
    std::vector<double> knot(size_t num, size_t order) const;

    /** Generate open uniform knot vector without multiple knots at ends */
    std::vector<double> openUniformKnot(size_t num, size_t order) const;

    /** Generate uniform knot vector for periodic splines */
    std::vector<double> knotu(size_t num, size_t order) const;

    /** Generate rational B-spline points (open) */
    void rbspline(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    /** Generate rational B-spline points (periodic) */
    void rbsplinu(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    /** Check if control points wrapped (for closed cubic splines) */
    bool hasWrappedControlPoints() const;

    /** Output operator */
    friend std::ostream& operator<<(std::ostream& os, const RS_Spline& l);

    /** Set fit points and generate spline via interpolation */
    void setFitPoints(const std::vector<RS_Vector>& fitPoints, bool useCentripetal = true);

    /** Change spline type (Standard, ClampedOpen, WrappedClosed) */
    void changeType(RS_SplineData::SplineType newType);
    static std::vector<double> rbasis(int c, double t, int npts,
                                      const std::vector<double>& x,
                                      const std::vector<double>& h);

private:
    /** Internal spline data */
    RS_SplineData data;


    /** Get non-rational B-spline basis functions */
    std::vector<double> getBSplineBasis(double t,
                                        const std::vector<double>& knots,
                                        int degree,
                                        size_t numControls) const;

    /** Solve linear system using Gaussian elimination */
    bool solveLinear(const std::vector<std::vector<double>>& A,
                     const std::vector<double>& b,
                     std::vector<double>& x) const;
};

#endif
