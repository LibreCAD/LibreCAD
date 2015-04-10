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

#include "rs_makercamsvg.h"

#include <stdio.h>
#include <string.h>

#include <libxml++/libxml++.h>

#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_point.h"
#include "rs_line.h"
#include "rs_leader.h"
#include "rs_polyline.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_system.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_grid.h"
#include "rs_dialogfactory.h"
#include "rs_units.h"
#include "rs_utility.h"

RS_MakerCamSVG::RS_MakerCamSVG(bool writeInvisibleLayers,
                               bool writeConstructionLayers,
                               bool writeBlocksInline,
                               bool convertEllipsesToBeziers) {

    RS_DEBUG->print("RS_MakerCamSVG::RS_MakerCamSVG()");

    this->writeInvisibleLayers = writeInvisibleLayers;
    this->writeConstructionLayers = writeConstructionLayers;
    this->writeBlocksInline = writeBlocksInline;
    this->convertEllipsesToBeziers = convertEllipsesToBeziers;

    this->doc = new xmlpp::Document();

    this->offset = RS_Vector(0, 0);
}

RS_MakerCamSVG::~RS_MakerCamSVG() {

    delete doc;
}

bool RS_MakerCamSVG::generate(RS_Graphic* graphic) {

    write(graphic);

    return true;
}

std::string RS_MakerCamSVG::resultAsString() {

    return doc->write_to_string_formatted();
}

void RS_MakerCamSVG::write(RS_Graphic* graphic) {

    RS_DEBUG->print("RS_MakerCamSVG::write: Writing root node ...");

    xmlpp::Element* e = doc->create_root_node("svg", "http://www.w3.org/2000/svg");

    graphic->calculateBorders();

    min = graphic->getMin();
    max = graphic->getMax();

    RS2::Unit raw_unit = graphic->getUnit();

    switch (raw_unit) {
        case RS2::Millimeter:
            unit = "mm";
            break;
        case RS2::Centimeter:
            unit = "cm";
            break;
        case RS2::Inch:
            unit = "in";
            break;

        default:
            unit = "";
            break;
    }

    e->set_attribute("width", numXml(max.x - min.x) + unit);

    e->set_attribute("height", numXml(max.y - min.y) + unit);

    e->set_attribute("viewBox", "0 0 "+ numXml(max.x - min.x) + " " + numXml(max.y - min.y));

    e->set_namespace_declaration("http://www.librecad.org", "lc");
    e->set_namespace_declaration("http://www.w3.org/1999/xlink", "xlink");

    writeBlocks(e, graphic);
    writeLayers(e, graphic);
}

void RS_MakerCamSVG::writeBlocks(xmlpp::Element* parent, RS_Document* document) {

    if (!writeBlocksInline) {

        RS_DEBUG->print("RS_MakerCamSVG::writeBlocks: Writing blocks ...");

        RS_BlockList* blocklist = document->getBlockList();

        if (blocklist->count() > 0) {

            xmlpp::Element* e = parent->add_child("defs");

            for (int i = 0; i < blocklist->count(); i++) {

                writeBlock(e, blocklist->at(i));
            }
        }

    }
}

void RS_MakerCamSVG::writeBlock(xmlpp::Element* parent, RS_Block* block) {

    RS_DEBUG->print("RS_MakerCamSVG::writeBlock: Writing block with name '%s'",
                    qPrintable(block->getName()));

    xmlpp::Element* e = parent->add_child("g");

    e->set_attribute("id", std::to_string(block->getId()));
    e->set_attribute("blockname", qPrintable(block->getName()), "lc");

    writeLayers(e, block);
}

void RS_MakerCamSVG::writeLayers(xmlpp::Element* parent, RS_Document* document) {

    RS_DEBUG->print("RS_MakerCamSVG::writeLayers: Writing layers ...");

    RS_LayerList* layerlist = document->getLayerList();

    for (unsigned int i = 0; i < layerlist->count(); i++) {

        writeLayer(parent, document, layerlist->at(i));
    }
}

