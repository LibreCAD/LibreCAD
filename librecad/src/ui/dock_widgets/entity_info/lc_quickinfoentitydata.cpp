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
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_point.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_spline.h"
#include "lc_splinepoints.h"
#include "lc_parabola.h"
#include "rs_hatch.h"
#include "rs_leader.h"
#include "lc_dimarc.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimradial.h"
#include "rs_dimlinear.h"
#include "rs_dimaligned.h"
#include "rs_math.h"
#include "lc_peninforegistry.h"
#include "lc_quickinfoentitydata.h"

#include "rs_units.h"

LC_QuickInfoEntityData::LC_QuickInfoEntityData():LC_QuickInfoBaseData(){}

LC_QuickInfoEntityData::~LC_QuickInfoEntityData(){
    clear();
}

/**
 * Returns true if there is entity data, false otherwise
 * @return
 */
bool LC_QuickInfoEntityData::hasData() const{
    return entityId > 0;
}

/**
 * Detects type of given entity and performs preparation of properties for it
 * @param en entity
 * @return true it view should be updated, false otherwise
 */
bool LC_QuickInfoEntityData::processEntity(RS_Entity *en){
    bool shouldUpdate;
    if (entityId == 0){ // no special value for empty id, yet according to implementation, it seems that 0 should not be used
        shouldUpdate = true;
    } else if (en->getId() == entityId){ // same entity... so we'll try to optimize a bit mouse move there.
        shouldUpdate = false;
    } else {
        shouldUpdate = true;
    }
    if (shouldUpdate){
        clear();

        // collecting some generic common properties of all entities
        collectGenericProperties(en);

        // just processing of entity based on type
        int rtti = en->rtti();
        switch (rtti) {
            case RS2::EntityLine: {
                auto *line = reinterpret_cast<RS_Line *>(en);
                collectLineProperties(line);
                break;
            }
            case RS2::EntityCircle: {
                auto *circle = reinterpret_cast<RS_Circle *>(en);
                collectCircleProperties(circle);
                break;
            }
            case RS2::EntityArc: {
                auto *arc = reinterpret_cast<RS_Arc *>(en);
                collectArcProperties(arc);
                break;
            }
            case RS2::EntityEllipse: {
                auto *ellipse = reinterpret_cast<RS_Ellipse *>(en);
                collectEllipseProperties(ellipse);
                break;
            }
            case RS2::EntityPoint: {
                auto *point = reinterpret_cast<RS_Point *>(en);
                collectPointProperties(point);
                break;
            }
            case RS2::EntityPolyline: {
                auto *pline = reinterpret_cast<RS_Polyline *>(en);
                collectPolylineProperties(pline);
                break;
            }
            case RS2::EntityInsert: {
                auto *pinsert = reinterpret_cast<RS_Insert *>(en);
                collectInsertProperties(pinsert);
                break;
            }
            case RS2::EntityMText: {         /**< Multi-line Text */
                auto *pmtext = reinterpret_cast<RS_MText *> (en);
                collectMTextProperties(pmtext);
                break;
            }
            case RS2::EntityText: {        /**< Single-line Text */
                auto *ptext = reinterpret_cast<RS_Text *> (en);
                collectTextProperties(ptext);
                break;
            }
            case RS2::EntityDimAligned:  {
                auto *dim = reinterpret_cast<RS_DimAligned *> (en);
                collectDimAlignedProperties(dim);
                break;
            }
            case RS2::EntityDimLinear: {
                auto *dim = reinterpret_cast<RS_DimLinear *> (en);
                collectDimLinearProperties(dim);
                break;
            }
            case RS2::EntityDimRadial: {
                auto *dimrad = reinterpret_cast<RS_DimRadial *> (en);
                collectDimRadialProperties(dimrad);
                break;
            }
            case RS2::EntityDimDiametric: {
                auto *dimdia = reinterpret_cast<RS_DimDiametric *> (en);
                collectDimDiametricProperties(dimdia);
                break;
            }
            case RS2::EntityDimAngular:{
                auto *dimang = reinterpret_cast<RS_DimAngular *> (en);
                collectDimAngularProperties(dimang);
                break;
            }
            case RS2::EntityDimArc:  {
                auto *dimarc = reinterpret_cast<LC_DimArc *> (en);
                collectDimArcProperties(dimarc);
                break;
            }
            case RS2::EntityDimLeader:{
                auto *leader = reinterpret_cast<RS_Leader *> (en);
                collectDimLeaderProperties(leader);
                break;
            }
            case RS2::EntityHatch:{
                auto *hatch = reinterpret_cast<RS_Hatch *> (en);
                collectHatchProperties(hatch);
                break;
            }
            case RS2::EntityImage:  {
               auto *pimage = reinterpret_cast<RS_Image *> (en);
               collectImageProperties(pimage);
               break;
            }
            case RS2::EntitySpline: {
                auto *pspline = reinterpret_cast<RS_Spline *> (en);
                collectSplineProperties(pspline);
                break;
            }
            case RS2::EntitySplinePoints: {
                auto *psplinepoints = reinterpret_cast<LC_SplinePoints *> (en);
                collectSplinePointsProperties(psplinepoints);
                break;
            }
            case RS2::EntityParabola: {
                auto *parabola = reinterpret_cast<LC_Parabola *> (en);
                collectParabolaProperties(parabola);
                break;
            }
            case RS2::EntitySolid:
                entityName = tr("SOLID");
                break;
            case RS2::EntityConstructionLine:
                entityName = tr("CONSTRUCTION");
                break;
            default:
                entityName = tr("UNKNOWN");
                break;
        }
    }
    return shouldUpdate;
}

