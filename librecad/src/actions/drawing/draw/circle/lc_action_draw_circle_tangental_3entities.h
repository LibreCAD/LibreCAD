/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/

#ifndef RS_ACTIONDRAWCIRCLETAN3_H
#define RS_ACTIONDRAWCIRCLETAN3_H

#include "lc_action_draw_circle_base.h"

struct RS_CircleData;
class RS_AtomicEntity;

/**
 * Draw Common tangential circle of 3 given circles, i.e. Appollonius's problem
 *
 * @author Dongxu Li
 */
class LC_ActionDrawCircleTangental3Entities : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    explicit LC_ActionDrawCircleTangental3Entities(LC_ActionContext* actionContext);
    ~LC_ActionDrawCircleTangental3Entities() override;
    void init(int status) override;
    //    void coordinateEvent(RS_CoordinateEvent* e) override;
    //    void commandEvent(RS_CommandEvent* e) override;
    void finish() override;

protected:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1 = InitialActionStatus, //  Setting the First Circle.  */
        SetCircle2, //  Setting the Second Circle.  */
        SetCircle3, //  Setting the Third Circle.  */
        SetCenter //  select the closest tangential Circle.  */
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    void doInitialInit() override;

    void drawSnapper() override;
    bool preparePreview() const;
    RS_Entity* catchCircle(const LC_MouseEvent* e, bool forPreview) const;
    void setCircle(RS_Entity* en, int status);
    bool getData(RS_Entity* testThirdEntity = nullptr) const;
    RS_Vector getTangentPoint(const RS_Vector& creatingCircleCenter, double creatingCircleRadius, const RS_AtomicEntity* circle);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
};
#endif
