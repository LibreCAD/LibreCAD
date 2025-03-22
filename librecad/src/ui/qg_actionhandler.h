/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef QG_ACTIONHANDLER_H
#define QG_ACTIONHANDLER_H

#include "rs_actioninterface.h"
#include "rs_graphicview.h"
class LC_DefaultActionContext;
class QG_SnapToolBar;
class RS_Layer;
/**
 * This class can trigger actions (from menus, buttons, ...).
 */
class QG_ActionHandler:public QObject {
Q_OBJECT

public:
    QG_ActionHandler(QObject *parent);
    virtual ~QG_ActionHandler() = default;
    RS_ActionInterface *getCurrentAction();
    RS_ActionInterface *setCurrentAction(RS2::ActionType id);
    /**
    * Kills all running selection actions. Called when a selection action
    * is launched to reduce confusion.
   */
    void killSelectActions();
    /**
    * @brief killAllActions kill all actions
    */
    void killAllActions();
    bool keycode(const QString &code);
    //special handling of actions issued from command line, currently used for snap actions
    //return true if handled
    bool commandLineActions(RS2::ActionType id);
    bool command(const QString &cmd);
    QStringList getAvailableCommands();
    RS_SnapMode getSnaps();
    RS2::SnapRestriction getSnapRestriction();
    void set_snap_toolbar(QG_SnapToolBar *snap_toolbar);
    void setDocumentAndView(RS_Document* document, RS_GraphicView* graphicView);
public slots:
    void slotZoomIn(); // fixme - remove
    void slotZoomOut();// fixme - remove

    void slotSetSnaps(RS_SnapMode const &s);
    void slotSnapFree();
    void slotSnapGrid();
    void slotSnapEndpoint();
    void slotSnapOnEntity();
    void slotSnapCenter();
    void slotSnapMiddle();
    void slotSnapDist();
    void slotSnapMiddleManual();
    void slotSnapIntersection();
    void slotSnapIntersectionManual();
    void slotRestrictNothing();
    void slotRestrictOrthogonal();
    void slotRestrictHorizontal();
    void slotRestrictVertical();
    void disableSnaps();
    void disableRestrictions();
    void slotSetRelativeZero();
    void slotLockRelativeZero(bool on);
    void toggleVisibility(RS_Layer *layer);
    void toggleLock(RS_Layer *layer);
    void togglePrint(RS_Layer *layer);
    void toggleConstruction(RS_Layer *layer);

private:
    // Type of draw order selected command
    RS2::ActionType orderType{RS2::ActionOrderTop};
    QG_SnapToolBar *snap_toolbar{nullptr};
    LC_DefaultActionContext* m_actionContext{nullptr}; // fixme complete initialization
};

#endif
