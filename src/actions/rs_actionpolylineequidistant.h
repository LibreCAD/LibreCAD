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
#ifndef RS_ACTIONPOLYLINEEQUIDISTANT_H
#define RS_ACTIONPOLYLINEEQUIDISTANT_H

#include "rs_previewactioninterface.h"
#include "rs_modification.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineEquidistant : public RS_PreviewActionInterface {
	Q_OBJECT
public:
	/**
	 * Action States.
	 */
	enum Status {
		ChooseEntity			/**< Choosing the original polyline. */
	};

public:
	RS_ActionPolylineEquidistant(RS_EntityContainer& container,
						RS_GraphicView& graphicView);
	~RS_ActionPolylineEquidistant() {}
	virtual RS2::ActionType rtti() {
		return RS2::ActionPolylineEquidistant;
	}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	virtual void init(int status=0);
	
	virtual void trigger();

	virtual void mouseMoveEvent(RS_MouseEvent* e);
	virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void updateMouseButtonHints();
	virtual void updateMouseCursor();
	virtual void updateToolBar();
	virtual void showOptions();
	virtual void hideOptions();

	void setDist(double d) {
		dist = d;
	}

	double getDist() {
		return dist;
	}

	void setNumber(int n) {
		number = n;
	}

	int getNumber() {
		return number;
	}

	bool makeContour();

private:
	RS_Entity* originalEntity;
	RS_Vector targetPoint;
	double dist;
	int number;
	bool bRightSide;
};

#endif
