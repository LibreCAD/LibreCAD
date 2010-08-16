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


#ifndef RS_MOUSEEVENT_H
#define RS_MOUSEEVENT_H

#include <qevent.h>

#define RS_MouseEvent QMouseEvent

/**
 * Events which can be triggered for every action.
 */
//enum RS_MouseButton { LEFT, MIDDLE, RIGHT, NONE };

/**
 * Mouse Events.
 */
/*
class RS_MouseEvent {
public:
    RS_MouseEvent(int x, int y, RS_MouseButton button) {
        this->x = x;
        this->y = y;
        this->button = button;
    }
 
    int getX() {
        return x;
    }
    int getY() {
        return y;
    }
    RS_MouseButton getButton() {
        return button;
    }
 
protected:
    int x;
    int y;
    RS_MouseButton button;
};
*/

#endif
