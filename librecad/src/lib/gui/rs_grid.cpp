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
    // auto scale grid?
    LC_GROUP("Appearance");
    scaleGrid = LC_GET_BOOL("ScaleGrid", true);
    // get grid setting
    RS_Graphic* graphic = graphicView->getGraphic();
    if (graphic != nullptr) {
        isometric = graphic->isIsometricGrid();
        isoViewType = graphic->getIsoView();
        userGrid = graphic->getVariableVector("$GRIDUNIT", RS_Vector(-1.0, -1.0));
    }else { // fixme - review, isoView
        isometric = LC_GET_ONE_BOOL("Defaults", "IsometricGrid");
        isoViewType=static_cast<RS2::IsoGridViewType>(LC_GET_ONE_INT("Defaults", "IsoGridView", 0));
        userGrid.x = LC_GET_STR("GridSpacingX", QString("-1")).toDouble();
        userGrid.y = LC_GET_STR("GridSpacingY", QString("-1")).toDouble();
    }

    bool showMetaGrid = LC_GET_BOOL("metaGridShow", true);
    bool simpleGridRendering = LC_GET_BOOL("GridRenderSimple", false);
    minGridSpacing = LC_GET_INT("MinGridSpacing", 10);
    int gridType = LC_GET_INT("GridType", 0);
    bool linesGrid = gridType == 1;
    
    RS2::LineType metagridLineType;
    int metaGridWidthPx;
    RS2::LineType gridLineType;
    int gridWidthPx;

    if (linesGrid){
        metagridLineType =  static_cast<RS2::LineType> (LC_GET_INT("metaGridLinesLineType", RS2::SolidLine));
        metaGridWidthPx = LC_GET_INT("metaGridLinesLineWidth", 1);
        gridWidthPx = LC_GET_INT("GridLineLinesWidth", 1);
        gridLineType =  static_cast<RS2::LineType> (LC_GET_INT("GridLineType", RS2::SolidLine));
    }
    else{
        metagridLineType =  static_cast<RS2::LineType> (LC_GET_INT("metaGridLineType", RS2::DotLineTiny));
        metaGridWidthPx = LC_GET_INT("metaGridPointsLineWidth", 1);
        gridWidthPx = LC_GET_INT("GridLinePointsWidth", 1);
        gridLineType =  static_cast<RS2::LineType> (LC_GET_INT("GridLineType", RS2::SolidLine));
    }

    bool disableGridOnPanning = LC_GET_BOOL("GridDisableWithinPan", false);

    bool drawIsoVerticalForTop = LC_GET_BOOL("GridDrawIsoVerticalForTop", true);
    LC_GROUP_END();

    LC_GROUP("Colors");
    RS_Color metaGridColor;
    RS_Color gridColor;
     if (linesGrid) {
         metaGridColor= QColor(LC_GET_STR("meta_grid_lines", RS_Settings::color_meta_grid_lines));
         gridColor = QColor(LC_GET_STR("gridLines", RS_Settings::color_meta_grid_lines));
     }
     else{
         metaGridColor= QColor(LC_GET_STR("meta_grid", RS_Settings::color_meta_grid_points));
         gridColor = QColor(LC_GET_STR("grid", RS_Settings::color_meta_grid_points));
     }
    LC_GROUP_END();

    auto* gridOptions = new LC_GridSystem::LC_GridOptions();
    gridOptions->drawMetaGrid = showMetaGrid;
    gridOptions->simpleGridRendering = simpleGridRendering;
    gridOptions->gridWidthPx = gridWidthPx;
    gridOptions->gridLineType = gridLineType;
    gridOptions->gridVisible = true;
    gridOptions->drawLines = linesGrid;
    gridOptions->gridColor = gridColor;
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

/**
 * Updates the grid point array.
 */
void RS_Grid::calculateGrid() {
    if (!graphicView->isGridOn()){
        return;
    }

    RS_Vector gridWidth = prepareGridWidth();

    if (gridWidth.x>minimumGridWidth && gridWidth.y>minimumGridWidth &&
        graphicView->toGuiDX(gridWidth.x)>2 &&
        graphicView->toGuiDY(gridWidth.y)>2) {

        RS_Vector viewZero = graphicView->toGraph(0, 0);
        RS_Vector viewSize = graphicView->toGraph(graphicView->getWidth(), graphicView->getHeight());

        // populate grid points and metaGrid line positions: pts, metaX, metaY

        gridSystem->createGrid(graphicView, viewZero, viewSize, metaGridWidth, gridWidth);
    }
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
    // for grid info:
    spacing = gridWidth.x;
    metaSpacing = metaGridWidth.x;

    return gridWidth;
}


RS_Vector RS_Grid::getMetricGridWidth(RS_Vector const &userGrid, bool scaleGrid, int minGridSpacing) {
    RS_Vector gridWidth;

    if (userGrid.x > 0.0) {
        gridWidth.x = userGrid.x;
    } else {
        gridWidth.x = minimumGridWidth;
    }

    if (userGrid.y > 0.0) {
        gridWidth.y = userGrid.y;
    } else {
        gridWidth.y = minimumGridWidth;
    }
    // auto scale grid
    //scale grid by drawing setting as well, bug#3416862
    // std::cout<<"RS_Grid::updatePointArray(): userGrid="<<userGrid<<std::endl;
    if (scaleGrid || userGrid.x <= minimumGridWidth || userGrid.y <= minimumGridWidth) {
        if (scaleGrid || userGrid.x <= minimumGridWidth) {
            while (graphicView->toGuiDX(gridWidth.x) < minGridSpacing) {
                gridWidth.x *= 10;
            }
        }
        if (scaleGrid || userGrid.y <= minimumGridWidth) {
            while (graphicView->toGuiDY(gridWidth.y) < minGridSpacing) {
                gridWidth.y *= 10;
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
    RS_Vector gridWidth;
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
//	RS2::LinearFormat format = RS2::Decimal;
    RS_Graphic *graphic = graphicView->getGraphic();

    if (graphic) {
        unit = graphic->getUnit();
    //		format = graphic->getLinearFormat();
    }

    if (unit == RS2::Inch) {
    // RS_DEBUG->print("RS_Grid::update: 007");

    // auto scale grid
        //scale grid by drawing setting as well, bug#3416862
        if (scaleGrid || userGrid.x <= minimumGridWidth || userGrid.y <= minimumGridWidth) {
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
        }

// RS_DEBUG->print("RS_Grid::update: 008");

// metagrid X shows inches..
        metaGridWidth.x = 1.0;

        if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacing * 2) {
// .. or feet
            metaGridWidth.x = 12.0;

// .. or yards
            if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacing * 2) {
                metaGridWidth.x = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDX(metaGridWidth.x)<20) {
                // metaGridWidth.x = 63360.0;
                //}

                // .. or nothing
                if (graphicView->toGuiDX(metaGridWidth.x) < minGridSpacing * 2) {
                    metaGridWidth.x = -1.0;
                }

            }
        }

// RS_DEBUG->print("RS_Grid::update: 009");

// metagrid Y shows inches..
        metaGridWidth.y = 1.0;

        if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacing * 2) {
// .. or feet
            metaGridWidth.y = 12.0;

// .. or yards
            if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacing * 2) {
                metaGridWidth.y = 36.0;

                // .. or miles (not really..)
                //if (graphicView->toGuiDY(metaGridWidth.y)<20) {
                // metaGridWidth.y = 63360.0;
                //}

                // .. or nothing
                if (graphicView->toGuiDY(metaGridWidth.y) < minGridSpacing * 2) {
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
    return QString("%1 / %2").arg(spacing).arg(metaSpacing);
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
