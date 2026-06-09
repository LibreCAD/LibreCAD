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

#ifndef LC_MENUFACTORYGRAPHICVIEW_H
#define LC_MENUFACTORYGRAPHICVIEW_H

#include "lc_menufactory_base.h"

class LC_ActionContext;
class RS_Vector;
class RS_Entity;
class QG_GraphicView;
struct LC_MenusHolder;

class LC_MenuFactoryGraphicView: public LC_MenuFactoryBase{
    Q_OBJECT
public:
    explicit LC_MenuFactoryGraphicView(QC_ApplicationWindow* mainWin, LC_ActionGroupManager* actionGroupManager, LC_MenusHolder* menusHolder)
        : LC_MenuFactoryBase(mainWin, actionGroupManager, menusHolder) {
    }

    QMenu* createGraphicViewPopupMenu(QG_GraphicView* graphicView, RS_Entity* contextEntity,
                                      const RS_Vector& contextPosition, QStringList& actionNames, bool mayInvokeDefaultMenu);
protected:
    QMenu* createGraphicViewDefaultPopupMenu(QG_GraphicView* graphicView,
                                             RS_Entity* contextEntity, const RS_Vector& contextPosition);
    QMenu* createGraphicViewCustomPopupMenu(QG_GraphicView* graphicView,
                                            RS_Entity* contextEntity, const RS_Vector& contextPosition,
                                            QStringList& actionNames) const;
    void createGVMenuView(QMenu* ctxMenu) const;
    void createGVMenuFiles(QMenu* ctxMenu) const;
    void createGVMenuEntitySpecific(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos);
    void createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity);
    void createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity, const QString &actionText);
    void createGVMenuModifyGeneral(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos,
                                   LC_ActionContext* actionContext);
    void createGVMenuSelect(QMenu* ctxMenu, RS_Entity* contextEntity, const RS_Vector& contextPosition, LC_ActionContext* actionContext, bool hasSelection) const;
    void createGVMenuRecent(const QG_GraphicView* graphicView, QMenu* ctxMenu, LC_ActionContext* actionContext,
                            RS_Entity* contextEntity, const RS_Vector &contextPosition, bool hasEntity) const;

    void createGVMenuEdit(QMenu* ctxMenu, LC_ActionContext* actionContext,RS_Entity* contextEntity, const RS_Vector &contextPosition) const;
    void createGVMenuOptions(QMenu* ctxMenu) const;

    void addProxyActions(QMenu* menu, RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext,
                       const std::vector<QString>& actionNames) const;
    void addActionProxy(QMenu* menu, QAction* srcAction, RS_Entity* entity, const RS_Vector& pos,
                           LC_ActionContext* actionContext) const;
    void addActionProxy(QMenu* menu, const QString& actionName, RS_Entity* entity, const RS_Vector& pos,
                        LC_ActionContext* actionContext) const;
    QMenu* addProxyActionsSubMenu(QMenu* menu, const QString &subMenuName, const char* subMenuIconName, RS_Entity* entity,
                                  const RS_Vector& pos, LC_ActionContext* actionContext,
                                  const std::vector<QString>& actionNames) const;
};

#endif
