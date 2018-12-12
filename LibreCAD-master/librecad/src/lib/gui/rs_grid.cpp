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
void RS_Grid::updatePointArray() {
	if (!graphicView->isGridOn()) return;

	RS_Graphic* graphic = graphicView->getGraphic();

	// auto scale grid?
	RS_SETTINGS->beginGroup("/Appearance");
	bool scaleGrid = (bool)RS_SETTINGS->readNumEntry("/ScaleGrid", 1);
	// get grid setting
	RS_Vector userGrid;
	if (graphic) {
		//$ISOMETRICGRID == $SNAPSTYLE
		isometric = static_cast<bool>(graphic->getVariableInt("$SNAPSTYLE",0));
		crosshairType=graphic->getCrosshairType();
		userGrid = graphic->getVariableVector("$GRIDUNIT",
											 RS_Vector(-1.0, -1.0));
	}else {
		isometric = (bool)RS_SETTINGS->readNumEntry("/IsometricGrid", 0);
		crosshairType=static_cast<RS2::CrosshairType>(RS_SETTINGS->readNumEntry("/CrosshairType",0));
		userGrid.x = RS_SETTINGS->readEntry("/GridSpacingX",QString("-1")).toDouble();
		userGrid.y = RS_SETTINGS->readEntry("/GridSpacingY",QString("-1")).toDouble();
	}
	int minGridSpacing = RS_SETTINGS->readNumEntry("/MinGridSpacing", 10);
	RS_SETTINGS->endGroup();

	// std::cout<<"Grid userGrid="<<userGrid<<std::endl;

	pt.clear();
	metaX.clear();
	metaY.clear();

	// RS_DEBUG->print("RS_Grid::update: 001");

	// find out unit:
	RS2::Unit unit = RS2::None;
	RS2::LinearFormat format = RS2::Decimal;
	if (graphic) {
		unit = graphic->getUnit();
		format = graphic->getLinearFormat();
	}

	RS_Vector gridWidth;
	// RS_Vector metaGridWidth;

	// RS_DEBUG->print("RS_Grid::update: 002");

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

	// RS_DEBUG->print("RS_Grid::update: 013");

	// for grid info:
	spacing = gridWidth.x;
	metaSpacing = metaGridWidth.x;
	//std::cout<<"Grid spacing="<<spacing<<std::endl;
	//std::cout<<"Grid metaSpacing="<<metaSpacing<<std::endl;

	if (gridWidth.x>minimumGridWidth && gridWidth.y>minimumGridWidth &&
			graphicView->toGuiDX(gridWidth.x)>2 &&
			graphicView->toGuiDY(gridWidth.y)>2) {

		// find grid boundaries
		double left = (int)(graphicView->toGraphX(0) / gridWidth.x)
				* gridWidth.x;
		double right = (int)(graphicView->toGraphX(graphicView->getWidth()) /
							 gridWidth.x) * gridWidth.x;
		double top = (int)(graphicView->toGraphY(0) /
						 gridWidth.y) * gridWidth.y;
		double bottom =
				(int)(graphicView->toGraphY(graphicView->getHeight()) /
					 gridWidth.y) * gridWidth.y;


		left -= gridWidth.x;
		right += gridWidth.x;
		top += gridWidth.y;
		bottom -= gridWidth.y;

		//top/bottom is reversed with RectF definition
		LC_Rect const rect{{left, bottom}, {right, top}};

		// populate grid points and metaGrid line positions: pts, metaX, metaY
		if(isometric){
			createIsometricGrid(rect, gridWidth);
		}else{
			createOrthogonalGrid(rect, gridWidth);

		}

		// RS_DEBUG->print("RS_Grid::update: 015");
	}

	// RS_DEBUG->print("RS_Grid::update: OK");
}


RS_Vector RS_Grid::getMetricGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing)
{
	RS_Vector gridWidth;

	if (userGrid.x>0.0) {
		gridWidth.x = userGrid.x;
	}
	else {
		gridWidth.x = minimumGridWidth;
	}

	if (userGrid.y>0.0) {
		gridWidth.y = userGrid.y;
	}
	else {
		gridWidth.y = minimumGridWidth;
	}

	// RS_DEBUG->print("RS_Grid::update: 003");

	// auto scale grid
	//scale grid by drawing setting as well, bug#3416862
	// std::cout<<"RS_Grid::updatePointArray(): userGrid="<<userGrid<<std::endl;
	if (scaleGrid|| userGrid.x<=minimumGridWidth || userGrid.y<=minimumGridWidth) {
		if(scaleGrid || userGrid.x<=minimumGridWidth) {
			while (graphicView->toGuiDX(gridWidth.x)<minGridSpacing) {
				gridWidth.x*=10;
			}
		}
		if(scaleGrid || userGrid.y<=minimumGridWidth) {
			while (graphicView->toGuiDY(gridWidth.y)<minGridSpacing) {
				gridWidth.y*=10;
			}
		}
	}
	// std::cout<<"RS_Grid::updatePointArray(): gridWidth="<<gridWidth<<std::endl;
	metaGridWidth.x = gridWidth.x*10;
	metaGridWidth.y = gridWidth.y*10;

	// RS_DEBUG->print("RS_Grid::update: 004");
	return gridWidth;
}

RS_Vector RS_Grid::getImperialGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing)
{
	RS_Vector gridWidth;
	// RS_DEBUG->print("RS_Grid::update: 005");

	if (userGrid.x>0.0) {
		gridWidth.x = userGrid.x;
	}
	else {
		gridWidth.x = 1.0/1024.0;
	}

	if (userGrid.y>0.0) {
		gridWidth.y = userGrid.y;
	}
	else {
		gridWidth.y = 1.0/1024.0;
	}
	// RS_DEBUG->print("RS_Grid::update: 006");

	RS2::Unit unit = RS2::None;
//	RS2::LinearFormat format = RS2::Decimal;
	RS_Graphic* graphic = graphicView->getGraphic();

	if (graphic) {
		unit = graphic->getUnit();
//		format = graphic->getLinearFormat();
	}

	if (unit==RS2::Inch) {
		// RS_DEBUG->print("RS_Grid::update: 007");

		// auto scale grid
		//scale grid by drawing setting as well, bug#3416862
		if (scaleGrid|| userGrid.x<=minimumGridWidth || userGrid.y<=minimumGridWidth) {
			if(scaleGrid || userGrid.x<=minimumGridWidth) {
				while (graphicView->toGuiDX(gridWidth.x)<minGridSpacing) {
					if (RS_Math::round(gridWidth.x)>=36) {
						gridWidth.x*=2;
					} else if (RS_Math::round(gridWidth.x)>=12) {
						gridWidth.x*=3;
					} else if (RS_Math::round(gridWidth.x)>=4) {
						gridWidth.x*=3;
					} else if (RS_Math::round(gridWidth.x)>=1) {
						gridWidth.x*=2;
					} else {
						gridWidth.x*=2;
					}
				}
			}
			if(scaleGrid || userGrid.y<=minimumGridWidth) {
				while (graphicView->toGuiDY(gridWidth.y)<minGridSpacing) {
					if (RS_Math::round(gridWidth.y)>=36) {
						gridWidth.y*=2;
					} else if (RS_Math::round(gridWidth.y)>=12) {
						gridWidth.y*=3;
					} else if (RS_Math::round(gridWidth.y)>=4) {
						gridWidth.y*=3;
					} else if (RS_Math::round(gridWidth.y)>=1) {
						gridWidth.y*=2;
					} else {
						gridWidth.y*=2;
					}
				}
			}
		}

		// RS_DEBUG->print("RS_Grid::update: 008");

		// metagrid X shows inches..
		metaGridWidth.x = 1.0;

		if (graphicView->toGuiDX(metaGridWidth.x)<minGridSpacing*2) {
			// .. or feet
			metaGridWidth.x = 12.0;

			// .. or yards
			if (graphicView->toGuiDX(metaGridWidth.x)<minGridSpacing*2) {
				metaGridWidth.x = 36.0;

				// .. or miles (not really..)
				//if (graphicView->toGuiDX(metaGridWidth.x)<20) {
				// metaGridWidth.x = 63360.0;
				//}

				// .. or nothing
				if (graphicView->toGuiDX(metaGridWidth.x)<minGridSpacing*2) {
					metaGridWidth.x = -1.0;
				}

			}
		}

		// RS_DEBUG->print("RS_Grid::update: 009");

		// metagrid Y shows inches..
		metaGridWidth.y = 1.0;

		if (graphicView->toGuiDY(metaGridWidth.y)<minGridSpacing*2) {
			// .. or feet
			metaGridWidth.y = 12.0;

			// .. or yards
			if (graphicView->toGuiDY(metaGridWidth.y)<minGridSpacing*2) {
				metaGridWidth.y = 36.0;

				// .. or miles (not really..)
				//if (graphicView->toGuiDY(metaGridWidth.y)<20) {
				// metaGridWidth.y = 63360.0;
				//}

				// .. or nothing
				if (graphicView->toGuiDY(metaGridWidth.y)<minGridSpacing*2) {
					metaGridWidth.y = -1.0;
				}
			}
		}
		// RS_DEBUG->print("RS_Grid::update: 010");
	} else {
		// RS_DEBUG->print("RS_Grid::update: 011");
		if (scaleGrid) {
			while (graphicView->toGuiDX(gridWidth.x)<minGridSpacing) {
				gridWidth.x*=2;
			}
			metaGridWidth.x = -1.0;

			while (graphicView->toGuiDY(gridWidth.y)<minGridSpacing) {
				gridWidth.y*=2;
			}
			metaGridWidth.y = -1.0;
		}
		// RS_DEBUG->print("RS_Grid::update: 012");
	}
	//gridWidth.y = gridWidth.x;
	//metaGridWidth.y = metaGridWidth.x;
	return gridWidth;
}


