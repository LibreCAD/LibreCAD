/****************************************************************************
**
 * Draw a tangential circle of two given circles, with given radius

Copyright (C) 2012-2015 Dongxu Li (dongxuli2011@gmail.com)
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

#ifndef RS_ACTIONDRAWCIRCLETAN2_H
#define RS_ACTIONDRAWCIRCLETAN2_H


#include "lc_actiondrawcirclebase.h"

class RS_AtomicEntity;
struct RS_CircleData;

/**
 * Draw a circle tangential to two give circles and with radius
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan2:public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawCircleTan2(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionDrawCircleTan2() override;
    void init(int status) override;
    void trigger() override;
    bool getCenters(RS_Entity* secondEntityCandidate = nullptr);
    bool preparePreview();
    void mouseMoveEvent(QMouseEvent *e) override;

//        void coordinateEvent(RS_CoordinateEvent* e) override;
//    void commandEvent(RS_CommandEvent* e) override;
    void finish(bool updateTB) override;
    void setRadius(double);
    double getRadius() const;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetCircle1,   //  Setting the First Circle.  */
        SetCircle2,   //  Setting the Second Circle.  */
        SetCenter   //  select the closest tangential Circle.  */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;

    RS_Entity *catchCircle(QMouseEvent *e);
    RS_Vector getTangentPoint(RS_Vector creatingCircleCenter, double creatingCircleRadius, const RS_AtomicEntity * circle);
    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};
#endif
