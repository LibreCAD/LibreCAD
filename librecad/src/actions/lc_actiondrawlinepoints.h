#ifndef LC_ACTIONDRAWLINEPOINTS_H
#define LC_ACTIONDRAWLINEPOINTS_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "lc_abstract_action_draw_line.h"

/**
 * Action that draws specified number of points on specified line. Points are distributed on equal distance
 */
class LC_ActionDrawLinePoints :public LC_AbstractActionDrawLine {
    Q_OBJECT

public:

    /**
     * Controls how to handle points on edges of the line
     */
    enum {
        DRAW_EDGE_NONE,
        DRAW_EDGE_BOTH,
        DRAW_EDGE_START,
        DRAW_EDGE_END
    };

    /**
     * additional statuses
     */
    enum{
        SetPointsCount = LAST,
        SetEdge = LAST + 1
    };


public:
    LC_ActionDrawLinePoints(RS_EntityContainer &container,RS_GraphicView &graphicView);
    ~LC_ActionDrawLinePoints() override;

    void updateMouseButtonHints() override;

    int getPointsCount() {return pointsCount;};
    void setPointsCount(int pointsCount);
    int getEdgePointsMode(){return edgePointsMode;};
    void setEdgePointsMode(int value);
    void init(int status) override;
    QStringList getAvailableCommands() override;

protected:
    void createOptionsWidget() override;
    bool doProceedCommand(RS_CommandEvent *pEvent, const QString &qString) override;
    bool doProcessCommandValue(RS_CommandEvent *e, const QString &c) override;
    const RS_Vector& getStartPointForAngleSnap() const override;
    void doBack(QMouseEvent *pEvent, int status) override;
    bool isStartPointValid() const override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void onCoordinateEvent(const RS_Vector &coord, bool isZero, int status) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool doCheckMayTrigger() override;
    void doAfterTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(RS_Vector vector) override;
private:
    /**
     * amount of points to create
     */
    int pointsCount;
    /**
     * how to handle points on edges
     */
    int edgePointsMode;

    /**
     * flag that indicates that point 1 is set
     */
    bool point1Set {false};

    /**
     * start point for line
     */
    RS_Vector startpoint;

    /**
     * end point of line
     */
    RS_Vector endpoint;

    void createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList);
    void setSetNumberOfPointsState(bool b);
    void updatePointsCount(int count);
    void setMajorStatus();
    void updateEdgePointsMode(int mode);
    bool isNonZeroLine(const RS_Vector &possiblePoint) const;
    RS_Point *createPointEntity(const RS_Vector &point) const;
};

#endif // LC_ACTIONDRAWLINEPOINTS_H
