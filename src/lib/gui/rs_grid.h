/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#ifndef RS_GRID_H
#define RS_GRID_H

#include "rs_graphicview.h"
#include "rs_vector.h"

/**
 * This class represents a grid. Grids can be drawn on graphic
 * views and snappers can snap to the grid points.
 *
 * @author Andrew Mustun
 */
class RS_Grid {
public:
    RS_Grid(RS_GraphicView* graphicView);
    ~RS_Grid();

    void update();
	
	/**
	 * @return Array of all visible grid points.
	 */
    RS_Vector* getPoints() {
        return pt;
    }
	
	/**
	 * @return Number of visible grid points.
	 */
    int count() {
        return number;
    }

	/**
	 * @return Current grid spacing.
	 */
	//double getSpacing() {
	//	return spacing;
	//}
	
	/**
	 * @return Current meta grid spacing.
	 */
	//double getMetaSpacing() {
	//	return metaSpacing;
	//}

	/**
	 * @return Grid info for status widget.
	 */
	RS_String getInfo() {
		return RS_String("%1 / %2").arg(spacing).arg(metaSpacing);
	}

	/**
	 * @return Meta grid positions in X.
	 */
	double* getMetaX() {
		return metaX;
	}
	
	/**
	 * @return Number of visible meta grid lines in X.
	 */
    int countMetaX() {
        return numMetaX;
    }
	
	/**
	 * @return Meta grid positions in Y.
	 */
	double* getMetaY() {
		return metaY;
	}
	
	/**
	 * @return Number of visible meta grid lines in Y.
	 */
    int countMetaY() {
        return numMetaY;
    }

protected:
    //! Graphic view this grid is connected to.
    RS_GraphicView* graphicView;
	
	//! Current grid spacing
	double spacing;
	//! Current meta grid spacing
	double metaSpacing;
	
    //! Pointer to array of grid points
    RS_Vector* pt;
    //! Number of points in the array
    int number;
	//! Meta grid positions in X
	double* metaX;
	//! Number of meta grid lines in X
	int numMetaX;
	//! Meta grid positions in Y
	double* metaY;
	//! Number of meta grid lines in Y
	int numMetaY;
};

#endif
