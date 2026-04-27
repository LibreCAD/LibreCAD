/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2024 LibreCAD.org
** Copyright (C) 2015-2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2026 Grok team (API documentation cleanup)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef LC_QUADRATIC_H
#define LC_QUADRATIC_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <memory>
#include <vector>

class RS_Vector;
class RS_VectorSolutions;
class RS_AtomicEntity;
class RS_Entity;
class RS_Line;

/**
 * @class LC_Quadratic
 * @brief Represents a general conic section (or straight line) in the algebraic form
 *
 * The equation is stored in the canonical form:
 *
 * \f[
 * A x^2 + B xy + C y^2 + D x + E y + F = 0
 * \f]
 *
 * where:
 * - Quadratic part is stored in a symmetric 2×2 matrix `m_mQuad` (with off-diagonal = B/2).
 * - Linear part is stored in `m_vLinear` = (D, E).
 * - Constant term is `m_dConst` = F.
 *
 * This class is the core algebraic engine in LibreCAD for:
 * - Loci of circle centers (tangent to entity + passing through point)
 * - Common tangent circle loci between two entities (ellipse/hyperbola)
 * - Perpendicular bisectors
 * - Dual curves (polar reciprocals)
 * - Intersection of any two conics/lines
 *
 * All geometric transformations (`move`, `rotate`, `scale`, `shear`, `mirror`) correctly update
 * the coefficients while preserving the geometric object.
 *
 * @author Dongxu Li (original implementation)
 * @author Grok team (2026 modern C++ cleanup + full API documentation)
 */
class LC_Quadratic {
public:
    /**
     * @brief Default constructor – creates an invalid quadratic.
     */
    LC_Quadratic();

    /**
     * @brief Copy constructor.
     * @param lc0 Source quadratic to copy from.
     */
    LC_Quadratic(const LC_Quadratic& lc0);

    /**
     * @brief Move constructor (C++11).
     * @param lc0 Rvalue quadratic to move from.
     */
    LC_Quadratic(LC_Quadratic&& lc0) noexcept;

    /**
     * @brief Copy assignment operator.
     * @param lc0 Source quadratic.
     * @return Reference to this object.
     */
    LC_Quadratic& operator=(const LC_Quadratic& lc0);

    /**
     * @brief Move assignment operator (C++11).
     * @param lc0 Rvalue quadratic.
     * @return Reference to this object.
     */
    LC_Quadratic& operator=(LC_Quadratic&& lc0) noexcept;

