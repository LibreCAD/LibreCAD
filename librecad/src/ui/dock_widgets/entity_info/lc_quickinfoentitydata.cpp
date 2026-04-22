/****************************************************************************
 *
* class that process given entity and creates the list of properties of it

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
**********************************************************************/

#include "lc_quickinfoentitydata.h"

#include "lc_containertraverser.h"
#include "lc_dimarc.h"
#include "lc_dimordinate.h"
#include "lc_hyperbola.h"
#include "lc_parabola.h"
#include "lc_peninforegistry.h"
#include "lc_quickinfowidgetoptions.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_spline.h"
#include "rs_text.h"
#include "rs_units.h"

LC_QuickInfoEntityData::LC_QuickInfoEntityData() : m_penRegistry{LC_PenInfoRegistry::instance()} {
}

LC_QuickInfoEntityData::~LC_QuickInfoEntityData() {
    LC_QuickInfoEntityData::clear();
}

// FIXME - sand - check add length property where its reasonable! (polyline, splines, etc)
// FIXME - sand - check start/end points where its reasonable! (polyline, splines, etc)

/**
 * Returns true if there is entity data, false otherwise
 * @return
 */
bool LC_QuickInfoEntityData::hasData() const {
    return m_entityId > 0;
}

/**
 * Detects type of given entity and returns string that describes that entity.
 * @param e entity
 * @param entityTypeName
 * @param level
 * @return true if entity description is needed (for info cursor) or more detailed (for info actions).
 */

// collecting some generic common properties of all entities
QString LC_QuickInfoEntityData::prepareGenericEntityDescription(const RS_Entity* e, const QString& entityTypeName,
                                                                const RS2::EntityDescriptionLevel level) {
    QString result;
    switch (level) {
        case RS2::EntityDescriptionLevel::DescriptionCreating: {
            result = tr("To be created: ").append(entityTypeName);
            return result;
        }
        case RS2::EntityDescriptionLevel::DescriptionCatched: {
            result = tr("Captured: ").append(entityTypeName);
            break;
        }
        default:
            result = entityTypeName;
            break;
    }

    const unsigned long id = e->getId();
    m_entityIdForDescription = id;
    if (m_options->displayEntityID) {
        QString idStr;
        idStr.setNum(id);
        result.append(" ").append(tr("ID"));
    }

    const RS_Layer* layer = e->getLayer(true);
    QString layerName = "";
    if (layer != nullptr) {
        layerName = layer->getName();
    }

    result.append(tr("\nLayer: "));
    result.append(layerName);

    return result;
}

QString LC_QuickInfoEntityData::getEntityDescription(const RS_Entity* en, const RS2::EntityDescriptionLevel level) {
    const unsigned long thisEntityId = en->getId();
    // no special value for empty id, yet according to implementation, it seems that 0 should not be used
    if (thisEntityId != 0 && thisEntityId == m_entityIdForDescription) {
        // same entity... so we'll try to optimize a bit mouse move there.
        return m_cachedEntityDescription;
    }
    m_cachedEntityDescription.clear();
    m_entityIdForDescription = thisEntityId;

    // just processing of entity based on type
    switch (en->rtti()) {
        case RS2::EntityLine: {
            auto* line = static_cast<const RS_Line*>(en);
            m_cachedEntityDescription = prepareLineDescription(line, level);
            break;
        }
        case RS2::EntityCircle: {
            auto* circle = static_cast<const RS_Circle*>(en);
            m_cachedEntityDescription = prepareCircleDescription(circle, level);
            break;
        }
        case RS2::EntityArc: {
            auto* arc = static_cast<const RS_Arc*>(en);
            m_cachedEntityDescription = prepareArcDescription(arc, level);
            break;
        }
        case RS2::EntityEllipse: {
            auto* ellipse = static_cast<const RS_Ellipse*>(en);
            m_cachedEntityDescription = prepareEllipseDescription(ellipse, level);
            break;
        }
        case RS2::EntityHyperbola: {
            auto* hyperbola = static_cast<const LC_Hyperbola*>(en);
            m_cachedEntityDescription = prepareHyperbolaDescription(hyperbola, level);
            break;
        }
        case RS2::EntityPoint: {
            auto* point = static_cast<const RS_Point*>(en);
            m_cachedEntityDescription = preparePointDescription(point, level);
            break;
        }
        case RS2::EntityPolyline: {
            auto* pline = static_cast<const RS_Polyline*>(en);
            m_cachedEntityDescription = preparePolylineDescription(pline, level);
            break;
        }
        case RS2::EntityInsert: {
            auto* pinsert = static_cast<const RS_Insert*>(en);
            m_cachedEntityDescription = prepareInsertDescription(pinsert, level);
            break;
        }
        case RS2::EntityMText: {
            auto* pmtext = static_cast<const RS_MText*>(en);
            m_cachedEntityDescription = prepareMTextDescription(pmtext, level);
            break;
        }
        case RS2::EntityText: {
            auto* ptext = static_cast<const RS_Text*>(en);
            m_cachedEntityDescription = prepareTextDescription(ptext, level);
            break;
        }
        case RS2::EntityDimAligned: {
            auto* dim = static_cast<const RS_DimAligned*>(en);
            m_cachedEntityDescription = prepareDimAlignedDescription(dim, level);
            break;
        }
        case RS2::EntityDimLinear: {
            auto* dim = static_cast<const RS_DimLinear*>(en);
            m_cachedEntityDescription = prepareDimLinearDescription(dim, level);
            break;
        }
        case RS2::EntityDimOrdinate: {
            auto* dim = reinterpret_cast<const LC_DimOrdinate*>(en);
            m_cachedEntityDescription = prepareDimOrdinateDescription(dim, level);
            break;
        }
        case RS2::EntityDimRadial: {
            auto* dimrad = static_cast<const RS_DimRadial*>(en);
            m_cachedEntityDescription = prepareDimRadialDescription(dimrad, level);
            break;
        }
        case RS2::EntityDimDiametric: {
            auto* dimdia = static_cast<const RS_DimDiametric*>(en);
            m_cachedEntityDescription = prepareDimDiametricDescription(dimdia, level);
            break;
        }
        case RS2::EntityDimAngular: {
            auto* dimang = static_cast<const RS_DimAngular*>(en);
            m_cachedEntityDescription = prepareDimAngularDescription(dimang, level);
            break;
        }
        case RS2::EntityDimArc: {
            auto* dimarc = static_cast<const LC_DimArc*>(en);
            m_cachedEntityDescription = prepareDimArcDescription(dimarc, level);
            break;
        }
        case RS2::EntityDimLeader: {
            auto* leader = static_cast<const RS_Leader*>(en);
            m_cachedEntityDescription = prepareDimLeaderDescription(leader, level);
            break;
        }
        case RS2::EntityHatch: {
            auto* hatch = static_cast<const RS_Hatch*>(en);
            m_cachedEntityDescription = prepareHatchDescription(hatch, level);
            break;
        }
        case RS2::EntityImage: {
            auto* pimage = static_cast<const RS_Image*>(en);
            m_cachedEntityDescription = prepareImageDescription(pimage, level);
            break;
        }
        case RS2::EntitySpline: {
            auto* pspline = static_cast<const RS_Spline*>(en);
            m_cachedEntityDescription = prepareSplineDescription(pspline, level);
            break;
        }
        case RS2::EntitySplinePoints: {
            auto* psplinepoints = static_cast<const LC_SplinePoints*>(en);
            m_cachedEntityDescription = prepareSplinePointsDescription(psplinepoints, level);
            break;
        }
        case RS2::EntityParabola: {
            auto* parabola = static_cast<const LC_Parabola*>(en);
            m_cachedEntityDescription = prepareParabolaDescription(parabola, level);
            break;
        }
        case RS2::EntitySolid:
            m_cachedEntityDescription = prepareGenericEntityDescription(en, tr("SOLID"), level);
            break;
        case RS2::EntityConstructionLine:
            m_cachedEntityDescription = prepareGenericEntityDescription(en, tr("CONSTRUCTION"), level);
            break;
        default:
            m_entityName = tr("UNKNOWN");
            m_cachedEntityDescription = prepareGenericEntityDescription(en, m_entityName, level);
            break;
    }
    return m_cachedEntityDescription;
}

