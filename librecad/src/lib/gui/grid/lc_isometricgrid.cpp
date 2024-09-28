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

#include "lc_isometricgrid.h"
#include "rs_graphicview.h"
#include "rs_math.h"

LC_IsometricGrid::LC_IsometricGrid(LC_GridSystem::LC_GridOptions *options, int iso_projection):LC_GridSystem(options) {
    if (iso_projection >= ISO_LEFT && iso_projection < ISO_LAST) {
        projection = iso_projection;
    } else {
        projection = ISO_TOP;
    }

    drawRightLine = projection == ISO_TOP || projection == ISO_RIGHT;
    drawLeftLine = projection == ISO_TOP || projection == ISO_LEFT;
    drawTopLines = projection == ISO_RIGHT || projection == ISO_LEFT || gridOptions->drawIsometricVerticalsAlways;

    gridLattice = new LC_Lattice();
}

void LC_IsometricGrid::setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) {
    // for isometric grid, we ignore irregular grid (so far, may be latter better support of them will be added).
    // and thus we use only Y coordinate
    double gridY = gridWidth.y;
    if (gridY != gridWidth.x){
        gridCellSize = RS_Vector(gridY, gridY);
        metaGridCellSize = RS_Vector(metaGridWidth.y, metaGridWidth.y);
    }
    else {
        LC_GridSystem::setCellSize(gridWidth, metaGridWidth);
    }
}

RS_Vector LC_IsometricGrid::snapGrid(const RS_Vector &coord) const {
    RS_Vector normalizedPosition(coord - gridBasePoint);

    //use remainder instead of fmod to locate the left-bottom corner for both positive and negative displacement

    double xInSnapCell = remainder(normalizedPosition.x, 2 * gridDeltaX.x);
    double yInSnapCell = remainder(normalizedPosition.y, 2 * gridDeltaY.y);
    RS_Vector positionInSnapCell(xInSnapCell, yInSnapCell);

    RS_Vector foundCellPoint = snapVectorSolution.getClosest(positionInSnapCell);

    RS_Vector result = gridBasePoint + foundCellPoint + normalizedPosition - positionInSnapCell;
    return result;
}

void LC_IsometricGrid::prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) {
    double angle30Deg = M_PI / 6;
    double deltaY = viewSize.y - viewZero.y;
    double offsetY = viewSize.y - metaGridMin.y;
    double offsetX = viewZero.x - metaGridMin.x;

//    if (deltaX / space.x > 1e3 || deltaY / space.y > 1e3) {
//        return;
//    }

    double tan30Deg = tan(angle30Deg);
    double dxLeft = fabs(offsetY / tan30Deg);
    double dxRight = fabs(deltaY / tan30Deg) - dxLeft;

    gridDeltaLeft = dxLeft;
    gridDeltaRight = dxRight;

    RS_Vector viewMaxPoint = RS_Vector(viewSize.x, viewZero.y);
    const RS_Vector metaGridViewOffset = RS_Vector(offsetX, offsetY);

    gridBasePoint = metaGridMin;

    numPointsInMetagridX = RS_Math::round(metaGridCellSize.x / gridCellSize.x) - 1;
    numPointsInMetagridY = RS_Math::round(metaGridCellSize.y / gridCellSize.y) - 1;


    calculateTilesGridMetrics(viewMaxPoint, metaGridViewOffset);
    gridLattice->update(30, 60, gridCellSize, 0);
    prepareSnapSolution();
}

void LC_IsometricGrid::determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) {
    isometricCell = RS_Vector(metaGridCellSize.x * 2, 0);
    double angle30Deg = M_PI / 6;
    isometricCell.rotate(angle30Deg);
    double metaHorizontalX = isometricCell.x;
    double metaVerticalY = isometricCell.y;


    metaGridMin.x = truncToStep(viewZero.x, metaHorizontalX);
    metaGridMax.x = truncToStep(viewSize.x, metaHorizontalX);
    metaGridMax.y = truncToStep(viewZero.y, metaVerticalY);
    metaGridMin.y = truncToStep(viewSize.y, metaVerticalY);

    if (metaGridMin.x < viewZero.x) {
        metaGridMin.x += metaHorizontalX;
    }
    if (metaGridMax.x > viewSize.x) {
        metaGridMax.x -= metaHorizontalX;
    }
    if (viewSize.y > metaGridMin.y) {
        metaGridMin.y += metaVerticalY;
    }
    if (metaGridMax.y > viewZero.y) {
        metaGridMax.y -= metaVerticalY;
    }

    metaGridViewOffset = RS_Vector(-metaHorizontalX, +metaVerticalY);
}

