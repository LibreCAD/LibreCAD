/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_HYPERBOLA_H
#define LC_HYPERBOLA_H

#include "rs_atomicentity.h"

class LC_Quadratic;

namespace lc {
    namespace geo {
        class Area;
    }
}

using LC_Rect = lc::geo::Area;

/**
 * Data structure for hyperbola (one or both branches)
 */
struct LC_HyperbolaData {
  LC_HyperbolaData() = default;
  LC_HyperbolaData(const RS_Vector &center, const RS_Vector &majorP,
                   double ratio, double angle1 = 0.0, double angle2 = 0.0,
                   bool reversed = false);

  LC_HyperbolaData(const RS_Vector &focus0, const RS_Vector &focus1,
                   const RS_Vector &point);

  RS_Vector getFocus1() const;
  RS_Vector getFocus2() const;
  bool isValid() const;

  RS_Vector center{};
  RS_Vector majorP{};
  double ratio = 0.0; // b/a
  double angle1 = 0.0;
  double angle2 = 0.0;
  bool reversed = false; // true = left branch
};

std::ostream &operator<<(std::ostream &os, const LC_HyperbolaData &d);

/**
 * @brief Hyperbola entity – full analytical support
 *
 * Represents a hyperbola (single branch or limited arc) with exact mathematical
 * operations. Supports:
 * - Construction from center/major axis/ratio or foci + point
 * - Exact point/tangent evaluation using hyperbolic functions (cosh/sinh)
 * - Precise intersection and geometric queries
 * - Export as standard rational quadratic SPLINE (exact, no approximation)
 *
 * Stored as a single branch aligned with positive major axis direction.
 * @author Dongxu Li
 */
class LC_Hyperbola : public RS_AtomicEntity {
public:
  LC_Hyperbola() = default;
  LC_Hyperbola(RS_EntityContainer *parent, const LC_HyperbolaData &d);

  RS_Entity *clone() const override;

  RS2::EntityType rtti() const override { return RS2::EntityHyperbola; }
  bool isValid() const { return m_valid; }

  LC_HyperbolaData &getData() { return m_data; }
  const LC_HyperbolaData &getData() const { return m_data; }

  // Core geometric accessors
  RS_VectorSolutions getFoci() const;
  RS_Vector getFocus1() const { return m_data.getFocus1(); }
  RS_Vector getFocus2() const { return m_data.getFocus2(); }

  double getMajorRadius() const { return m_data.majorP.magnitude(); }
  double getMinorRadius() const { return getMajorRadius() * m_data.ratio; }
  double getRatio() const { return m_data.ratio; }
  double getEccentricity() const {
    return std::sqrt(1.0 + m_data.ratio * m_data.ratio);
  }

  RS_Vector getPrimaryVertex() const;

  double getAngle1() const { return m_data.angle1; }
  double getAngle2() const { return m_data.angle2; }

  // the borders follow the arc: set with the default angles of 0, they span the unbounded branch
  void setAngle1(double a1) { m_data.angle1 = a1; calculateBorders(); }
  void setAngle2(double a2) { m_data.angle2 = a2; calculateBorders(); }

  RS_VectorSolutions getRefPoints() const override;

  RS_Vector getStartpoint() const override;
  RS_Vector getEndpoint() const override;
  RS_Vector getMiddlePoint() const override;

  double getLength() const override;

  // not a contour edge: hatch filling and clipping do not handle a hyperbola


  double getDirection1() const override;
  double getDirection2() const override;
  /**
   * @brief getTrimPoint
   * Determines which end of the hyperbola arc (start or end) is closer to the
   * given trim point. Used during trim/extend operations to decide which
   * endpoint should be moved.
   *    * @param trimCoord  Current mouse/coordinate position (selection point)
   * @param trimPoint  The point on the entity closest to trimCoord
   * (intersection or projection)
   * @return RS2::EndingStart if closer to start point, RS2::EndingEnd if closer
   * to end point
   */
  RS2::Ending getTrimPoint(const RS_Vector &trimCoord,
                           const RS_Vector &trimPoint) override;

  /**
   * @brief prepareTrim move whichever end of the arc the click is nearer to
   *        the chosen intersection. RS_Modification drives trim through this.
   */
  RS_Vector prepareTrim(const RS_Vector &trimCoord,
                        const RS_VectorSolutions &trimSol) override;

  RS_Vector getTangentDirectionParam(double parameter) const;
  RS_Vector getTangentDirection(const RS_Vector &point) const override;
  RS_VectorSolutions getTangentPoint(const RS_Vector &point) const override;

  RS_Vector getNearestOrthTan(const RS_Vector &coord, const RS_Line &normal,
                              bool onEntity = false) const override;

  bool isReversed() const { return m_data.reversed; }

  double getAngle() const { return m_data.majorP.angle(); }

  RS_Vector getCenter() const override { return m_data.center; }

  RS_Vector getMajorP() const { return m_data.majorP; }

  void calculateBorders() override;

  void moveRef(const RS_Vector &ref, const RS_Vector &offset) override;
  void move(const RS_Vector &offset) override;
  void rotate(const RS_Vector &center, const double &angle) override;
  void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
  void scale(const RS_Vector &center, const RS_Vector &factor) override;
  void mirror(const RS_Vector &axisPoint1,
              const RS_Vector &axisPoint2) override;
  RS_Entity &shear(double k) override;
  void revertDirection() override;

