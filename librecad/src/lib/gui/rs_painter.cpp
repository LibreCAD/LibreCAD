/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
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
#include "rs_graphicview.h"
#include "rs_linetypepattern.h"
#include "rs_arc.h"
#include "rs_ellipse.h"
#include "rs_circle.h"


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

/**
 * Draws a point at (x1, y1).
 */
void RS_Painter::drawPoint(const RS_Vector& p, int pdmode, int pdsize) {
    int screenX = p.x;
    int screenY = p.y;
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
            QPainter::drawLine(screenX - 1, screenY, screenX + 1, screenY);
            QPainter::drawLine(screenX, screenY - 1, screenX, screenY + 1);
            break;
        }
        case DXF_FORMAT_PDMode_CentreBlank: {
            /*	Centre is blank  */
            break ;
        }
        case DXF_FORMAT_PDMode_CentrePlus: {
            /*	Centre +  */
            QPainter::drawLine(screenX - pdsize, screenY, screenX + pdsize, screenY);
            QPainter::drawLine(screenX, screenY - pdsize, screenX, screenY + pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreCross: {
            /*	Centre X  */
            QPainter::drawLine(screenX - pdsize, screenY - pdsize, screenX + pdsize, screenY + pdsize);
            QPainter::drawLine(screenX + pdsize, screenY - pdsize, screenX - pdsize, screenY + pdsize);
            break;
        }
        case DXF_FORMAT_PDMode_CentreTick: {
            /*	Centre vertical tick  */
            QPainter::drawLine(screenX, screenY - halfPDSize, screenX, screenY);
            break;
        }
    }

/*	Surrounding circle if required  */
    if (DXF_FORMAT_PDMode_hasEncloseCircle(pdmode)) {
        /*	Approximate circle by an octagon  */
        int xMin = screenX-halfPDSize;
        int xMax = screenX+halfPDSize;
        int yMin = screenY-halfPDSize;
        int yMax = screenY+halfPDSize;
        int octOffset = halfPDSize * 0.71;
        int xOctMin = screenX - octOffset;
        int xOctMax = screenX + octOffset;
        int yOctMin = screenY - octOffset;
        int yOctMax = screenY + octOffset;

        QPainter::drawLine(screenX, yMin, xOctMax, yOctMin);
        QPainter::drawLine(screenX, yMin, xOctMin, yOctMin);
        QPainter::drawLine(screenX, yMax, xOctMax, yOctMax);
        QPainter::drawLine(screenX, yMax, xOctMin, yOctMax);

        QPainter::drawLine(xMin, screenY, xOctMin, yOctMin);
        QPainter::drawLine(xMin, screenY, xOctMin, yOctMax);
        QPainter::drawLine(xMax, screenY, xOctMax, yOctMin);
        QPainter::drawLine(xMax, screenY, xOctMax, yOctMax);
    }

/*	Surrounding square if required  */
    if (DXF_FORMAT_PDMode_hasEncloseSquare(pdmode)) {
        int xMin = screenX-halfPDSize;
        int xMax = screenX+halfPDSize;
        int yMin = screenY-halfPDSize;
        int yMax = screenY+halfPDSize;

        QPainter::drawLine(xMin, yMin, xMax, yMin);
        QPainter::drawLine(xMin, yMax, xMax, yMax);
        QPainter::drawLine(xMin, yMin, xMin, yMax);
        QPainter::drawLine(xMax, yMin, xMax, yMax);
    }
}

/**
 * Draws a line from (x1, y1) to (x2, y2).
 */
void RS_Painter::drawLine(const RS_Vector& p1, const RS_Vector& p2){
    QPainter::drawLine(QPointF(p1.x, p1.y),QPointF(p2.x, p2.y));
}

void RS_Painter::drawLineSimple(const double &x1, const double &y1, const double &x2, const double &y2){
    QPainter::drawLine(QPointF(x1, y1),QPointF(x2, y2));
}

void RS_Painter::drawLine(const double &x1, const double &y1, const double &x2, const double &y2){
    if(QPointF(x2-x1, y2-y1).manhattanLength() > minLineDrawingLen) {
        QPainter::drawLine(QPointF(x1, y1),QPointF(x2, y2));
    }
    else{
        QPainter::drawPoint(QPointF(x1, y1));
    }
}

/**
 * Draws an arc.
 *
 * @param cx center in x
 * @param cy center in y
 * @param radius Radius
 * @param a1 Angle 1 in rad
 * @param a2 Angle 2 in rad
 * @param reversed true: clockwise, false: counterclockwise
 */
