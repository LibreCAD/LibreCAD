/****************************************************************************
**
* Action that creates a set of lines, with support of angle and "snake" mode

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
#ifndef LC_ACTIONDRAWLINESNAKE_H
#define LC_ACTIONDRAWLINESNAKE_H

#include "lc_abstractactiondrawline.h"
#include "rs_line.h"

class LC_ActionDrawLineSnake :public LC_AbstractActionDrawLine {
    Q_OBJECT
public:
    LC_ActionDrawLineSnake(LC_ActionContext *actionContext, RS2::ActionType actionType);
    ~LC_ActionDrawLineSnake() override;
    void init(int status) override;
    void close();
    void next();
    void undo();
    void redo();
    void polyline();
    bool mayClose() const;
    bool mayUndo() const;
    bool mayStart() override;
    bool mayRedo() const;
    QStringList getAvailableCommands() override;
protected:
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool doProceedCommand(int status, const QString &command) override;
    bool doProcessCommandValue(int status, const QString &c) override;
    const RS_Vector& getStartPointForAngleSnap() const override;
    void doBack(const LC_MouseEvent* e, int status) override;
    bool isStartPointValid() const override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx)  override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(const RS_Vector& start) override;
    bool doCheckMayDrawPreview(const LC_MouseEvent* e, int status) override;
    void updateActionPrompt() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
private:
    /// History Actions
    enum HistoryAction {
        HA_SetStartpoint,   ///< Setting the start point
        HA_SetEndpoint,     ///< Setting the endpoint
        HA_Close,           ///< Close group of lines
        HA_Next,            ///< Start new group of lines
        HA_Polyline,
    };

    struct History{
        explicit History(const HistoryAction a,
                         const RS_Vector& p,
                         const RS_Vector& c,
                         const int s) :
            prevPt( p),
            currPt( c),
            histAct( a),
            startOffset( s) {}

        explicit History(const History& h) :
            prevPt( h.prevPt),
            currPt( h.currPt),
            histAct( h.histAct),
            startOffset( h.startOffset) {}

        History& operator=(const History& rho) {
            histAct     = rho.histAct;
            prevPt      = rho.prevPt;
            currPt      = rho.currPt;
            startOffset = rho.startOffset;
            return *this;
        }
        RS_Vector       prevPt;     // previous coordinate
        RS_Vector       currPt;     // current coordinate
        HistoryAction    histAct;    // action to undo/redo
        int             startOffset;// offset to start point for close method
    };

    struct ActionData
    {
        /// Line data defined so far
        RS_LineData data = RS_LineData();
        /// Point history (undo/redo pointer)
        int  historyIndex {-1};
        /// start point offset for close method
        int  startOffset {0};

        /// Point history (undo/redo buffer)
        std::vector<History> history;

        /// wrapper for historyIndex to avoid 'signedness' warnings where std::vector-methods expect size_t
        /// also, offset helps in close method to find starting point
        size_t index(int offset = 0) const;
    };

    /**
     * points data
     */
    std::unique_ptr<ActionData> m_actionData;
    void resetPoints();
    void addHistory(HistoryAction a, const RS_Vector& p, const RS_Vector& c, int s) const;
    void completeLineSegment(bool close);
    void calculateAngleSegment(double distance) const;
    RS_Vector calculateAngleEndpoint(const RS_Vector &snap) const;
    double defineActualSegmentAngle(double relativeAngleRad) const;
    bool isNonZeroLine(const RS_Vector &possiblePoint) const;
    void createEntities(const RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList) const;
protected:
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
