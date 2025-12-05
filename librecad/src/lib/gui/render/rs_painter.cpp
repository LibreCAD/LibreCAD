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

#include "rs_painter.h"

#include <QPainterPath>

#include "dxf_format.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewportrenderer.h"
#include "lc_linemath.h"
#include "lc_splinepoints.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_linetypepattern.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_spline.h"

struct RS_EllipseData;
struct RS_CircleData;

namespace {

const RS_Color colorBlack = RS_Color(Qt::black);
const RS_Color colorWhite = RS_Color(Qt::white);
const QColor qcolorBlack = colorBlack.toQColor();
const QColor qcolorWhite = colorWhite.toQColor();

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

    // RAII style QPainter
    class PainterGuard {
        QPainter* m_painter = nullptr;
    public:
    PainterGuard(QPainter& painter):
            m_painter{&painter}
        {
        painter.save();
        }
    ~PainterGuard()
        {
        try{
                m_painter->restore();
            } catch(...) {
            // should not happen
        }
        }
    };

    // draw a plus sign by given halfSize
    void drawPlus(QPainter& painter, QPointF uiPos, int halfSize)
    {
        QPointF left{uiPos.x() - halfSize, uiPos.y()};
        QPointF right{uiPos.x() + halfSize, uiPos.y()};
        painter.drawLine(left, right);
        QPointF bottom{uiPos.x(), uiPos.y() - halfSize};
        QPointF top{uiPos.x(), uiPos.y() + halfSize};
        painter.drawLine(bottom, top);
    }
    // draw a plus sign by given halfSize
    void drawCross(QPainter& painter, QPointF uiPos, int halfSize)
    {
        auto half = QPoint(halfSize, halfSize).toPointF();
        QPointF left = uiPos - half;
        QPointF right = uiPos + half;
        painter.drawLine(left, right);
        left.setY(left.y() + 2 * halfSize);
        right.setY(right.y() - 2 * halfSize);
        painter.drawLine(left, right);
    }

    void drawOctagon(QPainter& painter, const QPointF& uiPos, int halfSize)
    {
        QPointF dr0(double(halfSize), std::sin(M_PI/8.) * halfSize);
        std::vector<QPointF> vertices{{dr0, dr0.transposed()}};
        // mirroring by y-axis
        for(int i = 1; i >= 0; --i)
            vertices.emplace_back( - vertices[i].x(), vertices[i].y());
        // mirroring by x-axis
        for(int i = 3; i >= 0; --i)
            vertices.emplace_back( vertices[i].x(), - vertices[i].y());

        QPolygonF octagon;
        std::transform(vertices.cbegin(), vertices.cend(), std::back_inserter(octagon), [&uiPos](const QPointF& vertex) { return vertex + uiPos;});
        PainterGuard guard(painter);
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(octagon);
    }

    void drawSquare(QPainter& painter, const QPointF& uiPos, int halfSize)
    {
        auto dr0 = QPoint(halfSize, halfSize).toPointF();
        auto dr1 = QPoint(- halfSize, halfSize).toPointF();
        QPolygonF square{{uiPos + dr0, uiPos + dr1, uiPos - dr0, uiPos - dr1}};
        PainterGuard guard(painter);
        painter.setBrush(Qt::NoBrush);
        painter.drawPolygon(square);
    }
}
/**
 * Constructor.
 */
// RVT_PORT changed from RS_PainterQt::RS_PainterQt( const QPaintDevice* pd)
RS_Painter::RS_Painter( QPaintDevice* pd)
    : QPainter{pd}
    , cachedDpmm{getDpmm()}{
}

/**
 * Draws a grid point at (x1, y1).
 */
void RS_Painter::drawGridPoint(const RS_Vector& p) {
    QPainter::drawPoint(QPointF(p.x, p.y));
}

void RS_Painter::drawGridPoint(double x, double y) {
    QPainter::drawPoint(QPointF(x, y));
}

void RS_Painter::drawPointEntityWCS(const RS_Vector& wcsPos) {
    RS_Vector uiPos = toGui(wcsPos);
    drawPointEntityUI(uiPos, pointsMode, screenPointsSize);
}

void RS_Painter::drawRefPointEntityWCS(const RS_Vector &wcsPos, int pdMode, double pdSize){
    // fixme - sand - may we cache size of refPoints? It's hardly possible that they will have different size during the same point run...
    int screenPDSize = determinePointScreenSize(pdSize);
    double uiX, uiY;
    toGui(wcsPos, uiX, uiY);
    drawPointEntityUI({uiX, uiY},  pdMode, screenPDSize);
}

/**
 * Draws a point at (x1, y1).
 */