/**
 * Detects type of given entity and performs preparation of properties for it
 * @param en entity
 * @return true it view should be updated, false otherwise
 */
bool LC_QuickInfoEntityData::processEntity(const RS_Entity* en) {
    // no special value for empty id, yet according to implementation, it seems that 0 should not be used
    if (m_entityId != 0 && en->getId() == m_entityId) {
        // same entity... so we'll try to optimize a bit mouse move there.
        return false;
    }
    clear();

    // collecting some generic common properties of all entities
    collectGenericProperties(en);

    // just processing of entity based on type
    switch (en->rtti()) {
        case RS2::EntityLine: {
            auto* line = static_cast<const RS_Line*>(en);
            collectLineProperties(line);
            break;
        }
        case RS2::EntityCircle: {
            auto* circle = static_cast<const RS_Circle*>(en);
            collectCircleProperties(circle);
            break;
        }
        case RS2::EntityArc: {
            auto* arc = static_cast<const RS_Arc*>(en);
            collectArcProperties(arc);
            break;
        }
        case RS2::EntityEllipse: {
            auto* ellipse = static_cast<const RS_Ellipse*>(en);
            collectEllipseProperties(ellipse);
            break;
        }
        case RS2::EntityHyperbola: {
            auto* hyperbola = static_cast<const LC_Hyperbola*>(en);
            collectHyperbolaProperties(hyperbola);
            break;
        }
        case RS2::EntityPoint: {
            auto* point = static_cast<const RS_Point*>(en);
            collectPointProperties(point);
            break;
        }
        case RS2::EntityPolyline: {
            auto* pline = static_cast<const RS_Polyline*>(en);
            collectPolylineProperties(pline);
            break;
        }
        case RS2::EntityInsert: {
            auto* pinsert = static_cast<const RS_Insert*>(en);
            collectInsertProperties(pinsert);
            break;
        }
        case RS2::EntityMText: {
            /**< Multi-line Text */
            auto* pmtext = static_cast<const RS_MText*>(en);
            collectMTextProperties(pmtext);
            break;
        }
        case RS2::EntityText: {
            /**< Single-line Text */
            auto* ptext = static_cast<const RS_Text*>(en);
            collectTextProperties(ptext);
            break;
        }
        case RS2::EntityDimAligned: {
            auto* dim = static_cast<const RS_DimAligned*>(en);
            collectDimAlignedProperties(dim);
            break;
        }
        case RS2::EntityDimLinear: {
            auto* dim = static_cast<const RS_DimLinear*>(en);
            collectDimLinearProperties(dim);
            break;
        }
        case RS2::EntityDimOrdinate: {
            auto* dim = static_cast<const LC_DimOrdinate*>(en);
            collectDimOrdinateProperties(dim);
            break;
        }
        case RS2::EntityDimRadial: {
            auto* dimrad = static_cast<const RS_DimRadial*>(en);
            collectDimRadialProperties(dimrad);
            break;
        }
        case RS2::EntityDimDiametric: {
            auto* dimdia = static_cast<const RS_DimDiametric*>(en);
            collectDimDiametricProperties(dimdia);
            break;
        }
        case RS2::EntityDimAngular: {
            auto* dimang = static_cast<const RS_DimAngular*>(en);
            collectDimAngularProperties(dimang);
            break;
        }
        case RS2::EntityDimArc: {
            auto* dimarc = static_cast<const LC_DimArc*>(en);
            collectDimArcProperties(dimarc);
            break;
        }
        case RS2::EntityDimLeader: {
            auto* leader = static_cast<const RS_Leader*>(en);
            collectDimLeaderProperties(leader);
            break;
        }
        case RS2::EntityHatch: {
            auto* hatch = static_cast<const RS_Hatch*>(en);
            collectHatchProperties(hatch);
            break;
        }
        case RS2::EntityImage: {
            auto* pimage = static_cast<const RS_Image*>(en);
            collectImageProperties(pimage);
            break;
        }
        case RS2::EntitySpline: {
            auto* pspline = static_cast<const RS_Spline*>(en);
            collectSplineProperties(pspline);
            break;
        }
        case RS2::EntitySplinePoints: {
            auto* psplinepoints = static_cast<const LC_SplinePoints*>(en);
            collectSplinePointsProperties(psplinepoints);
            break;
        }
        case RS2::EntityParabola: {
            auto* parabola = static_cast<const LC_Parabola*>(en);
            collectParabolaProperties(parabola);
            break;
        }
        case RS2::EntitySolid:
            m_entityName = tr("SOLID");
            break;
        case RS2::EntityConstructionLine:
            m_entityName = tr("CONSTRUCTION");
            break;
        default:
            m_entityName = tr("UNKNOWN");
            break;
    }
    return true;
}

/**
 * Generates HTML table which includes collected properties of current entity. That html will be shown by the widget.
 * Table has 2 columns (label, value)
 * HTML links are used for coordinates and values, so they may be processed by the widget.
 *
 * @return html with entity properties.
 */
QString LC_QuickInfoEntityData::generateView() {
    const int propertiesCount = m_properties.size();
    QString data = "<body><table>";
    data.append("<tr><td colspan = '2'><b>").append(m_entityName).append("</b></td></tr>");
    for (int i = 0; i < propertiesCount; i++) {
        data.append("<tr>");
        const PropertyInfo* property = m_properties.at(i);
        data.append("<td>");

        if (property->type == PropertyType::PROPERTY_TYPE_VECTOR) {
            // support of setting relative zero
            createLink(data, "zero", i, tr("Set Relative Zero"), property->label);
            data.append(": ");
        }
        else {
            data.append(property->label).append(": ");
        }
        data.append("</td>");
        data.append("<td>");

        if (property->type == PropertyType::PROPERTY_TYPE_VECTOR) {
            createLink(data, "coord", i, tr("To Cmd"), property->value);
        }
        else if (property->type == PropertyType::PROPERTY_TYPE_LINEAR || property->type == PropertyType::PROPERTY_TYPE_ANGLE) {
            createLink(data, "val", i, tr("To Cmd"), property->value);
        }
        else {
            data.append("<b>");
            data.append(property->value);
            data.append("</b>");
        }
        data.append("</td>");
        data.append("</tr>");
    }
    data.append("</table></body>");
    return data;
}

/**
 * Recalculate view coordinates according to specified mode (absolute or relative to relative zero)
 * @param mode expected mode
 * @return true if mode changed and view should be updated
 */
bool LC_QuickInfoEntityData::updateForCoordinateViewMode(const int mode) {
    if (mode != m_coordinatesMode) {
        m_coordinatesMode = mode;
        const int propertiesCount = m_properties.size();
        const RS_Vector relativeZero = getRelativeZero();

        for (int i = 0; i < propertiesCount; i++) {
            PropertyInfo* propertyInfo = m_properties.at(i);
            if (propertyInfo->type == PROPERTY_TYPE_VECTOR) {
                const auto* vectorProperty = static_cast<VectorPropertyInfo*>(propertyInfo);
                RS_Vector data = vectorProperty->data;
                RS_Vector viewVector;
                QString vectorStr;
                if (mode == COORD_RELATIVE && relativeZero.valid) {
                    viewVector = data - relativeZero;
                    vectorStr = formatWCSDeltaVector(viewVector);
                }
                else {
                    viewVector = data;
                    vectorStr = formatWCSVector(viewVector);
                }
                propertyInfo->value = vectorStr;
            }
        }
        return true;
    }
    return false;
}

/**
 * Collection of generic properties that are common for all entities
 * @param e entity
 */
