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

#include "lc_orthogonalgrid.h"


#include "lc_lattice.h"
#include "lc_linemath.h"
#include "rs_math.h"

LC_OrthogonalGrid::LC_OrthogonalGrid(LC_GridOptions* options) : LC_GridSystem(options) {
}

LC_OrthogonalGrid::~LC_OrthogonalGrid() = default;

void LC_OrthogonalGrid::prepareGridOther(const RS_Vector& viewZero, const RS_Vector& viewSize) {
    // find meta grid boundaries

    projectMetaGridLinesAmount(m_metaGridCellSize);
    ensureAllMetaGridLinesInView(viewZero, viewSize);
    fillMetaGridCoordinates();

    determineGridBoundaries(viewZero, viewSize);
}

void LC_OrthogonalGrid::createCellVector(const RS_Vector& gridWidth) {
    m_cellVector = {fabs(gridWidth.x), fabs(gridWidth.y)};
}

void LC_OrthogonalGrid::createGridPoints([[maybe_unused]] const RS_Vector& min, [[maybe_unused]] const RS_Vector& max,
                                         const RS_Vector& gridWidth, const bool drawGridWithoutGaps, const int numPointsTotal) {
    m_gridLattice->update(0.0, 0.0, gridWidth, numPointsTotal);

    if (drawGridWithoutGaps) {
        m_gridLattice->fill(m_numPointsXTotal, m_numPointsYTotal, m_gridBasePoint, false, false);
    }
    else {
        fillPointsLatticeWithGapsForMetaGrid();
    }
}

void LC_OrthogonalGrid::determineGridBoundaries(const RS_Vector& viewZero, const RS_Vector& viewSize) {
    // find grid boundaries
    const double gridX = m_gridCellSize.x;
    const double gridY = m_gridCellSize.y;
    double left = truncToStep(viewZero.x, gridX);
    double right = truncToStep(viewSize.x, gridX);

    double top = truncToStep(viewZero.y, gridY);
    double bottom = truncToStep(viewSize.y, gridY);

    left -= gridX;
    right += gridX;
    top += gridY;
    bottom -= gridY;

    m_gridMin.x = left;
    m_gridMax.x = right;
    m_gridMin.y = bottom;
    m_gridMax.y = top;

    //    cellV.set(fabs(gridX), fabs(gridY));

    m_gridBasePoint.set(left, bottom);
}

RS_Vector LC_OrthogonalGrid::snapGrid(const RS_Vector& coord) const {
    const double gridX = m_gridCellSize.x;
    const double gridY = m_gridCellSize.y;
    if (gridX < RS_TOLERANCE || gridY < RS_TOLERANCE) {
        return coord;
    }

    RS_Vector correctionVector;
    if (m_hasAxisIndefinite) {
        if (m_indefiniteX) {
            const RS_Vector vp(coord - RS_Vector(0, m_gridBasePoint.y));
            correctionVector = RS_Vector(0, remainder(vp.y, gridY));
        }
        else {
            const RS_Vector vp(coord - RS_Vector(m_gridBasePoint.x, 0));
            correctionVector = RS_Vector(remainder(vp.x, gridX), 0);
        }
    }
    else {
        const RS_Vector vp(coord - m_gridBasePoint);
        correctionVector = RS_Vector(remainder(vp.x, gridX), remainder(vp.y, gridY));
    }
    return coord - correctionVector;
}

