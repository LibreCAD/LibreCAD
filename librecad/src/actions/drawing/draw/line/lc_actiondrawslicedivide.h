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

#include "rs_line.h"
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

    /**
     * Structure that describes single tick
     */
    struct TickData {
        explicit TickData(
            bool e,
            bool v,
            const RS_Vector &p,
            const RS_LineData &l,
            double ang):
            isVisible(v),
            edge(e),
            snapPoint(p),
            tickLine(l),
            arcAngle(ang){}

        bool isVisible{true}; // visible or not
        bool edge{false}; // is for edge?
        RS_Vector snapPoint; // point on entity where tick is snapped
        RS_LineData tickLine; // line data for tick
        double arcAngle; // angle for snapping tick
    };

    LC_ActionDrawSliceDivide(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        bool forCircle);

    bool isTickAngleRelative() const{return tickAngleIsRelative;}
    bool isDivideEntity() const{return doDivideEntity;}
    void setTickLength(double len){tickLength = len;}
    void setDrawTickOnEdgeMode(int i){tickEdgeDrawMode = i;}
    void setTickAngle(double a){tickAngle = a;}
    void setCircleStartTickAngle(double a){circleStartTickAngle = a;}
    void setTickAngleRelative(bool b){tickAngleIsRelative = b;}
    void setDivideEntity(bool value){doDivideEntity = value;}
    void setTickCount(int c){tickCount = c;}
    void setTickSnapMode(int m){tickSnapMode = m;}
    void setTickOffset(double offset){tickOffset = offset;}
    int getTickSnapMode() const{return tickSnapMode;}
    int getTickCount() const{return tickCount;}
    int getDrawTickOnEdgeMode() const{return tickEdgeDrawMode;}
    double getTickAngle() const{return tickAngle;}
    double getTickLength() const{return tickLength;}
    double getTickOffset() const{return tickOffset;}
    double getCircleStartAngle() const{return circleStartTickAngle;}
    void setDistance(double val){distance = val;};
    double getDistance() const{return distance;};
    void setFixedDistance(bool value){fixedDistance = value;};
    bool isFixedDistance() const{return fixedDistance;};
private:
    /**
     * amount of ticks to created
     */
    int tickCount = 2;
    /*
     * lengths of tick
     */
    double tickLength = 20;
    /**
     * offset of tick from the line
     */
    double tickOffset =0;
    /**
     * angle used to draw ticks. For alternative action mode (if shift is pressed on click), we'll use alternative (mirrored) angle
     * of tick instead of one set in options.
     */
    double tickAngle = 90;
    /**
     * start angle used for drawing ticks for circle
     */
    double circleStartTickAngle = 0;

    /**
     * Flag that indicates that the distance between ticks is fixed and is not calculated based on provided number of ticks
     */
    bool fixedDistance = false;

    /**
     * fixed distance between ticks
     */
    double distance = 0;

    /**
     * Defines in which point (start, middle, end) tick should be snapped to intersection with entity
     */
    int tickSnapMode= SNAP_MIDDLE;

    /**
     * Controls how ticks on entity's edge points should be drawn
     */
    int tickEdgeDrawMode = DRAW_EDGE_BOTH;

    /**
     * defines whether angle of tick is absolute or related to entity
     */
    bool tickAngleIsRelative = true;

    /**
     * Data that describes ticks
     */
    std::vector<TickData> ticksData;
    /**
     * original entity for which ticks will be created
     */
    RS_Entity *entity = nullptr;

    /**
     * Flag that indicates that original entity should be divided in ticks intersection points
     */
    bool doDivideEntity = false;

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
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    EntityTypeList getCatchEntityTypeList() const;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONDRAWSLICEDIVIDE_H
