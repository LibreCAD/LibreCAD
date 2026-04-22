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

#include "rs_grid.h"

#include "lc_graphicviewport.h"
#include "lc_gridsystem.h"
#include "lc_isometricgrid.h"
#include "lc_linemath.h"
#include "lc_orthogonalgrid.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

class RS_Graphic;

namespace {
    //minimum grid width to consider
    constexpr double MINIMUM_GRID_WIDTH = 1.0e-8;
}

/**
 * Constructor.
 */
RS_Grid::RS_Grid(LC_GraphicViewport* graphicView)
    :m_viewport(graphicView), m_minGridSpacing{10}
{
}

/**
 * find the closest grid point
 *@return the closest grid to given point
 *@coord: the given point
 */
RS_Vector RS_Grid::snapGrid(const RS_Vector& coord) const {
   return m_gridSystem->snapGrid(coord);
}

RS_Vector RS_Grid::snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) const {
    return m_gridSystem->snapGrid(coord, rayStart, rayEnd);
}

void RS_Grid::loadSettings(){

    LC_GROUP("Appearance");
    m_scaleGrid = LC_GET_BOOL("ScaleGrid", true);
    // get grid setting
    auto graphic = m_viewport->getGraphic();
    if (graphic != nullptr) {
        m_isometric = graphic->isIsometricGrid();
        m_isoViewType = graphic->getIsoView();
        m_userGrid = graphic->getVariableVector("$GRIDUNIT", RS_Vector(-1.0, -1.0));
    }else {
        m_isometric = LC_GET_ONE_BOOL("Defaults", "IsometricGrid");
        m_isoViewType=static_cast<RS2::IsoGridViewType>(LC_GET_ONE_INT("Defaults", "IsoGridView", 0));
        m_userGrid.x = LC_GET_STR("GridSpacingX", QString("-1")).toDouble();
        m_userGrid.y = LC_GET_STR("GridSpacingY", QString("-1")).toDouble();
    }

    bool drawMetaGrid = LC_GET_BOOL("metaGridDraw", true);
    bool drawGrid = LC_GET_BOOL("GridDraw", true);
    bool simpleGridRendering = LC_GET_BOOL("GridRenderSimple", false);
    m_minGridSpacing = LC_GET_INT("MinGridSpacing", 10);
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
    auto gridLineType =  static_cast<RS2::LineType> (LC_GET_INT("GridLinesLineType", RS2::SolidLine));

    bool disableGridOnPanning = LC_GET_BOOL("GridDisableWithinPan", false);
    bool drawIsoVerticalForTop = LC_GET_BOOL("GridDrawIsoVerticalForTop", true);

    m_metaGridEvery = LC_GET_INT("MetaGridEvery", 10);
    LC_GROUP_END();

    LC_GROUP("Colors");
    RS_Color metaGridColor;
    auto gridColorLines = RS_Color(LC_GET_STR("gridLines", RS_Settings::COLOR_META_GRID_LINES));
    auto gridColorPoint =  RS_Color(LC_GET_STR("grid", RS_Settings::COLOR_META_GRID_POINTS));
     if (linesGrid) {
         metaGridColor= RS_Color(LC_GET_STR("meta_grid_lines", RS_Settings::COLOR_META_GRID_LINES));
     }
     else{
         metaGridColor= RS_Color(LC_GET_STR("meta_grid", RS_Settings::COLOR_META_GRID_POINTS));
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

    delete m_gridSystem;

    if (m_isometric){
        m_gridSystem = new LC_IsometricGrid(gridOptions, m_isoViewType);
    }
    else{
        m_gridSystem = new LC_OrthogonalGrid(gridOptions);
    }
}

void RS_Grid::calculateSnapSettings(){
    if (m_gridSystem->isValid() || m_gridSystem->isGridDisabledByPanning(m_viewport)){
        return;
    }
    RS_Vector viewZero;
    RS_Vector viewSize;
    RS_Vector metaGridWidthToUse;
    RS_Vector gridWidthToUse;
    prepareGridCalculations(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);
    m_gridSystem->calculateSnapInfo(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);
    m_gridWidth = gridWidthToUse;
}

/**
 * Updates the grid point array.
 */
void RS_Grid::calculateGrid() {
    if (m_gridSystem->isValid() || m_gridSystem->isGridDisabledByPanning(m_viewport)){
        return;
    }

    RS_Vector viewZero;
    RS_Vector viewSize;
    RS_Vector metaGridWidthToUse;
    RS_Vector gridWidthToUse;
    prepareGridCalculations(viewZero, viewSize,metaGridWidthToUse, gridWidthToUse);

    m_gridSystem->createGrid(m_viewport, viewZero, viewSize, metaGridWidthToUse, gridWidthToUse);
    m_gridWidth = gridWidthToUse;
}

void RS_Grid::prepareGridCalculations(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse){
    const RS_Vector gridWidth = prepareGridWidth();

    gridWidthToUse = gridWidth;
    metaGridWidthToUse = m_metaGridWidth;
    gridWidthToUse.valid = true;
    metaGridWidthToUse.valid = true;
    // checking for minimal grid with constraint. It may occur either to zoom of drawing, or due to options that specifies minimal width of grid in pixel.
    // if grid is scaled, that option will lead to re-calculating of grid, but if the grid is not scaled (and so has fixed size) - it may lead to
    // too dense grid for current zoom level

    bool hasInfiniteAxis = false;
    const bool undefinedXSize = m_userGrid.x != 0; // fixme - sand - review floating-point comparison
    const bool undefinedYSize = m_userGrid.y != 0;

    hasInfiniteAxis = undefinedYSize != undefinedXSize;

    QString gridInfoStr;
    QString metaGridInfoStr;

    if (!(gridWidth.x>MINIMUM_GRID_WIDTH && gridWidth.y>MINIMUM_GRID_WIDTH)){
        gridWidthToUse.valid = false;
        metaGridWidthToUse.valid = false;
    }
    else {
        const double gridWidthGUIX = m_viewport->toGuiDX(gridWidth.x);
        const double gridWidthGUIY = m_viewport->toGuiDY(gridWidth.y);
        const bool squareGrid = LC_LineMath::isSameLength(gridWidth.x,gridWidth.y);
        if (gridWidthGUIX < m_minGridSpacing  || gridWidthGUIY < m_minGridSpacing){
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

        const double metaGridWidthGUIX = m_viewport->toGuiDX(m_metaGridWidth.x);
        const double metaGridWidthGUIY = m_viewport->toGuiDY(m_metaGridWidth.y);

        if (metaGridWidthGUIX < m_minGridSpacing  || metaGridWidthGUIY < m_minGridSpacing){
            metaGridWidthToUse.valid = false;
        }
        else{
            if (hasInfiniteAxis){
                metaGridInfoStr =  QString("%1").arg(undefinedXSize ? m_metaGridWidth.y : m_metaGridWidth.x);
            }
            else {
                metaGridInfoStr = squareGrid ? QString("%1").arg(m_metaGridWidth.x) : QString("%1 x %2").arg(m_metaGridWidth.x).arg(m_metaGridWidth.y);
            }
        }
    }

//    RS_Vector ucsZero = RS_Vector(0,0);
//    RS_Vector ucsSize = RS_Vector(graphicView->getWidth(), graphicView->getHeight());

    viewZero = m_viewport->toUCSFromGui(0, 0);
    viewSize = m_viewport->toUCSFromGui(m_viewport->getWidth(), m_viewport->getHeight());

    // populate grid points and metaGrid line positions: pts, metaX, metaY
    m_gridInfoString.clear();
    if (gridWidthToUse.valid){
        if (metaGridWidthToUse.valid){
            m_gridInfoString = QString("%1 / %2").arg(gridInfoStr).arg(metaGridInfoStr);
        }
        else{
            m_gridInfoString = QString("%1").arg(gridInfoStr);
        }
    }
    else{
        if (metaGridWidthToUse.valid){
            m_gridInfoString = QString("%1").arg(metaGridInfoStr);
        }
    }

    m_gridSystem->setGridInfiniteState(hasInfiniteAxis, undefinedXSize);
}

bool RS_Grid::isGridMetric() const {
    const RS_Graphic* graphic = m_viewport->getGraphic();

    RS2::Unit unit = RS2::None;
    RS2::LinearFormat format;
    if (graphic != nullptr) {
        unit = graphic->getUnit();
        format = graphic->getLinearFormat();
    }
    else {
        format = RS2::Decimal;
    }

    const bool gridIsMetric = RS_Units::isMetric(unit) || unit == RS2::None || format == RS2::Decimal || format == RS2::Engineering;
    return gridIsMetric;
}

bool RS_Grid::isDrawMetaGrid() const {
    if (m_gridSystem != nullptr) {
        return m_gridSystem->isDrawMetaGrid();
    }
    return true;
}

RS_Vector RS_Grid::prepareGridWidth() {// find out unit:
    const bool gridIsMetric = isGridMetric();

    // init grid spacing:
    // metric grid:
    RS_Vector gridWidth;
    if (gridIsMetric) {
        //metric grid
        gridWidth = getMetricGridWidth(m_userGrid, m_scaleGrid, m_minGridSpacing);
    }
    else {
        // imperial grid:
        gridWidth = getImperialGridWidth(m_userGrid, m_scaleGrid, m_minGridSpacing);
    }
    return gridWidth;
}


RS_Vector RS_Grid::getMetricGridWidth(const RS_Vector&userGrid, const bool scaleGrid, const int minGridSpacing) {
    RS_Vector gridWidth;

    bool hasExplicitUserGrid = false;

    if (userGrid.x > 0.0) {
        gridWidth.x = userGrid.x;
        hasExplicitUserGrid = true;
    } else {
        gridWidth.x = MINIMUM_GRID_WIDTH;
    }

    if (userGrid.y > 0.0) {
        gridWidth.y = userGrid.y;
        hasExplicitUserGrid = true;
    } else {
        gridWidth.y = MINIMUM_GRID_WIDTH;
    }
    // auto scale grid

    if (scaleGrid){
        double guiGridWithX = m_viewport->toGuiDX(gridWidth.x);
        double guiGridWithY = m_viewport->toGuiDY(gridWidth.y);
        const bool gridSmallerThanMin = guiGridWithX < minGridSpacing || guiGridWithY < minGridSpacing;
        if (gridSmallerThanMin) {
            do{
                gridWidth *= 10;
                guiGridWithX = m_viewport->toGuiDX(gridWidth.x);
                guiGridWithY = m_viewport->toGuiDY(gridWidth.y);
            }
            while (guiGridWithX < minGridSpacing  || guiGridWithY < minGridSpacing);
        }
        else if (hasExplicitUserGrid){
            // it might be that explicitly specified user grid is larger than grid calculate for current zoom. Thus we should decrease it proportionally

            RS_Vector decreasingWidth = gridWidth;
            RS_Vector previousWidth = decreasingWidth;
            while (true) {
                decreasingWidth /= 10;

                const double guiDecreasingGridWithX = m_viewport->toGuiDX(decreasingWidth.x);
                const double guiDecreasongGridWithY = m_viewport->toGuiDY(decreasingWidth.y);
                if (guiDecreasingGridWithX < minGridSpacing || guiDecreasongGridWithY < minGridSpacing){
                    gridWidth = previousWidth;
                    break;
                }
                previousWidth = decreasingWidth;
            }
        }
    }

    // std::cout<<"RS_Grid::updatePointArray(): gridWidth="<<gridWidth<<std::endl;
    const int metaGridStep = m_metaGridEvery;
    m_metaGridWidth.x = gridWidth.x * metaGridStep;
    m_metaGridWidth.y = gridWidth.y * metaGridStep;

    // RS_DEBUG->print("RS_Grid::update: 004");
    return gridWidth;
}

RS_Vector RS_Grid::getImperialGridWidth(const RS_Vector&userGrid, const bool scaleGrid, const int minGridSpacing) {
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
    const RS_Graphic *graphic = m_viewport->getGraphic();

    if (graphic != nullptr) {
        unit = graphic->getUnit();
    }

    if (unit == RS2::Inch) {
    // RS_DEBUG->print("RS_Grid::update: 007");

    // auto scale grid
        //scale grid by drawing setting as well, bug#3416862

            double guiGridWithX = m_viewport->toGuiDX(gridWidth.x);
            double guiGridWithY = m_viewport->toGuiDY(gridWidth.y);
            const bool gridSmallerThanMin = guiGridWithX < minGridSpacing || guiGridWithY < minGridSpacing;
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
                    guiGridWithX = m_viewport->toGuiDX(gridWidth.x);
                    guiGridWithY = m_viewport->toGuiDY(gridWidth.y);
                }
                while (guiGridWithX < minGridSpacing  || guiGridWithY < minGridSpacing);
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
        m_metaGridWidth.x = 1.0;

        const int minGridSpacingX2 = minGridSpacing * 2;
        if (m_viewport->toGuiDX(m_metaGridWidth.x) < minGridSpacingX2) {
// .. or feet
            m_metaGridWidth.x = 12.0;

// .. or yards
            if (m_viewport->toGuiDX(m_metaGridWidth.x) < minGridSpacingX2) {
                m_metaGridWidth.x = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDX(metaGridWidth.x)<20) {
                // metaGridWidth.x = 63360.0;
                //}

                // .. or nothing
                if (m_viewport->toGuiDX(m_metaGridWidth.x) < minGridSpacingX2) {
                    m_metaGridWidth.x = -1.0;
                }

            }
        }

// RS_DEBUG->print("RS_Grid::update: 009");

// metagrid Y shows inches..
        m_metaGridWidth.y = 1.0;

        if (m_viewport->toGuiDY(m_metaGridWidth.y) < minGridSpacingX2) {
// .. or feet
            m_metaGridWidth.y = 12.0;

// .. or yards
            if (m_viewport->toGuiDY(m_metaGridWidth.y) < minGridSpacingX2) {
                m_metaGridWidth.y = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDY(metaGridWidth.y)<20) {
                // metaGridWidth.y = 63360.0;
                //}

                // .. or nothing
                if (m_viewport->toGuiDY(m_metaGridWidth.y) < minGridSpacingX2) {
                    m_metaGridWidth.y = -1.0;
                }
            }
        }
// RS_DEBUG->print("RS_Grid::update: 010");
    } else {
// RS_DEBUG->print("RS_Grid::update: 011");
        if (scaleGrid) {
            while (m_viewport->toGuiDX(gridWidth.x) < minGridSpacing) {
                gridWidth.x *= 2;
            }
            m_metaGridWidth.x = -1.0;

            while (m_viewport->toGuiDY(gridWidth.y) < minGridSpacing) {
                gridWidth.y *= 2;
            }
            m_metaGridWidth.y = -1.0;
        }
// RS_DEBUG->print("RS_Grid::update: 012");
    }
//gridWidth.y = gridWidth.x;
//metaGridWidth.y = metaGridWidth.x;
    return gridWidth;
}


QString RS_Grid::getInfo() const {
    return m_gridInfoString;
}

bool RS_Grid::isIsometric() const {
    return m_isometric;
}

void RS_Grid::setIsometric(const bool b) {
    m_isometric = b;
}

RS_Vector RS_Grid::getGridWidth() const {
    return m_gridWidth;
}

RS_Vector RS_Grid::getMetaGridWidth() const {
    return m_metaGridWidth;
}

const RS_Vector&RS_Grid::getCellVector() const {
    return m_gridSystem->getCellVector();
}

void RS_Grid::setIsoViewType(const RS2::IsoGridViewType chType) {
    m_isoViewType = chType;
}

RS2::IsoGridViewType RS_Grid::getIsoViewType() const {
    return m_isoViewType;
}


void RS_Grid::drawGrid(RS_Painter *painter) const {
    m_gridSystem->draw(painter, m_viewport);
}

void RS_Grid::invalidate(const bool gridOn) {
    if (m_gridSystem != nullptr) {
        m_gridSystem->invalidate();
        if (gridOn) {
            calculateGrid();
        }
        else{
            calculateSnapSettings();
        }
    }
}
