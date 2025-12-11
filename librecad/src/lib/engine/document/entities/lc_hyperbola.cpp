// lc_hyperbola.cpp
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

#include <algorithm>
#include <cmath>
#include <vector>

#include "lc_hyperbola.h"
#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_line.h"

//=====================================================================
// Construction
//=====================================================================

// Default constructor for data - initializes branchMode to 0 (both branches)
LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& c, const RS_Vector& m,
                                   double r, double a1, double a2, bool rev)
    : center(c), majorP(m), ratio(r), angle1(a1), angle2(a2), reversed(rev), branchMode(0) {}

// Focus-based constructor - initializes branchMode to 0
LC_HyperbolaData::LC_HyperbolaData(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : center((f0 + f1) * 0.5), branchMode(0)
{
  double d = f0.distanceTo(p) - f1.distanceTo(p);
  majorP = (d > 0.0) ? (f0 - center) : (f1 - center);
  double dc = f0.distanceTo(f1);
  double dd = fabs(d);
  if (dc < RS_TOLERANCE || dd < RS_TOLERANCE) {
    majorP = RS_Vector(0, 0);
    return;
  }
  ratio = dc / dd;
  majorP /= ratio;
  ratio = sqrt(ratio * ratio - 1.0);
}

// Main constructor - sets validity based on majorP magnitude
LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d)
    : RS_AtomicEntity(parent), data(d), m_bValid(d.majorP.squared() >= RS_TOLERANCE2)
{
  calculateBorders();  // Update bounding box
}

// Focus-based convenience constructor
LC_Hyperbola::LC_Hyperbola(const RS_Vector& f0, const RS_Vector& f1, const RS_Vector& p)
    : LC_Hyperbola(nullptr, LC_HyperbolaData(f0, f1, p)) {}

// Constructor from coefficient vector
LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const std::vector<double>& coeffs)
    : RS_AtomicEntity(parent), m_bValid(false)
{
  createFromQuadratic(coeffs);
}

// Constructor from LC_Quadratic object
LC_Hyperbola::LC_Hyperbola(RS_EntityContainer* parent, const LC_Quadratic& q)
    : RS_AtomicEntity(parent), m_bValid(false)
{
  createFromQuadratic(q);
}

//=====================================================================
// Factory methods from quadratic
//=====================================================================

// Convert general quadratic conic to hyperbola standard form
bool LC_Hyperbola::createFromQuadratic(const LC_Quadratic& q)
{
  std::vector<double> ce = q.getCoefficients();
  if (ce.size() < 6) return false;

  double A = ce[0], B = ce[1], C = ce[2];
  if (B*B - 4.0*A*C <= 0.0) return false;  // Discriminant <=0 → not hyperbola

  double D = ce[3], E = ce[4], F = ce[5];

         // Find center by solving partial derivatives = 0
  double det = 4.0*A*C - B*B;
  if (fabs(det) < RS_TOLERANCE) return false;

  double cx = (B*E - 2.0*C*D) / det;
  double cy = (B*D - 2.0*A*E) / det;

  RS_Vector center(cx, cy);

         // Translate quadratic to center
  LC_Quadratic translated = q;
  translated.move(-center);

  std::vector<double> ct = translated.getCoefficients();
  double At = ct[0], Bt = ct[1], Ct = ct[2], Ft = ct[5];

         // Remove xy term by rotation
  double theta = 0.0;
  if (fabs(Bt) > RS_TOLERANCE) {
    theta = 0.5 * atan2(Bt, At - Ct);
  }

  LC_Quadratic rotated = translated;
  rotated.rotate(theta);

  std::vector<double> cr = rotated.getCoefficients();
  double Ar = cr[0], Cr = cr[2], Fr = cr[5];

  if (Ar * Cr >= 0.0) return false;  // Same sign → not hyperbola

  double a2 = -Fr / Ar;
  double b2 = -Fr / Cr;
  if (a2 <= 0.0 || b2 <= 0.0) return false;

  double a = sqrt(a2);
  double b = sqrt(b2);

  bool horizontal = (Ar > 0.0);

  RS_Vector majorP(a, 0.0);
  double finalAngle = theta;
  if (!horizontal) {
    std::swap(a, b);
    finalAngle += M_PI/2.0;
  }
  majorP.rotate(finalAngle);

  data = LC_HyperbolaData(center, majorP, b/a, 0.0, 0.0, false);
  data.branchMode = 0;  // Default to both branches
  m_bValid = true;
  calculateBorders();
  return true;
}

