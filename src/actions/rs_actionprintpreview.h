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

#ifndef RS_ACTIONPRINTPREVIEW_H
#define RS_ACTIONPRINTPREVIEW_H

#include "rs_actioninterface.h"

/**
 * Default action for print preview.
 *
 * @author Andrew Mustun
 */
class RS_ActionPrintPreview : public RS_ActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
		Neutral,
		Moving
    };

public:
    RS_ActionPrintPreview(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionPrintPreview();

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

	virtual RS2::ActionType rtti() {
		return RS2::ActionPrintPreview;
	}

    virtual void init(int status=0);

    virtual void trigger();

    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mousePressEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);

    virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();

	virtual void showOptions();
	virtual void hideOptions();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

	void center();
	void fit();
	void setScale(double f);
	double getScale();

	void setBlackWhite(bool bw);
	//bool isBlackWhite() {
	//	return blackWhite;
	//}
	RS2::Unit getUnit();

protected:
	//bool blackWhite;
	RS_Vector v1;
	RS_Vector v2;
};

#endif
