/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_SHORTCUTSTREEMODEL_H
#define LC_SHORTCUTSTREEMODEL_H

#include <QAbstractItemModel>
#include <QRegularExpression>
#include "lc_shortcuttreeitem.h"
#include "lc_actiongroupmanager.h"

class LC_ShortcutsTreeModel :public QAbstractItemModel {
    Q_OBJECT

public:
    enum {
        ICON, NAME,/* DESCRIPTION, */SHORTCUT, LAST
    };
    explicit LC_ShortcutsTreeModel(QObject *parent=nullptr);

    ~LC_ShortcutsTreeModel() override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    bool isRegexpApplied() const{return hasRegexp;};
    void setFilteringRegexp(QString &reqgexp, bool highlightMode);
    LC_ShortcutTreeItem *getItemForIndex(const QModelIndex &index) const;
    void rebuildModel(LC_ActionGroupManager *pManager);
    QModelIndexList getPersistentIndexList();
    void resetAllToDefault();
    int translateColumn(int column) const;
    bool checkForCollisions(LC_ShortcutInfo *shortcutInfo);
    void setFilterForConflicts(bool filter);
    void collectShortcuts(QList<LC_ShortcutInfo *> &items, bool includeEmpty = false);
    void applyShortcuts(const QMap<QString, QKeySequence> &map, bool replace);
    const QMap<QString, LC_ShortcutInfo *> &getShortcuts() const;
    bool isModified();
protected:
    // filtering/highlight regexp value
    QRegularExpression filteringRegexp{""};

    // flat that controls whether regexp should be applied
    bool hasRegexp{false};

    //  controls whether items matched to regexp are just highlighted or filtered
    bool regexpHighlightMode{true};

    QMap<QString, LC_ShortcutInfo*> shortcuts;

    // root item for layers hierarchy
    LC_ShortcutTreeItem *rootItem = nullptr;

    // filter content and show conflicts only
    bool filterForConflicts = false;

    void setRootItem(LC_ShortcutTreeItem *rootItem);
};

#endif // LC_SHORTCUTSTREEMODEL_H
