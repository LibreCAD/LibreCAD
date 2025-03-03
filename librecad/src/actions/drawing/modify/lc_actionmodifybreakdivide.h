/****************************************************************************
**
* Action that breaks line, arc or circle to segments by points of intersection
* with other entities.

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
#ifndef LC_ACTIONMODIFYBREAKOUTLINE_H
#define LC_ACTIONMODIFYBREAKOUTLINE_H

#include "rs_arc.h"
#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyBreakDivide:public LC_AbstractActionWithPreview
{
    Q_OBJECT

   /**
   * action state
   */
   enum{
        SetLine
    };

   /**
    * configuration of segment of entity on which snap selection occurred
    */
    enum{
        SEGMENT_INSIDE, // segment is between two intersection points
        SEGMENT_TO_START, // snap is between start point of entity and intersection point
        SEGMENT_TO_END // snap is between end point of entity and intersection point
    };

    /**
     * Snap segment info for line
     */
    struct LineSegmentData{
        int segmentDisposition;
        RS_Vector snap;
        RS_Vector snapSegmentStart;
        RS_Vector snapSegmentEnd;
    };

    /**
     * Snap segment for angle
     */
    struct ArcSegmentData{
        int segmentDisposition;
        double snapSegmentStartAngle;
        double snapSegmentEndAngle;
    };

    /**
     * Snap segment for circle     *
     */
    struct CircleSegmentData{
        double snapSegmentStartAngle;
        double snapSegmentEndAngle;
    };

    /**
     * Structure used to pass data to trigger method
     */
    struct TriggerData{
        RS_Entity* entity;
        RS_Vector snapPoint;
    };

public:
    LC_ActionModifyBreakDivide(RS_EntityContainer &container, RS_GraphicView &graphicView);

    bool isRemoveSegment() const{return removeSegments;}
    void setRemoveSegment(bool value){removeSegments = value;};
    bool isRemoveSelected() const{return removeSelected;};
    void setRemoveSelected(bool value){removeSelected = value;}
protected:
    /**
     * Flag that defines whether we should remove segments of entity or just divide entity
     */
    bool removeSegments = false;

    /**
     * For segments removal, specifies whether it is necessary to remove selected segment or remaining ones
     */
    bool removeSelected = false;

    TriggerData* triggerData = nullptr;

    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    LineSegmentData *calculateLineSegment(RS_Line *line, RS_Vector &snap);
    QVector<RS_Vector>  collectAllIntersectionsWithEntity(RS_Entity *entity);
    void addPointsFromSolutionToList(RS_VectorSolutions &sol, QVector<RS_Vector> &result) const;
    LineSegmentData *findLineSegmentEdges(RS_Line *line, RS_Vector &snap, QVector<RS_Vector> intersections);
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void createEntitiesForLine(RS_Line *line, RS_Vector &snap, QList<RS_Entity *> &list, bool preview);
    void createEntitiesForCircle(RS_Circle *circle, RS_Vector &vector, QList<RS_Entity *> &list, bool preview);
    void createEntitiesForArc(RS_Arc *arc, RS_Vector &snap, QList<RS_Entity *> &list, bool preview);
    ArcSegmentData *calculateArcSegments(RS_Arc *arc, RS_Vector &snap);
    ArcSegmentData *findArcSegmentEdges(RS_Arc *arc, RS_Vector &snap, const QVector<RS_Vector>& intersections);
    CircleSegmentData *calculateCircleSegment(RS_Circle *circle, RS_Vector &snap);
    CircleSegmentData *findCircleSegmentEdges(RS_Circle *circle, RS_Vector &snap, const QVector<RS_Vector> &intersections);
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayTrigger() override;
    void performTriggerDeletions() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void createArcEntity(const RS_ArcData &arcData, bool preview, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const;
    void createLineEntity(bool preview, const RS_Vector &start, const RS_Vector &end, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const;
    void doAfterTrigger() override;
    void doFinish(bool updateTB) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONMODIFYBREAKOUTLINE_H
