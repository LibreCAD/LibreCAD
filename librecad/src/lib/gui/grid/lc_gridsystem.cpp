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

#include "rs.h"
#include "rs_math.h"
#include "rs_vector.h"
#include "lc_gridsystem.h"
#include "rs_graphicview.h"
#include "rs_pen.h"
#include "rs_debug.h"

namespace {
//maximum number grid points to draw, for performance consideration
    const int maxGridPoints=1000000;
//minimum grid width to consider
    const double minimumGridWidth=1.0e-8;
}

LC_GridSystem::LC_GridSystem(LC_GridSystem::LC_GridOptions *options) {
   gridOptions = options;
   gridLattice = new LC_Lattice();
   metaGridLattice = new LC_Lattice();
}

LC_GridSystem::~LC_GridSystem() {
    delete gridOptions;
    delete gridLattice;
    delete metaGridLattice;
}

void LC_GridSystem::createGrid(
    RS_GraphicView* view,
    const RS_Vector &viewZero, const RS_Vector &viewSize,
    const RS_Vector &metaGridWidth, const RS_Vector &gridWidth) {

    if (!valid){
        doCreateGrid(view, viewZero, viewSize, metaGridWidth, gridWidth);
        valid = true;
    }
}

bool LC_GridSystem::isGridDisabledByPanning(RS_GraphicView* view){
    return gridOptions->disableGridOnPanning && view->isPanning();
}

void LC_GridSystem::doCalculateSnapInfo(RS_Vector &viewZero, RS_Vector &viewSize, RS_Vector &metaGridWidth, RS_Vector &gridWidth) {
    setCellSize(gridWidth, metaGridWidth);
    createCellVector(gridCellSize);
    determineMetaGridBoundaries(viewZero, viewSize);
    prepareGridOther(viewZero, viewSize);
}

void LC_GridSystem::doCreateGrid(
    RS_GraphicView *view, const RS_Vector &viewZero, const RS_Vector &viewSize, const RS_Vector &metaGridWidth, const RS_Vector &gridWidth) {

    bool gridVisible = gridOptions->drawGrid && gridWidth.valid;
    bool metaGridVisible = gridOptions->drawMetaGrid && metaGridWidth.valid;
    bool simpleGridRendering = gridOptions->simpleGridRendering;

    setCellSize(gridWidth, metaGridWidth);

    createCellVector(gridCellSize);
    determineMetaGridBoundaries(viewZero, viewSize);
    prepareGridOther(viewZero, viewSize);

    if (gridVisible) { // we'll not draw invalid grid (due to min grid width), but may draw metagrid width
        determineGridPointsAmount(viewZero);
        bool drawGridWithoutGaps = simpleGridRendering || !metaGridVisible;
        int numPointsTotal = determineTotalPointsAmount(drawGridWithoutGaps);

        if (gridOptions->drawLines || hasAxisIndefinite) {
            // grid lines data
            int lineOffsetPx = gridOptions->metaGridLineWidthPx * 2;
            RS_Vector lineOffset = view->toGraphD(lineOffsetPx, lineOffsetPx);
            createGridLines(viewZero, viewSize, gridCellSize, drawGridWithoutGaps, lineOffset);
            gridLattice->toGui(view);
        } else {
            // create points array
            if (isNumberOfPointsValid(numPointsTotal)) {
                createGridPoints(viewZero, viewSize, gridCellSize, drawGridWithoutGaps, numPointsTotal);
                gridLattice->toGui(view);
            } else {
                gridLattice->init(0);
            }
        }
    }
    else{
        gridLattice->init(0);
    }

    if (metaGridVisible){
        createMetaGridLines(viewZero, viewSize);
        metaGridLattice->toGui(view);
    }
    else{
        metaGridLattice->init(0);
    }
}

void LC_GridSystem::setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) {
    metaGridCellSize = metaGridWidth;
    gridCellSize = gridWidth;
}

