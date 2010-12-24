/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_OVERLAYLINE_H
#define RS_OVERLAYLINE_H

#include "rs_line.h"



/**
 * Class for a overlay line entity. It's used to draw lines on the overlay paint event
 * The main difference is that the coordinates are actual screen coordinates and not real world coordinates
 *
 * @author R. van Twisk
 */
class RS_OverlayLine : public RS_Line {
public:
	RS_OverlayLine(RS_EntityContainer* parent, const RS_LineData& d);
	
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double patternOffset=0.0);
}
;

#endif
