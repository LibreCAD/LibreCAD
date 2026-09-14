/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 pcfixindude (github.com/pcfixindude)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */
#ifndef LC_ACTIONDRAWLINEDIRECT_H
#define LC_ACTIONDRAWLINEDIRECT_H

#include <memory>
#include <vector>

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
    bool doCheckMayDrawPreview(const LC_MouseEvent *pEvent, int status) override;
    void doPreparePreviewEntities(const LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch &ctx) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doSetStartPoint(const RS_Vector &vector) override;
    const RS_Vector &getStartPointForAngleSnap() const override;
    bool isStartPointValid() const override;
    void doBack(const LC_MouseEvent *pEvent, int status) override;
    bool doProcessCommandValue(int status, const QString &c) override;
    bool doProceedCommand(int status, const QString &c) override;
    bool isAllowDirectionCommands() override;
    void updateActionPrompt() override;
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
