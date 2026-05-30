/**
 * Draw Fast: draw sequential segments by typed distance and mouse aim direction.
 * Commands: df / dfast
 *
 * Usage:
 *   1. Click to set the start point.
 *   2. Type a distance and press Enter; aim the mouse to choose direction, or
 *      simply click the desired endpoint.
 *   3. Each confirmed point advances the start automatically.
 *   4. Right-click or Escape to finish.  Type "undo" to remove the last segment.
 *
 * Subcommands after the first wall segment:
 *   o / opening  — draw a wall opening (perpendicular markers + span)
 *   d / door     — draw a door symbol (span + leaf + swing arc)
 * Subcommands whenever a start point exists:
 *   w / window   — draw a window symbol (center + offsets + caps)
 */
#ifndef LC_ACTIONDRAWLINEDIRECT_H
#define LC_ACTIONDRAWLINEDIRECT_H

#include "lc_abstractactiondrawline.h"
#include "rs_arc.h"
#include "rs_line.h"

class LC_ActionDrawLineDirect : public LC_AbstractActionDrawLine {
    Q_OBJECT
public:
    explicit LC_ActionDrawLineDirect(LC_ActionContext *actionContext);
    ~LC_ActionDrawLineDirect() override;
    void undo();
    bool mayUndo() const;
    bool mayStart() override;
    QStringList getAvailableCommands() override;

    enum ExtStatus {
        SetOpeningWidth = LAST,
        SetOpeningAim,
        SetWindowWidth,
        SetWindowAim,
        SetDoorWidth,
        SetDoorSwingSide,
        SetDoorHingeSide,
    };

protected:
    bool doCheckMayDrawPreview(LC_MouseEvent *pEvent, int status) override;
    void doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(RS_Vector vector) override;
    const RS_Vector &getStartPointForAngleSnap() const override;
    bool isStartPointValid() const override;
    void doBack(LC_MouseEvent *pEvent, int status) override;
    bool doProcessCommandValue(int status, const QString &c) override;
    bool doProceedCommand(int status, const QString &c) override;
    void updateMouseButtonHints() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
private:
    struct ActionData {
        RS_LineData data = RS_LineData();
        std::vector<RS_Vector> prevPoints;
    };
    std::unique_ptr<ActionData> m_actionData;

    // Opening mode
    RS_Vector          m_openingStart;
    double             m_openingWallAngle{0.0};
    double             m_openingWidth{0.0};
    QList<RS_LineData> m_pendingOpening;

    // Window mode
    RS_Vector          m_windowStart;
    double             m_windowWidth{0.0};

    // Door mode
    RS_Vector          m_doorStart;
    double             m_doorWallAngle{0.0};
    double             m_doorWidth{0.0};
    double             m_doorSwingSign{1.0};
    bool               m_doorHingeAtStart{true};
    QList<RS_ArcData>  m_pendingArcs;

    void completeLineSegment();
    double getLastWallAngle() const;
    void startOpeningMode();
    void completeOpening(const RS_Vector &snap);
    void startWindowMode();
    void completeWindow(const RS_Vector &snap);
    void completeWindowAlongWall();
    void startDoorMode();
    void completeDoor();
    void appendDoorPreview(QList<RS_Entity *> &list, double swingSign, bool hingeAtStart) const;
};

#endif // LC_ACTIONDRAWLINEDIRECT_H
