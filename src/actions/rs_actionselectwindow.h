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

#ifndef RS_ACTIONSELECTWINDOW_H
#define RS_ACTIONSELECTWINDOW_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"


/**
 * This action class can handle user events to select all entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionSelectWindow : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCorner1,     /**< Setting the 1st corner of the window.  */
        SetCorner2      /**< Setting the 2nd corner of the window. */
    };

public:
    RS_ActionSelectWindow(RS_EntityContainer& container,
                         RS_GraphicView& graphicView,
                         bool select);
    ~RS_ActionSelectWindow() {}

	static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

	virtual RS2::ActionType rtti() {
		return RS2::ActionSelectWindow;
	}

    virtual void init(int status=0);
	
    virtual void trigger();
	
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mousePressEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

protected:
    RS_Vector v1;
    RS_Vector v2;

    bool select;
};

#endif
