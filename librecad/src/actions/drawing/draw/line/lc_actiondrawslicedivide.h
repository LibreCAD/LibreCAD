/****************************************************************************
**
* Action that draws specified amount of ticks for line or circle or arc
* with specified distance, angle and size and may divide original entity by
* ticks if necessary

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
#ifndef LC_ACTIONDRAWSLICEDIVIDE_H
#define LC_ACTIONDRAWSLICEDIVIDE_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionDrawSliceDivide:public LC_AbstractActionWithPreview {
    Q_OBJECT
public:
    /**
     * controls how to snap tick
     */
    enum {
        SNAP_START,  // tick is snapped to it start
        SNAP_MIDDLE, // tick is snapped to it's end
        SNAP_END // tick is snapped to its middle point
    };

    /**
     * controls whether how ticks on the edges of line or arc should be drawn
     */
    enum {
        DRAW_EDGE_NONE, // no ticks in edges
        DRAW_EDGE_BOTH, // both start and end points
        DRAW_EDGE_START, // only for start point
        DRAW_EDGE_END // only for end point
    };

    /**
     * action state
     */
    enum {
        SetEntity
    };

    /**
     * Currently selected entity (used for update options)
     */
    enum {
        SELECTION_NONE,
        SELECTION_ARC,
        SELECTION_CIRCLE
    };


    LC_ActionDrawSliceDivide(LC_ActionContext *actionContext, bool forCircle);
    bool isTickAngleRelative() const{return m_tickAngleIsRelative;}
    bool isDivideEntity() const{return m_doDivideEntity;}
    void setTickLength(double len){m_tickLength = len;}
    void setDrawTickOnEdgeMode(int i){m_tickEdgeDrawMode = i;}
    void setTickAngle(double a){ m_tickAngleDegrees = a;}
    void setCircleStartTickAngle(double a){ m_circleStartTickAngleDegrees = a;}
    void setTickAngleRelative(bool b){m_tickAngleIsRelative = b;}
    void setDivideEntity(bool value){m_doDivideEntity = value;}
    void setTickCount(int c){m_tickCount = c;}
    void setTickSnapMode(int m){m_tickSnapMode = m;}
    void setTickOffset(double offset){m_tickOffset = offset;}
    int getTickSnapMode() const{return m_tickSnapMode;}
    int getTickCount() const{return m_tickCount;}
    int getDrawTickOnEdgeMode() const{return m_tickEdgeDrawMode;}
    double getTickAngle() const{return m_tickAngleDegrees;}
    double getTickLength() const{return m_tickLength;}
    double getTickOffset() const{return m_tickOffset;}
    double getCircleStartAngle() const{return m_circleStartTickAngleDegrees;}
    void setDistance(double val){m_distance = val;};
    double getDistance() const{return m_distance;};
    void setFixedDistance(bool value){m_fixedDistance = value;};
    bool isFixedDistance() const{return m_fixedDistance;};
private:
    /**
     * amount of ticks to created
     */
    int m_tickCount = 2;
    /*
     * lengths of tick
     */
    double m_tickLength = 20;
    /**
     * offset of tick from the line
     */
    double m_tickOffset =0;
    /**
     * angle used to draw ticks. For alternative action mode (if shift is pressed on click), we'll use alternative (mirrored) angle
     * of tick instead of one set in options.
     */
    double m_tickAngleDegrees = 90;
    /**
     * start angle used for drawing ticks for circle
     */
    double m_circleStartTickAngleDegrees = 0;

    /**
     * Flag that indicates that the distance between ticks is fixed and is not calculated based on provided number of ticks
     */
    bool m_fixedDistance = false;

    /**
     * fixed distance between ticks
     */
    double m_distance = 0;

    /**
     * Defines in which point (start, middle, end) tick should be snapped to intersection with entity
     */
    int m_tickSnapMode= SNAP_MIDDLE;

    /**
     * Controls how ticks on entity's edge points should be drawn
     */
    int m_tickEdgeDrawMode = DRAW_EDGE_BOTH;

    /**
     * defines whether angle of tick is absolute or related to entity
     */
    bool m_tickAngleIsRelative = true;

    /**
     * Data that describes ticks
     */
    struct TickData;

    std::vector<TickData*> m_ticksData;
    /**
     * original entity for which ticks will be created
     */
    RS_Entity *m_entity = nullptr;

    /**
     * Flag that indicates that original entity should be divided in ticks intersection points
     */
    bool m_doDivideEntity = false;

    void prepareLineTicks(RS_Line *e);
    void prepareArcTicks(RS_Arc *arc);
    void prepareCircleTicks(RS_Circle *circle);
    void prepareTickData(RS_Vector &tickSnapPosition, RS_Entity *entity, RS_LineData &tickLineData);
    void addTick(RS_Vector &tickSnapPoint, RS_LineData &lineData, bool edge, bool visible, double angle);
    void createTickData(RS_Entity *e, RS_Vector tickSnapPoint, double arcAngle, bool edge, bool visible = true);
    void prepareStartTick(RS_Entity *entity, const RS_Vector &tickSnapPoint, double arcAngle);
    void prepareEndTick(RS_Entity *entity, const RS_Vector &tickSnapPoint, double arcAngle);
    void prepareArcSegments(RS_Entity *e, double radius, RS_Vector &center, double startPointAngle, double arcLength);
    void createLineSegments(RS_Line *pLine, QList<RS_Entity *> &list);
    void createArcSegments(RS_Arc *pArc, QList<RS_Entity *> &list);
    void createCircleSegments(RS_Circle *pCircle, QList<RS_Entity *> &list);
    void doCreateArcSegments(RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed, QList<RS_Entity *> &list);
    bool checkShouldDivideEntity(const RS_Entity *entity, const QString &entityName) const;
protected:
    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void doAfterTrigger() override;
    void doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayDrawPreview(LC_MouseEvent *event, int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void clearTickData();
    RS2::CursorType doGetMouseCursor(int status) override;
    EntityTypeList getCatchEntityTypeList() const;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONDRAWSLICEDIVIDE_H
