/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_ACTIONDRAWCIRCLETAN2_1P_H
#define RS_ACTIONDRAWCIRCLETAN2_1P_H

#include "lc_actiondrawcirclebase.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Given two circles and a point, draw a common tangent circle passing the point
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan2_1P : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleTan2_1P(LC_ActionContext *actionContext);
    ~RS_ActionDrawCircleTan2_1P() override;
    void init(int status) override;
    bool getCenters();
    bool preparePreview();
    void finish(bool updateTB) override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1 = InitialActionStatus,   //  Setting the first circle.  */
        SetCircle2,   //  Setting the second circle.  */
        SetPoint,   //  Setting point on the desired circle.  */
        SetCenter
    };
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    void doInitialInit() override;
    RS_Entity* catchTangentEntity(LC_MouseEvent* e, bool forPreview);
    void setCircle(RS_Entity* en, int status);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;

    void doTrigger() override;
};
#endif
