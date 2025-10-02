                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      #ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a line.
 */
struct RS_SplineData {
    /**
 * Default constructor. Leaves the data object uninitialized.
 */
    RS_SplineData() = default;

    RS_SplineData(int degree, bool closed);


    /** Degree of the spline (1, 2, 3) */
    int degree = 3;
    /** Closed flag. */
    bool closed = false;
    /** Control points of the spline. */
    std::vector<RS_Vector> controlPoints;
    std::vector<double> knotslist;
    /** Weights for NURBS (default 1.0 for B-spline). Size must match controlPoints. */
    std::vector<double> weights;
};

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld);

/**
 * Class for a spline entity.
 *
 * @author Andrew Mustun
 */
class RS_Spline : public RS_EntityContainer {
public:
    RS_Spline(RS_EntityContainer* parent,
              const RS_SplineData& d);

    RS_Entity* clone() const override;


    /**	@return RS2::EntitySpline */
    RS2::EntityType rtti() const override{
        return RS2::EntitySpline;
    }
    /** @return false */
    bool isEdge() const override{
        return false;
    }

    /** @return Copy of data that defines the spline. */
    const RS_SplineData& getData() const {
        return data;
    }

    /** Sets the splines degree (1-3). */
    void setDegree(int degree);

    /** @return Degree of this spline curve (1-3).*/
    int getDegree() const;

    /** @return 0. */
    int getNumberOfKnots() {
        return 0;
    }

    /** @return Number of control points. */
    size_t getNumberOfControlPoints() const;

    /**
  * @retval true if the spline is closed.
  * @retval false otherwise.
  */
    bool isClosed() const;

    /**
  * Sets the closed flag of this spline.
  */
    void setClosed(bool c);

    /** @return Copy of weights (empty = uniform 1.0). */
    const std::vector<double>& getWeights() const;

    /** Sets weights (must match control points count). */
    void setWeights(const std::vector<double>& w);

    /** Returns the effective weights, filling with 1.0 if needed. */
    std::vector<double> getEffectiveWeights() const;

    /** Returns the effective knot vector, generating if empty. */
    std::vector<double> getKnotVector() const;

    /** Sets the knot vector. */
    void setKnotVector(const std::vector<double>& knots);

    /** Sets the control points. */
    void setControlPoints(const std::vector<RS_Vector>& cp);

    const std::vector<RS_Vector>& getControlPoints() const;

    RS_VectorSolutions getRefPoints() const override;
    RS_Vector getNearestRef( const RS_Vector& coord, double* dist = nullptr) const override;
    RS_Vector getNearestSelectedRef( const RS_Vector& coord, double* dist = nullptr) const override;

    RS_Vector getNearestPointOnEntity(const RS_Vector &coord, bool onEntity, double *dist, RS_Entity **entity) const override;

    /** @return Start point of the entity */
    RS_Vector getStartpoint() const override;
    /** @return End point of the entity */
    RS_Vector getEndpoint() const override;
    /** Sets the startpoint */
    /** Sets the endpoint */
    void update() override;

    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist = nullptr)const override;
    //RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
    //        bool onEntity=true, double* dist = nullptr, RS_Entity** entity=nullptr);
    RS_Vector getNearestCenter(const RS_Vector& coord,
                               double* dist = nullptr)const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord,
                               double* dist = nullptr,
                               int middlePoints = 1)const override;
    RS_Vector getNearestDist(double distance,
                             const RS_Vector& coord,
                             double* dist = nullptr)const override;

    /**
     * Appends the given point and weight to the control points/weights.
     * Weight defaults to 1.0 (B-spline).
     */
    void addControlPoint(const RS_Vector& v, double w = 1.0);

    /**
     * Removes the last control point/weight.
     */
    void removeLastControlPoint();

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;

    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;
    friend std::ostream& operator << (std::ostream& os, const RS_Spline& l);
    void calculateBorders() override;
    void fillStrokePoints(int splineSegments, std::vector<RS_Vector>& points);
    friend class RS_FilterDXFRW;
private:
    std::vector<double> knot(size_t num, size_t order) const;
    void rbspline(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    std::vector<double> knotu(size_t num, size_t order) const;
    void rbsplinu(size_t npts, size_t k, size_t p1,
                  const std::vector<RS_Vector>& b,
                  const std::vector<double>& h,
                  std::vector<RS_Vector>& p) const;

    /**
     * @brief hasWrappedControlPoints whether the control points are wrapped, needed for a closed spline.
     *          only implemented for cubic splines
     * @return bool - true, if the control points are already wrapped.
     *          for a cubic spline with wrapped splines, the last three control points are the same as the first three.
     */
    bool hasWrappedControlPoints() const;

protected:
    RS_SplineData data;
};

#endif // RS_SPLINE_H
