/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef RS_ACTIONDRAWELLIPSEFOCIPOINT_H
#define RS_ACTIONDRAWELLIPSEFOCIPOINT_H

#include "rs_previewactioninterface.h"
#include "rs_ellipse.h"

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawEllipseFociPoint : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetFocus1,   //  Setting the first focus.  */
        SetFocus2,    //  Setting the second focus. */
        SetPoint    //  Setting a point on ellipse
    };

public:
    RS_ActionDrawEllipseFociPoint(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);
    ~RS_ActionDrawEllipseFociPoint();

        static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

    virtual void init(int status=0);

    virtual void trigger();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

        virtual void coordinateEvent(RS_CoordinateEvent* e);
//    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

protected:
    // Foci of ellipse
    RS_Vector focus1,focus2;
    // A point on ellipse
    RS_Vector point;
    private:
    RS_Vector center,major;
    double c; //hold half of distance between foci
    double d; //hold half of distance
};

#endif
