#ifndef LC_LATTICE_H
#define LC_LATTICE_H

#include "rs_vector.h"

class LC_Lattice{
public:
    LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth);
    const RS_Vector& getMajorVector();
    const RS_Vector& getDeltaX();
    const RS_Vector& getDeltaY();
    void init(int projectedPointsCount);

    void fill(int numPointsByX, int numPointsByY,
              const RS_Vector &baseGridPoint,
              bool reverseX, bool reverseY);

    void fillWithoutDiagonal(int numPointsByX, int numPointsByY,
                             const RS_Vector &baseGridPoint,
                             bool reverseX, bool reverseY, bool bottomLeftToTopRightDiagonal, int totalSize);

    void fillVerticalEdge(int numPointsByX, const RS_Vector& baseGridPoint, bool reverseX, bool reverseY, bool skipFirstPoint = false);
    void fillHorizontalEdge(int numPointsByX, const RS_Vector& baseGridPoint, bool reverseX, bool reverseY, bool skipFirstPoint = false);

    int getPointsCount(){return currentIndex;}
    const std::vector<RS_Vector>getPoints();

    inline RS_Vector getPoint(int index) {return RS_Vector(pointsX[index], pointsY[index]);};

    RS_Vector getOffset(int xPoints, int yPoints);

    virtual ~LC_Lattice();

protected:
    RS_Vector deltaX;
    RS_Vector deltaY;
    RS_Vector majorVector;
    std::vector<double> pointsX;
    std::vector<double> pointsY;
    int currentIndex = 0;

    void fillAll(int numPointsByX, int numPointsByY,
                 const RS_Vector &baseGridPoint,
                 const RS_Vector& xDelta, const RS_Vector& yDelta);

    void fillExceptBLTRDiagonal(int numPointsByX, int numPointsByY,
                                const RS_Vector &baseGridPoint,
                                const RS_Vector& xDelta, const RS_Vector& yDelta);

    void fillExceptTLBRDiagonal(int numPointsByX, int numPointsByY,
                                const RS_Vector &baseGridPoint,
                                const RS_Vector& xDelta, const RS_Vector& yDelta, int totalSize);

    void fillHorizontalEdge(int numPointsByX, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool skipFirstPoint);
    void fillVerticalEdge(int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool skipFirstPoint);

    void prepareDeltas(bool reverseX, bool reverseY, RS_Vector &xDeltaToUse, RS_Vector &yDeltaToUse) const;
};

#endif // LC_LATTICE_H
