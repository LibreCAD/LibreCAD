/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2014 Christian Luginbühl (dinkel@pimprecords.com)
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

#include <string>
#include <memory>

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
	LC_MakerCamSVG(LC_XMLWriterInterface* xmlWriter,
                   bool writeInvisibleLayers = true,
                   bool writeConstructionLayers = true,
                   bool writeBlocksInline = false,
                   bool convertEllipsesToBeziers = false);

	~LC_MakerCamSVG() = default;

    bool generate(RS_Graphic* graphic);
    std::string resultAsString();

private:
    void write(RS_Graphic* graphic);

    void writeBlocks(RS_Document* document);
    void writeBlock(RS_Block* block);

    void writeLayers(RS_Document* document);
    void writeLayer(RS_Document* document, RS_Layer* layer);

    void writeEntities(RS_Document* document, RS_Layer* layer);
    void writeEntity(RS_Entity* entity);

    void writeInsert(RS_Insert* insert);
    void writePoint(RS_Point* point);
    void writeLine(RS_Line* line);
    void writePolyline(RS_Polyline* polyline);
    void writeCircle(RS_Circle* circle);
    void writeArc(RS_Arc* arc);
    void writeEllipse(RS_Ellipse* ellipse);
    void writeSpline(RS_Spline* spline);

    std::string numXml(double value);
    RS_Vector convertToSvg(RS_Vector vector);

    std::string svgPathClose();
    std::string svgPathCurveTo(RS_Vector point, RS_Vector controlpoint1, RS_Vector controlpoint2);
    std::string svgPathLineTo(RS_Vector point);
    std::string svgPathMoveTo(RS_Vector point);
    std::string svgPathArc(RS_Arc* arc);
    std::string svgPathArc(RS_Vector point, double radius_x, double radius_y, double x_axis_rotation, bool large_arc_flag, bool sweep_flag);

    RS_Vector calcEllipsePointDerivative(double majorradius, double minorradius, double x_axis_rotation, double angle);

    double calcAlpha(double angle);

    bool writeInvisibleLayers;
    bool writeConstructionLayers;
    bool writeBlocksInline;
    bool convertEllipsesToBeziers;

	std::unique_ptr<LC_XMLWriterInterface> xmlWriter;

    RS_Vector min;
    RS_Vector max;

    RS_Vector offset;

    std::string unit;
};

#endif
