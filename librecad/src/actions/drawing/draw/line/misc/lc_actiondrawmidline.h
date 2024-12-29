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

#ifndef LC_ACTIONDRAWMIDLINE_H
#define LC_ACTIONDRAWMIDLINE_H

#include <QObject>
#include "rs_previewactioninterface.h"

class LC_ActionDrawMidLine: public RS_PreviewActionInterface
{
    Q_OBJECT
public:
    LC_ActionDrawMidLine(RS_EntityContainer &container, RS_GraphicView &graphicView);
    void mouseMoveEvent(QMouseEvent *event) override;
    QStringList getAvailableCommands() override;
    double getOffset() const;
    void setOffset(double offset);
    void init(int status) override;
protected:
    LC_ActionOptionsWidget *createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
protected:
    enum State{
        SetEntity1,
        SetEntity2,
        SetOffset
    };

    struct LineInfo{
        RS_Vector middlePoint1;
        RS_Vector middlePoint2;
        RS_Line* line;

        RS_Vector start1;
        RS_Vector start2;
        RS_Vector end1;
        RS_Vector end2;
    };

    double offset = 0.0;
    bool alternateEndpoints = false;

    RS_Entity* firstEntity = nullptr;
    RS_Entity* secondEntity = nullptr;

    int mainStatus = 0;
    void restoreMainStatus(){setStatus(mainStatus);}
    void prepareLine(LineInfo &info, RS_Entity* ent, bool alternate);
    void doTrigger() override;
};

#endif // LC_ACTIONDRAWMIDLINE_H
