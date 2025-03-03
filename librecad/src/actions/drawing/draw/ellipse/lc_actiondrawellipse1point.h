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

#ifndef LC_ACTIONDRAWELLIPSE1POINT_H
#define LC_ACTIONDRAWELLIPSE1POINT_H

#include "lc_actiondrawcirclebase.h"

class LC_ActionDrawEllipse1Point:public LC_ActionDrawCircleBase{
Q_OBJECT
public:
    LC_ActionDrawEllipse1Point(
        RS_EntityContainer &container, RS_GraphicView &graphicView, bool isArc);

    ~LC_ActionDrawEllipse1Point() override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void trigger() override;
    void init(int status) override;

    double getMajorRadius();
    double getMinorRadius();
    double getAngle();
    bool hasAngle();
    bool isAngleFree();
    void setMajorRadius(double val);
    void setMinorRadius(double val);
    void setAngle(double val);
    void setHasAngle(bool val);
    void setAngleFree(bool val);
    bool isReversed() const override;
    void setReversed(bool b) const override;
protected:
    struct Points;

    enum Status{
        SetPoint,
        SetMajorAngle,
        SetAngle1,
        SetAngle2
    };

    std::unique_ptr<Points> pPoints;
    void toSetPointStatus();
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHints() override;
    bool doProcessCommand(int status, const QString &command) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};

#endif // LC_ACTIONDRAWELLIPSE1POINT_H
