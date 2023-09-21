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

    void moveTo(int x, int y) override;
    void lineTo(int x, int y) override;
    void drawGridPoint(const RS_Vector& p) override;
    void drawPoint(const RS_Vector& p, int pdmode, int pdsize) override;
    void drawLine(const RS_Vector& p1, const RS_Vector& p2) override;
    //virtual void drawRect(const RS_Vector& p1, const RS_Vector& p2);
    void fillRect ( const QRectF & rectangle, const RS_Color & color ) override;
    void fillRect ( const QRectF & rectangle, const QBrush & brush ) override;
    void drawArc(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         const RS_Vector& p1, const RS_Vector& p2,
                         bool reversed) override;

    void drawArc(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         bool reversed) override;
    virtual void drawArcMac(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         bool reversed);
    void drawCircle(const RS_Vector&, double radius) override;
    void drawEllipse(const RS_Vector& cp,
                             double radius1, double radius2,
                             double angle,
                             double a1, double a2,
                             bool reversed) override;
    void drawPolyline(const RS_Polyline& polyline, const RS_GraphicView& view) override;
    void drawSplinePoints(const LC_SplinePointsData& splineData) override;
    void drawSpline(const RS_Spline& spline, const RS_GraphicView& view) override;
    void drawImg(QImage& img, const RS_Vector& pos,
                               const RS_Vector& u, const RS_Vector& v, const RS_Vector& factor) override;
    void drawTextH(int x1, int y1, int x2, int y2,
                           const QString& text) override;
    void drawTextV(int x1, int y1, int x2, int y2,
                           const QString& text) override;

    void fillRect(int x1, int y1, int w, int h,
                          const RS_Color& col) override;

    void fillTriangle(const RS_Vector& p1,
                              const RS_Vector& p2,
                              const RS_Vector& p3) override;

    void drawPolygon(const QPolygon& a,Qt::FillRule rule=Qt::WindingFill) override;
    void drawPath ( const QPainterPath & path ) override;
    void erase() override;
    int getWidth() const override;
    /** get Density per millimeter on screen/print device
      *@return density per millimeter in pixel/mm
      */
    double getDpmm() const override;
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

protected:

    QPainterPath createSplinePoints(const LC_SplinePointsData& data) const;
    QPainterPath createSpline(const RS_Spline& spline, const RS_GraphicView& view) const;
    RS_Pen lpen;
    long rememberX = 0; // Used for the moment because QPainter doesn't support moveTo anymore, thus we need to remember ourselves the moveTo positions
    long rememberY = 0;
};

#endif