void RS_Painter::drawPointEntityUI(const RS_Vector& uiPos, int pdmode, int pdsize) {
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
    QPointF uiCoords {uiPos.x, uiPos.y};
    switch (DXF_FORMAT_PDMode_getCentre(pdmode)) {
        case DXF_FORMAT_PDMode_CentreDot:
        default: {
            /*	Centre dot - use a tiny + to make it visible  */
            drawPlus(*this, uiCoords, 1);
            break;
        }
        case DXF_FORMAT_PDMode_CentreBlank: {
            /*	Centre is blank  */
            break ;
        }
        case DXF_FORMAT_PDMode_CentrePlus: {
            /*	Centre +  */
            drawPlus(*this, uiCoords, pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreCross: {
            /*	Centre X  */
            drawCross(*this, uiCoords, pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreTick: {
            /*	Centre vertical tick  */
            QPainter::drawLine(QPointF{uiPos.x, uiPos.y - halfPDSize}, uiCoords);
            break;
        }
    }

/*	Surrounding circle if required  */
    if (DXF_FORMAT_PDMode_hasEncloseCircle(pdmode)) {
        /*	Approximate circle by an octagon  */
        drawOctagon(*this, uiCoords, halfPDSize);
    }

/*	Surrounding square if required  */
    if (DXF_FORMAT_PDMode_hasEncloseSquare(pdmode)) {
        drawSquare(*this, uiCoords, halfPDSize);
    }
}

void RS_Painter::drawSolidWCS(const RS_VectorSolutions& wcsVertices) {
    QPolygonF uiPolygon;
    for (const RS_Vector& wcsVertex : wcsVertices) {
        if (wcsVertex.valid) {
            uiPolygon.push_back(toGuiPointF(wcsVertex));
        }
    }

    // For quadrilaterals from RS_Solid, the point order is switched for corner3 and corner4.
    if (uiPolygon.size() == 4) {
        std::swap(uiPolygon[2], uiPolygon.back());
    }
    fillPolygonUI(uiPolygon);
}

void RS_Painter::drawFilledPolygonWCS(const RS_Vector& wcsV1, const RS_Vector& wcsV2, const RS_Vector& wcsV3,
                                     const RS_Vector& wcsV4, const RS_Vector& wcsV5) {
    QPolygonF uiPolygon;
    uiPolygon.push_back(toGuiPointF(wcsV1));
    uiPolygon.push_back(toGuiPointF(wcsV2));
    uiPolygon.push_back(toGuiPointF(wcsV3));
    uiPolygon.push_back(toGuiPointF(wcsV4));
    if (wcsV5.valid) {
        uiPolygon.push_back(toGuiPointF(wcsV5));
    }
    fillPolygonUI(uiPolygon);
}

void RS_Painter::drawFilledCircleWCS(const RS_Vector& wcsCenter, double radius) {
    fillEllipseUI(toGuiPointF(wcsCenter), toGuiDX(radius), toGuiDY(radius));
}


void RS_Painter::drawPolygonWCS(const RS_Vector& wcsV1, const RS_Vector& wcsV2, const RS_Vector& wcsV3,
                                const RS_Vector& wcsV4, const RS_Vector& wcsV5) {
    QPolygonF uiPolygon;
    uiPolygon.push_back(toGuiPointF(wcsV1));
    uiPolygon.push_back(toGuiPointF(wcsV2));
    uiPolygon.push_back(toGuiPointF(wcsV3));
    uiPolygon.push_back(toGuiPointF(wcsV4));
    if (wcsV5.valid) {
        uiPolygon.push_back(toGuiPointF(wcsV5));
    }
    drawPolyline(uiPolygon);
}

void RS_Painter::drawPolygonWCS(const std::vector<RS_Vector> &wcsPoints) {
    QPolygonF uiPolygon;
    for (auto& wcsPoint : wcsPoints) {
        uiPolygon.push_back(toGuiPointF(wcsPoint));
    }
    drawPolyline(uiPolygon);
}

void RS_Painter::drawSolidWCS(const RS_Vector& wcsP0, const RS_Vector& wcsP1, const RS_Vector& wcsP2,
                              const RS_Vector& wcsP3) {
    drawSolidWCS({wcsP0, wcsP1, wcsP2});
    if (wcsP3.valid) {
        drawSolidWCS({wcsP1, wcsP3, wcsP2});
    }
}


void RS_Painter::drawLineWCS(const RS_Vector& wcsP1, const RS_Vector& wcsP2){
    drawLineUI(toGuiPointF(wcsP1), toGuiPointF(wcsP2));
}

void RS_Painter::drawLineWCSScaled(const RS_Vector& wcsP1, const RS_Vector& wcsP2, double lineWidthFactor){
    drawLineUIScaled(toGuiPointF(wcsP1), toGuiPointF(wcsP2), lineWidthFactor);
}

void RS_Painter::drawLineUIScaled(QPointF from, QPointF to, double lineWidthFactor) {
    const auto savedPen = pen();
    auto width = savedPen.widthF();
    auto newPen = savedPen;
    newPen.setWidthF(width*lineWidthFactor);
    QPainter::setPen(newPen);
    QPainter::drawLine(from,to);
    QPainter::setPen(savedPen);
}

/**
 * Draws a line from (x1, y1) to (x2, y2).
 */
void RS_Painter::drawLineUISimple(const RS_Vector& p1, const RS_Vector& p2){
    QPainter::drawLine(QPointF(p1.x, p1.y),QPointF(p2.x, p2.y));
}

void RS_Painter::drawLineUISimple(double x1, double y1, double x2, double y2){
    QPainter::drawLine(QPointF(x1, y1),QPointF(x2, y2));
}

void RS_Painter::drawLineUI(const QPointF& startPoint, const QPointF& endPoint)
{
    if((startPoint - endPoint).manhattanLength() > minLineDrawingLen) {
        QPainter::drawLine(startPoint, endPoint);
    }
    else{
        QPainter::drawPoint((startPoint + endPoint) * 0.5);
    }
}

void RS_Painter::drawLineUI(double x1, double y1, double x2, double y2)
{
    drawLineUI({x1, y1}, {x2, y2});
}

#define DEBUG_ARC_RENDERING_NO


void RS_Painter::drawEntityArc(RS_Arc* arc) {
    QPainterPath path;
    drawArcEntity(arc, path);
    QPainter::drawPath(path);
}

void RS_Painter::drawEntityCircle(RS_Circle *circle) {
    const RS_CircleData &data = circle->getData();
    const double uiRadiusX = toGuiDX(data.radius);
    const RS_Vector uiCenter = toGui(data.center);
    if (uiRadiusX < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(uiCenter.x, uiCenter.y));
    }
    else if (circleRenderSameAsArcs &&  arcRenderInterpolate) {
        QPainterPath path;
        drawArcInterpolatedByLines(uiCenter, uiRadiusX, 0., 360., path);
        QPainter::drawPath(path);
    }
    else if (uiRadiusX <= getMaximumArcNonErrorRadius()){ // draw arc using QT
        double uiRadiusY = toGuiDY(data.radius);
        QPainter::drawEllipse(QPointF(uiCenter.x, uiCenter.y), uiRadiusX, uiRadiusY);
    }
    else {
        // Issue #2035, avoid rendering error by rendering arcs as quadratic splines
        RS_Arc arc{nullptr, {data.center, data.radius, 0., 2.*M_PI, false}};
        clearDashOffset();
        drawEntityArc(&arc);
    }
}

void RS_Painter::drawArcEntity(RS_Arc* arc, QPainterPath &path){
    const double radius = arc->getRadius();
    const RS_Vector &center = arc->getCenter();

    // convert to UI coordinates
    RS_Vector uiCenter = toGui(center);
    RS_Vector uiRadii { toGuiDX(radius),  toGuiDY(radius)};

    if(uiRadii.x<=minArcDrawingRadius) { // draw just a point
        QPainter::drawPoint(QPointF{uiCenter.x, uiCenter.y});
    }
    else if (arcRenderInterpolate){ // draw arc interpolated by lines
        drawArcInterpolatedByLines(uiCenter, uiRadii.x, toUCSAngleDegrees(arc->getData().startAngleDegrees), arc->getData().angularLength, path);
    }
    else {
        // same as
        // if (radiusGui * RS_Painter::getMaximumArcSplineError() <= 1.) {
        // yet faster
        if (uiRadii.x <= getMaximumArcNonErrorRadius()){ // draw arc using QT
            drawArcQT(uiCenter, uiRadii, toUCSAngleDegrees(arc->getData().startAngleDegrees), arc->getData().angularLength, path);
        }
        else { // draw arc by visible segments, interpolation by splines
            bool visualArcIsVisible = isFullyWithinBoundingRect(arc); // just visual part is within view
            if (visualArcIsVisible) {
                updateDashOffset(arc);
                double arcAngleLength = arc->getAngleLength();
                if (arc->isReversed()) {
                    arcAngleLength = -arcAngleLength;
                }
                drawArcSegmentBySplinePointsUI(uiCenter, uiRadii.x, toUCSAngle(arc->getAngle1()), arcAngleLength, path);
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
                        if (std::abs(std::remainder(ap2 - ap1, M_PI)) < RS_TOLERANCE_ANGLE) {
                            continue;
                        }
                        crossPoints.push_back(RS_Math::getAngleDifference(baseAngle, center.angleTo(vp)));
                    }
                }
                // start/end points of the arc
                if (vpStart.isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                    crossPoints.push_back(0.);
                }
                if (vpEnd.isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                    crossPoints.push_back(arc->getAngleLength());
                }

                std::sort(crossPoints.begin(), crossPoints.end());
                //draw visible
                RS_Arc arcSegment(*arc);
                arcSegment.setReversed(false);

                // Cannot assume angles are all unique due to rounding error
                // Instead of relying on odd-even orders, check all segments instead
                for (size_t i = 1; i < crossPoints.size(); ++i) {
                    arcSegment.setAngle1(baseAngle + crossPoints[i - 1]);
                    arcSegment.setAngle2(baseAngle + crossPoints[i]);

                    // fixme - sand - so it seems checking that segment is visible via middle point applies an additional check and performance overhead?
                    arcSegment.updateMiddlePoint();
                    if (arcSegment.getMiddlePoint().isInWindowOrdered(wcsBoundingBox.minP(), wcsBoundingBox.maxP())) {
                        drawArcSegmentBySplinePointsUI(uiCenter, uiRadii.x, toUCSAngle(arcSegment.getAngle1()), arcSegment.getAngleLength(), path);
                    }

#ifdef DEBUG_ARC_RENDERING
                    arcSegment.calculateBorders();
                    RS_Vector uiCenter = toGui(arcSegment.getStartpoint());
                    drawCircleUI(uiCenter, 20);
                    uiCenter = toGui(arcSegment.getEndpoint());
                    drawCircleUI(uiCenter, 20);
#endif
                }
            }
        }
    }
}

#define STRAIGHT_ARC_INTERPOLATION_NO

void RS_Painter::drawArcSegmentBySplinePointsUI(
    const RS_Vector& uiCenter, double uiRadiusX, double startAngleRad, double angularLengthRad, QPainterPath &path) {
// Issue #2035
// Estimate the rendering error by using a quadratic bezier to render an arc. The bezier
// curve(lc_splinepoints) is defined by a set of equidistant arc points
// Second order error of bezier approximation:
// r sin^4(dA/2)/(1 + \cos dA)
// with the radius r, and dA as the line segment spanning angle around the arc center
// for maximum error up to 1 pixel: 1 > r sin^4(dA/2)/2,s
// dA < 2 (2/r)^{1/4}
// The number of points needed is by angularLength/dA
    const double dA = 2. * pow(2./uiRadiusX, 1./4.);
    int arcPoints = int(ceil(std::abs(angularLengthRad) / dA));
    // At minimum control points: 3
    arcPoints = std::max(2, arcPoints);

    const double deltaAngleRad = angularLengthRad / arcPoints;

#ifdef STRAIGHT_ARC_INTERPOLATION
    double angle = startAngleRad;
//    double angle2 = startAngleRad + angularLengthRad;
    for (int i = 0; i <= arcPoints; ++i) {
        // more precise as no sum of rounding error - yet for small amount of points, it's not important, so use faster approach.
        //  const double angle = (startAngleRad * i  + angle2 * (arcPoints - i))/arcPoints;
        //  angle = startAngleRad + deltaAngleRad * i;

        RS_Vector currentRotation{-angle};
        // fit point is on the arc
        RS_Vector fitPoint = uiCenter + currentRotation * uiRadiusX;
        data.splinePoints.push_back(fitPoint);
        // faster
        angle += deltaAngleRad;
#ifdef DEBUG_ARC_RENDERING
        // draw fit point
        drawPointEntityUI(uiX, uiY, 3, 15);
#endif
    }
#else

    LC_SplinePointsData data;

    // The QPainter y-axis is pointing downwards
    // TODO: get the rotation direction automatically, instead of hard-coded
    RS_Vector fromCenter = RS_Vector{-startAngleRad} * uiRadiusX;
    const RS_Vector rotationStep{-deltaAngleRad};

    for (int i = 0; i <= arcPoints; ++i) {
        // fit point is on the arc
        const RS_Vector arcPoint = uiCenter + fromCenter;
        data.splinePoints.push_back(arcPoint);
        fromCenter.rotate(rotationStep);
#ifdef DEBUG_ARC_RENDERING
        // draw fit point
        drawPointEntityUI(arcPoint.x, arcPoint.y, 3, 15);
#endif
    }
#endif

    // LC_SplinePoints will update control points from splinePoints by default
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


void RS_Painter::drawArcQT(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiStartAngleDegrees, double angularLength, QPainterPath &path) {
// at the endpoints of the arcs due to internal interpolations.
// For some cases it's acceptable, however, so lets user's preference decide
    RS_Vector minCorner = uiCenter - uiRadii;
    RS_Vector uiSize = uiRadii + uiRadii;
    path.arcMoveTo(minCorner.x, minCorner.y, uiSize.x, uiSize.y, uiStartAngleDegrees);
    path.arcTo(minCorner.x, minCorner.y, uiSize.x, uiSize.y, uiStartAngleDegrees, angularLength);
}

void RS_Painter::drawArcInterpolatedByLines(const RS_Vector& uiCenter, double uiRadiusX, double uiStartAngleDegrees,
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
        // acos(x) loses significant digits, if x is close to 0
        // instead do: 1 - cos(x) = 2 \sin^2(x/2)
        //double lineSegmentAngle = 2 * acos(1 - arcRenderInterpolationMaxSagitta / uiRadiusX);
        const double relativeError = 0.5 * std::abs(arcRenderInterpolationMaxSagitta) / uiRadiusX;
        // avoid domain error of std::asin() by requiring: lineSegmentAngle < Pi/2
        const double lineSegmentAngle = 4. * std::asin(std::min(relativeError, std::sin(M_PI/8.)));
        double stepsTolerance = std::abs(angularLengthRad) / lineSegmentAngle;
        stepsCount = int(ceil(stepsTolerance)) + 2;
    }
//        LC_ERR << "ARC steps: " << stepsTol <<  " " << steps << " len " << angularLength << " start " << uiStartAngleDegrees;
    double uiStartAngleRad = RS_Math::deg2rad(uiStartAngleDegrees);

    double deltaAngleRad = angularLengthRad / stepsCount;

    // TODO: handle ui angle orientation
    RS_Vector fromCenter = RS_Vector{-uiStartAngleRad} * uiRadiusX;
    RS_Vector uiPosition = uiCenter + fromCenter;

    path.moveTo(QPointF{uiPosition.x, uiPosition.y});

#ifdef STRAIGHT_ARC_INTERPOLATION
    for (int i = 1; i <= stepsCount; ++i) {
        double a = uiStartAngleRad + deltaAngleRad * i;
        RS_Vector uiLinePoint = uiCenter + RS_Vector{-a} * uiRadiusX;
        path.lineTo(QPointF(uiLinePoint.x, uiLinePoint.y));
    }
#else
    const RS_Vector deltaRotation{-deltaAngleRad};

    for (int i = 1; i <= stepsCount; ++i) {
        // here we avoid computation of sin and cos on each approximation step
        // the approach is described, for example, here https://stackoverflow.com/a/6669751 and "Angle sum and difference identities"
        fromCenter.rotate(deltaRotation);
        uiPosition = uiCenter + fromCenter;
        path.lineTo(QPointF(uiPosition.x, uiPosition.y));
    }
    // complete interpolation - to the end point of the arc
#endif
}

/**
 * Draws a circle.
 * @param cp Center point
 * @param radius Radius
 */
void RS_Painter::drawCircleWCS(const RS_Vector& wcsCenter, double radius){
    RS_Vector uiCenter = toGui(wcsCenter);
    double uiRadius = toGuiDX(radius);
    drawCircleUI(uiCenter, uiRadius);
}

void RS_Painter::drawCircleUI(const RS_Vector& uiCenter, double uiRadius){
    if (uiRadius < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(uiCenter.x, uiCenter.y));
    }
    else {
        if (circleRenderSameAsArcs) {
            if (arcRenderInterpolate){
                QPainterPath path;
                drawArcInterpolatedByLines(uiCenter, uiRadius, 0, 360, path);
                QPainter::drawPath(path);
            }
            else {
                QPainter::drawEllipse(QPointF(uiCenter.x, uiCenter.y), uiRadius, uiRadius);
            }
        }
        else{
            QPainter::drawEllipse(QPointF(uiCenter.x, uiCenter.y), uiRadius, uiRadius);
        }
    }
}

