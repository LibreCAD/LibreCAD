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

#include "rs_painter.h"
#include "rs_pen.h"

/**
 * The Qt implementation of a painter. It can draw objects such as
 * lines or arcs in a widget. All coordinates are screen coordinates
 * and have nothing to do with the graphic view.
 */
class RS_PainterQt: public QPainter, public RS_Painter {

public:
    RS_PainterQt( QPaintDevice* pd);
    virtual ~RS_PainterQt()=default;

    virtual void moveTo(int x, int y);
    virtual void lineTo(int x, int y);
    virtual void drawGridPoint(const RS_Vector& p);
    virtual void drawPoint(const RS_Vector& p);
    virtual void drawLine(const RS_Vector& p1, const RS_Vector& p2);
    //virtual void drawRect(const RS_Vector& p1, const RS_Vector& p2);
    virtual void fillRect ( const QRectF & rectangle, const RS_Color & color );
    virtual void fillRect ( const QRectF & rectangle, const QBrush & brush );
    virtual void drawArc(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         const RS_Vector& p1, const RS_Vector& p2,
                         bool reversed);

    virtual void drawArc(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         bool reversed);
    virtual void drawArcMac(const RS_Vector& cp, double radius,
                         double a1, double a2,
                         bool reversed);
    virtual void drawCircle(const RS_Vector&, double radius);
    virtual void drawEllipse(const RS_Vector& cp,
                             double radius1, double radius2,
                             double angle,
                             double a1, double a2,
                             bool reversed);
        virtual void drawImg(QImage& img, const RS_Vector& pos,
            double angle, const RS_Vector& factor);
    virtual void drawTextH(int x1, int y1, int x2, int y2,
                           const QString& text);
    virtual void drawTextV(int x1, int y1, int x2, int y2,
                           const QString& text);

    virtual void fillRect(int x1, int y1, int w, int h,
                          const RS_Color& col);

    virtual void fillTriangle(const RS_Vector& p1,
                              const RS_Vector& p2,
                              const RS_Vector& p3);

    virtual void drawPolygon(const QPolygon& a,Qt::FillRule rule=Qt::WindingFill);
    virtual void drawPath ( const QPainterPath & path );
    virtual void erase();
    virtual int getWidth() const;
    /** get Density per millimeter on screen/print device
      *@return density per millimeter in pixel/mm
      */
    virtual double getDpmm() const;
    virtual int getHeight() const;


    virtual RS_Pen getPen() const;
    virtual void setPen(const RS_Pen& pen);
    virtual void setPen(const RS_Color& color);
    virtual void setPen(int r, int g, int b);
    virtual void disablePen();
    //virtual void setColor(const QColor& color);
    virtual const QBrush& brush() const;
    virtual void setBrush(const RS_Color& color);
    virtual void setBrush(const QBrush& color);

    virtual void setClipRect(int x, int y, int w, int h);
    virtual void resetClipping();

protected:
    RS_Pen lpen;
    long rememberX; // Used for the moment because QPainter doesn't support moveTo anymore, thus we need to remember ourselves the moveTo positions
    long rememberY;
};

#endif