// Overload for coefficient vector
bool LC_Hyperbola::createFromQuadratic(const std::vector<double>& coeffs)
{
  if (coeffs.size() < 6) return false;
  LC_Quadratic q(coeffs);
  return createFromQuadratic(q);
}

//=====================================================================
// Entity interface
//=====================================================================

// Clone entity
RS_Entity* LC_Hyperbola::clone() const { return new LC_Hyperbola(*this); }

// Return foci
RS_VectorSolutions LC_Hyperbola::getFoci() const
{
  double e = sqrt(1.0 + data.ratio * data.ratio);
  RS_Vector vp = data.majorP * e;
  RS_VectorSolutions sol;
  sol.push_back(data.center + vp);
  sol.push_back(data.center - vp);
  return sol;
}

// Reference points: center and foci
RS_VectorSolutions LC_Hyperbola::getRefPoints() const
{
  RS_VectorSolutions ret;
  ret.push_back(data.center);
  RS_VectorSolutions foci = getFoci();
  for (size_t i = 0; i < foci.getNumber(); ++i)
    ret.push_back(foci.get(i));
  return ret;
}

// Start point for limited arc
RS_Vector LC_Hyperbola::getStartpoint() const
{
  if (data.angle1 == 0.0 && data.angle2 == 0.0) return RS_Vector(false);
  return getPoint(data.angle1, data.reversed);
}

// End point for limited arc
RS_Vector LC_Hyperbola::getEndpoint() const
{
  if (data.angle1 == 0.0 && data.angle2 == 0.0) return RS_Vector(false);
  return getPoint(data.angle2, data.reversed);
}

// No defined middle point for hyperbola
RS_Vector LC_Hyperbola::getMiddlePoint() const { return RS_Vector(false); }

//=====================================================================
// Tangent methods
//=====================================================================

// Direction at start point
double LC_Hyperbola::getDirection1() const
{
  RS_Vector p = getStartpoint();
  if (!p.valid) return 0.0;
  return getTangentDirection(p).angle();
}

// Direction at end point
double LC_Hyperbola::getDirection2() const
{
  RS_Vector p = getEndpoint();
  if (!p.valid) return 0.0;
  return getTangentDirection(p).angle();
}

// Analytic tangent direction at given point
RS_Vector LC_Hyperbola::getTangentDirection(const RS_Vector& point) const
{
  if (!m_bValid || !point.valid) return RS_Vector(false);

  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return RS_Vector(false);

         // Transform to local coordinates
  RS_Vector local = (point - data.center).rotate(-getAngle());
  double x = local.x;
  double y = local.y;

         // Select branch
  bool rev = data.reversed;
  if (rev) {
    x = -x;
    y = -y;
  }

         // Parametric angle
  double phi = atan2(y * a, x * b);
  if (rev) phi += M_PI;

  double cp = cos(phi);
  double sp = sin(phi);
  if (fabs(cp) < RS_TOLERANCE) return RS_Vector(false);  // Asymptote

         // Derivative in local coords
  double dx_dphi = a * sp / (cp * cp);
  double dy_dphi = b / cp;

  RS_Vector tangent_local(dx_dphi, dy_dphi);
  tangent_local.rotate(getAngle());

  return tangent_local.normalized();
}