    /**
     * @brief Construct the locus of centers of circles that are tangent to `circle`
     *        and pass through the given `point`.
     *
     * This produces a parabola, ellipse, or hyperbola (or a straight line when the
     * point lies on a line entity).
     *
     * @param circle Arc, circle, or line to be tangent to.
     * @param point  Point that all circles must pass through.
     */
    LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point);

    /**
     * @brief Construct the locus of centers of circles that have a common tangent
     *        with both given entities (circle0 and circle1).
     *
     * Depending on the entities and radii this yields an ellipse, hyperbola,
     * degenerate hyperbola (pair of lines), or a straight line.
     *
     * @param circle0 First entity (circle/arc/line).
     * @param circle1 Second entity (circle/arc/line).
     * @param mirror  If true, the mirror solution with respect to the line connecting
     *                the centers is returned (only meaningful for circle+line case).
     */
    LC_Quadratic(const RS_AtomicEntity* circle0,
                 const RS_AtomicEntity* circle1,
                 bool mirror = false);

    /**
     * @brief Construct the perpendicular bisector of the segment between `point0`
     *        and `point1`. This is the locus of centers of circles passing through
     *        both points.
     *
     * @param point0 First point.
     * @param point1 Second point.
     */
    LC_Quadratic(const RS_Vector& point0, const RS_Vector& point1);

    /**
     * @brief Construct from explicit coefficients.
     *
     * Two supported formats:
     * - 6 coefficients: {A, B, C, D, E, F} → full quadratic
     * - 3 coefficients: {D, E, F} → straight line (D x + E y + F = 0)
     *
     * @param ce Vector of coefficients (size 3 or 6).
     */
    explicit LC_Quadratic(std::vector<double> ce);

    /**
     * @brief Returns the six coefficients in standard order {A, B, C, D, E, F}.
     *
     * If the object is only linear, the vector contains only {D, E, F}.
     * Returns empty vector if the quadratic is invalid.
     *
     * @return Coefficient vector.
     */
    std::vector<double> getCoefficients() const;

    // === Geometric transformations (all return *this for chaining) ===

    /**
     * @brief Translates the conic by the given offset vector.
     *
     * Correctly updates both quadratic, linear and constant terms.
     *
     * @param offset Translation vector (dx, dy).
     * @return Reference to this (modified) object.
     */
    LC_Quadratic& move(const RS_Vector& offset);

    /**
     * @brief Rotates the conic about the origin by `angle` radians.
     *
     * @param angle Rotation angle in radians.
     * @return Reference to this object.
     */
    LC_Quadratic& rotate(double angle);

    /**
     * @brief Rotates the conic about an arbitrary center point.
     *
     * @param center Center of rotation.
     * @param angle  Rotation angle in radians.
     * @return Reference to this object.
     */
    LC_Quadratic& rotate(const RS_Vector& center, double angle);

    /**
     * @brief Non-uniform scaling of the conic about an arbitrary center.
     *
     * Works for both quadratic and linear cases. Degenerates (invalidates)
     * the object if any scaling factor is zero.
     *
     * @param center Center point of scaling.
     * @param factor Scaling factors (sx, sy).
     * @return Reference to this object.
     */
    LC_Quadratic& scale(const RS_Vector& center, const RS_Vector& factor);

    /**
     * @brief Applies a shear transformation (parallel to x-axis).
     *
     * @param k Shear factor (y' = y + k·x).
     * @return Reference to this object.
     */
    LC_Quadratic& shear(double k);

    /**
     * @brief Mirrors the conic over the line defined by two points.
     *
     * @param p1 First point on mirror line.
     * @param p2 Second point on mirror line.
     * @return Reference to this object.
     */
    LC_Quadratic& mirror(const RS_Vector& p1, const RS_Vector& p2);

    /**
     * @brief Mirrors the conic over the given line entity.
     *
     * @param axis Line to mirror over.
     * @return Reference to this object.
     */
    LC_Quadratic& mirror(const RS_Line& axis);

    /**
     * @brief Swaps x and y coordinates (reflection over y = x).
     *
     * @return A new flipped quadratic (original object unchanged).
     */
    LC_Quadratic flipXY() const;

    // === Classification / validity ===

    /**
     * @brief Returns true if this represents a true quadratic (conic) rather than
     *        a pure linear equation.
     *
     * @return true if quadratic part is non-zero.
     */
    bool isQuadratic() const;

    /**
     * @brief Implicit conversion to bool – checks validity.
     */
    explicit operator bool() const;

    /**
     * @brief Explicit validity check.
     * @return true if the quadratic is valid and usable.
     */
    bool isValid() const;

    /**
     * @brief Manually set validity flag (used internally after failed operations).
     * @param value New validity state.
     */
    void setValid(bool value);

    bool operator==(bool valid) const;
    bool operator!=(bool valid) const;

    // === Coefficient access (fast inline getters) ===

    boost::numeric::ublas::vector<double>& getLinear();
    const boost::numeric::ublas::vector<double>& getLinear() const;
    boost::numeric::ublas::matrix<double>& getQuad();
    const boost::numeric::ublas::matrix<double>& getQuad() const;

    double constTerm() const;
    double& constTerm();

    /// Coefficient of x²
    double getA() const { return m_mQuad(0, 0); }
    /// Coefficient of xy
    double getB() const { return 2.0 * m_mQuad(0, 1); }
    /// Coefficient of y²
    double getC() const { return m_mQuad(1, 1); }
    /// Coefficient of x
    double getD() const { return m_vLinear(0); }
    /// Coefficient of y
    double getE() const { return m_vLinear(1); }
    /// Constant term F
    double getF() const { return m_dConst; }

    /**
     * @brief Evaluates the quadratic form at a given point.
     *
     * @param p Point (x, y) to evaluate.
     * @return Value of A x² + B xy + C y² + D x + E y + F at p.
     */
    double evaluateAt(const RS_Vector& p) const;

    // === Conversion to LibreCAD entities ===

    /**
     * @brief Converts the algebraic quadratic into a concrete LibreCAD entity.
     *
     * Returns:
     * - RS_Line for linear case
     * - RS_Ellipse / RS_Circle
     * - LC_Parabola
     * - LC_Hyperbola (container with both branches)
     * - RS_Point or RS_Polyline for degenerate cases
     *
     * @return Raw pointer to new entity (caller must delete) or nullptr on failure.
     */
    RS_Entity* toEntity() const;

    /**
     * @brief RAII version of toEntity() – returns a unique_ptr.
     *
     * Preferred modern interface.
     *
     * @return std::unique_ptr<RS_Entity> owning the new entity.
     */
    std::unique_ptr<RS_Entity> toEntityUnique() const;

    /**
     * @brief Returns the dual (polar reciprocal) conic.
     *
     * Uses the line convention u x + v y + 1 = 0.
     * Returns an invalid quadratic for degenerate cases (e.g. parabolas).
     *
     * @return Dual quadratic (or invalid object).
     */
    LC_Quadratic getDualCurve() const;

    // === Static utilities ===

    /**
     * @brief Returns the 2×2 rotation matrix for angle (in radians).
     */
    static boost::numeric::ublas::matrix<double> rotationMatrix(double angle);

    /**
     * @brief Computes the intersection points of two quadratics (any combination
     *        of lines and conics).
     *
     * Uses the most numerically stable solver available (linear, mixed quadratic,
     * full quartic, or radical-axis reduction when possible).
     *
     * @param l1 First quadratic/line.
     * @param l2 Second quadratic/line.
     * @return All real intersection points (up to 4).
     */
    static RS_VectorSolutions getIntersection(const LC_Quadratic& l1,
                                              const LC_Quadratic& l2);

    /**
     * @brief Pretty-print the quadratic equation to a stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const LC_Quadratic& q);

private:
    boost::numeric::ublas::matrix<double> m_mQuad{2, 2};
    boost::numeric::ublas::vector<double> m_vLinear{2};
    double m_dConst = 0.0;
    bool m_bIsQuadratic = false;
    bool m_bValid = false;
};

#endif // LC_QUADRATIC_H
//EOF