RS_Vector LC_OrthogonalGrid::snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) {
    const double gridX = m_gridCellSize.x;
    const double gridY = m_gridCellSize.y;
    if (gridX < RS_TOLERANCE || gridY < RS_TOLERANCE) {
        return coord;
    }
    // LC_ERR << " ****************************************************************************************************";
    // LC_ERR << " coord" << coord;
    // LC_ERR << " RayStart" << rayStart;
    // LC_ERR << " RayEnd" << rayEnd;
    // LC_ERR << " Base Point" << m_gridBasePoint;

    RS_Vector correctionVector;
    if (m_hasAxisIndefinite) {
        if (m_indefiniteX) {
            RS_Vector basepoint(0, m_gridBasePoint.y);
            const RS_Vector vp(coord - basepoint);
            correctionVector = RS_Vector(0, remainder(vp.y, gridY));
        }
        else {
            RS_Vector basepoint(m_gridBasePoint.x, 0);
            const RS_Vector vp(coord - basepoint);
            correctionVector = RS_Vector(remainder(vp.x, gridX), 0);
        }
    }
    else {
        const RS_Vector vp(coord - m_gridBasePoint);
        correctionVector = RS_Vector(remainder(vp.x, gridX), remainder(vp.y, gridY));
    }

    RS_Vector correctedPos = coord - correctionVector;
    // LC_ERR << " correctedPos " << correctedPos;

    double cellLeft;
    double cellRight;
    double cellTop;
    double cellBottom;
    double correctedX = correctedPos.x;
    if (correctedX > coord.x) {
        cellRight = correctedX;
        cellLeft = correctedX - gridX;
    }
    else {
        cellRight = correctedX + gridX;
        cellLeft = correctedX;
    }
    double correctedY = correctedPos.y;
    if (correctedY > coord.y) {
        cellTop = correctedY;
        cellBottom = correctedY - gridY;
    }
    else {
        cellTop = correctedY + gridY;
        cellBottom = correctedY;
    }

    RS_Vector leftBottom(cellLeft, cellBottom);
    RS_Vector leftTop(cellLeft, cellTop);
    RS_Vector rightBottom(cellRight, cellBottom);
    RS_Vector rightTop(cellRight, cellTop);

    // LC_ERR << " leftBottom " << leftBottom;
    // LC_ERR << " leftTop " << leftTop;
    // LC_ERR << " rightBottom " << rightBottom;
    // LC_ERR << " rightTop " << rightTop;

    RS_VectorSolutions corners({leftBottom, leftTop, rightBottom, rightTop});
    size_t closesIndex;
    double distanceToClosestCorner;
    RS_Vector closestCellCorner = corners.getClosest(coord, &distanceToClosestCorner, &closesIndex);

    // LC_ERR << " closestCellCorner " << closestCellCorner;
    // LC_ERR << " closesIndex " << closesIndex;

    const RS_Vector intersectionVert = LC_LineMath::getIntersectionLineLineFast(rayStart, rayEnd, closestCellCorner, RS_Vector(closestCellCorner.x, closestCellCorner.y+10));
    const RS_Vector intersectionHor = LC_LineMath::getIntersectionLineLineFast(rayStart, rayEnd, closestCellCorner, RS_Vector(closestCellCorner.x+10, closestCellCorner.y));

    double distanceIntersectionToCorner = RS_MAXDOUBLE;
    bool verticalIsCloser = false;
    if (intersectionVert.valid) {
        // LC_ERR << "intersectionVert.valid";
        distanceIntersectionToCorner = intersectionVert.distanceTo(/*closestCellCorner*/coord);
        verticalIsCloser = true;
    }
    if (intersectionHor.valid) {
        // LC_ERR << "intersectionHor.valid";
        double dist = intersectionHor.distanceTo(/*closestCellCorner*/coord);
        if (dist < distanceIntersectionToCorner) {
            verticalIsCloser = false;
        }
    }
    RS_Vector result(true);
    if (verticalIsCloser) {
        result.x = closestCellCorner.x;
        result.y = intersectionVert.y;
        // LC_ERR << " Vertical is closer ";
    }
    else {
        result.x = intersectionHor.x;
        result.y = closestCellCorner.y;
        // LC_ERR << " horizontal is closer ";
    }

    // LC_ERR << " result " << result;

    return result;
}

