/****************************************************************************
**
* Action that draws a line from selected line by specified angle (absolute or
* relating with given length.

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
#ifndef LC_ACTIONDRAWLINEANGLEREL_H
#define LC_ACTIONDRAWLINEANGLEREL_H

#include "lc_abstractactionwithpreview.h"
#include "rs_line.h"

class LC_ActionDrawLineAngleRel :public LC_AbstractActionWithPreview {
    Q_OBJECT
public:
    /**
     * Snapping for angle tick line
     */
    enum{
        TICK_SNAP_START, // tick angle line start is on original line
        TICK_SNAP_MIDDLE, // tick middle is on original line
        TICK_SNAP_END // tick end point is on original line
    };

    /**
     * Action states
     */
    enum {
        SetLine = InitialActionStatus,
        SetTickLength
    };

    LC_ActionDrawLineAngleRel(LC_ActionContext *actionContext, double angle = 0.0, bool fixedAngle = false);
    ~LC_ActionDrawLineAngleRel() override;
    void setLineSnapMode(int mode) {  m_lineSnapMode = mode;}
    int getLineSnapMode() const{return m_lineSnapMode;}
    void setTickSnapMode(int mode) {m_tickSnapMode = mode;}
    int getTickSnapMode() const {return m_tickSnapMode;}
    void setTickAngleDegrees(double a){ m_tickAngleDegrees = a;}
    double getTickAngleDegrees() const {return m_tickAngleDegrees;}
    void setTickLength(double len){m_tickLength = len;}
    double getTickLength() const {return m_tickLength;}
    void setTickOffset(double o){m_tickOffset = o;}
    double getTickOffset() const {return m_tickOffset;}
    void setSnapDistance(double d){m_snapDistance = d;}
    double getSnapDistance() const {return m_snapDistance;}
    bool isAngleRelative() const{return m_relativeAngle;}
    void setAngleIsRelative(bool value){m_relativeAngle = value;}
    bool isLengthFree() const{return m_lengthIsFree;}
    void setLengthIsFree(bool value){m_lengthIsFree = value;}
    bool isDivideLine() const{return m_divideLine;}
    void setDivideLine(bool value){m_divideLine = value;}
private:
    /**
     * Controls in which point of original line snap is performed
     */
    int m_lineSnapMode = LINE_SNAP_START;

    /**
     * controls how to snap tick line
     */
    int m_tickSnapMode = TICK_SNAP_END;
    /*
     * angle used for drawing line (absolute or relative).
     * Alternative action mode uses alternative angle (mirrored original) instead of original one
     */
    double m_tickAngleDegrees = 0;
    /**
     * Length of tick if it is fixed
     */
    double m_tickLength = 50;
    /**
     * offset of tick line
     */
    double m_tickOffset = 0;
    /**
     * indicates whether original line should be divided in point of intersection
     */
    bool m_divideLine = false;

    /**
     * Distance (offset) from original line snap-point to which intersection line will be shifted
     */
    double m_snapDistance = 0;

    /**
     * indicates that angle specified by options is relative (i.e between original line and drawn line).
     * If flag is false, absolute angle (from line tick and x axis) will be used.
     */
    bool m_relativeAngle = false;

    /**
     * Flag indicates that the user will specify length of tick line in interactive mode by mouse
     * instead of relying on fixed value of length
     */
    bool m_lengthIsFree = false;

    struct TickData{
        RS_Line* line {nullptr};
        RS_LineData tickLineData;
        /** Chosen position */
        RS_Vector tickSnapPosition;
        bool deleteOriginalLine {false};
    };

    /**
     * describes data needed for drawing tick angle
     */
    TickData* m_tickData = nullptr;

    RS_Vector obtainLineSnapPointForMode(RS_Line* targetLine, RS_Vector& snap) const;
    TickData* prepareLineData(RS_Line* targetLine, const RS_Vector& tickSnapPosition, const RS_Vector& tickEndPosition, bool alternateAngle);
protected:
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doCheckMayDrawPreview(LC_MouseEvent *event, int status) override;
    bool doCheckMayTrigger() override;
    void performTriggerDeletions() override;
    void divideOriginalLine(TickData *pData, QList<RS_Entity *> &list);
    bool isSetActivePenAndLayerOnTrigger() override;
    void doFinish(bool updateTB) override;
    void setLine(RS_Entity* en, const RS_Vector& snapPoint);
    void updateMouseButtonHints() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};

#endif // LC_ACTIONDRAWLINEANGLEREL_H
