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

#ifndef RS_ACTIONDIMDIAMETRIC_H
#define RS_ACTIONDIMDIAMETRIC_H

#include "rs_actiondimension.h"
#include "rs_dimdiametric.h"


/**
 * This action class can handle user events to draw diametric dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimDiametric : public RS_ActionDimension {
	Q_OBJECT
private:
    enum Status {
        SetEntity,     /**< Choose entity. */
        SetPos,        /**< Choose point. */
		SetText        /**< Setting text label in the command line. */
    };

public:
    RS_ActionDimDiametric(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionDimDiametric() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDimDiametric;
	}

	virtual void reset();
	
    virtual void trigger();
	void preparePreview();
	
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();

    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();

private:
    /** Chosen entity (arc / circle) */
    RS_Entity* entity;
	/** Chosen position */
	RS_Vector pos;
    /** Data of new dimension */
    RS_DimDiametricData edata;
	/** Last status before entering text. */
	Status lastStatus;
};

#endif
