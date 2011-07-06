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

#ifndef RS_ACTIONDRAWARC_H
#define RS_ACTIONDRAWARC_H

#include "rs_previewactioninterface.h"
#include "rs_arc.h"

/**
 * This action class can handle user events to draw 
 * simple arcs with the center, radius, start- and endangle given.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawArc : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCenter,       /**< Setting the center point. */
        SetRadius,       /**< Setting the radius. */
        SetAngle1,       /**< Setting the startpoint.  */
        SetAngle2,       /**< Setting the endpoint. */
		SetIncAngle,     /**< Setting the included angle. */
		SetChordLength   /**< Setting carc chord length. */
    };

public:
    RS_ActionDrawArc(RS_EntityContainer& container,
                     RS_GraphicView& graphicView);
    ~RS_ActionDrawArc();
	
	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawArc;
	}

    void reset();

    virtual void init(int status=0);
    virtual void trigger();
	
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

	bool isReversed() {
		return data.reversed;
	}

	void setReversed(bool r) {
		data.reversed = r;
	}

protected:
    /**
     * Arc data defined so far.
     */
    RS_ArcData data;
	/**
	 * Commands
	 */
        /*QString cmdReversed;
        QString cmdReversed2;
        QString cmdReversed3;
	*/
};

#endif
