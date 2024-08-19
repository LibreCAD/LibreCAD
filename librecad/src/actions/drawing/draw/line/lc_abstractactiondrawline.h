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

#include "rs_line.h"
#include "rs_vector.h"
#include "rs_previewactioninterface.h"
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

    LC_AbstractActionDrawLine(const char* name, RS_EntityContainer &container,RS_GraphicView &graphicView);
    ~LC_AbstractActionDrawLine() override;
    int getDirection() const{return direction;}
    void setNewStartPointState();
    void setSetAngleDirectionState();
    void setSetPointDirectionState();
    void setSetXDirectionState();
    void setSetYDirectionState();
    void setAngleValue(double value);
    void setAngle(double value){angle = value;};
    double getAngle() const;
    bool isAngleRelative() const;
    void setAngleIsRelative(bool value);
    virtual bool mayStart();
    bool doProcessCommand(int status, const QString &c) override;
protected:
    // action statuses
    enum Status {
        SetStartPoint,
        SetDirection,
        SetDistance,
        SetPoint,
        SetAngle,
        LAST
    };


    double angle = 0.0; // fixed angle for line
    bool angleIsRelative = true; // is angle relative to previous segment (if any)
    int direction = DIRECTION_NONE; // current line direction
    int primaryDirection = DIRECTION_NONE; // major direction of line - used for subsequent lines

    void setSetAngleState(bool relative);
    virtual bool processAngleValueInput(const QString &c);
    virtual bool doProceedCommand(int status, const QString &qString);
    virtual bool doProcessCommandValue(int status, const QString &c);
    virtual const RS_Vector& getStartPointForAngleSnap() const = 0;
    virtual bool isStartPointValid() const;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapped) override;
    bool doCheckMayDrawPreview(QMouseEvent *pEvent, int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
    virtual void doSetStartPoint(RS_Vector vector) = 0;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    void setStatusForValidStartPoint(int newStatus);
};
#endif //LIBRECAD_LC_ABSTRACTACTIONDRAWLINE_H
