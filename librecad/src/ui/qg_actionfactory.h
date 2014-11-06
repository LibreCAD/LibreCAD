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

#ifndef QG_ACTIONFACTORY_H
#define QG_ACTIONFACTORY_H

#include <QAction>

#include "qg_actionhandler.h"

class QMenu;
class QToolBar;

/**
 * This class can store recent files in a list.
 */
class QG_ActionFactory : public QObject {
    Q_OBJECT

public:
    QG_ActionFactory(QG_ActionHandler* ah, QWidget* w);
    virtual ~QG_ActionFactory();

    /**
     * @brief createAction, create GUI action by action type, and connect to slot of QObject obj
     * @param id, Action type enum
     * @param obj, the QObject used to connect slot per action type
     * @param obj2, a QDockWidget, to accept toggleViewAction() from the current action to show/hide
     * @return a pointer to the action created
     */
    QAction* createAction(RS2::ActionType id, QObject* obj, QObject* obj2=NULL) const;

    /**
     * @brief addGUI, create action by action type and add the action to menu
     * @param menu, a pointer to QMemu to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param id, action type
     * @return a pointer to the action created
     */
    QAction*  addGUI(QMenu* menu, QObject* obj, RS2::ActionType id) const;
    /**
     * @brief addGUI, create action by action type and add the action to menu
     * @param menu, a pointer to QMemu to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param obj2, a QDockWidget, to accept toggleViewAction() from the current action to show/hide
     * @param id, action type
     * @return a pointer to the action created
     */
    QAction*  addGUI(QMenu* menu, QObject* obj, QObject* obj2, RS2::ActionType id) const;
    /**
     * @brief addGUI, create action by action type and add the action to menu
     * @param menu, a pointer to QMemu to add the action
     * @param toolbar, a pointer to QToolBar to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param id, action type
     * @return a pointer to the action created
     */
    QAction*  addGUI(QMenu* menu, QToolBar* toolbar, QObject* obj, RS2::ActionType id) const;
    /**
     * @brief addGUI, create action by action type and add the action to menu
     * @param menu, a pointer to QMemu to add the action
     * @param toolbar, a pointer to QToolBar to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param obj2, a QDockWidget, to accept toggleViewAction() from the current action to show/hide
     * @param id, action type
     * @return a pointer to the action created
     */
    QAction*  addGUI(QMenu* menu, QToolBar* toolbar, QObject* obj, QObject* obj2, RS2::ActionType id) const;

    /**
     * @brief addGUI, create action by a list of action types and add the actions to menu
     * @param menu, a pointer to QMemu to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param list, an initializer list of action types
     */
    void addGUI(QMenu* menu, QObject* obj, const std::initializer_list<RS2::ActionType>& list) const;
    /**
     * @brief addGUI, create action by a list of action types and add the actions to menu
     * @param menu, a pointer to QMemu to add the action
     * @param toolbar, a pointer to QToolBar to add the action
     * @param obj, the QObject used to connect slot per action type
     * @param list, an initializer list of action types
     */
    void addGUI(QMenu* menu, QToolBar* toolbar, QObject* obj, const std::initializer_list<RS2::ActionType>& list) const;


private:
    QG_ActionHandler* actionHandler;
    QWidget* widget;
};

#endif

