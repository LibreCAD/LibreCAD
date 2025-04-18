/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_lattice.h"

#include "lc_graphicviewport.h"
#include "rs_math.h"

// fixme - potentially, distortion for axis x and y should be added, as so far distortion factor is 1 (which is fine for cartesian and isometric)
LC_Lattice::LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth) {
    calcDeltas(angleX, angleY, gridWidth);
}

LC_Lattice::LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth, int numPointsTotal) {
    calcDeltas(angleX, angleY, gridWidth);
    init(numPointsTotal);
}


LC_Lattice::~LC_Lattice() {
    pointsX.clear();
    pointsY.clear();
    pointsX.resize(0);
    pointsY.resize(0);
}

void LC_Lattice::update(double angleX, double angleY, const RS_Vector &gridWidth, int numPointsTotal) {
    calcDeltas(angleX, angleY, gridWidth);
    init(numPointsTotal);
}

void LC_Lattice::updateForLines(double angleX, double angleY, const RS_Vector& gridWidth, const RS_Vector& offsetForLine, int numPointsTotal){
    calcDeltas(angleX, angleY, gridWidth);
    calcLineOffsetDeltas(angleX, angleY, offsetForLine);
    init(numPointsTotal);
}

void LC_Lattice::calcLineOffsetDeltas(double angleX, double angleY, const RS_Vector &offset) {
    RS_Vector rowDelta{0, offset.y};
    rowDelta.rotate(RS_Math::deg2rad(angleY));
    lineOffsetY = rowDelta;

    RS_Vector columnDelta{offset.x, 0};
    columnDelta.rotate(RS_Math::deg2rad(angleX));
    lineOffsetX = columnDelta;
}

void LC_Lattice::calcDeltas(double angleX, double angleY, const RS_Vector &gridWidth) {
    RS_Vector rowDelta{0, gridWidth.y};
    rowDelta.rotate(RS_Math::deg2rad(angleY));
    deltaY = rowDelta;

    RS_Vector columnDelta{gridWidth.x, 0};
    columnDelta.rotate(RS_Math::deg2rad(angleX));
    deltaX = columnDelta;

    majorVector = deltaX + deltaY;
}

void LC_Lattice::init([[maybe_unused]]int projectedPointsCount) {
    pointsX.clear();
//    pointsX.resize(projectedPointsCount);
    pointsX.resize(0);
    pointsY.clear();
//    pointsY.resize(projectedPointsCount);
    pointsY.resize(0);
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
            pointsX.push_back(currentPoint.x);
            pointsY.push_back(currentPoint.y);
            currentPoint += xDelta;
        }
        rowStartPoint+=yDelta;
    }
}

void LC_Lattice::fillByLines(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY, bool fillLeftEdge, bool fillRightEdge) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAllByLine(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse, fillLeftEdge, fillRightEdge);
}

void LC_Lattice::fillAllByLinesExceptDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY, bool fillLeftEdge, bool fillRightEdge) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAllByLineExceptDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse, fillLeftEdge, fillRightEdge);
    fillByLinesParallelDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);

}

void LC_Lattice::fillByLinesParallelDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector& xDelta, const RS_Vector& yDelta){
    RS_Vector majorDiagonalStartPoint = baseGridPoint;
    RS_Vector majorDiagonalEndPoint = baseGridPoint + xDelta * (numPointsByX + 1) + yDelta * (numPointsByY + 1);

    RS_Vector lineOffset = lineOffsetX + lineOffsetY;

    RS_Vector startPoint = majorDiagonalStartPoint + lineOffset;
    RS_Vector endPoint = majorDiagonalEndPoint -  lineOffset;

    for (int i = 0; i < numPointsByX; i++){
        startPoint = startPoint + xDelta;
        endPoint = endPoint - yDelta;

        pointsX.push_back(startPoint.x);
        pointsY.push_back(startPoint.y);

        pointsX.push_back(endPoint.x);
        pointsY.push_back(endPoint.y);
    }

    startPoint = majorDiagonalStartPoint + lineOffset;
    endPoint = majorDiagonalEndPoint - lineOffset;
    for (int i = 0; i < numPointsByX; i++){
        startPoint = startPoint + yDelta;
        endPoint = endPoint - xDelta;

        pointsX.push_back(startPoint.x);
        pointsY.push_back(startPoint.y);

        pointsX.push_back(endPoint.x);
        pointsY.push_back(endPoint.y);
    }


}

void LC_Lattice::fillAllByLine(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint,
                         const RS_Vector& xDelta, const RS_Vector& yDelta, bool fillLeftEdge, bool fillRightEdge) {
    RS_Vector rowStartPoint = baseGridPoint + yDelta;
    const RS_Vector &xLength = xDelta * (numPointsByX + 1);
    RS_Vector rowEndPoint = rowStartPoint + xLength - lineOffsetX;

    rowStartPoint.plus(lineOffsetX);
    
    if (fillRightEdge){
        RS_Vector start = baseGridPoint + lineOffsetX;
        RS_Vector end = baseGridPoint + xLength - lineOffsetX;

        pointsX.push_back(start.x);
        pointsY.push_back(start.y);

        pointsX.push_back(end.x);
        pointsY.push_back(end.y);
    }
    
    for (int y = 0; y < numPointsByY; ++y) {
        pointsX.push_back(rowStartPoint.x);
        pointsY.push_back(rowStartPoint.y);

        pointsX.push_back(rowEndPoint.x);
        pointsY.push_back(rowEndPoint.y);

        rowStartPoint += yDelta;
        rowEndPoint += yDelta;
    }

    const RS_Vector &yLength = yDelta * (numPointsByY + 1);
    if (fillLeftEdge){
        RS_Vector start = baseGridPoint + lineOffsetY;
        RS_Vector end = baseGridPoint + yLength - lineOffsetY;
        pointsX.push_back(start.x);
        pointsY.push_back(start.y);

        pointsX.push_back(end.x);
        pointsY.push_back(end.y);
    }
    
    RS_Vector colStartPoint = baseGridPoint + xDelta;

    RS_Vector colEndPoint = colStartPoint + yLength - lineOffsetY;
    colStartPoint.plus( lineOffsetY);
    for (int x = 0; x < numPointsByX; ++x) {
        pointsX.push_back(colStartPoint.x);
        pointsY.push_back(colStartPoint.y);

        pointsX.push_back(colEndPoint.x);
        pointsY.push_back(colEndPoint.y);

        colEndPoint += xDelta;
        colStartPoint += xDelta;
    }
}