void LC_GridSystem::setOptions(LC_GridSystem::LC_GridOptions *options) {
    delete gridOptions;
    gridOptions = options;
}

void LC_GridSystem::invalidate() {
    valid = false;
}

bool LC_GridSystem::isValid() const {
    return valid;
}

void LC_GridSystem::draw(RS_Painter *painter, RS_GraphicView *view) {
    if (isGridDisabledByPanning(view)){
        return;
    }
    else {
        // fixme - special handling of order if simplify grid painter to make grid over meta grid?
        if (gridOptions->drawMetaGrid) {
            drawMetaGrid(painter, view);
        }
        drawGrid(painter, view);
    }
}

void LC_GridSystem::drawMetaGrid(RS_Painter *painter, RS_GraphicView *view) {
    RS_Pen pen = RS_Pen(gridOptions->metaGridColor, RS2::Width00, gridOptions->metaGridLineType);
    pen.setScreenWidth(gridOptions->metaGridLineWidthPx);
    painter->setPen(pen);
    painter->noCapStyle();
    drawMetaGridLines(painter, view);
}

void LC_GridSystem::drawGrid(RS_Painter *painter, RS_GraphicView *view) {
    if (gridOptions->drawLines || hasAxisIndefinite){
        RS_Pen pen = RS_Pen(gridOptions->gridColorLine, RS2::Width00, gridOptions->gridLineType);
        pen.setScreenWidth(gridOptions->gridWidthPx);
        painter->setPen(pen);
        painter->noCapStyle();
        drawGridLines(painter, view);
    }
    else{
        painter->setPen({gridOptions->gridColorPoint, RS2::Width00, RS2::SolidLine});
        drawGridPoints(painter, view);
    }
}

void LC_GridSystem::drawGridPoints(RS_Painter *painter, [[maybe_unused]]RS_GraphicView *view) {
    int pointsCount = getGridPointsCount();
    for (int i = 0; i < pointsCount; i++){
        double pX = gridLattice->getPointX(i);
        double pY = gridLattice->getPointY(i);
        painter->drawGridPoint(pX, pY);
    }
}

void LC_GridSystem::drawGridLines(RS_Painter *painter, RS_GraphicView *view) {
    doDrawLines(painter, view, gridLattice);
}

void LC_GridSystem::doDrawLines(RS_Painter *painter, [[maybe_unused]]RS_GraphicView *view, LC_Lattice* linesLattice) {
    int pointsCount = linesLattice->getPointsSize();
//    LC_ERR << "Lines Points Count: " << pointsCount;
    int i = 0;
    while (i < pointsCount) {
        double startPointX = linesLattice->getPointX(i);
        double startPointY = linesLattice->getPointY(i);
        i++;
        double endPointX = linesLattice->getPointX(i);
        double endPointY = linesLattice->getPointY(i);
        i++;
        painter->drawLineSimple(startPointX, startPointY, endPointX, endPointY);
    }
}

int LC_GridSystem::getGridPointsCount() {
    return gridLattice->getPointsSize();
}

double LC_GridSystem::truncToStep(double value, double step){
    double result = (int) (value / step) * step;
    return result;
}

bool LC_GridSystem::isNumberOfPointsValid(int numberOfPoints){
    return numberOfPoints >= 0 && numberOfPoints < maxGridPoints;
}

void LC_GridSystem::clearGrid() {
    gridLattice->init(0);
    if (metaGridLattice != nullptr){
        metaGridLattice->init(0);
    }
}

void LC_GridSystem::setGridInfiniteState(bool hasIndefiniteAxis, bool undefinedX) {
    hasAxisIndefinite = hasIndefiniteAxis;
    indefiniteX = undefinedX;
}

void LC_GridSystem::calculateSnapInfo(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse) {
    if (!valid){
        doCalculateSnapInfo( viewZero, viewSize, metaGridWidthToUse, gridWidthToUse);
        valid = true;
    }
}