void LC_QuickInfoEntityData::collectGenericProperties(const RS_Entity* e) {
    RS_Pen pen = e->getPen(false);
    RS_Pen resolvedPen = e->getPen(true);
    RS_Layer* layer = e->getLayer(true);
    QString layerName = "";
    if (layer != nullptr) {
        layerName = layer->getName();
    }

    // visual attributes
    RS_Color color = pen.getColor();
    RS_Color resolvedColor = resolvedPen.getColor();
    RS2::LineType lineType = pen.getLineType();
    RS2::LineType resolvedLineType = resolvedPen.getLineType();
    RS2::LineWidth lineWidth = pen.getWidth();
    RS2::LineWidth resolvedLineWidth = resolvedPen.getWidth();
    LC_PenInfoRegistry::ColorNameDisplayMode colorType = LC_PenInfoRegistry::NATURAL;
    QString colorName = m_penRegistry->getColorName(color, colorType);
    QString lineTypeName = m_penRegistry->getLineTypeText(lineType);
    QString lineWidthName = m_penRegistry->getLineWidthText(lineWidth);

    unsigned long id = e->getId();
    m_entityId = id;
    if (m_options->displayEntityID) {
        QString idStr;
        idStr.setNum(id);
        addProperty(tr("ID"), idStr, PROPERTY_TYPE_OTHER);
    }
    addProperty(tr("Layer"), layerName, PROPERTY_TYPE_OTHER);
    if (resolvedColor != color) {
        QString actualColorName = m_penRegistry->getColorName(resolvedColor, colorType);
        colorName = colorName.append(" / ").append(actualColorName);
    }
    addProperty(tr("Color"), colorName, PROPERTY_TYPE_OTHER);
    if (resolvedLineType != lineType) {
        QString resolvedLineTypeName = m_penRegistry->getLineTypeText(resolvedLineType);
        lineTypeName.append(" / ").append(resolvedLineTypeName);
    }
    addProperty(tr("Line Type"), lineTypeName, PROPERTY_TYPE_OTHER);
    if (resolvedLineType != lineType) {
        QString resolvedLineWidthName = m_penRegistry->getLineWidthText(resolvedLineWidth);
        lineWidthName.append(" / ").append(resolvedLineWidthName);
    }
    addProperty(tr("Line Width"), lineWidthName, PROPERTY_TYPE_OTHER);
    if (m_options->displayEntityBoundaries) {
        addVectorProperty(tr("Min"), e->getMin());
        addVectorProperty(tr("Max"), e->getMax());
    }
}

/**
 * Properties for line
 * @param line
 */
void LC_QuickInfoEntityData::collectLineProperties(const RS_Line* line) {
    m_entityName = tr("LINE");
    const RS_Vector& start = line->getStartpoint();
    const RS_Vector& end = line->getEndpoint();
    const double angle = line->getAngle1();
    const double angle2 = line->getAngle2();
    const double length = line->getLength();
    const RS_Vector delta = end - start;

    addVectorProperty(tr("From"), start);
    addVectorProperty(tr("To"), end);
    addVectorProperty(tr("Middle"), line->getMiddlePoint());
    addDeltaVectorProperty(tr("Delta"), delta, PROPERTY_TYPE_OTHER);
    addAngleProperty(tr("Angle"), angle);
    addAngleProperty(tr("Angle 2"), angle2);
    addLinearProperty(tr("Length"), length);
}

QString LC_QuickInfoEntityData::prepareLineDescription(const RS_Line* line, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(line, tr("LINE"), level);

    const RS_Vector& start = line->getStartpoint();
    const RS_Vector& end = line->getEndpoint();
    const double angle = line->getAngle1();
    const double angle2 = line->getAngle2();
    const double length = line->getLength();

    appendLinear(result, tr("Length"), length);
    appendWCSAngle(result, tr("Angle"), angle);
    appendWCSAngle(result, tr("Angle2"), angle2);
    appendWCSAbsolute(result, tr("From"), start);
    appendWCSAbsolute(result, tr("To"), end);
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendWCSAbsolute(result, tr("Middle"), line->getMiddlePoint());
        const RS_Vector delta = end - start;
        appendWCSAbsoluteDelta(result, tr("Delta"), delta);
    }
    return result;
}

/**
 * Properties for circle
 * @param circle
 */
void LC_QuickInfoEntityData::collectCircleProperties(const RS_Circle* circle) {
    m_entityName = tr("CIRCLE");
    const RS_Vector center = circle->getCenter();
    const double radius = circle->getRadius();
    const double circumference = circle->getLength();
    const double area = circle->areaLineIntegral();
    const double diameter = radius * 2;

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Radius"), radius);
    addLinearProperty(tr("Diameter"), diameter);
    addLinearProperty(tr("Circumference", "circle"), circumference, PROPERTY_TYPE_OTHER);
    addAreaProperty(tr("Area"), area);
}

QString LC_QuickInfoEntityData::prepareCircleDescription(const RS_Circle* circle, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(circle, tr("CIRCLE"), level);

    const RS_Vector center = circle->getCenter();
    const double radius = circle->getRadius();

    appendWCSAbsolute(result, tr("Center"), center);
    appendLinear(result, tr("Radius"), radius);
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        const double circumference = circle->getLength();
        const double area = circle->areaLineIntegral();
        const double diameter = radius * 2;

        appendLinear(result, tr("Diameter"), diameter);
        appendLinear(result, tr("Circumference", "circle"), circumference);
        appendArea(result, tr("Area"), area);
    }
    return result;
}

/**
 * properties for arc
 * @param arc
 */
void LC_QuickInfoEntityData::collectArcProperties(const RS_Arc* arc) {
    m_entityName = tr("ARC");
    const RS_Vector center = arc->getCenter();
    const double radius = arc->getRadius();
    const double circumference = arc->getLength();
    const double diameter = radius * 2;
    const double angleLength = arc->getAngleLength();
    const double startAngle = arc->getAngle1();
    const double endAngle = arc->getAngle2();
    const RS_Vector startPoint = arc->getStartpoint();
    const RS_Vector endPoint = arc->getEndpoint();
    const double chordLength = startPoint.distanceTo(endPoint);

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Radius"), radius);
    addLinearProperty(tr("Diameter"), diameter);
    addLinearProperty(tr("Circumference", "arc"), circumference, PROPERTY_TYPE_OTHER);
    addLinearProperty(tr("Chord Length"), chordLength, PROPERTY_TYPE_OTHER);
    addRawAngleProperty(tr("Angle Length"), angleLength);
    addVectorProperty(tr("Start"), startPoint);
    addAngleProperty(tr("Start Angle"), startAngle);
    addVectorProperty(tr("End"), endPoint);
    addAngleProperty(tr("End Angle"), endAngle);
    addLinearProperty(tr("Bulge"), arc->getBulge(), PROPERTY_TYPE_OTHER);
    addProperty(tr("Reversed"), arc->isReversed() ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
}

QString LC_QuickInfoEntityData::prepareArcDescription(const RS_Arc* arc, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(arc, tr("ARC"), level);
    const RS_Vector center = arc->getCenter();
    const double radius = arc->getRadius();
    const double angleLength = arc->getAngleLength();
    const RS_Vector startPoint = arc->getStartpoint();
    const RS_Vector endPoint = arc->getEndpoint();

    appendWCSAbsolute(result, tr("Center"), center);
    appendLinear(result, tr("Radius"), radius);
    appendWCSAbsolute(result, tr("Start"), startPoint);
    appendWCSAbsolute(result, tr("End"), endPoint);
    appendRawAngle(result, tr("Angle Length"), angleLength);
    appendValue(result, tr("Reversed"), arc->isReversed() ? tr("Yes") : tr("No"));

    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        const double circumference = arc->getLength();
        const double diameter = radius * 2;
        const double startAngle = arc->getAngle1();
        const double endAngle = arc->getAngle2();
        const double chordLength = startPoint.distanceTo(endPoint);
        const double sagitta = arc->getSagitta();

        appendLinear(result, tr("Diameter"), diameter);
        appendLinear(result, tr("Circumference", "arc"), circumference);
        appendLinear(result, tr("Chord Length"), chordLength);
        appendWCSAngle(result, tr("Start Angle"), startAngle);
        appendWCSAngle(result, tr("End Angle"), endAngle);
        appendLinear(result, tr("Sagitta"), sagitta);
        appendLinear(result, tr("Bulge"), arc->getBulge());
    }

    return result;
}

/**
 * properties for ellipse
 * @param ellipse
 */
