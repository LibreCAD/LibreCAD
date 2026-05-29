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
 */
#ifndef LC_ACTIONDRAWLINEDIRECT_H
#define LC_ACTIONDRAWLINEDIRECT_H

#include "lc_abstractactiondrawline.h"
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
    void completeLineSegment();
};

#endif // LC_ACTIONDRAWLINEDIRECT_H
