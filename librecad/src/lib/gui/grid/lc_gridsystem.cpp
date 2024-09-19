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
   pointsLattice = new LC_Lattice();
   if (options->drawLines){
       linesLattice = new LC_Lattice();
   }
   if (options->drawMetaGrid){
       metaGridLinesLattice = new LC_Lattice();
   }
}

LC_GridSystem::~LC_GridSystem() {
    delete gridOptions;
    delete pointsLattice;
    delete linesLattice;
    delete metaGridLinesLattice;
}

void LC_GridSystem::createGrid(
    RS_GraphicView* view,
    const RS_Vector &viewZero, const RS_Vector &viewSize,
    const RS_Vector &metaGridWidth, const RS_Vector &gridWidth) {

    if (!valid){
        if (gridOptions->disableGridOnPanning && view->isPanning()){
            return;
        }
        doCreateGrid(view, viewZero, viewSize, metaGridWidth, gridWidth);
        valid = true;
    }
}

void LC_GridSystem::setOptions(LC_GridSystem::LC_GridOptions *options) {
    delete gridOptions;
    gridOptions = options;
}

void LC_GridSystem::invalidate() {
    valid = false;
}

void LC_GridSystem::draw(RS_Painter *painter, RS_GraphicView *view) {
    if (gridOptions->disableGridOnPanning && view->isPanning()){
        return;
    }
    // fixme - special handling of order if simplify grid painter to make grid over meta grid?
    if (gridOptions->drawMetaGrid){
        drawMetaGrid(painter, view);
    }
    drawGrid(painter, view);
}

void LC_GridSystem::drawMetaGrid(RS_Painter *painter, RS_GraphicView *view) {
    painter->setPen({gridOptions->metaGridColor, RS2::Width00, gridOptions->metaGridLineType}, gridOptions->metaGridLineWidthPx);
    drawMetaGridLines(painter, view);
}

void LC_GridSystem::drawGrid(RS_Painter *painter, RS_GraphicView *view) {
    if (gridOptions->drawLines){
        painter->setPen({gridOptions->gridColor, RS2::Width00, gridOptions->gridLineType}, gridOptions->gridWidthPx);
        drawGridLines(painter, view);
    }
    else{
        painter->setPen({gridOptions->gridColor, RS2::Width00, RS2::SolidLine});
        drawGridPoints(painter, view);
    }
}

void LC_GridSystem::drawGridPoints(RS_Painter *painter, [[maybe_unused]]RS_GraphicView *view) {
    int pointsCount = getGridPointsCount();
    std::vector<double> pointsX = pointsLattice->getPointsX();
    std::vector<double> pointsY = pointsLattice->getPointsY();
    for (int i = 0; i < pointsCount; i++){
        double pX = pointsX[i];
        double pY = pointsY[i];
        painter->drawGridPoint(pX, pY);
    }
}

void LC_GridSystem::drawGridLines(RS_Painter *painter, RS_GraphicView *view) {
    doDrawLines(painter, view, linesLattice);
}

void LC_GridSystem::doDrawLines(RS_Painter *painter, [[maybe_unused]]RS_GraphicView *view, LC_Lattice* linesLattice) {
    int pointsCount = linesLattice->getPointsSize();
//    LC_ERR << "Lines Points Count: " << pointsCount;
    int i = 0;
    while (i < pointsCount){
        RS_Vector startPoint = linesLattice->getPoint(i);
        i++;
        RS_Vector endPoint = linesLattice->getPoint(i);
        i++;
        painter->drawLine(startPoint, endPoint);
    }
}

int LC_GridSystem::getGridPointsCount() {
    return pointsLattice->getPointsSize();
}

double LC_GridSystem::truncToStep(double value, double step){
    double result = (int) (value / step) * step;
    return result;
}

bool LC_GridSystem::isNumberOfPointsValid(int numberOfPoints){
    return numberOfPoints >= 0 || numberOfPoints < maxGridPoints;
}
