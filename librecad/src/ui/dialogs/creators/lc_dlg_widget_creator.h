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

#ifndef LC_DLGWIDGETCREATOR_H
#define LC_DLGWIDGETCREATOR_H

#include <QListWidget>

#include "lc_dialog.h"
#include "lc_menuactivator.h"

class LC_ActionGroupManager;

namespace Ui {
    class LC_DlgWidgetCreator;
}

class LC_DlgWidgetCreator : public LC_Dialog{
    Q_OBJECT
public:
    void updateSaveAndDeleteButtons() const;
    void reloadCustomWidgets();
    explicit LC_DlgWidgetCreator(QWidget *parent, bool forMenu, LC_ActionGroupManager* actionGroupManager);
    ~LC_DlgWidgetCreator() override;
protected slots:
    void addChosenAction() const;
    void addSeparator() const;
    void addChosenActionForItem(const QListWidgetItem* item) const;
    void removeChosenAction() const;
    void removeAllChosenActions() const;
    void removeChosenActionForItem(QListWidgetItem* item) const;
    void onCategoryActivated(int index) const;
    void onAssignMenu(bool checked);
    void onUnAssignMenu(bool checked);
    void onWidgetNameIndexChanged(int index);
    void newWidget();
    void saveWidget();
    void destroyWidget();
    void exportConfiguration();
    void importConfiguration();
signals:
    void widgetCreationRequest(const QString&, const QStringList&, int areaIndex);
    void widgetDestroyRequest(const QString&);
private:
    Ui::LC_DlgWidgetCreator *ui;
    bool m_forMenu;
    LC_ActionGroupManager* m_actionGroupManager;
    QList<LC_MenuActivator*> m_menuActivators;
    void setCategoryAll() const;
    void loadCustomWidgets();
    LC_MenuActivator* findMenuActivator(const QString& menuName);
    void removeMenuActivator(const QString& menuName);
    QString getSettingsGroupName() const;
    QString createMenuItemDisplayName(const QString& key, const LC_MenuActivator* activator);
    QString createMenuItemDisplayName(const QString& key);
    QStringList getChosenActionNames() const;
    void loadWidgetActions(int index);
    void onToolbarPlacementIndexChanged(int index);
};

#endif
