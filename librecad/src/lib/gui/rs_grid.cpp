/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include<cmath>
#include<QString>
#include "rs_grid.h"
#include "rs_graphicview.h"
#include "rs_units.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "lc_rect.h"
#include "rs_debug.h"
#include "lc_lattice.h"
#include "lc_orthogonalgrid.h"
#include "lc_isometricgrid.h"


#ifdef EMU_C99
#include "emu_c99.h"
#endif

namespace {

constexpr int MINIMAL_GRID_UI_STEP = 2;

//maximum number grid points to draw, for performance consideration
const int maxGridPoints=1000000;
//minimum grid width to consider
const double minimumGridWidth=1.0e-8;
}

/**
 * Constructor.
 */
RS_Grid::RS_Grid(RS_GraphicView* graphicView)
    :graphicView(graphicView)
{}

/**
 * find the closest grid point
 *@return the closest grid to given point
 *@coord: the given point
 */
RS_Vector RS_Grid::snapGrid(const RS_Vector& coord) const {
   return gridSystem->snapGrid(coord);
}

void RS_Grid::loadSettings(){

    LC_GROUP("Appearance");
    scaleGrid = LC_GET_BOOL("ScaleGrid", true);
    // get grid setting
    RS_Graphic* graphic = graphicView->getGraphic();
    if (graphic != nullptr) {
        isometric = graphic->isIsometricGrid();
        isoViewType = graphic->getIsoView();
        userGrid = graphic->getVariableVector("$GRIDUNIT", RS_Vector(-1.0, -1.0));
    }else {
        isometric = LC_GET_ONE_BOOL("Defaults", "IsometricGrid");
        isoViewType=static_cast<RS2::IsoGridViewType>(LC_GET_ONE_INT("Defaults", "IsoGridView", 0));
        userGrid.x = LC_GET_STR("GridSpacingX", QString("-1")).toDouble();
        userGrid.y = LC_GET_STR("GridSpacingY", QString("-1")).toDouble();
    }

    bool drawMetaGrid = LC_GET_BOOL("metaGridDraw", true);
    bool drawGrid = LC_GET_BOOL("GridDraw", true);
    bool simpleGridRendering = LC_GET_BOOL("GridRenderSimple", false);
    minGridSpacing = LC_GET_INT("MinGridSpacing", 10);
    int gridType = LC_GET_INT("GridType", 0);
    bool linesGrid = gridType == 1;

    RS2::LineType metagridLineType;
    int metaGridWidthPx;

    if (linesGrid){
        metagridLineType =  static_cast<RS2::LineType> (LC_GET_INT("metaGridLinesLineType", RS2::SolidLine));
        metaGridWidthPx = LC_GET_INT("metaGridLinesLineWidth", 1);
    }
    else{
        metagridLineType =  static_cast<RS2::LineType> (LC_GET_INT("metaGridPointsLineType", RS2::DotLineTiny));
        metaGridWidthPx = LC_GET_INT("metaGridPointsLineWidth", 1);
    }

    int gridWidthPx = LC_GET_INT("GridLinesLineWidth", 1);
    RS2::LineType gridLineType =  static_cast<RS2::LineType> (LC_GET_INT("GridLinesLineType", RS2::SolidLine));

    bool disableGridOnPanning = LC_GET_BOOL("GridDisableWithinPan", false);
    bool drawIsoVerticalForTop = LC_GET_BOOL("GridDrawIsoVerticalForTop", true);
    LC_GROUP_END();

    LC_GROUP("Colors");
    RS_Color metaGridColor;
    RS_Color gridColorLines = QColor(LC_GET_STR("gridLines", RS_Settings::color_meta_grid_lines));
    RS_Color gridColorPoint =  QColor(LC_GET_STR("grid", RS_Settings::color_meta_grid_points));
     if (linesGrid) {
         metaGridColor= QColor(LC_GET_STR("meta_grid_lines", RS_Settings::color_meta_grid_lines));
     }
     else{
         metaGridColor= QColor(LC_GET_STR("meta_grid", RS_Settings::color_meta_grid_points));
     }
    LC_GROUP_END();

    auto* gridOptions = new LC_GridSystem::LC_GridOptions();
    gridOptions->drawMetaGrid = drawMetaGrid;
    gridOptions->simpleGridRendering = simpleGridRendering;
    gridOptions->gridWidthPx = gridWidthPx;
    gridOptions->gridLineType = gridLineType;
    gridOptions->drawGrid = drawGrid;
    gridOptions->drawLines = linesGrid;
    gridOptions->gridColorPoint = gridColorPoint;
    gridOptions->gridColorLine = gridColorLines;
    gridOptions->metaGridLineWidthPx  = metaGridWidthPx;
    gridOptions->metaGridLineType = metagridLineType;
    gridOptions->metaGridColor = metaGridColor;
    gridOptions->disableGridOnPanning = disableGridOnPanning;
    gridOptions->drawIsometricVerticalsAlways = drawIsoVerticalForTop;

    delete gridSystem;

    if (isometric){
        gridSystem = new LC_IsometricGrid(gridOptions, isoViewType);
    }
    else{
        gridSystem = new LC_OrthogonalGrid(gridOptions);
    }
}

