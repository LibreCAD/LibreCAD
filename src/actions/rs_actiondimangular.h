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

#ifndef RS_ACTIONDIMANGULAR_H
#define RS_ACTIONDIMANGULAR_H

#include "rs_actiondimension.h"
#include "rs_dimangular.h"


/**
 * This action class can handle user events to draw angular dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimAngular : public RS_ActionDimension {
	Q_OBJECT
private:
    enum Status {
        SetLine1,      /**< Choose 1st line. */
        SetLine2,      /**< Choose 2nd line. */
        SetPos,        /**< Choose position. */
		SetText        /**< Setting text label in consle. */
    };

public:
    RS_ActionDimAngular(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionDimAngular() {}
	
	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	virtual RS2::ActionType rtti() {
		return RS2::ActionDimAngular;
	}

	virtual void reset();
	
    virtual void trigger();
	
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();

    virtual void updateMouseButtonHints();

private:
    /** 1st chosen line */
    RS_Line* line1;
    /** 2nd chosen line */
    RS_Line* line2;
	/** Center of arc */
	RS_Vector center;
    /** Data of new dimension */
    RS_DimAngularData edata;
	/** Last status before entering text. */
	Status lastStatus;
};

#endif
