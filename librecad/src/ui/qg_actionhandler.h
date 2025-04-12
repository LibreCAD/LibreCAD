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
#include "rs_snapper.h"

class RS_ActionInterface;
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
    ~QG_ActionHandler() override = default;
    RS_ActionInterface *getCurrentAction() const;
    RS_ActionInterface *setCurrentAction(RS2::ActionType id);
    /**
    * Kills all running selection actions. Called when a selection action
    * is launched to reduce confusion.
   */
    void killSelectActions() const;
    /**
    * @brief killAllActions kill all actions
    */
    void killAllActions() const;
    bool keycode(const QString &code);
    //special handling of actions issued from command line, currently used for snap actions
    //return true if handled
    bool commandLineActions(RS2::ActionType id) const;
    bool command(const QString &cmd);
    QStringList getAvailableCommands() const;
    RS_SnapMode getSnaps() const;
    RS2::SnapRestriction getSnapRestriction() const;
    void set_snap_toolbar(QG_SnapToolBar *snap_toolbar);
    void setDocumentAndView(RS_Document* document, RS_GraphicView* graphicView);
    void setActionContext(LC_DefaultActionContext* actionContext) {m_actionContext = actionContext;};
public slots:
    void slotZoomIn(); // fixme - remove
    void slotZoomOut();// fixme - remove

    void slotSetSnaps(RS_SnapMode const &s) const;
    void slotSnapFree() const;
    void slotSnapGrid() const;
    void slotSnapEndpoint() const;
    void slotSnapOnEntity() const;
    void slotSnapCenter() const;
    void slotSnapMiddle() const;
    void slotSnapDist() const;
    void slotSnapMiddleManual();
    void slotSnapIntersection() const;
    void slotSnapIntersectionManual();
    void slotRestrictNothing() const;
    void slotRestrictOrthogonal() const;
    void slotRestrictHorizontal() const;
    void slotRestrictVertical() const;
    void disableSnaps() const;
    void disableRestrictions() const;
    void slotSetRelativeZero();
    void slotLockRelativeZero(bool on);
    void toggleVisibility(RS_Layer *layer);
    void toggleLock(RS_Layer *layer);
    void togglePrint(RS_Layer *layer);
    void toggleConstruction(RS_Layer *layer);

    void slotBlocksDefreezeAll(); // fixme - remove
    void slotBlocksFreezeAll(); // fixme - remove
    void slotBlocksAdd(); // fixme - remove
    void slotBlocksRemove(); // fixme - remove
    void slotBlocksAttributes(); // fixme - remove
    void slotBlocksEdit(); // fixme - remove
    void slotBlocksSave(); // fixme - remove
    void slotBlocksInsert(); // fixme - remove
    void slotBlocksToggleView(); // fixme - remove
    void slotBlocksCreate(); // fixme - remove
    void slotBlocksExplode(); // fixme - remove
    void slotLayersDefreezeAll(); // fixme - remove
    void slotLayersFreezeAll();// fixme - remove
    void slotLayersUnlockAll();// fixme - remove
    void slotLayersLockAll();// fixme - remove
    void slotLayersAdd();// fixme - remove
    void slotLayersRemove();// fixme - remove
    void slotLayersEdit();// fixme - remove
    void slotLayersToggleView();// fixme - remove
    void slotLayersToggleLock();// fixme - remove
    void slotLayersTogglePrint();// fixme - remove
    void slotLayersToggleConstruction();// fixme - remove
    void slotLayersExportSelected();// fixme - remove
    void slotLayersExportVisible();// fixme - remove

private:
    // Type of draw order selected command
    RS_GraphicView *view {nullptr};
    RS_Document* document {nullptr};
    RS2::ActionType orderType{RS2::ActionOrderTop};
    QG_SnapToolBar *snap_toolbar{nullptr};
    LC_DefaultActionContext* m_actionContext{nullptr}; // fixme complete initialization
};

#endif
