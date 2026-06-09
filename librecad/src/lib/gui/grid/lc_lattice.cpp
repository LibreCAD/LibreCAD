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
LC_Lattice::LC_Lattice(const double angleX, const double angleY, const RS_Vector& gridWidth) {
    calcDeltas(angleX, angleY, gridWidth);
}

LC_Lattice::LC_Lattice(const double angleX, const double angleY, const RS_Vector& gridWidth, const int numPointsTotal) {
    calcDeltas(angleX, angleY, gridWidth);
    init(numPointsTotal);
}

LC_Lattice::~LC_Lattice() {
    m_pointsX.clear();
    m_pointsY.clear();
    m_pointsX.resize(0);
    m_pointsY.resize(0);
}

void LC_Lattice::update(const double angleX, const double angleY, const RS_Vector& gridWidth, const int numPointsTotal) {
    calcDeltas(angleX, angleY, gridWidth);
    init(numPointsTotal);
}

void LC_Lattice::updateForLines(const double angleX, const double angleY, const RS_Vector& gridWidth, const RS_Vector& offsetForLine,
                                const int numPointsTotal) {
    calcDeltas(angleX, angleY, gridWidth);
    calcLineOffsetDeltas(angleX, angleY, offsetForLine);
    init(numPointsTotal);
}

void LC_Lattice::calcLineOffsetDeltas(const double angleX, const double angleY, const RS_Vector& offset) {
    RS_Vector rowDelta{0, offset.y};
    rowDelta.rotate(RS_Math::deg2rad(angleY));
    m_lineOffsetY = rowDelta;

    RS_Vector columnDelta{offset.x, 0};
    columnDelta.rotate(RS_Math::deg2rad(angleX));
    m_lineOffsetX = columnDelta;
}

void LC_Lattice::calcDeltas(const double angleX, const double angleY, const RS_Vector& gridWidth) {
    RS_Vector rowDelta{0, gridWidth.y};
    rowDelta.rotate(RS_Math::deg2rad(angleY));
    m_deltaY = rowDelta;

    RS_Vector columnDelta{gridWidth.x, 0};
    columnDelta.rotate(RS_Math::deg2rad(angleX));
    m_deltaX = columnDelta;

    m_majorVector = m_deltaX + m_deltaY;
}

void LC_Lattice::init([[maybe_unused]] int projectedPointsCount) {
    m_pointsX.clear();
    m_pointsX.resize(0);
    m_pointsY.clear();
    m_pointsY.resize(0);
}

void LC_Lattice::fillVerticalEdge(const int numPointsByX, const RS_Vector& baseGridPoint, const bool reverseX, const bool reverseY,
                                  const bool skipFirstPoint) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);
    fillVerticalEdge(numPointsByX, baseGridPoint, xDeltaToUse, yDeltaToUse, skipFirstPoint);
}

void LC_Lattice::fillHorizontalEdge(const int numPointsByX, const RS_Vector& baseGridPoint, const bool reverseX, const bool reverseY,
                                    const bool skipFirstPoint) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);
    fillHorizontalEdge(numPointsByX, baseGridPoint, xDeltaToUse, yDeltaToUse, skipFirstPoint);
}

void LC_Lattice::fillHorizontalEdge(const int numPointsByX, const RS_Vector& baseGridPoint, const RS_Vector& xDelta,
                                    const RS_Vector& yDelta, const bool skipFirstPoint) {
    RS_Vector base = baseGridPoint - m_majorVector;
    if (skipFirstPoint) {
        base += xDelta;
    }
    fillAll(numPointsByX, 1, base, xDelta, yDelta);
}

void LC_Lattice::fillVerticalEdge(const int numPointsByY, const RS_Vector& baseGridPoint, const RS_Vector& xDelta, const RS_Vector& yDelta,
                                  const bool skipFirstPoint) {
    RS_Vector base = baseGridPoint - m_majorVector;
    if (skipFirstPoint) {
        base = base - yDelta;
    }
    fillAll(1, numPointsByY, base, xDelta, yDelta);
}