void RS_Grid::calculateSnapSettings(){
    if (gridSystem->isValid() || gridSystem->isGridDisabledByPanning(graphicView)){
        return;
    }
    RS_Vector viewZero;
    RS_Vector viewSize;
    RS_Vector metaGridWidthToUse;
    RS_Vector gridWidthToUse;
    prepareGridCalculations(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);
    gridSystem->calculateSnapInfo(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);
}

/**
 * Updates the grid point array.
 */
void RS_Grid::calculateGrid() {
    if (!graphicView->isGridOn()){
        return;
    }
    if (gridSystem->isValid() || gridSystem->isGridDisabledByPanning(graphicView)){
        return;
    }

    RS_Vector viewZero;
    RS_Vector viewSize;
    RS_Vector metaGridWidthToUse;
    RS_Vector gridWidthToUse;
    prepareGridCalculations(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);

    gridSystem->createGrid(graphicView, viewZero, viewSize, metaGridWidthToUse, gridWidthToUse);
}

void RS_Grid::prepareGridCalculations(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse){
    RS_Vector gridWidth = prepareGridWidth();

    gridWidthToUse = gridWidth;
    metaGridWidthToUse = metaGridWidth;
    gridWidthToUse.valid = true;
    metaGridWidthToUse.valid = true;
    // checking for minimal grid with constraint. It may occur either to zoom of drawing, or due to options that specifies minimal width of grid in pixel.
    // if grid is scaled, that option will lead to re-calculating of grid, but if the grid is not scaled (and so has fixed size) - it may lead to
    // too dense grid for current zoom level

    bool hasInfiniteAxis = false;
    bool undefinedXSize = userGrid.x != 0;
    bool undefinedYSize = userGrid.y != 0;

    hasInfiniteAxis = undefinedYSize != undefinedXSize;

    QString gridInfoStr;
    QString metaGridInfoStr;
    bool squareGrid = gridWidth.x == gridWidth.y;
    if (!(gridWidth.x>minimumGridWidth && gridWidth.y>minimumGridWidth)){
        gridWidthToUse.valid = false;
        metaGridWidthToUse.valid = false;
    }
    else {
        RS_Vector gridWidthGUI = graphicView->toGuiD(gridWidth);
        if (gridWidthGUI.x < minGridSpacing  || gridWidthGUI.y < minGridSpacing){
            gridWidthToUse.valid = false;
        }
        else{
            if (hasInfiniteAxis){
                gridInfoStr =  QString("%1").arg(undefinedXSize ? gridWidth.y : gridWidth.x);
            }
            else {
                gridInfoStr = squareGrid ? QString("%1").arg(gridWidth.x) : QString("%1 x %2").arg(gridWidth.x).arg(gridWidth.y);
            }
        }

        RS_Vector metaGridWidthGUI = graphicView->toGuiD(metaGridWidth);
        if (metaGridWidthGUI.x < minGridSpacing  || metaGridWidthGUI.y < minGridSpacing){
            metaGridWidthToUse.valid = false;
        }
        else{
            if (hasInfiniteAxis){
                metaGridInfoStr =  QString("%1").arg(undefinedXSize ? metaGridWidth.y : metaGridWidth.x);
            }
            else {
                metaGridInfoStr = squareGrid ? QString("%1").arg(metaGridWidth.x) : QString("%1 x %2").arg(metaGridWidth.x).arg(metaGridWidth.y);
            }
        }
    }

    viewZero = graphicView->toGraph(0, 0);
    viewSize = graphicView->toGraph(graphicView->getWidth(), graphicView->getHeight());

    // populate grid points and metaGrid line positions: pts, metaX, metaY
    gridInfoString = "";
    if (gridWidthToUse.valid){
        if (metaGridWidthToUse.valid){
            gridInfoString = QString("%1 / %2").arg(gridInfoStr).arg(metaGridInfoStr);
        }
        else{
            gridInfoString = QString("%1").arg(gridInfoStr);
        }
    }
    else{
        if (metaGridWidthToUse.valid){
            gridInfoString = QString("%1").arg(metaGridInfoStr);
        }
    }

    gridSystem->setGridInfiniteState(hasInfiniteAxis, undefinedXSize);
}

