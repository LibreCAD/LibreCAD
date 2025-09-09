/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef RS_EVENTHANDLER_H
#define RS_EVENTHANDLER_H

#include <QObject>

#include "rs.h"

class LC_CoordinatesParser;
class RS_ActionInterface;
class QAction;
class QMouseEvent;
class QKeyEvent;
class RS_CommandEvent;
class RS_Vector;
class RS_GraphicView;

struct RS_SnapMode;

/**
 * The event handler owns and manages all actions that are currently
 * active. All events going from the view to the actions come over
 * this class.
 */
class RS_EventHandler : public QObject {
    Q_OBJECT
public:
    explicit RS_EventHandler(RS_GraphicView* parent = 0);
    ~RS_EventHandler() override;
    void uncheckQAction();
    void setQAction(QAction* action, bool forceKillOtherActions);
    QAction* getQAction();

    void back();
    void enter();

    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseLeaveEvent();
    void mouseEnterEvent();

    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);

    void commandEvent(RS_CommandEvent* e);
    void enableCoordinateInput();
    void disableCoordinateInput();

    void setDefaultAction(RS_ActionInterface* action);
    RS_ActionInterface* getDefaultAction() const;

    bool setCurrentAction(std::shared_ptr<RS_ActionInterface> action);
    RS_ActionInterface* getCurrentAction();
    bool isValid(RS_ActionInterface* action) const;

    void killSelectActions();
    void killAllActions();

    bool hasAction();
    void cleanUp();
    void debugActions() const;
    void setSnapMode(RS_SnapMode sm);
    void setSnapRestriction(RS2::SnapRestriction sr);
    //! return true if the current action is for selecting
    bool inSelectionMode();

    void notifyLastActionFinished();
private:
    std::unique_ptr<LC_CoordinatesParser> m_coordinatesParser;
    RS_GraphicView* m_graphicView;
    QAction* m_QAction{nullptr};
    std::shared_ptr<RS_ActionInterface> m_defaultAction{nullptr};
    QList<std::shared_ptr<RS_ActionInterface>> m_currentActions;
    bool m_coordinateInputEnabled{true};
    void checkLastActionCompletedAndUncheckQAction(const std::shared_ptr<RS_ActionInterface> &lastAction);
};

#endif