void RS_Grid::createOrthogonalGrid(LC_Rect const& rect, RS_Vector const& gridWidth)
{
	double const left=rect.minP().x;
	double const right=rect.maxP().x;
	//top/bottom reversed
	double const top=rect.maxP().y;
	double const bottom=rect.minP().y;

	cellV.set(fabs(gridWidth.x),fabs(gridWidth.y));
	int numberX = (RS_Math::round((right-left) / gridWidth.x) + 1);
	int numberY = (RS_Math::round((top-bottom) / gridWidth.y) + 1);
	int number = numberX*numberY;
	//todo, fix baseGrid for orthogonal grid
	baseGrid.set(left,bottom);

	// create grid array:

	if (number<=0 || number>maxGridPoints) return;

	pt.resize(number);

	int i=0;
	RS_Vector bp0(baseGrid);
	for (int y=0; y<numberY; ++y) {
		RS_Vector bp1(bp0);
		for (int x=0; x<numberX; ++x) {
			pt[i++] = bp1;
			bp1.x += gridWidth.x;
		}
		bp0.y += gridWidth.y;
	}
	// find meta grid boundaries
	if (metaGridWidth.x>minimumGridWidth && metaGridWidth.y>minimumGridWidth &&
			graphicView->toGuiDX(metaGridWidth.x)>2 &&
			graphicView->toGuiDY(metaGridWidth.y)>2) {

		double mleft = (int)(graphicView->toGraphX(0) /
							 metaGridWidth.x) * metaGridWidth.x;
		double mright = (int)(graphicView->toGraphX(graphicView->getWidth()) /
							 metaGridWidth.x) * metaGridWidth.x;
		double mtop = (int)(graphicView->toGraphY(0) /
							metaGridWidth.y) * metaGridWidth.y;
		double mbottom = (int)(graphicView->toGraphY(graphicView->getHeight()) /
							 metaGridWidth.y) * metaGridWidth.y;

		mleft -= metaGridWidth.x;
		mright += metaGridWidth.x;
		mtop += metaGridWidth.y;
		mbottom -= metaGridWidth.y;

		// calculate number of visible meta grid lines:
		int numMetaX = (RS_Math::round((mright-mleft) / metaGridWidth.x) + 1);
		int numMetaY = (RS_Math::round((mtop-mbottom) / metaGridWidth.y) + 1);

		if (numMetaX<=0 || numMetaY<=0) return;
		// create meta grid arrays:
		metaX.resize(numMetaX);
		metaY.resize(numMetaY);

		int i=0;
		for (int x=0; x<numMetaX; ++x) {
			metaX[i++] = mleft+x*metaGridWidth.x;
		}
		i=0;
		for (int y=0; y<numMetaY; ++y) {
			metaY[i++] = mbottom+y*metaGridWidth.y;
		}
	}
}

