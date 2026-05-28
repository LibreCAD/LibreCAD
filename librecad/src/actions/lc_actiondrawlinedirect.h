/****************************************************************************
**
* Experimental direct-line action: click start, aim with mouse, type length,
* press Enter to place segment in aimed direction, auto-continue.
*
* Adapted from LC_ActionDrawLineSnake.
* Original copyright (C) 2024 LibreCAD.org / sand1024
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
****************************************************************************/

#ifndef LC_ACTIONDRAWLINEDIRECT_H
#define LC_ACTIONDRAWLINEDIRECT_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_line.h"
#include "lc_abstractactiondrawline.h"

class LC_ActionDrawLineDirect : public LC_AbstractActionDrawLine {
    Q_OBJECT

public:
    enum HistoryAction {
        HA_SetStartpoint,
        HA_SetEndpoint,
        HA_Close,
        HA_Next,
        HA_Polyline,
    };

    struct History {
        explicit History(HistoryAction a,
                         const RS_Vector& p,
                         const RS_Vector& c,
                         const int s)
            : histAct(a), prevPt(p), currPt(c), startOffset(s) {}

        explicit History(const History& h)
            : histAct(h.histAct), prevPt(h.prevPt), currPt(h.currPt), startOffset(h.startOffset) {}

        History& operator=(const History& rho) {
            histAct     = rho.histAct;
            prevPt      = rho.prevPt;
            currPt      = rho.currPt;
            startOffset = rho.startOffset;
            return *this;
        }

        HistoryAction histAct;
        RS_Vector     prevPt;
        RS_Vector     currPt;
        int           startOffset;
    };

    struct Points {
        RS_LineData data = RS_LineData();
        int historyIndex{-1};
        int startOffset{0};
        std::vector<History> history;
        size_t index(const int offset = 0);
    };

    LC_ActionDrawLineDirect(RS_EntityContainer& container, RS_GraphicView& graphicView);
    ~LC_ActionDrawLineDirect() override;

    void init(int status) override;
    void updateMouseButtonHints() override;

    void close();
    void next();
    void undo();
    void redo();
    void polyline();
    bool mayClose();
    bool mayUndo() const;
    bool mayStart() override;
    bool mayRedo();
    QStringList getAvailableCommands() override;

protected:
    bool doProceedCommand(RS_CommandEvent *pEvent, const QString &qString) override;
    bool doProcessCommandValue(RS_CommandEvent *e, const QString &c) override;
    const RS_Vector& getStartPointForAngleSnap() const override;
    void doBack(QMouseEvent *pEvent, int status) override;
    bool isStartPointValid() const override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void onCoordinateEvent(const RS_Vector &coord, bool isZero, int status) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(RS_Vector vector) override;
    bool doCheckMayDrawPreview(QMouseEvent *pEvent, int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;

private:
    std::unique_ptr<Points> pPoints;

    void resetPoints();
    void addHistory(HistoryAction a, const RS_Vector& p, const RS_Vector& c, const int s);
    void completeLineSegment(bool close);
    void calculateAngleSegment(double distance);
    RS_Vector calculateAngleEndpoint(const RS_Vector &snap);
    double defineActualSegmentAngle(double realAngle);
    bool isNonZeroLine(const RS_Vector &possiblePoint) const;
    void createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList);
};

#endif // LC_ACTIONDRAWLINEDIRECT_H