void LC_QuickInfoEntityData::collectEllipseProperties(const RS_Ellipse* ellipse) {
    const RS_Vector center = ellipse->getCenter();
    const double minorRadius = ellipse->getMinorRadius();
    const double majorRadius = ellipse->getMajorRadius();
    const double ratio = ellipse->getRatio();
    const double circumference = ellipse->getLength();
    const double angle = ellipse->getAngle();

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Minor Radius"), minorRadius);
    addLinearProperty(tr("Major Radius"), majorRadius);
    addProperty(tr("Ratio"), formatDouble(ratio), PROPERTY_TYPE_OTHER);
    addAngleProperty(tr("Angle"), angle);
    addLinearProperty(tr("Circumference", "ellipse"), circumference, PROPERTY_TYPE_OTHER);

    if (ellipse->isEllipticArc()) {
        m_entityName = tr("ELLIPSE ARC");

        const double angleLength = ellipse->getAngleLength();
        const double startAngle = ellipse->getAngle1();
        const double endAngle = ellipse->getAngle2();
        const RS_Vector startPoint = ellipse->getStartpoint();
        const RS_Vector endPoint = ellipse->getEndpoint();

        addRawAngleProperty(tr("Angle Length"), angleLength);
        addVectorProperty(tr("Start"), startPoint);
        addAngleProperty(tr("Start Angle"), startAngle); // fixme - sand - or raw angle value should be there? Check!
        addVectorProperty(tr("End"), endPoint);
        addAngleProperty(tr("End Angle"), endAngle); // fixme - sand - or raw angle value should be there? Check!
    }
    else {
        const double area = ellipse->areaLineIntegral();
        addAreaProperty(tr("Area"), area);
        m_entityName = tr("ELLIPSE");
    }
}

QString LC_QuickInfoEntityData::prepareEllipseDescription(const RS_Ellipse* ellipse, const RS2::EntityDescriptionLevel level) {
    const bool ellipticArc = ellipse->isEllipticArc();
    const QString entityName = ellipticArc ? tr("ELLIPSE ARC") : tr("ELLIPSE");

    QString result = prepareGenericEntityDescription(ellipse, entityName, level);

    const RS_Vector center = ellipse->getCenter();
    const double minorRadius = ellipse->getMinorRadius();
    const double majorRadius = ellipse->getMajorRadius();
    const double angle = ellipse->getAngle();

    appendWCSAbsolute(result, tr("Center"), center);
    appendLinear(result, tr("Minor Radius"), minorRadius);
    appendLinear(result, tr("Major Radius"), majorRadius);
    appendWCSAngle(result, tr("Angle"), angle);
    if (ellipticArc) {
        const double angleLength = ellipse->getAngleLength();
        appendRawAngle(result, tr("Angle Length"), angleLength);
    }

    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        const double ratio = ellipse->getRatio();
        const double circumference = ellipse->getLength();

        appendDouble(result, tr("Ratio"), ratio);
        appendLinear(result, tr("Circumference", "ellipse"), circumference);
        if (ellipticArc) {
            const double startAngle = ellipse->getAngle1();
            const double endAngle = ellipse->getAngle2();
            const RS_Vector startPoint = ellipse->getStartpoint();
            const RS_Vector endPoint = ellipse->getEndpoint();
            appendWCSAbsolute(result, tr("Start"), startPoint);
            appendWCSAngle(result, tr("Start Angle"), startAngle); // fixme - sand - or raw angle value should be there? Check!
            appendWCSAbsolute(result, tr("End"), endPoint);
            appendWCSAngle(result, tr("End Angle"), endAngle); // fixme - sand - or raw angle value should be there? Check!
        }
        else {
            const double area = ellipse->areaLineIntegral();
            appendArea(result, tr("Area"), area);
        }
    }
    return result;
}

