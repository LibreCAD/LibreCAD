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


#ifndef RS_PAINTERQT_H
#define RS_PAINTERQT_H

#include <QPainter>
#include <QPainterPath>

#include "rs_painter.h"
#include "rs_pen.h"

class RS_GraphicView;
class RS_Spline;

/**
 * The Qt implementation of a painter. It can draw objects such as
 * lines or arcs in a widget. All coordinates are screen coordinates
 * and have nothing to do with the graphic view.
 */
class RS_PainterQt: public QPainter, public RS_Painter {

public:
    RS_PainterQt( QPaintDevice* pd);
    virtual ~RS_PainterQt()=default;

    void moveTo(int x, int y);
    void lineTo(int x, int y);
    void drawGridPoint(const RS_Vector& p) override;
    void drawGridPoint(const double& x, const double& y) override;
    void drawPoint(const RS_Vector& p, int pdmode, int pdsize) override;
    void drawLine(const RS_Vector& p1, const RS_Vector& p2) override;
    void drawLine(const double &x1, const double &y1, const double &x2, const double &y2) override;
    void drawLineSimple(const double &x1, const double &y1, const double &x2, const double &y2) override;
    //virtual void drawRect(const RS_Vector& p1, const RS_Vector& p2);
    void fillRect ( const QRectF & rectangle, const RS_Color & color ) override;
    void fillRect ( const QRectF & rectangle, const QBrush & brush ) override;

    void drawArcEntity(const RS_Vector &cp, double radius, double startAngleDegrees,double angularLength) override;
    virtual void drawArcMac(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         bool reversed);
    void drawCircle(const RS_Vector&, double radius) override;
/*    void drawEllipse(const RS_Vector& cp,
                             double radius1, double radius2,
                             double angle,
                             double a1, double a2,
                             bool reversed) override;*/

    void drawEllipse(double centerX, double centerY,
                             double radius1, double radius2,
                             double angle) override;

    void drawEllipseArc(double centerX, double centerY,
                                double radius1, double radius2,
                                double angle,
                                double angle1, double angle2, double angleLength,
                                bool reversed) override;

    void drawPolyline(const RS_Polyline& polyline, const RS_GraphicView& view) override;
    void drawSplinePoints(const	std::vector<RS_Vector> &controlPoints, bool closed) override;
    void drawSpline(const RS_Spline& spline, const RS_GraphicView& view) override;
    void drawImg(QImage& img, const RS_Vector& pos,
                               const RS_Vector& u, const RS_Vector& v, const RS_Vector& factor) override;
    void drawTextH(int x1, int y1, int x2, int y2,
                           const QString& text) override;
    void drawTextV(int x1, int y1, int x2, int y2,
                           const QString& text) override;
    void drawText(const QRect& rect, const QString& text, QRect* boundingBox) override;

    void fillRect(int x1, int y1, int w, int h,
                          const RS_Color& col) override;

    void fillTriangle(const RS_Vector& p1,
                              const RS_Vector& p2,
                              const RS_Vector& p3) override;

    void drawPolygon(const QPolygon& a,Qt::FillRule rule=Qt::WindingFill) override;
    void fillPath ( const QPainterPath & path, const QBrush& brush) override;
    void drawPath ( const QPainterPath & path ) override;
    void erase() override;
    int getWidth() const override;
    /** get Density per millimeter on screen/print device
      *@return density per millimeter in pixel/mm
      */
    double getDpmm() const override;

    double getDpmmCached() const {return cachedDpmm;};
    int getHeight() const override;


    RS_Pen getPen() const override;
    void setPen(const RS_Pen& pen) override;
    void setPen(const RS_Color& color) override;
    void setPen(int r, int g, int b) override;
    void disablePen() override;
    //virtual void setColor(const QColor& color);
    const QBrush& brush() const override;
    void setBrush(const RS_Color& color) override;
    void setBrush(const QBrush& color) override;

    void setClipRect(int x, int y, int w, int h) override;
    void resetClipping() override;

    RS_Pen& getRsPen();

    void setPenJoinStyle(Qt::PenJoinStyle penJoinStyle);

    void setPenCapStyle(Qt::PenCapStyle penCapStyle);

    void setMinCircleDrawingRadius(double minCircleDrawingRadius);

    void setMinArcDrawingRadius(double minArcDrawingRadius);

    void setCachedDpmm(double cachedDpmm);

    void setMinEllipseMajorRadius(double minEllipseMajorRadius);

    double getMinEllipseMinorRadius() const;

    void setMinEllipseMinorRadius(double minEllipseMinorRadius);

    double getMinLineDrawingLen() const;

    void setMinLineDrawingLen(double minLineDrawingLen);
    void createSolidFillPath(QPainterPath &path, const RS_GraphicView *view, QList<RS_Entity *> entities) override;
    void noCapStyle() override;
protected:



};

#endif
