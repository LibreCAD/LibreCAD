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

#ifndef RS_ACTIONSETRELATIVEZERO_H
#define RS_ACTIONSETRELATIVEZERO_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to set
 * the relative Zero point.
 * It overrides but retains the locking of the relative Zero.
 *
 * @author Ulf Lehnert
 */
class RS_ActionSetRelativeZero : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    RS_ActionSetRelativeZero(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);
    ~RS_ActionSetRelativeZero() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	virtual RS2::ActionType rtti() {
		return RS2::ActionSetRelativeZero;
	}

    virtual void trigger();
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

private:
    RS_Vector pt;
};

#endif