RS_Vector RS_Grid::prepareGridWidth() {// find out unit:

    RS_Graphic* graphic = graphicView->getGraphic();

    RS2::Unit unit = RS2::None;
    RS2::LinearFormat format = RS2::Decimal;
    if (graphic != nullptr) {
        unit = graphic->getUnit();
        format = graphic->getLinearFormat();
    }

    RS_Vector gridWidth;
    // init grid spacing:
    // metric grid:
    if (RS_Units::isMetric(unit) || unit==RS2::None ||
        format==RS2::Decimal || format==RS2::Engineering) {
        //metric grid
        gridWidth = getMetricGridWidth(userGrid, scaleGrid, minGridSpacing);

    }else {
        // imperial grid:
        gridWidth = getImperialGridWidth(userGrid, scaleGrid, minGridSpacing);
    }
    return gridWidth;
}


RS_Vector RS_Grid::getMetricGridWidth(RS_Vector const &userGrid, bool scaleGrid, int minGridSpacing) {
    RS_Vector gridWidth;

    bool hasExplicitUserGrid = false;

    if (userGrid.x > 0.0) {
        gridWidth.x = userGrid.x;
        hasExplicitUserGrid = true;
    } else {
        gridWidth.x = minimumGridWidth;
    }

    if (userGrid.y > 0.0) {
        gridWidth.y = userGrid.y;
        hasExplicitUserGrid = true;
    } else {
        gridWidth.y = minimumGridWidth;
    }
    // auto scale grid

    if (scaleGrid){
        RS_Vector guiGridWith = graphicView->toGuiD(gridWidth);
        bool gridSmallerThanMin = guiGridWith.x < minGridSpacing || guiGridWith.y < minGridSpacing;
        if (gridSmallerThanMin) {
            do{
                gridWidth *= 10;
                guiGridWith = graphicView->toGuiD(gridWidth);
            }
            while (guiGridWith.x < minGridSpacing  || guiGridWith.y < minGridSpacing);
        }
        else if (hasExplicitUserGrid){
            // it might be that explicitly specified user grid is larger than grid calculate for current zoom. Thus we should decrease it proportionally

            RS_Vector decreasingWidth = gridWidth;
            RS_Vector previousWidth = decreasingWidth;
            while (true) {
                decreasingWidth /= 10;
                guiGridWith = graphicView->toGuiD(decreasingWidth);
                if (guiGridWith.x < minGridSpacing || guiGridWith.y < minGridSpacing){
                    gridWidth = previousWidth;
                    break;
                }
                else{
                    previousWidth = decreasingWidth;
                }
            }
        }
    }

    // std::cout<<"RS_Grid::updatePointArray(): gridWidth="<<gridWidth<<std::endl;
    metaGridWidth.x = gridWidth.x * 10;
    metaGridWidth.y = gridWidth.y * 10;

    // RS_DEBUG->print("RS_Grid::update: 004");
    return gridWidth;
}

