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

#ifndef RS_STATICGRAPHICVIEW_H
#define RS_STATICGRAPHICVIEW_H

#include "rs_graphicview.h"
#include "rs_layerlistlistener.h"

/**
 * This is an implementation of a graphic viewer with a fixed size
 * for drawing onto fixed devices (such as bitmaps).
 */
class RS_StaticGraphicView: public RS_GraphicView {
public:
    RS_StaticGraphicView(int w, int h, RS_Painter* p);
    virtual ~RS_StaticGraphicView();

    virtual int getWidth();
    virtual int getHeight();
    virtual void redraw(RS2::RedrawMethod) {}
    virtual void adjustOffsetControls() {}
    virtual void adjustZoomControls() {}
    virtual void setMouseCursor(RS2::CursorType ) {}

    virtual void emulateMouseMoveEvent() {}
	virtual void updateGridStatusWidget(const RS_String& ) {}

    void paint();

private:
    //! Width
    int width;

    //! Height
    int height;
	
	RS_Painter* painter;
};

#endif