void RS_Painter::drawCircleUIDirect(const RS_Vector& uiPos, double uiRadius) {
    if (uiRadius < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(uiPos.x, uiPos.y));
    }
    else {
       QPainter::drawEllipse(QPointF(uiPos.x, uiPos.y), uiRadius, uiRadius);
    }
}

void RS_Painter::drawEllipseWCS(const RS_Vector& wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees) {
    double uiMajorRadius = toGuiDX(wcsMajorRadius);
    double uiMinorRadius = ratio * uiMajorRadius;

    RS_Vector uiCenter = toGui(wcsCenter);
    const double uiAngleDegrees = toUCSAngleDegrees(wcsAngleDegrees);
    drawEllipseUI(uiCenter, {uiMajorRadius, uiMinorRadius}, uiAngleDegrees);
}

void RS_Painter::drawEllipseUI(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiAngleDegrees) {

    if (uiRadii.x < minEllipseMajorRadius){
        // as we have everything there, no need to transform, and save/restore the painter context
        QPainter::drawPoint(QPointF(uiCenter.x, uiCenter.y));
    }
    else {
        // RAII style restoring painter status
        // TODO - remove the comment
        // Yes, such pattern is recommended for resources management.
        // Yet honestly speaking, I can't understand the PRACTICAL reason why RAII is better HERE rather than the direct save/restore.
        // Especially considering the shortest scope between save/restore and time to live of the guard ...
        // How it's possible to forget calling restore() there? Why restore() may be not called? Due to some exception between save/restore? but it's not handled anyway, it's just a crash.
        // Thus it just looks like an embellishment (and -1 code line) without a real value - yet with added overhead for short-living object allocation, creation and destruction.
        // ok, let it be - yet it it's hardly could be considered as improvement, I suppose.
        PainterGuard painterGuard{*this};
        // ellipse transform
        QTransform ellipseTransform;
        ellipseTransform.translate(uiCenter.x, uiCenter.y);
        ellipseTransform.rotate(-uiAngleDegrees);
        setTransform(ellipseTransform, true);

        QPointF radii{uiRadii.x, uiRadii.y};

        if (uiRadii.y < minEllipseMinorRadius) {//ellipse too small
            QPainter::drawLine( - radii, radii);
        } else {
            QPainter::drawEllipse(QRectF{- radii, radii});
        }
    }
}

