#ifndef LC_ACTIONDRAWSLICEDIVIDE_H
#define LC_ACTIONDRAWSLICEDIVIDE_H

#include "rs_line.h"
#include "rs_previewactioninterface.h"

class LC_ActionDrawSliceDivide :public RS_PreviewActionInterface
{
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

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;
    void commandEvent(RS_CommandEvent *e) override;
    void coordinateEvent(RS_CoordinateEvent* e) override;

    void trigger() override;
    void init(int status) override;

    void showOptions() override;
    void hideOptions() override;


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
//    RS_Line* lineEntity {nullptr};
//    RS_Arc*  arcEntity {nullptr};
//    RS_Circle* circleEntity {nullptr};
    RS_Entity* entity;
    bool doDivideEntity {false};

    void updateOptions();
    void prepareLineTicks(RS_Line *e);
    void prepareArcTicks(RS_Arc *arc);
    void prepareCircleTicks(RS_Circle *circle);
    void prepareTickData(RS_Vector &tickSnapPosition, RS_Entity *entity, RS_LineData &tickLineData);
    void addTick(RS_Vector &tickSnapPoint, RS_LineData &lineData, bool edge, bool visible, double angle);
    void createTickData(RS_Entity *e, RS_Vector tickSnapPoint, double arcAngle, bool edge, bool visible = true);
    RS_Vector findPointOnCircle(double radius, double arcAngle, RS_Vector centerCircle);
    void prepareStartTick(RS_Entity *entity, const RS_Vector& startPoint, double arcAngle);
    void prepareEndTick(RS_Entity *entity, const RS_Vector& endPoint, double arcAngle);
    void prepareArcSegments(RS_Entity *e, double radius, RS_Vector &center, double startPointAngle, double arcLength);
    void updatePreview();
    void doDrawTicks();
    void drawLineTicks(RS_Line *pLine);
    void drawArcTicks(RS_Arc *pArc);
    void drawCircleTicks(RS_Circle *pCircle);
    void cutLineToSegments(RS_Line *pLine);
    void cutArcToSegments(RS_Arc *pArc);
    void cutCircleToSegments(RS_Circle *pCircle);
    void createArcSegments(RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed);
    void deleteOriginalEntity(RS_Entity *entity);
    bool checkShouldDivideEntity(const RS_Entity *entity, const QString &entityName) const;
};

#endif // LC_ACTIONDRAWSLICEDIVIDE_H
