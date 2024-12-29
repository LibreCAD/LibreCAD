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

#ifndef LC_ACTIONDRAWARC2POINTSBASE_H
#define LC_ACTIONDRAWARC2POINTSBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionDrawArc2PointsBase:public RS_PreviewActionInterface{
    Q_OBJECT
public:
    LC_ActionDrawArc2PointsBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    void mouseMoveEvent(QMouseEvent *event) override;
    bool isReversed() const;
    void setReversed(bool reversed);

    double getParameter() const;
    void setParameter(double parameter);
    QStringList getAvailableCommands() override;

protected:
    enum State{
        SetPoint1,
        SetPoint2,
        SetParameterValue
    };
    RS_Vector startPoint;
    RS_Vector endPoint;
    bool reversed = false;
    double parameterLen = 0.0;
    bool alternated = false;
    int savedState = SetPoint1;

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    virtual void onMouseLeftButtonReleaseForNonPointsStatus([[maybe_unused]]int status, [[maybe_unused]]RS_Vector vector, [[maybe_unused]]QMouseEvent *pEvent) {};
    LC_ActionOptionsWidget *createOptionsWidget() override;
    RS_Arc *createArc(int status, RS_Vector vector, bool reverse, bool reportErrors = false);
    virtual bool createArcData(RS_ArcData &data, int status, RS_Vector vector, bool alternate, bool reportErrors = false) = 0;
    virtual void doOnEntityNotCreated() {};
    virtual void doAfterTrigger(){};
    virtual void doPreviewOnPoint2Custom(RS_Arc *pArc) = 0;
    void proceedFromSetPoint2();
    virtual QString getParameterCommand() = 0;
    virtual void setParameterValue(double r);
    virtual QString getParameterPromptValue() const = 0;
    virtual QString getAlternativePoint2Prompt() const;

    void doTrigger() override;
};

#endif // LC_ACTIONDRAWARC2POINTSBASE_H