void RS_Painter::drawEllipseArcWCS(const RS_Vector& wcsCenter, double wcsMajorRadius, double ratio, double wcsAngleDegrees,
                                   double angle1Degrees, double angle2Degrees, double angularLength, bool reversed) {
    double uiMajorRadius = toGuiDX(wcsMajorRadius);
    double uiMinorRadius = ratio * uiMajorRadius;

    const RS_Vector uiCenter = toGui(wcsCenter);
    double uiAngleDegrees = toUCSAngleDegrees(wcsAngleDegrees);
    drawEllipseArcUI(uiCenter, {uiMajorRadius, uiMinorRadius}, uiAngleDegrees, angle1Degrees, angle2Degrees, angularLength, reversed);
}

void RS_Painter::drawEllipseArcUI(const RS_Vector& uiCenter, const RS_Vector& uiRadii, double uiMajorAngleDegrees,
                                   double angle1Degrees,[[maybe_unused]] double angle2Degrees, double angularLength,[[maybe_unused]] bool reversed) {
    // TODO - it also should be refactored to be consistent with drawEllipseUI()
    if (std::max(uiRadii.x, uiRadii.y) < minEllipseMajorRadius){
        QPainter::drawPoint(QPointF(uiCenter.x, uiCenter.y));
        return;
    }

    PainterGuard guard(*this);
    QTransform t1;
    t1.translate(uiCenter.x, uiCenter.y);
    t1.rotate(-uiMajorAngleDegrees);
    setTransform(t1, true);

    if (uiRadii.y < minEllipseMinorRadius) {//ellipse too small
        QPainter::drawLine(QPointF(- uiRadii.x, 0.), QPointF(uiRadii.x, 0.));
    }
    else {

        const bool useSpline = std::max(uiRadii.x, uiRadii.y) > getMaximumArcNonErrorRadius();

        QPainterPath path;
        addEllipseArcToPath(path, uiRadii, angle1Degrees, angularLength, useSpline);
        QPainter::drawPath(path);
    }
}

