/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024
 Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#ifndef LC_CREATORINVOKER_H
#define LC_CREATORINVOKER_H
#include <QMap>
#include <QObject>

#include "lc_menuactivator.h"

class RS_Entity;
class QAction;
class QG_GraphicView;
class QString;
class LC_ActionGroupManager;
class QC_ApplicationWindow;

class LC_CreatorInvoker : public QObject{
    Q_OBJECT
public:
    LC_CreatorInvoker(QC_ApplicationWindow * appWindows, LC_ActionGroupManager * actionGrupManager);
    void createCustomToolbars(bool showTooltips);
    void invokeToolbarCreator();
    void invokeMenuCreator();
    bool getMenuActionsForMouseEvent(QMouseEvent* event, RS_Entity* entity, QStringList& actions);
protected slots:
    void createToolbar(const QString& toolbar_name, const QStringList& actionNames, int areaIndex);
    void destroyToolbar(const QString& toolbar_name);
private:
    QC_ApplicationWindow *m_appWindow;
    LC_ActionGroupManager* m_actionGroupManager {nullptr};
    QList<LC_MenuActivator*> m_menuActivators;
    bool m_showToolbarTooltips {false};
    void loadMenuActivators();
    QAction*  getAction(const QString & key);
    bool isDefaultMenuInvokerEvent(QMouseEvent* event);
};
#endif // LC_CREATORINVOKER_H
