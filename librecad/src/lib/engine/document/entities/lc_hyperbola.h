/****************************************************************************
** lc_hyperbola.h – final, const-correct, LibreCAD-ready
****************************************************************************/

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

  /** Construct from foci and a point on the hyperbola */
  LC_HyperbolaData(const RS_Vector& focus0,
                   const RS_Vector& focus1,
                   const RS_Vector& point);

  RS_Vector center{};
  RS_Vector majorP{};
  double ratio = 0.0;
  double angle1 = 0.0;
  double angle2 = 0.0;
  bool reversed = false;
};

std::ostream& operator<<(std::ostream& os, const LC_HyperbolaData& d);

/**
 * Hyperbola entity – clipped spline rendering, arc length, nearest points
 */
class LC_Hyperbola : public RS_AtomicEntity {
public:
  LC_Hyperbola() = default;
  LC_Hyperbola(RS_EntityContainer* parent, const LC_HyperbolaData& d);

  LC_Hyperbola(const RS_Vector& focus0, const RS_Vector& focus1, const RS_Vector& point);

  bool createFromQuadratic(const LC_Quadratic& q);
  bool createFromQuadratic(const std::vector<double>& coeffs);

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

  double getDirection1() const override { return 0.0; }
  double getDirection2() const override { return 0.0; }

protected:
  LC_HyperbolaData data;
  bool m_bValid = false;

private:
  struct Segment { RS_Vector p1, p2; };

         // Point evaluation (const-safe)
  RS_Vector getPoint(double phi, bool useReversed) const;
  RS_Vector getPoint(double phi) const;

  double getParamFromPoint(const RS_Vector& p, bool branchReversed) const;
  bool isInClipRect(const RS_Vector& p, double xmin, double xmax, double ymin, double ymax) const;

  void drawClippedBranch(RS_Painter* painter, const std::vector<double>& m,
                         const std::vector<Segment>& vpSeg,
                         double xmin, double xmax, double ymin, double ymax,
                         double phiMin, double phiMax, bool branchReversed) const;
  void drawSplineSegment(RS_Painter* painter,
                         double phiStart, double phiEnd,
                         bool branchReversed,
                         double xmin, double xmax, double ymin, double ymax) const;
  void drawFullApproximation(RS_Painter* painter);

  double segmentLength(double phiStart, double phiEnd, bool branchReversed, int samples = 1000) const;

  RS_Vector pointAtDistance(double distance) const;

  void samplePhis(std::vector<double>& phis, double minPhi, double maxPhi, int n = 12) const;
  bool isValidPhi(double phi, double minPhi, double maxPhi) const;
};

#endif // LC_HYPERBOLA_H
