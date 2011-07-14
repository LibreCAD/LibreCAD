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

#ifndef RS_ACTIONDRAWLINEPARALLELTHROUGH_H
#define RS_ACTIONDRAWLINEPARALLELTHROUGH_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to draw parallel 
 * lines, arcs and circles.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineParallelThrough : public RS_PreviewActionInterface {
	Q_OBJECT
private:
    enum Status {
        SetEntity,    /**< Choose original entity. */
		SetPos,       /**< Setting point for this parallel to go through. */
		SetNumber     /**< Setting number in the command line. */
	};
	
public:
    RS_ActionDrawLineParallelThrough(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionDrawLineParallelThrough() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawLineParallelThrough;
	}

    virtual void trigger();
	
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void updateMouseButtonHints();
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();

    virtual void updateMouseCursor();
    virtual void updateToolBar();

	int getNumber() {
		return number;
	}

	void setNumber(int n) {
		number = n;
	}

private:
    /** Closest parallel. */
    RS_Entity* parallel;
    /** Data of new line */
    RS_LineData data;
	/** Number of parallels. */
	int number;
	/** Coordinate of the mouse. */
	RS_Vector coord;
	/** Original entity. */
	RS_Entity* entity;
	/** Last status before entering length or number. */
	Status lastStatus;
	/**
	 * Commands
	 */
	/*
        QString cmdDistance;
        QString cmdDistance2;
        QString cmdDistance3;
	
        QString cmdNumber;
        QString cmdNumber2;
        QString cmdNumber3;
	*/
};

#endif