void RS_MakerCamSVG::writeLayer(xmlpp::Element* parent, RS_Document* document, RS_Layer* layer) {

    if (writeInvisibleLayers || !layer->isFrozen()) {

        if (writeConstructionLayers || !layer->isConstruction()) {

            RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Writing layer with name '%s'",
                            qPrintable(layer->getName()));

            xmlpp::Element* e = parent->add_child("g");

            e->set_attribute("layername", qPrintable(layer->getName()), "lc");
            e->set_attribute("is_locked", (layer->isLocked() ? "true" : "false"), "lc");
            e->set_attribute("is_construction", (layer->isConstruction() ? "true" : "false"), "lc");

            if (layer->isFrozen())
            {
                e->set_attribute("style", "display: none;");
            }

            e->set_attribute("fill", "none");
            e->set_attribute("stroke", "black");
            e->set_attribute("stroke-width", "1");

            writeEntities(e, document, layer);
        }
        else {

            RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Omitting construction layer with name '%s'",
                            qPrintable(layer->getName()));
        }
    }
    else {

        RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Omitting invisible layer with name '%s'",
                        qPrintable(layer->getName()));
    }
}

void RS_MakerCamSVG::writeEntities(xmlpp::Element* parent, RS_Document* document, RS_Layer* layer) {

    RS_DEBUG->print("RS_MakerCamSVG::writeEntitiesFromBlock: Writing entities from layer ...");

    for (RS_Entity* e = document->firstEntity(RS2::ResolveNone); e != NULL; e = document->nextEntity(RS2::ResolveNone)) {

        if (e->getLayer() == layer) {

            if (!(e->getFlag(RS2::FlagUndone))) {

                writeEntity(parent, e);
            }
        }
    }
}

void RS_MakerCamSVG::writeEntity(xmlpp::Element* parent, RS_Entity* entity) {

    RS_DEBUG->print("RS_MakerCamSVG::writeEntity: Found entity ...");

    switch (entity->rtti()) {
        case RS2::EntityInsert:
            writeInsert(parent, (RS_Insert*)entity);
            break;
        case RS2::EntityPoint:
            writePoint(parent, (RS_Point*)entity);
            break;
        case RS2::EntityLine:
            writeLine(parent, (RS_Line*)entity);
            break;
        case RS2::EntityPolyline:
            writePolyline(parent, (RS_Polyline*)entity);
            break;
        case RS2::EntityCircle:
            writeCircle(parent, (RS_Circle*)entity);
            break;
        case RS2::EntityArc:
            writeArc(parent, (RS_Arc*)entity);
            break;
        case RS2::EntityEllipse:
            writeEllipse(parent, (RS_Ellipse*)entity);
            break;

        default:
            RS_DEBUG->print("RS_MakerCamSVG::writeEntity: Entity with type '%d' not yet implemented",
                            (int)entity->rtti());
            break;
    }
}

void RS_MakerCamSVG::writeInsert(xmlpp::Element* parent, RS_Insert* insert) {

    RS_Block* block = insert->getBlockForInsert();

    RS_Vector insertionpoint = convertToSvg(insert->getInsertionPoint());

    if (writeBlocksInline) {

        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert inline ...");

        offset.set(insertionpoint.x, insertionpoint.y - (max.y - min.y));

        xmlpp::Element* e = parent->add_child("g");

        e->set_attribute("blockname", qPrintable(block->getName()), "lc");

        writeLayers(e, block);

        offset.set(0, 0);
    }
    else {

        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert as reference to block ...");

        xmlpp::Element* e = parent->add_child("use");

        e->set_attribute("x", numXml(insertionpoint.x));
        e->set_attribute("y", numXml(insertionpoint.y - (max.y - min.y)));
        e->set_attribute("href", "#" + std::to_string(block->getId()), "xlink");
    }
}

