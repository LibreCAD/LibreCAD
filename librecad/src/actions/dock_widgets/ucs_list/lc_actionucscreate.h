/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_ACTIONUCSCREATE_H
#define LC_ACTIONUCSCREATE_H

#include "rs_previewactioninterface.h"
#include "lc_ucs_mark.h"

class LC_ActionUCSCreate:public RS_PreviewActionInterface{
Q_OBJECT

public:
    LC_ActionUCSCreate(LC_ActionContext *actionContext);
    ~LC_ActionUCSCreate() override;
    double getAngle() const {return m_angle;};
    void setAngle(double mAngle) {m_angle = mAngle;};
    bool isFixedAngle() const {return m_fixedAngle;};
    void setFixedAngle(bool val) {m_fixedAngle = val;};
    bool isParentIsWcs() const {return m_parentIsWCS;};
    void setParentIsWcs(bool parentIsWcs) {m_parentIsWCS =parentIsWcs;};
    QStringList getAvailableCommands() override;
    double getCurrentAngle(){return m_currentAngle;}
protected:

    enum{
        SetOrigin,
        SetAngle
    };

    RS_Vector m_originPoint = RS_Vector(false);
    RS_Vector m_xAxisPoint = RS_Vector(false);
    double m_angle = 0.0;
    double m_currentAngle = 0.0;
    bool m_fixedAngle = false;
    bool m_parentIsWCS = true;
    LC_UCSMarkOptions m_ucsMarkOptions;

    void doTrigger() override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void showUCSMark(RS_Vector &point, double angle);
    void initFromSettings() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};

#endif // LC_ACTIONUCSCREATE_H
