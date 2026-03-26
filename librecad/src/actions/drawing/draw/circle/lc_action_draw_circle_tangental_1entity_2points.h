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

#ifndef RS_ACTIONDRAWCIRCLETAN1_2P_H
#define RS_ACTIONDRAWCIRCLETAN1_2P_H

#include "lc_action_draw_circle_base.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Draw tangential circle passing 2 points
 *
 * @author Dongxu Li
 */
class LC_ActionDrawCircleTangental1Entity2Points : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    explicit LC_ActionDrawCircleTangental1Entity2Points(LC_ActionContext* actionContext);
    ~LC_ActionDrawCircleTangental1Entity2Points() override;
    void init(int status) override;
    bool getCenters() const;
    bool preparePreview() const;
    void finish() override;
    double getRadius() const;

protected:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1 = InitialActionStatus, //  Setting the First Circle.  */
        SetPoint1  = 1, //  Setting the First Point.  */
        SetPoint2  = 2, //  Setting the Second Point.  */
        SetCenter  = 3 //  Setting the internal or external tangent circle's center.  */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    RS_AtomicEntity* m_baseEntity = nullptr;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    void doInitialInit() override;
    RS_Entity* catchTangentEntity(const LC_MouseEvent* e, bool forPreview) const;
    void setCircleOne(RS_Entity* en);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& coord) override;
    RS_Vector getTangentPoint(const RS_Vector& creatingCircleCenter, bool fromOriginalCircle) const;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