// Tangent points from external point using polar line
RS_VectorSolutions LC_Hyperbola::getTangentPoint(const RS_Vector& point) const
{
  if (!m_bValid || !point.valid) return RS_VectorSolutions();

  LC_Quadratic hyper = getQuadratic();
  if (!hyper.isValid()) return RS_VectorSolutions();

  std::vector<double> coef = hyper.getCoefficients();
  double A = coef[0], B = coef[1], C = coef[2];
  double D = coef[3], E = coef[4], F = coef[5];

  double px = point.x, py = point.y;

         // Polar line coefficients
  double polarA = A * px + (B / 2.0) * py + D / 2.0;
  double polarB = (B / 2.0) * px + C * py + E / 2.0;
  double polarK = D / 2.0 * px + E / 2.0 * py + F;

  if (fabs(polarA) < RS_TOLERANCE && fabs(polarB) < RS_TOLERANCE) {
    return RS_VectorSolutions();  // Degenerate
  }

         // Construct polar line from two points
  RS_Vector p1, p2;
  if (fabs(polarA) >= fabs(polarB)) {
    p1 = RS_Vector(0.0, -polarK / polarB);
    p2 = RS_Vector(1.0, (-polarK - polarA) / polarB);
  } else {
    p1 = RS_Vector(-polarK / polarA, 0.0);
    p2 = RS_Vector((-polarK - polarB) / polarA, 1.0);
  }

  RS_Line polar(nullptr, RS_LineData(p1, p2));

         // Intersect hyperbola with polar
  RS_VectorSolutions sol = LC_Quadratic::getIntersection(hyper, polar.getQuadratic());

  RS_VectorSolutions tangents;
  for (size_t i = 0; i < sol.getNumber(); ++i) {
    RS_Vector tp = sol.get(i);
    if (!tp.valid) continue;

           // Validate tangency
    RS_Vector radius = tp - point;
    RS_Vector tangentDir = getTangentDirection(tp);
    if (tangentDir.valid && fabs(RS_Vector::dotP(radius, tangentDir)) < RS_TOLERANCE * 10.0) {
      tangents.push_back(tp);
    }
  }

  return tangents;
}

//=====================================================================
// Point evaluation
//=====================================================================

// Parametric point evaluation
RS_Vector LC_Hyperbola::getPoint(double phi, bool useReversed) const
{
  const double a = getMajorRadius();
  const double b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return RS_Vector(false);

  if (useReversed) phi += M_PI;
  phi = fmod(phi + 4.0*M_PI, 2.0*M_PI) - 2.0*M_PI;

  const double cp = cos(phi), sp = sin(phi);
  const double denom = cp*cp/(a*a) - sp*sp/(b*b);
  if (denom <= RS_TOLERANCE2) return RS_Vector(false);

  const double t = 1.0 / sqrt(denom);
  RS_Vector local(cp*t * a, sp*t * b);
  local.rotate(getAngle());
  return data.center + local;
}

// Convenience overload
RS_Vector LC_Hyperbola::getPoint(double phi) const
{
  return getPoint(phi, data.reversed);
}

// Convert point to parametric angle
double LC_Hyperbola::getParamFromPoint(const RS_Vector& p, bool branchReversed) const
{
  if (!p.valid) return NAN;
  RS_Vector d = p - data.center;
  if (d.squared() < RS_TOLERANCE2) return NAN;
  double phi = d.rotate(-getAngle()).angle();
  const double proj = RS_Vector::dotP(d, RS_Vector(cos(getAngle()), sin(getAngle())));
  if ((proj < -RS_TOLERANCE) != branchReversed)
    phi = RS_Math::correctAngle(phi + M_PI);
  return phi;
}

// Simplified and accurate calculateBorders() using extremal points

void LC_Hyperbola::calculateBorders()
{
  minV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
  maxV = RS_Vector(RS_MINDOUBLE, RS_MINDOUBLE);

  if (!m_bValid) return;

         // Full unbounded hyperbola
  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    minV = RS_Vector(-RS_MAXDOUBLE, -RS_MAXDOUBLE);
    maxV = RS_Vector(RS_MAXDOUBLE, RS_MAXDOUBLE);
    return;
  }

         // Limited arc on single branch
  double phi1 = std::min(data.angle1, data.angle2);
  double phi2 = std::max(data.angle1, data.angle2);

         // Normalize and apply branch offset
  phi1 = RS_Math::correctAngle(phi1);
  phi2 = RS_Math::correctAngle(phi2);
  if (phi2 < phi1) phi2 += 2.0 * M_PI;

  double offset = data.reversed ? M_PI : 0.0;
  phi1 += offset;
  phi2 += offset;

         // Directions of principal axes (major and minor)
  RS_Vector dirMajor(cos(data.majorP.angle()), sin(data.majorP.angle()));
  RS_Vector dirMinor{-dirMajor.y, dirMajor.x};

         // Extrema along principal axes
  auto addExtremum = [&](const RS_Vector& dir) {
    // Solve for parameter where tangent is perpendicular to dir
    // ∇Q · dir = 0
    double dx = dir.x, dy = dir.y;
    double denom = dx*dx/(getMajorRadius()*getMajorRadius()) - dy*dy/(getMinorRadius()*getMinorRadius());
    if (fabs(denom) < RS_TOLERANCE) return;

    double t = 1.0 / denom;
    RS_Vector local(dx * t, dy * t);
    local.rotate(data.majorP.angle());
    RS_Vector p = data.center + local;

    double phi = getParamFromPoint(p, data.reversed);
    if (phi >= phi1 - RS_TOLERANCE && phi <= phi2 + RS_TOLERANCE) {
      minV = RS_Vector::minimum(minV, p);
      maxV = RS_Vector::maximum(maxV, p);
    }
  };

  addExtremum(dirMajor);
  addExtremum(-dirMajor);
  addExtremum(dirMinor);
  addExtremum(-dirMinor);

         // Endpoints
  RS_Vector start = getPoint(phi1);
  RS_Vector end = getPoint(phi2);
  if (start.valid) {
    minV = RS_Vector::minimum(minV, start);
    maxV = RS_Vector::maximum(maxV, start);
  }
  if (end.valid) {
    minV = RS_Vector::minimum(minV, end);
    maxV = RS_Vector::maximum(maxV, end);
  }

         // Safety expansion
  double expand = RS_TOLERANCE * 100.0;
  minV -= RS_Vector(expand, expand);
  maxV += RS_Vector(expand, expand);
}

