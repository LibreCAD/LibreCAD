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
     * Currently selected entity (used for update options)
     */
    enum {
        SELECTION_NONE,
        SELECTION_ARC,
        SELECTION_CIRCLE
    };

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

    LC_ActionDrawSliceDivide(LC_ActionContext *actionContext, bool forCircle);
    bool isTickAngleRelative() const{return m_tickAngleIsRelative;}
    bool isDivideEntity() const{return m_doDivideEntity;}
    void setTickLength(const double len)  {m_tickLength = len;}
    void setDrawTickOnEdgeMode(const int i)  {m_tickEdgeDrawMode = i;}
    void setTickAngleDegrees(const double a)  { m_tickAngleDegrees = a;}
    void setCircleStartTickAngleDegrees(const double a)  { m_circleStartTickAngleDegrees = a;}
    void setTickAngleRelative(const bool b)  {m_tickAngleIsRelative = b;}
    void setDivideEntity(const bool value)  {m_doDivideEntity = value;}
    void setTickCount(const int c)  {m_tickCount = c;}
    void setTickSnapMode(const int m)  {m_tickSnapMode = m;}
    void setTickOffset(const double offset)  {m_tickOffset = offset;}
    int getTickSnapMode() const{return m_tickSnapMode;}
    int getTickCount() const{return m_tickCount;}
    int getDrawTickOnEdgeMode() const{return m_tickEdgeDrawMode;}
    double getTickAngleDegrees() const{return m_tickAngleDegrees;}
    double getTickLength() const{return m_tickLength;}
    double getTickOffset() const{return m_tickOffset;}
    double getCircleStartAngleDegrees() const{return m_circleStartTickAngleDegrees;}
    void setDistance(const double val)  {m_distance = val;}
    double getDistance() const{return m_distance;}
    void setFixedDistance(const bool value)  {m_fixedDistance = value;}
    bool isFixedDistance() const{return m_fixedDistance;}
protected:
    /**
     * action state
     */
    enum {
        SetEntity = InitialActionStatus
    };

    void doSaveOptions() override;
    void doLoadOptions() override;

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
     * Flag that indicates that original entity should be divided in ticks intersection points
     */
    bool m_doDivideEntity = false;

    /**
     * Data that describes ticks
     */
    struct TickData;

    std::vector<TickData*> m_ticksData;
    /**
     * original entity for which ticks will be created
     */
    RS_Entity *m_entity = nullptr;

    void prepareLineTicks(const RS_Line *line);
    void prepareArcTicks(const RS_Arc* arc);
    void prepareCircleTicks(const RS_Circle *circle);
    void prepareTickData(const RS_Vector &tickSnapPosition, const RS_Entity *entity, RS_LineData &tickLineData) const;
    void addTick(const RS_Vector &tickSnapPoint, const RS_LineData &lineData, bool edge, bool visible, double angle);
    void createTickData(const RS_Entity *e, const RS_Vector& tickSnapPoint, double arcAngle, bool edge, bool visible = true);
    void prepareStartTick(const RS_Entity *entity, const RS_Vector &tickSnapPoint, double arcAngle);
    void prepareEndTick(const RS_Entity *entity, const RS_Vector &tickSnapPoint, double arcAngle);
    void prepareArcSegments(const RS_Entity *e, double radius, const RS_Vector &center, double startPointAngle, double arcLength);
    void createLineSegments(const RS_Line *line, QList<RS_Entity *> &list) const;
    void createArcSegments(const RS_Arc *pArc, QList<RS_Entity *> &list) const;
    void createCircleSegments(const RS_Circle *pCircle, QList<RS_Entity *> &list) const;
    void doCreateArcSegments(const RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed, QList<RS_Entity *> &list) const;
    bool checkShouldDivideEntity(const RS_Entity *e, const QString &entityName) const;
protected:
    bool doCheckMayTrigger() override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx)  override;
    void doAfterTrigger() override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(const LC_MouseEvent* e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayDrawPreview(const LC_MouseEvent* event, int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void clearTickData();
    RS2::CursorType doGetMouseCursor(int status) override;
    EntityTypeList getCatchEntityTypeList() const;
    void updateActionPrompt() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
};

#endif
