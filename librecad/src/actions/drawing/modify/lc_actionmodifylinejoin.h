/****************************************************************************
**
* Action that joins two selected line in their intersection point.

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
#ifndef LC_ACTIONMODIFYLINEJOIN_H
#define LC_ACTIONMODIFYLINEJOIN_H

#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

/**
 * Action that joins two selected line in their intersection point.
 * May also perform trim or modify lengths, as specified by options
 */
class LC_ActionModifyLineJoin :public LC_AbstractActionWithPreview {
    Q_OBJECT

public:
    // states for the action
    enum{
        SetLine1, // selecting line 1
        SetLine2, // selecting line 2
        ResolveFirstLineTrim // resolving trim for line 1 after intersection
    };

    /**
     * Modes that defines how to process specific line
     */
    enum{
        EDGE_EXTEND_TRIM, // line should be extended/trimmed in the intersection point
        EDGE_ADD_SEGMENT, // additional segment (separate line) should be created from endpoint of line to intersection point
        EDGE_NO_MODIFICATION // do not modify the line
    };

    LC_ActionModifyLineJoin(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionModifyLineJoin() override;
    void init(int status) override;

    bool isCreatePolyline() const{return createPolyline;};
    void setCreatePolyline(bool value);

    bool isRemoveOriginalLines() const{return removeOriginalLines;};
    void setRemoveOriginalLines(bool value);

    int getLine1EdgeMode() const{return line1EdgeMode;};
    void setLine1EdgeMode(int value);

    int getLine2EdgeMode() const{return line2EdgeMode;};
    void setLine2EdgeMode(int index);

    void setAttributesSource(int value);
    int getAttributesSource() const{return attributesSource;};
protected:
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    RS2::CursorType doGetMouseCursor(int status) override;

    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doAfterTrigger() override;
    void performTriggerDeletions() override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void updateMouseButtonHints() override;
private:
    /**
     * Utility structure that describes how the intersection point and snap point are located relative to the line
     */
    struct LC_PointsDisposition{
        enum
        {
            BOTH_POINTS_ON_RIGHT, // two endpoints of line are at right side of intersection point
            BOTH_POINTS_ON_LEFT, // to endpoints of line are at left side of intersection
            MIDDLE_START_LEFT, // intersection is between endpoints, start endpoint is on the left
            MIDDLE_END_LEFT // intersection is between endpoints, end endpoint is on the left
        };

        int dispositionMode;
        RS_Vector closestPoint;
        RS_Vector farPoint;
        RS_Vector startPoint;
        RS_Vector endPoint;
        bool snapSelectionOnLeft;

        bool isIntersectionPointBelongsLine(){
            return dispositionMode == MIDDLE_START_LEFT || dispositionMode == MIDDLE_END_LEFT;
        }
    };

    /**
     * Overall data that describes how lines should be intersected. Used for preview and trigger
     */
    struct LC_LineJoinData{
        bool parallelLines; // are lines parallel?
        bool straightLinesConnection; // lines are on the same vector
        RS_Polyline* polyline {nullptr};
        RS_Vector intersectPoint;  // point of intersection
        RS_Vector majorPointLine1; // point used for drawing result for line 1
        RS_Vector majorPointLine2; // point used for drawing result for line 1
        LC_PointsDisposition line1Disposition; // intersection point disposition for line 1
        LC_PointsDisposition line2Disposition; // intersection point disposition for line 2

        bool areLinesAlreadyIntersected(){
            return line1Disposition.isIntersectionPointBelongsLine() && line2Disposition.isIntersectionPointBelongsLine();
        }

        bool isIntersectionOnLine1(){
            return line1Disposition.isIntersectionPointBelongsLine();
        }
    };

    /**
     * Enums that controls which pen and layer should be applied to created entities
     */
    enum {
        ATTRIBUTES_ACTIVE_PEN_LAYER, // rely on active pen and layer
        ATTRIBUTES_LINE_1, // pick them from line 1
        ATTRIBUTES_LINE_2, // pick them from line 2
        ATTRIBUTES_BOTH_LINES // if segments are added, for each segment use pen and layer from adjacent line
    };

    /*
     * should we create polyline or just individual lines
     */
    bool createPolyline = false;

    /**
     * specifies whether original lines be removed from drawing
     */
    bool removeOriginalLines = false;

    /**
     * where from apply attributes
     */
    int attributesSource = ATTRIBUTES_ACTIVE_PEN_LAYER;

    /**
     * entity types that may be caught
     */
    const EntityTypeList lineType = EntityTypeList{RS2::EntityLine};

    /**
     * specifies how to handle edges for line 1
     */
    int line1EdgeMode = EDGE_EXTEND_TRIM;

    /**
     * specifies how to handle edges for line 2
     */
    int line2EdgeMode = EDGE_EXTEND_TRIM;

    /**
     * selected line 1
     */
    RS_Line* line1 = nullptr;

    /**
     * selected line 2
     */
    RS_Line* line2 = nullptr;

    /**
     * data that describes join
     */
    LC_LineJoinData* linesJoinData = nullptr;

    RS_Vector line1ClickPosition = RS_Vector(false);

    LC_LineJoinData* createLineJoinData(RS_Line* secondLine, RS_Vector &snapPoint);
    LC_PointsDisposition determine3PointsDisposition(RS_Vector start, RS_Vector end, const RS_Vector intersection, const RS_Vector &snapPoint) const;
    RS_Vector getMajorPointFromLine(int edgeMode, const RS_Vector &lineStart, const RS_Vector &lineEnd, const LC_ActionModifyLineJoin::LC_PointsDisposition &lineDisposition) const;
    void updateLine1TrimData(RS_Vector snap);
    LC_LineJoinData* proceedParallelLinesJoin(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End) const;
    LC_LineJoinData* proceedNonParallelLines(
        RS_Vector &line1ClickPoint, RS_Vector &snapPoint,  const RS_Vector &intersection, const RS_Vector &line1Start, const RS_Vector &line1End,
        const RS_Vector &line2Start, const RS_Vector &line2End);

    void applyAttributes(RS_Entity *entity, bool forLine1);
    RS_Line *catchLine(QMouseEvent *e);
};

#endif // LC_ACTIONMODIFYLINEJOIN_H
