/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_EVENTHANDLER_H
#define RS_EVENTHANDLER_H

#include "rs_actioninterface.h"
//#include "rs_actiondrawpoint.h"

#include "rs_event.h"
#include "rs_keyevent.h"
#include "rs_mouseevent.h"

#define RS_MAXACTIONS 16

class RS_ActionInterface;

/**
 * The event handler owns and manages all actions that are currently 
 * active. All events going from the view to the actions come over
 * this class.
 */
class RS_EventHandler {
public:
    RS_EventHandler(RS_GraphicView* graphicView);
    ~RS_EventHandler();

    void back();
    void enter();

    void mousePressEvent(RS_MouseEvent *e);
    void mouseReleaseEvent(RS_MouseEvent *e);
    void mouseMoveEvent(RS_MouseEvent *e);
    void mouseLeaveEvent();
    void mouseEnterEvent();

    void keyPressEvent(RS_KeyEvent* e);
    void keyReleaseEvent(RS_KeyEvent* e);

	void commandEvent(RS_CommandEvent* e);
	void enableCoordinateInput();
	void disableCoordinateInput();

    void setDefaultAction(RS_ActionInterface* action);
    RS_ActionInterface* getDefaultAction();
	
    void setCurrentAction(RS_ActionInterface* action);
    RS_ActionInterface* getCurrentAction();
	
    void killSelectActions();
    void killAllActions();
	
    bool hasAction();
    void cleanUp();
    void debugActions();
    void setSnapMode(RS2::SnapMode sm);
    void setSnapRestriction(RS2::SnapRestriction sr);

protected:
	RS_GraphicView* graphicView;
	RS_ActionInterface* defaultAction;
    RS_ActionInterface* currentActions[RS_MAXACTIONS];
    int actionIndex;
	bool coordinateInputEnabled;
};

#endif
