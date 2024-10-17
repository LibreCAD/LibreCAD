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

#include <QImageCleanupFunction>
#include <QActionEvent>
#include "lc_orthogonalgrid.h"
#include "rs_vector.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs.h"
#include "lc_gridsystem.h"


LC_OrthogonalGrid::LC_OrthogonalGrid(LC_GridSystem::LC_GridOptions *options):LC_GridSystem(options) {}

LC_OrthogonalGrid::~LC_OrthogonalGrid() = default;

void LC_OrthogonalGrid::prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) {

    // find meta grid boundaries

    projectMetaGridLinesAmount(metaGridCellSize);
    ensureAllMetaGridLinesInView(viewZero, viewSize);
    fillMetaGridCoordinates();

    determineGridBoundaries(viewZero, viewSize);
}

void LC_OrthogonalGrid::createCellVector(const RS_Vector &gridWidth) { cellVector = {fabs(gridWidth.x), fabs(gridWidth.y)}; }

void LC_OrthogonalGrid::createGridPoints([[maybe_unused]]const RS_Vector &min, [[maybe_unused]]const RS_Vector &max, const RS_Vector &gridWidth, bool drawGridWithoutGaps, int numPointsTotal) {
    gridLattice->update(0.0, 0.0, gridWidth, numPointsTotal);

    if (drawGridWithoutGaps) {
        gridLattice->fill(numPointsXTotal, numPointsYTotal, gridBasePoint, false, false);
    } else {
        fillPointsLatticeWithGapsForMetaGrid();
    }
}

void LC_OrthogonalGrid::determineGridBoundaries(const RS_Vector &viewZero,const RS_Vector &viewSize) {
    // find grid boundaries
    double gridX = gridCellSize.x;
    double gridY = gridCellSize.y;
    double left = truncToStep(viewZero.x, gridX);
    double right = truncToStep(viewSize.x, gridX);

    double top = truncToStep(viewZero.y, gridY);
    double bottom = truncToStep(viewSize.y, gridY);

    left -= gridX;
    right += gridX;
    top += gridY;
    bottom -= gridY;

    gridMin.x = left;
    gridMax.x = right;
    gridMin.y = bottom;
    gridMax.y = top;

//    cellV.set(fabs(gridX), fabs(gridY));

    gridBasePoint.set(left, bottom);
}

RS_Vector LC_OrthogonalGrid::snapGrid(const RS_Vector& coord) const {
    double gridX = gridCellSize.x;
    double gridY = gridCellSize.y;
    if(gridX < RS_TOLERANCE || gridY < RS_TOLERANCE) {
        return coord;
    }

    RS_Vector correctionVector;
    if (hasAxisIndefinite){
        if (indefiniteX){
            RS_Vector vp(coord-RS_Vector(0, gridBasePoint.y));
            correctionVector = RS_Vector(0, remainder(vp.y, gridY));
        }
        else{
            RS_Vector vp(coord-RS_Vector(gridBasePoint.x,0));
            correctionVector = RS_Vector(remainder(vp.x, gridX), 0);
        }
    }
    else{
        RS_Vector vp(coord-gridBasePoint);
        correctionVector = RS_Vector(remainder(vp.x, gridX), remainder(vp.y, gridY));
    }
    return coord - correctionVector;

}

