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
#include<cmath>
#include "lc_makercamsvg.h"

#include "lc_xmlwriterinterface.h"

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_polyline.h"
#include "rs_point.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "rs_graphic.h"
#include "rs_system.h"
#include "rs_text.h"
#include "rs_units.h"
#include "rs_utility.h"
#include "rs_math.h"
#include "rs_debug.h"

namespace {
const std::string NAMESPACE_URI_SVG = "http://www.w3.org/2000/svg";
const std::string NAMESPACE_URI_LC = "http://www.librecad.org";
const std::string NAMESPACE_URI_XLINK = "http://www.w3.org/1999/xlink";
}

LC_MakerCamSVG::LC_MakerCamSVG(LC_XMLWriterInterface* xmlWriter,
                               bool writeInvisibleLayers,
                               bool writeConstructionLayers,
							   bool writeBlocksInline,
                               bool convertEllipsesToBeziers,
                               bool exportImages,
                               bool convertLineTypes,
                               double defaultElementWidth,
                               double defaultDashLinePatternLength):
  xmlWriter(xmlWriter)
  ,writeInvisibleLayers(writeInvisibleLayers)
  ,writeConstructionLayers(writeConstructionLayers)
  ,writeBlocksInline(writeBlocksInline)
  ,convertEllipsesToBeziers(convertEllipsesToBeziers)
  ,exportImages(exportImages)
  ,convertLineTypes(convertLineTypes)
  ,defaultElementWidth(defaultElementWidth)
  ,defaultDashLinePatternLength(defaultDashLinePatternLength)

  ,offset(0.,0.)
{
    RS_DEBUG->print("RS_MakerCamSVG::RS_MakerCamSVG()");
}

bool LC_MakerCamSVG::generate(RS_Graphic* graphic) {

    write(graphic);

    return true;
}

std::string LC_MakerCamSVG::resultAsString() {

    return xmlWriter->documentAsString();
}

void LC_MakerCamSVG::write(RS_Graphic* graphic) {

    RS_DEBUG->print("RS_MakerCamSVG::write: Writing root node ...");

    graphic->calculateBorders();

    min = graphic->getMin();
    max = graphic->getMax();

    RS2::Unit raw_unit = graphic->getUnit();
    lengthFactor=1.;
    switch (raw_unit) {
        case RS2::Centimeter:
            unit = "cm";
            break;
        case RS2::Inch:
            unit = "in";
            break;

        default:
            lengthFactor=RS_Units::convert(1., raw_unit, RS2::Millimeter);
            // falling through will make default will use mm and convert length to mm
            // fall-through
        case RS2::Millimeter:
            unit = "mm";
            break;
    }

    xmlWriter->createRootElement("svg", NAMESPACE_URI_SVG);

    xmlWriter->addNamespaceDeclaration("lc", NAMESPACE_URI_LC);
    xmlWriter->addNamespaceDeclaration("xlink", NAMESPACE_URI_XLINK);

    xmlWriter->addAttribute("width", lengthXml(max.x - min.x) + unit);
    xmlWriter->addAttribute("height", lengthXml(max.y - min.y) + unit);
    xmlWriter->addAttribute("viewBox", "0 0 "+ lengthXml(max.x - min.x) + " " + lengthXml(max.y - min.y));

    writeBlocks(graphic);
    writeLayers(graphic);
}

void LC_MakerCamSVG::writeBlocks(RS_Document* document) {

    if (!writeBlocksInline) {

        RS_DEBUG->print("RS_MakerCamSVG::writeBlocks: Writing blocks ...");

        RS_BlockList* blocklist = document->getBlockList();

        if (blocklist->count() > 0) {

            xmlWriter->addElement("defs", NAMESPACE_URI_SVG);

            for (int i = 0; i < blocklist->count(); i++) {

                writeBlock(blocklist->at(i));
            }

            xmlWriter->closeElement();
        }

    }
}