void RS_MakerCamSVG::writePoint(xmlpp::Element* parent, RS_Point* point) {

    RS_DEBUG->print("RS_MakerCamSVG::writePoint: Writing point ...");

    // NOTE: There is no "point" element in SVG, therefore creating a circle
    //       with minimal radius.

    xmlpp::Element* e = parent->add_child("circle");

    RS_Vector center = convertToSvg(point->getPos());

    e->set_attribute("cx", numXml(center.x));
    e->set_attribute("cy", numXml(center.y));
    e->set_attribute("r", numXml(0.1));
}

void RS_MakerCamSVG::writeLine(xmlpp::Element* parent, RS_Line* line) {

    RS_DEBUG->print("RS_MakerCamSVG::writeLine: Writing line ...");

    xmlpp::Element* e = parent->add_child("line");

    RS_Vector startpoint = convertToSvg(line->getStartpoint());
    RS_Vector endpoint = convertToSvg(line->getEndpoint());

    e->set_attribute("x1", numXml(startpoint.x));
    e->set_attribute("y1", numXml(startpoint.y));
    e->set_attribute("x2", numXml(endpoint.x));
    e->set_attribute("y2", numXml(endpoint.y));
}

void RS_MakerCamSVG::writePolyline(xmlpp::Element* parent, RS_Polyline* polyline) {

    RS_DEBUG->print("RS_MakerCamSVG::writePolyline: Writing polyline ...");

    xmlpp::Element* e = parent->add_child("path");

    std::string path = svgPathMoveTo(convertToSvg(polyline->getStartpoint()));

    for (RS_Entity* entity = polyline->firstEntity(RS2::ResolveNone); entity != NULL; entity = polyline->nextEntity(RS2::ResolveNone)) {

        if (!entity->isAtomic()) {
            continue;
        }

        if (entity->rtti() == RS2::EntityArc) {

            path += svgPathArc((RS_Arc*)entity);
        }
        else {

            path += svgPathLineTo(convertToSvg(entity->getEndpoint()));
        }
    }

    if (polyline->isClosed()) {

        path += svgPathClose();
    }

    e->set_attribute("d", path);
}

void RS_MakerCamSVG::writeCircle(xmlpp::Element* parent, RS_Circle* circle) {

    RS_DEBUG->print("RS_MakerCamSVG::writeCircle: Writing circle ...");

    xmlpp::Element* e = parent->add_child("circle");

    RS_Vector center = convertToSvg(circle->getCenter());

    e->set_attribute("cx", numXml(center.x));
    e->set_attribute("cy", numXml(center.y));
    e->set_attribute("r", numXml(circle->getRadius()));
}

void RS_MakerCamSVG::writeArc(xmlpp::Element* parent, RS_Arc* arc) {

    RS_DEBUG->print("RS_MakerCamSVG::writeArc: Writing arc ...");

    xmlpp::Element* e = parent->add_child("path");

    std::string path = svgPathMoveTo(convertToSvg(arc->getStartpoint())) +
                       svgPathArc(arc);

    e->set_attribute("d", path);
}