void LC_OrthogonalGrid::fillPointsLatticeWithGapsForMetaGrid() {
    RS_Vector tileBasePoint;
    // if metagrid is visible, we generate points with gaps, so there is no overlap between metagrid lines and points
    // to achieve this, we'll generate points for each individual cell of metagrid

    std::vector<double> metaX;
    std::vector<double> metaY;

    // create meta grid coordinates arrays:
    if (numMetaX > 0) {
        metaX.resize(numMetaX);
        int i = 0;
        for (int x = 0; x < numMetaX; ++x) {
            metaX[i++] = metaGridMin.x + x * metaGridCellSize.x;
        }
    }
    if (numMetaY > 0) {
        int i = 0;
        metaY.resize(numMetaY);
        for (int y = 0; y < numMetaY; ++y) {
            metaY[i++] = metaGridMin.y + y * metaGridCellSize.y;
        }
    }

    // first fill by points fully visible metaGrid cells
    if (numMetaX > 0 && numMetaY > 0) {
        for (int mx = 0; mx < numMetaX - 1; ++mx) {
            tileBasePoint.x = metaX[mx];
            for (int my = 0; my < numMetaY - 1; ++my) {
                tileBasePoint.y = metaY[my];
                gridLattice->fill(numPointsInMetagridX, numPointsInMetagridY, tileBasePoint, false, false);
            }
        }
    }

    // fill top row
    tileBasePoint.y = metaGridMax.y;
    for (int mx = 0; mx < numMetaX - 1; ++mx) {
        tileBasePoint.x = metaX[mx];
        gridLattice->fill(numPointsInMetagridX, numPointsYTop, tileBasePoint, false, false);
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
    tileBasePoint.x = metaGridMax.x;
//    if (numMetaX == 0) { // no vid
//        tileBasePoint.x = gridBasePointIfMetagridNotVisible.x;
//    }
    for (int my = 0; my < numMetaY - 1; ++my) {
        tileBasePoint.y = metaY[my];
        gridLattice->fill(numPointsXRight, numPointsInMetagridY, tileBasePoint, false, false);
    }

    // fill right top corner
    tileBasePoint.y = metaGridMax.y;
//    if (numMetaY == 0) {
//        tileBasePoint.y = gridBasePointIfMetagridNotVisible.y;
//    }
    gridLattice->fill(numPointsXRight, numPointsYTop, tileBasePoint, false, false);

    // fill right bottom corner
    tileBasePoint.y = metaGridMin.y;
//    if (numMetaX == 0) { // no vid
//        tileBasePoint.x = gridBasePointIfMetagridNotVisible.x;
//    }
    gridLattice->fill(numPointsXRight, numPointsYBottom, tileBasePoint, false, true);


    // fill bottom row
    tileBasePoint.y = metaGridMin.y;
    for (int mx = 0; mx < numMetaX - 1; ++mx) {
        tileBasePoint.x = metaX[mx];
        gridLattice->fill(numPointsInMetagridX, numPointsYBottom, tileBasePoint, false, true);
    }

    // fill left column
    tileBasePoint.x = metaGridMin.x;
    for (int my = 0; my < numMetaY - 1; ++my) {
        tileBasePoint.y = metaY[my];
        gridLattice->fill(numPointsXLeft, numPointsInMetagridY, tileBasePoint, true, false);
    }

    // fill left bottom corner
    tileBasePoint.y = metaGridMin.y;
    gridLattice->fill(numPointsXLeft, numPointsYBottom, tileBasePoint, true, true);

    // fill left top corner

    tileBasePoint.y = metaGridMax.y;
    /*if (numMetaY == 0) {
        tileBasePoint.y = gridBasePointIfMetagridNotVisible.y;
    }*/
    gridLattice->fill(numPointsXLeft, numPointsYTop, tileBasePoint, true, false);
}

int LC_OrthogonalGrid::determineTotalPointsAmount(bool drawGridWithoutGaps) {
    if (drawGridWithoutGaps) {
        numPointsInMetagridX++;
        numPointsInMetagridY++;
        numPointsXTotal = (RS_Math::round((gridMax.x - gridMin.x) / gridCellSize.x) + 1);
        numPointsYTotal = (RS_Math::round((gridMax.y - gridMin.y) / gridCellSize.y) + 1);
    } else {
        numPointsXTotal = numPointsXLeft + numPointsInMetagridX * numMetaX + numPointsXRight;
        numPointsYTotal = numPointsYTop + numPointsInMetagridY * numMetaY + numPointsYBottom;
    }
    int result = numPointsYTotal * numPointsXTotal;
    return result;
}

void LC_OrthogonalGrid::determineGridPointsAmount(const RS_Vector &viewZero) {
    double gridX = gridCellSize.x;
    if (numMetaX > 0){
        numPointsInMetagridX = RS_Math::round(metaGridCellSize.x / gridX) - 1;
        numPointsXLeft = RS_Math::round((metaGridMin.x - gridMin.x) / gridX) - 1;
        numPointsXRight = RS_Math::round((gridMax.x - metaGridMax.x) / gridX) - 1;
    }
    else{ // no metaGrid lines visible on screen at all
        numPointsXRight = RS_Math::round((gridMax.x+metaGridCellSize.x  - gridMin.x) / gridX) - 1;
    }

    gridBasePointIfMetagridNotVisible.x = viewZero.x - remainder(metaGridViewOffset.x, gridX);

    double gridY = gridCellSize.y;
    if (numMetaY > 0) {
        numPointsInMetagridY = RS_Math::round(metaGridCellSize.y / gridY) - 1;
        numPointsYBottom = RS_Math::round((metaGridMin.y - gridMin.y) / gridY) - 1;
        numPointsYTop = RS_Math::round((gridMax.y - metaGridMax.y) / gridY) - 1;
    }
    else{
        numPointsYTop = RS_Math::round((gridMax.y + metaGridCellSize.y  - gridMin.y) / gridY) - 1;
    }
    gridBasePointIfMetagridNotVisible.y = gridMin.y - remainder(metaGridViewOffset.y, gridY);
}

void LC_OrthogonalGrid::projectMetaGridLinesAmount(const RS_Vector &metaGridWidth) {
    // make projection regarding the number of visible meta grid lines:
    numMetaX = RS_Math::round((metaGridMax.x - metaGridMin.x) / metaGridWidth.x) + 1;
    numMetaY = RS_Math::round((metaGridMax.y - metaGridMin.y) / metaGridWidth.y) + 1;
}

void LC_OrthogonalGrid::determineMetaGridBoundaries(
    const RS_Vector &viewZero, const RS_Vector &viewSize) {// determine coordinate (in graph coordinate system) for leftmost vertical metaGrid line
    metaGridMin.x = truncToStep(viewZero.x, metaGridCellSize.x);
    // determine rightmost coordinate (in graph coordinate system) for vertical metaGrid line
    metaGridMax.x = truncToStep(viewSize.x, metaGridCellSize.x);
    // determine topmost horizontal metaGridLine
    metaGridMax.y = truncToStep(viewZero.y, metaGridCellSize.y);
    // determine bottom horizontal metaGridLine
    metaGridMin.y = truncToStep(viewSize.y, metaGridCellSize.y);

    // determine offset from left top corner of view to start position of grid. That position may be outside of view
    // for very large zoom and small size of view. That offset is needed to handle the case where no metaGrid
    // lines (either vert or hor) are visible within view.
    metaGridViewOffset = RS_Vector(viewZero.x - metaGridMin.x, viewSize.y - metaGridMin.y);
}

/**
 * Check pre-calculated coordinates and ensures that metaGrid lines are not outside of view (so all they are visible and within boundaries of the
 * view or there there is not metagrid lines in
 * view at all)
 * @param viewZero
 * @param viewWidth
 * @param viewHeight
 */
void LC_OrthogonalGrid::ensureAllMetaGridLinesInView(const RS_Vector &viewZero, const RS_Vector &viewSize) {// check that leftmost line is visible in view


    double metaWidthX = metaGridCellSize.x;
    double metaWidthY = metaGridCellSize.y;

    if (viewZero.x > metaGridMin.x) {
        numMetaX--;
        metaGridMin.x += metaWidthX;
    }
    // check that rightmost line is visible
    double lastGridX = metaGridMin.x + metaWidthX * (numMetaX - 1);
    if (lastGridX > viewSize.x) {
        numMetaX--;
        metaGridMax.x -= metaWidthX;
    }

    // ensure that horizontal metaGrid lines are within view and so visible

    // check bottom line visible
    if (viewSize.y > metaGridMin.y) {
        numMetaY--;
        metaGridMin.y += metaWidthY;
    }

    // check top metaGrid line visible
    if (viewZero.y < metaGridMax.y) {
        numMetaY--;
        metaGridMax.y -= metaWidthY;
    }
}

void LC_OrthogonalGrid::fillMetaGridCoordinates() {
    metaGridMax.x = metaGridMin.x +  (numMetaX - 1) * metaGridCellSize.x;
    metaGridMax.y = metaGridMin.y +  (numMetaY - 1) * metaGridCellSize.y;
}

void LC_OrthogonalGrid::createMetaGridLines(const RS_Vector& min, const RS_Vector &max){

    metaGridLattice->update(0, 0, metaGridCellSize, numMetaX * numMetaY * 4);
    // draw vertical lines
    if (numMetaX > 0) {
        doCreateVerticalLines(metaGridLattice, min.y, max.y, metaGridMin.x, metaGridCellSize.x, numMetaX);
        metaGridLattice->addLine(metaGridMin.x, min.y, metaGridMin.x, max.y);
    }

    // draw horizontal line for orthogonal grid

    if (numMetaY > 0) {
        doCreateHorizontalLines(metaGridLattice, min.x, max.x, metaGridMin.y, metaGridCellSize.y, numMetaY);
        metaGridLattice->addLine(min.x, metaGridMin.y, max.x, metaGridMin.y);
    }
}

#define DEBUG_META_GRID_

void LC_OrthogonalGrid::drawMetaGridLines(RS_Painter *painter, RS_GraphicView *view) {

    // draw meta grid:
    doDrawLines(painter, view, metaGridLattice);

#ifdef DEBUG_META_GRID
    RS2::LineType penLineType = gridOptions->metaGridLineType;
    int metaGridLineWidthPx = gridOptions->metaGridLineWidthPx;
    double  uiFirstMX = view->toGuiX( metaGridMin.x);
    double uiFirstMY = view->toGuiY(metaGridMin.y);
    RS_Vector viewMetaGridCellSize = view->toGuiD(metaGridCellSize);
    if (numMetaX > 0) {
        const double mx0 = uiFirstMX;
        const double mxLast = uiFirstMX + (numMetaX - 1) * viewMetaGridCellSize.x;

        int height = view->height();
        if (numMetaX > 1) {

            painter->setPen({RS_Color(QColor("red")), RS2::Width01, penLineType}, metaGridLineWidthPx);

            painter->drawLine(RS_Vector(mx0, 0), RS_Vector(mx0, height));

            painter->setPen({RS_Color(QColor("blue")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(mxLast, 0), RS_Vector(mxLast, height));
        }
        else {
            painter->setPen({RS_Color(QColor("yellow")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(mx0, 0), RS_Vector(mx0, height));
        }
    }

    if (numMetaY > 0) {
        const double my0 = uiFirstMY;
        const double myLast = uiFirstMY - (numMetaY - 1) * viewMetaGridCellSize.y;

        int width = view->width();
        if (numMetaY > 1) {
            painter->setPen({RS_Color(QColor("red")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, my0), RS_Vector(width, my0));
            painter->setPen({RS_Color(QColor("blue")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, myLast), RS_Vector(width, myLast));
        }
        else{
            painter->setPen({RS_Color(QColor("yellow")), RS2::Width01, penLineType}, metaGridLineWidthPx);
            painter->drawLine(RS_Vector(0, my0), RS_Vector(width, my0));
        }
    }

    painter->setPen({RS_Color(QColor("cyan")), RS2::Width01, penLineType}, metaGridLineWidthPx);
    painter->drawRect(RS_Vector(uiFirstMX, uiFirstMY) - 5, RS_Vector(uiFirstMX, uiFirstMY) + 5);

#endif
}

void LC_OrthogonalGrid::createGridLinesWithoutGaps(const RS_Vector &min, const RS_Vector &max) {
    // draw vertical line

    double firstMX = 0;
    double firstMY = 0;

    firstMX = metaGridMin.x;
    firstMY = metaGridMin.y;
    auto metaGridWidthX = metaGridCellSize.x;
    auto metaGridWidthY = metaGridCellSize.y;

    bool noMetaGrid = !gridOptions->drawMetaGrid;

    int pointsInMetaGridX = noMetaGrid ? numPointsInMetagridX : (numPointsInMetagridX - 1);
    int pointsInMetaGridY = noMetaGrid ? numPointsInMetagridY : (numPointsInMetagridY - 1);
    
    // simpler mode for grid drawing - here we ensure that grid line is not drawn over metaGrid lines if they are present
    // however, grid line may intersect grid line

    gridLattice->updateForLines(0, 0, gridCellSize, RS_Vector(0, 0), numPointsYTotal * numPointsXTotal * 4);

    // draw vertical lines
    double minY = min.y;
    double maxY = max.y;

    double gridX = gridCellSize.x;
    createVerticalLines(minY, maxY, firstMX, -gridX, numPointsXLeft);

    double metaX = firstMX;
    for (int i = 0; i < numMetaX - 1; i++) {
        createVerticalLines(minY, maxY, metaX, gridX, pointsInMetaGridX);
        metaX += metaGridWidthX;
    }

    createVerticalLines(minY, maxY, metaX, gridX, numPointsXRight);

    if (noMetaGrid){
        gridLattice->addLine(firstMX, minY, firstMX, maxY);
    }
    // draw horizontal lines

    double minX = min.x;
    double maxX = max.x;
    double gridY = gridCellSize.y;
    createHorizontalLines(minX, maxX, firstMY, -gridY, numPointsYBottom);

    double metaY = firstMY;
    for (int i = 0; i < numMetaY - 1; i++) {
        createHorizontalLines(minX, maxX, metaY, gridY, pointsInMetaGridY);
        metaY += metaGridWidthY;
    }

    createHorizontalLines(minX, maxX, metaY, gridY, numPointsYTop);

    if (noMetaGrid){
        gridLattice->addLine(minX, firstMY, maxX, firstMY);
    }    
}

void LC_OrthogonalGrid::createGridLines(const RS_Vector& min, const RS_Vector &max, [[maybe_unused]]const RS_Vector& gridWidth, bool drawGridWithoutGaps, const RS_Vector& lineOffset) {
    if (drawGridWithoutGaps) {
        createGridLinesWithoutGaps(min, max);
    } else {
        createGridLinesWithGaps(min, max, lineOffset);
    }
}

void LC_OrthogonalGrid::createGridLinesWithGaps(const RS_Vector &min, const RS_Vector &max, const RS_Vector &lineOffset) {
    double height = max.y;
    double width = max.x;

    double minX = min.x;
    double minY = min.y;

    // draw vertical line

    double firstMX = 0;
    double firstMY = 0;

    firstMX = metaGridMin.x;
    firstMY = metaGridMin.y;
    auto metaWidthX = metaGridCellSize.x;
    auto metaWidthY = metaGridCellSize.y;

    bool noMetaGrid = !gridOptions->drawMetaGrid;

    int pointsInMetaGridX = noMetaGrid ? numPointsInMetagridX : (numPointsInMetagridX - 1);
    int pointsInMetaGridY = noMetaGrid ? numPointsInMetagridY : (numPointsInMetagridY - 1);

    // the most accurate yet complex drawing mode - here we ensure that
    // grid lines are not drawn in metaGrid positions - PLUS - there is no intersections between grid lines and metaGrid lines
    gridLattice->updateForLines(0, 0, gridCellSize, lineOffset, numPointsYTotal * numPointsXTotal * 4);

    double metaX = firstMX;
    double metaY = firstMY;
    double sizeDeltaX = lineOffset.x;
    double sizeDeltaY = lineOffset.y;

    double lastMY = firstMY + (numMetaY - 1) * metaWidthY;
    double lastMX = firstMX + (numMetaX - 1) * metaWidthX;

    double vStart, vEnd, hStart, hEnd;

    double startShiftY = sizeDeltaY;
    double endShiftY = - sizeDeltaY * 2;
    double endShiftYHalf = - sizeDeltaY;
    double startShiftX = sizeDeltaX;
    double endShiftX = - sizeDeltaX/* * 2*/;

    double gridX = gridCellSize.x;
    double gridY = gridCellSize.y;
    for (int i = 0; i < numMetaX - 1; i++) {
        vStart = firstMY + startShiftY; // shift between lines
        vEnd = vStart + metaWidthY + endShiftY;
        hStart = metaX + startShiftX; // shift on px
        hEnd = metaX + metaWidthX + endShiftX; // shift on px
        metaY = firstMY;

        for (int j = 0; j < numMetaY - 1; j++) {
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
    for (int i = 0; i < numMetaX - 1; i++) {
        hStart = metaX + startShiftX;
        hEnd = metaX + metaWidthX + endShiftX;

        createVerticalLines(vStart, vEnd, metaX, gridX, pointsInMetaGridX + 1);
        createHorizontalLines(hStart, hEnd, firstMY, -gridY, numPointsYBottom);

        metaX += metaWidthX;
    }

    // top row
    metaX = firstMX;
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    for (int i = 0; i < numMetaX - 1; i++) {
        hStart = metaX + startShiftX;
        hEnd = metaX + metaWidthX + endShiftX;

        createVerticalLines(vStart, vEnd, metaX, gridX, pointsInMetaGridX + 1);
        createHorizontalLines(hStart, hEnd, lastMY, gridY, numPointsYTop);

        metaX += metaWidthX;
    }
    // left column
    metaY = firstMY;
    hStart = minX/* + startShiftX*/;
    hEnd = firstMX + endShiftX;
    for (int i = 0; i < numMetaY - 1; i++) {
        vStart = metaY + startShiftY;
        vEnd = metaY + metaWidthY + endShiftYHalf;

        createVerticalLines(vStart, vEnd, firstMX, -gridX, numPointsXLeft);
        createHorizontalLines(hStart, hEnd, metaY, gridY, pointsInMetaGridY + 1);
        metaY += metaWidthY;
    }

    // right column
    metaY = firstMY;
    hStart = lastMX + startShiftX;
    hEnd = width/* + endShiftX*/;
    for (int i = 0; i < numMetaY - 1; i++) {
        vStart = metaY + startShiftY;
        vEnd = metaY + metaWidthY + endShiftYHalf;

        createVerticalLines(vStart, vEnd, lastMX, gridX, numPointsXRight);
        createHorizontalLines(hStart, hEnd, metaY, gridY, pointsInMetaGridY + 1);
        metaY += metaWidthY;
    }

    // left top corners horizontal
    hStart = minX/* + startShiftX*/;
    hEnd = firstMX + endShiftX;
    createHorizontalLines(hStart, hEnd, lastMY, gridY, numPointsYTop);

    // left bottom corners horizontal
    createHorizontalLines(hStart, hEnd, firstMY, -gridY, numPointsYBottom);

    // left top vertical
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    createVerticalLines(vStart, vEnd, firstMX, -gridX, numPointsXLeft);

    // left bottom vertical
    vStart = height/* + startShiftY*/;
    vEnd = firstMY + endShiftYHalf;
    createVerticalLines(vStart, vEnd, firstMX, -gridX, numPointsXLeft);

    //  right bottom vertical
    vStart = height/* + startShiftY*/;
    vEnd = firstMY + endShiftYHalf;
    hStart = lastMX + startShiftX;
    hEnd = width/* + endShiftX*/;
    createVerticalLines(vStart, vEnd, lastMX, gridX, numPointsXRight);
    createHorizontalLines(hStart, hEnd, firstMY, -gridY, numPointsYBottom);

    // right top vertical
    vStart = lastMY + startShiftY;
    vEnd = minY/* + endShiftYHalf*/;
    createVerticalLines(vStart, vEnd, lastMX, gridX, numPointsXRight);
    createHorizontalLines(hStart, hEnd, lastMY, gridY, numPointsYTop);
}

void LC_OrthogonalGrid::doCreateVerticalLines(LC_Lattice* lattice, const double &start, const double &end, const double &baseX, const double &delta, const int &pointsToDraw) const {
    double currentX = baseX + delta;
    for (int j= 0; j < pointsToDraw; j++) {
        lattice->addLine(currentX, start, currentX, end);
        currentX += delta;
    }
}

void LC_OrthogonalGrid::createVerticalLines(const double &start, const double &end, const double &baseX, const double &delta, const int &pointsToDraw) const {
    doCreateVerticalLines(gridLattice, start, end, baseX, delta, pointsToDraw);
}

void LC_OrthogonalGrid::doCreateHorizontalLines(LC_Lattice* lattice, const double &start, const double &end, const double &baseY, const double &delta, const int &pointsToDraw) const {
    double currentY = baseY + delta;
    for (int j= 0; j < pointsToDraw; j++) {
        lattice->addLine(start, currentY,  end, currentY);
        currentY += delta;
    }
}

void LC_OrthogonalGrid::createHorizontalLines(const double &start, const double &end, const double &baseY, const double &delta, const int &pointsToDraw) const {
    doCreateHorizontalLines(gridLattice, start, end, baseY, delta, pointsToDraw);
}
