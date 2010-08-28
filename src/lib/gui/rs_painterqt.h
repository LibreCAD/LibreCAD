/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <qpainter.h>

#include "rs_color.h"
#include "rs_graphicview.h"
#include "rs_painter.h"

/**
 * The Qt implementation of a painter. It can draw objects such as
 * lines or arcs in a widget. All coordinates are screen coordinates
 * and have nothing to do with the graphic view.
 */
class RS_PainterQt: public QPainter, public RS_Painter {

public:
    RS_PainterQt( QPaintDevice* pd);
    virtual ~RS_PainterQt();

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
	virtual void drawImg(RS_Img& img, const RS_Vector& pos, 
			double angle, const RS_Vector& factor,
			int sx, int sy, int sw, int sh);
    virtual void drawTextH(int x1, int y1, int x2, int y2,
                           const QString& text);
    virtual void drawTextV(int x1, int y1, int x2, int y2,
                           const QString& text);

    virtual void fillRect(int x1, int y1, int w, int h,
                          const RS_Color& col);

    virtual void fillTriangle(const RS_Vector& p1,
                              const RS_Vector& p2,
                              const RS_Vector& p3);

    virtual void drawPolygon(const QPolygon& a);
	virtual void erase();
	virtual int getWidth();
	virtual int getHeight();

    virtual RS_Pen getPen();
    virtual void setPen(const RS_Pen& pen);
    virtual void setPen(const RS_Color& color);
    virtual void setPen(int r, int g, int b);
    virtual void disablePen();
    //virtual void setColor(const QColor& color);
    virtual void setBrush(const RS_Color& color);

    virtual void setClipRect(int x, int y, int w, int h);
    virtual void resetClipping();

protected:
	RS_Pen lpen;
	long rememberX; // Used for the moment because QPainter doesn't support moveTo anymore, thus we need to remember ourselve the moveTo positions
	long rememberY;
};

#endif

