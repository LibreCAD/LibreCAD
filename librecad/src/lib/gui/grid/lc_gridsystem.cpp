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

#include "lc_gridsystem.h"

#include "lc_graphicviewport.h"
#include "lc_lattice.h"
#include "rs_painter.h"
#include "rs_pen.h"

namespace {
//maximum number grid points to draw, for performance consideration
    const int maxGridPoints=1000000;
//minimum grid width to consider
    const double minimumGridWidth=1.0e-8;
}

LC_GridSystem::LC_GridSystem(LC_GridSystem::LC_GridOptions *options):
    m_gridOptions{std::make_unique<LC_GridSystem::LC_GridOptions>(options != nullptr ? *options: LC_GridSystem::LC_GridOptions{})}
, m_gridLattice{std::make_unique<LC_Lattice>()}
, m_metaGridLattice{std::make_unique<LC_Lattice>()}{
}

LC_GridSystem::~LC_GridSystem() = default;

void LC_GridSystem::createGrid(
    LC_GraphicViewport* view,
    const RS_Vector &viewZero, const RS_Vector &viewSize,
    const RS_Vector &metaGridWidth, const RS_Vector &gridWidth) {

    if (!m_valid){
        doCreateGrid(view, viewZero, viewSize, metaGridWidth, gridWidth);
        m_valid = true;
    }
}

bool LC_GridSystem::isGridDisabledByPanning(LC_GraphicViewport* view){
    return m_gridOptions->disableGridOnPanning && view->isPanning();
}

void LC_GridSystem::doCalculateSnapInfo(RS_Vector &viewZero, RS_Vector &viewSize, RS_Vector &metaGridWidth, RS_Vector &gridWidth) {
    setCellSize(gridWidth, metaGridWidth);
    createCellVector(m_gridCellSize);
    determineMetaGridBoundaries(viewZero, viewSize);
    prepareGridOther(viewZero, viewSize);
}

void LC_GridSystem::doCreateGrid(
    LC_GraphicViewport *view, const RS_Vector &viewZero, const RS_Vector &viewSize, const RS_Vector &metaGridWidth, const RS_Vector &gridWidth) {

    bool gridVisible = m_gridOptions->drawGrid && gridWidth.valid;
    bool metaGridVisible = m_gridOptions->drawMetaGrid && metaGridWidth.valid;
    bool simpleGridRendering = m_gridOptions->simpleGridRendering;

    setCellSize(gridWidth, metaGridWidth);

    createCellVector(m_gridCellSize);
    determineMetaGridBoundaries(viewZero, viewSize);
    prepareGridOther(viewZero, viewSize);

    if (gridVisible) { // we'll not draw invalid grid (due to min grid width), but may draw metagrid width
        determineGridPointsAmount(viewZero);
        bool drawGridWithoutGaps = simpleGridRendering || !metaGridVisible;
        int numPointsTotal = determineTotalPointsAmount(drawGridWithoutGaps);

        if (m_gridOptions->drawLines || m_hasAxisIndefinite) {
            // grid lines data
            int lineOffsetPx = m_gridOptions->metaGridLineWidthPx * 2;

            double ucsOffsetX = view->toUcsDX(lineOffsetPx);
            double ucsOffsetY = view->toUcsDY(lineOffsetPx);
            RS_Vector lineOffset = RS_Vector(ucsOffsetX, ucsOffsetY);
            createGridLines(viewZero, viewSize, m_gridCellSize, drawGridWithoutGaps, lineOffset);
            m_gridLattice->toGui(view);
        } else {
            // create points array
            if (isNumberOfPointsValid(numPointsTotal)) {
                createGridPoints(viewZero, viewSize, m_gridCellSize, drawGridWithoutGaps, numPointsTotal);
                m_gridLattice->toGui(view);
            } else {
                m_gridLattice->init(0);
            }
        }
    }
    else{
        m_gridLattice->init(0);
    }

    if (metaGridVisible){
        createMetaGridLines(viewZero, viewSize);
        m_metaGridLattice->toGui(view);
    }
    else{
        m_metaGridLattice->init(0);
    }
}