RS_Vector RS_Grid::getImperialGridWidth(RS_Vector const &userGrid, bool scaleGrid, int minGridSpacing) {
    RS_Vector gridWidth{};
    // RS_DEBUG->print("RS_Grid::update: 005");
    if (userGrid.x > 0.0) {
        gridWidth.x = userGrid.x;
    } else {
        gridWidth.x = 1.0 / 1024.0;
    }

    if (userGrid.y > 0.0) {
        gridWidth.y = userGrid.y;
    } else {
        gridWidth.y = 1.0 / 1024.0;
    }
    // RS_DEBUG->print("RS_Grid::update: 006");

    RS2::Unit unit = RS2::None;
    RS_Graphic *graphic = graphicView->getGraphic();

    if (graphic) {
        unit = graphic->getUnit();
    }

    if (unit == RS2::Inch) {
    // RS_DEBUG->print("RS_Grid::update: 007");

    // auto scale grid
        //scale grid by drawing setting as well, bug#3416862

            RS_Vector guiGridWith = graphicView->toGuiD(gridWidth);
            bool gridSmallerThanMin = guiGridWith.x < minGridSpacing || guiGridWith.y < minGridSpacing;
            if (scaleGrid || gridSmallerThanMin) {
                do{
                    if (RS_Math::round(gridWidth.x) >= 36) {
                        gridWidth *= 2;
                    } else if (RS_Math::round(gridWidth.x) >= 12) {
                        gridWidth *= 3;
                    } else if (RS_Math::round(gridWidth.x) >= 4) {
                        gridWidth *= 3;
                    } else if (RS_Math::round(gridWidth.x) >= 1) {
                        gridWidth *= 2;
                    } else {
                        gridWidth *= 2;
                    }
                    guiGridWith = graphicView->toGuiD(gridWidth);
                }
                while (guiGridWith.x < minGridSpacing  || guiGridWith.y < minGridSpacing);
            }

       /* if (scaleGrid || userGrid.x <= minimumGridWidth || userGrid.y <= minimumGridWidth) {
            if (scaleGrid || userGrid.x <= minimumGridWidth) {
                while (graphicView->toGuiDX(gridWidth.x) < minGridSpacing) {
                    if (RS_Math::round(gridWidth.x) >= 36) {
                        gridWidth.x *= 2;
                    } else if (RS_Math::round(gridWidth.x) >= 12) {
                        gridWidth.x *= 3;
                    } else if (RS_Math::round(gridWidth.x) >= 4) {
                        gridWidth.x *= 3;
                    } else if (RS_Math::round(gridWidth.x) >= 1) {
                        gridWidth.x *= 2;
                    } else {
                        gridWidth.x *= 2;
                    }
                }
            }
            if (scaleGrid || userGrid.y <= minimumGridWidth) {
                while (graphicView->toGuiDY(gridWidth.y) < minGridSpacing) {
                    if (RS_Math::round(gridWidth.y) >= 36) {
                        gridWidth.y *= 2;
                    } else if (RS_Math::round(gridWidth.y) >= 12) {
                        gridWidth.y *= 3;
                    } else if (RS_Math::round(gridWidth.y) >= 4) {
                        gridWidth.y *= 3;
                    } else if (RS_Math::round(gridWidth.y) >= 1) {
                        gridWidth.y *= 2;
                    } else {
                        gridWidth.y *= 2;
                    }
                }
            }
        }*/

// RS_DEBUG->print("RS_Grid::update: 008");

// metagrid X shows inches..
        metaGridWidth.x = 1.0;

        int minGridSpacingX2 = minGridSpacing * 2;
        if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacingX2) {
// .. or feet
            metaGridWidth.x = 12.0;

// .. or yards
            if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacingX2) {
                metaGridWidth.x = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDX(metaGridWidth.x)<20) {
                // metaGridWidth.x = 63360.0;
                //}

                // .. or nothing
                if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacingX2) {
                    metaGridWidth.x = -1.0;
                }

            }
        }

// RS_DEBUG->print("RS_Grid::update: 009");

// metagrid Y shows inches..
        metaGridWidth.y = 1.0;

        if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacingX2) {
// .. or feet
            metaGridWidth.y = 12.0;

// .. or yards
            if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacingX2) {
                metaGridWidth.y = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDY(metaGridWidth.y)<20) {
                // metaGridWidth.y = 63360.0;
                //}

                // .. or nothing
                if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacingX2) {
                    metaGridWidth.y = -1.0;
                }
            }
        }
// RS_DEBUG->print("RS_Grid::update: 010");
    } else {
// RS_DEBUG->print("RS_Grid::update: 011");
        if (scaleGrid) {
            while (graphicView->toGuiDX(gridWidth.x) < minGridSpacing) {
                gridWidth.x *= 2;
            }
            metaGridWidth.x = -1.0;

            while (graphicView->toGuiDY(gridWidth.y) < minGridSpacing) {
                gridWidth.y *= 2;
            }
            metaGridWidth.y = -1.0;
        }
// RS_DEBUG->print("RS_Grid::update: 012");
    }
//gridWidth.y = gridWidth.x;
//metaGridWidth.y = metaGridWidth.x;
    return gridWidth;
}


QString RS_Grid::getInfo() const {
    return gridInfoString;
}

bool RS_Grid::isIsometric() const {
    return isometric;
}

void RS_Grid::setIsometric(bool b) {
    isometric = b;
}

RS_Vector RS_Grid::getMetaGridWidth() const {
    return metaGridWidth;
}

RS_Vector const &RS_Grid::getCellVector() const {
    return gridSystem->getCellVector();
}

void RS_Grid::setIsoViewType(RS2::IsoGridViewType chType) {
    isoViewType = chType;
}

RS2::IsoGridViewType RS_Grid::getIsoViewType() const {
    return isoViewType;
}


void RS_Grid::drawGrid(RS_Painter *painter) {
    gridSystem->draw(painter, graphicView);
}
