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
    explicit LC_IsometricGrid(LC_GridOptions *options, int projection);

    RS_Vector snapGrid(const RS_Vector &coord) const override;

    enum Projection{
        ISO_LEFT,
        ISO_TOP,
        ISO_RIGHT,
        ISO_LAST
    };

protected:

    int projection = ISO_TOP;

    double gridDeltaLeft = 0.0;
    double gridDeltaRight = 0.0;
    bool drawVerticalAlways = false;

    // single tile info
    int tileNumPointsByX = 0;
    int tileNumPointsByY = 0;

    // files grid values
    RS_Vector tilesMaxPoint;
    RS_Vector tilesStartPoint;
    RS_Vector tilesDelta;
    double tilesRowShift = 0;

    RS_Vector isometricCell;

    RS_VectorSolutions snapVectorSolution;

    RS_Vector gridDeltaX;
    RS_Vector gridDeltaY;

    bool drawRightLine;
    bool drawLeftLine;
    bool drawTopLines;

    void drawMetaGridLines(RS_Painter *painter, RS_GraphicView *view) override;
    void createGridLines(const RS_Vector &min, const RS_Vector &max, const RS_Vector &gridWidth, bool drawGridWithoutGaps, const RS_Vector& lineInTileOffset) override;
    void createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, bool drawGridWithoutGaps, int total) override;

    void calculateTilesGridMetrics(const RS_Vector &maxCorner, const RS_Vector &offset);
    void fillTilesRowsByPointsExceptDiagonal();
    void fillTilesRowsByPoints();
    void fillTilesRowsByLinesNoDiagonals();
    void fillTilesRowsBylines();
    void prepareSnapSolution();
    void createMetaGridLines(const RS_Vector &min, const RS_Vector &max) override;
    void createGridLinesNoGaps(const RS_Vector &min, const RS_Vector &max);
    void fillPointsNoGaps(const RS_Vector &min, const RS_Vector &max);
    int  determineTotalPointsAmount(bool drawGridWithoutGaps) override;
    void determineGridPointsAmount(const RS_Vector &vector) override;
    void createCellVector(const RS_Vector &gridWidth) override;
    void determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) override;
    void prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) override;

    void setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth) override;
};

#endif // LC_ISOMETRICGRID_H