void LC_IsometricGrid::createCellVector(const RS_Vector &gridWidth) {
    cellVector.set(sqrt(3.) * gridWidth.y, fabs(gridWidth.y)); // fixme - sqrt(3) - is it approximation there?
}

void LC_IsometricGrid::determineGridPointsAmount([[maybe_unused]]const RS_Vector &vector) {
}

int LC_IsometricGrid::determineTotalPointsAmount([[maybe_unused]]bool drawGridWithoutGaps) {
    double metaHorizontalX = isometricCell.x;
    double metaVerticalY = isometricCell.y;

    int numTilesByX = (int) ((metaGridMax.x - metaGridMin.x) / metaHorizontalX) + 2;
    int numTilesByY = (int) ((metaGridMax.y - metaGridMin.y) / metaVerticalY) + 2;


    int numPointsTotal = numTilesByX * numTilesByY * numPointsInMetagridX * numPointsInMetagridY * 4;
    return numPointsTotal;
}

void LC_IsometricGrid::createGridLinesNoGaps(const RS_Vector &min, const RS_Vector &max) {

    double minX = min.x;
    double maxX = max.x;
    double minY = min.y;
    double maxY = max.y;


    double height = maxY;
    double width = maxX;
    double uiGridDeltaLeft = gridDeltaLeft;
    double uiGridDeltaRight = gridDeltaRight;

    double metaHorizontalX = isometricCell.x;
    double metaGridHalfWidthX = metaHorizontalX / 2;

    // just project a rough amount of lines will be drawn and so amount of points in lattice to reserve
    int numLinesX = width/metaGridHalfWidthX * 2;
    int numPointsProjection = numLinesX * numLinesX * 2;

    gridLattice->update(30, 60, gridCellSize, numPointsProjection);

    RS_Vector deltaX = gridLattice->getDeltaX() * 2;

    double distanceToMetaGridTolerance = deltaX.x * 0.9;

    if (drawTopLines){
        double halfMetaX = metaHorizontalX/2;
        double nextMetaX = metaGridMin.x + halfMetaX;
        double gridByX = deltaX.x/2;
        double currentX = metaGridMin.x + gridByX;
        double verticalTolerance = distanceToMetaGridTolerance /2;
        while (true){
            if (currentX > width) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > verticalTolerance) {
                gridLattice->addLine(currentX, minY, currentX, height);
                currentX += gridByX;
            }
            else{
                nextMetaX += halfMetaX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX, minY, currentX, height);
                }
                currentX += gridByX;
            }
        }
        // draw from minX to left
        nextMetaX = metaGridMin.x - halfMetaX;
        currentX = metaGridMin.x - gridByX;
        while (true) {
            double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < minX) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > verticalTolerance) {
                gridLattice->addLine(currentX, minY, currentX, height);
                currentX -= gridByX;
            } else {
                nextMetaX -= halfMetaX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX, minY, currentX, height);
                }
                currentX -= gridByX;
            }
        }

        if (!gridOptions->drawMetaGrid){
            gridLattice->addLine(metaGridMin.x, minY, metaGridMin.x, height);
        }
    }

    if (drawLeftLine) {
//  draw from minx to right
        double nextMetaX = metaGridMin.x + metaHorizontalX;
        double currentX = metaGridMin.x + deltaX.x;
        while (true) {
            double leftPoint = currentX - uiGridDeltaRight;
            if (leftPoint > width) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                currentX += deltaX.x;
            } else {
                nextMetaX += metaHorizontalX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                }
                currentX += deltaX.x;
            }
        }

        nextMetaX = metaGridMin.x - metaHorizontalX;
        currentX = metaGridMin.x - deltaX.x;

        // draw from minX to left
        while (true) {
            double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < minX) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                currentX -= deltaX.x;
            } else {
                nextMetaX -= metaHorizontalX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                }
                currentX -= deltaX.x;
            }
        }

        if (!gridOptions->drawMetaGrid){
            gridLattice->addLine(metaGridMin.x + uiGridDeltaLeft, height, metaGridMin.x - uiGridDeltaRight, minY);
        }
    }

    if (drawRightLine) {
//  draw from minx to right
        double nextMetaX = metaGridMin.x + metaHorizontalX;
        double currentX = metaGridMin.x + deltaX.x;
        while (true) {
            double leftPoint = currentX - uiGridDeltaLeft;
            if (leftPoint > width) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                currentX += deltaX.x;
            } else {
                nextMetaX += metaHorizontalX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                }
                currentX += deltaX.x;
            }
        }

        nextMetaX = metaGridMin.x - metaHorizontalX;
        currentX = metaGridMin.x - deltaX.x;

        // draw from minX to left
        while (true) {
            double rightPoint = currentX + uiGridDeltaRight;
            if (rightPoint < minX) {
                break;
            }
            double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                currentX -= deltaX.x;
            } else {
                nextMetaX -= metaHorizontalX;
                if (!gridOptions->drawMetaGrid){
                    gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                }
                currentX -= deltaX.x;
            }
        }

        if (!gridOptions->drawMetaGrid){
            gridLattice->addLine(metaGridMin.x - uiGridDeltaLeft, height, metaGridMin.x + uiGridDeltaRight, minY);
        }
    }
}


