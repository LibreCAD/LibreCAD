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

#ifndef LC_LATTICE_H
#define LC_LATTICE_H

#include "rs_vector.h"

class LC_GraphicViewport;

class LC_Lattice{
public:
    LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth);
    LC_Lattice(double angleX, double angleY, const RS_Vector& gridWidth, int numPointsTotal);

    LC_Lattice() = default;

    virtual ~LC_Lattice();
    const RS_Vector& getMajorVector() const;
    const RS_Vector& getDeltaX() const;
    const RS_Vector& getDeltaY() const;
    void init(int projectedPointsCount);

    void update(double angleX, double angleY, const RS_Vector& gridWidth, int numPointsTotal);
    void updateForLines(double angleX, double angleY, const RS_Vector& gridWidth, const RS_Vector& offsetForLine, int numPointsTotal);

    void fill(int numPointsByX, int numPointsByY,
              const RS_Vector &baseGridPoint,
              bool reverseX, bool reverseY);

    void fillWithoutDiagonal(int numPointsByX, int numPointsByY,
                             const RS_Vector &baseGridPoint,
                             bool reverseX, bool reverseY, bool bottomLeftToTopRightDiagonal, int totalSize);

    void fillVerticalEdge(int numPointsByX, const RS_Vector& baseGridPoint, bool reverseX, bool reverseY, bool skipFirstPoint = false);
    void fillHorizontalEdge(int numPointsByX, const RS_Vector& baseGridPoint, bool reverseX, bool reverseY, bool skipFirstPoint = false);

    int getPointsSize() const {return m_pointsX.size();}
    RS_Vector getPoint(const int index) const {return RS_Vector(m_pointsX[index], m_pointsY[index]);}

    RS_Vector getOffset(int xPointsDelta, int yPointsDelta) const;

    void fillByLines(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY, bool fillLeftEdge, bool fillRightEdge);
    void fillAllByLinesExceptDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, bool reverseX, bool reverseY, bool fillLeftEdge, bool fillRightEdge);

    double getPointX(const int i) const {return m_pointsX[i];}
    double getPointY(const int i) const {return m_pointsY[i];}

    const std::vector<double> &getPointsX() const;
    const std::vector<double> &getPointsY() const;
    void toGui(const LC_GraphicViewport *viewport);
    void addLine(double startX, double startY, double endX, double endY);
    void addPoint(double x, double y);
protected:
    RS_Vector m_deltaX;
    RS_Vector m_deltaY;
    RS_Vector m_majorVector;
    std::vector<double> m_pointsX;
    std::vector<double> m_pointsY;
    RS_Vector m_lineOffsetX;
    RS_Vector m_lineOffsetY;

    void fillAll(int numPointsByX, int numPointsByY,
                 const RS_Vector &baseGridPoint,
                 const RS_Vector& xDelta, const RS_Vector& yDelta);

    void fillExceptBLTRDiagonal(int numPointsByX, int numPointsByY,
                                const RS_Vector &baseGridPoint,
                                const RS_Vector& xDelta, const RS_Vector& yDelta);

    void fillExceptTLBRDiagonal(int numPointsByX, int numPointsByY,
                                const RS_Vector &baseGridPoint,
                                const RS_Vector& xDelta, const RS_Vector& yDelta, int totalSize);

    void fillHorizontalEdge(int numPointsByX, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool skipFirstPoint);
    void fillVerticalEdge(int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool skipFirstPoint);
    void prepareDeltas(bool reverseX, bool reverseY, RS_Vector &xDeltaToUse, RS_Vector &yDeltaToUse) const;
    void calcDeltas(double angleX, double angleY, const RS_Vector &gridWidth);
    void fillAllByLine(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool fillLeftEdge, bool fillRightEdge);
    void calcLineOffsetDeltas(double angleX, double angleY, const RS_Vector &offset);
    void fillAllByLineExceptDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta, bool fillLeftEdge, bool fillRightEdge);
    void fillByLinesParallelDiagonal(int numPointsByX, int numPointsByY, const RS_Vector &baseGridPoint, const RS_Vector &xDelta, const RS_Vector &yDelta);
};

#endif
