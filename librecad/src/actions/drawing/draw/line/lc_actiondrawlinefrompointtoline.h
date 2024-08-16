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
#include "qg_actionhandler.h"

/**
 * Actions draws line from given point to selected target line. Angle between target line and created line is either user defined
 * in range (-90..90 degrees) or fixed to 90 degrees (so line orthogonal to target line is created).
 *
 * The lengths of created line may be either fixed, or be from start point to the point of intersection of created line and target line.
 */
class LC_ActionDrawLineFromPointToLine:public LC_AbstractActionWithPreview
{
    Q_OBJECT
public:
    LC_ActionDrawLineFromPointToLine(QG_ActionHandler* a_handler, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionDrawLineFromPointToLine() override = default;
    void setLineSnapMode(int val) {lineSnapMode = val;};
    int getLineSnapMode() const{return lineSnapMode;};
    void setOrthogonal(bool value){orthogonalMode = value;};
    bool getOrthogonal() const{return orthogonalMode;};
    void setAngle(double ang){angle = ang;};
    double getAngle() const{return angle;};
    int getSizeMode() const{return sizeMode;};
    void setSizeMode(int mode){sizeMode = mode;};
    void setLength(double len){length = len;};
    double getLength() const{return length;}
    void setEndOffset(double len){endOffset = len;};
    double getEndOffset() const{return endOffset;}
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
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;

    /**
     * Target line to which line will be created
     */
    RS_Line *targetLine = nullptr;
    /**
     * start point from which line will be created
     */
    RS_Vector startPoint = RS_Vector(false);
    /**
     * desired angle between target line and line that will be built. angle 0..90 - line will be closer to left corner of target line,
     * -90..0 - will be closer to right corner of target line (considering that start point is located above the target line).
     * In Alternative action mode (SHIFT pressed with mouse), alternative (mirrored) angle will be used instead of provided one
     */
    double angle = 0.0;
    /**
     * controls the mode for line length calculation
     */
    int sizeMode = SIZE_INTERSECTION;
    /**
     * mode for snapping fixed size line relative to start point
     */
    int lineSnapMode = SNAP_MIDDLE;
    /**
     * flag determines whether angle is fixed to 90 degrees
     */
    bool orthogonalMode = false;
    /**
     * Length of the line if it is fixed
     */
    double length = 0.0;

    /**
     * For drawing line from point to intersection point, specifies offset from intersection point
     */
    double endOffset = 0.0;

    RS_Line *createLineFromPointToTarget(RS_Line *line, RS_Vector& intersection);
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector zero) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void doAfterTrigger() override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doFinish(bool updateTB) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};
#endif // LC_ACTIONDRAWLINEFROMPOINTTOLINE_H