void RS_Painter::addEllipseArcToPath(QPainterPath& localPath, const RS_Vector& uiRadii, double startAngleDeg, double angularLengthDeg, bool useSpline) {
    if (useSpline) {
        double startRad = RS_Math::deg2rad(toUCSAngleDegrees(startAngleDeg));
        double lenRad = RS_Math::deg2rad(toUCSAngleDegrees(angularLengthDeg));
        drawEllipseSegmentBySplinePointsUI(uiRadii, startRad, lenRad, localPath, false);
    } else {
        QRectF rect(-uiRadii.x, -uiRadii.y, 2 * uiRadii.x, 2 * uiRadii.y);
        localPath.arcMoveTo(rect, startAngleDeg);
        localPath.arcTo(rect, startAngleDeg, angularLengthDeg);
    }
}

void RS_Painter::drawEllipseSegmentBySplinePointsUI(const RS_Vector& uiRadii, double startRad, double lenRad, QPainterPath &path, bool closed)
{
    double r = std::max(uiRadii.x, uiRadii.y);
    // maximum angular step size: using this angular step size keeps the maximum
    // deviation of an arc from its parabola fitting
    const double dParam = std::pow(1./32. / r, 1. / 4.);
    int numSegments = std::max(1, int(std::ceil(std::abs(lenRad) / dParam)));
  // Avoid performance issue: too many points when zoomed in
  // The maximum rendering error is relaxed
    numSegments = std::min(24, numSegments);
    // Don't duplicate first point for closed
    int numPoints = closed ? numSegments : numSegments + 1;
    double delta = lenRad / numSegments;

    LC_SplinePointsData data;
    data.closed = closed;

    double param = startRad;

    const RS_Vector scaleXY{uiRadii.x, - uiRadii.y};
    for (int i = 0; i < numPoints; ++i) {
        data.splinePoints.push_back(RS_Vector{param}.scale(scaleXY));
        param += delta;
    }

    LC_SplinePoints spline(nullptr, data);
    addSplinePointsToPath(spline.getData().controlPoints, closed, path);
}


void RS_Painter::drawEllipseBySplinePointsUI(const RS_Ellipse& ellipse, QPainterPath &path)
{
  RS_Vector uiRadii{toGuiDX(ellipse.getMajorRadius()), toGuiDY(ellipse.getMinorRadius())};
  double r = std::max(uiRadii.x, uiRadii.y);
  // maximum angular step size: using this angular step size keeps the maximum
  // deviation of an arc from its parabola fitting
  const double dParam = std::pow(1./32. / r, 1. / 4.);
  double lenRad = ellipse.getAngleLength();
  int numSegments = std::max(1, int(std::ceil(std::abs(lenRad) / dParam)));
  // Avoid performance issue: too many points when zoomed in
  // The maximum rendering error is relaxed
  numSegments = std::min(numSegments, 24);
  // Don't duplicate first point for closed
  const bool closed = !ellipse.isEllipticArc();
  int numPoints = closed ? numSegments : numSegments + 1;
  double delta = lenRad / numSegments;

  LC_SplinePointsData data;
  data.closed = closed;

  double param = ellipse.isReversed() ? ellipse.getAngle1(): ellipse.getAngle2();
  RS_Vector rotation{- ellipse.getMajorP().angle()};
  RS_Vector uiCenter = toGui(ellipse.getCenter());

  const RS_Vector scaleXY{uiRadii.x, - uiRadii.y};
  for (int i = 0; i < numPoints; ++i) {
    data.splinePoints.push_back(RS_Vector{param}.scale(scaleXY).rotate(rotation).move(uiCenter));
    param += delta;
  }

  LC_SplinePoints spline(nullptr, data);
  addSplinePointsToPath(spline.getData().controlPoints, ellipse.isEllipticArc(), path);
}