void LC_Lattice::fillAllByLineExceptDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint,
                               const RS_Vector& xDelta, const RS_Vector& yDelta, bool fillLeftEdge, bool fillRightEdge) {
    RS_Vector rowStartPoint = baseGridPoint + yDelta;
    const RS_Vector &xLength = xDelta * (numPointsByX + 1);
    RS_Vector rowEndPoint = rowStartPoint + xLength - lineOffsetX;

    RS_Vector diagonalDelta = yDelta + xDelta;
    rowStartPoint.plus(lineOffsetX);

    RS_Vector diagonalPoint = baseGridPoint + diagonalDelta;
    RS_Vector diagonalPointLeft = diagonalPoint - lineOffsetX;
    RS_Vector diagonalPointRight = diagonalPoint + lineOffsetX;

    if (fillRightEdge){
        RS_Vector start = baseGridPoint + lineOffsetX;
        RS_Vector end = baseGridPoint + xLength - lineOffsetX;

        pointsX.push_back(start.x);
        pointsY.push_back(start.y);

        pointsX.push_back(end.x);
        pointsY.push_back(end.y);
    }

    for (int y = 0; y < numPointsByY; ++y) {
        pointsX.push_back(rowStartPoint.x);
        pointsY.push_back(rowStartPoint.y);

        pointsX.push_back(diagonalPointLeft.x);
        pointsY.push_back(diagonalPointLeft.y);

        pointsX.push_back(diagonalPointRight.x);
        pointsY.push_back(diagonalPointRight.y);

        pointsX.push_back(rowEndPoint.x);
        pointsY.push_back(rowEndPoint.y);

        rowStartPoint += yDelta;
        rowEndPoint += yDelta;
        diagonalPointLeft +=diagonalDelta;
        diagonalPointRight+=diagonalDelta;
    }

    const RS_Vector &yLength = yDelta * (numPointsByY + 1);

    if (fillLeftEdge){
        RS_Vector start = baseGridPoint + lineOffsetY;
        RS_Vector end = baseGridPoint + yLength - lineOffsetY;
        pointsX.push_back(start.x);
        pointsY.push_back(start.y);

        pointsX.push_back(end.x);
        pointsY.push_back(end.y);
    }

    diagonalPointLeft = diagonalPoint - lineOffsetY;
    diagonalPointRight = diagonalPoint + lineOffsetY;
    RS_Vector colStartPoint = baseGridPoint + xDelta;

    RS_Vector colEndPoint = colStartPoint + yLength - lineOffsetY;
    colStartPoint.plus( lineOffsetY);
    for (int x = 0; x < numPointsByX; ++x) {
        pointsX.push_back(colStartPoint.x);
        pointsY.push_back(colStartPoint.y);

        pointsX.push_back(diagonalPointLeft.x);
        pointsY.push_back(diagonalPointLeft.y);

        pointsX.push_back(diagonalPointRight.x);
        pointsY.push_back(diagonalPointRight.y);


        pointsX.push_back(colEndPoint.x);
        pointsY.push_back(colEndPoint.y);

        colEndPoint += xDelta;
        colStartPoint += xDelta;
        diagonalPointLeft +=diagonalDelta;
        diagonalPointRight+=diagonalDelta;
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
                pointsX.push_back(currentPoint.x);
                pointsY.push_back(currentPoint.y);
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
                pointsX.push_back(currentPoint.x);
                pointsY.push_back(currentPoint.y);
            }
            currentPoint += xDelta;
        }
        rowStartPoint+=yDelta;
    }
}

void LC_Lattice::toGui(LC_GraphicViewport* viewport){
    int numPoints = pointsX.size();
    for (int i = 0; i< numPoints; i++){
        double x = pointsX[i];
        double uiX = viewport->toGuiX(x);
        pointsX[i] = uiX;

        double y = pointsY[i];
        double uiY = viewport->toGuiY(y);
        pointsY[i] = uiY;
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


const std::vector<double> &LC_Lattice::getPointsX() const {
    return pointsX;
}

const std::vector<double> &LC_Lattice::getPointsY() const {
    return pointsY;
}

void LC_Lattice::addLine(const double x1, const double y1, const double x2, const double y2) {
    pointsX.push_back(x1);
    pointsX.push_back(x2);

    pointsY.push_back(y1);
    pointsY.push_back(y2);
}

void LC_Lattice::addPoint(double x, double y) {
    pointsX.push_back(x);
    pointsY.push_back(y);

}