void LC_QuickInfoEntityData::collectHyperbolaProperties(const LC_Hyperbola* hyperbola) {
    m_entityName = tr("HYPERBOLA");
    const auto& startPoint = hyperbola->getStartpoint();
    const auto& endPoint = hyperbola->getStartpoint();
    const double area = hyperbola->areaLineIntegral();
    const auto& focus1 = hyperbola->getFocus1();
    const auto& focus2 = hyperbola->getFocus2();
    const auto& center = hyperbola->getCenter();
    const auto& eccentricity = hyperbola->getEccentricity();
    const auto& length = hyperbola->getLength();
    const auto& majorR = hyperbola->getMajorRadius();
    const auto& minorR = hyperbola->getMinorRadius();
    const auto& ratio = hyperbola->getRatio();
    addVectorProperty(tr("Start"), startPoint);
    addVectorProperty(tr("End"), endPoint);
    addVectorProperty(tr("Focus 1"), focus1);
    addVectorProperty(tr("Focus 2"), focus2);
    addLinearProperty(tr("Major Radius"), majorR);
    addLinearProperty(tr("Minor Radius"), minorR);
    addAngleProperty(tr("Angle"), hyperbola->getAngle());
    addProperty(tr("Reversed"), hyperbola->isReversed() ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
    addLinearProperty(tr("Ratio"), ratio);
    addVectorProperty(tr("Center"), center);
    addDoubleProperty(tr("Eccentricity"), formatDouble(eccentricity), eccentricity, PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Middle"), hyperbola->getMiddlePoint());
    addAngleProperty(tr("Start Angle"), hyperbola->getAngle1());
    addAngleProperty(tr("End Angle"), hyperbola->getAngle2());
    addLinearProperty(tr("Circumference", "hyperbola"), length);
    addAreaProperty(tr("Total Area"), area);
}

QString LC_QuickInfoEntityData::prepareHyperbolaDescription(const LC_Hyperbola* hyperbola, RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(hyperbola, tr("HYPERBOLA"), level);
    appendWCSAbsolute(result, tr("Start"), hyperbola->getStartpoint());
    appendWCSAbsolute(result, tr("End"), hyperbola->getEndpoint());
    appendWCSAbsolute(result, tr("Focus 1"), hyperbola->getFocus1());
    appendWCSAbsolute(result, tr("Focus 2"), hyperbola->getFocus2());
    appendLinear(result, tr("Major Radius"), hyperbola->getMajorRadius());
    appendLinear(result, tr("Minor Radius"), hyperbola->getMinorRadius());
    appendWCSAngle(result, tr("Angle"), hyperbola->getAngle());
    appendLinear(result, tr("Ratio"), hyperbola->getRatio());
    appendValue(result, tr("Reversed"), hyperbola->isReversed() ? tr("Yes") : tr("No"));
    appendWCSAbsolute(result, tr("Center"), hyperbola->getCenter());
    appendWCSAbsolute(result, tr("Middle"), hyperbola->getMiddlePoint());
    appendWCSAngle(result, tr("Start Angle"), hyperbola->getAngle1());
    appendWCSAngle(result, tr("End Angle"), hyperbola->getAngle2());
    appendLinear(result, tr("Circumference", "hyperbola"), hyperbola->getLength());
    appendArea(result, tr("Area"), hyperbola->areaLineIntegral());
    return result;
}

/**
 * Properties for point
 */
void LC_QuickInfoEntityData::collectPointProperties(const RS_Point* point) {
    m_entityName = tr("POINT");
    const RS_Vector center = point->getPos();
    addVectorProperty(tr("Position"), center);
}

QString LC_QuickInfoEntityData::preparePointDescription(const RS_Point* point, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(point, tr("POINT"), level);
    appendWCSAbsolute(result, tr("Position"), point->getPos());
    return result;
}

QString LC_QuickInfoEntityData::preparePolylineDescription(const RS_Polyline* polyline, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(polyline, tr("POLYLINE"), level);
    appendWCSAbsolute(result, tr("Start"), polyline->getStartpoint());
    appendWCSAbsolute(result, tr("End"), polyline->getEndpoint());
    appendInt(result, tr("Segments"), polyline->count());
    appendValue(result, tr("Closed"), polyline->isClosed() ? tr("Yes") : tr("No"));
    return result;
}

/**
 * Properties for polyline. information about line and arc vertexes
 * @param polyline
 */
void LC_QuickInfoEntityData::collectPolylineProperties(const RS_Polyline* polyline) {
    m_entityName = tr("POLYLINE");

    double totalLengh = 0.0;
    const bool closed = polyline->isClosed();

    addProperty(tr("Closed"), closed ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);

    const RS_Entity* v = polyline->firstEntity(RS2::ResolveNone);
    //bad polyline without vertex
    if (v == nullptr) {
        return;
    }

    int index = 0;
    const int entitiesCount = polyline->count();
    addProperty(tr("Segments"), formatInt(entitiesCount), PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Vertex - 0:"), polyline->getStartpoint());

    for (RS_Entity* entity : lc::LC_ContainerTraverser{*polyline, RS2::ResolveAll}.entities()) {
        index++;
        if (!entity->isAtomic()) {
            continue;
        }
        const int rtti = entity->rtti();
        switch (rtti) {
            case RS2::EntityArc: {
                const auto arc = static_cast<RS_Arc*>(entity);
                addLinearProperty(tr("Bulge"), arc->getBulge(), PROPERTY_TYPE_OTHER);

                if (m_options->displayPolylineDetailed) {
                    // details of arc
                    const double len = arc->getLength();

                    addVectorProperty(tr("Center"), arc->getCenter());
                    addLinearProperty(tr("Radius"), arc->getRadius());
                    totalLengh += len;
                    addLinearProperty(tr("Circumference", "arc"), len, PROPERTY_TYPE_OTHER);
                    addRawAngleProperty(tr("Angle Length"), arc->getAngleLength());
                    addAngleProperty(tr("Start Angle"), arc->getAngle1());
                    addAngleProperty(tr("End Angle"), arc->getAngle1());
                }
                addVectorProperty(tr("Vertex - "), index, arc->getEndpoint());
                break;
            }
            case RS2::EntityLine: {
                const auto line = static_cast<RS_Line*>(entity);
                const double length = line->getLength();
                totalLengh += length;
                if (m_options->displayPolylineDetailed) {
                    // details of line
                    addVectorProperty(tr("Middle"), line->getMiddlePoint());
                    addAngleProperty(tr("Angle"), line->getAngle1());
                    addLinearProperty(tr("Length"), length);
                    // todo - add delta for line?
                }
                addVectorProperty(tr("Vertex - "), index, line->getEndpoint());
                break;
            }
            default: // actually, only line and arc in polyline, yet still...
                break;
        }
    }
    addLinearProperty(tr("Total Length"), totalLengh, PROPERTY_TYPE_OTHER);
}

QString LC_QuickInfoEntityData::prepareInsertDescription(const RS_Insert* insert, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(insert, tr("INSERT"), level);
    const RS_InsertData& data = insert->getData();
    appendValue(result, tr("Name"), data.name);
    appendWCSAbsolute(result, tr("Insertion Point"), data.insertionPoint);
    appendWCSAngle(result, tr("Angle"), data.angle);
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendDouble(result, tr("Scale X"), data.scaleFactor.x);
        appendDouble(result, tr("Scale Y"), data.scaleFactor.y);
        appendInt(result, tr("Cols"), data.cols);
        appendDouble(result, tr("Spacing X"), data.spacing.x);
        appendInt(result, tr("Rows"), data.rows);
        appendDouble(result, tr("Spacing Y"), data.spacing.y);
    }
    return result;
}

/**
 * Insert properties
 * @param insert
 */
void LC_QuickInfoEntityData::collectInsertProperties(const RS_Insert* insert) {
    m_entityName = tr("INSERT");
    const RS_InsertData& data = insert->getData();

    addProperty(tr("Name"), data.name, PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Scale X"), formatDouble(data.scaleFactor.x), data.scaleFactor.x, PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Scale Y"), formatDouble(data.scaleFactor.y), data.scaleFactor.y, PROPERTY_TYPE_OTHER);
    addProperty(tr("Cols"), formatInt(data.cols), PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Spacing X"), formatDouble(data.spacing.x), data.spacing.x, PROPERTY_TYPE_OTHER);
    addProperty(tr("Rows"), formatInt(data.rows), PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Spacing Y"), formatDouble(data.spacing.y), data.spacing.y, PROPERTY_TYPE_OTHER);
}

QString LC_QuickInfoEntityData::prepareTextDescription(const RS_Text* text, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(text, tr("TEXT"), level);
    const RS_TextData& data = text->getData();
    appendWCSAbsolute(result, tr("Insertion Point"), data.insertionPoint);
    appendWCSAbsolute(result, tr("Second Point"), data.insertionPoint);
    appendWCSAngle(result, tr("Angle"), data.angle);
    appendDouble(result, tr("Width/Height"), data.widthRel);
    appendValue(result, tr("Style"), data.style);
    appendDouble(result, tr("Height"), data.height);

    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        const RS_TextData::HAlign halign = data.halign;
        const RS_TextData::VAlign valign = data.valign;

        const QString halignStr = getHAlignStr(halign);
        const QString valignStr = getVAlignStr(valign);
        const QString generationStr = getTextGenerationStr(data.textGeneration);

        appendValue(result, tr("HAlign"), halignStr);
        appendValue(result, tr("VAlign"), valignStr);
        appendValue(result, tr("Generation"), generationStr);
    }
    return result;
}

/**
 * Text properties
 * @param text
 */
void LC_QuickInfoEntityData::collectTextProperties(const RS_Text* text) {
    m_entityName = tr("TEXT");
    const RS_TextData& data = text->getData();
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addVectorProperty(tr("Second Point"), data.secondPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Height"), formatDouble(data.height), data.height, PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Width/Height"), formatDouble(data.widthRel), data.height, PROPERTY_TYPE_OTHER);
    addProperty(tr("Style"), data.style, PROPERTY_TYPE_OTHER);
    const RS_TextData::HAlign halign = data.halign;
    const RS_TextData::VAlign valign = data.valign;
    const RS_TextData::TextGeneration generation = data.textGeneration;

    const QString halignStr = getHAlignStr(halign);
    const QString valignStr = getVAlignStr(valign);
    const QString generationStr = getTextGenerationStr(generation);

    addProperty(tr("HAlign"), halignStr, PROPERTY_TYPE_OTHER);
    addProperty(tr("VAlign"), valignStr, PROPERTY_TYPE_OTHER);
    addProperty(tr("Generation"), generationStr, PROPERTY_TYPE_OTHER);
}

/**
 * MText align value mapping
 * @param align
 * @return
 */
QString LC_QuickInfoEntityData::getHAlignStr(const RS_MTextData::HAlign align) {
    switch (align) {
        case RS_MTextData::HAlign::HALeft:
            return tr("Left");
        case RS_MTextData::HAlign::HACenter:
            return tr("Centered");
        case RS_MTextData::HAlign::HARight:
            return tr("Right");
        default:
            return "";
    }
}

/**
 * Text align value mapping
 * @param align
 * @return
 */
QString LC_QuickInfoEntityData::getHAlignStr(const RS_TextData::HAlign align) {
    switch (align) {
        case RS_TextData::HAlign::HALeft:
            return tr("Left");
        case RS_TextData::HAlign::HACenter:
            return tr("Centered");
        case RS_TextData::HAlign::HARight:
            return tr("Right");
        case RS_TextData::HAlign::HAAligned:
            return tr("Aligned");
        case RS_TextData::HAlign::HAMiddle:
            return tr("Middle");
        case RS_TextData::HAlign::HAFit:
            return tr("Fit");
        default:
            return "";
    }
}

/**
 * Text valign mapping
 * @param align
 * @return
 */
QString LC_QuickInfoEntityData::getVAlignStr(const RS_TextData::VAlign align) {
    switch (align) {
        case RS_TextData::VAlign::VABaseline:
            return tr("Baseline");
        case RS_TextData::VAlign::VABottom:
            return tr("Bottom");
        case RS_TextData::VAlign::VAMiddle:
            return tr("Middle");
        case RS_TextData::VAlign::VATop:
            return tr("Top");
        default:
            return "";
    }
}

/**
 * MText align value mapping
 * @param align
 * @return
 */
QString LC_QuickInfoEntityData::getVAlignStr(const RS_MTextData::VAlign align) {
    switch (align) {
        case RS_MTextData::VAlign::VABottom:
            return tr("Bottom");
        case RS_MTextData::VAlign::VAMiddle:
            return tr("Middle");
        case RS_MTextData::VAlign::VATop:
            return tr("Top");
        default:
            return "";
    }
}

/**
 * Text generation value mapping
 * @param generation
 * @return
 */
QString LC_QuickInfoEntityData::getTextGenerationStr(const RS_TextData::TextGeneration generation) {
    switch (generation) {
        case RS_TextData::TextGeneration::None:
            return tr("Normal text");
        case RS_TextData::TextGeneration::Backward:
            return tr("Mirrored in X");
        case RS_TextData::TextGeneration::UpsideDown:
            return tr("Mirrored in Y");
        default:
            return "";
    }
}

QString LC_QuickInfoEntityData::prepareMTextDescription(const RS_MText* pText, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(pText, tr("MTEXT"), level);
    const RS_MTextData& data = pText->getData();
    appendWCSAbsolute(result, tr("Insertion Point"), data.insertionPoint);
    appendWCSAngle(result, tr("Angle"), data.angle);
    appendValue(result, tr("Style"), data.style);
    appendDouble(result, tr("Height"), data.height); // todo - is linear format more suitable there?

    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendDouble(result, tr("Width"), data.width);
        appendInt(result, tr("Lines"), pText->getNumberOfLines());
        const RS_MTextData::HAlign halign = data.halign;
        const RS_MTextData::VAlign valign = data.valign;
        const RS_MTextData::MTextDrawingDirection drawingDirection = data.drawingDirection;
        const RS_MTextData::MTextLineSpacingStyle lineSpacingStyle = data.lineSpacingStyle;

        const QString halignStr = getHAlignStr(halign);
        const QString valignStr = getVAlignStr(valign);
        const QString directionStr = getDirectionStr(drawingDirection);
        const QString lineSpacing = getLineSpacingStyleStr(lineSpacingStyle);

        appendValue(result, tr("HAlign"), halignStr);
        appendValue(result, tr("VAlign"), valignStr);
        appendValue(result, tr("Direction"), directionStr);
        appendDouble(result, tr("Line Spacing Factor"), data.lineSpacingFactor);
        appendValue(result, tr("Line Spacing"), lineSpacing);
    }
    return result;
}