void LC_OrthogonalGrid::fillPointsLatticeWithGapsForMetaGrid() const {
    RS_Vector tileBasePoint;
    // if metagrid is visible, we generate points with gaps, so there is no overlap between metagrid lines and points
    // to achieve this, we'll generate points for each individual cell of metagrid

    std::vector<double> metaX; // fixme - sand - change to std::array
    std::vector<double> metaY;

    // create meta grid coordinates arrays:
    if (m_numMetaX > 0) {
        metaX.resize(m_numMetaX);
        int i = 0;
        for (int x = 0; x < m_numMetaX; ++x) {
            metaX[i++] = m_metaGridMin.x + (x * m_metaGridCellSize.x);
        }
    }
    if (m_numMetaY > 0) {
        size_t i = 0;
        metaY.resize(m_numMetaY);
        for (int y = 0; y < m_numMetaY; ++y) {
            metaY[i++] = m_metaGridMin.y + (y * m_metaGridCellSize.y);
        }
    }

    // first fill by points fully visible metaGrid cells
    if (m_numMetaX > 0 && m_numMetaY > 0) {
        for (int mx = 0; mx < m_numMetaX - 1; ++mx) {
            tileBasePoint.x = metaX[mx];
            for (int my = 0; my < m_numMetaY - 1; ++my) {
                tileBasePoint.y = metaY[my];
                m_gridLattice->fill(m_numPointsInMetagridX, m_numPointsInMetagridY, tileBasePoint, false, false);
            }
        }
    }

    // fill top row
    tileBasePoint.y = m_metaGridMax.y;
    for (int mx = 0; mx < m_numMetaX - 1; ++mx) {
        tileBasePoint.x = metaX[mx];
        m_gridLattice->fill(m_numPointsInMetagridX, m_numPointsYTop, tileBasePoint, false, false);
    }
    /*if (numMetaX > 1 && numMetaY == 0) {
        // very rare case - view height is so small, that no horizontal metaGrid lines in view - but there are vertical ones. Or, some irregular user grid
        // is used (with spacing by X smaller than spacing by Y) on large zoom level
        // to central tiles should be filled too
        tileBasePoint.y = gridBasePointIfMetagridNotVisible.y;
        for (int mx = 0; mx < numMetaX - 1; ++mx) {
            tileBasePoint.x = metaX[mx];
            gridLattice->fill(numPointsInMetagridX, numPointsYTop, tileBasePoint, false, false);
        }
    }*/

    // fill right column
    tileBasePoint.x = m_metaGridMax.x;
    //    if (numMetaX == 0) { // no vid
    //        tileBasePoint.x = gridBasePointIfMetagridNotVisible.x;
    //    }
    for (int my = 0; my < (m_numMetaY - 1); ++my) {
        tileBasePoint.y = metaY[my];
        m_gridLattice->fill(m_numPointsXRight, m_numPointsInMetagridY, tileBasePoint, false, false);
    }

    // fill right top corner
    tileBasePoint.y = m_metaGridMax.y;
    //    if (numMetaY == 0) {
    //        tileBasePoint.y = gridBasePointIfMetagridNotVisible.y;
    //    }
    m_gridLattice->fill(m_numPointsXRight, m_numPointsYTop, tileBasePoint, false, false);

    // fill right bottom corner
    tileBasePoint.y = m_metaGridMin.y;
    //    if (numMetaX == 0) { // no vid
    //        tileBasePoint.x = gridBasePointIfMetagridNotVisible.x;
    //    }
    m_gridLattice->fill(m_numPointsXRight, m_numPointsYBottom, tileBasePoint, false, true);

    // fill bottom row
    tileBasePoint.y = m_metaGridMin.y;
    for (int mx = 0; mx < m_numMetaX - 1; ++mx) {
        tileBasePoint.x = metaX[mx];
        m_gridLattice->fill(m_numPointsInMetagridX, m_numPointsYBottom, tileBasePoint, false, true);
    }

    // fill left column
    tileBasePoint.x = m_metaGridMin.x;
    for (int my = 0; my < m_numMetaY - 1; ++my) {
        tileBasePoint.y = metaY[my];
        m_gridLattice->fill(m_numPointsXLeft, m_numPointsInMetagridY, tileBasePoint, true, false);
    }

    // fill left bottom corner
    tileBasePoint.y = m_metaGridMin.y;
    m_gridLattice->fill(m_numPointsXLeft, m_numPointsYBottom, tileBasePoint, true, true);

    // fill left top corner

    tileBasePoint.y = m_metaGridMax.y;
    /*if (numMetaY == 0) {
        tileBasePoint.y = gridBasePointIfMetagridNotVisible.y;
    }*/
    m_gridLattice->fill(m_numPointsXLeft, m_numPointsYTop, tileBasePoint, true, false);
}