void RS_Grid::createIsometricGrid(LC_Rect const& rect, RS_Vector const& gridWidth)
{
	double const left=rect.minP().x;
	double const right=rect.maxP().x;
	//top/bottom reversed
	double const top=rect.maxP().y;
	double const bottom=rect.minP().y;
	int numberY = (RS_Math::round((top-bottom) / gridWidth.y) + 1);
	double dx=sqrt(3.)*gridWidth.y;
	cellV.set(fabs(dx),fabs(gridWidth.y));
	double hdx=0.5*dx;
	double hdy=0.5*gridWidth.y;
	int numberX = (RS_Math::round((right-left) / dx) + 1);
	int number = 2*numberX*numberY;
	baseGrid.set(left+remainder(-left,dx),bottom+remainder(-bottom,fabs(gridWidth.y)));

	if (number<=0 || number>maxGridPoints) return;

	pt.resize(number);

	int i=0;
	RS_Vector bp0(baseGrid),dbp1(hdx,hdy);
	for (int y=0; y<numberY; ++y) {
		RS_Vector bp1(bp0);
		for (int x=0; x<numberX; ++x) {
			pt[i++] = bp1;
			pt[i++] = bp1+dbp1;
			bp1.x += dx;
		}
		bp0.y += gridWidth.y;
	}
	//find metaGrid
	if (metaGridWidth.y>minimumGridWidth &&
			graphicView->toGuiDY(metaGridWidth.y)>2) {

		metaGridWidth.x=(metaGridWidth.x<0.)?-sqrt(3.)*fabs(metaGridWidth.y):sqrt(3.)*fabs(metaGridWidth.y);
		RS_Vector baseMetaGrid(left+remainder(-left,metaGridWidth.x)-fabs(metaGridWidth.x),bottom+remainder(-bottom,metaGridWidth.y)-fabs(metaGridWidth.y));

		// calculate number of visible meta grid lines:
		int numMetaX = (RS_Math::round((right-left) / metaGridWidth.x) + 1);
		int numMetaY = (RS_Math::round((top-bottom) / metaGridWidth.y) + 1);

		if (numMetaX<=0 || numMetaY<=0) return;
		// create meta grid arrays:
		metaX.resize(numMetaX);
		metaY.resize(numMetaY);

		double x0(baseMetaGrid.x);
		for (int i=0; i<numMetaX; x0 += metaGridWidth.x) {
			metaX[i++] = x0;
		}
		x0=baseMetaGrid.y;
		for (int i=0; i<numMetaY; x0 += metaGridWidth.y) {
			metaY[i++] = x0;
		}
	}
}

QString RS_Grid::getInfo() const{
	return QString("%1 / %2").arg(spacing).arg(metaSpacing);
}

std::vector<RS_Vector> const& RS_Grid::getPoints() const{
	return pt;
}

std::vector<double> const& RS_Grid::getMetaX() const{
	return metaX;
}

std::vector<double> const& RS_Grid::getMetaY() const{
	return metaY;
}

bool RS_Grid::isIsometric() const{
	return isometric;
}

void RS_Grid::setIsometric(bool b){
	isometric=b;
}

RS_Vector RS_Grid::getMetaGridWidth() const {
	return metaGridWidth;
}

RS_Vector const& RS_Grid::getCellVector() const
{
	return cellV;
}

void RS_Grid::setCrosshairType(RS2::CrosshairType chType){
	crosshairType=chType;
}
RS2::CrosshairType RS_Grid::getCrosshairType() const{
	return crosshairType;
}

// EOF