void RS_MakerCamSVG::writeEllipse(xmlpp::Element* parent, RS_Ellipse* ellipse) {

    RS_Vector center = convertToSvg(ellipse->getCenter());

    double majorradius = ellipse->getMajorRadius();
    double minorradius = ellipse->getMinorRadius();

    if (convertEllipsesToBeziers) {

        if (ellipse->isArc()) {

            const int segments = 4;

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse arc approximated by 'path' with %d cubic bézier segments (as discussed in https://www.spaceroots.org/documents/ellipse/elliptical-arc.pdf) ...", segments);

            xmlpp::Element* e = parent->add_child("path");

            double x_axis_rotation = 2 * M_PI - ellipse->getAngle();

            double start_angle = 2 * M_PI - ellipse->getAngle2();
            double end_angle = 2 * M_PI - ellipse->getAngle1();

            if (ellipse->isReversed()) {
                double temp_angle = start_angle;
                start_angle = end_angle;
                end_angle = temp_angle;
            }

            if (end_angle <= start_angle) {
                end_angle += 2 * M_PI;
            }

            double total_angle = end_angle - start_angle;

            double alpha = calcAlpha(total_angle / segments);

            RS_Vector start_point = calcEllipsePoint(center, majorradius, minorradius, x_axis_rotation, start_angle);

            std::string path = svgPathMoveTo(start_point);

            for (int i = 1; i <= segments; i++) {
                double segment_start_angle = start_angle + ((i - 1) / (double)segments) * total_angle;
                double segment_end_angle = start_angle + (i / (double)segments) * total_angle;

                RS_Vector segment_start_point = calcEllipsePoint(center, majorradius, minorradius, x_axis_rotation, segment_start_angle);
                RS_Vector segment_end_point = calcEllipsePoint(center, majorradius, minorradius, x_axis_rotation, segment_end_angle);

                RS_Vector segment_control_point_1 = segment_start_point + calcEllipsePointDerivative(majorradius, minorradius, x_axis_rotation, segment_start_angle) * alpha;
                RS_Vector segment_control_point_2 = segment_end_point - calcEllipsePointDerivative(majorradius, minorradius, x_axis_rotation, segment_end_angle) * alpha;

                path += svgPathCurveTo(segment_end_point, segment_control_point_1, segment_control_point_2);
            }

            e->set_attribute("d", path);
        }
        else {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse approximated by 'path' with 4 cubic bézier segments (as discussed in http://www.tinaja.com/glib/ellipse4.pdf) ...");

            const double kappa = 0.551784;

            xmlpp::Element* e = parent->add_child("path");

            RS_Vector major {majorradius, 0.0};
            RS_Vector minor {0.0, minorradius};

            RS_Vector flip_y {1.0, -1.0};

            major.rotate(ellipse->getAngle());
            minor.rotate(ellipse->getAngle());

            major.scale(flip_y);
            minor.scale(flip_y);

            RS_Vector offsetmajor {major * kappa};
            RS_Vector offsetminor {minor * kappa};

            std::string path = svgPathMoveTo(center - major) +
                               svgPathCurveTo((center - minor), (center - major - offsetminor), (center - minor - offsetmajor)) +
                               svgPathCurveTo((center + major), (center - minor + offsetmajor), (center + major - offsetminor)) +
                               svgPathCurveTo((center + minor), (center + major + offsetminor), (center + minor + offsetmajor)) +
                               svgPathCurveTo((center - major), (center + minor - offsetmajor), (center - major + offsetminor)) +
                               svgPathClose();

            e->set_attribute("d", path);
        }
    }
    else {

        if (ellipse->isArc()) {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse arc as 'path' with arc segments ...");

            xmlpp::Element* e = parent->add_child("path");

            double x_axis_rotation = 180 - (RS_Math::rad2deg(ellipse->getAngle()));

            double startangle = RS_Math::rad2deg(ellipse->getAngle1());
            double endangle = RS_Math::rad2deg(ellipse->getAngle2());

            if (endangle <= startangle) {
                endangle += 360;
            }

            bool large_arc_flag = ((endangle - startangle) > 180);
            bool sweep_flag = false;

            if (ellipse->isReversed()) {
                large_arc_flag = !large_arc_flag;
                sweep_flag = !sweep_flag;
            }

            std::string path = svgPathMoveTo(convertToSvg(ellipse->getStartpoint())) +
                               svgPathArc(convertToSvg(ellipse->getEndpoint()), majorradius, minorradius, x_axis_rotation, large_arc_flag, sweep_flag);

            e->set_attribute("d", path);
            e->set_attribute("stroke", "red");

        }
        else {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing full ellipse as 'ellipse' ...");

            double angle = 180 - (RS_Math::rad2deg(ellipse->getAngle()) - 90);

            xmlpp::Element* e = parent->add_child("ellipse");

            std::string transform = "translate(" + numXml(center.x) + ", " + numXml(center.y) + ") " +
                                    "rotate(" + numXml(angle) + ")";

            e->set_attribute("rx", numXml(minorradius));
            e->set_attribute("ry", numXml(majorradius));
            e->set_attribute("transform", transform);
        }
    }
}

std::string RS_MakerCamSVG::numXml(double value) {

    return RS_Utility::doubleToString(value, 8).toStdString();
}

RS_Vector RS_MakerCamSVG::convertToSvg(RS_Vector vector) {

    RS_Vector translated((vector.x - min.x), (max.y - vector.y));

    return translated + offset;
}

std::string RS_MakerCamSVG::svgPathClose() {

    return "Z ";
}

std::string RS_MakerCamSVG::svgPathCurveTo(RS_Vector point, RS_Vector controlpoint1, RS_Vector controlpoint2) {

    return "C" + numXml(controlpoint1.x) + "," + numXml(controlpoint1.y) + " " +
           numXml(controlpoint2.x) + "," + numXml(controlpoint2.y) + " " +
           numXml(point.x) + "," + numXml(point.y) + " ";
}

std::string RS_MakerCamSVG::svgPathLineTo(RS_Vector point) {

    return "L" + numXml(point.x) + "," + numXml(point.y) + " ";
}

std::string RS_MakerCamSVG::svgPathMoveTo(RS_Vector point) {

    return "M" + numXml(point.x) + "," + numXml(point.y) + " ";
}

std::string RS_MakerCamSVG::svgPathArc(RS_Arc* arc) {

    RS_Vector endpoint = convertToSvg(arc->getEndpoint());
    double radius = arc->getRadius();

    double startangle = RS_Math::rad2deg(arc->getAngle1());
    double endangle = RS_Math::rad2deg(arc->getAngle2());


    if (endangle <= startangle) {
        endangle += 360;
    }

    bool large_arc_flag = ((endangle - startangle) > 180);
    bool sweep_flag = false;

    if (arc->isReversed())
    {
        large_arc_flag = !large_arc_flag;
        sweep_flag = !sweep_flag;
    }

    return svgPathArc(endpoint, radius, radius, 0.0, large_arc_flag, sweep_flag);
}

std::string RS_MakerCamSVG::svgPathArc(RS_Vector point, double radius_x, double radius_y, double x_axis_rotation, bool large_arc_flag, bool sweep_flag) {

    return "A" + numXml(radius_x) + "," + numXml(radius_y) + " " +
           numXml(x_axis_rotation) + " " +
           (large_arc_flag ? "1" : "0") + "," +
           (sweep_flag ? "1" : "0") + " " +
           numXml(point.x) + "," + numXml(point.y) + " ";
}

RS_Vector RS_MakerCamSVG::calcEllipsePoint(RS_Vector center, double majorradius, double minorradius, double x_axis_rotation, double angle) {

    RS_Vector point(center.x + (majorradius * cos(x_axis_rotation) * cos(angle)) - (minorradius * sin(x_axis_rotation) * sin(angle)),
                    center.y + (majorradius * sin(x_axis_rotation) * cos(angle)) + (minorradius * cos(x_axis_rotation) * sin(angle)));

    return point;
}

RS_Vector RS_MakerCamSVG::calcEllipsePointDerivative(double majorradius, double minorradius, double x_axis_rotation, double angle) {

    RS_Vector vector((-majorradius * cos(x_axis_rotation) * sin(angle)) - (minorradius * sin(x_axis_rotation) * cos(angle)),
                     (-majorradius * sin(x_axis_rotation) * sin(angle)) + (minorradius * cos(x_axis_rotation) * cos(angle)));

    return vector;
}

double RS_MakerCamSVG::calcAlpha(double angle) {

    return sin(angle) * ((sqrt(4.0 + 3.0 * pow(tan(angle / 2), 2.0)) - 1.0) / 3.0);
}
