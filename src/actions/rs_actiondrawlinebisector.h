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

#ifndef RS_ACTIONDRAWLINEBISECTOR_H
#define RS_ACTIONDRAWLINEBISECTOR_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to draw bisectors.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineBisector : public RS_PreviewActionInterface {
	Q_OBJECT
private:
    enum Status {
        SetLine1,     /**< Choose the 1st line. */
        SetLine2,     /**< Choose the 2nd line. */
		SetLength,    /**< Set length in command line. */
		SetNumber     /**< Set number in command line. */
    };

public:
    RS_ActionDrawLineBisector(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionDrawLineBisector() {}

	static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDrawLineBisector;
	}

    virtual void trigger();
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);

    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();
	
	void setLength(double l) {
		length = l;
	}

	double getLength() {
		return length;
	}
	
	void setNumber(int n) {
		number = n;
	}

	int getNumber() {
		return number;
	}

private:
    /** Closest bisector. */
    RS_Line* bisector;
    /** First chosen entity */
    RS_Line* line1;
    /** Second chosen entity */
    RS_Line* line2;
    /** Data of new bisector */
    RS_LineData data;
    /** Length of the bisector. */
    double length;
	/** Number of bisectors to create. */
	int number;
	/** Mouse pos when choosing the 1st line */
	RS_Vector coord1;
	/** Mouse pos when choosing the 2nd line */
	RS_Vector coord2;
	/** Last status before entering length or number. */
	Status lastStatus;
};

#endif