/**
 * MText properties
 * @param pText
 */
void LC_QuickInfoEntityData::collectMTextProperties(const RS_MText* pText) {
    m_entityName = tr("MTEXT");
    const RS_MTextData& data = pText->getData();
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Height"), formatDouble(data.height), data.height, PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Width"), formatDouble(data.width), data.width, PROPERTY_TYPE_OTHER);
    addProperty(tr("Lines"), formatInt(pText->getNumberOfLines()), PROPERTY_TYPE_OTHER);
    addProperty(tr("Style"), data.style, PROPERTY_TYPE_OTHER);

    const RS_MTextData::HAlign halign = data.halign;
    const RS_MTextData::VAlign valign = data.valign;
    const RS_MTextData::MTextDrawingDirection drawingDirection = data.drawingDirection;
    const RS_MTextData::MTextLineSpacingStyle lineSpacingStyle = data.lineSpacingStyle;

    const QString halignStr = getHAlignStr(halign);
    const QString valignStr = getVAlignStr(valign);
    const QString directionStr = getDirectionStr(drawingDirection);
    const QString lineSpacing = getLineSpacingStyleStr(lineSpacingStyle);

    addProperty(tr("HAlign"), halignStr, PROPERTY_TYPE_OTHER);
    addProperty(tr("VAlign"), valignStr, PROPERTY_TYPE_OTHER);
    addProperty(tr("Direction"), directionStr, PROPERTY_TYPE_OTHER);
    addDoubleProperty(tr("Line Spacing Factor"), formatDouble(data.lineSpacingFactor), data.lineSpacingFactor, PROPERTY_TYPE_OTHER);
    addProperty(tr("Line Spacing"), lineSpacing, PROPERTY_TYPE_OTHER);
}

/**
 * MText direction value mapping
 * @param direction
 * @return
 */
QString LC_QuickInfoEntityData::getDirectionStr(const RS_MTextData::MTextDrawingDirection direction) {
    switch (direction) {
        case RS_MTextData::MTextDrawingDirection::ByStyle:
            return tr("By Style");
        case RS_MTextData::MTextDrawingDirection::LeftToRight:
            return tr("Left To Right");
        case RS_MTextData::MTextDrawingDirection::RightToLeft:
            return tr("Right To Left");
        case RS_MTextData::MTextDrawingDirection::TopToBottom:
            return tr("Top To Bottom");
        default:
            return "";
    }
}

/**
 * MText line spacing value mapping
 * @param style
 * @return
 */
QString LC_QuickInfoEntityData::getLineSpacingStyleStr(const RS_MTextData::MTextLineSpacingStyle style) {
    switch (style) {
        case RS_MTextData::MTextLineSpacingStyle::AtLeast:
            return tr("At Least");
        case RS_MTextData::MTextLineSpacingStyle::Exact:
            return tr("Exact");
        default:
            return "";
    }
}

QString LC_QuickInfoEntityData::prepareImageDescription(const RS_Image* image, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(image, tr("IMAGE"), level);
    const RS_ImageData& data = image->getData();
    appendValue(result, tr("File"), data.file);
    appendWCSAbsolute(result, tr("Insertion Point"), data.insertionPoint);
    const double scale = data.uVector.magnitude();
    appendDouble(result, tr("Scale"), scale);
    appendWCSAngle(result, tr("Angle"), data.uVector.angle());

    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendLinear(result, tr("Size (X) px"), data.size.x);
        appendLinear(result, tr("Size (Y) px"), data.size.y);
        appendLinear(result, tr("Width"), image->getImageWidth());
        appendLinear(result, tr("Height"), image->getImageHeight());
        appendDouble(result, tr("DPI"), RS_Units::scaleToDpi(scale, image->getGraphicUnit()));
    }
    return result;
}

/**
 * Image properties
 * @param image
 */
void LC_QuickInfoEntityData::collectImageProperties(const RS_Image* image) {
    m_entityName = tr("IMAGE");
    const RS_ImageData& data = image->getData();

    addProperty(tr("File"), data.file, PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), image->getUVector().angle());
    const double scale = data.uVector.magnitude();
    addProperty(tr("Scale"), formatDouble(scale), PROPERTY_TYPE_OTHER);
    addLinearProperty(tr("Size (X) px"), data.size.x);
    addLinearProperty(tr("Size (Y) px"), data.size.y);
    addLinearProperty(tr("Width"), image->getImageWidth());
    addLinearProperty(tr("Height"), image->getImageHeight());
    addProperty(tr("DPI"), formatDouble(RS_Units::scaleToDpi(scale, image->getGraphicUnit())), PROPERTY_TYPE_OTHER);
}

QString LC_QuickInfoEntityData::prepareSplineDescription(const RS_Spline* spline, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(spline, tr("SPLINE"), level);
    const RS_SplineData data = spline->getData();
    appendInt(result, tr("Degree"), data.degree);
    appendInt(result, tr("Control Points"), data.controlPoints.size());
    appendValue(result, tr("Closed"), data.isClosed() ? tr("Yes") : tr("No"));
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendLinear(result, tr("Length"), spline->getLength());
    }
    return result;
}

/**
 * Spline properties
 * @param spline
 */
void LC_QuickInfoEntityData::collectSplineProperties(const RS_Spline* spline) {
    m_entityName = tr("SPLINE");
    const RS_SplineData& data = spline->getData();
    addLinearProperty(tr("Length"), spline->getLength());
    addProperty(tr("Degree"), formatInt(data.degree), PROPERTY_TYPE_OTHER);
    addProperty(tr("Closed"), data.isClosed() ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
    const size_t size = data.controlPoints.size();
    for (size_t i = 0; i < size; i++) {
        RS_Vector cp = data.controlPoints[i];
        if (cp.valid) {
            addVectorProperty(tr("Control Point "), i, cp);
        }
    }
}

QString LC_QuickInfoEntityData::prepareSplinePointsDescription(const LC_SplinePoints* spline, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(spline, tr("SPLINEPOINTS"), level);
    const LC_SplinePointsData data = spline->getData();
    appendValue(result, tr("Use Control Points"), data.useControlPoints ? tr("Yes") : tr("No"));
    if (data.useControlPoints) {
        appendInt(result, tr("Control Points"), data.controlPoints.size());
    }
    else {
        appendInt(result, tr("Spline Points"), data.splinePoints.size());
    }
    appendValue(result, tr("Closed"), data.closed ? tr("Yes") : tr("No"));
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendLinear(result, tr("Length"), spline->getLength());
    }

    return result;
}

/**
 * Spline points properties
 * @param spline
 */