void RS_Painter::drawArcEntity( const RS_Vector& cp,
                                  double radiusX,
                                  double radiusY,
                                  double startAngleDegrees,
                                  double angularLength){
    if(radiusX<=minArcDrawingRadius) {
        QPainter::drawPoint(QPointF(cp.x, cp.y));
        return;
    }
    // The rect for the circle
    QPainterPath path;
    double rx = cp.x - radiusX;
    double ry = cp.y - radiusY;
    double dX = radiusX + radiusX;
    double dY = radiusY + radiusY;
    path.arcMoveTo(rx, ry, dX, dY, startAngleDegrees);
    path.arcTo(rx, ry, dX, dY, startAngleDegrees, angularLength);
    QPainter::drawPath(path);
}

/**
 * Draws a circle.
 * @param cp Center point
 * @param radius Radius
 */
void RS_Painter::drawCircle(const RS_Vector& cp, double radius){
    if (radius < minCircleDrawingRadius){
        QPainter::drawPoint(QPointF(cp.x, cp.y));
    }
    else {
        QPainter::drawEllipse(QPointF(cp.x, cp.y), radius, radius);
    }
}

void RS_Painter::drawEllipse(double centerX, double centerY, double radius1, double radius2, double angle) {
    if (radius1 < minEllipseMajorRadius){
        QPainter::drawPoint(QPointF(centerX, centerY));
    }
    else if (radius2 < minEllipseMinorRadius) {//ellipse too small
        QTransform t1;
        t1.translate(centerX, centerY);
        t1.rotate(-angle);
        t1.translate(-centerX, -centerY);
        save();
        setTransform(t1, false);
        QPainter::drawLine(QPointF(centerX - radius1, centerY), QPointF(centerX + radius1, centerY));
        restore();
    }
    else {
        QTransform t1;
        t1.translate(centerX, centerY);
        t1.rotate(-angle);
        t1.translate(-centerX, -centerY);
        save();
        setTransform(t1, false);
        QPainter::drawEllipse(QRectF(centerX - radius1, centerY - radius2, radius1 + radius1, radius2 + radius2));
        restore();
    }
}


