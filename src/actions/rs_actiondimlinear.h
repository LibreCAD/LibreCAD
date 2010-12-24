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

#ifndef RS_ACTIONDIMLINEAR_H
#define RS_ACTIONDIMLINEAR_H

#include "rs_actiondimension.h"
#include "rs_dimlinear.h"

/**
 * This action class can handle user events to draw 
 * aligned dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimLinear : public RS_ActionDimension {
	Q_OBJECT
public:
	/**
	 * Varitions of this action.
	 */
	enum Variation {
		AnyAngle,
		Horizontal,
		Vertical
	};
	 
    /**
     * Action States.
     */
    enum Status {
        SetExtPoint1,    /**< Setting the 1st ext point.  */
        SetExtPoint2,    /**< Setting the 2nd ext point. */
        SetDefPoint,     /**< Setting the common def point. */
		SetText,         /**< Setting the text label in the command line. */
		SetAngle         /**< Setting the angle in the command line. */
    };

public:
    RS_ActionDimLinear(RS_EntityContainer& container,
                       RS_GraphicView& graphicView,
                       double angle=0.0, bool fixedAngle=false);
    ~RS_ActionDimLinear();
	
	static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionDimLinear;
	}

    void reset();

    virtual void trigger();
	void preparePreview();
	
    virtual void mouseMoveEvent(RS_MouseEvent* e);
    virtual void mouseReleaseEvent(RS_MouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
	virtual RS_StringList getAvailableCommands();
	
    virtual void hideOptions();
    virtual void showOptions();
	
    virtual void updateMouseButtonHints();

	double getAngle() {
		return edata.angle;
	}

	void setAngle(double a) {
		edata.angle = a;
	}

	bool hasFixedAngle() {
		return fixedAngle;
	}

protected:
    /**
     * Aligned dimension data.
     */
    RS_DimLinearData edata;
    /**
     * Is the angle fixed?
     */
    bool fixedAngle;

	/** Last status before entering text or angle. */
	Status lastStatus;
};

#endif