void LC_IsometricGrid::createMetaGridLines(const RS_Vector& min, const RS_Vector &max) {

    double x;

    double minX = min.x;
    double maxX = max.x;
    double minY = min.y;
    double maxY = max.y;


    double height = maxY;
    double width = maxX;
    double uiGridDeltaLeft = gridDeltaLeft;
    double uiGridDeltaRight = gridDeltaRight;

    double metaHorizontalX = isometricCell.x;
    double metaGridHalfWidthX = metaHorizontalX / 2;

    // just project a rough amount of lines will be drawn and so amount of points in lattice to reserve
    int numLinesX = (width - min.x)/metaGridHalfWidthX * 2;
    int numPointsProjection = numLinesX * numLinesX * 2;

    metaGridLattice->update(30, 60, metaGridCellSize, numPointsProjection);


    for (x = metaGridMin.x; x < width; x += metaHorizontalX) {
        if (drawRightLine) {
            metaGridLattice->addLine(x - uiGridDeltaLeft, height, x + uiGridDeltaRight, minY);
        }

        if (drawLeftLine) {
            metaGridLattice->addLine(x + uiGridDeltaLeft, height, x - uiGridDeltaRight, minY);
        }

        // vertical grid lines:qc
        if (drawTopLines) {
            double halfX = x - metaGridHalfWidthX;
            metaGridLattice->addLine(x, minY, x, height);
            metaGridLattice->addLine(halfX, minY, halfX, height);
        }
    }

    double lastX = x;
    if (drawTopLines) {
        // draw rightmost possible vertical line
        x -= metaGridHalfWidthX;
        if (x < width) {
            metaGridLattice->addLine(x, minY, x, height);
        }
    }

    if (drawRightLine) {
        // paint left top corner
        double currentX = metaGridMin.x - metaHorizontalX;
        while (true) {
            double rightPoint = currentX + uiGridDeltaRight;
            if (rightPoint < minX) {
                break;
            }
            metaGridLattice->addLine(currentX - uiGridDeltaLeft, height, rightPoint, minY);
            currentX -= metaHorizontalX;
        }

        // paint left bottom corner
        currentX = lastX;
        while (true) {
            double leftPoint = currentX - uiGridDeltaLeft;
            if (leftPoint > width) {
                break;
            }
            metaGridLattice->addLine(leftPoint, height, currentX + uiGridDeltaRight, minY);
            currentX += metaHorizontalX;
        }
    }

    if (drawLeftLine) {
//  draw right top corner
        double currentX = lastX;
        while (true) {
            double leftPoint = currentX - uiGridDeltaRight;
            if (leftPoint > width) {
                break;
            }
            metaGridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
            currentX += metaHorizontalX;
        }

// fill left bottom corner
        currentX = metaGridMin.x;
        while (true) {
            double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < 0) {
                break;
            }
            metaGridLattice->addLine(rightPoint, height, currentX - uiGridDeltaRight, minY);
            currentX -= metaHorizontalX;
        }
    }
}

