/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_ACTIONDRAWLINETOPERSPECTIVEPOINT_H
#define LC_ACTIONDRAWLINETOPERSPECTIVEPOINT_H

#include "lc_undoabledocumentmodificationaction.h"

class LC_ActionDrawLineRadiant : public LC_UndoableDocumentModificationAction{
    Q_OBJECT
public:
    enum RadiantIdx {
        ONE = 0, TWO, THREE, FOUR, LAST
    };

    enum LenghtType {
        LINE,
        BY_X,
        BY_Y,
        TO_POINT,
        FREE
    };
    explicit LC_ActionDrawLineRadiant(LC_ActionContext *actionContext);
    QStringList getAvailableCommands() override;
    void init(int status) override;
    bool isFreeLength() const {return m_lengthType == FREE;}
    RS_Vector getActiveRadiant() const {return m_radiantPoints[m_activeRadiantIndex];}
    RadiantIdx getActiveRadiantIndex() const {return m_activeRadiantIndex;}
    void setLength(double len) {m_length = len;}
    double getLength() const {return m_length;}
    void setLengthType(LenghtType type);
    LenghtType getLenghType() const {return m_lengthType;}
    RS_Vector getRadiantPoint(RadiantIdx idx) const {return m_radiantPoints[idx];}
    void setRadiantPoint(RadiantIdx idx, const RS_Vector& pos) {m_radiantPoints[idx] = pos;}
    void setActiveRadiantIndex(RadiantIdx idx) { m_activeRadiantIndex = (idx == LAST) ? FOUR : idx;}
    void setActiveX(const double val) {m_radiantPoints[m_activeRadiantIndex].setX(val);}
    void setActiveRadiantPoint(const RS_Vector& v) {m_radiantPoints[m_activeRadiantIndex] = v;}
    void setActiveY(double val) {m_radiantPoints[m_activeRadiantIndex].setY(val);}
    double getActiveX() const {return m_radiantPoints[m_activeRadiantIndex].x;}
    double getActiveY() const {return m_radiantPoints[m_activeRadiantIndex].y;}
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateActionPrompt() override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    RS_Vector defineLineSecondPointAuto(const RS_Vector& snapped) const;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    RS_Vector defineLineSecondPointFree(const RS_Vector& snapped) const;
    void doSaveOptions() override;
    void doLoadOptions() override;


    enum State{
        SetPoint,
        SetPoint2,
        SetRadiant,
        SetLength,
        SetLengthType,
        SetActive
    };

    RS_Vector m_startPoint;
    RS_Vector m_secondSnapPoint;

    RS_Vector m_radiantPoints[LAST];
    RadiantIdx m_activeRadiantIndex {ONE};
    double m_length {100.0};
    LenghtType m_lengthType {LINE};

    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    bool doUpdatePointByInteractiveInput(const QString& tag, RS_Vector& point) override;

    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelections(const LC_DocumentModificationBatch& ctx) override;
    void previewRadiantLine(const RS_Vector& snapped, const RS_Vector& secondPoint) const;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