int LC_OrthogonalGrid::determineTotalPointsAmount(const bool drawGridWithoutGaps) {
    if (drawGridWithoutGaps) {
        m_numPointsInMetagridX++;
        m_numPointsInMetagridY++;
        m_numPointsXTotal = RS_Math::round((m_gridMax.x - m_gridMin.x) / m_gridCellSize.x) + 1;
        m_numPointsYTotal = RS_Math::round((m_gridMax.y - m_gridMin.y) / m_gridCellSize.y) + 1;
    }
    else {
        m_numPointsXTotal = m_numPointsXLeft + (m_numPointsInMetagridX * m_numMetaX) + m_numPointsXRight;
        m_numPointsYTotal = m_numPointsYTop + (m_numPointsInMetagridY * m_numMetaY) + m_numPointsYBottom;
    }
    const int result = m_numPointsYTotal * m_numPointsXTotal;
    return result;
}

void LC_OrthogonalGrid::determineGridPointsAmount(const RS_Vector& viewZero) {
    const double gridX = m_gridCellSize.x;
    if (m_numMetaX > 0) {
        m_numPointsInMetagridX = RS_Math::round(m_metaGridCellSize.x / gridX) - 1;
        m_numPointsXLeft = RS_Math::round((m_metaGridMin.x - m_gridMin.x) / gridX) - 1;
        m_numPointsXRight = RS_Math::round((m_gridMax.x - m_metaGridMax.x) / gridX) - 1;
    }
    else {
        // no metaGrid lines visible on screen at all
        m_numPointsXRight = RS_Math::round((m_gridMax.x + m_metaGridCellSize.x - m_gridMin.x) / gridX) - 1;
    }

    m_gridBasePointIfMetagridNotVisible.x = viewZero.x - remainder(m_metaGridViewOffset.x, gridX);

    const double gridY = m_gridCellSize.y;
    if (m_numMetaY > 0) {
        m_numPointsInMetagridY = RS_Math::round(m_metaGridCellSize.y / gridY) - 1;
        m_numPointsYBottom = RS_Math::round((m_metaGridMin.y - m_gridMin.y) / gridY) - 1;
        m_numPointsYTop = RS_Math::round((m_gridMax.y - m_metaGridMax.y) / gridY) - 1;
    }
    else {
        m_numPointsYTop = RS_Math::round((m_gridMax.y + m_metaGridCellSize.y - m_gridMin.y) / gridY) - 1;
    }
    m_gridBasePointIfMetagridNotVisible.y = m_gridMin.y - remainder(m_metaGridViewOffset.y, gridY);
}

void LC_OrthogonalGrid::projectMetaGridLinesAmount(const RS_Vector& metaGridWidth) {
    // make projection regarding the number of visible meta grid lines:
    m_numMetaX = RS_Math::round((m_metaGridMax.x - m_metaGridMin.x) / metaGridWidth.x) + 1;
    m_numMetaY = RS_Math::round((m_metaGridMax.y - m_metaGridMin.y) / metaGridWidth.y) + 1;
}

void LC_OrthogonalGrid::determineMetaGridBoundaries(const RS_Vector& viewZero, const RS_Vector& viewSize) {
    // determine coordinate (in graph coordinate system) for leftmost vertical metaGrid line
    m_metaGridMin.x = truncToStep(viewZero.x, m_metaGridCellSize.x);
    // determine rightmost coordinate (in graph coordinate system) for vertical metaGrid line
    m_metaGridMax.x = truncToStep(viewSize.x, m_metaGridCellSize.x);
    // determine topmost horizontal metaGridLine
    m_metaGridMax.y = truncToStep(viewZero.y, m_metaGridCellSize.y);
    // determine bottom horizontal metaGridLine
    m_metaGridMin.y = truncToStep(viewSize.y, m_metaGridCellSize.y);

    // determine offset from left top corner of view to start position of grid. That position may be outside of view
    // for very large zoom and small size of view. That offset is needed to handle the case where no metaGrid
    // lines (either vert or hor) are visible within view.
    m_metaGridViewOffset = RS_Vector(viewZero.x - m_metaGridMin.x, viewSize.y - m_metaGridMin.y);
}

/**
 * Check pre-calculated coordinates and ensures that metaGrid lines are not outside of view (so all they are visible and within boundaries of the
 * view or there is not metagrid lines in
 * view at all)
 * @param viewZero
 * @param viewSize
 */
void LC_OrthogonalGrid::ensureAllMetaGridLinesInView(const RS_Vector& viewZero, const RS_Vector& viewSize) {
    // check that leftmost line is visible in view

    const double metaWidthX = m_metaGridCellSize.x;
    const double metaWidthY = m_metaGridCellSize.y;

    if (viewZero.x > m_metaGridMin.x) {
        m_numMetaX--;
        m_metaGridMin.x += metaWidthX;
    }
    // check that rightmost line is visible
    const double lastGridX = m_metaGridMin.x + (metaWidthX * (m_numMetaX - 1));
    if (lastGridX > viewSize.x) {
        m_numMetaX--;
        m_metaGridMax.x -= metaWidthX;
    }

    // ensure that horizontal metaGrid lines are within view and so visible

    // check bottom line visible
    if (viewSize.y > m_metaGridMin.y) {
        m_numMetaY--;
        m_metaGridMin.y += metaWidthY;
    }

    // check top metaGrid line visible
    if (viewZero.y < m_metaGridMax.y) {
        m_numMetaY--;
        m_metaGridMax.y -= metaWidthY;
    }
}

void LC_OrthogonalGrid::fillMetaGridCoordinates() {
    m_metaGridMax.x = m_metaGridMin.x + ((m_numMetaX - 1) * m_metaGridCellSize.x);
    m_metaGridMax.y = m_metaGridMin.y + ((m_numMetaY - 1) * m_metaGridCellSize.y);
}

void LC_OrthogonalGrid::createMetaGridLines(const RS_Vector& min, const RS_Vector& max) {
    m_metaGridLattice->update(0, 0, m_metaGridCellSize, m_numMetaX * m_numMetaY * 4);
    // draw vertical lines
    if (m_numMetaX > 0) {
        doCreateVerticalLines(m_metaGridLattice.get(), min.y, max.y, m_metaGridMin.x, m_metaGridCellSize.x, m_numMetaX);
        m_metaGridLattice->addLine(m_metaGridMin.x, min.y, m_metaGridMin.x, max.y);
    }

    // draw horizontal line for orthogonal grid

    if (m_numMetaY > 0) {
        doCreateHorizontalLines(m_metaGridLattice.get(), min.x, max.x, m_metaGridMin.y, m_metaGridCellSize.y, m_numMetaY);
        m_metaGridLattice->addLine(min.x, m_metaGridMin.y, max.x, m_metaGridMin.y);
    }
}

#define DEBUG_META_GRID_

