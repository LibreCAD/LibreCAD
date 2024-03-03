#ifndef LC_ACTIONDRAWLINEREL_H
#define LC_ACTIONDRAWLINEREL_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_line.h"

class LC_ActionDrawLineRel :public RS_PreviewActionInterface
{
    Q_OBJECT



public:
    enum Status {
        SetStartPoint,
        SetDirection,
        SetDistance,
        SetPoint,
        SetAngle
    };

    enum Direction{
        DIRECTION_NONE, DIRECTION_X, DIRECTION_Y, DIRECTION_POINT, DIRECTION_ANGLE
    };


    /// History Actions
    enum HistoryAction {
        HA_SetStartpoint,   ///< Setting the startpoint
        HA_SetEndpoint,     ///< Setting the endpoint
        HA_Close,           ///< Close group of lines
        HA_Next,            ///< Start new group of lines
        HA_Polyline,
    };

    struct History
    {
        explicit History(HistoryAction a,
                         const RS_Vector& p,
                         const RS_Vector& c,
                         const int s) :
            histAct( a),
            prevPt( p),
            currPt( c),
            startOffset( s) {}

        explicit History(const History& h) :
            histAct( h.histAct),
            prevPt( h.prevPt),
            currPt( h.currPt),
            startOffset( h.startOffset) {}

        History& operator=(const History& rho) {
            histAct     = rho.histAct;
            prevPt      = rho.prevPt;
            currPt      = rho.currPt;
            startOffset = rho.startOffset;
            return *this;
        }

        HistoryAction    histAct;    ///< action to undo/redo
        RS_Vector       prevPt;     ///< previous coordinate
        RS_Vector       currPt;     ///< current coordinate
        int             startOffset;///< offset to start point for close method
    };

    struct Points
    {
        /// Line data defined so far
        RS_LineData data;
        /// Point history (undo/redo pointer)
        int  historyIndex {-1};
        /// start point offset for close method
        int  startOffset {0};

        /// Point history (undo/redo buffer)
        std::vector<History> history;

        /// wrapper for historyIndex to avoid 'signedness' warnings where std::vector-methods expect size_t
        /// also, offset helps in close method to find starting point
        size_t index(const int offset = 0);
    };


    LC_ActionDrawLineRel(RS_EntityContainer& container, RS_GraphicView& graphicView, int direction = LC_ActionDrawLineRel::DIRECTION_NONE);
    ~LC_ActionDrawLineRel() override;
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

    void close();
    void next();
    void undo();
    void redo();
    void polyline();
    void setNewStartPointState();
    void setSetPointDirectionState();
    void setSetXDirectionState();
    void setSetYDirectionState();
    void setSetAngleDirectionState();
    int getDirection(){return direction;};
    double getAngleValue();
    bool  isAngleRelative();
    bool mayClose();
    bool mayUndo() const;
    bool mayStart();
    bool mayRedo();
    QStringList getAvailableCommands() override;
    void setAngleValue(double value);
    void setAngleIsRelative(bool value);

private:
    std::unique_ptr<Points> pPoints;
    void resetPoints();
    int direction {DIRECTION_NONE};
    bool negativeDirection {false};
    void addHistory(HistoryAction a, const RS_Vector& p, const RS_Vector& c, const int s);
    void doTrigger(bool close);
    void updateOptions();
    void setSetAngleState(bool relative);
    bool angleIsRelative {true};
    double angleValue;
    void calculateAngleSegment(double distance);
    RS_Vector calculateAngleEndpoint(const RS_Vector &snap);
    double defineActualSegmentAngle(double realAngle);
};

#endif // LC_ACTIONDRAWLINEREL_H
