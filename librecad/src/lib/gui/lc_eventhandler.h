/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#ifndef LC_EVENTHANDLER_H
#define LC_EVENTHANDLER_H

#include <QObject>

#include "rs.h"

class LC_CoordinatesParser;
struct RS_SnapMode;
class RS_ActionInterface;
class RS_CommandEvent;
class QKeyEvent;
class QMouseEvent;
class QAction;
class RS_GraphicView;

class LC_EventHandler : public QObject {
    Q_OBJECT
public:
    explicit LC_EventHandler(RS_GraphicView* parent = nullptr);
    ~LC_EventHandler() override;
    void uncheckQAction();
    void setQAction(QAction* action);
    QAction* getQAction() const;

    void back(Qt::KeyboardModifiers modifiers);
    void enter();

    void mousePressEvent(QMouseEvent *e) const;
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseLeaveEvent() const;
    void mouseEnterEvent() const;

    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);

    void commandEvent(RS_CommandEvent* e);
    void enableCoordinateInput();
    void disableCoordinateInput();

    void setDefaultAction(RS_ActionInterface* action);
    RS_ActionInterface* getDefaultAction() const;

    bool setCurrentAction(std::shared_ptr<RS_ActionInterface> action);
    void resumeAction(const std::shared_ptr<RS_ActionInterface>& action);
    RS_ActionInterface* getCurrentAction() const;
    bool isValid(const RS_ActionInterface* action) const;
    bool killAllActions();
    bool hasAction() const;
    void setSnapMode(RS_SnapMode sm) const;
    void setSnapRestriction(RS2::SnapRestriction sr) const;
    void notifyLastActionFinished();
private:
    std::unique_ptr<LC_CoordinatesParser> m_coordinatesParser;
    RS_GraphicView* m_graphicView;
    QAction* m_QAction{nullptr};
    std::shared_ptr<RS_ActionInterface> m_defaultAction{nullptr};
    std::shared_ptr<RS_ActionInterface> m_currentAction{nullptr};
    QList<std::shared_ptr<RS_ActionInterface>> m_currentActions;
    bool m_coordinateInputEnabled{true};
    bool checkLastActionFinishedAndUncheckQAction();
    void switchToDefaultAction();
};

#endif