void LC_IsometricGrid::createGridLines(const RS_Vector &min, const RS_Vector &max, const RS_Vector &gridWidth,  bool drawGridWithoutGaps, const RS_Vector& lineInTileOffset) {
    int linesCount = numPointsInMetagridX + numPointsInMetagridY;
    if (drawGridWithoutGaps){
        createGridLinesNoGaps(min, max);
    }
    else{
        if (drawTopLines){
            linesCount = linesCount * 4;
        }
        gridLattice->updateForLines(30, 60, gridWidth, lineInTileOffset, linesCount * 2);
        if (drawTopLines){
            fillTilesRowsByLinesNoDiagonals();
        }
        else{
            fillTilesRowsBylines();
        }
    }
}
#define DEBUG_ISO_META_

#ifdef DEBUG_ISO_META
#include "rs_pen.h"
#include "lc_defaults.h"
#endif

void LC_IsometricGrid::drawMetaGridLines(RS_Painter *painter, RS_GraphicView *view) {
    doDrawLines(painter, view, metaGridLattice);

#ifdef DEBUG_ISO_META

    double firstX = view->toGuiX(metaGridMin.x);

    double uiGridDeltaLeft = view->toGuiDX(gridDeltaLeft);
    double uiGridDeltaRight = view->toGuiDX(gridDeltaRight);

    painter->setPen({RS_Color(QColor("blue")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    if (drawRightLine) {
        painter->drawLine(firstX - uiGridDeltaLeft, view->getHeight(), firstX + uiGridDeltaRight, 0);
    }

    if (drawLeftLine) {
        painter->drawLine(firstX + uiGridDeltaLeft, view->getHeight(), firstX - uiGridDeltaRight, 0);
    }

    painter->setPen({RS_Color(QColor("yellow")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    painter->drawLine(view->toGui(metaGridMin), view->toGui(metaGridMax));
    painter->drawLine(view->toGui(metaGridMin), view->toGui(metaGridMax.x, metaGridMin.y));

    painter->setPen({RS_Color(QColor("cyan")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    painter->drawPoint(view->toGui(gridBasePoint), LC_DEFAULTS_PDMode, LC_DEFAULTS_PDSize);
#endif
}

void LC_IsometricGrid::createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth,  bool drawGridWithoutGaps, int totalPoints) {
    gridLattice->update(30, 60, gridWidth, totalPoints);
    if (drawGridWithoutGaps){
        fillPointsNoGaps(min, max);
    }
    else {
        if (drawTopLines) {
            fillTilesRowsByPointsExceptDiagonal();
        } else {
            fillTilesRowsByPoints();
        }
    }
}

void LC_IsometricGrid::prepareSnapSolution() {
    gridDeltaX = gridLattice->getDeltaX();
    gridDeltaY = gridLattice->getDeltaY();
    snapVectorSolution = RS_VectorSolutions({RS_Vector(0,0), gridDeltaY, gridDeltaX, -gridDeltaX, -gridDeltaY});
}

void LC_IsometricGrid::calculateTilesGridMetrics(const RS_Vector &maxCorner, const RS_Vector &offset) {
    double metaHorizontalX = isometricCell.x / 2;
    double metaVerticalY = isometricCell.y / 2;
    double minX = metaGridMin.x - isometricCell.x;
    double minY = metaGridMin.y - isometricCell.y;

    tileNumPointsByX = numPointsInMetagridX;
    tileNumPointsByY = numPointsInMetagridY;

    // fixme - check size of lines that are actually drawn
    if (!drawRightLine) {
        tileNumPointsByY++;
    }

    if (!drawLeftLine) {
        tileNumPointsByX++;
    }

    tilesRowShift = metaHorizontalX / 2;
    bool firstRowIsNearToMin = std::abs(offset.y) < metaVerticalY;
    if (firstRowIsNearToMin) {
        double shift = metaHorizontalX;
        if (metaHorizontalX > offset.x) {
            shift = -metaHorizontalX;
        }
        tilesStartPoint = RS_Vector(minX + shift - tilesRowShift, minY);
    } else {
        double shift = 0;
        if (metaHorizontalX > offset.x) {
            shift =  metaHorizontalX;
        }
        tilesStartPoint = RS_Vector(minX + shift - metaHorizontalX * 2 + tilesRowShift, minY - metaVerticalY);
    }
    tilesMaxPoint = RS_Vector(maxCorner.x + metaHorizontalX, maxCorner.y);

    tilesDelta = RS_Vector(isometricCell.x, metaVerticalY);
}


void LC_IsometricGrid::fillTilesRowsByPointsExceptDiagonal() {
    RS_Vector startPoint = tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    double maxX = tilesMaxPoint.x;
    double maxY = tilesMaxPoint.y;
    double rowShift = tilesRowShift;

    double tilesDeltaX = tilesDelta.x;
    double tilesDeltaY = tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            gridLattice->fillWithoutDiagonal(tileNumPointsByX, tileNumPointsByY, basePoint, false, false, true, numPointsInMetagridX);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;        
    }
}

void LC_IsometricGrid::fillTilesRowsByPoints() {
    RS_Vector startPoint = tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    double maxX = tilesMaxPoint.x;
    double maxY = tilesMaxPoint.y;
    double rowShift = tilesRowShift;
    double tilesDeltaX = tilesDelta.x;
    double tilesDeltaY = tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            gridLattice->fill(tileNumPointsByX, tileNumPointsByY, basePoint, false, false);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;    
    }
}

void LC_IsometricGrid::fillTilesRowsByLinesNoDiagonals() {
    RS_Vector startPoint = tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    double maxX = tilesMaxPoint.x;
    double maxY = tilesMaxPoint.y;
    double rowShift = tilesRowShift;

    double tilesDeltaX = tilesDelta.x;
    double tilesDeltaY = tilesDelta.y;

    int numPointsX = tileNumPointsByX;
    int numPointsY = tileNumPointsByY;

    if (!drawRightLine){
        numPointsY --;
    }
    if (!drawLeftLine){
        numPointsX --;
    }

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            gridLattice->fillAllByLinesExceptDiagonal(numPointsX, numPointsY, basePoint, false, false, !drawLeftLine, !drawRightLine);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;        
    }
}

void LC_IsometricGrid::fillTilesRowsBylines() {
    RS_Vector startPoint = tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    double maxX = tilesMaxPoint.x;
    double maxY = tilesMaxPoint.y;
    double rowShift = tilesRowShift;

    double tilesDeltaX = tilesDelta.x;
    double tilesDeltaY = tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            gridLattice->fillByLines(tileNumPointsByX, tileNumPointsByY, basePoint, false, false, !drawLeftLine, !drawRightLine);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;    
    }
}

void LC_IsometricGrid::fillPointsNoGaps(const RS_Vector &min, const RS_Vector &max) {
    RS_Vector deltaX = gridLattice->getDeltaX();
    RS_Vector deltaY = gridLattice->getDeltaY();
    double pointsDeltaX = deltaX.x;
    double pointsDeltaY = deltaY.y;

    RS_Vector leftBottomCorner = RS_Vector(min.x, max.y);

    RS_Vector offset = leftBottomCorner-metaGridMin;

    double startX =  min.x - remainder(offset.x, pointsDeltaX*2)+pointsDeltaX/2;
    double startY =  max.y - remainder(offset.y, pointsDeltaY*2)+pointsDeltaY;

    double maxX = max.x;    

    double shiftByX = pointsDeltaX*2;
    double rowShift = deltaY.x/2;
    double currentY = startY;
    while (currentY < min.y) {
        double currentX = startX - rowShift;
        while (currentX < maxX) {
            gridLattice->addPoint(currentX, currentY);
            currentX += shiftByX;
        }
        currentY += pointsDeltaY;
        rowShift = -rowShift;    
    }
}
