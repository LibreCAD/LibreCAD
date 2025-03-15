/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
#include<cmath>

#include<QPainterPath>
#include<QPolygon>

#include "dxf_format.h"
#include "rs_color.h"
#include "rs_spline.h"
#include "rs_polyline.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_linetypepattern.h"
#include "rs_arc.h"
#include "rs_ellipse.h"
#include "rs_circle.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_splinepoints.h"
#include "rs_information.h"


namespace {
// Convert from LibreCAD line style pattern to QPen Dash Pattern.
// QPen dash pattern by default is in the unit of pixel
    QVector<qreal> rsToQDashPattern(const RS2::LineType &t, double screenWidth, double dpmm, double &newDashOffset) {
        // dash pattern is in mm
        // d*dpmm/screenWidth, so, the scaling factor k = dpmm/screenWidth
        dpmm = std::max(dpmm, 1e-6);
        double k = dpmm / std::max(screenWidth, 1.);

        const std::vector<double> &pattern = RS_LineTypePattern::getPattern(t)->pattern;
        QVector<qreal> dashPattern;
        std::transform(pattern.cbegin(), pattern.cend(), std::back_inserter(dashPattern), [k](double d) {
            return std::max(k * std::abs(d), 1.);
        });
        dashPattern.resize(dashPattern.size() - dashPattern.size() % 2);

        newDashOffset = newDashOffset * k;
        return dashPattern;
    }

/**
 * Wrapper for Qt
 * convert RS2::LineType to Qt::PenStyle
 */
    Qt::PenStyle rsToQtLineType(const RS2::LineType &t) {
        switch (t) {
            case RS2::NoPen:
                return Qt::NoPen;
            case RS2::SolidLine:
            case RS2::LineByLayer:
            case RS2::LineByBlock:
                return Qt::SolidLine;
            default:
                return Qt::CustomDashLine;
        }
    }
}
/**
 * Constructor.
 */
// RVT_PORT changed from RS_PainterQt::RS_PainterQt( const QPaintDevice* pd)
RS_Painter::RS_Painter( QPaintDevice* pd)
    : QPainter{pd}, lastUsedPen() {
    drawingMode = RS2::ModeFull;
    drawSelectedEntities=false;
    cachedDpmm = getDpmm();
}

/**
 * Draws a grid point at (x1, y1).
 */
void RS_Painter::drawGridPoint(const RS_Vector& p) {
    QPainter::drawPoint(QPointF(p.x, p.y));
}

void RS_Painter::drawGridPoint(const double &x, const double &y) {
    QPainter::drawPoint(QPointF(x, y));
}

void RS_Painter::drawPointEntityWCS(const RS_Vector& wcsPos) {
    double uiX, uiY;
    toGui(wcsPos, uiX, uiY);
    drawPointEntityUI(uiX, uiY, pointsMode, screenPointsSize);
}

void RS_Painter::drawRefPointEntityWCS(const RS_Vector &wcsPos, int pdMode, double pdSize){
    // fixme - sand - may we cache size of refPoints? It's hardly possible that they will have different size during the same point run...
    int screenPDSize = determinePointScreenSize(pdSize);
    double uiX, uiY;
    toGui(wcsPos, uiX, uiY);
    drawPointEntityUI(uiX, uiY,  pdMode, screenPDSize);
}

/**
 * Draws a point at (x1, y1).
 */