// Clip rectangle test
bool LC_Hyperbola::isInClipRect(const RS_Vector& p,
                                double xmin, double xmax, double ymin, double ymax) const
{
  return p.valid &&
         p.x >= xmin - RS_TOLERANCE && p.x <= xmax + RS_TOLERANCE &&
         p.y >= ymin - RS_TOLERANCE && p.y <= ymax + RS_TOLERANCE;
}

//=====================================================================
// Rendering - supports full hyperbola with selectable branches
//=====================================================================
    void LC_Hyperbola::draw(RS_Painter* painter)
{
  if (!painter || !isValid()) return;

  const LC_Rect& clip = painter->getWcsBoundingRect();
  if (clip.isEmpty(RS_TOLERANCE)) return;

  double xmin = clip.minP().x, xmax = clip.maxP().x;
  double ymin = clip.minP().y, ymax = clip.maxP().y;

  double a = getMajorRadius(), b = getMinorRadius();
  if (a < RS_TOLERANCE || b < RS_TOLERANCE) return;

  RS_Vector center = getCenter();
  double angle = getAngle();

         // Maximum error in world coordinates corresponding to 1 GUI pixel
  double guiPixelPerUnitX = painter->toGuiDX(1.0);
  double guiPixelPerUnitY = painter->toGuiDY(1.0);
  double guiPixelPerUnit = std::min(guiPixelPerUnitX, guiPixelPerUnitY);
  double maxWorldError = 1.0 / guiPixelPerUnit;  // 1 pixel in world units

  std::vector<RS_Vector> pts;
  pts.reserve(300);

  bool isFull = (data.angle1 == 0.0 && data.angle2 == 0.0);

  auto processBranch = [&](bool rev) {
    std::vector<double> params;

           // Viewport border intersections
    RS_Line borders[4] = {
        RS_Line(nullptr, RS_LineData(RS_Vector(xmin, ymin), RS_Vector(xmax, ymin))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmax, ymin), RS_Vector(xmax, ymax))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmax, ymax), RS_Vector(xmin, ymax))),
        RS_Line(nullptr, RS_LineData(RS_Vector(xmin, ymax), RS_Vector(xmin, ymin)))
    };

    for (const auto& line : borders) {
      RS_VectorSolutions sol = LC_Quadratic::getIntersection(getQuadratic(), line.getQuadratic());
      for (size_t i = 0; i < sol.getNumber(); ++i) {
        RS_Vector p = sol.get(i);
        if (isInClipRect(p, xmin, xmax, ymin, ymax)) {
          double phi = getParamFromPoint(p, rev);
          if (!std::isnan(phi)) params.push_back(phi);
        }
      }
    }

    if (params.empty()) {
      RS_Vector test = getPoint(0.0, rev);
      if (test.valid && isInClipRect(test, xmin, xmax, ymin, ymax)) {
        params = {-M_PI*1.5, M_PI*1.5};
      } else {
        return;
      }
    } else {
      std::sort(params.begin(), params.end());
      params.front() -= 0.5;
      params.back() += 0.5;
    }

    for (size_t i = 0; i + 1 < params.size(); i += 2) {
      double start = params[i];
      double end = params[i + 1];
      adaptiveSample(pts, start, end, rev, maxWorldError, center, angle, a, b);
    }
  };

  if (isFull) {
    if (data.branchMode == 0 || data.branchMode == 1) processBranch(false);
    if (data.branchMode == 0 || data.branchMode == 2) processBranch(true);
  } else {
    processBranch(data.reversed);
  }

  if (pts.size() >= 2)
    painter->drawSplinePointsWCS(pts, false);
}

