#ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include <iostream>
#include <stdexcept>
#include <vector>
#include <ostream>

#include "rs_entitycontainer.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_vector.h"

/**
 * Data structure for spline.
 */
struct RS_SplineData {
    RS_SplineData() = default;
    RS_SplineData(int _degree, bool _closed);
    friend std::ostream& operator << (std::ostream& os, const RS_SplineData& ld);

    std::vector<RS_Vector> controlPoints;
    std::vector<double> knotslist;
    std::vector<double> weights;
    int degree = 3;
    bool closed = false;
    bool wrapped = false;
};

/**
 * Class for a spline entity.
 */
class RS_Spline : public RS_EntityContainer {
public:
    RS_Spline(RS_EntityContainer* parent, const RS_SplineData& d);

    RS2::EntityType rtti() const override
    {
        return RS2::EntitySpline;
    }

    RS_Entity* clone() const override;

    RS_SplineData& getData();
    const RS_SplineData& getData() const;

    size_t getUnwrappedSize() const;
    std::vector<RS_Vector> getUnwrappedControlPoints() const;
    std::vector<double> getUnwrappedWeights() const;
    std::vector<double> getUnwrappedKnotVector() const;

    void removeWrapping();
    void addWrapping();
    void updateControlAndWeightWrapping();
    void updateKnotWrapping();

    void calculateBorders();

    void setDegree(int degree);
    int getDegree() const;

    size_t getNumberOfControlPoints() const;
    size_t getNumberOfKnots() const;

    bool isClosed() const;
    void setClosed(bool c);

    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getNearestRef(const RS_Vector& coord, double* dist = nullptr) const override;
    RS_Vector getNearestSelectedRef(const RS_Vector& coord, double* dist = nullptr) const override;

    void update() override;

    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points);

    RS_Vector getStartpoint() const override;
    RS_Vector getEndpoint() const override;

    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;

    RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
                                      bool onEntity = true,
                                      double* dist = nullptr,
                                      RS_Entity** entity=nullptr)const override;
    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector getNearestDist(double distance, const RS_Vector& coord, double* dist) const override;

    double getDistanceToPoint(const RS_Vector& coord,
                              RS_Entity** entity,
                              RS2::ResolveLevel level=RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    RS_Entity& shear(double k) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;

    const std::vector<RS_Vector>& getControlPoints() const;
    const std::vector<double>& getWeights() const;
    double getWeight(size_t index) const;

    void addControlPoint(const RS_Vector& v, double w = 1.0);
    void addControlPointRaw(const RS_Vector& v, double w = 1.0);
    void removeLastControlPoint();

    void setWeight(size_t index, double w);
    void setWeights(const std::vector<double>& weights);

    void setControlPoint(size_t index, const RS_Vector& v);
    void setKnot(size_t index, double k);

    void insertControlPoint(size_t index, const RS_Vector& v, double w);
    void removeControlPoint(size_t index);

    const std::vector<double>& getKnotVector() const;
    void setKnotVector(const std::vector<double>& knots);

    std::vector<double> knot(size_t num, size_t order) const;
    std::vector<double> knotu(size_t num, size_t order) const;

    void rbspline(size_t npts, size_t k, size_t p1, const std::vector<RS_Vector>& b, const std::vector<double>& h, std::vector<RS_Vector>& p) const;
    void rbsplinu(size_t npts, size_t k, size_t p1, const std::vector<RS_Vector>& b, const std::vector<double>& h, std::vector<RS_Vector>& p) const;

    bool hasWrappedControlPoints() const;

    friend std::ostream& operator << (std::ostream& os, const RS_Spline& l);

protected:
    RS_SplineData data;
};

#endif