void RS_Painter::drawPointEntityUI(double uiX, double uiY, int pdmode, int pdsize) {
    int halfPDSize = pdsize/2;

/*	PDMODE values =>
 bits 0-3 = 0, centre dot
          = 1, centre blank
          = 2, centre +
          = 3, centre X
          = 4, centre vertical tick
 bit 5 = 1 => added surrounding circle
 bit 6 = 1 => added surrounding square
*/
    switch (DXF_FORMAT_PDMode_getCentre(pdmode)) {
        case DXF_FORMAT_PDMode_CentreDot:
        default: {
            /*	Centre dot - use a tiny + to make it visible  */
            QPainter::drawLine(uiX - 1, uiY, uiX + 1, uiY);
            QPainter::drawLine(uiX, uiY - 1, uiX, uiY + 1);
            break;
        }
        case DXF_FORMAT_PDMode_CentreBlank: {
            /*	Centre is blank  */
            break ;
        }
        case DXF_FORMAT_PDMode_CentrePlus: {
            /*	Centre +  */
            QPainter::drawLine(uiX - pdsize, uiY, uiX + pdsize, uiY);
            QPainter::drawLine(uiX, uiY - pdsize, uiX, uiY + pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreCross: {
            /*	Centre X  */
            QPainter::drawLine(uiX - pdsize, uiY - pdsize, uiX + pdsize, uiY + pdsize);
            QPainter::drawLine(uiX + pdsize, uiY - pdsize, uiX - pdsize, uiY + pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreTick: {
            /*	Centre vertical tick  */
            QPainter::drawLine(uiX, uiY - halfPDSize, uiX, uiY);
            break;
        }
    }

/*	Surrounding circle if required  */
    if (DXF_FORMAT_PDMode_hasEncloseCircle(pdmode)) {
        /*	Approximate circle by an octagon  */
        int xMin = uiX - halfPDSize;
        int xMax = uiX + halfPDSize;
        int yMin = uiY - halfPDSize;
        int yMax = uiY + halfPDSize;
        int octOffset = halfPDSize * 0.71;
        int xOctMin = uiX - octOffset;
        int xOctMax = uiX + octOffset;
        int yOctMin = uiY - octOffset;
        int yOctMax = uiY + octOffset;

        QPainter::drawLine(uiX, yMin, xOctMax, yOctMin);
        QPainter::drawLine(uiX, yMin, xOctMin, yOctMin);
        QPainter::drawLine(uiX, yMax, xOctMax, yOctMax);
        QPainter::drawLine(uiX, yMax, xOctMin, yOctMax);

        QPainter::drawLine(xMin, uiY, xOctMin, yOctMin);
        QPainter::drawLine(xMin, uiY, xOctMin, yOctMax);
        QPainter::drawLine(xMax, uiY, xOctMax, yOctMin);
        QPainter::drawLine(xMax, uiY, xOctMax, yOctMax);
    }

/*	Surrounding square if required  */
    if (DXF_FORMAT_PDMode_hasEncloseSquare(pdmode)) {
        int xMin = uiX - halfPDSize;
        int xMax = uiX + halfPDSize;
        int yMin = uiY - halfPDSize;
        int yMax = uiY + halfPDSize;

        QPainter::drawLine(xMin, yMin, xMax, yMin);
        QPainter::drawLine(xMin, yMax, xMax, yMax);
        QPainter::drawLine(xMin, yMin, xMin, yMax);
        QPainter::drawLine(xMax, yMin, xMax, yMax);
    }
}

void RS_Painter::drawSolidWCS(const RS_Vector &wcsP0, const RS_Vector &wcsP1, const RS_Vector &wcsP2, const RS_Vector &wcsP3) {

    double uiX0, uiX1, uiX2, uiY0, uiY1, uiY2;

    toGui(wcsP0, uiX0, uiY0);
    toGui(wcsP1, uiX1, uiY1);
    toGui(wcsP2, uiX2, uiY2);

    fillTriangleUI(uiX0, uiY0,uiX1, uiY1, uiX2, uiY2);
    if (wcsP3.valid) {
        double uiX3, uiY3;
        toGui(wcsP3, uiX3, uiY3);
        fillTriangleUI(uiX0,uiY0, uiX1, uiY1, uiX3, uiY3);
    }
}


void RS_Painter::drawLineWCS(const RS_Vector& wcsP1, const RS_Vector& wcsP2){
    double uiX1, uiY1, uiX2, uiY2;
    toGui(wcsP1, uiX1, uiY1);
    toGui(wcsP2, uiX2, uiY2);
    drawLineUI(uiX1, uiY1, uiX2, uiY2);
}

/**
 * Draws a line from (x1, y1) to (x2, y2).
 */
void RS_Painter::drawLineUISimple(const RS_Vector& p1, const RS_Vector& p2){
    QPainter::drawLine(QPointF(p1.x, p1.y),QPointF(p2.x, p2.y));
}

void RS_Painter::drawLineUISimple(const double &x1, const double &y1, const double &x2, const double &y2){
    QPainter::drawLine(QPointF(x1, y1),QPointF(x2, y2));
}

void RS_Painter::drawLineUI(const double &x1, const double &y1, const double &x2, const double &y2){
    if(QPointF(x2-x1, y2-y1).manhattanLength() > minLineDrawingLen) {
        QPainter::drawLine(QPointF(x1, y1),QPointF(x2, y2));
    }
    else{
        QPainter::drawPoint(QPointF(x1, y1));
    }
}

#define DEBUG_ARC_RENDERING_NO


void RS_Painter::drawEntityArc(RS_Arc* arc) {
    QPainterPath path;
    drawArcEntity(arc, path);
    QPainter::drawPath(path);
}

void RS_Painter::drawEntityCircle(RS_Circle *circle) {
    const RS_CircleData &data = circle->getData();
    double uiRadiusX = toGuiDX(data.radius);
    double uiCenterX, uiCenterY;
    toGui(data.center, uiCenterX, uiCenterY);
    if (uiRadiusX < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(uiCenterX, uiCenterY));
    }
    else if (circleRenderSameAsArcs &&  arcRenderInterpolate) {
        QPainterPath path;
        drawArcInterpolatedByLines(uiCenterX, uiCenterY, uiRadiusX, 0, 360, path);
        QPainter::drawPath(path);
    }
    else if (uiRadiusX <= getMaximumArcNonErrorRadius()){ // draw arc using QT
        double uiRadiusY = toGuiDY(data.radius);
        QPainter::drawEllipse(QPointF(uiCenterX, uiCenterY), uiRadiusX, uiRadiusY);
    }
    else {
        // Issue #2035, avoid rendering error by rendering arcs as quadratic splines
        RS_Arc arc{nullptr, {data.center, data.radius, 0., 2.*M_PI, false}};
        clearDashOffset();
        drawEntityArc(&arc);
    }
}

void RS_Painter::drawArcEntity(RS_Arc* arc, QPainterPath &path){
    RS_ArcData data = arc->getData();
    RS_Vector &center = data.center;

    // convert to UI coordinates
    double uiCenterX, uiCenterY;
    toGui(center, uiCenterX, uiCenterY);
    double uiRadiusX = toGuiDX(data.radius);
    double uiRadiusY = toGuiDY(data.radius);

    if(uiRadiusX<=minArcDrawingRadius) { // draw just a point
        QPainter::drawPoint(QPointF(uiCenterX, uiCenterY));
    }
    else if (arcRenderInterpolate){ // draw arc interpolated by lines
        drawArcInterpolatedByLines(uiCenterX, uiCenterY, uiRadiusX, toUCSAngleDegrees(data.startAngleDegrees), data.angularLength, path);
    }
    else {
        // same as
        // if (radiusGui * RS_Painter::getMaximumArcSplineError() <= 1.) {
        // yet faster
        if (uiRadiusX <= getMaximumArcNonErrorRadius()){ // draw arc using QT
            drawArcQT(uiCenterX, uiCenterY, uiRadiusX, uiRadiusY, toUCSAngleDegrees(data.startAngleDegrees), data.angularLength, path);
        }
        else { // draw arc by visible segments, interpolation by splines
            bool visualArcIsVisible = isFullyWithinBoundingRect(arc); // just visual part is within view
            if (visualArcIsVisible) {
                updateDashOffset(arc);
                double arcAngleLength = arc->getAngleLength();
                if (arc->isReversed()) {
                    arcAngleLength = -arcAngleLength;
                }
                drawArcSegmentBySplinePointsUI(uiCenterX, uiCenterY, uiRadiusX, toUCSAngle(data.angle1), arcAngleLength, path);
            } else {
                updateDashOffset(arc);

                const RS_Vector &endpoint = arc->getEndpoint();
                const RS_Vector &startpoint = arc->getStartpoint();
                bool reversed = arc->isReversed();
                RS_Vector vpStart(reversed ? endpoint : startpoint);
                RS_Vector vpEnd(reversed ? startpoint : endpoint);

                const LC_Rect &wcsBoundingBox = getWcsBoundingRect();
                QPolygonF visualBox(QRectF(wcsBoundingBox.minP().x, wcsBoundingBox.minP().y, wcsBoundingBox.maxP().x - wcsBoundingBox.minP().x,
                                           wcsBoundingBox.maxP().y - wcsBoundingBox.minP().y));
                std::vector<RS_Vector> vertex(0);
                for (unsigned short i = 0; i < 4; i++) {
                    const QPointF &vp(visualBox.at(i));
                    vertex.push_back(RS_Vector(vp.x(), vp.y()));
                }
                /** angles at cross points */
                std::vector<double> crossPoints(0);

                double baseAngle = reversed ? arc->getAngle2() : arc->getAngle1();
                for (unsigned short i = 0; i < 4; i++) {
                    RS_Line line{vertex.at(i), vertex.at((i + 1) % 4)};
                    auto vpIts = RS_Information::getIntersection(static_cast<RS_Entity *>(arc), &line, true);
                    if (vpIts.size() == 0) {
                        continue;
                    }
                    for (const RS_Vector &vp: vpIts) {
                        auto ap1 = arc->getTangentDirection(vp).angle();
                        auto ap2 = line.getTangentDirection(vp).angle();
                        //ignore tangent points, because the arc doesn't cross over
                        if (fabs(remainder(ap2 - ap1, M_PI)) < RS_TOLERANCE_ANGLE) {
                            continue;
                        }
                        crossPoints.push_back(RS_Math::getAngleDifference(baseAngle, center.angleTo(vp)));
                    }
                }
                if (vpStart.isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                    crossPoints.push_back(0.);
                }
                if (vpEnd.isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                    crossPoints.push_back(arc->getAngleLength());
                }

                //sorting
                std::sort(crossPoints.begin(), crossPoints.end());
                //draw visible
                RS_Arc arcSegment(*arc);
                arcSegment.setReversed(false);

                for (size_t i = 1; i < crossPoints.size(); ++i) {
//                for (size_t i = 1; i < crossPoints.size(); i += 2) {
                    arcSegment.setAngle1(baseAngle + crossPoints[i - 1]);
                    arcSegment.setAngle2(baseAngle + crossPoints[i]);

                    // fixme - sand - it seems this check is redundant if i is increased by 2
                    // fixme - sand - so it seems checking that segment is visible via middle point applies an additional check and performance overhead?
                    arcSegment.updateMiddlePoint();
                    if (arcSegment.getMiddlePoint().isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                        drawArcSegmentBySplinePointsUI(uiCenterX, uiCenterY, uiRadiusX, toUCSAngle(arcSegment.getAngle1()), arcSegment.getAngleLength(), path);
                    }

#ifdef DEBUG_ARC_RENDERING
                    arcSegment.calculateBorders();
                    double uiCenterX, uiCenterY;
                    toGui(arcSegment.getStartpoint(), uiCenterX, uiCenterY);
                    drawCircleUI(uiCenterX, uiCenterY, 20);
                    toGui(arcSegment.getEndpoint(), uiCenterX, uiCenterY);
                    drawCircleUI(uiCenterX, uiCenterY, 20);
#endif
                }
            }
        }
    }
}

#define STRAIGHT_ARC_INTERPOLATION_NO

void RS_Painter::drawArcSegmentBySplinePointsUI(
    double uiCenterX, double uiCenterY, double uiRadiusX, double startAngleRad, double angularLengthRad, QPainterPath &path) {
// Issue #2035
// Estimate the rendering error by using a quadratic bezier to render an arc. The bezier
// curve(lc_splinepoints) is defined by a set of equidistant arc points
// Second order error of bezier approximation:
// r sin^4(dA/2)/2
// with the radius r, and dA as the line segment spanning angle around the arc center
// for maximum error up to 1 pixel: 1 > r sin^4(dA/2)/2,s
// dA < 2 (2/r)^{1/4}
// The number of points needed is by angularLength/dA
    const double dA = 2. * pow(2./uiRadiusX, 1./4.);
    int arcPoints = int(ceil(std::abs(angularLengthRad) / dA));
    // At minimum control points: 4
    arcPoints = std::max(2, arcPoints);
    std::vector<RS_Vector> uiPoints;

    double deltaAngleRad = angularLengthRad / arcPoints;

    LC_SplinePointsData data = LC_SplinePointsData();

#ifdef STRAIGHT_ARC_INTERPOLATION
    double angle = startAngleRad;
//    double angle2 = startAngleRad + angularLengthRad;
//    RS_Vector uiCenter(uiCenterX, uiCenterY);
    for (int i = 0; i <= arcPoints; ++i) {
        // more precise as no sum of rounding error - yet for small amount of points, it's not important, so use faster approach.
        //  const double angle = (startAngleRad * i  + angle2 * (arcPoints - i))/arcPoints;
        //  angle = startAngleRad + deltaAngleRad * i;

        double currentCos = cos(angle);
        double currentSin = sin(angle);
        // fit point is on the arc
        double uiX = uiCenterX + currentCos * uiRadiusX;
        double uiY = uiCenterY - currentSin * uiRadiusX;
        const RS_Vector fitPoint = RS_Vector(uiX, uiY);
        data.splinePoints.push_back(fitPoint);
        // faster
        angle = angle + deltaAngleRad;
#ifdef DEBUG_ARC_RENDERING
        // draw fit point
        drawPointEntityUI(uiX, uiY, 3, 15);
#endif
    }
#else
    double cosStart = cos(startAngleRad);
    double sinStart = sin(startAngleRad);

    double cosDelta = cos(deltaAngleRad);
    double sinDelta = sin(deltaAngleRad);

    double cosCurrent = cosStart;
    double sinCurrent = sinStart;
    for (int i = 0; i <= arcPoints; ++i) {
        // fit point is on the arc
        double uiX = uiCenterX + cosCurrent * uiRadiusX;
        double uiY = uiCenterY - sinCurrent * uiRadiusX;
        const RS_Vector fitPoint = RS_Vector(uiX, uiY);
        data.splinePoints.push_back(fitPoint);

        double tmp = cosCurrent * cosDelta - sinCurrent * sinDelta;
        sinCurrent = sinCurrent * cosDelta + cosCurrent * sinDelta;
        cosCurrent = tmp;

#ifdef DEBUG_ARC_RENDERING
        // draw fit point
        drawPointEntityUI(uiX, uiY, 3, 15);
#endif
    }
#endif

    LC_SplinePoints splinePoints(nullptr, data);
    drawArcSplinePointsUI(splinePoints.getData().controlPoints, path);
}

void RS_Painter::drawArcSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, QPainterPath &path) {
    size_t n = uiControlPoints.size();
    if(n < 2)
        return;

    RS_Vector vStart = uiControlPoints.front();
    RS_Vector vEnd(false);

    path.moveTo(QPointF(vStart.x, vStart.y));
//    QPainterPath qPath(QPointF(vStart.x, vStart.y));
#ifdef DEBUG_ARC_RENDERING
    drawPointEntityUI(vStart.x, vStart.y, 2, 15);
#endif
    const RS_Vector &cp1 = uiControlPoints[1];
    if(n < 3) {
        path.lineTo(QPointF(cp1.x, cp1.y));
    }
    else {
        const RS_Vector &cp2 = uiControlPoints[2];
        if (n < 4) {
            path.quadTo(QPointF(cp1.x, cp1.y), QPointF(cp2.x, cp2.y));
        }
        else {
            vEnd = (cp1 + cp2) / 2.0;
            path.quadTo(QPointF(cp1.x, cp1.y), QPointF(vEnd.x, vEnd.y));

            for (size_t i = 2; i < n - 2; i++) {
                const RS_Vector &cpi = uiControlPoints[i];
                vEnd = (cpi + uiControlPoints[i + 1]) / 2.0;
                path.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
#ifdef DEBUG_ARC_RENDERING
                drawPointEntityUI(cpi.x, cpi.y, 2, 15);
                drawPointEntityUI(vEnd.x, vEnd.y, 4, 15);
#endif
            }

            path.quadTo(QPointF(uiControlPoints[n - 2].x, uiControlPoints[n - 2].y), QPointF(uiControlPoints[n - 1].x, uiControlPoints[n - 1].y));
#ifdef DEBUG_ARC_RENDERING
            drawPointEntityUI(cp1.x, cp1.y, 2, 15);
            drawPointEntityUI(cp2.x, cp2.y, 2, 15);
            drawPointEntityUI(uiControlPoints[n - 2].x, uiControlPoints[n - 2].y, 2, 15);
            drawPointEntityUI(uiControlPoints[n - 1].x, uiControlPoints[n - 1   ].y, 2, 15);
#endif
        }
    }
}


void RS_Painter::drawArcQT(
    double uiCenterX, double uiCenterY, double uiRadiusX, double uiRadiusY, double uiStartAngleDegrees, double angularLength, QPainterPath &path) {// this is faster and QT-rendering native. However, it delivers rendering artefacts on large zooms/arcs sizes
// at the endpoints of the arcs due to internal interpolations.
// For some cases it's acceptable, however, so lets user's preference decide
    double rx = uiCenterX - uiRadiusX;
    double ry = uiCenterY - uiRadiusY;
    double dX = uiRadiusX + uiRadiusX;
    double dY = uiRadiusY + uiRadiusY;
    path.arcMoveTo(rx, ry, dX, dY, uiStartAngleDegrees);
    path.arcTo(rx, ry, dX, dY, uiStartAngleDegrees, angularLength);
}

void RS_Painter::drawArcInterpolatedByLines(double uiCenterX, double uiCenterY, double uiRadiusX, double uiStartAngleDegrees,
                                            double angularLength, QPainterPath &path) const {
    // draw arc interpolated by a set of line segments.
    // This is more precise drawing for arc's endpoints, yet in general slower(?) by performance.
    // Also, with too high allowed tolerance, arcs may be drawn not smoothly.

    double angularLengthRad = RS_Math::deg2rad(angularLength);
    // actually, this is not only tolerance, but also arc's height (sagitta, https://en.wikipedia.org/wiki/Sagitta_(geometry))
    // sagitta will represent max distance between true arc and line chord that is used for interpolation
    // so, based on expected sagitta we'll calculate the angle for single line interpolation segment

    int stepsCount = 0;
    if (arcRenderInterpolationAngleFixed){
        // this is fixes amount of steps - based on line segment angle
        stepsCount = int(angularLengthRad / arcRenderInterpolationAngleValue) + 2;
    }
    else {
        double lineSegmentAngle = 2 * acos(1 - arcRenderInterpolationMaxSagitta / uiRadiusX);
        double stepsTolerance = angularLengthRad / lineSegmentAngle;
        stepsCount = int(ceil(stepsTolerance)) + 2;
    }
//        LC_ERR << "ARC steps: " << stepsTol <<  " " << steps << " len " << angularLength << " start " << uiStartAngleDegrees;
    double uiStartAngleRad = RS_Math::deg2rad(uiStartAngleDegrees);

    double deltaAngleRad = std::abs(angularLengthRad) / stepsCount;
    if (angularLength < 0) {
        deltaAngleRad = -deltaAngleRad;
    }

    double cosStart = cos(uiStartAngleRad);
    double sinStart = sin(uiStartAngleRad);

    double uiX = uiCenterX + cosStart * uiRadiusX;
    double uiY = uiCenterY - sinStart * uiRadiusX;

    path.moveTo(QPointF(uiX, uiY));

#ifdef STRAIGHT_ARC_INTERPOLATION
    for (int i = 1; i <= stepsCount; ++i) {
        double a = uiStartAngleRad + deltaAngleRad * i;
        double currentCos = std::cos(a);
        double currentSin = std::sin(a);
        double uiX = uiCenterX + currentCos * uiRadiusX;
        double uiY = uiCenterY - currentSin * uiRadiusX;
        path.lineTo(QPointF(uiX, uiY));
    }
#else
    double cosDelta = cos(deltaAngleRad);
    double sinDelta = sin(deltaAngleRad);

    double cosCurrent = cosStart;
    double sinCurrent = sinStart;

    double remainingAngle = angularLengthRad;
    for (int i = 1; i <= stepsCount; ++i) {
        // here we avoid computation of sin and cos on each approximation step
        // the approach is described, for example, here https://stackoverflow.com/a/6669751 and "Angle sum and difference identities"
        double uiX = uiCenterX + cosCurrent * uiRadiusX;
        double uiY = uiCenterY - sinCurrent * uiRadiusX;
        double tmp = cosCurrent * cosDelta - sinCurrent * sinDelta;
        sinCurrent = sinCurrent * cosDelta + cosCurrent * sinDelta;
        cosCurrent = tmp;

        remainingAngle -= deltaAngleRad;
        path.lineTo(QPointF(uiX, uiY));
    }
    // complete interpolation - to the end point of the arc
    cosDelta = cos(remainingAngle);
    sinDelta = sin(remainingAngle);

    double tmp = cosCurrent * cosDelta - sinCurrent * sinDelta;
    sinCurrent = sinCurrent * cosDelta + cosCurrent * sinDelta;
    cosCurrent = tmp;

    double uiEndpointX = uiCenterX + cosCurrent * uiRadiusX;
    double uiEndpointY = uiCenterY - sinCurrent * uiRadiusX;

    path.lineTo(QPointF(uiEndpointX, uiEndpointY));
#endif
}

/**
 * Draws a circle.
 * @param cp Center point
 * @param radius Radius
 */
void RS_Painter::drawCircleWCS(const RS_Vector& wcsCenter, double radius){
    double uiCenterX, uiCenterY;
    toGui(wcsCenter, uiCenterX, uiCenterY);
    double uiRadius = toGuiDX(radius);
    drawCircleUI(uiCenterX, uiCenterY, uiRadius);
}

void RS_Painter::drawCircleUI(double uiCenterX, double uiCenterY, double uiRadius){
    if (uiRadius < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(uiCenterX, uiCenterY));
    }
    else {
        if (circleRenderSameAsArcs) {
            if (arcRenderInterpolate){
                QPainterPath path;
                drawArcInterpolatedByLines(uiCenterX, uiCenterY, uiRadius, 0, 360, path);
                QPainter::drawPath(path);
            }
            else {
                QPainter::drawEllipse(QPointF(uiCenterX, uiCenterY), uiRadius, uiRadius);
            }
        }
        else{
            QPainter::drawEllipse(QPointF(uiCenterX, uiCenterY), uiRadius, uiRadius);
        }
    }
}

void RS_Painter::drawEllipseWCS(const RS_Vector& wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees) {
    double uiMajorRadius = toGuiDX(wcsMajorRadius);
    double uiMinorRadius = ratio * uiMajorRadius;

    double uiCenterX;
    double uiCenterY;
    toGui(wcsCenter, uiCenterX, uiCenterY);
    double uiAngleDegrees = toUCSAngleDegrees(wcsAngleDegrees);
    drawEllipseUI(uiCenterX, uiCenterY, uiMajorRadius, uiMinorRadius, uiAngleDegrees);
}

void RS_Painter::drawEllipseUI(double uiCenterX, double uiCenterY, double uiRadiusMajor, double uiRadiusMinor, double uiAngleDegrees) {
    if (uiRadiusMajor < minEllipseMajorRadius){
        QPainter::drawPoint(QPointF(uiCenterX, uiCenterY));
    }
    else if (uiRadiusMinor < minEllipseMinorRadius) {//ellipse too small
        QTransform t1;
        t1.translate(uiCenterX, uiCenterY);
        t1.rotate(-uiAngleDegrees);
        t1.translate(-uiCenterX, -uiCenterY);
        save();
        setTransform(t1, false);
        QPainter::drawLine(QPointF(uiCenterX - uiRadiusMajor, uiCenterY), QPointF(uiCenterX + uiRadiusMajor, uiCenterY));
        restore();
    }
    else {
        QTransform t1;
        t1.translate(uiCenterX, uiCenterY);
        t1.rotate(-uiAngleDegrees);
        t1.translate(-uiCenterX, -uiCenterY);
        save();
        setTransform(t1, false);
        QPainter::drawEllipse(QRectF(uiCenterX - uiRadiusMajor, uiCenterY - uiRadiusMinor, uiRadiusMajor + uiRadiusMajor, uiRadiusMinor + uiRadiusMinor));
        restore();
    }
}

void RS_Painter::drawEllipseArcWCS(const RS_Vector& wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees,
                                   double angle1Degrees, double angle2Degrees, double angularLength, bool reversed) {
    double uiMajorRadius = toGuiDX(wcsMajorRadius);
    double uiMinorRadius = ratio * uiMajorRadius;

    double uiCenterX;
    double uiCenterY;
    toGui(wcsCenter, uiCenterX, uiCenterY);
    double uiAngleDegrees = toUCSAngleDegrees(wcsAngleDegrees);
    drawEllipseArcUI(uiCenterX, uiCenterY, uiMajorRadius, uiMinorRadius, uiAngleDegrees, angle1Degrees, angle2Degrees, angularLength, reversed);
}

void RS_Painter::drawEllipseArcUI(double uiCenterX, double uiCenterY, double uiMajorRadius, double uiMinorRadius, double uiMajorAngleDegrees,
                                   double angle1Degrees, double angle2Degrees, double angularLength, bool reversed) {
    if (uiMajorRadius < minEllipseMajorRadius){
        QPainter::drawPoint(QPointF(uiCenterX, uiCenterY));
    }
    else if (uiMinorRadius < minEllipseMinorRadius) {//ellipse too small
        QTransform t1;
        t1.translate(uiCenterX, uiCenterY);
        t1.rotate(-uiMajorAngleDegrees);
        t1.translate(-uiCenterX, -uiCenterY);
        save();
        setTransform(t1, false);
        QPainter::drawLine(QPointF(uiCenterX - uiMajorRadius, uiCenterY), QPointF(uiCenterX + uiMajorRadius, uiCenterY));
        restore();
    }
    else {
        QTransform t1;
        t1.translate(uiCenterX, uiCenterY);
        t1.rotate(-uiMajorAngleDegrees);
        t1.translate(-uiCenterX, -uiCenterY);
        save();
        setTransform(t1, false);
        double rx = uiCenterX - uiMajorRadius;
        double ry = uiCenterY - uiMinorRadius;
        double size1 = uiMajorRadius + uiMajorRadius;
        double size2 = uiMinorRadius + uiMinorRadius;
        if (reversed){
            angle1Degrees = angle2Degrees - 360;
            angularLength = -angularLength;
        }
        else{

        }
        QPainterPath path;
        path.arcMoveTo(rx, ry, size1, size2, angle1Degrees);
        path.arcTo(rx, ry, size1, size2, angle1Degrees, angularLength);
        QPainter::drawPath(path);
        restore();
    }
}


void RS_Painter::createSolidFillPath(QPainterPath &path,   QList<RS_Entity *> entities)  {
        double centerX, centerY, startX, startY, endX, endY;
        foreach (auto l, entities) {
            if (l->rtti()==RS2::EntityContainer) {
                auto* loop = (RS_EntityContainer*)l;
                QPainterPath loopPath;
                // edges:
//                LC_ERR << "loop------------------------------- " << loop->count();
                for(auto e: *loop){
                    switch (e->rtti()) {
                        case RS2::EntityLine: {
                            toGui(e->getStartpoint(), startX,startY);
                            QPoint pt1(RS_Math::round(startX),RS_Math::round(startY));
                            toGui(e->getEndpoint(), endX,endY);
                            QPoint pt2(RS_Math::round(endX),RS_Math::round(endY));

                            const QPointF &currentPosition = loopPath.currentPosition();
//                            LC_ERR << "CP " << currentPosition.x() << " " << currentPosition.y();
                            if (loopPath.isEmpty() || (currentPosition - pt1).manhattanLength() >= 1){
                                loopPath.moveTo(pt1);
                            }
                            loopPath.lineTo(pt2);
//                            LC_ERR << "Pt1 " << pt1.x() << "  " << pt1.y();
//                            LC_ERR << "Added " << pt2.x() << "  " << pt2.y();
                            break;
                        }
                        case RS2::EntityArc: {
                            auto* arc=static_cast<RS_Arc*>(e);
                            const RS_ArcData &arcData = arc->getData();
                            double radius = toGuiDX(arcData.radius);
                            // can't skip due to minimal radius, it will lead to filling errors
//                        if (radius > view->getMinArcDrawingRadius()) {
                            const RS_Vector &cp = arcData.center;
                            double cpx, cpy;
                            toGui(cp, cpx, cpy);
                            double rx = cpx - radius;
                            double ry = cpy - radius;
                            double size = radius + radius;
                            double startAngleDegrees, angularLength;
                            if (arcData.reversed) {
                                startAngleDegrees = arcData.otherAngleDegrees;
                                startAngleDegrees = startAngleDegrees - 360;
                                angularLength = -arcData.angularLength;
                            } else {
                                startAngleDegrees = arcData.startAngleDegrees;
                                angularLength = arcData.angularLength;
                            }
                            startAngleDegrees = toUCSAngleDegrees(startAngleDegrees);
                            if (loopPath.isEmpty()) {
                                loopPath.arcMoveTo(rx, ry, size, size, startAngleDegrees);
                            }
                            loopPath.arcTo(rx, ry, size, size, startAngleDegrees, angularLength);
//                        }
                            break;
                        }
                        case RS2::EntityCircle: {
                            auto* circle = static_cast<RS_Circle*>(e);
                            toGui(circle->getCenter(),centerX, centerY);
                            double r=toGuiDX(circle->getRadius());
                            path.addEllipse(QPointF(centerX,centerY),r,r);
                            break;
                        }
                        case RS2::EntityEllipse: {
                            auto ellipse = static_cast<RS_Ellipse *>(e);
                            const RS_EllipseData &ellipseData = ellipse->getData();

                            toGui(ellipseData.center, centerX, centerY);
                            double angle = toUCSAngleDegrees(ellipseData.angleDegrees);
                            double radius1 = toGuiDX(ellipse->getMajorRadius());
                            double radius2 = ellipseData.ratio*radius1;

                            QTransform t1;
                            t1.translate(centerX, centerY);
                            t1.rotate(-angle);
                            t1.translate(-centerX, -centerY);

                            double rx = centerX - radius1;
                            double ry = centerY - radius2;
                            double size1 = radius1 + radius1;
                            double size2 = radius2 + radius2;
                            if (ellipse->isEllipticArc()) {
                                double angle1 = ellipseData.startAngleDegrees;
                                double angle2 = ellipseData.otherAngleDegrees;
                                double angularLength = ellipseData.angularLength;
                                if (ellipseData.reversed){
                                    angle1 = angle2 - 360;
                                    angularLength = -angularLength;
                                }

                                QPainterPath arcPath;
                                angle1 = toUCSAngleDegrees(angle1);
                                arcPath.arcMoveTo(rx, ry, size1, size2, angle1);
                                arcPath.arcTo(rx, ry, size1, size2, angle1, angularLength);
                                arcPath = t1.map(arcPath);
                                loopPath.addPath(arcPath);
                                /*

                                if (pa.size() && pa2.size() && (pa.last() - pa2.first()).manhattanLength() < 1)
                                    pa2.remove(0, 1);
                                pa << pa2;*/
                            } else {
                                QPainterPath ellipsePath;
                                ellipsePath.addEllipse(QRectF(rx, ry, size1, size2));
                                ellipsePath = t1.map(ellipsePath);
                                loopPath.addPath(ellipsePath);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                path.addPath(loopPath);
            }
        }
}

void RS_Painter::debugOutPath(const QPainterPath &tmpPath) const {
    int c = tmpPath.elementCount();
    for (int i = 0; i < c; i++){
        const QPainterPath::Element &element = tmpPath.elementAt(i);
        LC_ERR << "i " << i << "("<< element.x << "," << element.y <<  ") Line To " << element.isLineTo() << " Move To: " << element.isMoveTo() << " Is Curve:" << element.isCurveTo();
    }
}

void RS_Painter::drawSplinePointsWCS(const 	std::vector<RS_Vector> &wcsControlPoints, bool closed){
    int controlPointsCount = wcsControlPoints.size()/*data.controlPoints.size()*/;
    // fixme - sand - render - eliminate creation of intermediate vector...
    std::vector<RS_Vector> uiControlPoints = std::vector<RS_Vector>(wcsControlPoints);
    for (int i = 0; i < controlPointsCount; i++){
        double x, y;
        toGui(wcsControlPoints[i], x,y);
        uiControlPoints[i] = RS_Vector(x,y);
    }
    drawSplinePointsUI(uiControlPoints, closed);
}

#define DEBUG_RENDER_SPLINEPOINTS_NO

void RS_Painter::drawSplinePointsUI(const std::vector<RS_Vector> &uiControlPoints, bool closed){
    size_t n = uiControlPoints.size();
    if(n < 2)
        return;

    RS_Vector vStart = uiControlPoints.front();
    RS_Vector vControl(false), vEnd(false);

    QPainterPath qPath(QPointF(vStart.x, vStart.y));
#ifdef DEBUG_RENDER_SPLINEPOINTS
    drawPointEntityUI(vStart.x, vStart.y, 2, 15);
#endif

    if(closed){
        if(n < 3){
            qPath.lineTo(QPointF(uiControlPoints[1].x, uiControlPoints[1].y));
        }
        else {
            const RS_Vector &cp0 = uiControlPoints[0];
            const RS_Vector &cpNMinus1 = uiControlPoints[n - 1];
            vStart = (cpNMinus1 + cp0) / 2.0;
            qPath.moveTo(QPointF(vStart.x, vStart.y));

            vEnd = (cp0 + uiControlPoints[1]) / 2.0;
            qPath.quadTo(QPointF(cp0.x, cp0.y), QPointF(vEnd.x, vEnd.y));

            for (size_t i = 1; i < n - 1; i++) {
                const RS_Vector &cpi = uiControlPoints[i];
                vEnd = (cpi + uiControlPoints[i + 1]) / 2.0;
                qPath.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
            }
            qPath.quadTo(QPointF(cpNMinus1.x, cpNMinus1.y), QPointF(vStart.x, vStart.y));
        }
    }
    else {
        const RS_Vector &cp1 = uiControlPoints[1];
        if(n < 3) {
            qPath.lineTo(QPointF(cp1.x, cp1.y));
        }
        else {
            const RS_Vector &cp2 = uiControlPoints[2];
            if (n < 4) {
                qPath.quadTo(QPointF(cp1.x, cp1.y), QPointF(cp2.x, cp2.y));
            }
            else {
                vEnd = (cp1 + cp2) / 2.0;
                qPath.quadTo(QPointF(cp1.x, cp1.y), QPointF(vEnd.x, vEnd.y));

                for (size_t i = 2; i < n - 2; i++) {
                    const RS_Vector &cpi = uiControlPoints[i];
                    vEnd = (cpi + uiControlPoints[i + 1]) / 2.0;
                    qPath.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
#ifdef DEBUG_RENDER_SPLINEPOINTS
                    drawPointEntityUI(cpi.x, cpi.y, 2, 15);
                    drawPointEntityUI(vEnd.x, vEnd.y, 4, 15);
#endif
                }

                qPath.quadTo(QPointF(uiControlPoints[n - 2].x, uiControlPoints[n - 2].y), QPointF(uiControlPoints[n - 1].x, uiControlPoints[n - 1].y));
#ifdef DEBUG_RENDER_SPLINEPOINTS
                drawPointEntityUI(cp1.x, cp1.y, 2, 15);
                drawPointEntityUI(cp2.x, cp2.y, 2, 15);
                drawPointEntityUI(uiControlPoints[n - 2].x, uiControlPoints[n - 2].y, 2, 15);
                drawPointEntityUI(uiControlPoints[n - 1].x, uiControlPoints[n - 1   ].y, 2, 15);
#endif
            }
        }
    }
    QPainter::drawPath(qPath);
}

void RS_Painter::drawEntityPolyline(const RS_Polyline* polyline){
    QPainterPath path;
    double startX, startY, endX, endY;
    toGui(polyline->getStartpoint(), startX, startY);
    path.moveTo(startX, startY);

    for(RS_Entity* entity: *polyline) {
        switch(entity->rtti()) {
            case RS2::EntityLine: {
                toGui(entity->getStartpoint(), startX, startY);
                path.moveTo(startX, startY);
                toGui(entity->getEndpoint(), endX, endY);
                path.lineTo(endX, endY);
                break;
            }
            case RS2::EntityArc: {
                auto arc = *static_cast<RS_Arc *>(entity);
                drawArcEntity(&arc, path);
                break;
            }
            // well, actually this is just for fonts.. better to have separate entity for this. fixme - change latter
            case RS2::EntityEllipse: { // fixme - coordinates translation

                // !! FIXME - sand - why not the same path of the polyline is used??
                auto arc = *static_cast<RS_Ellipse *>(entity);
                const RS_EllipseData& data = arc.getData();
                double uiCenterX, uiCenterY;
                toGui(data.center, uiCenterX, uiCenterY);

                const double uiMajorRadius = toGuiDX(data.majorP.magnitude()); // fixme - sand - render - cache?
                const double uiMinorRadius = data.ratio * uiMajorRadius;
                if (data.isArc) {
                    drawEllipseArcUI(uiCenterX, uiCenterY, uiMajorRadius, uiMinorRadius, toWorldAngleDegrees(data.angleDegrees), /*view.toWorldAngleDegrees(*/data.startAngleDegrees/*)*/,
                                   /*view.toWorldAngleDegrees(*/data.otherAngleDegrees/*)*/, data.angularLength, data.reversed);
                }
                else {
                    drawEllipseUI(uiCenterX, uiCenterY, uiMajorRadius, uiMinorRadius, toWorldAngleDegrees(data.angleDegrees));
                }
                break;
            }
            default:
                LC_ERR<<"Polyline may contain lines/arcs only: found rtti() ="<<entity->rtti();
        }
    }
    QPainter::drawPath(path);
}

void RS_Painter::drawSplineWCS(const RS_Spline& spline){
    QPainterPath path;
    unsigned int count = spline.count();
    if (count > 0) {
        RS_Entity *child = spline.unsafeEntityAt(0);
        double uiX, uiY;
        toGui(child->getStartpoint(), uiX, uiY);
        path.moveTo(uiX, uiY);
        for (unsigned int i = 0; i < count;i++) {
            child = spline.unsafeEntityAt(i);
            toGui(child->getEndpoint(), uiX, uiY);
            path.lineTo(uiX, uiY);
        }
    }

    QPainter::drawPath(path);
}

void RS_Painter::drawImgWCS(QImage& img, const RS_Vector& wcsInsertionPoint,
                           const RS_Vector& uVector, const RS_Vector& vVector) {

//    if (viewport->hasUCS()) {
    double wcsAngle = uVector.angle();
    double ucsAngle = toUCSAngle(wcsAngle);

    auto ucsUVector = uVector;
    auto ucsVVector = vVector;

    auto angleVector = RS_Vector(ucsAngle - wcsAngle);

    ucsUVector.rotate(angleVector);
    ucsVVector.rotate(angleVector);
//    }

    double magnitudeU = uVector.magnitude(); // fixme - sand - render - cache?
    double magnitudeV = vVector.magnitude(); // fixme - sand - render - cache?
    RS_Vector scale{toGuiDX(magnitudeU),toGuiDY(magnitudeV)};
    double uiInsertX, uiInsertY;
    toGui(wcsInsertionPoint, uiInsertX, uiInsertY);
    drawImgUI(img,uiInsertX, uiInsertY,ucsUVector, ucsVVector, scale);
}

void RS_Painter::drawImgUI(QImage& img, double uiInsertX, double uiInsertY,
                           const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor) {
    save();

//    LC_ERR << "IMG FACTOR " << factor;
    // Render smooth only at close zooms
    // fixme - sand - check later - actually, these two hints are equivalent!
    if (factor.x < 1 || factor.y < 1) {
        RS_Painter::setRenderHint(SmoothPixmapTransform , true);
    }
    else {
        RS_Painter::setRenderHint(SmoothPixmapTransform);
    }

    RS_Vector un = uVector/uVector.magnitude();
    RS_Vector vn = vVector/vVector.magnitude();

    // Image mirroring is switching the handedness of u-v vectors pair which can be detected by
    // looking at the sign of the z component of their cross product. If z is negative image is mirrored.
    std::unique_ptr<QTransform> wm;
    if(RS_Vector::crossP(uVector, vVector).z < 0) { // mirrored
        wm.reset(new QTransform(un.x, -vn.x, -un.y, vn.y, uiInsertX, uiInsertY));
    } else {
        wm.reset( new QTransform(un.x, vn.x, un.y, vn.y, uiInsertX, uiInsertY));
    }

    wm->scale(factor.x, factor.y);
    setWorldTransform(*wm);

    drawImage(0,-img.height(), img);

    restore();
}

void RS_Painter::drawTextH(int x1, int y1,
                             int x2, int y2,
                             const QString& text) {
    QPainter::drawText(x1, y1, x2, y2,Qt::AlignRight|Qt::AlignVCenter,text);
}

void RS_Painter::drawTextV(int x1, int y1,
                             int x2, int y2,
                             const QString& text) {
    save();
    QTransform wm = worldTransform();
    wm.rotate(-90.0);
    setWorldTransform(wm);

    QPainter::drawText(x1, y1, x2, y2,Qt::AlignRight|Qt::AlignVCenter,text);
    restore();
}

void RS_Painter::fillRect(int x1, int y1, int w, int h,
                            const RS_Color& col) {
    QPainter::fillRect(x1, y1, w, h, col);
}

void RS_Painter::fillTriangleUI(
    const RS_Vector &uiP1,
    const RS_Vector &uiP2,
    const RS_Vector &uiP3) {
    QPolygonF arr;
    QBrush brushSaved = brush();
    arr.append({uiP1.x, uiP1.y});
    arr.append({uiP2.x, uiP2.y});
    arr.append({uiP3.x, uiP3.y});
    setBrushColor(RS_Color(pen().color()));
    QPainter::drawPolygon(arr, Qt::OddEvenFill);
    QPainter::setBrush(brushSaved);
}

void RS_Painter::fillTriangleUI(double uiX1, double uiY1, double uiX2, double uiY2, double uiX3, double uiY3) {
    QPolygonF arr;
    QBrush brushSaved = brush();
    arr.append({uiX1, uiY1});
    arr.append({uiX2, uiY2});
    arr.append({uiX3, uiY3});
    setBrushColor(RS_Color(pen().color()));
    QPainter::drawPolygon(arr, Qt::OddEvenFill);
    QPainter::setBrush(brushSaved);
}


void RS_Painter::erase() {
    QPainter::eraseRect(0,0,getWidth(),getHeight());
}

int RS_Painter::getWidth() const{
    return device()->width();
}

/** get Density per millimeter on screen/print device
  *@return density per millimeter in pixel/mm
  */
double RS_Painter::getDpmm() const{
    int mm(device()->widthMM());
    if(mm==0) mm=400;
    return double(device()->width())/mm;
}

int RS_Painter::getHeight() const{
    return device()->height();
}


RS_Pen RS_Painter::getPen() const{
    return lpen;
}

namespace {
    const RS_Color colorBlack = RS_Color(Qt::black);
    const RS_Color colorWhite = RS_Color(Qt::white);
    const QColor qcolorBlack = colorBlack.toQColor();
    const QColor qcolorWhite = colorWhite.toQColor();
}

void RS_Painter::noCapStyle(){
    QPen pen = QPainter::pen();
    pen.setCapStyle(Qt::PenCapStyle::FlatCap);
    QPainter::setPen(pen);
}

void RS_Painter::setPen(const RS_Pen& pen) {
    lpen = pen;
    QColor pColor;
    switch (drawingMode) {
        case RS2::ModeBW:
            pColor = qcolorBlack;
            break;

        case RS2::ModeWB:
            pColor = qcolorWhite;
            break;

        default:
            pColor = pen.getColor().toQColor();
            break;
    }

    pColor.setAlphaF(pen.getAlpha());
    RS2::LineType lineType = pen.getLineType();
    Qt::PenStyle style = rsToQtLineType(lineType);

    double screenWidth = pen.getScreenWidth();
    if (style == Qt::CustomDashLine){
        double newDashOffset = pen.dashOffset();
        auto dashPattern = rsToQDashPattern(lineType,
                                            screenWidth/*p.widthF()*/,
                                            getDpmmCached(),
                                            newDashOffset);
        if (dashPattern.isEmpty()) {
            style = Qt::SolidLine;
        } else {
            QPen p(pColor, screenWidth, style);
            p.setDashPattern(std::move(dashPattern));
            // fixme - how this is related to RS_AtomicEntity::updateDashOffset??? Will we set dash offset twice?
            p.setDashOffset(newDashOffset);
            p.setJoinStyle(penJoinStyle);
            p.setCapStyle(penCapStyle);
            lastUsedPen = p;
            QPainter::setPen(p);
            return;
        }
    }
    // processing solid line

    bool changed = false;
    if (lastUsedPen.color() != pColor){
        lastUsedPen.setColor(pColor);
        changed = true;
    }
    if (lastUsedPen.widthF() != screenWidth){
        lastUsedPen.setWidthF(screenWidth);
        changed = true;
    }
    if (lastUsedPen.style() != style){
        lastUsedPen.setStyle(style);
        changed = true;
    }
    lastUsedPen.setJoinStyle(penJoinStyle);
    lastUsedPen.setCapStyle(penCapStyle);

    if (changed){
        QPainter::setPen(lastUsedPen);
    }
}

void RS_Painter::setPen(const RS_Color& color) {
    switch (drawingMode) {
        case RS2::ModeBW: {
            const RS_Color &color = RS_Color(Qt::black);
            lpen.setColor(color);
            QPainter::setPen(color);
            break;
        }
        case RS2::ModeWB: {
            const RS_Color &color = RS_Color(Qt::white);
            lpen.setColor(color);
            QPainter::setPen(color);
            break;
        }
        default:
            lpen.setColor( color);
            QPainter::setPen( color);
            break;
    }
}

void RS_Painter::setPen(int r, int g, int b) {
    switch (drawingMode) {
        case RS2::ModeBW: {
            RS_Color color = RS_Color(Qt::black);
            lpen.setColor(color);
            QPainter::setPen(color);
            break;
        }
        case RS2::ModeWB: {
            RS_Color color = RS_Color(Qt::white);
            lpen.setColor(color);
            QPainter::setPen(color);
            break;
        }
        default: {
            const RS_Color color = RS_Color(r, g, b);
            lpen.setColor(color);
            QPainter::setPen(color);
            break;
        }
    }
}

void RS_Painter::disablePen() {
    lpen = RS_Pen(RS2::FlagInvalid);
    QPainter::setPen(Qt::NoPen);
}


void RS_Painter::setBrushColor(const RS_Color& color) {
    switch (drawingMode) {
        case RS2::ModeBW:
            QPainter::setBrush( QColor( Qt::black));
            break;

        case RS2::ModeWB:
            QPainter::setBrush( QColor( Qt::white));
            break;

        default:
            QPainter::setBrush( color);
            break;
    }
}

void RS_Painter::fillPath ( const QPainterPath & path, const QBrush& brush){
    QPainter::fillPath(path, brush);
}
void RS_Painter::drawPath ( const QPainterPath & path ) {
    QPainter::drawPath(path);
}

void RS_Painter::setClipRect(int x, int y, int w, int h) {
    QPainter::setClipRect(x, y, w, h);
    setClipping(true);
}

void RS_Painter::resetClipping() {
    setClipping(false);
}

void RS_Painter::fillRect ( const QRectF & rectangle, const RS_Color & color ) {

    double x1=rectangle.left();
    double x2=rectangle.right();
    double y1=rectangle.top();
    double y2=rectangle.bottom();
    // fixme - review (width height semantics)
//        QPainter::fillRect(toScreenX(x1),toScreenY(y1),toScreenX(x2)-toScreenX(x1),toScreenY(y2)-toScreenX(y1), color);
    QPainter::fillRect(x1,y1,x2-x1,y2-y1, color);
}
void RS_Painter::fillRect ( const QRectF & rectangle, const QBrush & brush ) {
  /*  double x1=rectangle.left();
    double x2=rectangle.right();
    double y1=rectangle.top();
    double y2=rectangle.bottom();*/
    // fixme - review (width height semantics)
//        QPainter::fillRect(toScreenX(x1),toScreenY(y1),toScreenX(x2),toScreenY(y2), brush);
    QPainter::fillRect(rectangle, brush);
}

RS_Pen& RS_Painter::getRsPen(){
    return lpen;
}

void RS_Painter::drawText(const QRect& rect, int flags, const QString& text, QRect* boundingBox){
    QPainter::drawText(rect, flags, text, boundingBox);
}

void RS_Painter::drawText(const QRect& rect, const QString& text, QRect* boundingBox){
    QPainter::drawText(rect, Qt::AlignTop | Qt::AlignLeft | Qt::TextDontClip, text, boundingBox);
}

void RS_Painter::setPenJoinStyle(Qt::PenJoinStyle style){
    penJoinStyle = style;
}

void RS_Painter::setPenCapStyle(Qt::PenCapStyle style){
    penCapStyle = style;
}

void RS_Painter::setMinCircleDrawingRadius(double val) {
    minCircleDrawingRadius = val;
}

void RS_Painter::setMinArcDrawingRadius(double val) {
    minArcDrawingRadius = val;
}


void RS_Painter::setMinEllipseMajorRadius(double val) {
    minEllipseMajorRadius = val;
}

void RS_Painter::setMinEllipseMinorRadius(double val) {
    minEllipseMinorRadius = val;
}

void RS_Painter::setMinLineDrawingLen(double val) {
    minLineDrawingLen = val;
}

void RS_Painter::drawRectUI(const double uiX1, const double uiY1, const double uiX2, const double uiY2) {
    drawPolygon(QRect(int(uiX1 + 0.5), int(uiY1 + 0.5), int(uiX2 - uiX1 + 0.5), int(uiY2 - uiY1 + 0.5)));
}

void RS_Painter::drawRectUI(const RS_Vector& p1, const RS_Vector& p2) {
    drawPolygon(QRect(int(p1.x+0.5), int(p1.y+0.5), int(p2.x - p1.x+0.5), int(p2.y - p1.y+0.5)));
}

void RS_Painter::drawHandleWCS(const RS_Vector& wcsPos, const RS_Color& c, int size) {
    int doubleSize = 2 * size;
    double uiX, uiY;
    toGui(wcsPos, uiX, uiY);
    fillRect((int)(uiX - size), (int)(uiY - size), doubleSize, doubleSize, c);
}

void RS_Painter::setMinRenderableTextHeightInPx(int i) {
    minRenderableTextHeightInPx = i;
}

void RS_Painter::updateDashOffset(RS_Entity *e) {
    // Adjust dash offset
    if (lpen.getLineType() == RS2::SolidLine /*|| view.getGraphic() == nullptr*/)
        return;

    // factor from model space to GUI
    const double toMm = defaultWidthFactor;
    currenPatternOffset -= e->getLength() * toMm;
}

int RS_Painter::determinePointScreenSize(double pdsize) const{
    int screenPointSize;
    int deviceHeight = getHeight();
    if (pdsize == 0){
        screenPointSize = deviceHeight / 20;
    }
    else if (DXF_FORMAT_PDSize_isPercent(pdsize)){
        screenPointSize = (deviceHeight * DXF_FORMAT_PDSize_Percent(pdsize)) / 100;
    }
    else {
        screenPointSize = toGuiDY(pdsize);
    }
    return screenPointSize;
}

void RS_Painter::updatePointsScreenSize(double pdSize) {
    screenPointsSize = determinePointScreenSize(pdSize);
}

void RS_Painter::drawInfiniteWCS(RS_Vector startpoint, RS_Vector endpoint) {
    const LC_Rect viewportRect = renderer->getBoundingClipRect();
    RS_Vector start(false);

    double offsetX = toGuiDX(0.25); // todo - check why gui coordinates are used there -  while intersection is with WCS coordinates?
    double offsetY = toGuiDY(0.25);

    RS_Vector pLeft = LC_LineMath::getIntersectionInfiniteLineLineFast(startpoint, endpoint,
                                                                       viewportRect.minP(), RS_Vector(viewportRect.minP().x, viewportRect.maxP().y),
                                                                       offsetX, offsetY);
    if (pLeft.valid){
        start = pLeft;
    }
    RS_Vector pBottom = LC_LineMath::getIntersectionInfiniteLineLineFast(startpoint, endpoint,
                                                                         viewportRect.minP(), RS_Vector(viewportRect.maxP().x, viewportRect.minP().y),
                                                                         offsetX, offsetY);
    if (pBottom.valid){
        if (start.valid){
            drawLineWCS(start, pBottom);
            return;
        }
        else{
            start = pBottom;
        }
    }

    RS_Vector pRight = LC_LineMath::getIntersectionInfiniteLineLineFast(startpoint, endpoint,
                                                                        RS_Vector(viewportRect.maxP().x, viewportRect.minP().y),viewportRect.maxP(),
                                                                        offsetX, offsetY);
    if (pRight.valid){
        if (start.valid){
            drawLineWCS(start, pRight);
            return;
        }
        else {
            start = pRight;
        }
    }
    if (start.valid) {
        RS_Vector pTop = LC_LineMath::getIntersectionInfiniteLineLineFast(startpoint, endpoint,
                                                                          RS_Vector(viewportRect.minP().x, viewportRect.maxP().y), viewportRect.maxP(),
                                                                          offsetX, offsetY);
        if (pTop.valid){
            drawLineWCS(start, pTop);
        }
    }
}

bool RS_Painter::isTextLineNotRenderable(double wcsLineHeight) {
    double uiHeight = toGuiDY(wcsLineHeight);
    return renderer->isTextLineNotRenderable(uiHeight);
}

void RS_Painter::setViewPort(LC_GraphicViewport *v) {
    viewport = v;
    apply(viewport);
    RS_Vector factor = v->getFactor();
    viewPortFactorX = factor.x;
    viewPortFactorY = factor.y;
    viewPortOffsetX = v->getOffsetX();
    viewPortOffsetY = v->getOffsetY();
    viewPortHeight = v->getHeight();
}

// NOTE:
// ----------------------------------------------------------------------------------------------------------------
// The code below duplicates coordinates translations from Viewport/mapper. This is INTENTIONAL and is performed for the
// performance's sake, as coordinates translation is more than heavily used operation during the rendering pass.
// Painter is inherited from Coordinates Mapper also for increasing the speed of rendering.
//
// The major gain for performance is gained due to
//  1) one time calculation of sin/cos of xaxis angle
//  2) methods unwrapping/inlining
// ----------------------------------------------------------------------------------------------------------------

void RS_Painter::toGui(const RS_Vector &wcsCoordinate, double &uiX, double &uiY) {
//    viewport->toUI(pos, x,y);

    if (m_hasUcs){
//   ucsToUCS(wcsCoordinate.x, wcsCoordinate.y, uiX, uiY);
// the code below is equivalent to

/*
        RS_Vector wcs = RS_Vector(wcsCoordinate.x, wcsCoordinate.y);
        RS_Vector newPos = wcs-ucsOrigin;
        newPos.rotate(xAxisAngle);
        uiY = newPos.x;
        uiX = newPos.y;
*/
        double ucsPositionX = wcsCoordinate.x - ucsOrigin.x;
        double ucsPositionY = wcsCoordinate.y - ucsOrigin.y;

        double ucsX = ucsPositionX * cosXAngle - ucsPositionY * sinXAngle;
        double ucsY = ucsPositionX * sinXAngle + ucsPositionY * cosXAngle;

//        uiX = toGuiX(uiX);
        uiX = ucsX * viewPortFactorX + viewPortOffsetX;
//        uiY = toGuiY(uiY);
        uiY = -ucsY * viewPortFactorY + viewPortHeight - viewPortOffsetY;
    }
    else{
//        uiX = toGuiX(wcsCoordinate.x);
        uiX = wcsCoordinate.x * viewPortFactorX + viewPortOffsetX;
//        uiY = toGuiY(wcsCoordinate.y);
        uiY = -wcsCoordinate.y * viewPortFactorY + viewPortHeight - viewPortOffsetY;
    }
}

double RS_Painter::toGuiDX(double ucsDX) const {
//    return viewport->toGuiDX(d);
   return ucsDX * viewPortFactorX;
}

double RS_Painter::toGuiDY(double ucsDY) const {
//    return viewport->toGuiDY(d);
    return ucsDY * viewPortFactorY;
}

void RS_Painter::disableUCS(){
    m_hasUcs = false;
}

bool RS_Painter::isFullyWithinBoundingRect(RS_Entity* e){
    // we have checks LC_GraphicViewportRenderer::isOutsideOfBoundingClipRect(RS_Entity* e, bool constructionEntity)
    // this check we are not outside view rect. It ensures that max coordinate of entity is larger than min coordinate of viewport (same for min coordinate).
    // Thus, we can use a shorter check - instead checking for ranges, we check that max coordinate of viewport is less than max coordinate of view

    return e->getMax().x < wcsBoundingRect.maxP().x && e->getMin().x > wcsBoundingRect.minP().x &&
           e->getMax().y < wcsBoundingRect.maxP().y && e->getMin().y > wcsBoundingRect.minP().y;

}

bool RS_Painter::isFullyWithinBoundingRect(const LC_Rect &rect){
    return rect.maxP().x < wcsBoundingRect.maxP().x && rect.minP().x > wcsBoundingRect.minP().x &&
    rect.maxP().y < wcsBoundingRect.maxP().y && rect.minP().y > wcsBoundingRect.minP().y;
}

const LC_Rect &RS_Painter::getWcsBoundingRect() const {
    return wcsBoundingRect;
}
