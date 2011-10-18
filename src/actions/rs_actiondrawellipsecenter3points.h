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

#ifndef RS_ACTIONDRAWELLIPSECENTER3POINTS_H
#define RS_ACTIONDRAWELLIPSECENTER3POINTS_H

#include "rs_previewactioninterface.h"
#include "rs_ellipse.h"

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawEllipseCenter3Points : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCenter,   //  Setting the Center.  */
        SetPoint1,   //  Setting the First Point.  */
        SetPoint2,   //  Setting the Second Point.  */
        SetPoint3   //  Setting the Third Point.  */
    };

public:
    RS_ActionDrawEllipseCenter3Points(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);
    ~RS_ActionDrawEllipseCenter3Points();

        static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

    virtual void init(int status=0);

    virtual void trigger();
    virtual bool preparePreview();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

        virtual void coordinateEvent(RS_CoordinateEvent* e);
//    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

protected:
    // 4 points on ellipse
    RS_VectorSolutions points;
    private:
    RS_CircleData cData;
    RS_EllipseData eData;
    bool valid;
};

#endif
