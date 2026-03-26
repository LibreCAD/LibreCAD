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

#include "lc_gridsystem.h"
#include "rs_grid.h"

class LC_OrthogonalGrid:public LC_GridSystem{
public:

    explicit LC_OrthogonalGrid(LC_GridOptions *options);
    ~LC_OrthogonalGrid() override;
    RS_Vector snapGrid(const RS_Vector &coord) const override;
    RS_Vector snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd);

protected:

    // Metagrid data
    /**
     * left,bottom coordinate for the first visible metaGrid point
     */
    RS_Vector m_gridMin;
    /**
     * right, top coordinate for last visible metaGrid point
     */
    RS_Vector m_gridMax;
    int m_numPointsXLeft = 0;
    int m_numPointsXRight = 0;
    int m_numPointsYBottom = 0;
    int m_numPointsYTop = 0;
    int m_numPointsXTotal = 0;
    int m_numPointsYTotal = 0;

    /**
     * with some combination of view size and grid sizes, it might be that no metaGrid lines is visible on the screen.
     * This point is used as base point for creation grids point for such case (it is located outside of screen and
     * determines the offset for grid from leftmost top corner of view (view 0,height).
     */
    RS_Vector m_gridBasePointIfMetagridNotVisible;

    void prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) override;

    void fillMetaGridCoordinates();

    void ensureAllMetaGridLinesInView(const RS_Vector &viewZero, const RS_Vector &viewSize);

    void determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) override;

    void projectMetaGridLinesAmount(const RS_Vector &metaGridWidth);

    void determineGridPointsAmount(const RS_Vector &viewZero) override;

    int  determineTotalPointsAmount(bool drawGridWithoutGaps) override;

    void fillPointsLatticeWithGapsForMetaGrid() const;

    void determineGridBoundaries(const RS_Vector &viewZero,const RS_Vector &viewSize);

    void drawMetaGridLines(RS_Painter *painter, LC_GraphicViewport *view) override;

    void createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, bool drawGridWithoutGaps, int numPointsTotal) override;

    void createGridLines(const RS_Vector& min, const RS_Vector &max, const RS_Vector & gridWidth, bool drawGridWithoutGaps, const RS_Vector& lineOffset) override;

    void createVerticalLines(double start, double end, double baseX, double delta, int pointsToDraw) const;

    void createHorizontalLines(double start, double end, double baseY, double delta, int pointsToDraw) const;

    void createMetaGridLines(const RS_Vector &min, const RS_Vector &max) override;

    void doCreateVerticalLines(LC_Lattice *lattice, double start, double end, double baseX, double delta, int pointsToDraw) const;

    void doCreateHorizontalLines(LC_Lattice *lattice, double start, double end, double baseY, double delta, int pointsToDraw) const;

    void createGridLinesWithoutGaps(const RS_Vector &min, const RS_Vector &max) const;

    void createGridLinesWithGaps(const RS_Vector &min, const RS_Vector &max, const RS_Vector &lineOffset) const;

    void createCellVector(const RS_Vector &gridWidth) override;


};

#endif
