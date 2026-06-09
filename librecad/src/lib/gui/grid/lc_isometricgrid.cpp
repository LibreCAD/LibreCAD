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

#include "lc_lattice.h"
#include "lc_linemath.h"
#include "rs_math.h"

LC_IsometricGrid::LC_IsometricGrid(LC_GridOptions *options, const int isoProjection):LC_GridSystem(options) {
    if (isoProjection >= ISO_LEFT && isoProjection < ISO_LAST) {
        m_projection = isoProjection;
    } else {
        m_projection = ISO_TOP;
    }

    m_drawRightLine = m_projection == ISO_TOP || m_projection == ISO_RIGHT;
    m_drawLeftLine = m_projection == ISO_TOP || m_projection == ISO_LEFT;
    m_drawTopLines = m_projection == ISO_RIGHT || m_projection == ISO_LEFT || m_gridOptions->drawIsometricVerticalsAlways;

    m_gridLattice = std::make_unique<LC_Lattice>();
}

void LC_IsometricGrid::setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) {
    // for isometric grid, we ignore irregular grid (so far, may be latter better support of them will be added).
    // and thus we use only Y coordinate
    const double gridY = gridWidth.y;
    if (!LC_LineMath::isSameLength(gridY,gridWidth.x)){
        m_gridCellSize = RS_Vector(gridY, gridY);
        m_metaGridCellSize = RS_Vector(metaGridWidth.y, metaGridWidth.y);
    }
    else {
        LC_GridSystem::setCellSize(gridWidth, metaGridWidth);
    }
}

RS_Vector LC_IsometricGrid::snapGrid(const RS_Vector &coord) const {
    const RS_Vector normalizedPosition(coord - m_gridBasePoint);

    //use remainder instead of fmod to locate the left-bottom corner for both positive and negative displacement

    const double xInSnapCell = remainder(normalizedPosition.x, 2 * m_gridDeltaX.x);
    const double yInSnapCell = remainder(normalizedPosition.y, 2 * m_gridDeltaY.y);
    const RS_Vector positionInSnapCell(xInSnapCell, yInSnapCell);

    const RS_Vector foundCellPoint = m_snapVectorSolution.getClosest(positionInSnapCell);

    const RS_Vector result = m_gridBasePoint + foundCellPoint + normalizedPosition - positionInSnapCell;
    return result;
}

RS_Vector LC_IsometricGrid::snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) {
    const RS_Vector normalizedPosition(coord - m_gridBasePoint);

    // LC_ERR << " RayStart" << rayStart;
    // LC_ERR << " RayEnd" << rayEnd;

    //use remainder instead of fmod to locate the left-bottom corner for both positive and negative displacement

    const double xInSnapCell = remainder(normalizedPosition.x, 2 * m_gridDeltaX.x);
    const double yInSnapCell = remainder(normalizedPosition.y, 2 * m_gridDeltaY.y);

    const RS_Vector positionInSnapCell(xInSnapCell, yInSnapCell);

    // LC_ERR << " Position in cell" << positionInSnapCell;

    // LC_ERR << " CELL1" << m_snapVectorSolution.at(1);
    // LC_ERR << " CELL2" << m_snapVectorSolution.at(2);
    // LC_ERR << " CELL3" << m_snapVectorSolution.at(3);
    // LC_ERR << " CELL4" << m_snapVectorSolution.at(4);

    double dist;
    size_t index;
    const RS_Vector closestCellpoint = m_snapVectorSolution.getClosest(positionInSnapCell, &dist, &index);

    // LC_ERR << " Closest" << closestCellpoint;

    const RS_Vector cornerGlobal =  m_gridBasePoint + closestCellpoint + normalizedPosition - positionInSnapCell;

    // LC_ERR << " Global" << cornerGlobal;


    const RS_Vector intersectionVert = LC_LineMath::getIntersectionLineLineFast(rayStart, rayEnd, cornerGlobal, RS_Vector(cornerGlobal.x, cornerGlobal.y+10));
    const RS_Vector intersectionHor = LC_LineMath::getIntersectionLineLineFast(rayStart, rayEnd, cornerGlobal, RS_Vector(cornerGlobal.x+10, cornerGlobal.y));

    // LC_ERR << " intersectionVert" << intersectionVert;
    // LC_ERR << " intersectionHor" << intersectionHor;

    double minDist = RS_MAXDOUBLE;

    RS_Vector minIntersectionPoint{false};
    if (intersectionVert.valid) {
       dist = intersectionVert.distanceTo(cornerGlobal);
       minDist = dist;
       minIntersectionPoint = intersectionVert;
        // LC_ERR << " min intersectionVert";
    }

    if (intersectionHor.valid) {
        dist = intersectionHor.distanceTo(cornerGlobal);
        if (dist < minDist) {
            minIntersectionPoint = intersectionHor;
            // LC_ERR << " min intersection HOr!";
        }
    }

    if (!minIntersectionPoint.valid) {
        // LC_ERR << "ISO - GridRay: no intersections";
        return coord;
    }
    // LC_ERR << "###################   #####################ISO - GridRay: HAS intersections";
    const RS_Vector result = minIntersectionPoint;
    return result;
}