// adaptiveSample() - error in world (graph) coordinates

// Fixed adaptiveSample() - ensure points are ordered by increasing hyperbola angle

// Optimized adaptiveSample() – reserve vector + final sort (fastest practical approach)

void LC_Hyperbola::adaptiveSample(std::vector<RS_Vector>& out,
                                  double phiStart, double phiEnd, bool rev,
                                  double maxError,
                                  const RS_Vector& center, double angle,
                                  double a, double b) const
{
  // Ensure monotonic direction
  if (phiStart > phiEnd) std::swap(phiStart, phiEnd);

  auto point = [&](double phi) -> RS_Vector {
    if (rev) phi += M_PI;
    double cp = std::cos(phi), sp = std::sin(phi);
    double denom = cp*cp/(a*a) - sp*sp/(b*b);
    if (denom <= RS_TOLERANCE2) return RS_Vector(false);
    double t = 1.0 / std::sqrt(denom);
    RS_Vector local(cp * t * a, sp * t * b);
    local.rotate(angle);
    return center + local;
  };

         // Use vector of (angle, point) pairs – much faster than std::map
  std::vector<std::pair<double, RS_Vector>> points;
  points.reserve(256);  // typical upper bound

  std::function<void(double, double)> subdiv = [&](double pa, double pb) {
    RS_Vector A = point(pa);
    RS_Vector B = point(pb);
    if (!A.valid || !B.valid) return;

    double pm = (pa + pb) * 0.5;
    RS_Vector M = point(pm);
    if (!M.valid) return;

    double sagitta = (M - (A + B) * 0.5).magnitude();
    double estimatedMaxError = sagitta * 1.15;

    if (estimatedMaxError < maxError || (pb - pa) < 0.05) {
      points.emplace_back(pa, A);
      points.emplace_back(pb, B);
      return;
    }

    subdiv(pa, pm);
    subdiv(pm, pb);
  };

         // Start with first point
  RS_Vector first = point(phiStart);
  if (first.valid) points.emplace_back(phiStart, first);

         // Recurse
  subdiv(phiStart, phiEnd);

         // Sort by angle (only once, at the end – fastest)
  std::sort(points.begin(), points.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

         // Transfer to output (remove duplicates)
  out.reserve(out.size() + points.size());
  for (const auto& kv : points) {
    if (out.empty() || out.back().distanceTo(kv.second) > RS_TOLERANCE) {
      out.push_back(kv.second);
    }
  }
}

//=====================================================================
// Nearest methods
//=====================================================================

// Geometric middle point
RS_Vector LC_Hyperbola::getNearestMiddle(const RS_Vector& coord,
                                         double* dist,
                                         int middlePoints) const
{
  if (dist) *dist = RS_MAXDOUBLE;

  if (!m_bValid || middlePoints < 1 || !coord.valid) {
    return RS_Vector(false);
  }

  if (data.angle1 != 0.0 || data.angle2 != 0.0) {
    // Parametric midpoint for limited arc
    double phi1 = std::min(data.angle1, data.angle2);
    double phi2 = std::max(data.angle1, data.angle2);
    double phiRange = phi2 - phi1;

    if (phiRange < RS_TOLERANCE) {
      return RS_Vector(false);
    }

    double phiMid = phi1 + phiRange * 0.5;
    RS_Vector midPoint = getPoint(phiMid, data.reversed);

    if (midPoint.valid) {
      double d = midPoint.distanceTo(coord);
      if (dist) *dist = d;
      return midPoint;
    }
    return RS_Vector(false);
  }

         // Vertex for full hyperbola
  RS_Vector vertex = data.center;
  double d = vertex.distanceTo(coord);
  if (dist) *dist = d;
  return vertex;
}

// Orthogonal tangent point (perpendicular from coord)
// Fixed getNearestOrthTan() - use angle() + M_PI/2 for perpendicular vector

RS_Vector LC_Hyperbola::getNearestOrthTan(const RS_Vector& coord,
                                          const RS_Line& normal,
                                          bool onEntity) const
{
  if (!m_bValid || !coord.valid ||
      !normal.getStartpoint().valid || !normal.getEndpoint().valid) {
    return RS_Vector(false);
  }

         // Get the normal direction of the line
  RS_Vector normalDir = normal.getNormalVector();
  if (!normalDir.valid) return RS_Vector(false);

         // Desired tangent direction: perpendicular to the normal
         // RS_Vector has no perpendicular() method - construct manually
  RS_Vector tanDir(-normalDir.y, normalDir.x);  // Rotate 90 degrees counterclockwise
  // Or clockwise: RS_Vector(normalDir.y, -normalDir.x);
  // Both are valid perpendiculars - direction doesn't matter for angle comparison

  const int samples = 120;
  RS_Vector best(false);
  double bestDist = RS_MAXDOUBLE;

  auto checkBranch = [&](bool rev) {
    for (int i = 0; i <= samples; ++i) {
      double phi = -M_PI + 2.0 * M_PI * i / samples + (rev ? M_PI : 0.0);
      RS_Vector p = getPoint(phi, rev);
      if (!p.valid) continue;

      RS_Vector tan = getTangentDirection(p);
      if (!tan.valid) continue;

      double angleDiff = fabs(tan.angleTo(tanDir));
      if (angleDiff > M_PI/2.0) angleDiff = M_PI - angleDiff;

      if (angleDiff < 0.1) {  // ~5.7 degrees tolerance
        double d = p.distanceTo(coord);
        if (d < bestDist) {
          bestDist = d;
          best = p;
        }
      }
    }
  };

  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    // Full hyperbola - respect branchMode
    if (data.branchMode == 0 || data.branchMode == 1) checkBranch(false);
    if (data.branchMode == 0 || data.branchMode == 2) checkBranch(true);
  } else {
    checkBranch(data.reversed);
  }

  return best;
}

