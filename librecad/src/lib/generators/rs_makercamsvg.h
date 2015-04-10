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

#ifndef RS_MAKERCAMSVG_H
#define RS_MAKERCAMSVG_H

#include <libxml++/libxml++.h>

#include <rs_arc.h>
#include <rs_block.h>
#include <rs_circle.h>
#include <rs_document.h>
#include <rs_entity.h>
#include <rs_ellipse.h>
#include <rs_graphic.h>
#include <rs_insert.h>
#include <rs_layer.h>
#include <rs_line.h>
#include <rs_point.h>
#include <rs_polyline.h>
#include <rs_vector.h>

class RS_MakerCamSVG {
public:
    RS_MakerCamSVG(bool writeInvisibleLayers = true,
                   bool writeConstructionLayers = true,
                   bool writeBlocksInline = false,
                   bool convertEllipsesToBeziers = false);

    ~RS_MakerCamSVG();

    bool generate(RS_Graphic* graphic);
    std::string resultAsString();

private:
    void write(RS_Graphic* graphic);

    void writeBlocks(xmlpp::Element* parent, RS_Document* document);
    void writeBlock(xmlpp::Element* parent, RS_Block* block);

    void writeLayers(xmlpp::Element* parent, RS_Document* document);
    void writeLayer(xmlpp::Element* parent, RS_Document* document, RS_Layer* layer);

    void writeEntities(xmlpp::Element* parent, RS_Document* document, RS_Layer* layer);
    void writeEntity(xmlpp::Element* parent, RS_Entity* entity);

    void writeInsert(xmlpp::Element* parent, RS_Insert* insert);
    void writePoint(xmlpp::Element* parent, RS_Point* point);
    void writeLine(xmlpp::Element* parent, RS_Line* line);
    void writePolyline(xmlpp::Element* parent, RS_Polyline* polyline);
    void writeCircle(xmlpp::Element* parent, RS_Circle* circle);
    void writeArc(xmlpp::Element* parent, RS_Arc* arc);
    void writeEllipse(xmlpp::Element* parent, RS_Ellipse* ellipse);

    std::string numXml(double value);
    RS_Vector convertToSvg(RS_Vector vector);

    std::string svgPathClose();
    std::string svgPathCurveTo(RS_Vector point, RS_Vector controlpoint1, RS_Vector controlpoint2);
    std::string svgPathLineTo(RS_Vector point);
    std::string svgPathMoveTo(RS_Vector point);
    std::string svgPathArc(RS_Arc* arc);
    std::string svgPathArc(RS_Vector point, double radius_x, double radius_y, double x_axis_rotation, bool large_arc_flag, bool sweep_flag);

    RS_Vector calcEllipsePoint(RS_Vector center, double majorradius, double minorradius, double x_axis_rotation, double angle);
    RS_Vector calcEllipsePointDerivative(double majorradius, double minorradius, double x_axis_rotation, double angle);

    double calcAlpha(double angle);

    bool writeInvisibleLayers;
    bool writeConstructionLayers;
    bool writeBlocksInline;
    bool convertEllipsesToBeziers;

    xmlpp::Document* doc;

    RS_Vector min;
    RS_Vector max;

    RS_Vector offset;

    std::string unit;
};

#endif