void RS_Painter::addSplinePointsToPath(const std::vector<RS_Vector> &uiControlPoints, bool closed, QPainterPath &path) const
{
    size_t n = uiControlPoints.size();
    if (n < 2)
        return;

    RS_Vector vStart = uiControlPoints.front();
    RS_Vector vEnd(false);

    if (closed) {
        if (n < 3)
            return;
        const RS_Vector &cp0 = uiControlPoints[0];
        const RS_Vector &cpNMinus1 = uiControlPoints[n - 1];
        vStart = (cpNMinus1 + cp0) / 2.0;
        path.moveTo(QPointF(vStart.x, vStart.y));

        vEnd = (cp0 + uiControlPoints[1]) / 2.0;
        path.quadTo(QPointF(cp0.x, cp0.y), QPointF(vEnd.x, vEnd.y));

        for (size_t i = 1; i < n - 1; i++) {
            const RS_Vector &cpi = uiControlPoints[i];
            vEnd = (cpi + uiControlPoints[i + 1]) / 2.0;
            path.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
        }
        path.quadTo(QPointF(cpNMinus1.x, cpNMinus1.y), QPointF(vStart.x, vStart.y));
    } else {
        path.moveTo(QPointF(vStart.x, vStart.y));
        const RS_Vector &cp1 = uiControlPoints[1];
        if (n < 3) {
            path.lineTo(QPointF(cp1.x, cp1.y));
        } else {
            const RS_Vector &cp2 = uiControlPoints[2];
            if (n < 4) {
                path.quadTo(QPointF(cp1.x, cp1.y), QPointF(cp2.x, cp2.y));
            } else {
                vEnd = (cp1 + cp2) / 2.0;
                path.quadTo(QPointF(cp1.x, cp1.y), QPointF(vEnd.x, vEnd.y));

                for (size_t i = 2; i < n - 2; i++) {
                    const RS_Vector &cpi = uiControlPoints[i];
                    vEnd = (cpi + uiControlPoints[i + 1]) / 2.0;
                    path.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
                }

                path.quadTo(QPointF(uiControlPoints[n - 2].x, uiControlPoints[n - 2].y), QPointF(uiControlPoints[n - 1].x, uiControlPoints[n - 1].y));
            }
        }
    }
}

QPainterPath RS_Painter::createSolidFillPath(const RS_EntityContainer& loops)  {
    QPainterPath path;
    for(auto* loop: loops) {
        if (loop == nullptr || loop->rtti()!=RS2::EntityContainer)
            continue;

        auto toUiPointF = [this](const RS_Vector& vp) {
            RS_Vector uiPos = toGui(vp);
            return QPointF{uiPos.x, uiPos.y};
        };
        auto toUcsDegrees = [this](double angleRadian) {
            return toUCSAngleDegrees(RS_Math::rad2deg(angleRadian));
        };

        QPainterPath loopPath;
        QPointF uiStart;
        bool hasStart = false;
        for(auto* e: *static_cast<RS_EntityContainer*>(loop)){
            if (e==nullptr)
                continue;

            if (loopPath.isEmpty()) {
                RS_Vector startPoint = e->getStartpoint();
                // Issue #2202: complete circles/ellipses have no start point defined
                // getStartpoint() should return RS_Vector{false}
                hasStart = startPoint.valid;
                if (hasStart)
                    uiStart = toUiPointF(startPoint);
                loopPath.moveTo(uiStart);
            }

            switch (e->rtti()) {
            case RS2::EntityLine: {
                loopPath.lineTo(toUiPointF(e->getEndpoint()));
            }
                break;
            case RS2::EntityArc: {
                auto* arc = static_cast<RS_Arc*>(e);
                double radius = toGuiDX(arc->getRadius());
                double startAngleDegrees = toUcsDegrees(arc->getAngle1());
                double angularLength = RS_Math::rad2deg(arc->isReversed() ? - arc->getAngleLength() : arc->getAngleLength());
                QPointF uiCenter = toUiPointF(arc->getCenter());
                QRectF arcRect{uiCenter - QPointF{radius, radius}, QSizeF{radius, radius}* 2};
                loopPath.arcMoveTo(arcRect, startAngleDegrees);
                loopPath.arcTo(arcRect, startAngleDegrees, angularLength);
            }
                break;
            case RS2::EntityCircle: {
                auto* circle = static_cast<RS_Circle*>(e);
                QPointF uiCenter = toUiPointF(circle->getCenter());
                double radius=toGuiDX(circle->getRadius());
                loopPath.moveTo(uiCenter);
                loopPath.addEllipse(uiCenter, radius, radius);
            }
                break;
            case RS2::EntityEllipse: {
                auto* ellipse = static_cast<RS_Ellipse *>(e);

                double majorRadius = toGuiDX(ellipse->getMajorRadius());
                double minorRadius = ellipse->getRatio() * majorRadius;
                QRectF ellipseRect{- QPointF{majorRadius, minorRadius}, QSizeF{majorRadius, minorRadius} * 2};
                QPainterPath ellipsePath;
                if (ellipse->isEllipticArc()) {
                    double startAngle = toUcsDegrees(ellipse->getAngle1());
                    double angularLength = RS_Math::rad2deg(ellipse->isReversed() ? - ellipse->getAngleLength() : ellipse->getAngleLength());
                    ellipsePath.arcMoveTo(ellipseRect, startAngle);
                    ellipsePath.arcTo(ellipseRect, startAngle, angularLength);
                } else {
                    ellipsePath.addEllipse(ellipseRect);
                }

                QTransform ellipseTransform;
                QPointF uiCenter = toUiPointF(ellipse->getCenter());
                ellipseTransform.translate(uiCenter.x(), uiCenter.y());
                const double ellipseAngle = toUcsDegrees(ellipse->getAngle());
                ellipseTransform.rotate(-ellipseAngle);
                loopPath.addPath(ellipseTransform.map(ellipsePath));
                break;
            }
            default:
                break;
            }
        }
        // Issue #2202: circles/ellipses have no start point defined
        if (hasStart)
            loopPath.lineTo(uiStart);
        path.addPath(loopPath);
    }
    return path;
}