void LC_IsometricGrid::prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) {
    constexpr double angle30Deg = M_PI / 6;
    const double deltaY = viewSize.y - viewZero.y;
    const double offsetY = viewSize.y - m_metaGridMin.y;
    const double offsetX = viewZero.x - m_metaGridMin.x;

//    if (deltaX / space.x > 1e3 || deltaY / space.y > 1e3) {
//        return;
//    }

    const double tan30Deg = tan(angle30Deg);
    const double dxLeft = fabs(offsetY / tan30Deg);
    const double dxRight = fabs(deltaY / tan30Deg) - dxLeft;

    m_gridDeltaLeft = dxLeft;
    m_gridDeltaRight = dxRight;

    const auto viewMaxPoint = RS_Vector(viewSize.x, viewZero.y);
    const auto metaGridViewOffset = RS_Vector(offsetX, offsetY);

    m_gridBasePoint = m_metaGridMin;

    m_numPointsInMetagridX = RS_Math::round(m_metaGridCellSize.x / m_gridCellSize.x) - 1;
    m_numPointsInMetagridY = RS_Math::round(m_metaGridCellSize.y / m_gridCellSize.y) - 1;


    calculateTilesGridMetrics(viewMaxPoint, metaGridViewOffset);
    m_gridLattice->update(30, 60, m_gridCellSize, 0);
    prepareSnapSolution();
}

void LC_IsometricGrid::determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) {
    m_isometricCell = RS_Vector(m_metaGridCellSize.x * 2, 0);
    constexpr double angle30Deg = M_PI / 6;
    m_isometricCell.rotate(angle30Deg);
    const double metaHorizontalX = m_isometricCell.x;
    const double metaVerticalY = m_isometricCell.y;


    m_metaGridMin.x = truncToStep(viewZero.x, metaHorizontalX);
    m_metaGridMax.x = truncToStep(viewSize.x, metaHorizontalX);
    m_metaGridMax.y = truncToStep(viewZero.y, metaVerticalY);
    m_metaGridMin.y = truncToStep(viewSize.y, metaVerticalY);

    if (m_metaGridMin.x < viewZero.x) {
        m_metaGridMin.x += metaHorizontalX;
    }
    if (m_metaGridMax.x > viewSize.x) {
        m_metaGridMax.x -= metaHorizontalX;
    }
    if (viewSize.y > m_metaGridMin.y) {
        m_metaGridMin.y += metaVerticalY;
    }
    if (m_metaGridMax.y > viewZero.y) {
        m_metaGridMax.y -= metaVerticalY;
    }

    m_metaGridViewOffset = RS_Vector(-metaHorizontalX, +metaVerticalY);
}

void LC_IsometricGrid::createCellVector(const RS_Vector &gridWidth) {
    m_cellVector.set(sqrt(3.) * gridWidth.y, fabs(gridWidth.y)); // fixme - sqrt(3) - is it approximation there?
}

void LC_IsometricGrid::determineGridPointsAmount([[maybe_unused]]const RS_Vector &vector) {
}

int LC_IsometricGrid::determineTotalPointsAmount([[maybe_unused]]bool drawGridWithoutGaps) {
    const double metaHorizontalX = m_isometricCell.x;
    const double metaVerticalY = m_isometricCell.y;

    const int numTilesByX = static_cast<int>((m_metaGridMax.x - m_metaGridMin.x) / metaHorizontalX) + 2;
    const int numTilesByY = static_cast<int>((m_metaGridMax.y - m_metaGridMin.y) / metaVerticalY) + 2;


    const int numPointsTotal = numTilesByX * numTilesByY * m_numPointsInMetagridX * m_numPointsInMetagridY * 4;
    return numPointsTotal;
}

