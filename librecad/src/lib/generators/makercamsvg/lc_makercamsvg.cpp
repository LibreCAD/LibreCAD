// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

#include "lc_makercamsvg.h"

#include "lc_splinepoints.h"
#include "lc_xmlwriterinterface.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_spline.h"
#include "rs_units.h"
#include "rs_utility.h"

namespace {
    const std::string NAMESPACE_URI_SVG = "http://www.w3.org/2000/svg";
    const std::string NAMESPACE_URI_LC = "https://librecad.org";
    const std::string NAMESPACE_URI_XLINK = "http://www.w3.org/1999/xlink";
}

LC_MakerCamSVG::LC_MakerCamSVG(std::unique_ptr<LC_XMLWriterInterface> xmlWriter, const bool writeInvisibleLayers,
                               const bool writeConstructionLayers, const bool writeBlocksInline, const bool convertEllipsesToBeziers,
                               const bool exportImages, const bool convertLineTypes, const double defaultElementWidth,
                               const double defaultDashLinePatternLength) : m_xmlWriter(std::move(xmlWriter)),
                                                                            m_writeInvisibleLayers(writeInvisibleLayers),
                                                                            m_writeConstructionLayers(writeConstructionLayers),
                                                                            m_writeBlocksInline(writeBlocksInline),
                                                                            m_convertEllipsesToBeziers(convertEllipsesToBeziers),
                                                                            m_exportImages(exportImages), m_convertLineTypes(convertLineTypes),
                                                                            m_defaultElementWidth(defaultElementWidth),
                                                                            m_defaultDashLinePatternLength(defaultDashLinePatternLength),
                                                                            m_offset(0., 0.) {
    RS_DEBUG->print("RS_MakerCamSVG::RS_MakerCamSVG()");
}

bool LC_MakerCamSVG::generate(RS_Graphic* graphic) {
    write(graphic);

    return true;
}

std::string LC_MakerCamSVG::resultAsString() const {
    return m_xmlWriter->documentAsString();
}

void LC_MakerCamSVG::write(RS_Graphic* graphic) {
    RS_DEBUG->print("RS_MakerCamSVG::write: Writing root node ...");

    graphic->calculateBorders();

    m_min = graphic->getMin();
    m_max = graphic->getMax();

    const RS2::Unit raw_unit = graphic->getUnit();
    m_lengthFactor = 1.;
    switch (raw_unit) {
        case RS2::Centimeter:
            m_unit = "cm";
            break;
        case RS2::Inch:
            m_unit = "in";
            break;

        default:
            m_lengthFactor = RS_Units::convert(1., raw_unit, RS2::Millimeter);
        // falling through will make default will use mm and convert length to mm
        // fall-through
        case RS2::Millimeter:
            m_unit = "mm";
            break;
    }

    m_xmlWriter->createRootElement("svg", NAMESPACE_URI_SVG);

    m_xmlWriter->addNamespaceDeclaration("lc", NAMESPACE_URI_LC);
    m_xmlWriter->addNamespaceDeclaration("xlink", NAMESPACE_URI_XLINK);

    m_xmlWriter->addAttribute("width", lengthXml(m_max.x - m_min.x) + m_unit);
    m_xmlWriter->addAttribute("height", lengthXml(m_max.y - m_min.y) + m_unit);
    m_xmlWriter->addAttribute("viewBox", "0 0 " + lengthXml(m_max.x - m_min.x) + " " + lengthXml(m_max.y - m_min.y));

    writeBlocks(graphic);
    writeLayers(graphic);
}

void LC_MakerCamSVG::writeBlocks(RS_Document* document) {
    if (!m_writeBlocksInline) {
        RS_DEBUG->print("RS_MakerCamSVG::writeBlocks: Writing blocks ...");

        RS_BlockList* blocklist = document->getBlockList();

        if (blocklist->count() > 0) {
            m_xmlWriter->addElement("defs", NAMESPACE_URI_SVG);

            for (int i = 0; i < blocklist->count(); i++) {
                writeBlock(blocklist->at(i));
            }

            m_xmlWriter->closeElement();
        }
    }
}

void LC_MakerCamSVG::writeBlock(RS_Block* block) {
    RS_DEBUG->print("RS_MakerCamSVG::writeBlock: Writing block with name '%s'", qPrintable(block->getName()));

    m_xmlWriter->addElement("g", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("id", std::to_string(block->getId()));
    m_xmlWriter->addAttribute("blockname", qPrintable(block->getName()), NAMESPACE_URI_LC);

    writeLayers(block);

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeLayers(RS_Document* document) {
    RS_DEBUG->print("RS_MakerCamSVG::writeLayers: Writing layers ...");

    const RS_LayerList* layerlist = document->getLayerList();

    for (unsigned int i = 0; i < layerlist->count(); i++) {
        writeLayer(document, layerlist->at(i));
    }
}

void LC_MakerCamSVG::writeLayer(RS_Document* document, const RS_Layer* layer) {
    if (m_writeInvisibleLayers || !layer->isFrozen()) {
        if (m_writeConstructionLayers || !layer->isConstruction()) {
            RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Writing layer with name '%s'", qPrintable(layer->getName()));

            m_xmlWriter->addElement("g", NAMESPACE_URI_SVG);

            m_xmlWriter->addAttribute("layername", qPrintable(layer->getName()), NAMESPACE_URI_LC);
            m_xmlWriter->addAttribute("is_locked", layer->isLocked() ? "true" : "false", NAMESPACE_URI_LC);
            m_xmlWriter->addAttribute("is_construction", layer->isConstruction() ? "true" : "false", NAMESPACE_URI_LC);

            if (layer->isFrozen()) {
                m_xmlWriter->addAttribute("style", "display: none;");
            }

            m_xmlWriter->addAttribute("fill", "none");
            m_xmlWriter->addAttribute("stroke", "black");
            m_xmlWriter->addAttribute("stroke-width", QString::number(m_defaultElementWidth).toStdString());

            writeEntities(document, layer);

            m_xmlWriter->closeElement();
        }
        else {
            RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Omitting construction layer with name '%s'", qPrintable(layer->getName()));
        }
    }
    else {
        RS_DEBUG->print("RS_MakerCamSVG::writeLayer: Omitting invisible layer with name '%s'", qPrintable(layer->getName()));
    }
}

void LC_MakerCamSVG::writeEntities(RS_Document* document, const RS_Layer* layer) {
    RS_DEBUG->print("RS_MakerCamSVG::writeEntities: Writing entities from layer ...");
    for (const auto e : *document) {
        if (e->getLayer() == layer) {
            if (e->isAlive()) {
                writeEntity(e);
            }
        }
    }
}

void LC_MakerCamSVG::writeEntity(RS_Entity* entity) {
    RS_DEBUG->print("RS_MakerCamSVG::writeEntity: Found entity ...");

    switch (entity->rtti()) {
        case RS2::EntityInsert:
            writeInsert(static_cast<RS_Insert*>(entity));
            break;
        case RS2::EntityPoint:
            if (m_exportPoints) {
                writePoint(static_cast<RS_Point*>(entity));
            }
            break;
        case RS2::EntityLine:
            writeLine(static_cast<RS_Line*>(entity));
            break;
        case RS2::EntityPolyline:
            writePolyline(static_cast<RS_Polyline*>(entity));
            break;
        case RS2::EntityCircle:
            writeCircle(static_cast<RS_Circle*>(entity));
            break;
        case RS2::EntityArc:
            writeArc(static_cast<RS_Arc*>(entity));
            break;
        case RS2::EntityEllipse:
            writeEllipse(static_cast<RS_Ellipse*>(entity));
            break;
        case RS2::EntitySpline:
            writeSpline(static_cast<RS_Spline*>(entity));
            break;
        case RS2::EntitySplinePoints:
            writeSplinepoints(static_cast<LC_SplinePoints*>(entity));
            break;
        case RS2::EntityImage:
            writeImage(static_cast<RS_Image*>(entity));
            break;

        default: RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_MakerCamSVG::writeEntity: Entity with type '%d' not yet implemented",
                                 static_cast<int>(entity->rtti()));
            break;
    }
}

void LC_MakerCamSVG::writeInsert(const RS_Insert* insert) {
    RS_Block* block = insert->getBlockForInsert();
    if (block == nullptr) {
        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Insert references missing block, skipping");
        return;
    }

    const RS_Vector insertionpoint = insert->getInsertionPoint();
    // The conversion from drawing space to the svg space (column major) transform matrix(M):
    // 1  0  0
    // 0 -1  max.y
    // 0  0  1
    // Or: MV = RV + T, with R the rotation part, T the translation part, T=(0; max.y)
    // For insertions the transform should be M(V + I), with I as the insertion point offset
    // M(V+I) = R(V+I) + T = M(V) + RI
    // Therefore, insertion point offset means RI in SVG offset
    // (x, y) -> (x, -y)

    if (m_writeBlocksInline) {
        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert inline ...");

        m_offset.set(insertionpoint.x, -insertionpoint.y);

        m_xmlWriter->addElement("g", NAMESPACE_URI_SVG);

        m_xmlWriter->addAttribute("blockname", qPrintable(block->getName()), NAMESPACE_URI_LC);

        writeLayers(block);

        m_xmlWriter->closeElement();

        m_offset.set(0, 0);
    }
    else {
        RS_DEBUG->print("RS_MakerCamSVG::writeInsert: Writing insert as reference to block ...");

        m_xmlWriter->addElement("use", NAMESPACE_URI_SVG);

        m_xmlWriter->addAttribute("x", lengthXml(insertionpoint.x));
        m_xmlWriter->addAttribute("y", lengthXml(-insertionpoint.y));
        m_xmlWriter->addAttribute("href", "#" + std::to_string(block->getId()), NAMESPACE_URI_XLINK);

        m_xmlWriter->closeElement();
    }
}

void LC_MakerCamSVG::writePoint(const RS_Point* point) const {
    RS_DEBUG->print("RS_MakerCamSVG::writePoint: Writing point ...");

    // NOTE: There is no "point" element in SVG, therefore creating a circle
    //       with minimal radius.

    const RS_Vector center = convertToSvg(point->getPos());

    m_xmlWriter->addElement("circle", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("cx", lengthXml(center.x));
    m_xmlWriter->addAttribute("cy", lengthXml(center.y));
    m_xmlWriter->addAttribute("r", lengthXml(0.1));

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeLine(const RS_Line* line) const {
    RS_DEBUG->print("RS_MakerCamSVG::writeLine: Writing line ...");

    const RS_Vector startpoint = convertToSvg(line->getStartpoint());
    const RS_Vector endpoint = convertToSvg(line->getEndpoint());

    const RS_Pen pen = line->getPen();
    const RS2::LineType lineType = pen.getLineType();

    if (static_cast<int>(RS2::SolidLine != lineType) & m_convertLineTypes) {
        RS_DEBUG->print("RS_MakerCamSVG::writeLine: write baked line as path");

        std::string path;
        path += svgPathAnyLineType(startpoint, endpoint, pen.getLineType());

        m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);
        m_xmlWriter->addAttribute("id", QString::number(line->getId()).toStdString());
        if (RS2::Width00 != pen.getWidth()) {
            m_xmlWriter->addAttribute("stroke-width", QString::number(pen.getWidth() / 100.0).toStdString());
        }
        else {
            m_xmlWriter->addAttribute("stroke-width", QString::number(m_defaultElementWidth).toStdString());
        }
        m_xmlWriter->addAttribute("d", path);
        m_xmlWriter->closeElement();
    }
    else {
        RS_DEBUG->print("RS_MakerCamSVG::writeLine: write standard line ");
        m_xmlWriter->addElement("line", NAMESPACE_URI_SVG);

        m_xmlWriter->addAttribute("x1", lengthXml(startpoint.x));
        m_xmlWriter->addAttribute("y1", lengthXml(startpoint.y));
        m_xmlWriter->addAttribute("x2", lengthXml(endpoint.x));
        m_xmlWriter->addAttribute("y2", lengthXml(endpoint.y));

        m_xmlWriter->closeElement();
    }
}

void LC_MakerCamSVG::writePolyline(RS_Polyline* polyline) const {
    RS_DEBUG->print("RS_MakerCamSVG::writePolyline: Writing polyline ...");

    std::string path = svgPathMoveTo(convertToSvg(polyline->getStartpoint()));

    for (const auto entity : *polyline) {
        if (!entity->isAtomic()) {
            continue;
        }

        if (entity->rtti() == RS2::EntityArc) {
            path += svgPathArc(static_cast<RS_Arc*>(entity));
        }
        else {
            path += svgPathLineTo(convertToSvg(entity->getEndpoint()));
        }
    }

    if (polyline->isClosed()) {
        path += svgPathClose();
    }

    m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("d", path);

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeCircle(const RS_Circle* circle) const {
    RS_DEBUG->print("RS_MakerCamSVG::writeCircle: Writing circle ...");

    const RS_Vector center = convertToSvg(circle->getCenter());

    m_xmlWriter->addElement("circle", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("cx", lengthXml(center.x));
    m_xmlWriter->addAttribute("cy", lengthXml(center.y));
    m_xmlWriter->addAttribute("r", lengthXml(circle->getRadius()));

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeArc(const RS_Arc* arc) const {
    RS_DEBUG->print("RS_MakerCamSVG::writeArc: Writing arc ...");

    const std::string path = svgPathMoveTo(convertToSvg(arc->getStartpoint())) + svgPathArc(arc);

    m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("d", path);

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeEllipse(RS_Ellipse* ellipse) const {
    RS_Vector center = convertToSvg(ellipse->getCenter());
    double majorradius = ellipse->getMajorRadius();
    double minorradius = ellipse->getMinorRadius();

    if (m_convertEllipsesToBeziers) {
        std::string path = "";

        if (ellipse->isEllipticArc()) {
            constexpr int segments = 4;

            RS_DEBUG->print(
                "RS_MakerCamSVG::writeEllipse: Writing ellipse arc approximated by 'path' with %d cubic bézier segments (as discussed in https://www.spaceroots.org/documents/ellipse/elliptical-arc.pdf) ...",
                segments);

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
            const RS_Vector centerTranslation = center - ellipse->getCenter();
            RS_Vector start_point = centerTranslation + ellipse->getEllipsePoint(start_angle);

            path = svgPathMoveTo(start_point);

            for (int i = 1; i <= segments; i++) {
                double segment_start_angle = start_angle + (i - 1) / static_cast<double>(segments) * total_angle;
                double segment_end_angle = start_angle + i / static_cast<double>(segments) * total_angle;

                RS_Vector segment_start_point = centerTranslation + ellipse->getEllipsePoint(segment_start_angle);
                RS_Vector segment_end_point = centerTranslation + ellipse->getEllipsePoint(segment_end_angle);

                RS_Vector segment_control_point_1 = segment_start_point + calcEllipsePointDerivative(
                    majorradius, minorradius, x_axis_rotation, segment_start_angle) * alpha;
                RS_Vector segment_control_point_2 = segment_end_point - calcEllipsePointDerivative(
                    majorradius, minorradius, x_axis_rotation, segment_end_angle) * alpha;

                path += svgPathCurveTo(segment_end_point, segment_control_point_1, segment_control_point_2);
            }
        }
        else {
            RS_DEBUG->print(
                "RS_MakerCamSVG::writeEllipse: Writing ellipse approximated by 'path' with 4 cubic bézier segments (as discussed in http://www.tinaja.com/glib/ellipse4.pdf) ...");

            constexpr double kappa = 0.551784;

            RS_Vector major{majorradius, 0.0};
            RS_Vector minor{0.0, minorradius};

            RS_Vector flip_y{1.0, -1.0};

            major.rotate(ellipse->getAngle());
            minor.rotate(ellipse->getAngle());

            major.scale(flip_y);
            minor.scale(flip_y);

            RS_Vector offsetmajor{major * kappa};
            RS_Vector offsetminor{minor * kappa};

            path = svgPathMoveTo(center - major) +
                svgPathCurveTo(center - minor, center - major - offsetminor, center - minor - offsetmajor) +
                svgPathCurveTo(center + major, center - minor + offsetmajor, center + major - offsetminor) + svgPathCurveTo(
                    center + minor, center + major + offsetminor, center + minor + offsetmajor) + svgPathCurveTo(
                    center - major, center + minor - offsetmajor, center - major + offsetminor) + svgPathClose();
        }

        m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

        m_xmlWriter->addAttribute("d", path);

        m_xmlWriter->closeElement();
    }
    else {
        if (ellipse->isEllipticArc()) {
            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing ellipse arc as 'path' with arc segments ...");

            double x_axis_rotation = 180 - RS_Math::rad2deg(ellipse->getAngle());

            double startangle = RS_Math::rad2deg(ellipse->getAngle1());
            double endangle = RS_Math::rad2deg(ellipse->getAngle2());

            if (endangle <= startangle) {
                endangle += 360;
            }

            bool large_arc_flag = endangle - startangle > 180;
            bool sweep_flag = false;

            if (ellipse->isReversed()) {
                large_arc_flag = !large_arc_flag;
                sweep_flag = !sweep_flag;
            }

            std::string path = svgPathMoveTo(convertToSvg(ellipse->getStartpoint())) + svgPathArc(
                convertToSvg(ellipse->getEndpoint()), majorradius, minorradius, x_axis_rotation, large_arc_flag, sweep_flag);

            m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

            m_xmlWriter->addAttribute("d", path);

            m_xmlWriter->closeElement();
        }
        else {
            RS_DEBUG->print("RS_MakerCamSVG::writeEllipse: Writing full ellipse as 'ellipse' ...");

            double angle = 180 - (RS_Math::rad2deg(ellipse->getAngle()) - 90);

            std::string transform = "translate(" + lengthXml(center.x) + ", " + lengthXml(center.y) + ") " + "rotate(" + numXml(angle) +
                ")";

            m_xmlWriter->addElement("ellipse", NAMESPACE_URI_SVG);

            m_xmlWriter->addAttribute("rx", lengthXml(minorradius));
            m_xmlWriter->addAttribute("ry", lengthXml(majorradius));
            m_xmlWriter->addAttribute("transform", transform);

            m_xmlWriter->closeElement();
        }
    }
}

// NOTE: Quite obviously, the spline implementation in LibreCAD is a bit shaky.
//       It looks as if degree 1 to 3 splines are hold in the RS_Spline object
//       (if created using the control point method vs. the pass through point
//       method). However after saving degree 2 splines and reopening the file,
//       these splines are hold in the "artificial" LC_SplinePoints object.
void LC_MakerCamSVG::writeSpline(const RS_Spline* spline) {
    if (spline->getDegree() == 2) {
        RS_DEBUG->print("RS_MakerCamSVG::writeSpline: Writing piecewise quadratic spline as 'path' with quadratic bézier segments");

        writeQuadraticBeziers(spline->getControlPoints(), spline->isClosed());
    }
    else if (spline->getDegree() == 3) {
        RS_DEBUG->print("RS_MakerCamSVG::writeSpline: Writing piecewise cubic spline as 'path' with cubic bézier segments");

        writeCubicBeziers(spline->getControlPoints(), spline->isClosed());
    }
    else {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "RS_MakerCamSVG::writePiecewiseCubicSpline: Splines with degree '%d' not implemented",
                        spline->getDegree());
    }
}

void LC_MakerCamSVG::writeSplinepoints(const LC_SplinePoints* splinepoints) {
    RS_DEBUG->print("RS_MakerCamSVG::writeSplinepoints: Writing piecewise quadratic spline as 'path' with quadratic bézier segments");

    writeQuadraticBeziers(splinepoints->getControlPoints(), splinepoints->isClosed());
}

void LC_MakerCamSVG::writeCubicBeziers(const std::vector<RS_Vector>& controlPoints, const bool isClosed) {
    const std::vector<RS_Vector> bezier_points = calcCubicBezierPoints(controlPoints, isClosed);

    std::string path = svgPathMoveTo(convertToSvg(bezier_points[0]));

    const size_t bezier_points_size = bezier_points.size();

    const size_t bezier_count = (bezier_points_size - 1) / 3;

    for (size_t i = 0; i < bezier_count; i++) {
        path += svgPathCurveTo(convertToSvg(bezier_points[3 * (i + 1)]), convertToSvg(bezier_points[3 * (i + 1) - 2]),
                               convertToSvg(bezier_points[3 * (i + 1) - 1]));
    }

    m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("d", path);

    m_xmlWriter->closeElement();
}

void LC_MakerCamSVG::writeQuadraticBeziers(const std::vector<RS_Vector>& controlPoints, const bool isClosed) {
    const std::vector<RS_Vector> bezier_points = calcQuadraticBezierPoints(controlPoints, isClosed);

    std::string path = svgPathMoveTo(convertToSvg(bezier_points[0]));

    const size_t bezier_points_size = bezier_points.size();

    const int bezier_count = (bezier_points_size - 1) / 2;

    for (int i = 0; i < bezier_count; i++) {
        path += svgPathQuadraticCurveTo(convertToSvg(bezier_points[2 * (i + 1)]), convertToSvg(bezier_points[2 * (i + 1) - 1]));
    }

    m_xmlWriter->addElement("path", NAMESPACE_URI_SVG);

    m_xmlWriter->addAttribute("d", path);

    m_xmlWriter->closeElement();
}

std::vector<RS_Vector> LC_MakerCamSVG::calcCubicBezierPoints(const std::vector<RS_Vector>& controlPoints, const bool isClosed) {
    std::vector<RS_Vector> bezier_points;

    const size_t control_points_size = controlPoints.size();

    size_t bezier_points_size;

    if (isClosed) {
        for (size_t i = 0; i < control_points_size - 1; i++) {
            bezier_points.push_back(controlPoints[i]);

            bezier_points.push_back((controlPoints[i] * 2.0 + controlPoints[i + 1]) / 3.0);
            bezier_points.push_back((controlPoints[i] + controlPoints[i + 1] * 2.0) / 3.0);
        }

        bezier_points.push_back(controlPoints[control_points_size - 1]);
        bezier_points.push_back((controlPoints[control_points_size - 1] * 2.0 + controlPoints[0]) / 3.0);
        bezier_points.push_back((controlPoints[control_points_size - 1] + controlPoints[0] * 2.0) / 3.0);
        bezier_points.push_back(controlPoints[0]);

        // Auxiliary points for easier calculation
        bezier_points.insert(bezier_points.begin(), (controlPoints[control_points_size - 1] + controlPoints[0] * 2.0) / 3.0);
        bezier_points.push_back((controlPoints[0] * 2.0 + controlPoints[1]) / 3.0);

        bezier_points_size = bezier_points.size();

        for (size_t i = 1; i < bezier_points_size - 1; i += 3) {
            bezier_points[i] = (bezier_points[i - 1] + bezier_points[i + 1]) / 2.0;
        }

        // Remove auxiliary points
        bezier_points.pop_back();
        bezier_points.erase(bezier_points.begin());
    }
    else {
        // Extend control point list with interpolation points that act as control
        // points for the bezier curves
        for (size_t i = 0; i < control_points_size - 1; i++) {
            bezier_points.push_back(controlPoints[i]);

            const bool more_than_bezier = control_points_size > 4;

            if (more_than_bezier) {
                const bool first_or_last = i == 0 || i == control_points_size - 2;

                if (!first_or_last) {
                    const bool second_or_second_last = i == 1 || i == control_points_size - 3;

                    if (second_or_second_last) {
                        bezier_points.push_back((controlPoints[i] + controlPoints[i + 1]) / 2.0);
                    }
                    else {
                        bezier_points.push_back((controlPoints[i] * 2.0 + controlPoints[i + 1]) / 3.0);
                        bezier_points.push_back((controlPoints[i] + controlPoints[i + 1] * 2.0) / 3.0);
                    }
                }
            }
        }

        bezier_points.push_back(controlPoints[control_points_size - 1]);

        bezier_points_size = bezier_points.size();

        // Update the up to now original spline control points to bezier endpoints
        for (size_t i = 3; i < bezier_points_size - 1; i += 3) {
            bezier_points[i] = (bezier_points[i - 1] + bezier_points[i + 1]) / 2.0;
        }
    }

    return bezier_points;
}

std::vector<RS_Vector> LC_MakerCamSVG::calcQuadraticBezierPoints(const std::vector<RS_Vector>& controlPoints, const bool isClosed) {
    std::vector<RS_Vector> bezier_points;

    const size_t control_points_size = controlPoints.size();

    if (isClosed) {
        for (size_t i = 0; i < control_points_size - 1; i++) {
            bezier_points.push_back(controlPoints[i]);

            bezier_points.push_back((controlPoints[i] + controlPoints[i + 1]) / 2.0);
        }

        bezier_points.push_back(controlPoints[control_points_size - 1]);
        bezier_points.push_back((controlPoints[control_points_size - 1] + controlPoints[0]) / 2.0);
        bezier_points.push_back(controlPoints[0]);
        bezier_points.push_back((controlPoints[0] + controlPoints[1]) / 2.0);

        // Remove superfluous first point
        bezier_points.erase(bezier_points.begin());
    }
    else {
        for (size_t i = 0; i < control_points_size - 1; i++) {
            bezier_points.push_back(controlPoints[i]);

            const bool first_or_last = i == 0 || i == control_points_size - 2;

            if (!first_or_last) {
                bezier_points.push_back((controlPoints[i] + controlPoints[i + 1]) / 2.0);
            }
        }

        bezier_points.push_back(controlPoints[control_points_size - 1]);
    }

    return bezier_points;
}

std::string LC_MakerCamSVG::numXml(const double value) {
    return RS_Utility::doubleToString(value, 8).toStdString();
}

std::string LC_MakerCamSVG::lengthXml(const double value) const {
    return numXml(m_lengthFactor * value);
}

RS_Vector LC_MakerCamSVG::convertToSvg(const RS_Vector& vector) const {
    const RS_Vector translated(vector.x - m_min.x, m_max.y - vector.y);

    return translated + m_offset;
}

std::string LC_MakerCamSVG::svgPathClose() const {
    return "Z ";
}

std::string LC_MakerCamSVG::svgPathCurveTo(const RS_Vector& point, const RS_Vector& controlpoint1, const RS_Vector& controlpoint2) const {
    return "C" + lengthXml(controlpoint1.x) + "," + lengthXml(controlpoint1.y) + " " + lengthXml(controlpoint2.x) + "," +
        lengthXml(controlpoint2.y) + " " + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathQuadraticCurveTo(const RS_Vector& point, const RS_Vector& controlpoint) const {
    return "Q" + lengthXml(controlpoint.x) + "," + lengthXml(controlpoint.y) + " " + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathLineTo(const RS_Vector& point) const {
    return "L" + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathMoveTo(const RS_Vector& point) const {
    return "M" + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

std::string LC_MakerCamSVG::svgPathArc(const RS_Arc* arc) const {
    const RS_Vector endpoint = convertToSvg(arc->getEndpoint());
    const double radius = arc->getRadius();

    const double startangle = RS_Math::rad2deg(arc->getAngle1());
    double endangle = RS_Math::rad2deg(arc->getAngle2());

    if (endangle <= startangle) {
        endangle += 360;
    }

    bool large_arc_flag = endangle - startangle > 180;
    bool sweep_flag = false;

    if (arc->isReversed()) {
        large_arc_flag = !large_arc_flag;
        sweep_flag = !sweep_flag;
    }

    return svgPathArc(endpoint, radius, radius, 0.0, large_arc_flag, sweep_flag);
}

std::string LC_MakerCamSVG::svgPathArc(const RS_Vector& point, const double radiusX, const double radiusY, const double xAxisRotation, const bool largeArcFlag, const bool sweepFlag) const {
    return "A" + lengthXml(radiusX) + "," + lengthXml(radiusY) + " " + numXml(xAxisRotation) + " " + (largeArcFlag ? "1" : "0") + ","
        + (sweepFlag ? "1" : "0") + " " + lengthXml(point.x) + "," + lengthXml(point.y) + " ";
}

RS_Vector LC_MakerCamSVG::calcEllipsePointDerivative(const double majorRadius, const double minorRadius, const double xAxisRotation, const double angle) const {
    const RS_Vector vector(-majorRadius * cos(xAxisRotation) * sin(angle) - minorRadius * sin(xAxisRotation) * cos(angle),
                     -majorRadius * sin(xAxisRotation) * sin(angle) + minorRadius * cos(xAxisRotation) * cos(angle));

    return vector;
}

double LC_MakerCamSVG::calcAlpha(const double angle) {
    return sin(angle) * ((sqrt(4.0 + 3.0 * pow(tan(angle / 2), 2.0)) - 1.0) / 3.0);
}

void LC_MakerCamSVG::writeImage(const RS_Image* image) const {
    RS_DEBUG->print("RS_MakerCamSVG::writeImage: Writing image ...");
    if (m_exportImages) {
        const RS_Vector insertionPoint = convertToSvg(image->getInsertionPoint());

        m_xmlWriter->addElement("image", NAMESPACE_URI_SVG);
        m_xmlWriter->addAttribute("x", lengthXml(insertionPoint.x));
        m_xmlWriter->addAttribute("y", lengthXml(insertionPoint.y - image->getImageHeight()));
        m_xmlWriter->addAttribute("height", lengthXml(image->getImageHeight()));
        m_xmlWriter->addAttribute("width", lengthXml(image->getImageWidth()));
        m_xmlWriter->addAttribute("preserveAspectRatio", "none"); //height and width above used
        m_xmlWriter->addAttribute("href", image->getData().file.toStdString(), NAMESPACE_URI_XLINK);
        m_xmlWriter->closeElement();
    }
}

std::string LC_MakerCamSVG::svgPathAnyLineType(RS_Vector startpoint, RS_Vector endpoint, RS2::LineType type) const {
    RS_DEBUG->print("RS_MakerCamSVG::svgPathUniLineType: convert line to dot/dash path");

    constexpr int dotFactor = 1; // ..........
    constexpr int dashFactor = 3; // -- -- -- --
    constexpr int dashDotFactor = 4; // --. --. --.
    constexpr int divideFactor = 5; // --.. --.. --.. --..
    constexpr int centerFactor = 5; // -- - -- - -- -
    constexpr int borderFactor = 7; // -- -- . -- -- . -- -- .

    constexpr double lineScaleTiny = 0.25;
    constexpr double lineScale2 = 0.5;
    constexpr double lineScaleOne = 1.0;
    constexpr double lineScaleX2 = 2.0;

    std::string path;
    double lineScale;
    double lineFactor;

    double lineLengh = startpoint.distanceTo(endpoint);
    switch (type) {
        case RS2::DotLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = dotFactor;
            break;
        }
        case RS2::DotLine2: {
            lineScale = lineScale2;
            lineFactor = dotFactor;
            break;
        }
        case RS2::DotLine: {
            lineScale = lineScaleOne;
            lineFactor = dotFactor;
            break;
        }
        case RS2::DotLineX2: {
            lineScale = lineScaleX2;
            lineFactor = dotFactor;
            break;
        }

        case RS2::DashLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = dashFactor;
            break;
        }
        case RS2::DashLine2: {
            lineScale = lineScale2;
            lineFactor = dashFactor;
            break;
        }
        case RS2::DashLine: {
            lineScale = lineScaleOne;
            lineFactor = dashFactor;
            break;
        }
        case RS2::DashLineX2: {
            lineScale = lineScaleX2;
            lineFactor = dashFactor;
            break;
        }

        case RS2::DashDotLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = dashDotFactor;
            break;
        }
        case RS2::DashDotLine2: {
            lineScale = lineScale2;
            lineFactor = dashDotFactor;
            break;
        }
        case RS2::DashDotLine: {
            lineScale = lineScaleOne;
            lineFactor = dashDotFactor;
            break;
        }
        case RS2::DashDotLineX2: {
            lineScale = lineScaleX2;
            lineFactor = dashDotFactor;
            break;
        }

        case RS2::DivideLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = divideFactor;
            break;
        }
        case RS2::DivideLine2: {
            lineScale = lineScale2;
            lineFactor = divideFactor;
            break;
        }
        case RS2::DivideLine: {
            lineScale = lineScaleOne;
            lineFactor = divideFactor;
            break;
        }
        case RS2::DivideLineX2: {
            lineScale = lineScaleX2;
            lineFactor = divideFactor;
            break;
        }

        case RS2::CenterLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = centerFactor;
            break;
        }
        case RS2::CenterLine2: {
            lineScale = lineScale2;
            lineFactor = centerFactor;
            break;
        }
        case RS2::CenterLine: {
            lineScale = lineScaleOne;
            lineFactor = centerFactor;
            break;
        }
        case RS2::CenterLineX2: {
            lineScale = lineScaleX2;
            lineFactor = centerFactor;
            break;
        }

        case RS2::BorderLineTiny: {
            lineScale = lineScaleTiny;
            lineFactor = borderFactor;
            break;
        }
        case RS2::BorderLine2: {
            lineScale = lineScale2;
            lineFactor = borderFactor;
            break;
        }
        case RS2::BorderLine: {
            lineScale = lineScaleOne;
            lineFactor = borderFactor;
            break;
        }
        case RS2::BorderLineX2: {
            lineScale = lineScaleX2;
            lineFactor = borderFactor;
            break;
        }
        default: {
            lineScale = lineScaleOne;
            lineFactor = dotFactor;
            break;
        }
    }

    //doesn't make an sense to have pattern longer than a line
    double dashLinePatternLength = m_defaultDashLinePatternLength;

    if (lineLengh < dashLinePatternLength * lineFactor * lineScale) {
        dashLinePatternLength = lineLengh / (lineFactor * lineScale);
        RS_DEBUG->print(RS_Debug::D_WARNING, "Line length shorter than a line pattern, updated length is %f mm", dashLinePatternLength);
    }

    double lineStep = lineScale * dashLinePatternLength * lineFactor;
    int numOfIter = round(lineLengh / lineStep);

    RS_Vector step((endpoint.x - startpoint.x) / numOfIter, (endpoint.y - startpoint.y) / numOfIter);
    RS_Vector lastPos(startpoint.x, startpoint.y);

    for (int i = 0; i < numOfIter; i++) {
        path += getLinePattern(&lastPos, step, type, 1.0 / lineFactor);
    }

    return path;
}

std::string LC_MakerCamSVG::getLinePattern(RS_Vector* lastPos, RS_Vector step, RS2::LineType type, double lineScale) const {
    std::string path;

    switch (type) {
        case RS2::DotLineTiny:
        case RS2::DotLine2:
        case RS2::DotLine:
        case RS2::DotLineX2: {
            path += getPointSegment(lastPos, step, lineScale);
            break;
        }

        case RS2::DashLineTiny:
        case RS2::DashLine2:
        case RS2::DashLine:
        case RS2::DashLineX2: {
            path += getLineSegment(lastPos, step, lineScale, true);
            break;
        }

        case RS2::DashDotLineTiny:
        case RS2::DashDotLine2:
        case RS2::DashDotLine:
        case RS2::DashDotLineX2: {
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
        case RS2::CenterLineX2: {
            path += getLineSegment(lastPos, step, lineScale, true);
            path += getLineSegment(lastPos, step, lineScale, false);
            break;
        }

        case RS2::BorderLineTiny:
        case RS2::BorderLine2:
        case RS2::BorderLine:
        case RS2::BorderLineX2: {
            path += getLineSegment(lastPos, step, lineScale, true);
            path += getLineSegment(lastPos, step, lineScale, true);
            path += getPointSegment(lastPos, step, lineScale);
            break;
        }

        default: {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_MakerCamSVG::getLinePattern: unsupported line type %d\n", type);
            path += svgPathMoveTo(convertToSvg(*lastPos));
            *lastPos += step * lineScale;
            path += svgPathLineTo(convertToSvg(*lastPos));
            break;
        }
    }
    return path;
}

std::string LC_MakerCamSVG::getPointSegment(RS_Vector* lastPos, const RS_Vector& step, const double lineScale) const {
    std::string path;
    //0.2 - is a diametr of point on early implementation of MakerCAM.
    //! \todo need to add a option to control this value from export dialog and test on laser engraver
    constexpr double dotSize = 0.2;
    double scaleTo;

    if (fabs(step.x) >= fabs(step.y)) {
        scaleTo = dotSize / fabs(step.x);
    }
    else {
        scaleTo = dotSize / fabs(step.y);
    }
    path += svgPathMoveTo(*lastPos);
    path += svgPathLineTo(*lastPos + step * scaleTo);
    *lastPos += step * lineScale;
    return path;
}

std::string LC_MakerCamSVG::getLineSegment(RS_Vector* lastPos, const RS_Vector& step, const double lineScale, const bool x2) const {
    std::string path;
    path += svgPathMoveTo(*lastPos);
    if (x2) {
        *lastPos += step * lineScale * 2;
    }
    else {
        *lastPos += step * lineScale;
    }
    path += svgPathLineTo(*lastPos);
    *lastPos += step * lineScale;
    return path;
}
