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
#ifndef RS_ACTIONPOLYLINEAPPEND_H
#define RS_ACTIONPOLYLINEAPPEND_H

//#include "rs_previewactioninterface.h"
//#include "rs_modification.h"
//#include "rs_polyline.h"
#include "rs_actiondrawpolyline.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineAppend : public RS_ActionDrawPolyline {//public RS_PreviewActionInterface {
	Q_OBJECT
public:
	/**
	 * Action States.
	 */
//	enum Status {
//		SetStartpoint,   /**< Setting the startpoint.  */
//		SetNextPoint	  /**< Setting the endpoint. */
//	};

public:
	RS_ActionPolylineAppend(RS_EntityContainer& container,
						RS_GraphicView& graphicView);
//	~RS_ActionPolylineAppend();

	virtual RS2::ActionType rtti() {
		return RS2::ActionPolylineAppend;
	}
	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

//	void reset();

//	virtual void init(int status=0);
	
	virtual void trigger();
	
//	virtual void mouseMoveEvent(RS_MouseEvent* e);
	virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);
//  virtual void commandEvent(RS_CommandEvent* e);
//	virtual RS_StringList getAvailableCommands();

//	virtual void showOptions();
//	virtual void hideOptions();

	virtual void updateMouseButtonHints();
//  virtual void updateMouseCursor();
//	virtual void updateToolBar();
//	void close();
//	void undo();

private:
	/**
	 * Line data defined so far.
	 */
//	RS_PolylineData data;
	
	/**
	 * Polyline entity we're working on.
	 */
//	RS_Polyline* polyline;
	RS_Polyline* originalPolyline;
	bool prepend;
	/**
	 * last point.
	 */
//	RS_Vector point;

	/**
	 * Start point of the series of lines. Used for close function.
	 */
//	RS_Vector start;

	/**
	 * Point history (for undo)
	 */
//	RS_PtrList<RS_Vector*> history;
	
	/**
	 * Bulge history (for undo)
	 */
//	RS_PtrList<double*> bHistory;

};

#endif