void LC_Lattice::fill(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint, const bool reverseX,
                      const bool reverseY) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAll(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);
}

void LC_Lattice::prepareDeltas(const bool reverseX, const bool reverseY, RS_Vector& xDeltaToUse, RS_Vector& yDeltaToUse) const {
    if (reverseX) {
        xDeltaToUse = -m_deltaX;
    }
    else {
        xDeltaToUse = m_deltaX;
    }

    if (reverseY) {
        yDeltaToUse = -m_deltaY;
    }
    else {
        yDeltaToUse = m_deltaY;
    }
}

void LC_Lattice::fillWithoutDiagonal(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint, const bool reverseX,
                                     const bool reverseY, const bool bottomLeftToTopRightDiagonal, const int totalSize) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;

    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    if (bottomLeftToTopRightDiagonal) {
        fillExceptBLTRDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);
    }
    else {
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
void LC_Lattice::fillAll(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint, const RS_Vector& xDelta,
                         const RS_Vector& yDelta) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        for (int x = 0; x < numPointsByX; ++x) {
            m_pointsX.push_back(currentPoint.x);
            m_pointsY.push_back(currentPoint.y);
            currentPoint += xDelta;
        }
        rowStartPoint += yDelta;
    }
}

void LC_Lattice::fillByLines(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint, const bool reverseX,
                             const bool reverseY, const bool fillLeftEdge, const bool fillRightEdge) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAllByLine(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse, fillLeftEdge, fillRightEdge);
}

void LC_Lattice::fillAllByLinesExceptDiagonal(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint,
                                              const bool reverseX, const bool reverseY, const bool fillLeftEdge, const bool fillRightEdge) {
    RS_Vector xDeltaToUse;
    RS_Vector yDeltaToUse;
    prepareDeltas(reverseX, reverseY, xDeltaToUse, yDeltaToUse);

    fillAllByLineExceptDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse, fillLeftEdge, fillRightEdge);
    fillByLinesParallelDiagonal(numPointsByX, numPointsByY, baseGridPoint, xDeltaToUse, yDeltaToUse);
}

void LC_Lattice::fillByLinesParallelDiagonal(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint,
                                             const RS_Vector& xDelta, const RS_Vector& yDelta) {
    const RS_Vector majorDiagonalStartPoint = baseGridPoint;
    const RS_Vector majorDiagonalEndPoint = baseGridPoint + xDelta * (numPointsByX + 1) + yDelta * (numPointsByY + 1);

    const RS_Vector lineOffset = m_lineOffsetX + m_lineOffsetY;

    RS_Vector startPoint = majorDiagonalStartPoint + lineOffset;
    RS_Vector endPoint = majorDiagonalEndPoint - lineOffset;

    for (int i = 0; i < numPointsByX; i++) {
        startPoint = startPoint + xDelta;
        endPoint = endPoint - yDelta;

        m_pointsX.push_back(startPoint.x);
        m_pointsY.push_back(startPoint.y);

        m_pointsX.push_back(endPoint.x);
        m_pointsY.push_back(endPoint.y);
    }

    startPoint = majorDiagonalStartPoint + lineOffset;
    endPoint = majorDiagonalEndPoint - lineOffset;
    for (int i = 0; i < numPointsByX; i++) {
        startPoint = startPoint + yDelta;
        endPoint = endPoint - xDelta;

        m_pointsX.push_back(startPoint.x);
        m_pointsY.push_back(startPoint.y);

        m_pointsX.push_back(endPoint.x);
        m_pointsY.push_back(endPoint.y);
    }
}

