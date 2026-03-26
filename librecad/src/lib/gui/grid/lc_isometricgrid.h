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

#ifndef LC_ISOMETRICGRID_H
#define LC_ISOMETRICGRID_H

#include "lc_gridsystem.h"

class LC_IsometricGrid :public LC_GridSystem {
public:
    explicit LC_IsometricGrid(LC_GridOptions *options, int isoProjection);

    RS_Vector snapGrid(const RS_Vector &coord) const override;
    RS_Vector snapGrid(const RS_Vector& coord, const RS_Vector& rayStart, const RS_Vector& rayEnd) override;

    enum Projection{
        ISO_LEFT,
        ISO_TOP,
        ISO_RIGHT,
        ISO_LAST
    };

protected:
    int m_projection = ISO_TOP;
    double m_gridDeltaLeft = 0.0;
    double m_gridDeltaRight = 0.0;
    bool m_drawVerticalAlways = false;

    // single tile info
    int m_tileNumPointsByX = 0;
    int m_tileNumPointsByY = 0;

    // files grid values
    RS_Vector m_tilesMaxPoint;
    RS_Vector m_tilesStartPoint;
    RS_Vector m_tilesDelta;
    double m_tilesRowShift = 0;

    RS_Vector m_isometricCell;

    RS_VectorSolutions m_snapVectorSolution;

    RS_Vector m_gridDeltaX;
    RS_Vector m_gridDeltaY;

    bool m_drawRightLine;
    bool m_drawLeftLine;
    bool m_drawTopLines;

    void drawMetaGridLines(RS_Painter *painter, LC_GraphicViewport *view) override;
    void createGridLines(const RS_Vector &min, const RS_Vector &max, const RS_Vector &gridWidth, bool drawGridWithoutGaps, const RS_Vector& lineInTileOffset) override;
    void createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, bool drawGridWithoutGaps, int totalPoints) override;

    void calculateTilesGridMetrics(const RS_Vector &maxCorner, const RS_Vector &offset);
    void fillTilesRowsByPointsExceptDiagonal() const;
    void fillTilesRowsByPoints() const;
    void fillTilesRowsByLinesNoDiagonals() const;
    void fillTilesRowsBylines() const;
    void prepareSnapSolution();
    void createMetaGridLines(const RS_Vector &min, const RS_Vector &max) override;
    void createGridLinesNoGaps(const RS_Vector &min, const RS_Vector &max) const;
    void fillPointsNoGaps(const RS_Vector &min, const RS_Vector &max) const;
    int  determineTotalPointsAmount(bool drawGridWithoutGaps) override;
    void determineGridPointsAmount(const RS_Vector &vector) override;
    void createCellVector(const RS_Vector &gridWidth) override;
    void determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) override;
    void prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) override;
    void setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) override;
};

#endif
