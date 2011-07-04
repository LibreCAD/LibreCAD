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

#ifndef RS_ACTIONSELECT_H
#define RS_ACTIONSELECT_H

#include "rs_actioninterface.h"


/**
 * This action class can handle user events to select entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionSelect : public RS_ActionInterface {
	Q_OBJECT
public:
    RS_ActionSelect(RS_EntityContainer& container,
                    RS_GraphicView& graphicView,
                    RS2::ActionType nextAction);
    ~RS_ActionSelect() {}

    void init(int status);
    //virtual void keyPressEvent(RS_KeyEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void updateToolBar();

private:
    RS2::ActionType nextAction;
};

#endif