/**
 * Generates HTML table which includes collected properties of current entity. That html will be shown by the widget.
 * Table has 2 columns (label, value)
 * HTML links are used for coordinates and values, so they may be processed by the widget.
 *
 * @return html with entity properties.
 */
QString LC_QuickInfoEntityData::generateView(){
    int propertiesCount = properties.size();
    QString data = "<body><table>";
    data.append("<tr><td colspan = '2'><b>").append(entityName).append("</b></td></tr>");
    for (int i = 0; i < propertiesCount; i++) {
        data.append("<tr>");
        PropertyInfo *property = properties.at(i);
        data.append("<td>");

        if (property->type == PropertyType::VECTOR){
            // support of setting relative zero
            createLink(data, "zero", i, tr("Set Relative Zero"), property->label);
            data.append(": ");
        } else {
            data.append(property->label).append(": ");
        }
        data.append("</td>");
        data.append("<td>");


        if (property->type == PropertyType::VECTOR){
            createLink(data, "coord", i, tr("To Cmd"), property->value);
        } else if (property->type == PropertyType::LINEAR || property->type == PropertyType::ANGLE){
            createLink(data, "val", i, tr("To Cmd"), property->value);
        } else {
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
bool LC_QuickInfoEntityData::updateForCoordinateViewMode(int mode){
    if (mode != coordinatesMode){
        coordinatesMode = mode;
        int propertiesCount = properties.size();
        RS_Vector relativeZero = graphicView->getRelativeZero();

        for (int i = 0; i < propertiesCount; i++) {
            PropertyInfo *propertyInfo = properties.at(i);
            if (propertyInfo->type == VECTOR){
                auto *vectorProperty = static_cast<VectorPropertyInfo *>(propertyInfo);
                RS_Vector data = vectorProperty->data;
                RS_Vector viewVector;
                if (mode == COORD_RELATIVE && relativeZero.valid){
                    viewVector = data - relativeZero;
                } else {
                    viewVector = data;
                }
                QString newValue = formatVector(viewVector);
                propertyInfo->value = newValue;
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
void LC_QuickInfoEntityData::collectGenericProperties(RS_Entity *e){
    RS_Pen pen = e->getPen(false);
    RS_Pen resolvedPen = e->getPen(true);
    RS_Layer *layer = e->getLayer(true);
    QString layerName = "";
    if (layer != nullptr){
        layerName = layer->getName();
    }

    // visual attributes
    RS_Color color = pen.getColor();
    RS_Color resolvedColor = resolvedPen.getColor();
    RS2::LineType lineType = pen.getLineType();
    RS2::LineType resolvedLineType = resolvedPen.getLineType();
    RS2::LineWidth lineWidth = pen.getWidth();
    RS2::LineWidth resolvedLineWidth = resolvedPen.getWidth();
    int colorType = LC_PenInfoRegistry::NATURAL;
    QString colorName = penRegistry->getColorName(color, colorType);
    QString lineTypeName = penRegistry->getLineTypeText(lineType);
    QString lineWidthName = penRegistry->getLineWidthText(lineWidth);

    unsigned long id = e->getId();
    entityId = id;
    if (options->displayEntityID) {
        QString idStr;
        idStr.setNum(id);
        addProperty(tr("ID"), idStr, OTHER);
    }
    addProperty(tr("Layer"), layerName, OTHER);
    if (resolvedColor != color){
        QString actualColorName = penRegistry->getColorName(resolvedColor, colorType);
        colorName = colorName.append(" / ").append(actualColorName);
    }
    addProperty(tr("Color"), colorName, OTHER);
    if (resolvedLineType != lineType){
        QString resolvedLineTypeName = penRegistry->getLineTypeText(resolvedLineType);
        lineTypeName.append(" / ").append(resolvedLineTypeName);
    }
    addProperty(tr("Line Type"), lineTypeName, OTHER);
    if (resolvedLineType != lineType){
        QString resolvedLineWidthName = penRegistry->getLineWidthText(resolvedLineWidth);
        lineWidthName.append(" / ").append(resolvedLineWidthName);
    }
    addProperty(tr("Line Width"), lineWidthName, OTHER);
    if (options->displayEntityBoundaries){
        addVectorProperty(tr("Min"), e->getMin());
        addVectorProperty(tr("Max"), e->getMax());
    }
}

/**
 * Properties for line
 * @param line
 */
void LC_QuickInfoEntityData::collectLineProperties(RS_Line *line){
    entityName = tr("LINE");
    const RS_Vector &start = line->getStartpoint();
    const RS_Vector &end = line->getEndpoint();
    double angle = line->getAngle1();
    double length = line->getLength();
    RS_Vector delta = end - start;

    addVectorProperty(tr("From"), start);
    addVectorProperty(tr("To"), end);
    addVectorProperty(tr("Middle"), line->getMiddlePoint());
    addVectorProperty(tr("Delta"), delta, OTHER);
    addAngleProperty(tr("Angle"), angle);
    addLinearProperty(tr("Length"), length);
}

/**
 * Properties for circle
 * @param circle
 */
void LC_QuickInfoEntityData::collectCircleProperties(RS_Circle *circle){
    entityName = tr("CIRCLE");
    RS_Vector center = circle->getCenter();
    double radius = circle->getRadius();
    double circumference = circle->getLength();
    double area = circle->areaLineIntegral();
    double diameter = radius * 2;

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Radius"), radius);
    addLinearProperty(tr("Diameter"), diameter);
    addLinearProperty(tr("Circumference"), circumference, OTHER);
    addAreaProperty(tr("Area"), area);
}

/**
 * properties for arc
 * @param arc
 */
void LC_QuickInfoEntityData::collectArcProperties(RS_Arc *arc){
    entityName = tr("ARC");
    RS_Vector center = arc->getCenter();
    double radius = arc->getRadius();
    double circumference = arc->getLength();
    double diameter = radius * 2;
    double angleLength = arc->getAngleLength();
    double startAngle = arc->getAngle1();
    double endAngle = arc->getAngle2();
    RS_Vector startPoint = arc->getStartpoint();
    RS_Vector endPoint = arc->getEndpoint();
    double chordLength = startPoint.distanceTo(endPoint);

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Radius"), radius);
    addLinearProperty(tr("Diameter"), diameter);
    addLinearProperty(tr("Circumference"), circumference, OTHER);
    addLinearProperty(tr("Chord Length"), chordLength, OTHER);
    addAngleProperty(tr("Angle Length"), angleLength);
    addVectorProperty(tr("Start"), startPoint);
    addAngleProperty(tr("Start Angle"), startAngle);
    addVectorProperty(tr("End"), endPoint);
    addAngleProperty(tr("End Angle"), endAngle);
    addLinearProperty(tr("Bulge"), arc->getBulge(), OTHER);
}

/**
 * properties for ellipse
 * @param ellipse
 */
void LC_QuickInfoEntityData::collectEllipseProperties(RS_Ellipse *ellipse){
    RS_Vector center = ellipse->getCenter();
    double minorRadius = ellipse->getMinorRadius();
    double majorRadius = ellipse->getMajorRadius();
    double ratio = ellipse->getRatio();
    double circumference = ellipse->getLength();
    double angle = ellipse->getAngle();

    addVectorProperty(tr("Center"), center);
    addLinearProperty(tr("Minor Radius"), minorRadius);
    addLinearProperty(tr("Major Radius"), majorRadius);
    addProperty(tr("Ratio"), formatDouble(ratio), OTHER);
    addAngleProperty(tr("Angle"), angle);
    addLinearProperty(tr("Circumference"), circumference, OTHER);

    if (ellipse->isEllipticArc()){
        entityName = tr("ELLIPSE ARC");

        double angleLength = ellipse->getAngleLength();
        double startAngle = ellipse->getAngle1();
        double endAngle = ellipse->getAngle2();
        RS_Vector startPoint = ellipse->getStartpoint();
        RS_Vector endPoint = ellipse->getEndpoint();

        addAngleProperty(tr("Angle Length"), angleLength);
        addVectorProperty(tr("Start"), startPoint);
        addAngleProperty(tr("Start Angle"), startAngle);
        addVectorProperty(tr("End"), endPoint);
        addAngleProperty(tr("End Angle"), endAngle);

    } else {
        double area = ellipse->areaLineIntegral();
        addAreaProperty(tr("Area"), area);
        entityName = tr("ELLIPSE");
    }
}

/**
 * Properties for point
 */
void LC_QuickInfoEntityData::collectPointProperties(RS_Point *point){
    entityName = tr("POINT");
    RS_Vector center = point->getPos();
    addVectorProperty(tr("Position"), center);
}

/**
 * Properties for polyline. information about line and arc vertexes
 * @param l
 */
void LC_QuickInfoEntityData::collectPolylineProperties(RS_Polyline *l){
    entityName = tr("POLYLINE");

    double totalLengh = 0.0;
    bool closed = l->isClosed();

    addProperty(tr("Closed"), closed ? tr("Yes") : tr("No"), OTHER);

    RS_Entity *v = l->firstEntity(RS2::ResolveNone);
    //bad polyline without vertex
    if (!v) return;

    RS_Arc *arc;
    RS_Line *line;
    int index = 0;
    int entitiesCount = l->count();
    addProperty(tr("Segments"), formatInt(entitiesCount), OTHER);
    addVectorProperty(tr("Vertex - 0:"), l->getStartpoint());

    for (RS_Entity *entity = l->firstEntity(RS2::ResolveAll); entity; entity = l->nextEntity(RS2::ResolveAll)) {
        index++;
        if (!entity->isAtomic()){
            continue;
        }
        int rtti = entity->rtti();
        switch (rtti) {
            case RS2::EntityArc: {
                arc = dynamic_cast<RS_Arc *> (entity);
                addLinearProperty(tr("Bulge"), arc->getBulge(), OTHER);
                double len = arc->getLength();
                if (options->displayPolylineDetailed){ // details of arc
                    addVectorProperty(tr("Center"), arc->getCenter());
                    addLinearProperty(tr("Radius"), arc->getRadius());
                    totalLengh += len;
                    addLinearProperty(tr("Circumference"), len, OTHER);
                    addAngleProperty(tr("Angle Length"), arc->getAngleLength());
                    addAngleProperty(tr("Start Angle"), arc->getAngle1());
                    addAngleProperty(tr("End Angle"), arc->getAngle1());
                }
                addVectorProperty(tr("Vertex - "), index, arc->getEndpoint());
                break;
            }
            case RS2::EntityLine: {
                line = dynamic_cast<RS_Line *>(entity);
                double length = line->getLength();
                totalLengh += length;
                if (options->displayPolylineDetailed){ // details of line
                    addVectorProperty(tr("Middle"), line->getMiddlePoint());
                    addAngleProperty(tr("Angle"), line->getAngle1());
                    addLinearProperty(tr("Length"), length);
                }
                addVectorProperty(tr("Vertex - "), index, line->getEndpoint());
                break;
            }
            default: // actually, only line and arc in polyline, yet still...
                break;
        }
    }
    addLinearProperty(tr("Total Length"), totalLengh, OTHER);
}

/**
 * Insert properties
 * @param insert
 */
void LC_QuickInfoEntityData::collectInsertProperties(RS_Insert *insert){
    entityName = tr("INSERT");
    const RS_InsertData &data = insert->getData();

    addProperty(tr("Name"), data.name, OTHER);
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Scale X"), formatDouble(data.scaleFactor.x), data.scaleFactor.x, OTHER);
    addDoubleProperty(tr("Scale Y"), formatDouble(data.scaleFactor.y), data.scaleFactor.y, OTHER);
    addProperty(tr("Cols"), formatInt(data.cols), OTHER);
    addDoubleProperty(tr("Spacing X"), formatDouble(data.spacing.x), data.spacing.x, OTHER);
    addProperty(tr("Rows"), formatInt(data.rows), OTHER);
    addDoubleProperty(tr("Spacing Y"), formatDouble(data.spacing.y), data.spacing.y, OTHER);
}

/**
 * Text properties
 * @param text
 */
void LC_QuickInfoEntityData::collectTextProperties(RS_Text *text){
    entityName = tr("TEXT");
    const RS_TextData &data = text->getData();
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addVectorProperty(tr("Second Point"), data.secondPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Height"), formatDouble(data.height), data.height, OTHER);
    addDoubleProperty(tr("Width/Height"), formatDouble(data.widthRel), data.height, OTHER);
    addProperty(tr("Style"), data.style, OTHER);
    RS_TextData::HAlign halign = data.halign;
    RS_TextData::VAlign valign = data.valign;
    RS_TextData::TextGeneration generation = data.textGeneration;
    
    QString halignStr = getHAlignStr(halign);
    QString valignStr = getVAlignStr(valign);
    QString generationStr = getTextGenerationStr(generation);

    addProperty(tr("HAlign"), halignStr, OTHER);
    addProperty(tr("VAlign"), valignStr, OTHER);
    addProperty(tr("Generation"), generationStr, OTHER);
}

/**
 * MText align value mapping
 * @param align
 * @return
 */
QString LC_QuickInfoEntityData::getHAlignStr(RS_MTextData::HAlign align){
    switch (align){
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
QString LC_QuickInfoEntityData::getHAlignStr(RS_TextData::HAlign align){    
    switch (align){
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
QString LC_QuickInfoEntityData::getVAlignStr(RS_TextData::VAlign align){
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
QString LC_QuickInfoEntityData::getVAlignStr(RS_MTextData::VAlign align){
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
QString LC_QuickInfoEntityData::getTextGenerationStr(RS_TextData::TextGeneration generation){
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

/**
 * MText properties
 * @param pText
 */
void LC_QuickInfoEntityData::collectMTextProperties(RS_MText *pText){
    entityName = tr("MTEXT");
    const RS_MTextData &data = pText->getData();
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), data.angle);
    addDoubleProperty(tr("Height"), formatDouble(data.height), data.height, OTHER);
    addDoubleProperty(tr("Width"), formatDouble(data.width), data.width, OTHER);
    addProperty(tr("Lines"), formatInt(pText->getNumberOfLines()),  OTHER);
    addProperty(tr("Style"), data.style, OTHER);

    RS_MTextData::HAlign halign = data.halign;
    RS_MTextData::VAlign valign = data.valign;
    RS_MTextData::MTextDrawingDirection drawingDirection = data.drawingDirection;
    RS_MTextData::MTextLineSpacingStyle lineSpacingStyle = data.lineSpacingStyle;

    QString halignStr = getHAlignStr(halign);
    QString valignStr = getVAlignStr(valign);
    QString directionStr = getDirectionStr(drawingDirection);
    QString lineSpacing = getLineSpacingStyleStr(lineSpacingStyle);

    addProperty(tr("HAlign"), halignStr, OTHER);
    addProperty(tr("VAlign"), valignStr, OTHER);
    addProperty(tr("Direction"), directionStr, OTHER);
    addDoubleProperty(tr("Line Spacing Factor"), formatDouble(data.lineSpacingFactor), data.lineSpacingFactor, OTHER);
    addProperty(tr("Line Spacing"), lineSpacing, OTHER);
}

/**
 * MText direction value mapping
 * @param direction
 * @return
 */
QString LC_QuickInfoEntityData::getDirectionStr(RS_MTextData::MTextDrawingDirection direction){
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
QString LC_QuickInfoEntityData::getLineSpacingStyleStr(RS_MTextData::MTextLineSpacingStyle style){
    switch (style){
        case RS_MTextData::MTextLineSpacingStyle::AtLeast:
            return tr("At Least");
        case RS_MTextData::MTextLineSpacingStyle::Exact:
            return tr("Exact");
        default:
          return "";
    }
}

/**
 * Image properties
 * @param image
 */
void LC_QuickInfoEntityData::collectImageProperties(RS_Image *image){
    entityName = tr("IMAGE");
    const RS_ImageData &data = image->getData();

    addProperty(tr("File"), data.file, OTHER);
    addVectorProperty(tr("Insertion Point"), data.insertionPoint);
    addAngleProperty(tr("Angle"), image->getUVector().angle());
    double scale = data.uVector.magnitude();
    addProperty(tr("Scale"), formatDouble(scale), OTHER);
    addLinearProperty(tr("Size (X) px"), data.size.x);
    addLinearProperty(tr("Size (Y) px"), data.size.y);
    addLinearProperty(tr("Width"), image->getImageWidth());
    addLinearProperty(tr("Height"), image->getImageHeight());
    addProperty(tr("DPI"), formatDouble(RS_Units::scaleToDpi(scale,image->getGraphicUnit())), OTHER);
}

/**
 * Spline properties
 * @param spline
 */
void LC_QuickInfoEntityData::collectSplineProperties(RS_Spline *spline){
    entityName = tr("SPLINE");
    const RS_SplineData &data = spline->getData();
    addLinearProperty(tr("Length"), spline->getLength());
    addProperty(tr("Degree"), formatInt(data.degree), OTHER);
    addProperty(tr("Closed"), data.closed? tr("Yes"): tr("No"), OTHER);
    size_t size = data.controlPoints.size();
    for (size_t i = 0; i < size; i++){
        RS_Vector cp = data.controlPoints.at(i);
        if (cp.valid){
           addVectorProperty(tr("Control Point "), i, cp);
        }
    }
}

/**
 * Spline points properties
 * @param spline
 */
void LC_QuickInfoEntityData::collectSplinePointsProperties(LC_SplinePoints *spline){
    entityName = tr("SPLINEPOINTS");
    LC_SplinePointsData data = spline->getData();
    addLinearProperty(tr("Length"), spline->getLength());
    addProperty(tr("Use Control Points"), data.useControlPoints ? tr("Yes"): tr("No"), OTHER);
    addProperty(tr("Closed"), data.closed? tr("Yes"): tr("No"), OTHER);

    size_t size = data.controlPoints.size();
    for (size_t i = 0; i < size; i++){
        addVectorProperty(tr("Control Point "), i+1, data.controlPoints.at(i)); // fixme - translation in cycle
    }
    size = data.splinePoints.size();
    for (size_t i = 0; i < size; i++){
        addVectorProperty(tr("Spline Point "), i+1, data.splinePoints.at(i)); // fixme - translation in cycle
    }
}

/**
 * Parabola properties
 * @param parabola
 */
void LC_QuickInfoEntityData::collectParabolaProperties(LC_Parabola *parabola){
    entityName = tr("PARABOLA");
    LC_ParabolaData &data = parabola->getData();
    addVectorProperty(tr("Focus"), data.focus);
    addVectorProperty(tr("Vertex"), data.vertex);
    addAngleProperty(tr("Axis Angle"), data.axis.angle());
    addLinearProperty(tr("Length"),  parabola->getLength());

    for (size_t i = 0; i < data.controlPoints.size(); i++){
        RS_Vector cp = data.controlPoints.at(i);
        addVectorProperty(tr("Control Point"), i+1, cp); // fixme - translation in cycle
    }
}

/**
 * Hatch properties
 * @param hatch
 */
void LC_QuickInfoEntityData::collectHatchProperties(RS_Hatch *hatch){
   entityName = tr("HATCH");
    const RS_HatchData &data = hatch->getData();
    bool solid = data.solid;
    addProperty(tr("Solid"), solid ? tr("Yes") : tr("No"), OTHER);
    if (!solid){
        double scale = data.scale;
        double angle = data.angle;
        QString pattern = data.pattern;

        addProperty(tr("Pattern"), pattern, OTHER);
        addProperty(tr("Scale"), formatDouble(scale), OTHER);
        addAngleProperty(tr("Angle"), angle);
        addAreaProperty(tr("Total Area"), hatch->getTotalArea());
    }
}
/**
 * Dim leader properties
 * @param leader
 */
void LC_QuickInfoEntityData::collectDimLeaderProperties(RS_Leader *leader){
    entityName = tr("DIMLEADER");
    const RS_LeaderData &data = leader->getData();
    addProperty(tr("Arrow Head"), data.arrowHead ? tr("Yes") : tr("No"), OTHER);
}

/**
 * Dim arc properties
 * @param dimarc
 */
void LC_QuickInfoEntityData::collectDimArcProperties(LC_DimArc *dimarc){
    entityName = tr("DIMARC");
    const LC_DimArcData &data = dimarc->getData();

    addLinearProperty(tr("Radius"), data.radius);
    addLinearProperty(tr("Arc Length"), data.arcLength);
    addVectorProperty(tr("Center"), data.centre);

    addAngleProperty(tr("Start Angle"), dimarc->getStartAngle());
    addAngleProperty(tr("End Angle"), dimarc->getEndAngle());

//    todo - potentially, for dimensions we can also could show variables - yet they are defined by settings for
//    todo drawing, so this rather will be overkill?
//    addLinearProperty("Arrow Size",dimarc->getArrowSize());
}

/**
 * Dimangular properties
 * @param dimang
 */
void LC_QuickInfoEntityData::collectDimAngularProperties([[maybe_unused]]RS_DimAngular *dimang){
    entityName = tr("DIMANGULAR");
//    const RS_DimensionData &data = dimang->getData();
//    todo - is it actually necessary to show more info here?
}

void LC_QuickInfoEntityData::collectDimDiametricProperties([[maybe_unused]]RS_DimDiametric *dimdia){
    entityName = tr("DIMDIAMETRIC");
    addVectorProperty(tr("Definition Point"), dimdia->getDefinitionPoint());
//    addLinearProperty("Leader", dimdia->getLeader());
}

/**
 * Dim radial properties
 * @param dimrad
 */
void LC_QuickInfoEntityData::collectDimRadialProperties(RS_DimRadial *dimrad){
    entityName = tr("DIMRADIAL");
    addVectorProperty(tr("Definition Point"), dimrad->getDefinitionPoint());
//    addLinearProperty("Leader", dimrad->getLeader());
}

/**
 * Dim linear properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimLinearProperties(RS_DimLinear *dim){
    entityName = tr("DIMLINEAR");
    addVectorProperty(tr("Extension Point 1"), dim->getExtensionPoint1());
    addVectorProperty(tr("Extension Point 2"), dim->getExtensionPoint2());
    addAngleProperty(tr("Angle"), dim->getAngle());
    addAngleProperty(tr("Oblique"), dim->getOblique());
}

/**
 * Dim Aligned properties
 * @param dim
 */
void LC_QuickInfoEntityData::collectDimAlignedProperties(RS_DimAligned *dim){
    entityName = tr("DIMALIGNED");
//    addAngleProperty("Angle", dim->getAngle());
    addVectorProperty(tr("Extension Point 1"), dim->getExtensionPoint1());
    addVectorProperty(tr("Extension Point 2"), dim->getExtensionPoint2());
}

/**
 * Utility method for adding vector property (with recalculation of coordinate, if needed)
 * @param name
 * @param value
 * @param type
 */

void LC_QuickInfoEntityData::addVectorProperty(QString name, const RS_Vector &value, PropertyType type){
    RS_Vector viewValue;
    RS_Vector relZero = graphicView->getRelativeZero();
    if (coordinatesMode == COORD_RELATIVE && relZero.valid){
        viewValue = value - relZero;
    } else {
        viewValue = value;
    }
    addVectorProperty(name, formatVector(viewValue), value, type);
}

/**
 * Utility method for adding vector property, where property's name is followed by index
 * @param name
 * @param count
 * @param value
 * @param type
 */
void LC_QuickInfoEntityData::addVectorProperty(QString name, int count, const RS_Vector &value, PropertyType type){
    RS_Vector viewValue;
    RS_Vector relZero = graphicView->getRelativeZero();
    if (coordinatesMode == COORD_RELATIVE && relZero.valid){
        viewValue = value - relZero;
    } else {
        viewValue = value;
    }
    addVectorProperty(name, count,formatVector(viewValue), value, type);
}

/**
 * Adding angle property
 * @param name
 * @param value
 */
void LC_QuickInfoEntityData::addAngleProperty(QString name, double value){
    addDoubleProperty(name, formatAngle(value), value, ANGLE);
}

/**
 * adding Linear property
 * @param name
 * @param value
 * @param type
 */
void LC_QuickInfoEntityData::addLinearProperty(QString name, double value, PropertyType type){
    addDoubleProperty(name, formatLinear(value), value, type);
}

/**
 * Adding area property
 * @param name
 * @param value
 */
void LC_QuickInfoEntityData::addAreaProperty(QString name, double value){
    addDoubleProperty(name, formatLinear(value), value, AREA);
}

/**
 * Return vector for property with given index
 * @param index
 * @return
 */
RS_Vector LC_QuickInfoEntityData::getVectorForIndex(int index) const{
    auto result = RS_Vector(false);
    int size = properties.size();
    if (index < size){
        auto *property = (VectorPropertyInfo *) properties.at(index);
        result = property->data;
    }
    return result;
}

/**
 * Returns value for given property's index
 * @param index
 * @return
 */
QString LC_QuickInfoEntityData::getValue(int index){
    auto *property = properties.at(index);
    QString result = property->value;
    return result;
}

void LC_QuickInfoEntityData::addVectorProperty(const QString name, const QString &valueStr, const RS_Vector &coord, PropertyType type){
    auto *prop = new VectorPropertyInfo(name, valueStr, type, coord);
    properties << prop;
}

void LC_QuickInfoEntityData::addVectorProperty(QString name, const int count, const QString &valueStr, const RS_Vector &coord, PropertyType type){
    QString idx;
    idx.setNum(count);
    auto *prop = new VectorPropertyInfo(name.append(" ").append(idx), valueStr, type, coord);
    properties << prop;
}

void LC_QuickInfoEntityData::addDoubleProperty(const QString name, const QString &valueStr, double value, PropertyType type){
    auto *prop = new DoublePropertyInfo(name, valueStr, type, value);
    properties << prop;
}

void LC_QuickInfoEntityData::addProperty(const QString name, const QString &valueStr, PropertyType type){
    auto *prop = new PropertyInfo(name, valueStr, type);
    properties << prop;
}

/**
 * cleanup
 */
void LC_QuickInfoEntityData::clear(){
    qDeleteAll(properties.begin(), properties.end());
    properties.clear();
    entityId = 0;
    entityName = "";
}

/**
 * Formatting double value
 * @param x
 * @return
 */
QString LC_QuickInfoEntityData::formatDouble(const double &x) const{
    RS_Graphic* graphic = document->getGraphic();
    QString result =  RS_Units::formatDecimal(x, RS2::Unit::None, graphic->getLinearPrecision(), false);
    return result;
}

/**
 * formatting int value
 * @param x
 * @return
 */
QString LC_QuickInfoEntityData::formatInt(const int &x) const{
    QString result;
    result.setNum(x);
    return result;
}

void LC_QuickInfoEntityData::setOptions(LC_QuickInfoOptions *opt){
    options = opt;
}

LC_QuickInfoEntityData::PropertyInfo::PropertyInfo(const QString &label, const QString &value, int type):label(label), value(value), type(type){}

LC_QuickInfoEntityData::VectorPropertyInfo::VectorPropertyInfo(const QString &label, const QString &value, int type, const RS_Vector &coord)
    :PropertyInfo(label, value, type),
     data(coord){}

LC_QuickInfoEntityData::DoublePropertyInfo::DoublePropertyInfo(const QString &label, const QString &value, int type, double d):PropertyInfo(label, value, type),
                                                                                                                               data(d){}
