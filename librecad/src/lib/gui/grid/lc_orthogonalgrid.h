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

#ifndef LC_ORTHOGONALGRID_H
#define LC_ORTHOGONALGRID_H

#include "lc_lattice.h"

#include "rs_grid.h"
#include "lc_gridsystem.h"

class LC_OrthogonalGrid:public LC_GridSystem{
public:

    explicit LC_OrthogonalGrid(LC_GridOptions *options);
    ~LC_OrthogonalGrid() override;
    RS_Vector snapGrid(const RS_Vector &coord) const override;
protected:

    // Metagrid data
    /**
     * left,bottom coordinate for the first visible metaGrid point
     */
    RS_Vector gridMin;
    /**
     * right, top coordinate for last visible metaGrid point
     */
    RS_Vector gridMax;


    int numPointsXLeft = 0;
    int numPointsXRight = 0;
    int numPointsYBottom = 0;
    int numPointsYTop = 0;
    int numPointsXTotal = 0;
    int numPointsYTotal = 0;

    /**
     * with some combination of view size and grid sizes, it might be that no metaGrid lines is visible on the screen.
     * This point is used as base point for creation grids point for such case (it is located outside of screen and
     * determines the offset for grid from leftmost top corner of view (view 0,height).
     */
    RS_Vector gridBasePointIfMetagridNotVisible;

    void prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) override;

    void fillMetaGridCoordinates();

    void ensureAllMetaGridLinesInView(const RS_Vector &viewZero, const RS_Vector &viewSize);

    void determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) override;

    void projectMetaGridLinesAmount(const RS_Vector &metaGridWidth);

    void determineGridPointsAmount(const RS_Vector &viewZero) override;

    int  determineTotalPointsAmount(bool drawGridWithoutGaps) override;

    void fillPointsLatticeWithGapsForMetaGrid();

    void determineGridBoundaries(const RS_Vector &viewZero,const RS_Vector &viewSize);

    void drawMetaGridLines(RS_Painter *painter, RS_GraphicView *view) override;

    void createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, bool drawGridWithoutGaps, int numPointsTotal) override;

    void createGridLines(const RS_Vector& min, const RS_Vector &max, const RS_Vector & gridWidth, bool gaps, const RS_Vector& lineOffset) override;

    void createVerticalLines(const double &start, const double &end, const double &baseX, const double &delta, const int &pointsToDraw) const;

    void createHorizontalLines(const double &start, const double &end, const double &baseY, const double &delta, const int &pointsToDraw) const;

    void createMetaGridLines(const RS_Vector &min, const RS_Vector &max) override;

    void doCreateVerticalLines(LC_Lattice *lattice, const double &start, const double &end, const double &baseX,
                               const double &delta, const int &pointsToDraw) const;

    void doCreateHorizontalLines(LC_Lattice *lattice, const double &start, const double &end, const double &baseY,
                                 const double &delta, const int &pointsToDraw) const;

    void createGridLinesWithoutGaps(const RS_Vector &min, const RS_Vector &max);

    void createGridLinesWithGaps(const RS_Vector &min, const RS_Vector &max, const RS_Vector &lineOffset);

    void createCellVector(const RS_Vector &gridWidth) override;


};

#endif // LC_ORTHOGONALGRID_H