  void draw(RS_Painter *painter, RS_GraphicView *view,
            double &patternOffset) override;

  LC_Quadratic getQuadratic() const override;

  double getParamFromPoint(const RS_Vector &p,
                           bool branchReversed = false) const;
  RS_Vector getPoint(double phi, bool useReversed) const;

  /**
   * @brief dualLineTangentPoint
   * Returns the point of tangency on the hyperbola for the tangent line
   * that is orthogonal to the line defined by the given point (pole-polar
   * duality).
   *    * This implements the dual conic correspondence:
   * For a point (line) outside the hyperbola, there exists a unique polar line
   * that is tangent to the hyperbola at this returned point.
   *    * Used primarily for orth-tangent snapping (getNearestOrthTan) when a
   * normal line is provided.
   *    * @param line  A point defining the direction of the normal line
   * (through origin or arbitrary)
   * @return The point of tangency on the hyperbola, or invalid vector if no
   * real tangent exists
   */
  RS_Vector dualLineTangentPoint(const RS_Vector &line) const override;

  /**
   * @brief moveStartpoint
   * Moves the start point of the hyperbola arc to a new position.
   * The new position is projected onto the arc, and the start parameter moves
   * there while the end point stays where it is, trimming the arc.
   *
   * For unbounded (full-branch) hyperbolas, the operation is ignored because no
   * defined start point exists.
   *
   * @param pos Desired new position for the start point
   */
  void moveStartpoint(const RS_Vector &pos) override;

  /**
   * @brief moveEndpoint
   * Moves the end point of the hyperbola arc to a new position.
   * The new position is projected onto the hyperbola curve.
   * The original start point is kept fixed, and only the end angle is updated.
   *
   * For unbounded hyperbolas, the operation is ignored.
   *
   * @param pos Desired new position for the end point
   */
  void moveEndpoint(const RS_Vector &pos) override;

  /**
   * @brief areaLineIntegral
   * Computes the line integral ∮ x dy along the hyperbola arc.
   *
   * This is used for closed contour area calculation via Green's theorem:
   *     Area = ½ (∮ x dy - ∮ y dx)
   *
   * The integral is evaluated analytically using the hyperbolic
   * parametrization. Returns 0 for unbounded hyperbolas (where the integral
   * diverges) or invalid entities.
   *
   * @return The value of ∮ x dy along the arc (twice the signed area
   * contribution)
   */
  double areaLineIntegral() const override;

  /**
   * @brief Arc length of the hyperbola between parameter values phi1 and phi2.
   * @param phi1  Start hyperbolic parameter (dimensionless; argument of
   * cosh/sinh).
   * @param phi2  End hyperbolic parameter.
   * @return Signed arc length: positive when phi2 > phi1, negative otherwise.
   */
  double getArcLength(double phi1, double phi2) const;

  /**
   * @brief Returns true when both angle1 and angle2 are 0, meaning the
   *        hyperbola is unbounded (infinite arc).
   */
  bool isInfinite() const;

private:
  /**
   * @brief worldToLocal convert from world coordinates to the local coordinates
   *        the hyperbola is centered in local coordinates, and with majorP along
   *        the local x-axis direction
   * @param world world coordinates
   * @return local coordinates
   */
  RS_Vector worldToLocal(const RS_Vector& world) const;
  RS_Vector localToWorld(const RS_Vector& local) const;

  /**
   * @brief pointAtArcLength the point at an arc length from the start point,
   *        measured along the arc towards the end point
   * @param fromStart   the arc length, from 0 to totalLength
   * @param totalLength getLength(), which the callers already have
   */
  RS_Vector pointAtArcLength(double fromStart, double totalLength) const;

  /**
   * @brief transformLinear applies an affine map exactly, the image of a
   *        hyperbola being a hyperbola
   * @param newCenter the image of the centre
   * @param imageOfX  the image of the unit vector along x
   * @param imageOfY  the image of the unit vector along y
   */
  void transformLinear(const RS_Vector &newCenter, const RS_Vector &imageOfX,
                       const RS_Vector &imageOfY);

  /**
   * @brief hasLength whether an arc from phi1 to phi2 has distinct ends. Equal
   *        parameters leave no arc, and both 0 would read as the unbounded branch.
   */
  bool hasLength(double phi1, double phi2) const;

public:
    RS_Vector getNearestEndpoint(const RS_Vector &coord, double *dist = nullptr) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector &coord, bool onEntity = true,
                                      double *dist = nullptr,
                                      RS_Entity **entity = nullptr) const override;
    RS_Vector getNearestCenter(const RS_Vector &coord, double *dist = nullptr) const override;
    RS_Vector getNearestMiddle(const RS_Vector &coord, double *dist = nullptr,
                               int middlePoints = 1) const override;
    RS_Vector getNearestDist(double distance, const RS_Vector &coord,
                             double *dist = nullptr) const override;
    double getDistanceToPoint(const RS_Vector &coord, RS_Entity **entity = nullptr,
                              RS2::ResolveLevel level = RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;
    bool isPointOnEntity(const RS_Vector &coord,
                         double tolerance = RS_TOLERANCE) const override;
protected:


private:

  LC_HyperbolaData m_data;
  bool m_valid = false;
};

#endif // LC_HYPERBOLA_H