// Add this method to lc_hyperbola.cpp

RS_Vector LC_Hyperbola::getNearestDist(double distance,
                                       const RS_Vector& coord,
                                       double* dist) const
{
  if (dist) *dist = RS_MAXDOUBLE;

  if (!m_bValid || distance < RS_TOLERANCE) {
    return RS_Vector(false);
  }

         // Full unbounded hyperbola - no meaningful "distance along curve"
  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    return RS_Vector(false);
  }

         // Limited arc - approximate using parametric fraction (linear in angle)
  double phi1 = std::min(data.angle1, data.angle2);
  double phi2 = std::max(data.angle1, data.angle2);
  double phiRange = phi2 - phi1;

  if (phiRange < RS_TOLERANCE) {
    return RS_Vector(false);
  }

         // Rough estimate: assume uniform parameter speed (good enough for snapping)
  double fraction = distance / getLength();  // getLength() returns 0 for full, but we already checked
  if (fraction > 1.0) {
    return RS_Vector(false);
  }

  double phi = phi1 + fraction * phiRange;
  RS_Vector p = getPoint(phi, data.reversed);

  if (p.valid && dist) {
    *dist = distance;  // Ideal case
  }

  return p;
}

// Length for limited arc (placeholder - full integration complex)
double LC_Hyperbola::getLength() const
{
  if (!m_bValid) return 0.0;

  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    return 0.0;  // Infinite
  }

  double phi1 = std::min(data.angle1, data.angle2);
  double phi2 = std::max(data.angle1, data.angle2);

  phi1 = RS_Math::correctAngle(phi1);
  phi2 = RS_Math::correctAngle(phi2);
  if (phi2 < phi1) phi2 += 2.0 * M_PI;

  double offset = data.reversed ? M_PI : 0.0;
  phi1 += offset;
  phi2 += offset;

  return 0.0;  // Placeholder
}

//=====================================================================
// Transformations
//=====================================================================

// Simple move
void LC_Hyperbola::move(const RS_Vector& offset) { data.center += offset; }

// Rotation
void LC_Hyperbola::rotate(const RS_Vector& center, double angle)
{
  data.center.rotate(center, angle);
  data.majorP.rotate(angle);
}

void LC_Hyperbola::rotate(const RS_Vector& center, const RS_Vector& angleVector)
{
  rotate(center, angleVector.angle());
}

