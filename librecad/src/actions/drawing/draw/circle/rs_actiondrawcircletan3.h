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

#include<vector>
#include "lc_actiondrawcirclebase.h"

struct RS_CircleData;
class RS_AtomicEntity;

/**
 * Draw Common tangential circle of 3 given circles, i.e. Appollonius's problem
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan3:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleTan3(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawCircleTan3() override;
    void init(int status) override;
    void trigger() override;
    bool preparePreview();
    void mouseMoveEvent(QMouseEvent *e) override;
//    void coordinateEvent(RS_CoordinateEvent* e) override;
//    void commandEvent(RS_CommandEvent* e) override;
    void finish(bool updateTB) override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1,   //  Setting the First Circle.  */
        SetCircle2,   //  Setting the Second Circle.  */
        SetCircle3,   //  Setting the Third Circle.  */
        SetCenter   //  select the closest tangential Circle.  */
    };
    struct Points;
    RS_Entity *catchCircle(QMouseEvent *e);
    std::unique_ptr<Points> pPoints;
    bool getData(RS_Entity *en = nullptr);
    RS_Vector getTangentPoint(RS_Vector creatingCircleCenter, double creatingCircleRadius, RS_AtomicEntity *pEntity);

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};
#endif
