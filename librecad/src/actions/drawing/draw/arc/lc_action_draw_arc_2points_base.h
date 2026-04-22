/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_ACTIONDRAWARC2POINTSBASE_H
#define LC_ACTIONDRAWARC2POINTSBASE_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class LC_ActionDrawArc2PointsBase:public LC_SingleEntityCreationAction{
    Q_OBJECT
public:
    bool isReversed() const;
    void setReversed(bool r);
    double getParameter() const;
    void setParameter(double val);
    QStringList getAvailableCommands() override;
protected:
    enum State{
        SetPoint1 = InitialActionStatus,
        SetPoint2,
        SetParameterValue
    };
    RS_Vector m_startPoint;
    RS_Vector m_endPoint;
    bool m_reversed = false;
    double m_parameterLen = 0.0;
    bool m_alternated = false;
    int m_savedState = SetPoint1;

    LC_ActionDrawArc2PointsBase(const QString& name, LC_ActionContext *actionContext, RS2::ActionType actionType = RS2::ActionNone);
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateActionPrompt() override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    virtual void onMouseLeftButtonReleaseForNonPointsStatus([[maybe_unused]]int status, [[maybe_unused]]RS_Vector vector, [[maybe_unused]] const LC_MouseEvent* e) {}
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    RS_Arc *createArc(int status, const RS_Vector& pos, bool reverse, bool reportErrors = false);
    virtual bool createArcData(RS_ArcData &data, int status, const RS_Vector& vector, bool alternate, bool reportErrors = false) = 0;
    virtual void doPreviewOnPoint2Custom(RS_Arc *pArc) = 0;
    void proceedFromSetPoint2();
    virtual QString getParameterCommand() = 0;
    virtual void setParameterValue(double r);
    virtual QString getParameterPromptValue() const = 0;
    virtual QString getAlternativePoint2Prompt() const;
    RS_Entity* doTriggerCreateEntity() override;
    void doTriggerCompletion(bool success) override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