void LC_Lattice::fillAllByLine(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint, const RS_Vector& xDelta,
                               const RS_Vector& yDelta, const bool fillLeftEdge, const bool fillRightEdge) {
    RS_Vector rowStartPoint = baseGridPoint + yDelta;
    const RS_Vector& xLength = xDelta * (numPointsByX + 1);
    RS_Vector rowEndPoint = rowStartPoint + xLength - m_lineOffsetX;

    rowStartPoint.plus(m_lineOffsetX);

    if (fillRightEdge) {
        const RS_Vector start = baseGridPoint + m_lineOffsetX;
        const RS_Vector end = baseGridPoint + xLength - m_lineOffsetX;

        m_pointsX.push_back(start.x);
        m_pointsY.push_back(start.y);

        m_pointsX.push_back(end.x);
        m_pointsY.push_back(end.y);
    }

    for (int y = 0; y < numPointsByY; ++y) {
        m_pointsX.push_back(rowStartPoint.x);
        m_pointsY.push_back(rowStartPoint.y);

        m_pointsX.push_back(rowEndPoint.x);
        m_pointsY.push_back(rowEndPoint.y);

        rowStartPoint += yDelta;
        rowEndPoint += yDelta;
    }

    const RS_Vector& yLength = yDelta * (numPointsByY + 1);
    if (fillLeftEdge) {
        const RS_Vector start = baseGridPoint + m_lineOffsetY;
        const RS_Vector end = baseGridPoint + yLength - m_lineOffsetY;
        m_pointsX.push_back(start.x);
        m_pointsY.push_back(start.y);

        m_pointsX.push_back(end.x);
        m_pointsY.push_back(end.y);
    }

    RS_Vector colStartPoint = baseGridPoint + xDelta;

    RS_Vector colEndPoint = colStartPoint + yLength - m_lineOffsetY;
    colStartPoint.plus(m_lineOffsetY);
    for (int x = 0; x < numPointsByX; ++x) {
        m_pointsX.push_back(colStartPoint.x);
        m_pointsY.push_back(colStartPoint.y);

        m_pointsX.push_back(colEndPoint.x);
        m_pointsY.push_back(colEndPoint.y);

        colEndPoint += xDelta;
        colStartPoint += xDelta;
    }
}

void LC_Lattice::fillAllByLineExceptDiagonal(int numPointsByX, int numPointsByY, const RS_Vector& baseGridPoint, const RS_Vector& xDelta,
                                             const RS_Vector& yDelta, bool fillLeftEdge, bool fillRightEdge) {
    RS_Vector rowStartPoint = baseGridPoint + yDelta;
    const RS_Vector& xLength = xDelta * (numPointsByX + 1);
    RS_Vector rowEndPoint = rowStartPoint + xLength - m_lineOffsetX;

    RS_Vector diagonalDelta = yDelta + xDelta;
    rowStartPoint.plus(m_lineOffsetX);

    RS_Vector diagonalPoint = baseGridPoint + diagonalDelta;
    RS_Vector diagonalPointLeft = diagonalPoint - m_lineOffsetX;
    RS_Vector diagonalPointRight = diagonalPoint + m_lineOffsetX;

    if (fillRightEdge) {
        RS_Vector start = baseGridPoint + m_lineOffsetX;
        RS_Vector end = baseGridPoint + xLength - m_lineOffsetX;

        m_pointsX.push_back(start.x);
        m_pointsY.push_back(start.y);

        m_pointsX.push_back(end.x);
        m_pointsY.push_back(end.y);
    }

    for (int y = 0; y < numPointsByY; ++y) {
        m_pointsX.push_back(rowStartPoint.x);
        m_pointsY.push_back(rowStartPoint.y);

        m_pointsX.push_back(diagonalPointLeft.x);
        m_pointsY.push_back(diagonalPointLeft.y);

        m_pointsX.push_back(diagonalPointRight.x);
        m_pointsY.push_back(diagonalPointRight.y);

        m_pointsX.push_back(rowEndPoint.x);
        m_pointsY.push_back(rowEndPoint.y);

        rowStartPoint += yDelta;
        rowEndPoint += yDelta;
        diagonalPointLeft += diagonalDelta;
        diagonalPointRight += diagonalDelta;
    }

    const RS_Vector& yLength = yDelta * (numPointsByY + 1);

    if (fillLeftEdge) {
        RS_Vector start = baseGridPoint + m_lineOffsetY;
        RS_Vector end = baseGridPoint + yLength - m_lineOffsetY;
        m_pointsX.push_back(start.x);
        m_pointsY.push_back(start.y);

        m_pointsX.push_back(end.x);
        m_pointsY.push_back(end.y);
    }

    diagonalPointLeft = diagonalPoint - m_lineOffsetY;
    diagonalPointRight = diagonalPoint + m_lineOffsetY;
    RS_Vector colStartPoint = baseGridPoint + xDelta;

    RS_Vector colEndPoint = colStartPoint + yLength - m_lineOffsetY;
    colStartPoint.plus(m_lineOffsetY);
    for (int x = 0; x < numPointsByX; ++x) {
        m_pointsX.push_back(colStartPoint.x);
        m_pointsY.push_back(colStartPoint.y);

        m_pointsX.push_back(diagonalPointLeft.x);
        m_pointsY.push_back(diagonalPointLeft.y);

        m_pointsX.push_back(diagonalPointRight.x);
        m_pointsY.push_back(diagonalPointRight.y);

        m_pointsX.push_back(colEndPoint.x);
        m_pointsY.push_back(colEndPoint.y);

        colEndPoint += xDelta;
        colStartPoint += xDelta;
        diagonalPointLeft += diagonalDelta;
        diagonalPointRight += diagonalDelta;
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
void LC_Lattice::fillExceptBLTRDiagonal(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint,
                                        const RS_Vector& xDelta, const RS_Vector& yDelta) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        for (int x = 0; x < numPointsByX; ++x) {
            if (x != y) {
                m_pointsX.push_back(currentPoint.x);
                m_pointsY.push_back(currentPoint.y);
            }
            currentPoint += xDelta;
        }
        rowStartPoint += yDelta;
    }
}

