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

#ifndef RS_ACTIONDIMALIGNED_H
#define RS_ACTIONDIMALIGNED_H

#include "rs_actiondimension.h"
#include "rs_dimaligned.h"

/**
 * This action class can handle user events to draw 
 * aligned dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimAligned : public RS_ActionDimension {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetExtPoint1,    /**< Setting the 1st ext point.  */
        SetExtPoint2,    /**< Setting the 2nd ext point. */
        SetDefPoint,     /**< Setting the common def point */
		SetText          /**< Setting the text label in command line */
    };

public:
    RS_ActionDimAligned(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionDimAligned();

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDimAligned;
	}

    virtual void reset();

    virtual void trigger();
	void preparePreview();
	
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();

protected:
    /**
     * Aligned dimension data.
     */
    RS_DimAlignedData edata;

	/** Last status before entering text. */
	Status lastStatus;
}
;

#endif
