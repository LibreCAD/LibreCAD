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

#ifndef RS_ACTIONDRAWELLIPSEAXIS_H
#define RS_ACTIONDRAWELLIPSEAXIS_H

#include "rs_previewactioninterface.h"
#include "rs_ellipse.h"

/**
 * This action class can handle user events to draw ellipses
 * with a center point and the endpoints of minor and major axis.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawEllipseAxis : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCenter,   /**< Settinge the center.  */
        SetMajor,    /**< Setting endpoint of major axis. */
        SetMinor,    /**< Setting minor/major ratio. */
        SetAngle1,   /**< Setting start angle. */
        SetAngle2    /**< Setting end angle. */
    };

public:
    RS_ActionDrawEllipseAxis(RS_EntityContainer& container,
                             RS_GraphicView& graphicView,
                             bool isArc);
    ~RS_ActionDrawEllipseAxis();
	
    static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);
    RS2::ActionType rtti(){
        return actionType;
    }

    virtual void init(int status=0);
	
    virtual void trigger();
	
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

	virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
//    virtual void updateToolBar();

protected:
    /** Center of ellipse */
    RS_Vector center;
    /** Endpoint of major axis */
    RS_Vector major;
    /** Ratio major / minor */
    double ratio;
    /** Start angle */
    double angle1;
    /** End angle */
    double angle2;
    /** Do we produce an arc (true) or full ellipse (false) */
    bool isArc;
};

#endif