void LC_IsometricGrid::createGridLinesNoGaps(const RS_Vector &min, const RS_Vector &max) const {

    const double minX = min.x;
    const double maxX = max.x;
    const double minY = min.y;
    const double maxY = max.y;


    const double height = maxY;
    const double width = maxX;
    const double uiGridDeltaLeft = m_gridDeltaLeft;
    const double uiGridDeltaRight = m_gridDeltaRight;

    const double metaHorizontalX = m_isometricCell.x;
    const double metaGridHalfWidthX = metaHorizontalX / 2;

    // just project a rough amount of lines will be drawn and so amount of points in lattice to reserve
    const int numLinesX = width/metaGridHalfWidthX * 2;
    const int numPointsProjection = numLinesX * numLinesX * 2;

    m_gridLattice->update(30, 60, m_gridCellSize, numPointsProjection);

    const RS_Vector deltaX = m_gridLattice->getDeltaX() * 2;

    const double distanceToMetaGridTolerance = deltaX.x * 0.9;

    if (m_drawTopLines){
        const double halfMetaX = metaHorizontalX/2;
        double nextMetaX = m_metaGridMin.x + halfMetaX;
        const double gridByX = deltaX.x/2;
        double currentX = m_metaGridMin.x + gridByX;
        const double verticalTolerance = distanceToMetaGridTolerance /2;
        while (true){
            if (currentX > width) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > verticalTolerance) {
                m_gridLattice->addLine(currentX, minY, currentX, height);
                currentX += gridByX;
            }
            else{
                nextMetaX += halfMetaX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX, minY, currentX, height);
                }
                currentX += gridByX;
            }
        }
        // draw from minX to left
        nextMetaX = m_metaGridMin.x - halfMetaX;
        currentX = m_metaGridMin.x - gridByX;
        while (true) {
            const double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < minX) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > verticalTolerance) {
                m_gridLattice->addLine(currentX, minY, currentX, height);
                currentX -= gridByX;
            } else {
                nextMetaX -= halfMetaX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX, minY, currentX, height);
                }
                currentX -= gridByX;
            }
        }

        if (!m_gridOptions->drawMetaGrid){
            m_gridLattice->addLine(m_metaGridMin.x, minY, m_metaGridMin.x, height);
        }
    }

    if (m_drawLeftLine) {
//  draw from minx to right
        double nextMetaX = m_metaGridMin.x + metaHorizontalX;
        double currentX = m_metaGridMin.x + deltaX.x;
        while (true) {
            const double leftPoint = currentX - uiGridDeltaRight;
            if (leftPoint > width) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                m_gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                currentX += deltaX.x;
            } else {
                nextMetaX += metaHorizontalX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                }
                currentX += deltaX.x;
            }
        }

        nextMetaX = m_metaGridMin.x - metaHorizontalX;
        currentX = m_metaGridMin.x - deltaX.x;

        // draw from minX to left
        while (true) {
            const double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < minX) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                m_gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                currentX -= deltaX.x;
            } else {
                nextMetaX -= metaHorizontalX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
                }
                currentX -= deltaX.x;
            }
        }

        if (!m_gridOptions->drawMetaGrid){
            m_gridLattice->addLine(m_metaGridMin.x + uiGridDeltaLeft, height, m_metaGridMin.x - uiGridDeltaRight, minY);
        }
    }

    if (m_drawRightLine) {
//  draw from minx to right
        double nextMetaX = m_metaGridMin.x + metaHorizontalX;
        double currentX = m_metaGridMin.x + deltaX.x;
        while (true) {
            const double leftPoint = currentX - uiGridDeltaLeft;
            if (leftPoint > width) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                m_gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                currentX += deltaX.x;
            } else {
                nextMetaX += metaHorizontalX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                }
                currentX += deltaX.x;
            }
        }

        nextMetaX = m_metaGridMin.x - metaHorizontalX;
        currentX = m_metaGridMin.x - deltaX.x;

        // draw from minX to left
        while (true) {
            const double rightPoint = currentX + uiGridDeltaRight;
            if (rightPoint < minX) {
                break;
            }
            const double distanceToMetaLine = std::abs(currentX - nextMetaX);
            if (distanceToMetaLine > distanceToMetaGridTolerance) {
                m_gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                currentX -= deltaX.x;
            } else {
                nextMetaX -= metaHorizontalX;
                if (!m_gridOptions->drawMetaGrid){
                    m_gridLattice->addLine(currentX - uiGridDeltaLeft, height, currentX + uiGridDeltaRight, minY);
                }
                currentX -= deltaX.x;
            }
        }

        if (!m_gridOptions->drawMetaGrid){
            m_gridLattice->addLine(m_metaGridMin.x - uiGridDeltaLeft, height, m_metaGridMin.x + uiGridDeltaRight, minY);
        }
    }
}


