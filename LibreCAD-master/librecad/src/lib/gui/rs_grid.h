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

#include "rs_vector.h"

class RS_GraphicView;
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
class RS_Grid
{

public:
	RS_Grid(RS_GraphicView* graphicView);

	void updatePointArray();

	/**
		 * @return Array of all visible grid points.
		 */
	std::vector<RS_Vector> const& getPoints() const;

	/**
	* \brief the closest grid point
	* \return the closest grid to given point
	* \param coord the given point
	*/
	RS_Vector snapGrid(const RS_Vector& coord) const;

	/**
		 * @return Number of visible grid points.
		 */
	int count() const;
	void setCrosshairType(RS2::CrosshairType chType);
	RS2::CrosshairType getCrosshairType() const;

	/**
		 * @return Grid info for status widget.
		 */
	QString getInfo() const;

	/**
		 * @return a vector of Meta grid positions in X.
		 */
	std::vector<double> const& getMetaX() const;

	/**
		 * @return a vector of Meta grid positions in Y.
		 */
	std::vector<double> const& getMetaY() const;

	bool isIsometric() const;
	void setIsometric(bool b);
	RS_Vector getMetaGridWidth() const;
	RS_Vector const& getCellVector() const;

private:
	//! copy ctor disabled
	RS_Grid(RS_Grid const&) = delete;
	RS_Grid& operator = (RS_Grid const&) = delete;
	//! \{ \brief create grid points
	void createOrthogonalGrid(LC_Rect const& rect, RS_Vector const& gridWidth);
	void createIsometricGrid(LC_Rect const& rect, RS_Vector const& gridWidth);
	//! \}

	//! \{ \brief determine grid width
	RS_Vector getMetricGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing);
	RS_Vector getImperialGridWidth(RS_Vector const& userGrid, bool scaleGrid, int minGridSpacing);
	//! \}

	//! Graphic view this grid is connected to.
	RS_GraphicView* graphicView;

	//! Current grid spacing
	double spacing;
	//! Current meta grid spacing
	double metaSpacing;

	//! Pointer to array of grid points
	std::vector<RS_Vector> pt;
	RS_Vector baseGrid; // the left-bottom grid point
	RS_Vector cellV;// (dx,dy)
	RS_Vector metaGridWidth;
	//! Meta grid positions in X
	std::vector<double> metaX;
	//! Meta grid positions in Y
	std::vector<double> metaY;
	bool isometric;
	RS2::CrosshairType crosshairType;

};

#endif