void RS_Painter::debugOutPath(const QPainterPath &tmpPath) const {
    int c = tmpPath.elementCount();
    for (int i = 0; i < c; i++){
        const QPainterPath::Element &element = tmpPath.elementAt(i);
        LC_ERR << "i " << i << "("<< element.x << "," << element.y <<  ") Line To " << element.isLineTo() << " Move To: " << element.isMoveTo() << " Is Curve:" << element.isCurveTo();
    }
}

void RS_Painter::drawSplinePointsWCS(const 	std::vector<RS_Vector> &wcsControlPoints, bool closed){
    std::vector<RS_Vector> uiControlPoints;
    std::transform(wcsControlPoints.cbegin(), wcsControlPoints.cend(), std::back_inserter(uiControlPoints),
                   [this](const RS_Vector& wcsPoint) {
                       return toGui(wcsPoint);
    });
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
    path.moveTo(toGuiPointF(polyline->getStartpoint()));

    for(RS_Entity* entity: *polyline) {
        switch(entity->rtti()) {
            case RS2::EntityLine: {
                path.moveTo(toGuiPointF(entity->getStartpoint()));
                path.lineTo(toGuiPointF(entity->getEndpoint()));
                break;
            }
            case RS2::EntityArc: {
                auto* arc = static_cast<RS_Arc *>(entity);
                drawArcEntity(arc, path);
                break;
            }
            // well, actually this is just for fonts.. better to have separate entity for this. fixme - change latter
            case RS2::EntityEllipse: { // fixme - coordinates translation

                // !! FIXME - sand - why not the same path of the polyline is used??
                const auto* arc = static_cast<RS_Ellipse *>(entity);
                const RS_EllipseData& data = arc->getData();
                const RS_Vector uiCenter = toGui(data.center);

                const double uiMajorRadius = toGuiDX(data.majorP.magnitude()); // fixme - sand - render - cache?
                const double uiMinorRadius = data.ratio * uiMajorRadius;
                if (data.isArc) {
                    drawEllipseArcUI(uiCenter, {uiMajorRadius, uiMinorRadius}, toWorldAngleDegrees(data.angleDegrees), /*view.toWorldAngleDegrees(*/data.startAngleDegrees/*)*/,
                                   /*view.toWorldAngleDegrees(*/data.otherAngleDegrees/*)*/, data.angularLength, data.reversed);
                }
                else {
                    drawEllipseUI(uiCenter, {uiMajorRadius, uiMinorRadius}, toWorldAngleDegrees(data.angleDegrees));
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
    const RS_Vector uiInsert = toGui(wcsInsertionPoint);
    drawImgUI(img, uiInsert, ucsUVector, ucsVVector, scale);
}

void RS_Painter::drawImgUI(QImage& img, const RS_Vector& uiInsert,
                           const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor) {
    PainterGuard painterGuard(*this);

//    LC_ERR << "IMG FACTOR " << factor;
    // Render smooth only at close zooms
    // fixme - sand - check later - actually, these two hints are equivalent!
    if (factor.x < 1 || factor.y < 1) {
        RS_Painter::setRenderHint(SmoothPixmapTransform , true);
    }
    else {
        RS_Painter::setRenderHint(SmoothPixmapTransform);
    }

    RS_Vector un = uVector.normalized();
    RS_Vector vn = vVector.normalized();

    // Image mirroring is switching the handedness of u-v vectors pair which can be detected by
    // looking at the sign of the z component of their cross product. If z is negative image is mirrored.
    std::unique_ptr<QTransform> wm;
    if(std::signbit(RS_Vector::crossP(uVector, vVector).z)) { // mirrored
        wm = std::make_unique<QTransform>(un.x, -vn.x, -un.y, vn.y, uiInsert.x, uiInsert.y);
    } else {
        wm = std::make_unique<QTransform>(un.x, vn.x, un.y, vn.y, uiInsert.x, uiInsert.y);
    }

    wm->scale(factor.x, factor.y);
    setWorldTransform(*wm);

    drawImage(0,-img.height(), img);
}

void RS_Painter::drawTextH(int x1, int y1,
                           int x2, int y2,
                           const QString& text) {
    QPainter::drawText(x1, y1, x2, y2,Qt::AlignRight|Qt::AlignVCenter,text);
}

void RS_Painter::drawTextV(int x1, int y1,
                           int x2, int y2,
                           const QString& text) {
    PainterGuard painterGuard{*this};
    QTransform wm = worldTransform();
    wm.rotate(-90.0);
    setWorldTransform(wm);

    QPainter::drawText(x1, y1, x2, y2,Qt::AlignRight|Qt::AlignVCenter,text);
}

void RS_Painter::fillRect(int x1, int y1, int w, int h,
                            const RS_Color& col) {
    QPainter::fillRect(x1, y1, w, h, col);
}

void RS_Painter::fillPolygonUI( const QPolygonF& uiPolygon)
{
    if (uiPolygon.size() <= 2)
        return;

    const QBrush brushSaved = brush();
    setBrushColor(RS_Color(pen().color()));
    QPainter::drawPolygon(uiPolygon, Qt::OddEvenFill);
    QPainter::setBrush(brushSaved);
}

void RS_Painter::fillEllipseUI(QPointF uiCenter, double radiusX, double radiusY) {
    const QBrush brushSaved = brush();
    setBrushColor(RS_Color(pen().color()));
    QPainter::drawEllipse(uiCenter, radiusX, radiusY);
    QPainter::setBrush(brushSaved);
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
    auto paintDevice = device();
    int mm(paintDevice->widthMM());
    if (mm <= 0) {
        mm=400;
    }
    return double(paintDevice->width())/mm;
}

int RS_Painter::getHeight() const{
    return device()->height();
}


RS_Pen RS_Painter::getPen() const{
    return lpen;
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
            QPainter::setBrush(color);
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

void RS_Painter::drawRectUI(double  uiX1, double  uiY1, double  uiX2, double  uiY2) {
    drawPolygon(QRect(int(uiX1 + 0.5), int(uiY1 + 0.5), int(uiX2 - uiX1 + 0.5), int(uiY2 - uiY1 + 0.5)));
}

void RS_Painter::drawRectUI(const RS_Vector& p1, const RS_Vector& p2) {
    drawPolygon(QRect(int(p1.x+0.5), int(p1.y+0.5), int(p2.x - p1.x+0.5), int(p2.y - p1.y+0.5)));
}

void RS_Painter::drawHandleWCS(const RS_Vector& wcsPos, const RS_Color& c, int size) {
    QPointF uiPos = toGuiPointF(wcsPos);
    fillRect(QRectF{uiPos - QPointF(size, size), QSize{size, size}*2}, c);
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
    int deviceHeight = getHeight();
    if (!std::isnormal(pdsize)){
        int screenPointSize = deviceHeight / 20;
        return screenPointSize;
    }
    else if (DXF_FORMAT_PDSize_isPercent(pdsize)){
        int screenPointSize = (deviceHeight * DXF_FORMAT_PDSize_Percent(pdsize)) / 100;
        return screenPointSize;
    }
    else {
        int screenPointSize = toGuiDY(pdsize);
        return screenPointSize;
    }
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

void RS_Painter::drawEntity(RS_Entity* entity) {
    renderer->renderEntity(this, entity);
}

void RS_Painter::drawAsChild(RS_Entity* entity) {
    renderer->renderEntityAsChild(this, entity);
}

bool RS_Painter::isTextLineNotRenderable(double wcsLineHeight) const {
    double uiHeight = toGuiDY(wcsLineHeight);
    return renderer->isTextLineNotRenderable(uiHeight);
}

void RS_Painter::setViewPort(LC_GraphicViewport *v) {
    viewport = v;
    apply(viewport);
    m_viewPortFactor = v->getFactor();
    viewPortOffsetX = v->getOffsetX();
    viewPortOffsetY = v->getOffsetY();
    m_viewPortOffset.set(viewPortOffsetX, viewPortOffsetY);
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

void RS_Painter::toGui(const RS_Vector &wcsCoordinate, double &uiX, double &uiY) const {
//    viewport->toUI(pos, x,y);

    if (hasUCS()){
//   ucsToUCS(wcsCoordinate.x, wcsCoordinate.y, uiX, uiY);
// the code below is equivalent to

/*
        RS_Vector wcs = RS_Vector(wcsCoordinate.x, wcsCoordinate.y);
        RS_Vector newPos = wcs-m_ucsOrigin;
        newPos.rotate(xAxisAngle);
        uiY = newPos.x;
        uiX = newPos.y;
*/
        double ucsPositionX = wcsCoordinate.x - getUcsOrigin().x;
        double ucsPositionY = wcsCoordinate.y - getUcsOrigin().y;

        const RS_Vector& ucsRotation = getUcsRotation();
        double ucsX = ucsPositionX * ucsRotation.x - ucsPositionY * ucsRotation.y;
        double ucsY = ucsPositionX * ucsRotation.y + ucsPositionY * ucsRotation.x;

//        uiX = toGuiX(uiX);
        uiX = ucsX * viewPortFactorX + viewPortOffsetX;
//        uiY = toGuiY(uiY);
        uiY = -ucsY * viewPortFactorY - viewPortOffsetY + viewPortHeight;
    }
    else{
//        uiX = toGuiX(wcsCoordinate.x);
        uiX = wcsCoordinate.x * viewPortFactorX + viewPortOffsetX;
//        uiY = toGuiY(wcsCoordinate.y);
        uiY = -wcsCoordinate.y * viewPortFactorY - viewPortOffsetY + viewPortHeight;
    }
}

RS_Vector RS_Painter::toGui(const RS_Vector& worldCoordinates) const
{
    RS_Vector uiPosition = worldCoordinates;
    if (hasUCS()) {
        uiPosition.move(-getUcsOrigin()).rotate(getUcsRotation());
    }
    uiPosition.scale(m_viewPortFactor).move(m_viewPortOffset);
    uiPosition.y = viewPortHeight - uiPosition.y;

#ifdef DEBUG_RENDERING_TOGUI
    {
        using namespace RS_Math;
        double uiX=0., uiY=0.;
        const_cast<RS_Painter*>(this)->toGui(worldCoordinates, uiX, uiY);
        if (!(equal(uiX, uiPosition.x) && equal(uiY, uiPosition.y))) {
            LC_ERR<<QString{" : (%1, %2) vs (%3, %4)"}
                          .arg(uiPosition.x, 10, 'g', 10)
                          .arg(uiPosition.y, 10, 'g', 10)
                          .arg(uiX, 10, 'g', 10)
                          .arg(uiY, 10, 'g', 10);
            LC_ERR<<"delta: "<<uiPosition.x - uiX<<"(ulp "<<ulp(uiX)<<", "<<uiPosition.y - uiY<<"(ulp: "<<ulp(uiY);
            assert(!"toGui() failure");
        }
    }
#endif

   return uiPosition;
}

QTransform RS_Painter::getToGuiTransform() const
{
    QPolygonF wcs{ {0., 0.}, {1., 0.}, {0., 1.}, {1., 1.}};
    QPolygonF gui;
    std::transform(wcs.begin(), wcs.end(), std::back_inserter(gui), [this](const QPointF& wcsPoint) {
        RS_Vector guiV = toGui({wcsPoint.x(), wcsPoint.y()});
        return QPointF{guiV.x, guiV.y};
    });
    QTransform transform;
    QTransform::quadToQuad(wcs, gui, transform);
    return transform;
}


QPointF RS_Painter::toGuiPointF(const RS_Vector& worldCoordinates) const{
    RS_Vector uiPos = toGui(worldCoordinates);
    return {uiPos.x, uiPos.y};
}

double RS_Painter::toGuiDX(double ucsDX) const {
//    return viewport->toGuiDX(d);
   return ucsDX * m_viewPortFactor.x;
}

double RS_Painter::toGuiDY(double ucsDY) const {
//    return viewport->toGuiDY(d);
    return ucsDY * m_viewPortFactor.y;
}

void RS_Painter::disableUCS(){
    useUCS(false);
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
