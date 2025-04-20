/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright(C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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

#include <QString>

#include "rs.h"
#include "rs_vector.h"

class RS_Painter;
class RS_GraphicView;
class LC_GraphicViewport;
class LC_GridSystem;

class QString;
namespace lc {
	namespace geo {
		class Area;
	}
}

using LC_Rect = lc::geo::Area;


/**
 * This class represents a grid. Grids can be drawn on graphic
 * views and snappers can snap to the grid points.
 *
 * @author Andrew Mustun
 */
class RS_Grid{
public:

	RS_Grid(LC_GraphicViewport* graphicView);
	void calculateGrid();
	void calculateSnapSettings();
	void invalidate(bool gridOn);

	/**
	* \brief the closest grid point
	* \return the closest grid to given point
	* \param coord the given point
	*/
	RS_Vector snapGrid(const RS_Vector& coord) const;

	void setIsoViewType(RS2::IsoGridViewType chType);
	RS2::IsoGridViewType getIsoViewType() const;

	/**
		 * @return Grid info for status widget.
		 */
	QString getInfo() const;
	bool isIsometric() const;
	void setIsometric(bool b);
	RS_Vector getMetaGridWidth() const;
	RS_Vector const &getCellVector() const;
	void loadSettings();
	void drawGrid(RS_Painter *painter);
private:
	//! copy ctor disabled
    RS_Grid(RS_Grid const&) = delete;
    RS_Grid& operator = (RS_Grid const&) = delete;
    //! \{ \brief determine grid width
    RS_Vector getMetricGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing);
    RS_Vector getImperialGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing);
    void prepareGridCalculations(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse);
    //! \}

    //! Graphic view this grid is connected to.
    LC_GraphicViewport* viewport = nullptr;
    QString gridInfoString = "";
    RS_Vector metaGridWidth;
    bool isometric = false;
    RS2::IsoGridViewType isoViewType = RS2::IsoLeft;
    bool scaleGrid = true;
    RS_Vector userGrid;
    int minGridSpacing;
	LC_GridSystem *gridSystem {nullptr};
    RS_Vector prepareGridWidth();
};

#endif
