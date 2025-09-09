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

#ifndef LC_GRIDSYSTEM_H
#define LC_GRIDSYSTEM_H

#include <memory>

#include "rs_vector.h"
#include "rs_color.h"

class RS_Painter;
class LC_Lattice;
class LC_GraphicViewport;

class LC_GridSystem {
public:
    struct LC_GridOptions{
        bool simpleGridRendering = false;
        bool drawLines = false;

        bool drawGrid = true;
        RS2::LineType gridLineType{};
        int gridWidthPx = 1;

        RS_Color gridColorLine;
        RS_Color gridColorPoint;
        bool drawMetaGrid = true;
        RS_Color metaGridColor;
        int metaGridLineWidthPx = 1;
        RS2::LineType metaGridLineType{};
        bool disableGridOnPanning = false;
        bool drawIsometricVerticalsAlways = true; // fixme - complete initialization
    };

    LC_GridSystem(LC_GridOptions* options);
    virtual ~LC_GridSystem() ;

    void setOptions(std::unique_ptr<LC_GridOptions> options);
    void invalidate();
    RS_Vector const &getCellVector() const {
        return m_cellVector;
    }
    virtual RS_Vector snapGrid(const RS_Vector &coord) const = 0;
    void createGrid(LC_GraphicViewport* view, const RS_Vector &viewZero, const RS_Vector &viewSize, const RS_Vector &metaGridWidth, const RS_Vector &gridWidth);
    void draw(RS_Painter *painter, LC_GraphicViewport* view);
    void clearGrid();
    void setGridInfiniteState(bool hasIndefiniteAxis, bool undefinedX);
    bool isGridDisabledByPanning(LC_GraphicViewport *view);
    bool isValid() const;
    void calculateSnapInfo(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse);

protected:
    bool m_valid = false;
    RS_Vector m_cellVector = {0., 0.};
    std::unique_ptr<LC_GridOptions> m_gridOptions;
    std::unique_ptr<LC_Lattice> m_gridLattice;
    std::unique_ptr<LC_Lattice> m_metaGridLattice;

    /**
    * Grid metrics
    */

    RS_Vector m_gridCellSize;
    RS_Vector m_gridBasePoint;

    /**
     * amount of visible metaGrid lines by X
    */
    int m_numMetaX = 0;
    /**
     * amount of visible metaGrid lines by Y
     */

    int m_numMetaY = 0;

    /**
     * metaGrid cell size by X and Y axis
     */
    RS_Vector m_metaGridCellSize;

    /**
     * offset from left top corner of view to start position of grid. That position may be outside of view
     * (for very large zoom and small size of view for cartesian grid) or for isometric view
     */
    RS_Vector m_metaGridViewOffset;

    /**
    * left,bottom coordinate for the first visible metaGrid point
    */
    RS_Vector m_metaGridMin;
    /**
     * right, top coordinate for last visible metaGrid point
     */
    RS_Vector m_metaGridMax;

    int m_numPointsInMetagridX = 0;
    int m_numPointsInMetagridY = 0;

    bool m_hasAxisIndefinite = false;
    bool m_indefiniteX  = false;

    void doCreateGrid(LC_GraphicViewport* view, const RS_Vector &viewZero, const RS_Vector &viewSize, const RS_Vector &metaGridWidth, const RS_Vector &gridWidth);
    virtual void createMetaGridLines(const RS_Vector& min, const RS_Vector &max)  = 0;
    void drawMetaGrid(RS_Painter *painter, LC_GraphicViewport *view);
    void drawGrid(RS_Painter *painter, LC_GraphicViewport *view);
    void drawGridPoints(RS_Painter *painter, LC_GraphicViewport *view);
    void drawGridLines(RS_Painter *painter, LC_GraphicViewport *view);
    int getGridPointsCount();
    virtual void drawMetaGridLines(RS_Painter *painter, LC_GraphicViewport *view) = 0;
    virtual void createGridPoints(const RS_Vector &min, const RS_Vector &max,const RS_Vector &gridWidth, bool drawGridWithoutGaps, int numPointsTotal) = 0;
    virtual void createGridLines(const RS_Vector& min, const RS_Vector &max, const RS_Vector & gridWidth, bool gaps, const RS_Vector& lineOffset) = 0;
    virtual int  determineTotalPointsAmount(bool drawGridWithoutGaps) = 0;
    virtual void determineGridPointsAmount(const RS_Vector &vector) = 0;
    virtual void createCellVector(const RS_Vector &gridWidth) = 0;
    virtual void determineMetaGridBoundaries(const RS_Vector &viewZero, const RS_Vector &viewSize) = 0;
    virtual void prepareGridOther(const RS_Vector &viewZero, const RS_Vector &viewSize) = 0;
    double truncToStep(double value, double step);
    void doDrawLines(RS_Painter *painter, LC_GraphicViewport *view, LC_Lattice *lattice);
    bool isNumberOfPointsValid(int numberOfPoints);
    virtual void setCellSize(const RS_Vector &gridWidth, const RS_Vector &metaGridWidth);
    void doCalculateSnapInfo(RS_Vector& viewZero,RS_Vector& viewSize,RS_Vector& metaGridWidthToUse,RS_Vector& gridWidthToUse);
};

#endif // LC_GRIDSYSTEM_H
