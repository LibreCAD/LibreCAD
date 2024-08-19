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

#include "lc_actiondrawcirclebase.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Draw tangential circle passing 2 points
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan1_2P:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleTan1_2P(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawCircleTan1_2P() override;
    void init(int status) override;
    void trigger() override;
    bool getCenters();
    bool preparePreview();
    void mouseMoveEvent(QMouseEvent *e) override;
    //        void commandEvent(RS_CommandEvent* e) override;
    void finish(bool updateTB) override;
    //    void setRadius(const double& r);
    double getRadius() const;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1 = 0, //  Setting the First Circle.  */
        SetPoint1 = 1, //  Setting the First Point.  */
        SetPoint2 = 2, //  Setting the Second Point.  */
        SetCenter //  Setting the internal or external tangent circle's center.  */
    };
    struct Points;
    std::unique_ptr<Points> pPoints;

    RS_Entity *catchCircle(QMouseEvent *e);
    RS_AtomicEntity *baseEntity = nullptr;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    RS_Vector getTangentPoint(RS_Vector &creatingCircleCenter, bool fromOriginalCircle) const;
    void updateMouseButtonHints() override;
};
#endif