/**
 * * Fills lattice leaving top-left/bottom-right corner diagonal empty
 * @param numPointsByX
 * @param numPointsByY
 * @param baseGridPoint
 * @param yDelta
 * @param totalSize
 * @param xDelta
 */
void LC_Lattice::fillExceptTLBRDiagonal(const int numPointsByX, const int numPointsByY, const RS_Vector& baseGridPoint,
                                        const RS_Vector& xDelta, const RS_Vector& yDelta, const int totalSize) {
    RS_Vector rowStartPoint = baseGridPoint + xDelta + yDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        const int columnToExclude = totalSize - y;
        for (int x = 0; x < numPointsByX; ++x) {
            if (x != columnToExclude) {
                m_pointsX.push_back(currentPoint.x);
                m_pointsY.push_back(currentPoint.y);
            }
            currentPoint += xDelta;
        }
        rowStartPoint += yDelta;
    }
}

void LC_Lattice::toGui(const LC_GraphicViewport* viewport) {
    const size_t numPoints = m_pointsX.size();
    for (size_t i = 0; i < numPoints; i++) {
        const double x = m_pointsX[i];
        const double uiX = viewport->toGuiX(x);
        m_pointsX[i] = uiX;

        const double y = m_pointsY[i];
        const double uiY = viewport->toGuiY(y);
        m_pointsY[i] = uiY;
    }
}

RS_Vector LC_Lattice::getOffset(const int xPointsDelta, const int yPointsDelta) const {
    const RS_Vector result = m_deltaX * xPointsDelta + m_deltaY * yPointsDelta;
    return result;
}

const RS_Vector& LC_Lattice::getMajorVector() const {
    return m_majorVector;
}

const RS_Vector& LC_Lattice::getDeltaX() const {
    return m_deltaX;
}

const RS_Vector& LC_Lattice::getDeltaY() const {
    return m_deltaY;
}

const std::vector<double>& LC_Lattice::getPointsX() const {
    return m_pointsX;
}

const std::vector<double>& LC_Lattice::getPointsY() const {
    return m_pointsY;
}

void LC_Lattice::addLine(const double startX, const double startY, const double endX, const double endY) {
    m_pointsX.push_back(startX);
    m_pointsX.push_back(endX);

    m_pointsY.push_back(startY);
    m_pointsY.push_back(endY);
}

void LC_Lattice::addPoint(const double x, const double y) {
    m_pointsX.push_back(x);
    m_pointsY.push_back(y);
}
