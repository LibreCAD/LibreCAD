/****************************************************************************
** $Id: rs_actiondrawpolyline.h,v 1.4 2004/07/21 22:44:34 andrew Exp $
**
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2009 Jani Frilander.
**
** This file is part of the qcadlib Library project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**********************************************************************/
// New drawing modes added by Jani Frilander 2009

#ifndef RS_ACTIONDRAWPOLYLINE_H
#define RS_ACTIONDRAWPOLYLINE_H

#include "rs_previewactioninterface.h"
#include "rs_polyline.h"

#include <qaction.h>


/**
 * This action class can handle user events to draw 
 * simple lines with the start- and endpoint given.
 *
 * @author Andrew Mustun
*/
class RS_ActionDrawPolyline : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetStartpoint,   /**< Setting the startpoint.  */
        SetNextPoint      /**< Setting the endpoint. */
    };

    enum Mode {
	Line,
	Tangential,
	TanRad,
//	TanAng,
//	TanRadAng,
	Ang,
//	RadAngEndp,
//	RadAngCenp
    };

public:
    RS_ActionDrawPolyline(RS_EntityContainer& container,
                      RS_GraphicView& graphicView);
    virtual ~RS_ActionDrawPolyline();

	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawPolyline;
	}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
    void reset();

    virtual void init(int status=0);
    virtual void trigger();
	
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);
	
	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();
	
	virtual void showOptions();
	virtual void hideOptions();
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

	void close();
	void undo();

	void setMode(int m) {
		Mode=m;
	}

	int getMode() {
		return Mode;
	}

	void setRadius(double r) {
        	Radius=r;
	}

        double getRadius() {
		return Radius;
        }

        void setAngle(double a) {
        	Angle=a;
	}

        double getAngle() {
        	return Angle;
	}

	void setReversed( bool c) {
		if (c)
			Reversed = -1;
		else
			Reversed = 1;
	}

	bool isReversed() {
		if(Reversed==-1)
		  return true;
		else
		  return false;
	}

	double solveBulge(RS_Vector mouse);

protected:
    double Radius;
    double Angle;
    int Mode;
    int Reversed;
    bool calculatedSegment;

    /**
     * Line data defined so far.
     */
    RS_PolylineData data;
    RS_ArcData arc_data;	
    /**
     * Polyline entity we're working on.
     */
    RS_Polyline* polyline;
	
    /**
     * last point.
     */
    RS_Vector point;
    RS_Vector calculatedEndpoint;
	/**
	 * Start point of the series of lines. Used for close function.
	 */
	RS_Vector start;

	/**
	 * Point history (for undo)
	 */
	RS_PtrList<RS_Vector> history;
	
	/**
	 * Bulge history (for undo)
	 */
	RS_PtrList<double> bHistory;
};

#endif