void LC_MakerCamSVG::writeBlock(RS_Block* block) {

    RS_DEBUG->print("RS_MakerCamSVG::writeBlock: Writing block with name '%s'",
                    qPrintable(block->getName()));

    xmlWriter->addElement("g", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("id", std::to_string(block->getId()));
    xmlWriter->addAttribute("blockname", qPrintable(block->getName()), NAMESPACE_URI_LC);

    writeLayers(block);

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeLayers(RS_Document* document) {

    RS_DEBUG->print("RS_MakerCamSVG::writeLayers: Writing layers ...");

    RS_LayerList* layerlist = document->getLayerList();

    for (unsigned int i = 0; i < layerlist->count(); i++) {

        writeLayer(document, layerlist->at(i));
    }
}

void LC_MakerCamSVG::writeLayer(RS_Document* document, RS_Layer* layer) {

    if (writeInvisibleLayers || !layer->isFrozen()) {

        if (writeConstructionLayers || !layer->isConstruction()) {

            RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Writing layer with name '%s'",
                            qPrintable(layer->getName()));

            xmlWriter->addElement("g", NAMESPACE_URI_SVG);

            xmlWriter->addAttribute("layername", qPrintable(layer->getName()), NAMESPACE_URI_LC);
            xmlWriter->addAttribute("is_locked", (layer->isLocked() ? "true" : "false"), NAMESPACE_URI_LC);
            xmlWriter->addAttribute("is_construction", (layer->isConstruction() ? "true" : "false"), NAMESPACE_URI_LC);

            if (layer->isFrozen())
            {
                xmlWriter->addAttribute("style", "display: none;");
            }

            xmlWriter->addAttribute("fill", "none");
            xmlWriter->addAttribute("stroke", "black");
            xmlWriter->addAttribute("stroke-width", QString::number(defaultElementWidth).toStdString());

            writeEntities(document, layer);

            xmlWriter->closeElement();
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

void LC_MakerCamSVG::writeEntities(RS_Document* document, RS_Layer* layer) {

    RS_DEBUG->print("RS_MakerCamSVG::writeEntities: Writing entities from layer ...");

	for (auto e: *document) {

        if (e->getLayer() == layer) {

            if (!(e->getFlag(RS2::FlagUndone))) {

                writeEntity(e);
            }
        }
    }
}

void LC_MakerCamSVG::writeEntity(RS_Entity* entity) {

    RS_DEBUG->print("RS_MakerCamSVG::writeEntity: Found entity ...");

    switch (entity->rtti()) {
        case RS2::EntityInsert:
            writeInsert((RS_Insert*)entity);
            break;
        case RS2::EntityPoint:
            writePoint((RS_Point*)entity);
            break;
        case RS2::EntityLine:
            writeLine((RS_Line*)entity);
            break;
        case RS2::EntityPolyline:
            writePolyline((RS_Polyline*)entity);
            break;
        case RS2::EntityCircle:
            writeCircle((RS_Circle*)entity);
            break;
        case RS2::EntityArc:
            writeArc((RS_Arc*)entity);
            break;
        case RS2::EntityEllipse:
            writeEllipse((RS_Ellipse*)entity);
            break;
        case RS2::EntitySpline:
            writeSpline((RS_Spline*)entity);
            break;
        case RS2::EntitySplinePoints:
            writeSplinepoints((LC_SplinePoints*)entity);
            break;
        case RS2::EntityImage:
            writeImage((RS_Image*)entity);
            break;

        default:
            RS_DEBUG->print(RS_Debug::D_NOTICE,
                            "RS_MakerCamSVG::writeEntity: Entity with type '%d' not yet implemented",
                            (int)entity->rtti());
            break;
    }
}

void LC_MakerCamSVG::writeInsert(RS_Insert* insert) {

    RS_Block* block = insert->getBlockForInsert();

    RS_Vector insertionpoint = convertToSvg(insert->getInsertionPoint());

    if (writeBlocksInline) {

        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert inline ...");

        offset.set(insertionpoint.x, insertionpoint.y - (max.y - min.y));

        xmlWriter->addElement("g", NAMESPACE_URI_SVG);

        xmlWriter->addAttribute("blockname", qPrintable(block->getName()), NAMESPACE_URI_LC);

        writeLayers(block);

        xmlWriter->closeElement();

        offset.set(0, 0);
    }
    else {

        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert as reference to block ...");

        xmlWriter->addElement("use", NAMESPACE_URI_SVG);

        xmlWriter->addAttribute("x", lengthXml(insertionpoint.x));
        xmlWriter->addAttribute("y", lengthXml(insertionpoint.y - (max.y - min.y)));
        xmlWriter->addAttribute("href", "#" + std::to_string(block->getId()), NAMESPACE_URI_XLINK);

        xmlWriter->closeElement();
    }
}

void LC_MakerCamSVG::writePoint(RS_Point* point) {

    RS_DEBUG->print("RS_MakerCamSVG::writePoint: Writing point ...");

    // NOTE: There is no "point" element in SVG, therefore creating a circle
    //       with minimal radius.

    RS_Vector center = convertToSvg(point->getPos());

    xmlWriter->addElement("circle", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("cx", lengthXml(center.x));
    xmlWriter->addAttribute("cy", lengthXml(center.y));
    xmlWriter->addAttribute("r", lengthXml(0.1));

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeLine(RS_Line* line) {

    RS_DEBUG->print("RS_MakerCamSVG::writeLine: Writing line ...");

    RS_Vector startpoint = convertToSvg(line->getStartpoint());
    RS_Vector endpoint = convertToSvg(line->getEndpoint());

    RS_Pen pen = line->getPen();
    RS2::LineType lineType = pen.getLineType();

    if ((RS2::SolidLine != lineType) & convertLineTypes ) {
        RS_DEBUG->print("RS_MakerCamSVG::writeLine: write baked line as path");

        std::string path;
        path += svgPathAnyLineType(startpoint, endpoint, pen.getLineType());

        xmlWriter->addElement("path", NAMESPACE_URI_SVG);
        xmlWriter->addAttribute("id", QString::number(line->getId()).toStdString());
        if (RS2::Width00 != pen.getWidth()){
            xmlWriter->addAttribute("stroke-width", QString::number((pen.getWidth()/100.0)).toStdString());
         } else {
            xmlWriter->addAttribute("stroke-width", QString::number(defaultElementWidth).toStdString());
        }
        xmlWriter->addAttribute("d", path);
        xmlWriter->closeElement();
    }
    else {
        RS_DEBUG->print("RS_MakerCamSVG::writeLine: write standard line ");
        xmlWriter->addElement("line", NAMESPACE_URI_SVG);

        xmlWriter->addAttribute("x1", lengthXml(startpoint.x));
        xmlWriter->addAttribute("y1", lengthXml(startpoint.y));
        xmlWriter->addAttribute("x2", lengthXml(endpoint.x));
        xmlWriter->addAttribute("y2", lengthXml(endpoint.y));

        xmlWriter->closeElement();
    }
}

void LC_MakerCamSVG::writePolyline(RS_Polyline* polyline) {

    RS_DEBUG->print("RS_MakerCamSVG::writePolyline: Writing polyline ...");

    std::string path = svgPathMoveTo(convertToSvg(polyline->getStartpoint()));

	for (auto entity: *polyline) {

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

    xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("d", path);

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeCircle(RS_Circle* circle) {

    RS_DEBUG->print("RS_MakerCamSVG::writeCircle: Writing circle ...");

    RS_Vector center = convertToSvg(circle->getCenter());

    xmlWriter->addElement("circle", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("cx", lengthXml(center.x));
    xmlWriter->addAttribute("cy", lengthXml(center.y));
    xmlWriter->addAttribute("r", lengthXml(circle->getRadius()));

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeArc(RS_Arc* arc) {

    RS_DEBUG->print("RS_MakerCamSVG::writeArc: Writing arc ...");

    std::string path = svgPathMoveTo(convertToSvg(arc->getStartpoint())) +
                       svgPathArc(arc);

    xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("d", path);

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeEllipse(RS_Ellipse* ellipse) {

    RS_Vector center = convertToSvg(ellipse->getCenter());
	const RS_Vector centerTranslation=center - ellipse->getCenter();

    double majorradius = ellipse->getMajorRadius();
    double minorradius = ellipse->getMinorRadius();

    if (convertEllipsesToBeziers) {

        std::string path = "";

        if (ellipse->isEllipticArc()) {

            const int segments = 4;

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse arc approximated by 'path' with %d cubic bézier segments (as discussed in https://www.spaceroots.org/documents/ellipse/elliptical-arc.pdf) ...", segments);

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

			RS_Vector start_point = centerTranslation + ellipse->getEllipsePoint(start_angle);

            path = svgPathMoveTo(start_point);

            for (int i = 1; i <= segments; i++) {
                double segment_start_angle = start_angle + ((i - 1) / (double)segments) * total_angle;
                double segment_end_angle = start_angle + (i / (double)segments) * total_angle;

				RS_Vector segment_start_point = centerTranslation + ellipse->getEllipsePoint(segment_start_angle);
				RS_Vector segment_end_point = centerTranslation + ellipse->getEllipsePoint(segment_end_angle);

                RS_Vector segment_control_point_1 = segment_start_point + calcEllipsePointDerivative(majorradius, minorradius, x_axis_rotation, segment_start_angle) * alpha;
                RS_Vector segment_control_point_2 = segment_end_point - calcEllipsePointDerivative(majorradius, minorradius, x_axis_rotation, segment_end_angle) * alpha;

                path += svgPathCurveTo(segment_end_point, segment_control_point_1, segment_control_point_2);
            }
        }
        else {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse approximated by 'path' with 4 cubic bézier segments (as discussed in http://www.tinaja.com/glib/ellipse4.pdf) ...");

            const double kappa = 0.551784;

            RS_Vector major {majorradius, 0.0};
            RS_Vector minor {0.0, minorradius};

            RS_Vector flip_y {1.0, -1.0};

            major.rotate(ellipse->getAngle());
            minor.rotate(ellipse->getAngle());

            major.scale(flip_y);
            minor.scale(flip_y);

            RS_Vector offsetmajor {major * kappa};
            RS_Vector offsetminor {minor * kappa};

            path = svgPathMoveTo(center - major) +
                   svgPathCurveTo((center - minor), (center - major - offsetminor), (center - minor - offsetmajor)) +
                   svgPathCurveTo((center + major), (center - minor + offsetmajor), (center + major - offsetminor)) +
                   svgPathCurveTo((center + minor), (center + major + offsetminor), (center + minor + offsetmajor)) +
                   svgPathCurveTo((center - major), (center + minor - offsetmajor), (center - major + offsetminor)) +
                   svgPathClose();
        }

        xmlWriter->addElement("path", NAMESPACE_URI_SVG);

        xmlWriter->addAttribute("d", path);

        xmlWriter->closeElement();
    }
    else {

        if (ellipse->isEllipticArc()) {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse arc as 'path' with arc segments ...");

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

            xmlWriter->addElement("path", NAMESPACE_URI_SVG);

            xmlWriter->addAttribute("d", path);

            xmlWriter->closeElement();
        }
        else {

            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing full ellipse as 'ellipse' ...");

            double angle = 180 - (RS_Math::rad2deg(ellipse->getAngle()) - 90);

            std::string transform = "translate(" + lengthXml(center.x) + ", " + lengthXml(center.y) + ") " +
                                    "rotate(" + numXml(angle) + ")";

            xmlWriter->addElement("ellipse", NAMESPACE_URI_SVG);

            xmlWriter->addAttribute("rx", lengthXml(minorradius));
            xmlWriter->addAttribute("ry", lengthXml(majorradius));
            xmlWriter->addAttribute("transform", transform);

            xmlWriter->closeElement();
        }
    }
}

// NOTE: Quite obviously, the spline implementation in LibreCAD is a bit shaky.
//       It looks as if degree 1 to 3 splines are hold in the RS_Spline object
//       (if created using the control point method vs. the pass through point
//       method). However after saving degree 2 splines and reopening the file,
//       these splines are hold in the "artificial" LC_SplinePoints object.
void LC_MakerCamSVG::writeSpline(RS_Spline* spline) {

    if (spline->getDegree() == 2) {
        RS_DEBUG->print("RS_MakerCamSVG::writeSpline: Writing piecewise quadratic spline as 'path' with quadratic bézier segments");

        writeQuadraticBeziers(spline->getControlPoints(), spline->isClosed());
    }
    else if (spline->getDegree() == 3) {
        RS_DEBUG->print("RS_MakerCamSVG::writeSpline: Writing piecewise cubic spline as 'path' with cubic bézier segments");

        writeCubicBeziers(spline->getControlPoints(), spline->isClosed());
    }
    else {
        RS_DEBUG->print(RS_Debug::D_NOTICE,
                        "RS_MakerCamSVG::writePiecewiseCubicSpline: Splines with degree '%d' not implemented",
                        (int)spline->getDegree());
    }
}

void LC_MakerCamSVG::writeSplinepoints(LC_SplinePoints* splinepoints) {

    RS_DEBUG->print("RS_MakerCamSVG::writeSplinepoints: Writing piecewise quadratic spline as 'path' with quadratic bézier segments");

    writeQuadraticBeziers(splinepoints->getControlPoints(), splinepoints->isClosed());
}

void LC_MakerCamSVG::writeCubicBeziers(const std::vector<RS_Vector> &control_points, bool is_closed) {

    std::vector<RS_Vector> bezier_points = calcCubicBezierPoints(control_points, is_closed);

    std::string path = svgPathMoveTo(convertToSvg(bezier_points[0]));

    int bezier_points_size = bezier_points.size();

    int bezier_count = ((bezier_points_size - 1) / 3);

    for (int i = 0; i < bezier_count; i++) {
        path += svgPathCurveTo(convertToSvg(bezier_points[3 * (i + 1)]), convertToSvg(bezier_points[3 * (i + 1) - 2]), convertToSvg(bezier_points[3 * (i + 1) - 1]));
    }

    xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("d", path);

    xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeQuadraticBeziers(const std::vector<RS_Vector> &control_points, bool is_closed) {

    std::vector<RS_Vector> bezier_points = calcQuadraticBezierPoints(control_points, is_closed);

    std::string path = svgPathMoveTo(convertToSvg(bezier_points[0]));

    int bezier_points_size = bezier_points.size();

    int bezier_count = ((bezier_points_size - 1) / 2);

    for (int i = 0; i < bezier_count; i++) {
        path += svgPathQuadraticCurveTo(convertToSvg(bezier_points[2 * (i + 1)]), convertToSvg(bezier_points[2 * (i + 1) - 1]));
    }

    xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    xmlWriter->addAttribute("d", path);

    xmlWriter->closeElement();
}

std::vector<RS_Vector> LC_MakerCamSVG::calcCubicBezierPoints(const std::vector<RS_Vector> &control_points, bool is_closed) {

    std::vector<RS_Vector> bezier_points;

    int control_points_size = control_points.size();

    int bezier_points_size;

    if (is_closed) {

        for (int i = 0; i < (control_points_size - 1); i++) {
            bezier_points.push_back(control_points[i]);

            bezier_points.push_back((control_points[i] * 2.0 + control_points[i + 1]) / 3.0);
            bezier_points.push_back((control_points[i] + control_points[i + 1] * 2.0) / 3.0);
        }

        bezier_points.push_back(control_points[control_points_size - 1]);
        bezier_points.push_back((control_points[control_points_size - 1] * 2.0 + control_points[0]) / 3.0);
        bezier_points.push_back((control_points[control_points_size - 1] + control_points[0] * 2.0) / 3.0);
        bezier_points.push_back(control_points[0]);

        // Auxiliary points for easier calculation
        bezier_points.insert(bezier_points.begin(), ((control_points[control_points_size - 1] + control_points[0] * 2.0) / 3.0));
        bezier_points.push_back((control_points[0] * 2.0 + control_points[1]) / 3.0);

        bezier_points_size = bezier_points.size();

        for (int i = 1; i < (bezier_points_size - 1); i += 3) {
            bezier_points[i] = ((bezier_points[i - 1] + bezier_points[i + 1]) / 2.0);
        }

        // Remove auxiliary points
        bezier_points.pop_back();
        bezier_points.erase(bezier_points.begin());
    }
    else {
        // Extend control point list with interpolation points that act as control
        // points for the bezier curves
        for (int i = 0; i < (control_points_size - 1); i++) {
            bezier_points.push_back(control_points[i]);

            bool more_than_bezier = (control_points_size > 4);

            if (more_than_bezier) {

                bool first_or_last = ((i == 0) || (i == (control_points_size - 2)));

                if (!first_or_last) {

                    bool second_or_second_last = ((i == 1) || (i == (control_points_size - 3)));

                    if (second_or_second_last) {
                        bezier_points.push_back((control_points[i] + control_points[i + 1]) / 2.0);
                    }
                    else {
                        bezier_points.push_back((control_points[i] * 2.0 + control_points[i + 1]) / 3.0);
                        bezier_points.push_back((control_points[i] + control_points[i + 1] * 2.0) / 3.0);
                    }
                }
            }
        }

        bezier_points.push_back(control_points[control_points_size - 1]);

        bezier_points_size = bezier_points.size();

        // Update the up to now original spline control points to bezier endpoints
        for (int i = 3; i < (bezier_points_size - 1); i += 3) {
            bezier_points[i] = ((bezier_points[i - 1] + bezier_points[i + 1]) / 2.0);
        }
    }

    return bezier_points;
}

std::vector<RS_Vector> LC_MakerCamSVG::calcQuadraticBezierPoints(const std::vector<RS_Vector> &control_points, bool is_closed) {

    std::vector<RS_Vector> bezier_points;

    int control_points_size = control_points.size();

    if (is_closed) {
        for (int i = 0; i < (control_points_size - 1); i++) {
            bezier_points.push_back(control_points[i]);

            bezier_points.push_back((control_points[i] + control_points[i + 1]) / 2.0);
        }

        bezier_points.push_back(control_points[control_points_size - 1]);
        bezier_points.push_back((control_points[control_points_size - 1] + control_points[0]) / 2.0);
        bezier_points.push_back(control_points[0]);
        bezier_points.push_back((control_points[0] + control_points[1]) / 2.0);

        // Remove superfluous first point
        bezier_points.erase(bezier_points.begin());
    }
    else {
        for (int i = 0; i < (control_points_size - 1); i++) {
            bezier_points.push_back(control_points[i]);

            bool first_or_last = ((i == 0) || (i == (control_points_size - 2)));

            if (!first_or_last) {
                bezier_points.push_back((control_points[i] + control_points[i + 1]) / 2.0);
            }
        }

        bezier_points.push_back(control_points[control_points_size - 1]);
    }

    return bezier_points;
}

std::string LC_MakerCamSVG::numXml(double value) {

    return RS_Utility::doubleToString(value, 8).toStdString();
}

std::string LC_MakerCamSVG::lengthXml(double value) const
{
    return numXml(lengthFactor*value);
}

RS_Vector LC_MakerCamSVG::convertToSvg(RS_Vector vector) const
{

    RS_Vector translated((vector.x - min.x), (max.y - vector.y));

    return translated + offset;
}

std::string LC_MakerCamSVG::svgPathClose() const
{

    return "Z ";
}

std::string LC_MakerCamSVG::svgPathCurveTo(RS_Vector point, RS_Vector controlpoint1, RS_Vector controlpoint2) const
{
    return "C" + lengthXml(controlpoint1.x) + "," + lengthXml(controlpoint1.y) + " " +
           lengthXml(controlpoint2.x) + "," + lengthXml(controlpoint2.y) + " " +
           lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathQuadraticCurveTo(RS_Vector point, RS_Vector controlpoint) const
{
    return "Q" + lengthXml(controlpoint.x) + "," + lengthXml(controlpoint.y) + " " +
           lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathLineTo(RS_Vector point) const
{
    return "L" + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathMoveTo(RS_Vector point) const
{
    return "M" + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathArc(RS_Arc* arc) const
{
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

std::string LC_MakerCamSVG::svgPathArc(RS_Vector point, double radius_x, double radius_y, double x_axis_rotation, bool large_arc_flag, bool sweep_flag) const
{
    return "A" + lengthXml(radius_x) + "," + lengthXml(radius_y) + " " +
           numXml(x_axis_rotation) + " " +
           (large_arc_flag ? "1" : "0") + "," +
           (sweep_flag ? "1" : "0") + " " +
           lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

RS_Vector LC_MakerCamSVG::calcEllipsePointDerivative(double majorradius, double minorradius, double x_axis_rotation, double angle) const
{
    RS_Vector vector((-majorradius * cos(x_axis_rotation) * sin(angle)) - (minorradius * sin(x_axis_rotation) * cos(angle)),
                     (-majorradius * sin(x_axis_rotation) * sin(angle)) + (minorradius * cos(x_axis_rotation) * cos(angle)));

    return vector;
}

double LC_MakerCamSVG::calcAlpha(double angle) {

    return sin(angle) * ((sqrt(4.0 + 3.0 * pow(tan(angle / 2), 2.0)) - 1.0) / 3.0);
}

void LC_MakerCamSVG::writeImage(RS_Image* image)
{
    RS_DEBUG->print("RS_MakerCamSVG::writeImage: Writing image ...");
    if (exportImages){
        RS_Vector insertionPoint = convertToSvg(image->getInsertionPoint());

        xmlWriter->addElement("image", NAMESPACE_URI_SVG);
        xmlWriter->addAttribute("x", lengthXml(insertionPoint.x));
        xmlWriter->addAttribute("y", lengthXml(insertionPoint.y - image->getImageHeight()));
        xmlWriter->addAttribute("height", lengthXml(image->getImageHeight()));
        xmlWriter->addAttribute("width", lengthXml(image->getImageWidth()));
        xmlWriter->addAttribute("preserveAspectRatio", "none");  //height and width above used
        xmlWriter->addAttribute("href", image->getData().file.toStdString(), NAMESPACE_URI_XLINK);
        xmlWriter->closeElement();
    }
}


std::string LC_MakerCamSVG::svgPathAnyLineType(RS_Vector startpoint, RS_Vector endpoint, RS2::LineType type) const
{
    RS_DEBUG->print("RS_MakerCamSVG::svgPathUniLineType: convert line to dot/dash path");

    const int dotFactor     = 1;    // ..........
    const int dashFactor    = 3;    // -- -- -- --
    const int dashDotFactor = 4;    // --. --. --.
    const int divideFactor  = 5;    // --.. --.. --.. --..
    const int centerFactor  = 5;    // -- - -- - -- -
    const int borderFactor  = 7;    // -- -- . -- -- . -- -- .

    const double lineScaleTiny  = 0.25;
    const double lineScale2     = 0.5;
    const double lineScaleOne   = 1.0;
    const double lineScaleX2    = 2.0;

    std::string path;
    double lineScale;
    double lineFactor;

    double lineLengh = startpoint.distanceTo(endpoint);
    switch(type){

    case RS2::DotLineTiny:{ lineScale = lineScaleTiny; lineFactor = dotFactor; break;}
    case RS2::DotLine2:{ lineScale = lineScale2; lineFactor = dotFactor; break;}
    case RS2::DotLine:{ lineScale = lineScaleOne;  lineFactor = dotFactor; break;}
    case RS2::DotLineX2:{ lineScale = lineScaleX2; lineFactor = dotFactor; break;}

    case RS2::DashLineTiny:{ lineScale = lineScaleTiny; lineFactor = dashFactor; break;}
    case RS2::DashLine2:{ lineScale = lineScale2; lineFactor = dashFactor; break;}
    case RS2::DashLine:{ lineScale = lineScaleOne;  lineFactor = dashFactor; break;}
    case RS2::DashLineX2:{ lineScale = lineScaleX2; lineFactor = dashFactor; break;}

    case RS2::DashDotLineTiny:{ lineScale = lineScaleTiny; lineFactor = dashDotFactor; break;}
    case RS2::DashDotLine2:{ lineScale = lineScale2; lineFactor = dashDotFactor; break;}
    case RS2::DashDotLine:{ lineScale = lineScaleOne;  lineFactor = dashDotFactor; break;}
    case RS2::DashDotLineX2:{ lineScale = lineScaleX2; lineFactor = dashDotFactor; break;}

    case RS2::DivideLineTiny:{ lineScale = lineScaleTiny; lineFactor = divideFactor; break;}
    case RS2::DivideLine2:{ lineScale = lineScale2; lineFactor = divideFactor; break;}
    case RS2::DivideLine:{ lineScale = lineScaleOne;  lineFactor = divideFactor; break;}
    case RS2::DivideLineX2:{ lineScale = lineScaleX2; lineFactor = divideFactor; break;}

    case RS2::CenterLineTiny:{ lineScale = lineScaleTiny; lineFactor = centerFactor; break;}
    case RS2::CenterLine2:{ lineScale = lineScale2; lineFactor = centerFactor; break;}
    case RS2::CenterLine:{ lineScale = lineScaleOne;  lineFactor = centerFactor; break;}
    case RS2::CenterLineX2:{ lineScale = lineScaleX2; lineFactor = centerFactor; break;}

    case RS2::BorderLineTiny:{ lineScale = lineScaleTiny; lineFactor = borderFactor; break;}
    case RS2::BorderLine2:{ lineScale = lineScale2; lineFactor = borderFactor; break;}
    case RS2::BorderLine:{ lineScale = lineScaleOne;  lineFactor = borderFactor; break;}
    case RS2::BorderLineX2:{ lineScale = lineScaleX2; lineFactor = borderFactor; break;}
    default: { lineScale = lineScaleOne; lineFactor = dotFactor; break;}
    }

    //doesn't make an sense to have pattern longer than a line
    double dashLinePatternLength = defaultDashLinePatternLength;

    if (lineLengh < (dashLinePatternLength*lineFactor*lineScale)) {
        dashLinePatternLength = lineLengh/(lineFactor*lineScale);
        RS_DEBUG->print(RS_Debug::D_WARNING, "Line length shorter than a line pattern, updated length is %f mm", dashLinePatternLength);
    }

    double lineStep = lineScale*dashLinePatternLength*lineFactor;
    int numOfIter = round(lineLengh/lineStep);

    RS_Vector step((endpoint.x-startpoint.x)/numOfIter,(endpoint.y-startpoint.y)/numOfIter);
    RS_Vector lastPos(startpoint.x, startpoint.y);

    for (int i=0; i< numOfIter; i++){
        path += getLinePattern(&lastPos, step, type, (1.0/lineFactor) );
    }

    return path;
}

std::string LC_MakerCamSVG::getLinePattern(RS_Vector *lastPos, RS_Vector step, RS2::LineType type, double lineScale) const
{
    std::string path;

    switch(type){
    case RS2::DotLineTiny:
    case RS2::DotLine2:
    case RS2::DotLine:
    case RS2::DotLineX2:{
        path += getPointSegment(lastPos, step, lineScale);
        break;
    }

    case RS2::DashLineTiny:
    case RS2::DashLine2:
    case RS2::DashLine:
    case RS2::DashLineX2:{
        path += getLineSegment(lastPos, step, lineScale, true);
        break;
    }

    case RS2::DashDotLineTiny:
    case RS2::DashDotLine2:
    case RS2::DashDotLine:
    case RS2::DashDotLineX2:{
        path += getLineSegment(lastPos, step, lineScale, true);
        path += getPointSegment(lastPos, step, lineScale);
        break;
    }

    case RS2::DivideLineTiny:
    case RS2::DivideLine2:
    case RS2::DivideLine:
    case RS2::DivideLineX2: {
        path += getLineSegment(lastPos, step, lineScale, true);
        path += getPointSegment(lastPos, step, lineScale);
        path += getPointSegment(lastPos, step, lineScale);
        break;
    }

    case RS2::CenterLineTiny:
    case RS2::CenterLine2:
    case RS2::CenterLine:
    case RS2::CenterLineX2:{
        path += getLineSegment(lastPos, step, lineScale, true);
        path += getLineSegment(lastPos, step, lineScale, false);
        break;
    }

    case RS2::BorderLineTiny:
    case RS2::BorderLine2:
    case RS2::BorderLine:
    case RS2::BorderLineX2:{
        path += getLineSegment(lastPos, step, lineScale, true);
        path += getLineSegment(lastPos, step, lineScale, true);
        path += getPointSegment(lastPos, step, lineScale);
        break;
    }

    default:{
        RS_DEBUG->print(RS_Debug::D_WARNING,"RS_MakerCamSVG::getLinePattern: unsupported line type %d\n", type);
        path += svgPathMoveTo(convertToSvg(*lastPos));
        *lastPos += step*lineScale;
        path += svgPathLineTo(convertToSvg(*lastPos));
        break;
    }
    }
    return path;
}

std::string LC_MakerCamSVG::getPointSegment(RS_Vector *lastPos, RS_Vector step, double lineScale) const
{
    std::string path;
    //0.2 - is a diametr of point on early implementation of MakerCAM.
    //! \todo need to add a option to control this value from export dialog and test on laser engraver
    const double dotSize = 0.2;
    double scaleTo;

    if (fabs(step.x) >= fabs(step.y)){
        scaleTo = dotSize/fabs(step.x);
    } else {
        scaleTo = dotSize/fabs(step.y);
    }
    path += svgPathMoveTo(*lastPos);
    path += svgPathLineTo(*lastPos+step*scaleTo);
    *lastPos += step*lineScale;
    return path;
}

std::string LC_MakerCamSVG::getLineSegment(RS_Vector *lastPos, RS_Vector step, double lineScale, bool x2)const
{
    std::string path;
    path += svgPathMoveTo(*lastPos);
    if (x2)
        *lastPos += (step*lineScale*2);
    else
        *lastPos += (step*lineScale);
    path += svgPathLineTo(*lastPos);
    *lastPos += step*lineScale;
    return path;
}
