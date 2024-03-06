/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)

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

#ifndef LC_ActionDrawParabolaFD_H
#define LC_ActionDrawParabolaFD_H

#include "rs_previewactioninterface.h"

class RS_Vector;

/**
 * Draw a Parabola by focus and directrix
 *
 * @author Dongxu Li
 */
class LC_ActionDrawParabolaFD : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetFocus = 0,   //  Setting the Focus.  */
        SetDirectrix,   //  Setting the Directrix.  */
        SetStartPoint,  //  Setting the Start Point.  */
        SetEndPoint     //  Setting the End Point.  */
    };

public:
    LC_ActionDrawParabolaFD(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
    ~LC_ActionDrawParabolaFD() override;

    void init(int status=0) override;

    void trigger() override;
    bool preparePreview();

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    //    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

protected:
    struct Points;
    std::unique_ptr<Points> pPoints;
};

#endif
