/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
** Copyright (C) 2018 Andrey Yaromenok (ayaromenok@gmail.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
**********************************************************************/

#ifndef LC_MAKERCAMSVG_H
#define LC_MAKERCAMSVG_H

#include <memory>
#include <string>

#include "rs.h"
#include "rs_vector.h"

class RS_Arc;
class RS_Block;
class RS_Circle;
class RS_Ellipse;
class RS_Entity;
class RS_Hatch;
class RS_Image;
class RS_Insert;
class RS_Layer;
class RS_Leader;
class RS_Line;
class RS_MText;
class RS_Point;
class RS_Polyline;
class RS_Solid;
class RS_Spline;
class LC_SplinePoints;
class RS_Text;

class LC_XMLWriterInterface;

class RS_Document;
class RS_Graphic;

class LC_MakerCamSVG {
public:
    LC_MakerCamSVG(std::unique_ptr<LC_XMLWriterInterface> xmlWriter, bool writeInvisibleLayers = true, bool writeConstructionLayers = true,
                   bool writeBlocksInline = false, bool convertEllipsesToBeziers = false, bool exportImages = false,
                   bool convertLineTypes = false, double defaultElementWidth = 1.0, double defaultDashLinePatternLength = 10.0);

    ~LC_MakerCamSVG() = default;

    bool generate(RS_Graphic* graphic);
    std::string resultAsString() const;

    void setExportPoints(const bool exportPoints) {
        m_exportPoints = exportPoints;
    }

private:
    void write(RS_Graphic* graphic);

    void writeBlocks(RS_Document* document);
    void writeBlock(RS_Block* block);

    void writeLayers(RS_Document* document);
    void writeLayer(RS_Document* document, const RS_Layer* layer);

    void writeEntities(RS_Document* document, const RS_Layer* layer);
    void writeEntity(RS_Entity* entity);

    void writeInsert(const RS_Insert* insert);
    void writePoint(const RS_Point* point) const;
    void writeLine(const RS_Line* line) const;
    void writePolyline(RS_Polyline* polyline) const;
    void writeCircle(const RS_Circle* circle) const;
    void writeArc(const RS_Arc* arc) const;
    void writeEllipse(RS_Ellipse* ellipse) const;
    void writeSpline(const RS_Spline* spline);
    void writeSplinepoints(const LC_SplinePoints* splinepoints);

    void writeCubicBeziers(const std::vector<RS_Vector>& controlPoints, bool isClosed);
    void writeQuadraticBeziers(const std::vector<RS_Vector>& controlPoints, bool isClosed);
    void writeImage(const RS_Image* image) const;

    std::vector<RS_Vector> calcCubicBezierPoints(const std::vector<RS_Vector>& controlPoints, bool isClosed);
    std::vector<RS_Vector> calcQuadraticBezierPoints(const std::vector<RS_Vector>& controlPoints, bool isClosed);

    static std::string numXml(double value);
    /**
     * @brief lengthXml convert length to xml string
     * using lengthFactor to convert unknown units into mm
     * @param value
     * @return
     */
    std::string lengthXml(double value) const;
    RS_Vector convertToSvg(const RS_Vector& vector) const;

    std::string svgPathClose() const;
    std::string svgPathCurveTo(const RS_Vector& point, const RS_Vector& controlpoint1, const RS_Vector& controlpoint2) const;
    std::string svgPathQuadraticCurveTo(const RS_Vector& point, const RS_Vector& controlpoint) const;
    std::string svgPathLineTo(const RS_Vector& point) const;
    std::string svgPathMoveTo(const RS_Vector& point) const;
    std::string svgPathArc(const RS_Arc* arc) const;
    std::string svgPathArc(const RS_Vector& point, double radiusX, double radiusY, double xAxisRotation, bool largeArcFlag,
                           bool sweepFlag) const;
    std::string svgPathAnyLineType(RS_Vector startpoint, RS_Vector endpoint, RS2::LineType type) const;
    std::string getLinePattern(RS_Vector* lastPos, RS_Vector step, RS2::LineType type, double lineScale) const;
    std::string getPointSegment(RS_Vector* lastPos, const RS_Vector& step, double lineScale) const;
    std::string getLineSegment(RS_Vector* lastPos, const RS_Vector& step, double lineScale, bool x2 = false) const;

    RS_Vector calcEllipsePointDerivative(double majorRadius, double minorRadius, double xAxisRotation, double angle) const;

    static double calcAlpha(double angle);

    std::unique_ptr<LC_XMLWriterInterface> m_xmlWriter;

    bool m_writeInvisibleLayers = false;
    bool m_writeConstructionLayers = false;
    bool m_writeBlocksInline = false;
    bool m_convertEllipsesToBeziers = false;
    bool m_exportImages = false;
    bool m_convertLineTypes = false;
    bool m_exportPoints = false;
    double m_defaultElementWidth = 0.;
    double m_defaultDashLinePatternLength = 0.;

    RS_Vector m_min;
    RS_Vector m_max;

    RS_Vector m_offset;

    std::string m_unit;
    /**
     * @brief lengthFactor factor from current unit to svg length units
     */
    double m_lengthFactor = 0.;
};

#endif