void LC_IsometricGrid::createMetaGridLines(const RS_Vector& min, const RS_Vector &max) {

    double x;

    const double minX = min.x;
    const double maxX = max.x;
    const double minY = min.y;
    const double maxY = max.y;


    const double height = maxY;
    const double width = maxX;
    const double uiGridDeltaLeft = m_gridDeltaLeft;
    const double uiGridDeltaRight = m_gridDeltaRight;

    const double metaHorizontalX = m_isometricCell.x;
    const double metaGridHalfWidthX = metaHorizontalX / 2;

    // just project a rough amount of lines will be drawn and so amount of points in lattice to reserve
    const int numLinesX = (width - min.x)/metaGridHalfWidthX * 2;
    const int numPointsProjection = numLinesX * numLinesX * 2;

    m_metaGridLattice->update(30, 60, m_metaGridCellSize, numPointsProjection);

    for (x = m_metaGridMin.x; x < width; x += metaHorizontalX) {
        if (m_drawRightLine) {
            m_metaGridLattice->addLine(x - uiGridDeltaLeft, height, x + uiGridDeltaRight, minY);
        }

        if (m_drawLeftLine) {
            m_metaGridLattice->addLine(x + uiGridDeltaLeft, height, x - uiGridDeltaRight, minY);
        }

        // vertical grid lines:qc
        if (m_drawTopLines) {
            const double halfX = x - metaGridHalfWidthX;
            m_metaGridLattice->addLine(x, minY, x, height);
            m_metaGridLattice->addLine(halfX, minY, halfX, height);
        }
    }

    const double lastX = x;
    if (m_drawTopLines) {
        // draw rightmost possible vertical line
        x -= metaGridHalfWidthX;
        if (x < width) {
            m_metaGridLattice->addLine(x, minY, x, height);
        }
    }

    if (m_drawRightLine) {
        // paint left top corner
        double currentX = m_metaGridMin.x - metaHorizontalX;
        while (true) {
            const double rightPoint = currentX + uiGridDeltaRight;
            if (rightPoint < minX) {
                break;
            }
            m_metaGridLattice->addLine(currentX - uiGridDeltaLeft, height, rightPoint, minY);
            currentX -= metaHorizontalX;
        }

        // paint left bottom corner
        currentX = lastX;
        while (true) {
            const double leftPoint = currentX - uiGridDeltaLeft;
            if (leftPoint > width) {
                break;
            }
            m_metaGridLattice->addLine(leftPoint, height, currentX + uiGridDeltaRight, minY);
            currentX += metaHorizontalX;
        }
    }

    if (m_drawLeftLine) {
//  draw right top corner
        double currentX = lastX;
        while (true) {
            const double leftPoint = currentX - uiGridDeltaRight;
            if (leftPoint > width) {
                break;
            }
            m_metaGridLattice->addLine(currentX + uiGridDeltaLeft, height, currentX - uiGridDeltaRight, minY);
            currentX += metaHorizontalX;
        }

// fill left bottom corner
        currentX = m_metaGridMin.x;
        while (true) {
            const double rightPoint = currentX + uiGridDeltaLeft;
            if (rightPoint < 0) {
                break;
            }
            m_metaGridLattice->addLine(rightPoint, height, currentX - uiGridDeltaRight, minY);
            currentX -= metaHorizontalX;
        }
    }
}