void LC_QuickInfoEntityData::collectSplinePointsProperties(const LC_SplinePoints* spline) {
    m_entityName = tr("SPLINEPOINTS");
    const LC_SplinePointsData data = spline->getData();
    addLinearProperty(tr("Length"), spline->getLength());
    addProperty(tr("Use Control Points"), data.useControlPoints ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
    addProperty(tr("Closed"), data.closed ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);

    size_t size = data.controlPoints.size();
    const QString& name = tr("Control Point ");
    for (size_t i = 0; i < size; i++) {
        addVectorProperty(name, i + 1, data.controlPoints.at(i));
    }
    size = data.splinePoints.size();
    const QString& name1 = tr("Spline Point ");
    for (size_t i = 0; i < size; i++) {
        addVectorProperty(name1, i + 1, data.splinePoints.at(i));
    }
}

QString LC_QuickInfoEntityData::prepareParabolaDescription(const LC_Parabola* parabola, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(parabola, tr("PARABOLA"), level);
    const LC_ParabolaData &data = parabola->getData();
    appendWCSAbsolute(result, tr("Focus"), data.m_focus);
    appendWCSAbsolute(result, tr("Vertex"), data.m_vertex);
    appendWCSAngle(result, tr("Axis Angle"), data.m_axis.angle());
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendLinear(result, tr("Length"), parabola->getLength());
    }
    return result;
}

/**
 * Parabola properties
 * @param parabola
 */
void LC_QuickInfoEntityData::collectParabolaProperties(const LC_Parabola* parabola) {
    m_entityName = tr("PARABOLA");
    const LC_ParabolaData& data = parabola->getData();
    addVectorProperty(tr("Focus"), data.m_focus);
    addVectorProperty(tr("Vertex"), data.m_vertex);
    addAngleProperty(tr("Axis Angle"), data.m_axis.angle());
    addLinearProperty(tr("Length"), parabola->getLength());

    const QString &name = tr("Control Point");
    for (size_t i = 0; i < data.m_controlPoints.size(); i++){
        RS_Vector cp = data.m_controlPoints.at(i);
        addVectorProperty(name, i + 1, cp);
    }
}

QString LC_QuickInfoEntityData::prepareHatchDescription(const RS_Hatch* hatch, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(hatch, tr("HATCH"), level);
    appendValue(result, tr("Solid"), hatch->isSolid() ? tr("Yes") : tr("No"));
    appendValue(result, tr("Pattern"), hatch->getPattern());
    appendDouble(result, tr("Scale"), hatch->getScale());
    appendWCSAngle(result, tr("Angle"), hatch->getAngle()); // fixme - sand - or raw angle value should be there? Check!
    if (level != RS2::EntityDescriptionLevel::DescriptionCatched) {
        appendArea(result, tr("Area"), hatch->getTotalArea());
    }
    return result;
}

/**
 * Hatch properties
 * @param hatch
 */
