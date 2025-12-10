// lc_hyperbola.h
/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2025 LibreCAD.org
Copyright (C) 2025 Dongxu Li (github.com/dxli)

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
******************************************************************************/

#ifndef LC_HYPERBOLA_H
#define LC_HYPERBOLA_H

#include "rs_atomicentity.h"

class LC_Quadratic;

/**
 * Data structure for hyperbola (one or both branches)
 */
struct LC_HyperbolaData {
  LC_HyperbolaData() = default;
  LC_HyperbolaData(const RS_Vector& center,
                   const RS_Vector& majorP,
                   double ratio,
                   double angle1 = 0.0,
                   double angle2 = 0.0,
                   bool reversed = false);

  LC_HyperbolaData(const RS_Vector& focus0,
                   const RS_Vector& focus1,
                   const RS_Vector& point);

  RS_Vector center{};
  RS_Vector majorP{};
  double ratio = 0.0;
  // Angular range:
  // - If angle1 == angle2 == 0.0: full (infinite) hyperbola
  //   Branch drawing controlled by branchMode
  // - Otherwise: limited arc on the branch selected by 'reversed'
  double angle1 = 0.0;
  double angle2 = 0.0;
  bool reversed = false;  // for limited arcs: false = right branch, true = left branch

         // For full infinite hyperbola:
         // 0 = both branches (default)
         // 1 = right branch only
         // 2 = left branch only
  int branchMode = 0;
};

std::ostream& operator<<(std::ostream& os, const LC_HyperbolaData& d);

/**
 * Hyperbola entity – full analytical support
 */
class LC_Hyperbola : public RS_AtomicEntity {
public:
  LC_Hyperbola() = default;
  LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d);
  LC_Hyperbola(const RS_Vector& focus0, const RS_Vector& focus1, const RS_Vector& point);

         // Constructors from quadratic form
  LC_Hyperbola(RS_EntityContainer* parent, const std::vector<double>& coeffs);
  LC_Hyperbola(RS_EntityContainer* parent, const LC_Quadratic& q);

         // Factory methods
  bool createFromQuadratic(const LC_Quadratic& q);
  bool createFromQuadratic(const std::vector<double>& coeffs);

         // Optional convenience setter for branch mode
  void setBranchMode(int mode);  // 0=both, 1=right only, 2=left only

  RS_Entity* clone() const override;

  RS2::EntityType rtti() const override { return RS2::EntityHyperbola; }
  bool isValid() const { return m_bValid; }

  LC_HyperbolaData getData() const { return data; }
  RS_VectorSolutions getFoci() const;
  RS_VectorSolutions getRefPoints() const override;

  RS_Vector getStartpoint() const override;
  RS_Vector getEndpoint() const override;
  RS_Vector getMiddlePoint() const override;

  double getLength() const override;

  RS_Vector getNearestMiddle(const RS_Vector& coord,
                             double* dist = nullptr,
                             int middlePoints = 1) const override;

  RS_Vector getNearestDist(double distance,
                           const RS_Vector& coord,
                           double* dist = nullptr) const override;

         // Tangent methods
  double getDirection1() const override;
  double getDirection2() const override;

  RS_Vector getTangentDirection(const RS_Vector& point) const override;
  RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;

  RS_Vector getNearestOrthTan(const RS_Vector& coord,
                              const RS_Line& normal,
                              bool onEntity = false) const override;

  bool isReversed() const { return data.reversed; }
  void setReversed(bool r) { data.reversed = r; }

  double getAngle() const { return data.majorP.angle(); }
  double getAngle1() const { return data.angle1; }
  void setAngle1(double a) { data.angle1 = a; }
  double getAngle2() const { return data.angle2; }
  void setAngle2(double a) { data.angle2 = a; }

  RS_Vector getCenter() const override { return data.center; }
  void setCenter(const RS_Vector& c) { data.center = c; }

  RS_Vector getMajorP() const { return data.majorP; }
  void setMajorP(const RS_Vector& p) { data.majorP = p; }

  double getRatio() const { return data.ratio; }
  void setRatio(double r) { data.ratio = r; }

  double getMajorRadius() const { return data.majorP.magnitude(); }
  double getMinorRadius() const { return getMajorRadius() * data.ratio; }

  void calculateBorders() override {}

  RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist = nullptr) const override;
  RS_Vector getNearestPointOnEntity(const RS_Vector& coord, bool onEntity = true,
                                    double* dist = nullptr, RS_Entity** entity = nullptr) const override;
  double getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity = nullptr,
                            RS2::ResolveLevel level = RS2::ResolveNone,
                            double solidDist = RS_MAXDOUBLE) const override;
  bool isPointOnEntity(const RS_Vector& coord, double tolerance = RS_TOLERANCE) const override;

  void move(const RS_Vector& offset) override;
  void rotate(const RS_Vector& center, double angle) override;
  void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
  void scale(const RS_Vector& center, const RS_Vector& factor) override;
  void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

  void draw(RS_Painter* painter) override;

  LC_Quadratic getQuadratic() const override;

         // Public parametric utilities
  double getParamFromPoint(const RS_Vector& p, bool branchReversed = false) const;

protected:
  LC_HyperbolaData data;
  bool m_bValid = false;

private:
  // Point evaluation
  RS_Vector getPoint(double phi, bool useReversed) const;
  RS_Vector getPoint(double phi) const;

  bool isInClipRect(const RS_Vector& p,
                    double xmin, double xmax, double ymin, double ymax) const;

         // Bezier approximation for a parametric segment
  void approximateSegmentWithBeziers(std::vector<RS_Vector>& out,
                                     double phiStart, double phiEnd, bool rev,
                                     RS_Painter* painter, double errorTol,
                                     const RS_Vector& center, double angle,
                                     double a, double b) const;
};

#endif // LC_HYPERBOLA_H