void RS_Painter::createSolidFillPath(QPainterPath &path, const RS_GraphicView *view,   QList<RS_Entity *> entities)  {
        foreach (auto l, entities) {
            if (l->rtti()==RS2::EntityContainer) {
                auto* loop = (RS_EntityContainer*)l;
                QPainterPath loopPath;
                // edges:
//                LC_ERR << "loop------------------------------- " << loop->count();
                for(auto e: *loop){
                    switch (e->rtti()) {
                        case RS2::EntityLine: {
                            QPoint pt1(RS_Math::round(view->toGuiX(e->getStartpoint().x)),
                                       RS_Math::round(view->toGuiY(e->getStartpoint().y)));
                            QPoint pt2(RS_Math::round(view->toGuiX(e->getEndpoint().x)),
                                       RS_Math::round(view->toGuiY(e->getEndpoint().y)));

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
                            double radius = view->toGuiDX(arcData.radius);
                            // can't skip due to minimal radius, it will lead to filling errors
//                        if (radius > view->getMinArcDrawingRadius()) {
                            const RS_Vector &cp = arcData.center;
                            double rx = view->toGuiX(cp.x) - radius;
                            double ry = view->toGuiY(cp.y) - radius;
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
                            if (loopPath.isEmpty()) {
                                loopPath.arcMoveTo(rx, ry, size, size, startAngleDegrees);
                            }
                            loopPath.arcTo(rx, ry, size, size, startAngleDegrees, angularLength);
//                        }
                            break;
                        }
                        case RS2::EntityCircle: {
                            auto* circle = static_cast<RS_Circle*>(e);
                            RS_Vector c=view->toGui(circle->getCenter());
                            double r=view->toGuiDX(circle->getRadius());
                            path.addEllipse(QPointF(c.x,c.y),r,r);
                            break;
                        }
                        case RS2::EntityEllipse: {
                            auto ellipse = static_cast<RS_Ellipse *>(e);
                            const RS_EllipseData &ellipseData = ellipse->getData();
                            double centerX = view->toGuiX(ellipseData.center.x);
                            double centerY = view->toGuiY(ellipseData.center.y);
                            double angle = ellipseData.angleDegrees;
                            double radius1 = ellipse->getMajorRadius()*view->getFactor().x;
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



void RS_Painter::drawEllipseArc(double centerX, double centerY, double radius1, double radius2, double angle,
                                  double angle1, double angle2, double angularLength, bool reversed) {
    if (radius1 < minEllipseMajorRadius){
        QPainter::drawPoint(QPointF(centerX, centerY));
    }
    else if (radius2 < minEllipseMinorRadius) {//ellipse too small
        QTransform t1;
        t1.translate(centerX, centerY);
        t1.rotate(-angle);
        t1.translate(-centerX, -centerY);
        save();
        setTransform(t1, false);
        QPainter::drawLine(QPointF(centerX - radius1, centerY), QPointF(centerX + radius1, centerY));
        restore();
    }
    else {
        QTransform t1;
        t1.translate(centerX, centerY);
        t1.rotate(-angle);
        t1.translate(-centerX, -centerY);
        save();
        setTransform(t1, false);
        double rx = centerX - radius1;
        double ry = centerY - radius2;
        double size1 = radius1 + radius1;
        double size2 = radius2 + radius2;
        if (reversed){
            angle1 = angle2 - 360;
            angularLength = -angularLength;
        }
        else{

        }
        QPainterPath path;
        path.arcMoveTo(rx, ry, size1, size2, angle1);
        path.arcTo(rx, ry, size1, size2, angle1, angularLength);
        QPainter::drawPath(path);
        restore();
    }
}

void RS_Painter::drawSplinePoints(const 	std::vector<RS_Vector> &controlPoints, bool closed){
    size_t n = controlPoints.size();
    if(n < 2)
        return;

    RS_Vector vStart = controlPoints.front();
    RS_Vector vControl(false), vEnd(false);

    QPainterPath qPath(QPointF(vStart.x, vStart.y));

    if(closed){
        if(n < 3){
            qPath.lineTo(QPointF(controlPoints[1].x, controlPoints[1].y));
        }
        else {
            const RS_Vector &cp0 = controlPoints[0];
            const RS_Vector &cpNMinus1 = controlPoints[n - 1];
            vStart = (cpNMinus1 + cp0) / 2.0;
            qPath.moveTo(QPointF(vStart.x, vStart.y));

            vEnd = (cp0 + controlPoints[1]) / 2.0;
            qPath.quadTo(QPointF(cp0.x, cp0.y), QPointF(vEnd.x, vEnd.y));

            for (size_t i = 1; i < n - 1; i++) {
                const RS_Vector &cpi = controlPoints[i];
                vEnd = (cpi + controlPoints[i + 1]) / 2.0;
                qPath.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
            }
            qPath.quadTo(QPointF(cpNMinus1.x, cpNMinus1.y), QPointF(vStart.x, vStart.y));
        }
    }
    else {
        const RS_Vector &cp1 = controlPoints[1];
        if(n < 3) {
            qPath.lineTo(QPointF(cp1.x, cp1.y));
        }
        else {
            const RS_Vector &cp2 = controlPoints[2];
            if (n < 4) {
                qPath.quadTo(QPointF(cp1.x, cp1.y), QPointF(cp2.x, cp2.y));
            }
            else {
                vEnd = (cp1 + cp2) / 2.0;
                qPath.quadTo(QPointF(cp1.x, cp1.y), QPointF(vEnd.x, vEnd.y));

                for (size_t i = 2; i < n - 2; i++) {
                    const RS_Vector &cpi = controlPoints[i];
                    vEnd = (cpi + controlPoints[i + 1]) / 2.0;
                    qPath.quadTo(QPointF(cpi.x, cpi.y), QPointF(vEnd.x, vEnd.y));
                }

                qPath.quadTo(QPointF(controlPoints[n - 2].x, controlPoints[n - 2].y), QPointF( controlPoints[n - 1].x,  controlPoints[n - 1].y));
            }
        }
    }
    QPainter::drawPath(qPath);
}

void RS_Painter::drawPolyline(const RS_Polyline& polyline, const RS_GraphicView& view){
    if (polyline.isEmpty()) {
        return;
    }
    QPainterPath path;
    path.moveTo(view.toGuiX(polyline.getStartpoint().x), view.toGuiY(polyline.getStartpoint().y));

    for(RS_Entity* entity: polyline) {
        switch(entity->rtti()) {
        case RS2::EntityLine: {
            path.moveTo(view.toGuiX(entity->getStartpoint().x),
                        view.toGuiY(entity->getStartpoint().y));
            path.lineTo(view.toGuiX(entity->getEndpoint().x),
                        view.toGuiY(entity->getEndpoint().y));
            break;
        }
            case RS2::EntityArc: {
                auto arc = *static_cast<RS_Arc *>(entity);
                RS_ArcData data = arc.getData();
                double radius = data.radius * view.getFactor().x;
                if (radius > minArcDrawingRadius) {
                    double centerX = view.toGuiX(data.center.x);
                    double centerY = view.toGuiY(data.center.y);
                    double startAngleDegrees, angularLength;

                    if (arc.isReversed()) {
                        startAngleDegrees = data.otherAngleDegrees;
                        startAngleDegrees = startAngleDegrees - 360;
                        angularLength = -data.angularLength;
                    } else {
                        startAngleDegrees = data.startAngleDegrees;
                        angularLength = data.angularLength;
                    }
                    double size = radius + radius;
                    RS_Vector startoPint = view.toGui(arc.getStartpoint());
                    path.moveTo(startoPint.x, startoPint.y);
                    path.arcTo(centerX - radius, centerY - radius, size, size, startAngleDegrees, angularLength);
                }
                break;
            }
            case RS2::EntityEllipse: {
                auto arc = *static_cast<RS_Ellipse *>(entity);
                const RS_EllipseData& data = arc.getData();
                const RS_Vector center=view.toGui(data.center);
                const double ra = data.majorP.magnitude()*view.getFactor().x;
                const double rb = data.ratio*ra;
                if (data.isArc)
                    drawEllipseArc(center.x, center.y, ra, rb, data.angleDegrees, data.startAngleDegrees, data.otherAngleDegrees, data.angularLength, data.reversed);
                else
                    drawEllipse(center.x, center.y, ra, rb, data.angleDegrees);
                break;
            }
            default:
                LC_ERR<<"Polyline may contain lines/arcs only: found rtti() ="<<entity->rtti();
        }
    }
    QPainter::drawPath(path);
}

void RS_Painter::drawSpline(const RS_Spline& spline, const RS_GraphicView& view){
    QPainterPath path;
    unsigned int count = spline.count();
    if (count > 0) {
        RS_Entity *child = spline.unsafeEntityAt(0);
        path.moveTo(view.toGuiX(child->getStartpoint().x), view.toGuiY(child->getStartpoint().y));
        for (unsigned int i = 0; i < count;i++) {
            child = spline.unsafeEntityAt(i);
            path.lineTo(view.toGuiX(child->getEndpoint().x), view.toGuiY(child->getEndpoint().y));
        }
    }

    QPainter::drawPath(path);
}

void RS_Painter::drawImg(QImage& img, const RS_Vector& pos,
                           const RS_Vector& uVector, const RS_Vector& vVector, const RS_Vector& factor) {
    save();
    // Render smooth only at close zooms
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
    if(RS_Vector::crossP(uVector, vVector).z < 0) {
        wm.reset(new QTransform(un.x, -vn.x, -un.y, vn.y, pos.x, pos.y));
    } else {
        wm.reset( new QTransform(un.x, vn.x, un.y, vn.y, pos.x, pos.y));
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

void RS_Painter::fillTriangle(const RS_Vector& p1,
                                const RS_Vector& p2,
                                const RS_Vector& p3) {

    QPolygonF arr;
//    save();
    QBrush brushSaved=brush();
    arr.append({p1.x, p1.y});
    arr.append({p2.x, p2.y});
    arr.append({p3.x, p3.y});
    setBrush(RS_Color(pen().color()));
    QPainter::drawPolygon(arr,Qt::OddEvenFill);
    setBrush(brushSaved);
//    restore();
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

const QBrush& RS_Painter::brush() const{
    return QPainter::brush();
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

void RS_Painter::setBrush(const RS_Color& color) {
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

void RS_Painter::setBrush(const QBrush& color) {
    QPainter::setBrush(color);
}

void RS_Painter::drawPolygon(const QPolygon& a, Qt::FillRule rule) {
    QPainter::drawPolygon(a,rule);
}

void RS_Painter::drawPolygonF(const QPolygonF& a, Qt::FillRule rule) {
    QPainter::drawPolygon(a,rule);
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

void RS_Painter::drawRect(const RS_Vector& p1, const RS_Vector& p2) {
    drawPolygon(QRect(int(p1.x+0.5), int(p1.y+0.5), int(p2.x - p1.x+0.5), int(p2.y - p1.y+0.5)));
}

void RS_Painter::drawHandle(const RS_Vector& p, const RS_Color& c, int size) {
    if (size<0) { // fixme - remove redundant check in painting
        size = 2;
    }
    int doubleSize = 2 * size;
    fillRect((int)(p.x - size), (int)(p.y - size), doubleSize, doubleSize, c);
}
