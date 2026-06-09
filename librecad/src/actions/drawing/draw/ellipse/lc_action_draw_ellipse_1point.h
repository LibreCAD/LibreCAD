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

#include "lc_action_draw_circle_base.h"

class LC_ActionDrawEllipse1Point : public LC_ActionDrawCircleBase {
    Q_OBJECT
public:
    LC_ActionDrawEllipse1Point(LC_ActionContext* actionContext, bool isArc);
    ~LC_ActionDrawEllipse1Point() override;
    void init(int status) override;
    double getMajorRadius() const;
    double getMinorRadius() const;
    double getUcsMajorAngleDegrees() const;
    bool hasAngle() const;
    bool isAngleFree() const;
    void setMajorRadius(double val) const;
    void setMinorRadius(double val) const;
    void setUcsMajorAngleDegrees(double ucsBasisAngleDegrees) const;
    void setUcsMajorAngle(double ucsBasisAngleRad) const;
    void setHasAngle(bool val);
    void setAngleFree(bool val);
    bool isReversed() const override;
    void setReversed(bool b) const override;
    QStringList getAvailableCommands() override;

protected:
    enum Status {
        SetPoint = InitialActionStatus,
        SetMajorAngle,
        SetAngle1,
        SetAngle2
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    void toSetPointStatus();
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateActionPrompt() override;
    bool doProcessCommand(int status, const QString& command) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    RS_Vector getMajorP() const;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
