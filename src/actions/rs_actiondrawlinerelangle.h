/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#ifndef RS_ACTIONDRAWLINERELANGLE_H
#define RS_ACTIONDRAWLINERELANGLE_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to draw lines with a given angle
 * to a given entity.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineRelAngle : public RS_PreviewActionInterface {
	Q_OBJECT
private:
    enum Status {
        SetEntity,     /**< Choose entity. */
        SetPos,        /**< Choose position. */
		SetAngle,      /**< Set angle in console. */
		SetLength      /**< Set length in console. */
    };

public:
    RS_ActionDrawLineRelAngle(RS_EntityContainer& container,
                              RS_GraphicView& graphicView,
                              double angle=0.0,
                              bool fixedAngle=false);
    ~RS_ActionDrawLineRelAngle() {}
	
	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawLineRelAngle;
	}

    virtual void trigger();

    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();
	
	void setAngle(double a) {
		angle = a;
	}

	double getAngle() {
		return angle;
	}

	void setLength(double l) {
		length = l;
	}

	double getLength() {
		return length;
	}

	bool hasFixedAngle() {
		return fixedAngle;
	}

private:
    /** new line */
    //RS_Line* line;
    /** Chosen entity */
    RS_Entity* entity;
    /** Chosen position */
    RS_Vector pos;
    /** Data of new line */
    RS_LineData data;
    /**
     * Line angle.
     */
    double angle;
    /**
     * Line length.
     */
    double length;
    /**
     * Is the angle fixed?
     */
    bool fixedAngle;
};

#endif
