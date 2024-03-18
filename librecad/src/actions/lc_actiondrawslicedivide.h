#ifndef LC_ACTIONDRAWSLICEDIVIDE_H
#define LC_ACTIONDRAWSLICEDIVIDE_H

#include "rs_line.h"
#include "rs_previewactioninterface.h"
#include "lc_abstractactionwithpreview.h"

//fixme - draw or modify in name?
// Fixme - review initial highligth mode?
class LC_ActionDrawSliceDivide:public LC_AbstractActionWithPreview{
    Q_OBJECT

public:
    enum{
        SNAP_START,
        SNAP_MIDDLE,
        SNAP_END
    };

    enum {
        DRAW_EDGE_NONE,
        DRAW_EDGE_BOTH,
        DRAW_EDGE_START,
        DRAW_EDGE_END
    };

    enum{
        SetEntity,
        SetCount,
        SetTickLength,
        SetTickAngle,
        SetTickSnap,
        SetTickEdgeMode,
        SetCircleStartAngle
    };

    struct TickData{
        explicit TickData(bool e,
        bool v,
        const RS_Vector& p,
        const RS_LineData & l,
        double ang) :
        edge(e),
        isVisible(v),
        snapPoint( p),
        tickLine(l),
        arcAngle(ang){}


        bool isVisible {true};
        bool edge {false};
        RS_Vector snapPoint;
        RS_LineData tickLine;
        double arcAngle;
    };


    LC_ActionDrawSliceDivide(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);

    void updateMouseButtonHints() override;
    void commandEvent(RS_CommandEvent *e) override;
    void coordinateEvent(RS_CoordinateEvent* e) override;

    void init(int status) override;

    void setTickOffset(double offset);
    void setTickLength(double len);
    void setDrawTickOnEdgeMode(int i);
    void setTickAngle(double a);
    void setCircleStartTickAngle(double a);
    void setTickCount(int c);
    void setTickSnapMode(int m);
    bool isCircleEntity();
    int getTickSnapMode();

    int getTickCount();
    int getDrawTickOnEdgeMode();
    double getTickAngle();
    double getTickLength();
    double getTickOffset();
    bool isTickAngleRelative();
    void setTickAngleRelative(bool b);
    bool isDivideEntity();
    void setDivideEntity(bool value);

    double getCircleStartAngle();
    void finish(bool updateTB) override;
private:
    double tickCount {2};
    double tickLength {20};
    double tickOffset {0};
    double tickAngle {90};
    double circleStartTickAngle {0};
    int tickSnapMode {SNAP_MIDDLE};
    int tickEdgeDrawMode {DRAW_EDGE_BOTH};
    bool tickAngleIsRelative {true};


    std::vector<TickData> ticksData;

    RS_Entity* entity;
    bool doDivideEntity {false};

    void prepareLineTicks(RS_Line *e);
    void prepareArcTicks(RS_Arc *arc);
    void prepareCircleTicks(RS_Circle *circle);
    void prepareTickData(RS_Vector &tickSnapPosition, RS_Entity *entity, RS_LineData &tickLineData);
    void addTick(RS_Vector &tickSnapPoint, RS_LineData &lineData, bool edge, bool visible, double angle);
    void createTickData(RS_Entity *e, RS_Vector tickSnapPoint, double arcAngle, bool edge, bool visible = true);
    void prepareStartTick(RS_Entity *entity, const RS_Vector& startPoint, double arcAngle);
    void prepareEndTick(RS_Entity *entity, const RS_Vector& endPoint, double arcAngle);
    void prepareArcSegments(RS_Entity *e, double radius, RS_Vector &center, double startPointAngle, double arcLength);
    void updatePreview();
    void doDrawTicks();

    void createLineSegments(RS_Line *pLine, QList<RS_Entity *> &list);
    void createArcSegments(RS_Arc *pArc, QList<RS_Entity *> &list);
    void createCircleSegments(RS_Circle *pCircle, QList<RS_Entity *> &list);
    void doCreateArcSegments(RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed, QList<RS_Entity *> &list);

    bool checkShouldDivideEntity(const RS_Entity *entity, const QString &entityName) const;
protected:
    void createOptionsWidget() override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void doAfterTrigger() override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
};

#endif // LC_ACTIONDRAWSLICEDIVIDE_H
