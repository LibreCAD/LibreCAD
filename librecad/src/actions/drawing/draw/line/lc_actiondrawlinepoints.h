/****************************************************************************
**
* Action that creates a set of points located on line

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
#ifndef LC_ACTIONDRAWLINEPOINTS_H
#define LC_ACTIONDRAWLINEPOINTS_H

#include "lc_abstractactiondrawline.h"

/**
 * Action that draws specified number of points on specified line. Points are distributed on equal distance
 */
class LC_ActionDrawLinePoints :public LC_AbstractActionDrawLine {
    Q_OBJECT
public:
    LC_ActionDrawLinePoints(RS_EntityContainer &container,RS_GraphicView &graphicView);
    ~LC_ActionDrawLinePoints() override;
    int getPointsCount() const {return pointsCount;};
    void setPointsCount(int count) {pointsCount = count;};
    int getEdgePointsMode() const{return edgePointsMode;};
    void setEdgePointsMode(int value);
    void setFixedDistanceMode(bool value) {fixedDistanceMode = value;};
    bool isFixedDistanceMode() const{return fixedDistanceMode;};
    void setWithinLineMode(bool value) {withinLineMode = value;};
    bool isWithinLineMode() const{return withinLineMode;};
    double getPointsDistance() const{return fixedDistance;};
    void setPointsDistance(double val){fixedDistance = val;};
    void init(int status) override;
    QStringList getAvailableCommands() override;
protected:

    /**
     * Controls how to handle points on edges of the line
     */
    enum {
        DRAW_EDGE_NONE, // no points in line's  edges
        DRAW_EDGE_BOTH, // each edge of line has point
        DRAW_EDGE_START, // point in added in start point of line, no point in endpoint
        DRAW_EDGE_END // point is created in end point of line, no point in start point
    };

    /**
     * statuses of action (in addition of ones defined in base class)
     */
    enum{
        SetPointsCount = LAST,
        SetEdge = LAST + 1,
        SetFixDistance = LAST+2
    };

    /**
 * amount of points to create
 */
    int pointsCount = 0;
    /**
     * how to handle points on edges
     */
    int edgePointsMode = DRAW_EDGE_BOTH;

    /**
     * flag that indicates that point 1 is set
     */
    bool point1Set = false;

    /**
     * start point for line
     */
    RS_Vector startpoint = RS_Vector(false);

    /**
     * end point of line
     */
    RS_Vector endpoint = RS_Vector(false);

    /**
     * flag that defines whether distance between points is fixed
     */
    bool fixedDistanceMode = false;

    /**
     * flag that is applicable if we'll draw points with fixed distance between them.
     * It specifies whether points should be within a line (and so some points may be skipped if line is too short and
     * so less than pointsCount points will be created or (if false) if the line is rather considered as a direction vector
     * and the number of points specified by pointsCount will be always equal to pointsCount (plus edges points, of course)
     */
    bool withinLineMode = false;

    /**
     * Distance between points for fixed distance mode
     */
    double fixedDistance = false;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool doProceedCommand(int status, const QString &qString) override;
    bool doProcessCommandValue(int status, const QString &c) override;
    const RS_Vector& getStartPointForAngleSnap() const override;
    void doBack(QMouseEvent *pEvent, int status) override;
    bool isStartPointValid() const override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool doCheckMayTrigger() override;
    void doAfterTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(RS_Vector vector) override;
    void updateMouseButtonHints() override;
    void createPoints(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList);
    void updatePointsCount(int count);
    void setMajorStatus();
    void updateEdgePointsMode(int mode);
    bool isNonZeroLine(const RS_Vector &possiblePoint) const;
    RS_Vector getPossibleEndPointForAngle(const RS_Vector &snap);
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};
#endif // LC_ACTIONDRAWLINEPOINTS_H
