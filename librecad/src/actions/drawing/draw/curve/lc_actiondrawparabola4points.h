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

#ifndef LC_ActionDrawParabola4Points_H
#define LC_ActionDrawParabola4Points_H

#include "rs_previewactioninterface.h"

class RS_Vector;

/**
 * Draw a Parabola by 4 points on parabola
 *
 * @author Dongxu Li
 */
class LC_ActionDrawParabola4Points : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    LC_ActionDrawParabola4Points(LC_ActionContext *actionContext);
    ~LC_ActionDrawParabola4Points() override;
    void init(int status) override;
    QStringList getAvailableCommands() override;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetPoint1 = 0,   //  Setting the First Point.  */
        SetPoint2,   //  Setting the Second Point.  */
        SetPoint3,   //  Setting the Third Point.  */
        SetPoint4,   //  Setting the Last Point.  */
        SetAxis   //  select the axis/orientation  */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    bool preparePreview(const RS_Vector& mouse, bool rebuild);
    void doTrigger() override;
};
#endif
