/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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

#include "rs_entitycontainer.h"
#include "rs_vector.h"

#include <vector>

/**
 * @brief Data structure for spline properties.
 */
struct RS_SplineData {
    RS_SplineData() = default;
    RS_SplineData(int _degree, bool _closed);

    int degree = 3;                    ///< Degree of the spline (1-3)
    bool closed = false;                   ///< True if the spline is closed
    std::vector<RS_Vector> controlPoints; ///< Control points
    std::vector<double> weights;   ///< Weights for NURBS
    std::vector<double> knotslist; ///< Knot vector (unwrapped)

    friend std::ostream& operator<<(std::ostream& os, const RS_SplineData& ld);
};

/**
 * @brief Class representing a B-spline (or NURBS) entity in LibreCAD.
 */
class RS_Spline : public RS_EntityContainer {
public:
    RS_Spline(RS_EntityContainer* parent, const RS_SplineData& d);
    RS_Entity* clone() const override;

    void calculateBorders() override;

    /**
     * @brief Sets the degree of the spline (1-3).
     * @param degree The desired degree.
     * @throws std::invalid_argument if degree is not 1, 2, or 3.
     */
    void setDegree(int degree);

    /** @return Degree of the spline (1-3). */
    int getDegree() const;

    /** @return Number of control points. */
    size_t getNumberOfControlPoints() const;

    /** @return Number of knots (n + k for open, n + k + degree for closed with wrapping). */
    size_t getNumberOfKnots() const;

    /** @return True if the spline is closed. */
    bool isClosed() const;

    /** @brief Sets the closed flag of the spline. */
    void setClosed(bool c);

    /** @brief Returns the spline's data (degree, closed, control points, weights, knots). */
    const RS_SplineData &getData() const;
    RS_SplineData& getData();

    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;
    RS_Vector getNearestSelectedRef(const RS_Vector& coord, double* dist = nullptr) const override;

    void update() override;

    /** @brief Generates points along the spline curve for rendering. */
    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points);

    RS_Vector getStartpoint() const override;
    RS_Vector getEndpoint() const override;
    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector getNearestDist(double distance, const RS_Vector& coord, double* dist) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle);
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    RS_Entity& shear(double k) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;

    /** @return The control points of the spline. */
    const std::vector<RS_Vector>& getControlPoints() const;

    /** @return The weights of the control points. */
    const std::vector<double>& getWeights() const;

    /** @brief Appends a control point with an optional weight. */
    void addControlPoint(const RS_Vector& v, double w = 1.0);

    /** @brief Removes the last control point. */
    void removeLastControlPoint();

    /** @brief Gets the weight of a control point at the given index. */
    double getWeight(size_t index) const;

    /** @brief Sets the weight of a control point at the given index. */
    void setWeight(size_t index, double w);

    /** @brief Sets all weights. */
    void setWeights(const std::vector<double>& weights);

    /** @brief Checks if control points are wrapped for closed splines (cubic only). */
    bool hasWrappedControlPoints() const;

    /** @brief Sets a control point at the given index. */
    void setControlPoint(size_t index, const RS_Vector& v);

    /** @brief Sets a knot value at the given index. */
    void setKnot(size_t index, double k);

    /** @brief Inserts a control point at the given index. */
    void insertControlPoint(size_t index, const RS_Vector& v, double w = 1.0);

    /** @brief Removes a control point at the given index. */
    void removeControlPoint(size_t index);

    /**
     * @brief Returns the knot vector without wrapping.
     * @return A reference to the base knot vector (n + k elements).
     */
    const std::vector<double>& getKnotVector() const;

    /**
     * @brief Sets the knot vector, handling wrapping for closed splines.
     * @param knots The knot vector (must be non-decreasing and match expected size).
     * @throws std::invalid_argument if size or ordering is invalid.
     */
    void setKnotVector(const std::vector<double>& knots);

    /** @brief Generates an open knot vector. */
    std::vector<double> knot(size_t num, size_t order) const;

    /** @brief Generates a periodic knot vector for closed splines. */
    std::vector<double> knotu(size_t num, size_t order) const;

    /** @brief Generates points for an open rational B-spline. */
    void rbspline(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    /** @brief Generates points for a closed rational B-spline. */
    void rbsplinu(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    friend std::ostream& operator<<(std::ostream& os, const RS_Spline& l);

protected:
    RS_SplineData data;
};

#endif // RS_SPLINE_H
