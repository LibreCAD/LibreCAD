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

#ifndef RS_ACTIONDRAWELLIPSECENTER3POINTS_H
#define RS_ACTIONDRAWELLIPSECENTER3POINTS_H

#include "lc_actiondrawcirclebase.h"

/**
 * Draw ellipse by center and 3 points on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawEllipseCenter3Points : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    RS_ActionDrawEllipseCenter3Points(RS_EntityContainer& container,
                                      RS_GraphicView& graphicView);
    ~RS_ActionDrawEllipseCenter3Points() override;
    void init(int status) override;
    void trigger() override;
    bool preparePreview();
    void mouseMoveEvent(QMouseEvent* e) override;
    QStringList getAvailableCommands() override;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetCenter=0,   //  Setting the Center.  */
        SetPoint1=1,   //  Setting the First Point.  */
        SetPoint2=2,   //  Setting the Second Point.  */
        SetPoint3=3   //  Setting the Third Point.  */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;

    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};
#endif
