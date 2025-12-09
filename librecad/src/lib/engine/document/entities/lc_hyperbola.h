/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD.org
** Copyright (C) 2025 Dongxu Li github.com/dxli
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
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
******************************************************************************/
// File: lc_hyperbola.h

#ifndef LC_HYPERBOLA_H
#define LC_HYPERBOLA_H

#include "rs_atomicentity.h"

struct LC_HyperbolaData {
  RS_Vector center{}, majorP{};
  double ratio = 0.0;
  double angle1 = 0.0, angle2 = 0.0;
  bool reversed = false;

  LC_HyperbolaData() = default;
  LC_HyperbolaData(const RS_Vector& c, const RS_Vector& m, double r,
                   double a1=0.0, double a2=0.0, bool rev=false)
      : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev) {}
  LC_HyperbolaData(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p);
};

class LC_Hyperbola : public RS_AtomicEntity {
public:
  LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d);
  LC_Hyperbola(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p);

  RS_Entity* clone() const override;

  RS2::EntityType rtti() const override { return RS2::EntityHyperbola; }
  bool isValid() const { return m_bValid; }

  double getAngle() const { return data.majorP.angle(); }
  double getMajorRadius() const { return data.majorP.magnitude(); }
  double getMinorRadius() const { return getMajorRadius() * data.ratio; }
  // Add these methods to the public section
  double getAngle1() const { return data.angle1; }
  void setAngle1(double a) { data.angle1 = a; calculateBorders(); }

  double getAngle2() const { return data.angle2; }
  void setAngle2(double a) { data.angle2 = a; calculateBorders(); }
  RS_Vector getCenter() const override { return data.center; }
  RS_VectorSolutions getFoci() const;
  RS_VectorSolutions getRefPoints() const override;

  RS_Vector getStartpoint() const override { return getPoint(data.angle1); }
  RS_Vector getEndpoint() const override   { return getPoint(data.angle2); }
  RS_Vector getMiddlePoint() const override;
  double getLength() const override;
  RS_Vector pointAtDistance(double d) const;

  RS_Vector getPoint(double phi, bool rev=false) const;
  RS_Vector getPointExact(double phi) const;

  RS_Vector getNearestPointOnEntity(const RS_Vector& c, bool onEntity=true,
                                    double* dist=nullptr, RS_Entity** ent=nullptr) const override;
  RS_Vector getNearestCenter(const RS_Vector& c, double* d=nullptr) const override;
  RS_Vector getNearestEndpoint(const RS_Vector& c, double* d=nullptr) const override;
  double getDistanceToPoint(const RS_Vector& c, RS_Entity** ent=nullptr,
                            RS2::ResolveLevel=RS2::ResolveNone, double solidDist=RS_MAXDOUBLE) const override;
  RS_Vector getNearestMiddle(const RS_Vector& coord,
                             double* dist = nullptr,
                             int middlePoints = 1) const override;

  RS_Vector getNearestDist(double distance,
                           const RS_Vector& coord,
                           double* dist = nullptr) const override;
  bool isPointOnEntity(const RS_Vector& c, double tol=RS_TOLERANCE) const override;

  RS_VectorSolutions getTangentPoint(const RS_Vector& point) const override;
  RS_Vector getTangentDirection(const RS_Vector& point) const override;

  RS2::Ending getTrimPoint(const RS_Vector& trimCoord, const RS_Vector& trimPoint) override;
  RS_Vector prepareTrim(const RS_Vector& trimCoord, const RS_VectorSolutions& sol) override;

  void move(const RS_Vector& o) override;
  void rotate(const RS_Vector& c, double a) override;
  void rotate(const RS_Vector& c, const RS_Vector& av) override;
  void scale(const RS_Vector& c, const RS_Vector& f) override;
  RS_Entity& shear(double k) override;
  void mirror(const RS_Vector& a1, const RS_Vector& a2) override;

  double areaLineIntegral() const override;
  void draw(RS_Painter* p) override;
  void calculateBorders() override;
  LC_Quadratic getQuadratic() const override;

protected:
  double getParamFromPoint(const RS_Vector& p, bool rev) const;

  LC_HyperbolaData data;
  bool m_bValid = false;
};

#endif // LC_HYPERBOLA_H
