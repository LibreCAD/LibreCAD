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
class LC_ActionDrawPointsLine :public LC_AbstractActionDrawLine {
    Q_OBJECT
public:
    /**
     * Controls how to handle points on edges of the line
     */
    enum EdgesMode{
        DRAW_EDGE_NONE, // no points in line's  edges
        DRAW_EDGE_BOTH, // each edge of line has point
        DRAW_EDGE_START, // point in added in start point of line, no point in endpoint
        DRAW_EDGE_END // point is created in end point of line, no point in start point
    };

    LC_ActionDrawPointsLine(LC_ActionContext *actionContext, bool drawMiddle);
    ~LC_ActionDrawPointsLine() override;
    int getPointsCount() const {return m_pointsCount;}
    void setPointsCount(const int count) {m_pointsCount = count;}
    int getEdgePointsMode() const{return m_edgePointsMode;}
    void setEdgePointsMode(int value);
    void setFixedDistanceMode(const bool value) {m_fixedDistanceMode = value;}
    bool isFixedDistanceMode() const{return m_fixedDistanceMode;}
    void setWithinLineMode(const bool value) {m_withinLineMode = value;}
    bool isWithinLineMode() const{return m_withinLineMode;}
    double getPointsDistance() const{return m_fixedDistance;}
    void setPointsDistance(const double val){m_fixedDistance = val;}
    void init(int status) override;
    QStringList getAvailableCommands() override;
protected:


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
    int m_pointsCount = 0;
    /**
     * how to handle points on edges
     */
    int m_edgePointsMode = DRAW_EDGE_BOTH;

    /**
     * flag that indicates that point 1 is set
     */
    bool m_point1Set = false;

    /**
     * start point for line
     */
    RS_Vector m_startpoint = RS_Vector(false);

    /**
     * end point of line
     */
    RS_Vector m_endpoint = RS_Vector(false);

    /**
     * flag that defines whether distance between points is fixed
     */
    bool m_fixedDistanceMode = false;

    /**
     * flag that is applicable if we'll draw points with fixed distance between them.
     * It specifies whether points should be within a line (and so some points may be skipped if line is too short and
     * so less than pointsCount points will be created or (if false) if the line is rather considered as a direction vector
     * and the number of points specified by pointsCount will be always equal to pointsCount (plus edges points, of course)
     */
    bool m_withinLineMode = false;

    /**
     * Distance between points for fixed distance mode
     */
    double m_fixedDistance = 0.0;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doProceedCommand(int status, const QString &command) override;
    bool doProcessCommandValue(int status, const QString &c) override;
    const RS_Vector& getStartPointForAngleSnap() const override;
    void doBack(const LC_MouseEvent* e, int status) override;
    bool isStartPointValid() const override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx)  override;
    bool doCheckMayTrigger() override;
    void doAfterTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(const RS_Vector& vector) override;
    void updateActionPrompt() override;
    void createPoints(const RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList);
    void updatePointsCount(int count);
    void setMajorStatus();
    void updateEdgePointsMode(int mode);
    bool isNonZeroLine(const RS_Vector &possiblePoint) const;
    RS_Vector getPossibleEndPointForAngle(const RS_Vector &snap) const;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    bool isAllowDirectionCommands() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
