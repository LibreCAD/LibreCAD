/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#include "rs_grid.h"

#include "rs_units.h"
#include "rs_graphic.h"
#include "rs_settings.h"

/**
 * Constructor.
 */
RS_Grid::RS_Grid(RS_GraphicView* graphicView) {
    this->graphicView = graphicView;
    pt = NULL;
    number = 0;
    metaX = NULL;
    numMetaX = 0;
    metaY = NULL;
    numMetaY = 0;
}


/**
 * Destructor.
 */
RS_Grid::~RS_Grid() {
    if (pt!=NULL) {
        delete[] pt;
    }
    if (metaX!=NULL) {
        delete[] metaX;
    }
    if (metaY!=NULL) {
        delete[] metaY;
    }
}


/**
 * Updates the grid point array.
 */
void RS_Grid::updatePointArray() {
    RS_DEBUG->print("RS_Grid::update");
    if (graphicView->isGridOn()) {

        RS_Graphic* graphic = graphicView->getGraphic();

        // auto scale grid?
        RS_SETTINGS->beginGroup("/Appearance");
        bool scaleGrid = (bool)RS_SETTINGS->readNumEntry("/ScaleGrid", 1);
        int minGridSpacing = RS_SETTINGS->readNumEntry("/MinGridSpacing", 10);
        RS_SETTINGS->endGroup();
		
        // get grid setting
		RS_Vector userGrid;
        if (graphic!=NULL) {
            userGrid = graphic->getVariableVector("$GRIDUNIT", 
				RS_Vector(-1.0, -1.0));
        }

        // delete old grid:
        if (pt!=NULL) {
            delete[] pt;
            pt = NULL;
        }
        if (metaX!=NULL) {
            delete[] metaX;
            metaX = NULL;
        }
        if (metaY!=NULL) {
            delete[] metaY;
            metaY = NULL;
        }
		number = 0;
		numMetaX = 0;
		numMetaY = 0;
		
    	RS_DEBUG->print("RS_Grid::update: 001");

        // find out unit:
        RS2::Unit unit = RS2::None;
        RS2::LinearFormat format = RS2::Decimal;
        if (graphic!=NULL) {
            unit = graphic->getUnit();
            format = graphic->getLinearFormat();
        }

        RS_Vector gridWidth;
        RS_Vector metaGridWidth;
		
    	RS_DEBUG->print("RS_Grid::update: 002");

        // init grid spacing:
        // metric grid:
        if (RS_Units::isMetric(unit) || unit==RS2::None ||
                format==RS2::Decimal || format==RS2::Engineering) {

			if (userGrid.x>0.0) {
				gridWidth.x = userGrid.x;
			}
			else {
            	gridWidth.x = 0.000001;
			}
			
			if (userGrid.y>0.0) {
				gridWidth.y = userGrid.y;
			}
			else {
            	gridWidth.y = 0.000001;
			}
			
    		RS_DEBUG->print("RS_Grid::update: 003");
			
            // auto scale grid
            if (scaleGrid) {
                while (graphicView->toGuiDX(gridWidth.x)<minGridSpacing) {
                    gridWidth.x*=10;
                }
                while (graphicView->toGuiDY(gridWidth.y)<minGridSpacing) {
                    gridWidth.y*=10;
                }
            }
            metaGridWidth.x = gridWidth.x*10;
            metaGridWidth.y = gridWidth.y*10;
			
    		RS_DEBUG->print("RS_Grid::update: 004");
        }

        // imperial grid:
        else {
		
    		RS_DEBUG->print("RS_Grid::update: 005");
			
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
    		RS_DEBUG->print("RS_Grid::update: 006");

            if (unit==RS2::Inch) {
    			RS_DEBUG->print("RS_Grid::update: 007");
				
                // auto scale grid
                if (scaleGrid) {
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
				
    			RS_DEBUG->print("RS_Grid::update: 008");

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
                        //    metaGridWidth.x = 63360.0;
                        //}

                        // .. or nothing
                        if (graphicView->toGuiDX(metaGridWidth.x)<minGridSpacing*2) {
                            metaGridWidth.x = -1.0;
                        }

                    }
                }
				
    			RS_DEBUG->print("RS_Grid::update: 009");
				
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
                        //    metaGridWidth.y = 63360.0;
                        //}

                        // .. or nothing
                        if (graphicView->toGuiDY(metaGridWidth.y)<minGridSpacing*2) {
                            metaGridWidth.y = -1.0;
                        }

                    }
                }

    			RS_DEBUG->print("RS_Grid::update: 010");
            } else {
			
    			RS_DEBUG->print("RS_Grid::update: 011");
				
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
    			RS_DEBUG->print("RS_Grid::update: 012");
            }
            //gridWidth.y = gridWidth.x;
            //metaGridWidth.y = metaGridWidth.x;
        }
		
    	RS_DEBUG->print("RS_Grid::update: 013");

        // for grid info:
        spacing = gridWidth.x;
        metaSpacing = metaGridWidth.x;

        if (gridWidth.x>1.0e-6 && gridWidth.y>1.0e-6 && 
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


            // calculate number of visible grid points
            int numberX = (RS_Math::round((right-left) / gridWidth.x) + 1);
            int numberY = (RS_Math::round((top-bottom) / gridWidth.y) + 1);
            number = numberX*numberY;

    		RS_DEBUG->print("RS_Grid::update: 014");

            // create grid array:
            if (number>0 && number<1000000) {

                pt = new RS_Vector[number];

                int i=0;
                for (int y=0; y<numberY; ++y) {
                    for (int x=0; x<numberX; ++x) {
                        pt[i++] = RS_Vector(left+x*gridWidth.x,
                                            bottom+y*gridWidth.y);
                    }
                }
            } else {
                number = 0;
				pt = NULL;
            }
			
    		RS_DEBUG->print("RS_Grid::update: 015");
        }


        // find meta grid boundaries
        if (metaGridWidth.x>1.0e-6 && metaGridWidth.y>1.0e-6 && 
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
            numMetaX = (RS_Math::round((mright-mleft) / metaGridWidth.x) + 1);
            numMetaY = (RS_Math::round((mtop-mbottom) / metaGridWidth.y) + 1);

            if (numMetaX>0 && numMetaY>0) {
                // create meta grid arrays:
                metaX = new double[numMetaX];
                metaY = new double[numMetaY];

                int i=0;
                for (int x=0; x<numMetaX; ++x) {
                    metaX[i++] = mleft+x*metaGridWidth.x;
                }
                i=0;
                for (int y=0; y<numMetaY; ++y) {
                    metaY[i++] = mbottom+y*metaGridWidth.y;
                }
            }
			else {
				numMetaX = 0;
				metaX = NULL;
				numMetaY = 0;
				metaY = NULL;
			}
        }
    }
    RS_DEBUG->print("RS_Grid::update: OK");
}


// EOF
