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
#include "grid/lc_lattice.h"

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
    ,baseGrid(false)
{}

/**
 * find the closest grid point
 *@return the closest grid to given point
 *@coord: the given point
 */
RS_Vector RS_Grid::snapGrid(const RS_Vector& coord) const {
    if( cellV.x<RS_TOLERANCE || cellV.y<RS_TOLERANCE) return coord;
    RS_Vector vp(coord-baseGrid);
    if(isometric){
//use remainder instead of fmod to locate the left-bottom corner for both positive and negative displacement
        RS_Vector vp1( vp-RS_Vector( remainder(vp.x-0.5*cellV.x,cellV.x)+0.5*cellV.x, remainder(vp.y-0.5*cellV.y,cellV.y)+0.5*cellV.y));
        RS_VectorSolutions sol({vp1,vp1+cellV,vp1+cellV*0.5, vp1+RS_Vector(cellV.x,0.), vp1+RS_Vector(0.,cellV.y)});
        vp1=sol.getClosest(vp);
        return baseGrid+vp1;

    }else{
        return baseGrid+vp-RS_Vector(remainder(vp.x,cellV.x),remainder(vp.y,cellV.y));
    }
}

/**
 * Updates the grid point array.
 */
void RS_Grid::updatePointArray(bool metagridVisible) {
    if (!graphicView->isGridOn()) return;

    RS_Graphic* graphic = graphicView->getGraphic();

    // auto scale grid?
    LC_GROUP("Appearance"); // fixme settings, FIX - REMOVE FROM PAINTING
    bool scaleGrid = LC_GET_BOOL("ScaleGrid", true);
    // get grid setting
    RS_Vector userGrid;
    if (graphic) {
    //$ISOMETRICGRID == $SNAPSTYLE
        isometric = static_cast<bool>(graphic->getVariableInt("$SNAPSTYLE",0));
        crosshairType=graphic->getCrosshairType();
        userGrid = graphic->getVariableVector("$GRIDUNIT",
                                              RS_Vector(-1.0, -1.0));
    }else {
        isometric = LC_GET_BOOL("IsometricGrid");
        crosshairType=static_cast<RS2::CrosshairType>(LC_GET_INT("CrosshairType", 0));
        userGrid.x = LC_GET_STR("GridSpacingX", QString("-1")).toDouble();
        userGrid.y = LC_GET_STR("GridSpacingY", QString("-1")).toDouble();
    }
    int minGridSpacing = LC_GET_INT("MinGridSpacing", 10);
    LC_GROUP_END();

    pt.clear();
    metaX.clear();
    metaY.clear();

    // find out unit:
    RS2::Unit unit = RS2::None;
    RS2::LinearFormat format = RS2::Decimal;
    if (graphic) {
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

    if (gridWidth.x>minimumGridWidth && gridWidth.y>minimumGridWidth &&
        graphicView->toGuiDX(gridWidth.x)>2 &&
        graphicView->toGuiDY(gridWidth.y)>2) {

        RS_Vector viewZero = graphicView->toGraph(0, 0);
        RS_Vector viewSize = graphicView->toGraph(graphicView->getWidth(), graphicView->getHeight());

        // find grid boundaries
        double left = (int)(viewZero.x / gridWidth.x) * gridWidth.x;
        double right = (int)(viewSize.x / gridWidth.x) * gridWidth.x;
        double top = (int)(viewZero.y / gridWidth.y) * gridWidth.y;
        double bottom = (int)(viewSize.y /gridWidth.y) * gridWidth.y;


        left -= gridWidth.x;
        right += gridWidth.x;
        top += gridWidth.y;
        bottom -= gridWidth.y;

        //top/bottom is reversed with RectF definition
        LC_Rect const rect{{left, bottom}, {right, top}};

        // populate grid points and metaGrid line positions: pts, metaX, metaY
        if(isometric){
            createIsometricGrid(rect,  gridWidth);
        }else{
            createOrthogonalGrid(rect, viewZero, viewSize,  gridWidth, metagridVisible);

        }
    }
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

#define DEBUG_ISO true

void RS_Grid::createOrthogonalGrid(LC_Rect const &rect,const RS_Vector &viewZero, const RS_Vector &viewSize, RS_Vector const &gridWidth, bool metaGridVisible) {
    int numMetaX = 0;
    int numMetaY = 0;

    double viewWidth = viewSize.x;
    double viewHeight = viewSize.y;

    // find meta grid boundaries
    double metaWidthX = metaGridWidth.x;
    double metaWidthY = metaGridWidth.y;
    double mViewOffsetX = 0;
    double mViewOffsetY = 0;
    double mbottom;
    RS_Vector baseMeta;
    if (metaWidthX > minimumGridWidth && metaWidthY > minimumGridWidth &&
        graphicView->toGuiDX(metaWidthX) > 2 &&
        graphicView->toGuiDY(metaWidthY) > 2) {

        double mleft = (int) (viewZero.x / metaWidthX) * metaWidthX;
        mViewOffsetX = viewZero.x - mleft;
        double mright = (int) (viewWidth / metaWidthX) * metaWidthX;

        double mtop = (int) (viewZero.y / metaWidthY) * metaWidthY;
        mbottom = (int) (viewHeight / metaWidthY) * metaWidthY;
        mViewOffsetX = viewHeight - mbottom;

        LC_ERR << "MLeft " << mleft << " z " << viewZero.x << " mvx " << metaWidthX << " uiWidth " << viewWidth;

// calculate number of visible meta grid lines:

        numMetaX = (RS_Math::round((mright-mleft) /  metaGridWidth.x) + 1);
        numMetaY = (RS_Math::round((mtop-mbottom) / metaGridWidth.y) + 1);

        baseMeta = RS_Vector(mleft, mbottom);

        LC_ERR << "X" << numMetaX << "  Y" << numMetaY;
        // ensure that vertical metaGrid lines are not outside of view (so all they are visible)
        if (viewZero.x > mleft){
            numMetaX --;
            mleft += metaWidthX;
        }

        double lastGridX = mleft + metaWidthX * (numMetaX - 1);
        if (lastGridX > (viewWidth)){
            numMetaX --;
        }
        LC_ERR << "numMetaX " << numMetaX << " lastGridX " << lastGridX;

        // ensure that horizontal metaGrid lines are within view and so visible
        if (viewHeight > mbottom){
            numMetaY --;
            mbottom+= metaWidthY;
        }

        if (viewZero.y < mtop){
            numMetaY --;
            mtop-= metaWidthY;
        }

        // fixme - check bounds that verticals are in bounds

        // create meta grid arrays:
        if (numMetaX > 0){
            metaX.resize(numMetaX);
            int i = 0;
            for (int x = 0; x < numMetaX; ++x) {
                metaX[i++] = mleft + x * metaWidthX;
            }
        }
        if (numMetaY > 0){
            int i = 0;
            metaY.resize(numMetaY);
            for (int y = 0; y < numMetaY; ++y) {
                metaY[i++] = mbottom + y * metaWidthY;
            }
        }
    }

    double const left = rect.minP().x;
    double const right = rect.maxP().x;
//top/bottom reversed
    double const top = rect.maxP().y;
    double const bottom = rect.minP().y;

    double gridX = gridWidth.x;
    double gridY = gridWidth.y;
    cellV.set(fabs(gridX), fabs(gridY));
    int numberX = (RS_Math::round((right - left) / gridX) + 1);
    int numberY = (RS_Math::round((top - bottom) / gridY) + 1);
    int number = numberX * numberY;
//todo, fix baseGrid for orthogonal grid
    baseGrid.set(left, bottom);

    double metaXFirst = 0;
    double metaXLast = 0;
    int numPointsInMetagridX = 0;
    int numPointsXLeft = 0;
    int numPointsXRight = 0;
    double baseNoVisibleMetaX;
    if (numMetaX > 0){
        numPointsInMetagridX = RS_Math::round(metaWidthX / gridX) - 1;
        metaXLast = metaX[numMetaX - 1];
        metaXFirst = metaX[0];
        numPointsXLeft = RS_Math::round((metaXFirst - left) / gridX) - 1;
        numPointsXRight = RS_Math::round((right - metaXLast) / gridX) - 1;
    }
    else{ // no metaGrid lines visible on screen at all
        numPointsXRight = RS_Math::round((right - left) / gridX) - 1;
        baseNoVisibleMetaX = viewZero.x-remainder(mViewOffsetX, gridX);
    }

    double metaYFirst = 0;
    double metaYLast = 0;
    int numPointsInMetagridY = 0;
    int numPointsYBottom =0;
    int numPointsYTop = 0;
    double baseNoVisibleMetaY;
    if (numMetaY > 0) {
        numPointsInMetagridY = RS_Math::round(metaWidthY / gridY) - 1;
        metaYFirst = metaY[0];
        metaYLast = metaY[numMetaY - 1];
        numPointsYBottom = RS_Math::round((metaYFirst - bottom) / gridY) - 1;
        numPointsYTop = RS_Math::round((top - metaYLast) / gridY) - 1;
    }
    else{
        numPointsYTop = RS_Math::round((top - bottom) / gridY) - 1;
        baseNoVisibleMetaY = bottom-remainder(mViewOffsetY, gridY);
    }

    int totalPointsX;
    int totalPointsY;
    int totalByMetaGrid;

    if (metaGridVisible) {
        totalPointsX = numPointsXLeft + numPointsInMetagridX * numMetaX + numPointsXRight;
        totalPointsY = numPointsYTop + numPointsInMetagridY * numMetaY + numPointsYBottom;
        totalByMetaGrid = totalPointsX * totalPointsY;
    } else {
        numPointsInMetagridX++;
        numPointsInMetagridY++;
        totalPointsX = numPointsXLeft + numPointsInMetagridX * numMetaX + numPointsXRight;
        totalPointsY = numPointsYTop + numPointsInMetagridY * numMetaY + numPointsYBottom;
        totalByMetaGrid = totalPointsX * totalPointsY + totalPointsX + totalPointsY;
    }

    // create grid array:

    if (totalByMetaGrid <= 0 || totalByMetaGrid > maxGridPoints) {
        return;
    }

    pt.resize(totalByMetaGrid);

    LC_Lattice lattice(0,0, gridWidth);
    lattice.init(totalByMetaGrid);

 /*   // fixme - remove
    RS_Vector baseGridPoint(metaX[0], metaY[0]);
//    fillRectByGridPoints(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, gridWidth);

    baseGridPoint = RS_Vector(metaX[0], metaY[1]);
    RS_Vector rowDelta{gridWidth.x*0.5, gridWidth.y*0.87};
    RS_Vector colDelta(gridWidth.x, 0);
//    fillGenericGridPointsLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, rowDelta, colDelta);

//    tmpFillLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, 0, 45, gridWidth);



    baseGridPoint = RS_Vector(metaX[0], metaY[0 ]);

    lattice.fill(numPointsInMetagridX, numPointsInMetagridY, baseGridPoint, false, false);
//    tmpFillLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, 30, 60, gridWidth);

    baseGridPoint = RS_Vector(metaX[2], metaY[1]);
    lattice.fillWithoutDiagonal(numPointsInMetagridX, numPointsInMetagridY, baseGridPoint, false, false, true, numPointsInMetagridY);
//    tmpFillLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, 0, 45, gridWidth);

    baseGridPoint = RS_Vector(metaX[3], metaY[0]);
    lattice.fillWithoutDiagonal(numPointsInMetagridX, numPointsInMetagridY, baseGridPoint, false, false, false, numPointsInMetagridY-1);
//    tmpFillLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, 0, 0, gridWidth);

    baseGridPoint = RS_Vector(metaX[3], metaY[0]);
//    tmpFillLattice(numPointsInMetagridX, numPointsInMetagridY, i, baseGridPoint, 0, -45, gridWidth);


*/
// ---
    RS_Vector delta;
    RS_Vector tileBasePoint;
    if (metaGridVisible) {

        // if metagrid is visible, we generate points with gaps, so there is no overlap between metagrid lines and points
        // to achieve this, we'll generate points for each individual cell of metagrid

        // first fill by points fully visible metagrid cells
        for (int mx = 0; mx < numMetaX - 1; ++mx) {
            for (int my = 0; my < numMetaY - 1; ++my) {
                RS_Vector baseGridPoint(metaX[mx], metaY[my]);
                lattice.fill(numPointsInMetagridX, numPointsInMetagridY, baseGridPoint, false, false);
            }
        }

        // fill top row
        tileBasePoint.y = metaYLast;
        for (int mx = 0; mx < numMetaX - 1; ++mx) {
            tileBasePoint.x = metaX[mx];
            lattice.fill(numPointsInMetagridX, numPointsYTop, tileBasePoint, false, false);
        }

        // fill right column
        tileBasePoint.x = metaXLast;
        for (int my = 0; my < numMetaY - 1; ++my) {
            tileBasePoint.y = metaY[my];
            lattice.fill(numPointsXRight, numPointsInMetagridY, tileBasePoint, false, false);
        }

        // fill right top corner
        tileBasePoint.y = metaYLast;
        if (numMetaX == 0){ // no vid
            tileBasePoint.x = baseNoVisibleMetaX;
        }
        if (numMetaY == 0){
            tileBasePoint.y = baseNoVisibleMetaY;
        }
        lattice.fill(numPointsXRight, numPointsYTop, tileBasePoint, false, false);

        // fill right bottom corner
        tileBasePoint.y = metaYFirst;
        if (numMetaX == 0){ // no vid
            tileBasePoint.x = baseNoVisibleMetaX;
        }
        lattice.fill(numPointsXRight, numPointsYBottom, tileBasePoint, false, true);


        // fill bottom row
        tileBasePoint.y = metaYFirst;
        for (int mx = 0; mx < numMetaX - 1; ++mx) {
            tileBasePoint.x = metaX[mx];
            lattice.fill(numPointsInMetagridX, numPointsYBottom, tileBasePoint, false, true);
        }

        // fill left column
        tileBasePoint.x = metaXFirst;
        for (int my = 0; my < numMetaY - 1; ++my) {
            tileBasePoint.y = metaY[my];
            lattice.fill(numPointsXLeft, numPointsInMetagridY, tileBasePoint, true, false);
        }

        // fill left bottom corner
        tileBasePoint.y = metaYFirst;
        lattice.fill(numPointsXLeft, numPointsYBottom, tileBasePoint, true, true);

        // fill left top corner

        tileBasePoint.y = metaYLast;
        if (numMetaY == 0){
            tileBasePoint.y = baseNoVisibleMetaY;
        }
        lattice.fill(numPointsXLeft, numPointsYTop, tileBasePoint, true, false);
    } else {

        // if metagrid is not visible, we can fill the entire area by points without gaps

        RS_Vector offset = lattice.getOffset(numPointsXLeft+1, numPointsYBottom+1);

        // leftmost bottom point of grid
        tileBasePoint = RS_Vector(metaXFirst, metaYFirst) - offset;

        // just fill whole area by points
        lattice.fill(totalPointsX, totalPointsY, tileBasePoint, false, false);
    }

    // fixme - temporary
    int pointsSize = lattice.getPointsCount();
    pt.resize(pointsSize);
    for (int i = 0; i < pointsSize;i++){
        pt[i] = lattice.getPoint(i);
    }
//    pt = lattice.getPoints();
    orthoGridMetrics.numPointsYBottom = numPointsYBottom;
    orthoGridMetrics.numPointsYTop = numPointsYTop;
    orthoGridMetrics.numPointsXLeft = numPointsXLeft;
    orthoGridMetrics.numPointsXRight = numPointsXRight;
    orthoGridMetrics.numPointsInMetagridX = numPointsInMetagridX;
    orthoGridMetrics.numPointsInMetagridY = numPointsInMetagridY;
    orthoGridMetrics.numMetaX = numMetaX;
    orthoGridMetrics.numMetaY = numMetaY;
    orthoGridMetrics.baseMetagridPoint = baseMeta;


//    LC_ERR << number << " allocated - actual "<< i << " total" << totalByMetaGrid;
//
//    RS_Vector bp0(baseGrid);
//    for (int y = 0; y < numberY; ++y) {
//        RS_Vector bp1(bp0);
//        for (int x = 0; x < numberX; ++x) {
//            pt[i++] = bp1;
//            bp1.x += gridWidth.x;
//        }
//        bp0.y += gridWidth.y;
//    }

}

void RS_Grid::fillRectByGridPoints(int numPointsByX, int numPointsByY, int &currentPointIndex, const RS_Vector &baseGridPoint, const RS_Vector& delta) {
//    RS_Vector bp0 = baseGridPoint + delta;
//    for (int y = 0; y < numPointsByY; ++y) {
//        RS_Vector bp1(bp0);
//        for (int x = 0; x < numPointsByX; ++x) {
//            pt[currentPointIndex++] = bp1;
//            bp1.x += delta.x;
//        }
//        bp0.y += delta.y;
//    }
      RS_Vector rowDelta{0, delta.y};
      RS_Vector colDelta(delta.x, 0);
    fillGenericGridPointsLattice(numPointsByX, numPointsByY, currentPointIndex, baseGridPoint, rowDelta, colDelta);
}

void RS_Grid::tmpFillLattice(int numpointsByX, int numPointsByY, int &currentPointIndex,  const RS_Vector &baseGridPoint,
                             double anglex, double angley, RS_Vector gridWidth){

    RS_Vector rDelta{0, gridWidth.y};
    rDelta.rotate(RS_Math::deg2rad(angley));

    RS_Vector cDelta{gridWidth.x, 0};
    cDelta.rotate(RS_Math::deg2rad(anglex));

    fillGenericGridPointsLattice(numpointsByX, numPointsByY, currentPointIndex, baseGridPoint, rDelta, cDelta);
}

void RS_Grid::fillGenericGridPointsLattice(int numPointsByX, int numPointsByY, int &currentPointIndex, const RS_Vector &baseGridPoint,
                                            const RS_Vector& rowDelta, const RS_Vector& columnDelta) {

    RS_Vector rowStartPoint = baseGridPoint + rowDelta + columnDelta;
    for (int y = 0; y < numPointsByY; ++y) {
        RS_Vector currentPoint(rowStartPoint);
        for (int x = 0; x < numPointsByX; ++x) {
            pt[currentPointIndex++] = currentPoint;
            currentPoint += columnDelta;
        }
        rowStartPoint+=rowDelta;
    }

//    bp0 = baseGridPoint + RS_Vector(200,0);
//
//    for (int y = 0; y < numPointsByY; ++y) {
//        RS_Vector bp1(bp0);
//        bp1.x += delta.x*y;
//        bp1.y += delta.y;
//        for (int x = 0; x < numPointsByX; ++x) {
//            pt[currentPointIndex++] = bp1;
////            pt[currentPointIndex++] = bp1 + secondDelta;
//            bp1.y += delta.y;
//            bp1.x += delta.x;
//        }
//        bp0.y += delta.y;
//    }
}


// fixme - modify building grid in such way that grid points are not drawn in metagrid positions
void RS_Grid::createIsometricGrid(LC_Rect const &rect, RS_Vector const &gridWidth) {

    double const left = rect.minP().x;
    double const right = rect.maxP().x;
//top/bottom reversed
    double const top = rect.maxP().y;
    double const bottom = rect.minP().y;

    //find metaGrid
    RS_Vector baseMetaGrid;
    double metaWidthY = metaGridWidth.y;
    if (metaWidthY > minimumGridWidth && graphicView->toGuiDY(metaWidthY) > 2) {
        double absMetaY = fabs(metaWidthY);

        double sqr3Y = sqrt(3.) * absMetaY;
        metaGridWidth.x = (metaGridWidth.x < 0.) ? -sqr3Y : sqr3Y;

        double metaWidthX = metaGridWidth.x;

        double absMetaX = fabs(metaWidthX);
        baseMetaGrid = RS_Vector(left + remainder(-left, metaWidthX) - absMetaX,
                               bottom + remainder(-bottom, metaWidthY) - absMetaY);

// calculate number of visible meta grid lines:
        int numMetaX = (RS_Math::round((right - left) / metaWidthX) + 1);
        int numMetaY = (RS_Math::round((top - bottom) / metaWidthY) + 1);

        if (numMetaX <= 0 || numMetaY <= 0) return;
// create meta grid arrays:
        metaX.resize(numMetaX);
        metaY.resize(numMetaY);

        double x0(baseMetaGrid.x);
        for (int i = 0; i < numMetaX; x0 += metaWidthX) {
            metaX[i++] = x0;
        }
        x0 = baseMetaGrid.y;
        for (int i = 0; i < numMetaY; x0 += metaWidthY) {
            metaY[i++] = x0;
        }


      orthoGridMetrics.numMetaX = numMetaX;
      orthoGridMetrics.numMetaY = numMetaY;
      orthoGridMetrics.baseMetagridPoint = baseMetaGrid;
   }

    int numberY = (RS_Math::round((top - bottom) / gridWidth.y) + 1);
    double dx = sqrt(3.) * gridWidth.y;
    cellV.set(fabs(dx), fabs(gridWidth.y));
    double hdx = 0.5 * dx;
    double hdy = 0.5 * gridWidth.y;
    int numberX = (RS_Math::round((right - left) / dx) + 1);
    int number = 2 * numberX * numberY;
    baseGrid.set(left + remainder(-left, dx), bottom + remainder(-bottom, fabs(gridWidth.y)));

    if (number <= 0 || number > maxGridPoints) {
        return;
    }

    pt.resize(number);

    int i = 0;
    RS_Vector bp0(baseGrid), dbp1(hdx, hdy);
    /*for (int y = 0; y < numberY; ++y) {
        RS_Vector bp1(bp0);
        for (int x = 0; x < numberX; ++x) {
            pt[i++] = bp1;
            pt[i++] = bp1 + dbp1;
            bp1.x += dx;
        }
        bp0.y += gridWidth.y;
    }*/

    fillGenericGridPointsLattice(9, 9, i, graphicView->toGui(RS_Vector(0,0)), RS_Vector(dx, gridWidth.y), dbp1);
    
    

}

QString RS_Grid::getInfo() const {
    return QString("%1 / %2").arg(spacing).arg(metaSpacing);
}

std::vector<RS_Vector> const &RS_Grid::getPoints() const {
    return pt;
}

std::vector<double> const &RS_Grid::getMetaX() const {
    return metaX;
}

std::vector<double> const &RS_Grid::getMetaY() const {
    return metaY;
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
    return cellV;
}

void RS_Grid::setCrosshairType(RS2::CrosshairType chType) {
    crosshairType = chType;
}

RS2::CrosshairType RS_Grid::getCrosshairType() const {
    return crosshairType;
}

const RS_Grid::OrthoGridMetrics &RS_Grid::getOrthoGridMetrics() const {
    return orthoGridMetrics;
}
