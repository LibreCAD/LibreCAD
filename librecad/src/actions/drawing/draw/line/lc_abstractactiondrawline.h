/****************************************************************************
**
* Abstract base class for actions that draws a line

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
**********************************************************************/

#ifndef LIBRECAD_LC_ABSTRACTACTIONDRAWLINE_H
#define LIBRECAD_LC_ABSTRACTACTIONDRAWLINE_H

#include "lc_abstractactionwithpreview.h"

/**
 * Utility base class for actions that draw a line
 */
class LC_AbstractActionDrawLine:public LC_AbstractActionWithPreview {
    Q_OBJECT
public:
    // direction in which line should be drawn
    enum Direction{
        DIRECTION_NONE, DIRECTION_X, DIRECTION_Y, DIRECTION_POINT, DIRECTION_ANGLE
    };

    LC_AbstractActionDrawLine(const char* name, LC_ActionContext *actionContext, RS2::ActionType actionType = RS2::ActionNone);
    ~LC_AbstractActionDrawLine() override;
    int getDirection() const{return m_direction;}
    void setNewStartPointState();
    void setSetAngleDirectionState();
    void setSetPointDirectionState();
    void setSetXDirectionState();
    void setSetYDirectionState();
    void setAngleValueDegrees(double value);
    void setAngleDegrees(double value){doSetAngleDegrees(value);}
    double getAngleDegrees() const;
    bool isAngleRelative() const;
    void setAngleIsRelative(bool value);
    virtual bool mayStart();
    bool doProcessCommand(int status, const QString &c) override;
protected:
    // action statuses
    enum Status {
        SetStartPoint = InitialActionStatus,
        SetDirection,
        SetDistance,
        SetPoint,
        SetAngle,
        LAST
    };


    double m_angleDegrees = 0.0; // fixed angle for line
    bool m_angleIsRelative = true; // is angle relative to previous segment (if any)
    int m_direction = DIRECTION_NONE; // current line direction
    int m_primaryDirection = DIRECTION_NONE; // major direction of line - used for subsequent lines

    void setSetAngleState(bool relative);
    virtual bool processAngleValueInput(const QString &c);
    virtual bool doProceedCommand(int status, const QString &qString);
    virtual bool doProcessCommandValue(int status, const QString &c);
    virtual const RS_Vector& getStartPointForAngleSnap() const = 0;
    virtual bool isStartPointValid() const;
    void doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapped) override;
    bool doCheckMayDrawPreview(LC_MouseEvent *pEvent, int status) override;
    RS_Vector doGetMouseSnapPoint(LC_MouseEvent *e) override;
    virtual void doSetStartPoint(RS_Vector vector) = 0;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    void setStatusForValidStartPoint(int newStatus);
    virtual bool isAllowDirectionCommands();
    void doSetAngleDegrees(double value);
};
#endif //LIBRECAD_LC_ABSTRACTACTIONDRAWLINE_H
