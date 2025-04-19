/****************************************************************************
**
* Action that draws a line from given point to selected line.
* Created line may be either orthogonal to selected line, or be with
* some specified angle.

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

#ifndef LC_ACTIONDRAWLINEFROMPOINTTOLINE_H
#define LC_ACTIONDRAWLINEFROMPOINTTOLINE_H

#include "lc_abstractactionwithpreview.h"

/**
 * Actions draws line from given point to selected target line. Angle between target line and created line is either user defined
 * in range (-90..90 degrees) or fixed to 90 degrees (so line orthogonal to target line is created).
 *
 * The lengths of created line may be either fixed, or be from start point to the point of intersection of created line and target line.
 */
class LC_ActionDrawLineFromPointToLine:public LC_AbstractActionWithPreview{
    Q_OBJECT
public:
    LC_ActionDrawLineFromPointToLine(LC_ActionContext *actionContext);
    ~LC_ActionDrawLineFromPointToLine() override = default;
    void setLineSnapMode(int val) {m_lineSnapMode = val;};
    int getLineSnapMode() const{return m_lineSnapMode;};
    void setOrthogonal(bool value){m_orthogonalMode = value;};
    bool getOrthogonal() const{return m_orthogonalMode;};
    void setAngle(double ang){m_angle = ang;};
    double getAngle() const{return m_angle;};
    int getSizeMode() const{return m_sizeMode;};
    void setSizeMode(int mode){m_sizeMode = mode;};
    void setLength(double len){m_length = len;};
    double getLength() const{return m_length;}
    void setEndOffset(double len){m_endOffset = len;};
    double getEndOffset() const{return m_endOffset;}
protected:
    // action state
    enum{
        SetPoint,
        SelectLine
    };

    /**
     * How to determine the length of created line
     */
    enum{
        SIZE_INTERSECTION,
        SIZE_FIXED_LENGTH
    };

    /**
     * Positioning of created line relating to start point (if fixed length of line is used)
     */
    enum {
        SNAP_START,
        SNAP_MIDDLE,
        SNAP_END
    };

    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doCheckMayDrawPreview(LC_MouseEvent *event, int status) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;

    /**
     * Target line to which line will be created
     */
    RS_Line *m_targetLine = nullptr;
    /**
     * start point from which line will be created
     */
    RS_Vector m_startPoint = RS_Vector(false);
    /**
     * desired angle between target line and line that will be built. angle 0..90 - line will be closer to left corner of target line,
     * -90..0 - will be closer to right corner of target line (considering that start point is located above the target line).
     * In Alternative action mode (SHIFT pressed with mouse), alternative (mirrored) angle will be used instead of provided one
     */
    double m_angle = 0.0;
    /**
     * controls the mode for line length calculation
     */
    int m_sizeMode = SIZE_INTERSECTION;
    /**
     * mode for snapping fixed size line relative to start point
     */
    int m_lineSnapMode = SNAP_MIDDLE;
    /**
     * flag determines whether angle is fixed to 90 degrees
     */
    bool m_orthogonalMode = false;
    /**
     * Length of the line if it is fixed
     */
    double m_length = 0.0;

    /**
     * For drawing line from point to intersection point, specifies offset from intersection point
     */
    double m_endOffset = 0.0;

    RS_Line *createLineFromPointToTarget(RS_Line *line, RS_Vector& intersection);
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector zero) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void doAfterTrigger() override;
    void doBack(LC_MouseEvent *pEvent, int status) override;
    void doFinish(bool updateTB) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};
#endif // LC_ACTIONDRAWLINEFROMPOINTTOLINE_H