void LC_GridSystem::setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) {
    m_metaGridCellSize = metaGridWidth;
    m_gridCellSize = gridWidth;
}

void LC_GridSystem::setOptions(std::unique_ptr<LC_GridSystem::LC_GridOptions> options) {
    m_gridOptions = std::move(options);
}

void LC_GridSystem::invalidate() {
    m_valid = false;
}

bool LC_GridSystem::isValid() const {
    return m_valid;
}

void LC_GridSystem::draw(RS_Painter *painter, LC_GraphicViewport *view) {
    if (isGridDisabledByPanning(view)){
        return;
    }
    else {
        // fixme - special handling of order if simplify grid painter to make grid over meta grid?
        if (m_gridOptions->drawMetaGrid) {
            drawMetaGrid(painter, view);
        }
        drawGrid(painter, view);
    }
}

void LC_GridSystem::drawMetaGrid(RS_Painter *painter, LC_GraphicViewport *view) {
    RS_Pen pen = RS_Pen(m_gridOptions->metaGridColor, RS2::Width00, m_gridOptions->metaGridLineType);
    pen.setScreenWidth(m_gridOptions->metaGridLineWidthPx);
    painter->setPen(pen);
    painter->noCapStyle();
    drawMetaGridLines(painter, view);
}

void LC_GridSystem::drawGrid(RS_Painter *painter, LC_GraphicViewport *view) {
    if (m_gridOptions->drawLines || m_hasAxisIndefinite){
        RS_Pen pen = RS_Pen(m_gridOptions->gridColorLine, RS2::Width00, m_gridOptions->gridLineType);
        pen.setScreenWidth(m_gridOptions->gridWidthPx);
        painter->setPen(pen);
        painter->noCapStyle();
        drawGridLines(painter, view);
    }
    else{
        painter->setPen({m_gridOptions->gridColorPoint, RS2::Width00, RS2::SolidLine});
        drawGridPoints(painter, view);
    }
}

void LC_GridSystem::drawGridPoints(RS_Painter *painter, [[maybe_unused]]LC_GraphicViewport *view) {
    int pointsCount = getGridPointsCount();
    for (int i = 0; i < pointsCount; i++){
        double pX = m_gridLattice->getPointX(i);
        double pY = m_gridLattice->getPointY(i);
        painter->drawGridPoint(pX, pY);
    }
}

void LC_GridSystem::drawGridLines(RS_Painter *painter, LC_GraphicViewport *view) {
    doDrawLines(painter, view, m_gridLattice.get());
}

void LC_GridSystem::doDrawLines(RS_Painter *painter, [[maybe_unused]]LC_GraphicViewport *view, LC_Lattice* linesLattice) {
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
        painter->drawLineUISimple(startPointX, startPointY, endPointX, endPointY);
    }
}

int LC_GridSystem::getGridPointsCount() {
    return m_gridLattice->getPointsSize();
}

double LC_GridSystem::truncToStep(double value, double step){
    double result = (int) (value / step) * step;
    return result;
}

bool LC_GridSystem::isNumberOfPointsValid(int numberOfPoints){
    return numberOfPoints >= 0 && numberOfPoints < maxGridPoints;
}

void LC_GridSystem::clearGrid() {
    m_gridLattice->init(0);
    if (m_metaGridLattice != nullptr){
        m_metaGridLattice->init(0);
    }
}

void LC_GridSystem::setGridInfiniteState(bool hasIndefiniteAxis, bool undefinedX) {
    m_hasAxisIndefinite = hasIndefiniteAxis;
    m_indefiniteX = undefinedX;
}

void LC_GridSystem::calculateSnapInfo(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse) {
    if (!m_valid){
        doCalculateSnapInfo( viewZero, viewSize, metaGridWidthToUse, gridWidthToUse);
        m_valid = true;
    }
}