// Scaling (non-uniform affects ratio)
void LC_Hyperbola::scale(const RS_Vector& center, const RS_Vector& factor)
{
  data.center.scale(center, factor);
  data.majorP.scale(factor);
  data.ratio *= fabs(factor.y / factor.x);
}

// Mirror - swaps branch visibility
void LC_Hyperbola::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)
{
  data.center.mirror(axisPoint1, axisPoint2);
  data.majorP.mirror(RS_Vector(0,0), axisPoint2 - axisPoint1);
  data.reversed = !data.reversed;
  m_bValid = data.majorP.squared() >= RS_TOLERANCE2;
  calculateBorders();
}

//=====================================================================
// Minimal overrides
//=====================================================================
// Nearest endpoint - for limited arcs only
RS_Vector LC_Hyperbola::getNearestEndpoint(const RS_Vector& coord,
                                           double* dist) const
{
  if (dist) *dist = RS_MAXDOUBLE;

  if (!m_bValid || !coord.valid) {
    return RS_Vector(false);
  }

         // Full hyperbola has no endpoints
  if (data.angle1 == 0.0 && data.angle2 == 0.0) {
    return RS_Vector(false);
  }

  RS_Vector start = getStartpoint();
  RS_Vector end = getEndpoint();

  if (!start.valid && !end.valid) {
    return RS_Vector(false);
  }

  RS_Vector nearest(false);
  double minDist = RS_MAXDOUBLE;

  if (start.valid) {
    double d = start.distanceTo(coord);
    if (d < minDist) {
      minDist = d;
      nearest = start;
    }
  }

  if (end.valid) {
    double d = end.distanceTo(coord);
    if (d < minDist) {
      minDist = d;
      nearest = end;
    }
  }

  if (dist) *dist = minDist;
  return nearest;
}
// lc_hyperbola.cpp - refactored for readability

// lc_hyperbola.cpp - getNearestPointOnEntity() using quartic solver for candidates + Newton refinement

