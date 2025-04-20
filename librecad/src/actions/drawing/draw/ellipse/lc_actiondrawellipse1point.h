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
    LC_ActionDrawEllipse1Point(LC_ActionContext *actionContext, bool isArc);
    ~LC_ActionDrawEllipse1Point() override;
    void init(int status) override;
    double getMajorRadius();
    double getMinorRadius();
    double getUcsMajorAngleDegrees();
    bool hasAngle();
    bool isAngleFree();
    void setMajorRadius(double val);
    void setMinorRadius(double val);
    void setUcsMajorAngleDegrees(double ucsBasisAngleDegrees);
    void setHasAngle(bool val);
    void setAngleFree(bool val);
    bool isReversed() const override;
    void setReversed(bool b) const override;
    QStringList getAvailableCommands() override;
protected:
enum Status{
        SetPoint,
        SetMajorAngle,
        SetAngle1,
        SetAngle2
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_ActionData;
    void toSetPointStatus();
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHints() override;
    bool doProcessCommand(int status, const QString &command) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void doTrigger() override;

    RS_Vector getMajorP();
};

#endif // LC_ACTIONDRAWELLIPSE1POINT_H
