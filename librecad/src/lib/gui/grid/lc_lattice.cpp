#include "lc_lattice.h"
#include "rs_math.h"

LC_Lattice::LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth) {
    RS_Vector rowDelta{0, gridWidth.y};
    rowDelta.rotate(RS_Math::deg2rad(angleY));
    deltaY = rowDelta;

    RS_Vector columnDelta{gridWidth.x, 0};
    columnDelta.rotate(RS_Math::deg2rad(angleX));
    deltaX = columnDelta;

    majorVector = deltaX + deltaY;
}

LC_Lattice::~LC_Lattice() {
    pointsX.clear();
    pointsY.clear();
    pointsX.resize(0);
    pointsY.resize(0);
}

void LC_Lattice::init(int projectedPointsCount) {
    pointsX.clear();
    pointsX.resize(projectedPointsCount);
    pointsY.clear();
    pointsY.resize(projectedPointsCount);
    currentIndex = 0;
}

void LC_Lattice::fillVerticalEdge(int numPointsByX, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY, bool skipFirstPoint) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);
    fillVerticalEdge(numPointsByX, baseGridPoint, xDeltaToUse, yDeltaToUse, skipFirstPoint);
}

void LC_Lattice::fillHorizontalEdge(int numPointsByX, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY,  bool skipFirstPoint) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);
    fillHorizontalEdge(numPointsByX, baseGridPoint, xDeltaToUse, yDeltaToUse, skipFirstPoint);
}

void LC_Lattice::fillHorizontalEdge(int numPointsByX, const RS_Vector &baseGridPoint,
                                    const RS_Vector& xDelta, const RS_Vector& yDelta, bool skipFirstPoint) {
    RS_Vector base = baseGridPoint - majorVector;
    if (skipFirstPoint){
       base += xDelta;
    }
    fillAll(numPointsByX, 1, base, xDelta, yDelta);
}

void LC_Lattice::fillVerticalEdge(int numPointsByY, const RS_Vector &baseGridPoint,
                                  const RS_Vector& xDelta, const RS_Vector& yDelta, bool skipFirstPoint) {
    RS_Vector base = baseGridPoint - majorVector;
    if (skipFirstPoint) {
        base = base - yDelta;
    }
    fillAll(1, numPointsByY, base, xDelta, yDelta);
}

void LC_Lattice::fill(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAll(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);
}

void LC_Lattice::prepareDeltas(bool reverseX, bool reverseY, RS_Vector &xDeltaToUse, RS_Vector &yDeltaToUse) const {
    if (reverseX){
        xDeltaToUse = -deltaX;
    }
    else{
        xDeltaToUse = deltaX;
    }

    if (reverseY){
        yDeltaToUse = -deltaY;
    }
    else{
        yDeltaToUse = deltaY;
    }
}

void LC_Lattice::fillWithoutDiagonal(int numPointsByX, int numPointsByY,
                                     const RS_Vector &baseGridPoint,
                                     bool reverseX, bool reverseY, bool bottomLeftToTopRightDiagonal, int totalSize){
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;

    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    if (bottomLeftToTopRightDiagonal)
    {
        fillExceptBLTRDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);
    }
    else{
        fillExceptTLBRDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse, totalSize);
    }
}

//
/**
 * Fills tile by lattice starting from base point (mostly it's x0,y0), in directions provided by deltas with initial offset from base point.
 *
 * It's expected that full tile is usually square, yet it may also have some rectangular form.
 *
 * Lattice is build by columns (filling x axis) and then by rows (step to next y delta).
 * if deltas are positive, grid filled from left bottom point (base point) to right->top direction.
 * @param numPointsByX
 * @param numPointsByY
 * @param baseGridPoint
 * @param yDelta
 * @param xDelta
 */
void LC_Lattice::fillAll(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint,
                        const RS_Vector& xDelta, const RS_Vector& yDelta) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        for (int x = 0; x < numPointsByX; ++x) {
            pointsX[currentIndex] = currentPoint.x;
            pointsY[currentIndex] = currentPoint.y;
            currentIndex++;
            currentPoint += xDelta;
        }
        rowStartPoint+=yDelta;
    }
}
/**
 * Fills lattice leaving bottom-left/top-right corner diagonal empty
 * @param numPointsByX
 * @param numPointsByY
 * @param baseGridPoint
 * @param yDelta
 * @param xDelta
 */
void LC_Lattice::fillExceptBLTRDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint,
                                        const RS_Vector& xDelta, const RS_Vector& yDelta) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        for (int x = 0; x < numPointsByX; ++x) {
            if (x != y) {
                pointsX[currentIndex] = currentPoint.x;
                pointsY[currentIndex] = currentPoint.y;
                currentIndex++;
            }
            currentPoint += xDelta;
        }
        rowStartPoint+=yDelta;
    }
}
/**
 * * Fills lattice leaving top-left/bottom-right corner diagonal empty
 * @param numPointsByX
 * @param numPointsByY
 * @param baseGridPoint
 * @param yDelta
 * @param xDelta
 */
void LC_Lattice::fillExceptTLBRDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint,
                                        const RS_Vector& xDelta, const RS_Vector& yDelta, int totalSize) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        int columnToExclude = totalSize - y;
        for (int x = 0; x < numPointsByX; ++x) {
            if (x != columnToExclude) {
                pointsX[currentIndex] = currentPoint.x;
                pointsY[currentIndex] = currentPoint.y;
                currentIndex++;
            }
            currentPoint += xDelta;
        }
        rowStartPoint+=yDelta;
    }
}

const std::vector<RS_Vector> LC_Lattice::getPoints() {
    return {};// points;
}

RS_Vector LC_Lattice::getOffset(int xPointsDelta, int yPointsDelta) {
    RS_Vector result = deltaX*xPointsDelta + deltaY*yPointsDelta;
    return result;
}

const  RS_Vector &LC_Lattice::getMajorVector() {
    return majorVector;
}

const RS_Vector &LC_Lattice::getDeltaX() {
    return deltaX;
}

const  RS_Vector &LC_Lattice::getDeltaY() {
    return deltaY;
}
