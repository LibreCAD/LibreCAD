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


#ifndef RS_PAINTERADAPTER_H
#define RS_PAINTERADAPTER_H

#include "rs_painter.h"
//Added by qt3to4:
#include <qpolygon.h>



/**
 * An abstract adapter class for painter object. The methods in this class are empty. 
 * This class exists as convenience for creating painter objects.
 */
class RS_PainterAdapter: public RS_Painter {
public:
    RS_PainterAdapter() : RS_Painter() {}
    virtual ~RS_PainterAdapter() {}
	
	virtual void moveTo(int , int ) {}
	virtual void lineTo(int , int ) {}

    virtual void drawGridPoint(const RS_Vector&) {}
    virtual void drawPoint(const RS_Vector&) {}
    virtual void drawLine(const RS_Vector&, const RS_Vector&) {}
    virtual void drawRect(const RS_Vector&, const RS_Vector&) {}
    virtual void drawArc(const RS_Vector&, double,
                         double, double,
                         const RS_Vector&, const RS_Vector&,
                         bool ) {}
    virtual void drawArc(const RS_Vector&, double,
                         double, double,
                         bool ) {}
    void createArc(QPolygon& ,
                   const RS_Vector&, double,
                   double, double,
                   bool ) {}
    virtual void drawCircle(const RS_Vector&, double) {}
    virtual void drawEllipse(const RS_Vector&,
                             double, double,
                             double,
                             double, double,
                             bool ) {}
        virtual void drawImg(QImage& , const RS_Vector&,
			double, const RS_Vector&,
			int, int, int, int) {}

    virtual void drawTextH(int, int, int, int,
                           const RS_String&) {}
    virtual void drawTextV(int, int, int, int,
                           const RS_String&) {}

    virtual void fillRect(int, int, int, int,
                          const RS_Color&) {}

    virtual void fillTriangle(const RS_Vector&,
                              const RS_Vector&,
                              const RS_Vector&) {}

    virtual RS_Pen getPen() { return RS_Pen(); }
    virtual void setPen(const RS_Pen&) {}
    virtual void setPen(const RS_Color&) {}
    virtual void setPen(int, int, int) {}
    virtual void disablePen() {}
	virtual void setBrush(const RS_Color&) {}
	virtual void drawPolygon(const QPolygon& ) {}
	virtual void erase() {}
	virtual int getWidth() { return 0; }
	virtual int getHeight() { return 0; }

    virtual void setClipRect(int, int, int, int) {}
    virtual void resetClipping() {}

};

#endif