void LC_IsometricGrid::createGridLines(const RS_Vector& min, const RS_Vector& max, const RS_Vector& gridWidth, const bool drawGridWithoutGaps,
                                       const RS_Vector& lineInTileOffset) {

    if (drawGridWithoutGaps){
        createGridLinesNoGaps(min, max);
    }
    else{
        int linesCount = m_numPointsInMetagridX + m_numPointsInMetagridY;
        if (m_drawTopLines){
            linesCount = linesCount * 4;
        }
        m_gridLattice->updateForLines(30, 60, gridWidth, lineInTileOffset, linesCount * 2);
        if (m_drawTopLines){
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

void LC_IsometricGrid::drawMetaGridLines(RS_Painter *painter, LC_GraphicViewport *view) {
    doDrawLines(painter, view, m_metaGridLattice.get());

#ifdef DEBUG_ISO_META

    double firstX = view->toGuiX(m_metaGridMin.x);

    double uiGridDeltaLeft = view->toGuiDX(m_gridDeltaLeft);
    double uiGridDeltaRight = view->toGuiDX(m_gridDeltaRight);

    painter->setPen({RS_Color(QColor("blue")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    if (m_drawRightLine) {
        painter->drawLine(firstX - uiGridDeltaLeft, view->getHeight(), firstX + uiGridDeltaRight, 0);
    }

    if (m_drawLeftLine) {
        painter->drawLine(firstX + uiGridDeltaLeft, view->getHeight(), firstX - uiGridDeltaRight, 0);
    }

    painter->setPen({RS_Color(QColor("yellow")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    painter->drawLine(view->toGui(m_metaGridMin), view->toGui(m_metaGridMax));
    painter->drawLine(view->toGui(m_metaGridMin), view->toGui(m_metaGridMax.x, m_metaGridMin.y));

    painter->setPen({RS_Color(QColor("cyan")), RS2::LineWidth::Width00, RS2::LineType::SolidLine,});
    painter->drawPoint(view->toGui(m_gridBasePoint), LC_DEFAULTS_PDMode, LC_DEFAULTS_PDSize);
#endif
}

void LC_IsometricGrid::createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, const bool drawGridWithoutGaps, const int totalPoints) {
    m_gridLattice->update(30, 60, gridWidth, totalPoints);
    if (drawGridWithoutGaps){
        fillPointsNoGaps(min, max);
    }
    else {
        if (m_drawTopLines) {
            fillTilesRowsByPointsExceptDiagonal();
        } else {
            fillTilesRowsByPoints();
        }
    }
}

void LC_IsometricGrid::prepareSnapSolution() {
    m_gridDeltaX = m_gridLattice->getDeltaX();
    m_gridDeltaY = m_gridLattice->getDeltaY();
    m_snapVectorSolution = RS_VectorSolutions({RS_Vector(0,0), m_gridDeltaY, m_gridDeltaX, -m_gridDeltaX, -m_gridDeltaY});
}

void LC_IsometricGrid::calculateTilesGridMetrics(const RS_Vector &maxCorner, const RS_Vector &offset) {
    const double metaHorizontalX = m_isometricCell.x / 2;
    const double metaVerticalY = m_isometricCell.y / 2;
    const double minX = m_metaGridMin.x - m_isometricCell.x;
    const double minY = m_metaGridMin.y - m_isometricCell.y;

    m_tileNumPointsByX = m_numPointsInMetagridX;
    m_tileNumPointsByY = m_numPointsInMetagridY;

    // fixme - check size of lines that are actually drawn
    if (!m_drawRightLine) {
        m_tileNumPointsByY++;
    }

    if (!m_drawLeftLine) {
        m_tileNumPointsByX++;
    }

    m_tilesRowShift = metaHorizontalX / 2;
    const bool firstRowIsNearToMin = std::abs(offset.y) < metaVerticalY;
    if (firstRowIsNearToMin) {
        double shift = metaHorizontalX;
        if (metaHorizontalX > offset.x) {
            shift = -metaHorizontalX;
        }
        m_tilesStartPoint = RS_Vector(minX + shift - m_tilesRowShift, minY);
    } else {
        double shift = 0;
        if (metaHorizontalX > offset.x) {
            shift =  metaHorizontalX;
        }
        m_tilesStartPoint = RS_Vector(minX + shift - (metaHorizontalX * 2) + m_tilesRowShift, minY - metaVerticalY);
    }
    m_tilesMaxPoint = RS_Vector(maxCorner.x + metaHorizontalX, maxCorner.y);

    m_tilesDelta = RS_Vector(m_isometricCell.x, metaVerticalY);
}


void LC_IsometricGrid::fillTilesRowsByPointsExceptDiagonal() const {
    const RS_Vector startPoint = m_tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    const double maxX = m_tilesMaxPoint.x;
    const double maxY = m_tilesMaxPoint.y;
    double rowShift = m_tilesRowShift;

    const double tilesDeltaX = m_tilesDelta.x;
    const double tilesDeltaY = m_tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            m_gridLattice->fillWithoutDiagonal(m_tileNumPointsByX, m_tileNumPointsByY, basePoint, false, false, true, m_numPointsInMetagridX);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;
    }
}

void LC_IsometricGrid::fillTilesRowsByPoints() const {
    const RS_Vector startPoint = m_tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    const double maxX = m_tilesMaxPoint.x;
    const double maxY = m_tilesMaxPoint.y;
    double rowShift = m_tilesRowShift;
    const double tilesDeltaX = m_tilesDelta.x;
    const double tilesDeltaY = m_tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            m_gridLattice->fill(m_tileNumPointsByX, m_tileNumPointsByY, basePoint, false, false);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;
    }
}

void LC_IsometricGrid::fillTilesRowsByLinesNoDiagonals() const {
    const RS_Vector startPoint = m_tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    const double maxX = m_tilesMaxPoint.x;
    const double maxY = m_tilesMaxPoint.y;
    double rowShift = m_tilesRowShift;

    const double tilesDeltaX = m_tilesDelta.x;
    const double tilesDeltaY = m_tilesDelta.y;

    int numPointsX = m_tileNumPointsByX;
    int numPointsY = m_tileNumPointsByY;

    if (!m_drawRightLine){
        numPointsY --;
    }
    if (!m_drawLeftLine){
        numPointsX --;
    }

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            m_gridLattice->fillAllByLinesExceptDiagonal(numPointsX, numPointsY, basePoint, false, false, !m_drawLeftLine, !m_drawRightLine);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;
    }
}

void LC_IsometricGrid::fillTilesRowsBylines() const {
    const RS_Vector startPoint = m_tilesStartPoint;
    double currentY = startPoint.y;
    RS_Vector basePoint = startPoint;

    const double maxX = m_tilesMaxPoint.x;
    const double maxY = m_tilesMaxPoint.y;
    double rowShift = m_tilesRowShift;

    const double tilesDeltaX = m_tilesDelta.x;
    const double tilesDeltaY = m_tilesDelta.y;

    while (currentY < maxY) {
        double currentX = startPoint.x - rowShift;
        basePoint.x = currentX;
        while (currentX < maxX) {
            m_gridLattice->fillByLines(m_tileNumPointsByX, m_tileNumPointsByY, basePoint, false, false, !m_drawLeftLine, !m_drawRightLine);
            currentX += tilesDeltaX;
            basePoint.x = currentX;
        }
        currentY += tilesDeltaY;
        basePoint.y = currentY;
        rowShift = -rowShift;
    }
}

void LC_IsometricGrid::fillPointsNoGaps(const RS_Vector &min, const RS_Vector &max) const {
    const RS_Vector deltaX = m_gridLattice->getDeltaX();
    const RS_Vector deltaY = m_gridLattice->getDeltaY();
    const double pointsDeltaX = deltaX.x;
    const double pointsDeltaY = deltaY.y;

    const auto leftBottomCorner = RS_Vector(min.x, max.y);

    const RS_Vector offset = leftBottomCorner-m_metaGridMin;

    const double startX = min.x - remainder(offset.x, pointsDeltaX * 2) + (pointsDeltaX / 2);
    const double startY =  max.y - remainder(offset.y, pointsDeltaY*2)+pointsDeltaY;

    const double maxX = max.x;

    const double shiftByX = pointsDeltaX*2;
    double rowShift = deltaY.x/2;
    double currentY = startY;
    while (currentY < min.y) {
        double currentX = startX - rowShift;
        while (currentX < maxX) {
            m_gridLattice->addPoint(currentX, currentY);
            currentX += shiftByX;
        }
        currentY += pointsDeltaY;
        rowShift = -rowShift;
    }
}