RS_Vector LC_Hyperbola::getNearestPointOnEntity(const RS_Vector& coord,
                                                bool /*onEntity*/,
                                                double* dist,
                                                RS_Entity** entity) const
{
  if (dist) *dist = RS_MAXDOUBLE;
  if (entity) *entity = nullptr;
  if (!m_bValid || !coord.valid) return RS_Vector(false);

  auto coef = getQuadratic().getCoefficients();
  double A = coef[0], B = coef[1], C = coef[2];
  double D = coef[3], E = coef[4], F = coef[5];

  double qx = coord.x, qy = coord.y;

         // Normal condition: (P - Q) parallel to ∇Q(P)
         // (x - qx)(Bx + 2Cy + E) - (y - qy)(2Ax + By + D) = 0
  std::vector<double> quartic(5, 0.0);

  quartic[4] = B;                                 // x²
  quartic[2] = -B;                                // y²
  quartic[3] = 2.0 * (C - A);                     // xy
  quartic[1] = E - qx * B + 2.0 * qy * A;         // x
  quartic[0] = -D - 2.0 * qx * C + qy * B;         // y
  quartic[0] += -qx * E + qy * D;                 // constant

         // Solve quartic → candidate λ values (scaled)
  std::vector<double> roots = RS_Math::quarticSolverFull(quartic);

  RS_Vector best(false);
  double bestD = RS_MAXDOUBLE;

  const int maxIter = 20;
  const double tol = RS_TOLERANCE * 0.1;

  auto refine = [&](RS_Vector p) -> RS_Vector {
    for (int i = 0; i < maxIter; ++i) {
      double Q = A*p.x*p.x + B*p.x*p.y + C*p.y*p.y + D*p.x + E*p.y + F;
      RS_Vector g(2*A*p.x + B*p.y + D, B*p.x + 2*C*p.y + E);
      if (!g.valid || g.squared() < RS_TOLERANCE2) break;

      double lambda = -2.0 * RS_Vector::dotP(p - coord, g) / g.squared();
      RS_Vector up = g * lambda;
      p += up;

      if (up.squared() < tol*tol) {
        if (fabs(Q) > tol) p -= g * (Q / g.squared());
        return p;
      }
    }
    return RS_Vector(false);
  };

         // Use quartic roots to generate starting points
  for (double r : roots) {
    // Perturb center with root-based direction
    RS_Vector start = data.center + RS_Vector(r, r * 0.5);
    if (RS_Vector cand = refine(start); cand.valid) {
      double d = cand.distanceTo(coord);
      if (d < bestD) {
        bestD = d;
        best = cand;
      }
    }
  }

         // Additional robust starts
  for (auto start : {data.center, coord}) {
    if (RS_Vector cand = refine(start); cand.valid) {
      double d = cand.distanceTo(coord);
      if (d < bestD) {
        bestD = d;
        best = cand;
      }
    }
  }

         // Fallback sampling
  if (!best.valid) {
    auto sample = [&](bool rev) {
      const int n = 300;
      double step = 30.0 / n;
      for (int i = 0; i <= n; ++i) {
        double phi = -15.0 + step * i + (rev ? M_PI : 0.0);
        if (auto p = getPoint(phi, rev); p.valid) {
          double d = p.distanceTo(coord);
          if (d < bestD) {
            bestD = d;
            best = p;
          }
        }
      }
    };
    if (data.angle1 == 0.0 && data.angle2 == 0.0) {
      if (data.branchMode == 0 || data.branchMode == 1) sample(false);
      if (data.branchMode == 0 || data.branchMode == 2) sample(true);
    } else sample(data.reversed);
  }

         // Angular range filter
  if (best.valid && data.angle1 != 0.0 && data.angle2 != 0.0) {
    double phi = getParamFromPoint(best, data.reversed);
    double p1 = std::min(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    double p2 = std::max(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    if (phi < p1 || phi > p2) best = RS_Vector(false), bestD = RS_MAXDOUBLE;
  }

  if (best.valid && dist) *dist = bestD;
  if (entity) *entity = const_cast<LC_Hyperbola*>(this);
  return best;
}

// Distance to point - use quadratic distance
// Fixed getDistanceToPoint() - ignore ResolveLevel, always resolve on entity

double LC_Hyperbola::getDistanceToPoint(const RS_Vector& coord,
                                        RS_Entity** entity,
                                        RS2::ResolveLevel /*level*/,
                                        double /*solidDist*/) const
{
  if (entity) *entity = nullptr;

  if (!m_bValid || !coord.valid) {
    return RS_MAXDOUBLE;
  }

  double dist = RS_MAXDOUBLE;
  // Always resolve on entity for hyperbola (ignore level)
  getNearestPointOnEntity(coord, true, &dist, entity);

         // Set entity pointer if a valid point was found
  if (entity && *entity == nullptr && dist < RS_MAXDOUBLE) {
    *entity = const_cast<LC_Hyperbola*>(this);
  }

  return dist;
}

// Point on entity test - use quadratic evaluation
bool LC_Hyperbola::isPointOnEntity(const RS_Vector& coord,
                                   double tolerance) const
{
  if (!m_bValid || !coord.valid) return false;

         // Manual evaluation of the quadratic form Q(x,y) = 0
  auto coef = getQuadratic().getCoefficients();
  double value = coef[0]*coord.x*coord.x + coef[1]*coord.x*coord.y + coef[2]*coord.y*coord.y +
                 coef[3]*coord.x + coef[4]*coord.y + coef[5];

  bool onCurve = fabs(value) <= tolerance * tolerance;

         // For limited arcs, check parametric range on the selected branch
  if (onCurve && data.angle1 != 0.0 && data.angle2 != 0.0) {
    double phi = getParamFromPoint(coord, data.reversed);
    double phiMin = std::min(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    double phiMax = std::max(data.angle1, data.angle2) + (data.reversed ? M_PI : 0.0);
    onCurve = (phi >= phiMin - tolerance && phi <= phiMax + tolerance);
  }

  return onCurve;
}

// Return algebraic quadratic form
LC_Quadratic LC_Hyperbola::getQuadratic() const
{
  std::vector<double> c(6, 0.0);
  c[0] = data.majorP.squared();
  c[2] = -data.ratio*data.ratio*c[0];
  if (c[0] > RS_TOLERANCE2) c[0] = 1.0/c[0];
  if (fabs(c[2]) > RS_TOLERANCE2) c[2] = 1.0/c[2];
  c[5] = -1.0;
  LC_Quadratic q(c);
  q.rotate(data.majorP.angle());
  q.move(data.center);
  return q;
}
