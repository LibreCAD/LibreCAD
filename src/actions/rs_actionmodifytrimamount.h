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

#ifndef RS_ACTIONMODIFYTRIMAMOUNT_H
#define RS_ACTIONMODIFYTRIMAMOUNT_H

#include "rs_previewactioninterface.h"
#include "rs_modification.h"


/**
 * This action class can handle user events to trim entities by a given
 * amount.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyTrimAmount : public RS_ActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        ChooseTrimEntity      /**< Choosing the entity to trim. */
    };

public:
    RS_ActionModifyTrimAmount(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionModifyTrimAmount() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	virtual RS2::ActionType rtti() {
		return RS2::ActionModifyTrimAmount;
	}

    virtual void init(int status=0);
	
    virtual void trigger();

    //virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();

    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();
	
	double getDistance() {
		return distance;
	}

	void setDistance(double d) {
		distance = d;
	}

private:
    RS_Entity* trimEntity;
	RS_Vector trimCoord;
	double distance;
	/**
	 * Commands
	 */
	/*
	RS_String cmdDistance;
	RS_String cmdDistance2;
	RS_String cmdDistance3;
	*/
};

#endif