void LC_OrthogonalGrid::drawMetaGridLines(RS_Painter* painter, LC_GraphicViewport* view) {
    // draw meta grid:
    doDrawLines(painter, view, m_metaGridLattice.get());

#ifdef DEBUG_META_GRID
    RS2::LineType penLineType = m_gridOptions->metaGridLineType; int metaGridLineWidthPx = m_gridOptions->metaGridLineWidthPx; double
        uiFirstMX = view->toGuiX(m_metaGridMin.x); double uiFirstMY = view->toGuiY(m_metaGridMin.y); RS_Vector viewMetaGridCellSize = view->
        toGuiD(m_metaGridCellSize); if (m_numMetaX > 0) {
        const double mx0 = uiFirstMX;
        const double mxLast = uiFirstMX + (m_numMetaX - 1) * viewMetaGridCellSize.x;

        int height = view->height();
        if (m_numMetaX > 1) {
            painter->setPen({RS_Color(QColor("red")), RS2::Width01, penLineType}, metaGridLineWidthPx);

            painter->drawLine(RS_Vector(mx0, 0), RS_Vector(mx0, height));

            painter->setPen({RS_Color(QColor("blue")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(mxLast, 0), RS_Vector(mxLast, height));
        }
        else {
            painter->setPen({RS_Color(QColor("yellow")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(mx0, 0), RS_Vector(mx0, height));
        }
    } if (m_numMetaY > 0) {
        const double my0 = uiFirstMY;
        const double myLast = uiFirstMY - (m_numMetaY - 1) * viewMetaGridCellSize.y;

        int width = view->width();
        if (m_numMetaY > 1) {
            painter->setPen({RS_Color(QColor("red")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, my0), RS_Vector(width, my0));
            painter->setPen({RS_Color(QColor("blue")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, myLast), RS_Vector(width, myLast));
        }
        else {
            painter->setPen({RS_Color(QColor("yellow")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, my0), RS_Vector(width, my0));
        }
    } painter->setPen({RS_Color(QColor("cyan")), RS2::Width01, penLineType}, metaGridLineWidthPx); painter->drawRect(
        RS_Vector(uiFirstMX, uiFirstMY) - 5, RS_Vector(uiFirstMX, uiFirstMY) + 5);

#endif
}

void LC_OrthogonalGrid::createGridLinesWithoutGaps(const RS_Vector& min, const RS_Vector& max) const {
    // draw vertical line

    double firstMX = 0;
    double firstMY = 0;

    firstMX = m_metaGridMin.x;
    firstMY = m_metaGridMin.y;
    const auto metaGridWidthX = m_metaGridCellSize.x;
    const auto metaGridWidthY = m_metaGridCellSize.y;

    const bool noMetaGrid = !m_gridOptions->drawMetaGrid;

    const int pointsInMetaGridX = noMetaGrid ? m_numPointsInMetagridX : (m_numPointsInMetagridX - 1);
    const int pointsInMetaGridY = noMetaGrid ? m_numPointsInMetagridY : (m_numPointsInMetagridY - 1);

    // simpler mode for grid drawing - here we ensure that grid line is not drawn over metaGrid lines if they are present
    // however, grid line may intersect grid line

    m_gridLattice->updateForLines(0, 0, m_gridCellSize, RS_Vector(0, 0), m_numPointsYTotal * m_numPointsXTotal * 4);

    // draw vertical lines
    const double minY = min.y;
    const double maxY = max.y;

    const double gridX = m_gridCellSize.x;
    createVerticalLines(minY, maxY, firstMX, -gridX, m_numPointsXLeft);

    double metaX = firstMX;
    for (int i = 0; i < m_numMetaX - 1; i++) {
        createVerticalLines(minY, maxY, metaX, gridX, pointsInMetaGridX);
        metaX += metaGridWidthX;
    }

    createVerticalLines(minY, maxY, metaX, gridX, m_numPointsXRight);

    if (noMetaGrid) {
        m_gridLattice->addLine(firstMX, minY, firstMX, maxY);
    }
    // draw horizontal lines

    const double minX = min.x;
    const double maxX = max.x;
    const double gridY = m_gridCellSize.y;
    createHorizontalLines(minX, maxX, firstMY, -gridY, m_numPointsYBottom);

    double metaY = firstMY;
    for (int i = 0; i < m_numMetaY - 1; i++) {
        createHorizontalLines(minX, maxX, metaY, gridY, pointsInMetaGridY);
        metaY += metaGridWidthY;
    }

    createHorizontalLines(minX, maxX, metaY, gridY, m_numPointsYTop);

    if (noMetaGrid) {
        m_gridLattice->addLine(minX, firstMY, maxX, firstMY);
    }
}

void LC_OrthogonalGrid::createGridLines(const RS_Vector& min, const RS_Vector& max, [[maybe_unused]] const RS_Vector& gridWidth,
                                        const bool drawGridWithoutGaps, const RS_Vector& lineOffset) {
    if (drawGridWithoutGaps) {
        createGridLinesWithoutGaps(min, max);
    }
    else {
        createGridLinesWithGaps(min, max, lineOffset);
    }
}

void LC_OrthogonalGrid::createGridLinesWithGaps(const RS_Vector& min, const RS_Vector& max, const RS_Vector& lineOffset) const {
    const double height = max.y;
    const double width = max.x;

    const double minX = min.x;
    const double minY = min.y;

    // draw vertical line

    double firstMX = 0;
    double firstMY = 0;

    firstMX = m_metaGridMin.x;
    firstMY = m_metaGridMin.y;
    const auto metaWidthX = m_metaGridCellSize.x;
    const auto metaWidthY = m_metaGridCellSize.y;

    const bool noMetaGrid = !m_gridOptions->drawMetaGrid;

    const int pointsInMetaGridX = noMetaGrid ? m_numPointsInMetagridX : (m_numPointsInMetagridX - 1);
    const int pointsInMetaGridY = noMetaGrid ? m_numPointsInMetagridY : (m_numPointsInMetagridY - 1);

    // the most accurate yet complex drawing mode - here we ensure that
    // grid lines are not drawn in metaGrid positions - PLUS - there is no intersections between grid lines and metaGrid lines
    m_gridLattice->updateForLines(0, 0, m_gridCellSize, lineOffset, m_numPointsYTotal * m_numPointsXTotal * 4);

    double metaX = firstMX;

    const double sizeDeltaX = lineOffset.x;
    const double sizeDeltaY = lineOffset.y;

    const double lastMY = firstMY + ((m_numMetaY - 1) * metaWidthY);
    const double lastMX = firstMX + ((m_numMetaX - 1) * metaWidthX);

    double vStart, vEnd, hStart, hEnd;

    const double startShiftY = sizeDeltaY;
    const double endShiftY = -sizeDeltaY * 2;
    const double endShiftYHalf = -sizeDeltaY;
    const double startShiftX = sizeDeltaX;
    const double endShiftX = -sizeDeltaX/* * 2*/;

    const double gridX = m_gridCellSize.x;
    const double gridY = m_gridCellSize.y;
    for (int i = 0; i < m_numMetaX - 1; i++) {
        vStart = firstMY + startShiftY; // shift between lines
        vEnd = vStart + metaWidthY + endShiftY;
        hStart = metaX + startShiftX; // shift on px
        hEnd = metaX + metaWidthX + endShiftX; // shift on px
        double metaY = firstMY;

        for (int j = 0; j < m_numMetaY - 1; j++) {
            createVerticalLines(vStart, vEnd, metaX, gridX, pointsInMetaGridX + 1);
            createHorizontalLines(hStart, hEnd, metaY, gridY, pointsInMetaGridY + 1);
            vStart += metaWidthY;
            vEnd += metaWidthY;
            metaY += metaWidthY;
        }
        metaX += metaWidthX;
    }

    // bottom row
    metaX = firstMX;
    vStart = height/* + startShiftY*/;
    vEnd = firstMY + endShiftYHalf;
    for (int i = 0; i < m_numMetaX - 1; i++) {
        hStart = metaX + startShiftX;
        hEnd = metaX + metaWidthX + endShiftX;

        createVerticalLines(vStart, vEnd, metaX, gridX, pointsInMetaGridX + 1);
        createHorizontalLines(hStart, hEnd, firstMY, -gridY, m_numPointsYBottom);

        metaX += metaWidthX;
    }

    // top row
    metaX = firstMX;
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    for (int i = 0; i < m_numMetaX - 1; i++) {
        hStart = metaX + startShiftX;
        hEnd = metaX + metaWidthX + endShiftX;

        createVerticalLines(vStart, vEnd, metaX, gridX, pointsInMetaGridX + 1);
        createHorizontalLines(hStart, hEnd, lastMY, gridY, m_numPointsYTop);

        metaX += metaWidthX;
    }
    // left column
    double metaY = firstMY;
    hStart = minX/* + startShiftX*/;
    hEnd = firstMX + endShiftX;
    for (int i = 0; i < m_numMetaY - 1; i++) {
        vStart = metaY + startShiftY;
        vEnd = metaY + metaWidthY + endShiftYHalf;

        createVerticalLines(vStart, vEnd, firstMX, -gridX, m_numPointsXLeft);
        createHorizontalLines(hStart, hEnd, metaY, gridY, pointsInMetaGridY + 1);
        metaY += metaWidthY;
    }

    // right column
    metaY = firstMY;
    hStart = lastMX + startShiftX;
    hEnd = width/* + endShiftX*/;
    for (int i = 0; i < m_numMetaY - 1; i++) {
        vStart = metaY + startShiftY;
        vEnd = metaY + metaWidthY + endShiftYHalf;

        createVerticalLines(vStart, vEnd, lastMX, gridX, m_numPointsXRight);
        createHorizontalLines(hStart, hEnd, metaY, gridY, pointsInMetaGridY + 1);
        metaY += metaWidthY;
    }

    // left top corners horizontal
    hStart = minX/* + startShiftX*/;
    hEnd = firstMX + endShiftX;
    createHorizontalLines(hStart, hEnd, lastMY, gridY, m_numPointsYTop);

    // left bottom corners horizontal
    createHorizontalLines(hStart, hEnd, firstMY, -gridY, m_numPointsYBottom);

    // left top vertical
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    createVerticalLines(vStart, vEnd, firstMX, -gridX, m_numPointsXLeft);

    // left bottom vertical
    vStart = height/* + startShiftY*/;
    vEnd = firstMY + endShiftYHalf;
    createVerticalLines(vStart, vEnd, firstMX, -gridX, m_numPointsXLeft);

    //  right bottom vertical
    vStart = height/* + startShiftY*/;
    vEnd = firstMY + endShiftYHalf;
    hStart = lastMX + startShiftX;
    hEnd = width/* + endShiftX*/;
    createVerticalLines(vStart, vEnd, lastMX, gridX, m_numPointsXRight);
    createHorizontalLines(hStart, hEnd, firstMY, -gridY, m_numPointsYBottom);

    // right top vertical
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    createVerticalLines(vStart, vEnd, lastMX, gridX, m_numPointsXRight);
    createHorizontalLines(hStart, hEnd, lastMY, gridY, m_numPointsYTop);
}

void LC_OrthogonalGrid::doCreateVerticalLines(LC_Lattice* lattice, const double start, const double end, const double baseX,
                                              const double delta, const int pointsToDraw) const {
    double currentX = baseX + delta;
    for (int j = 0; j < pointsToDraw; j++) {
        lattice->addLine(currentX, start, currentX, end);
        currentX += delta;
    }
}

void LC_OrthogonalGrid::createVerticalLines(const double start, const double end, const double baseX, const double delta,
                                            const int pointsToDraw) const {
    doCreateVerticalLines(m_gridLattice.get(), start, end, baseX, delta, pointsToDraw);
}

void LC_OrthogonalGrid::doCreateHorizontalLines(LC_Lattice* lattice, const double start, const double end, const double baseY,
                                                const double delta, const int pointsToDraw) const {
    double currentY = baseY + delta;
    for (int j = 0; j < pointsToDraw; j++) {
        lattice->addLine(start, currentY, end, currentY);
        currentY += delta;
    }
}

void LC_OrthogonalGrid::createHorizontalLines(const double start, const double end, const double baseY, const double delta,
                                              const int pointsToDraw) const {
    doCreateHorizontalLines(m_gridLattice.get(), start, end, baseY, delta, pointsToDraw);
}
