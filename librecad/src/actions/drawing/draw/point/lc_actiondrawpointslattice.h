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

#ifndef LC_ACTIONDRAWPOINTSLATTICE_H
#define LC_ACTIONDRAWPOINTSLATTICE_H

#include "rs_previewactioninterface.h"

class LC_ActionDrawPointsLattice:public RS_PreviewActionInterface{
   Q_OBJECT
public:
    LC_ActionDrawPointsLattice(LC_ActionContext *actionContext);
    int getColumnPointsCount() const;
    void setColumnPointsCount(int pointsByX);
    int getRowPointsCount() const;
    void setRowPointsCount(int pointsByY);
    QStringList getAvailableCommands() override;
    bool isAdjustLastPointToFirst() const;
    void setAdjustLastPointToFirst(bool adjustLastPointToFirst);
protected:
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;

    enum State{
        SetPoint1,
        SetPoint2,
        SetPoint3,
        SetPoint4,
        SetNumXPoints,
        SetNumYPoints
    };
    RS_Vector m_point1;
    RS_Vector m_point2;
    RS_Vector m_point3;
    RS_Vector m_point4;

    int m_pointsAmountByX = 5;
    int m_pointsAmountByY = 5;

    bool m_adjustLastPointToFirst = false;
    int m_majorStatus;
    void createPointsLine(RS_Vector start, RS_Vector end, int count, QVector<RS_Vector> &points);
    void createPointsLattice(RS_Vector vector, QVector<RS_Vector> &points);
    RS_Vector getLastPointPosition(RS_Vector &pos, bool alternate) const;
    void doTrigger() override;
};

#endif // LC_ACTIONDRAWPOINTSLATTICE_H
