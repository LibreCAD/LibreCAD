#ifndef LC_ACTIONMODIFYLINEJOIN_H
#define LC_ACTIONMODIFYLINEJOIN_H

#include "rs_previewactioninterface.h"
#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyLineJoin :public LC_AbstractActionWithPreview {
    Q_OBJECT



public:

    enum{
        SetLine1,
        SetLine2,
        ResolveFirstLineTrim
    };

    enum{
        EDGE_EXTEND_TRIM,
        EDGE_ADD_SEGMENT,
        EDGE_NO_MODIFICATION
    };

    LC_ActionModifyLineJoin(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionModifyLineJoin() override;

    void trigger() override;


    void commandEvent(RS_CommandEvent* e) override;
    void coordinateEvent(RS_CoordinateEvent* e) override;
    void updateMouseButtonHints() override;

    void init(int status) override;

    QStringList getAvailableCommands() override;


    bool isCreatePolyline(){return createPolyline;};
    void setCreatePolyline(bool value);

    bool isRemoveOriginalLines(){return removeOriginalLines;};
    void setRemoveOriginalLines(bool value);

    int getLine1EdgeMode(){return line1EdgeMode;};
    void setLine1EdgeMode(int value);

    int getLine2EdgeMode(){return line2EdgeMode;};
    void setLine2EdgeMode(int index);

    void setAttributesSource(int value);
    int getAttributesSource(){return attributesSource;};

protected:
    void createOptionsWidget() override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    RS2::CursorType doGetMouseCursor(int status) override;

    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;

private:
    struct LC_PointsDisposition{
        enum
        {
            BOTH_POINTS_ON_RIGHT,
            BOTH_POINTS_ON_LEFT,
            MIDDLE_START_LEFT,
            MIDDLE_END_LEFT
        };

        int dispositionMode;

        bool isStartPointClosest;
        RS_Vector closestPoint;
        RS_Vector farPoint;
        RS_Vector startPoint;
        RS_Vector endPoint;
        RS_Vector majorPoint;

        bool snapSelectionOnLeft;

        bool isIntersectionPointBelongsLine(){
            return dispositionMode == MIDDLE_START_LEFT || dispositionMode == MIDDLE_END_LEFT;
        }
    };

    struct LC_LineJoinData{
        bool parallelLines;
        bool straightLinesConnection;
        RS_Polyline* polyline;
        RS_Vector intersectPoint;
        RS_Vector majorPointLine1;
        RS_Vector majorPointLine2;
        LC_PointsDisposition line1Disposition;
        LC_PointsDisposition line2Disposition;

        bool areLinesAlreadyIntersected(){
            return line1Disposition.isIntersectionPointBelongsLine() && line2Disposition.isIntersectionPointBelongsLine();
        }

        bool isIntersectionOnLine1(){
            return line1Disposition.isIntersectionPointBelongsLine();
        }
    };

    enum {
        ATTRIBUTES_ACTIVE_PEN_LAYER,
        ATTRIBUTES_LINE_1,
        ATTRIBUTES_LINE_2,
        ATTRIBUTES_BOTH_LINES
    };


    bool createPolyline;
    bool removeOriginalLines;
    int attributesSource;

    const EntityTypeList lineType = EntityTypeList{RS2::EntityLine};

    int line1EdgeMode;
    int line2EdgeMode;

    RS_Line* line1;
    RS_Line* line2;
    RS_Line* highlightedLine;
    LC_LineJoinData* linesJoinData;

    LC_LineJoinData* createLineJoinData(RS_Line* secondLine, RS_Vector &snapPoint);
    LC_PointsDisposition determine3PointsDisposition(const RS_Vector start, const RS_Vector end, const RS_Vector intersection, const RS_Vector &snapPoint) const;
    RS_Vector getMajorPointFromLine(const int edgeMode, const RS_Vector &lineStart, const RS_Vector &lineEnd, const LC_ActionModifyLineJoin::LC_PointsDisposition &lineDisposition) const;
    void updateLine1TrimData(RS_Vector snap);
    RS_Vector getTrimStartPoint(LC_PointsDisposition disposition, RS_Vector& intersection);
    RS_Vector getTrimEndPoint(LC_PointsDisposition disposition, RS_Vector& intersection);
    LC_LineJoinData* proceedParallelLinesJoin(const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End) const;
    LC_LineJoinData* proceedNonParallelLines(
        RS_Vector &snapPoint,  const RS_VectorSolutions &sol, const RS_Vector &line1Start, const RS_Vector &line1End,
        const RS_Vector &line2Start, const RS_Vector &line2End);

    void applyAttributes(RS_Entity *entity, bool forLine1);
    void deleteOriginalEntity(RS_Entity *entity);
    void doTrigger();

};

#endif // LC_ACTIONMODIFYLINEJOIN_H