void LC_QuickInfoEntityData::collectHatchProperties(const RS_Hatch* hatch) {
    m_entityName = tr("HATCH");
    const RS_HatchData& data = hatch->getData();
    const bool solid = data.solid;
    addProperty(tr("Solid"), solid ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
    if (!solid) {
        const double scale = data.scale;
        const double angle = data.angle;
        const QString pattern = data.pattern;

        addProperty(tr("Pattern"), pattern, PROPERTY_TYPE_OTHER);
        addProperty(tr("Scale"), formatDouble(scale), PROPERTY_TYPE_OTHER);
        addAngleProperty(tr("Angle"), angle); // fixme - sand - or raw angle value should be there? Check!
        addAreaProperty(tr("Total Area"), hatch->getTotalArea());
    }
}

QString LC_QuickInfoEntityData::prepareDimLeaderDescription(const RS_Leader* leader, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(leader, tr("DIMLEADER"), level);
    const RS_LeaderData& data = leader->getData();
    appendValue(result, tr("Arrow Head"), data.arrowHead ? tr("Yes") : tr("No"));
    return result;
}

/**
 * Dim leader properties
 * @param leader
 */
void LC_QuickInfoEntityData::collectDimLeaderProperties(const RS_Leader* leader) {
    m_entityName = tr("DIMLEADER");
    const RS_LeaderData& data = leader->getData();
    addProperty(tr("Arrow Head"), data.arrowHead ? tr("Yes") : tr("No"), PROPERTY_TYPE_OTHER);
}

QString LC_QuickInfoEntityData::prepareDimArcDescription(const LC_DimArc* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMARC"), level);
    // if (!level){
    const LC_DimArcData& data = dim->getData();
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendLinear(result, tr("Radius"), data.radius);
    appendLinear(result, tr("Arc Length"), data.arcLength);
    appendWCSAbsolute(result, tr("Center"), data.centre);
    appendWCSAngle(result, tr("Start Angle"), dim->getStartAngle());
    appendWCSAngle(result, tr("End Angle"), dim->getEndAngle());
    // }
    return result;
}

/**
 * Dim arc properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimArcProperties(const LC_DimArc* dim) {
    m_entityName = tr("DIMARC");
    const LC_DimArcData& data = dim->getData();
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);

    addLinearProperty(tr("Radius"), data.radius);
    addLinearProperty(tr("Arc Length"), data.arcLength);
    addVectorProperty(tr("Center"), data.centre);

    addAngleProperty(tr("Start Angle"), dim->getStartAngle());
    addAngleProperty(tr("End Angle"), dim->getEndAngle());

    //    todo - potentially, for dimensions we can also could show variables - yet they are defined by settings for
    //    todo drawing, so this rather will be overkill?
    //    addLinearProperty("Arrow Size",dimarc->getArrowSize());
}

QString LC_QuickInfoEntityData::prepareDimAngularDescription(const RS_DimAngular* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMANGULAR"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    //    appendAbsolute(result, tr("Extension Point 1"), dim->getExtensionPoint1());
    //    appendAbsolute(result, tr("Extension Point 2"), dim->getExtensionPoint1());
    return result;
}

/**
 * Dimangular properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimAngularProperties([[maybe_unused]] const RS_DimAngular* dim) {
    m_entityName = tr("DIMANGULAR");
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    //    const RS_DimensionData &data = dimang->getData();
    //    todo - is it actually necessary to show more info here?
}

QString LC_QuickInfoEntityData::prepareDimDiametricDescription(const RS_DimDiametric* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMDIAMETRIC"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendWCSAbsolute(result, tr("Definition Point"), dim->getDiametricDefinitionPoint());
    return result;
}

void LC_QuickInfoEntityData::collectDimDiametricProperties([[maybe_unused]] const RS_DimDiametric* dim) {
    m_entityName = tr("DIMDIAMETRIC");
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Definition Point"), dim->getDiametricDefinitionPoint());
    //    addLinearProperty("Leader", dimdia->getLeader());
}

QString LC_QuickInfoEntityData::prepareDimRadialDescription(const RS_DimRadial* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMRADIAL"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendWCSAbsolute(result, tr("Definition Point"), dim->getRadialDefinitionPoint());
    return result;
}

/**
 * Dim radial properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimRadialProperties(const RS_DimRadial* dim) {
    m_entityName = tr("DIMRADIAL");
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Definition Point"), dim->getRadialDefinitionPoint());
    //    addLinearProperty("Leader", dimrad->getLeader());
}

QString LC_QuickInfoEntityData::getDimensionStyleString(const RS_Dimension* dim) {
    QString style = dim->getStyle();
    if (dim->getDimStyleOverride() != nullptr) {
        style.append(" - ").append(tr("[Override]"));
    }
    return style;
}

QString LC_QuickInfoEntityData::prepareDimLinearDescription(const RS_DimLinear* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMLINEAR"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendWCSAbsolute(result, tr("Definition Point"), dim->getDefinitionPoint());
    appendWCSAbsolute(result, tr("Extension Point 1"), dim->getExtensionPoint1());
    appendWCSAbsolute(result, tr("Extension Point 2"), dim->getExtensionPoint2());
    appendWCSAbsolute(result, tr("Text Middle Point"), dim->getMiddleOfText());
    appendWCSAngle(result, tr("Angle"), dim->getAngle());
    appendWCSAngle(result, tr("Oblique"), dim->getOblique());
    return result;
}

/**
 * Dim linear properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimLinearProperties(const RS_DimLinear* dim) {
    m_entityName = tr("DIMLINEAR");
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Definition Point"), dim->getDefinitionPoint());
    addVectorProperty(tr("Extension Point 1"), dim->getExtensionPoint1());
    addVectorProperty(tr("Extension Point 2"), dim->getExtensionPoint2());
    addVectorProperty(tr("Text Middle Point"), dim->getMiddleOfText());
    addAngleProperty(tr("Angle"), dim->getAngle());
    addAngleProperty(tr("Oblique"), dim->getOblique());
}

QString LC_QuickInfoEntityData::prepareDimOrdinateDescription(const LC_DimOrdinate* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMORDINATE"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendValue(result, tr("Ordinate"), dim->isForXDirection() ? "X" : "Y");
    appendWCSAbsolute(result, tr("Origin Point"), dim->getGenericData().definitionPoint);
    appendWCSAngle(result, tr("Horizontal Direction"), dim->getHDir());
    appendWCSAbsolute(result, tr("Feature Point"), dim->getFeaturePoint());
    appendWCSAbsolute(result, tr("Leader End Point"), dim->getLeaderEndPoint());
    appendWCSAbsolute(result, tr("Text Middle Point "), dim->getGenericData().middleOfText);
    return result;
}

void LC_QuickInfoEntityData::collectDimOrdinateProperties(const LC_DimOrdinate* dim) {
    m_entityName = tr("DIMORDINATE");
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Origin Point"), dim->getDefinitionPoint());
    addAngleProperty(tr("Horizontal Direction"), dim->getHDir());
    addProperty(tr("Ordinate"), dim->isForXDirection() ? "X" : "Y", PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Feature Point"), dim->getFeaturePoint());
    addVectorProperty(tr("Leader End Point"), dim->getLeaderEndPoint());
    addVectorProperty(tr("Text Middle Point"), dim->getGenericData().middleOfText);
}

QString LC_QuickInfoEntityData::prepareDimAlignedDescription(const RS_DimAligned* dim, const RS2::EntityDescriptionLevel level) {
    QString result = prepareGenericEntityDescription(dim, tr("DIMALIGNED"), level);
    appendValue(result, tr("Style"), getDimensionStyleString(dim));
    appendWCSAbsolute(result, tr("Definition Point"), dim->getDefinitionPoint());
    appendWCSAbsolute(result, tr("Extension Point 1"), dim->getExtensionPoint1());
    appendWCSAbsolute(result, tr("Extension Point 2"), dim->getExtensionPoint2());
    appendWCSAbsolute(result, tr("Text Middle Point"), dim->getMiddleOfText());
    return result;
}

/**
 * Dim Aligned properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimAlignedProperties(const RS_DimAligned* dim) {
    m_entityName = tr("DIMALIGNED");
    //    addAngleProperty("Angle", dim->getAngle());
    addProperty(tr("Style"), getDimensionStyleString(dim), PropertyType::PROPERTY_TYPE_OTHER);
    addVectorProperty(tr("Definition Point"), dim->getDefinitionPoint());
    addVectorProperty(tr("Extension Point 1"), dim->getExtensionPoint1());
    addVectorProperty(tr("Extension Point 2"), dim->getExtensionPoint2());
    addVectorProperty(tr("Text Middle Point"), dim->getMiddleOfText());
}

/**
 * Utility method for adding vector property (with recalculation of coordinate, if needed)
 * @param name
 * @param value
 * @param type
 */

void LC_QuickInfoEntityData::addVectorProperty(const QString& name, const RS_Vector& value, const PropertyType type) {
    const RS_Vector relZero = getRelativeZero();
    QString vectorStr;
    if (m_coordinatesMode == COORD_RELATIVE && relZero.valid) {
        // fixme - check!
        const RS_Vector viewValue = value - relZero;
        vectorStr = formatWCSDeltaVector(viewValue);
    }
    else {
        vectorStr = formatWCSVector(value);
    }
    addVectorProperty(name, vectorStr, value, type);
}

void LC_QuickInfoEntityData::addDeltaVectorProperty(const QString& name, const RS_Vector& value, const PropertyType type) {
    addVectorProperty(name, formatWCSDeltaVector(value), value, type);
}

/**
 * Utility method for adding vector property, where property's name is followed by index
 * @param name
 * @param count
 * @param value
 * @param type
 */
void LC_QuickInfoEntityData::addVectorProperty(const QString& name, const size_t count, const RS_Vector& value, const PropertyType type) {
    const RS_Vector relZero = getRelativeZero();
    QString vectorStr;
    if (m_coordinatesMode == COORD_RELATIVE && relZero.valid) {
        const RS_Vector viewValue = value - relZero;
        vectorStr = formatWCSDeltaVector(viewValue);
    }
    else {
        vectorStr = formatWCSVector(value);
    }
    addVectorProperty(name, count, vectorStr, value, type);
}

/**
 * Adding angle property
 * @param name
 * @param value
 */
void LC_QuickInfoEntityData::addAngleProperty(const QString& name, const double value) {
    addDoubleProperty(name, formatWCSAngle(value), value, PROPERTY_TYPE_ANGLE);
}

void LC_QuickInfoEntityData::addRawAngleProperty(const QString& name, const double value) {
    addDoubleProperty(name, formatRawAngle(value), value, PROPERTY_TYPE_ANGLE);
}

/**
 * adding Linear property
 * @param name
 * @param value
 * @param type
 */
void LC_QuickInfoEntityData::addLinearProperty(const QString& name, const double value, const PropertyType type) {
    addDoubleProperty(name, formatLinear(value), value, type);
}

/**
 * Adding area property
 * @param name
 * @param value
 */
void LC_QuickInfoEntityData::addAreaProperty(const QString& name, const double value) {
    addDoubleProperty(name, formatLinear(value), value, PROPERTY_TYPE_AREA);
}

/**
 * Return vector for property with given index
 * @param index
 * @return
 */
RS_Vector LC_QuickInfoEntityData::getVectorForIndex(const int index) const {
    auto result = RS_Vector(false);
    const size_t size = m_properties.size();
    if (index < size) {
        const auto property = static_cast<VectorPropertyInfo*>(m_properties.at(index));
        result = property->data;
    }
    return result;
}

/**
 * Returns value for given property's index
 * @param index
 * @return
 */
QString LC_QuickInfoEntityData::getValue(const int index) const {
    const auto property = m_properties.at(index);
    QString result = property->value;
    return result;
}

void LC_QuickInfoEntityData::addVectorProperty(const QString& name, const QString& valueStr, const RS_Vector& coord,
                                               const PropertyType type) {
    auto* prop = new VectorPropertyInfo(name, valueStr, type, coord);
    m_properties << prop;
}

void LC_QuickInfoEntityData::addVectorProperty(QString name, const int count, const QString& valueStr, const RS_Vector& coord,
                                               const PropertyType type) {
    QString idx;
    idx.setNum(count);
    auto* prop = new VectorPropertyInfo(name.append(" ").append(idx), valueStr, type, coord);
    m_properties << prop;
}

void LC_QuickInfoEntityData::addDoubleProperty(const QString& name, const QString& valueStr, const double value, const PropertyType type) {
    auto* prop = new DoublePropertyInfo(name, valueStr, type, value);
    m_properties << prop;
}

void LC_QuickInfoEntityData::addProperty(const QString& name, const QString& valueStr, const PropertyType type) {
    auto* prop = new PropertyInfo(name, valueStr, type);
    m_properties << prop;
}

/**
 * cleanup
 */
void LC_QuickInfoEntityData::clear() {
    qDeleteAll(m_properties.begin(), m_properties.end());
    m_properties.clear();
    m_entityId = 0;
    m_entityName.clear();
}

void LC_QuickInfoEntityData::setOptions(LC_QuickInfoOptions* opt) {
    m_options = opt;
}

LC_QuickInfoEntityData::PropertyInfo::PropertyInfo(const QString& label, const QString& value, const int type) : label(label), value(value),
    type(type) {
}

LC_QuickInfoEntityData::VectorPropertyInfo::VectorPropertyInfo(const QString& label, const QString& value, const int type,
                                                               const RS_Vector& coord)
    : PropertyInfo(label, value, type), data(coord) {
}

LC_QuickInfoEntityData::DoublePropertyInfo::DoublePropertyInfo(const QString& label, const QString& value, const int type,
                                                               const double d) : PropertyInfo(label, value, type), data(d) {
}
