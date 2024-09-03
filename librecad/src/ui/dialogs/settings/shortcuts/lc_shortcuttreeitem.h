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

#ifndef LC_SHORTCUTTREEITEM_H
#define LC_SHORTCUTTREEITEM_H

#include <QList>
#include "lc_shortcutinfo.h"

class LC_ShortcutTreeItem
{
public:
    LC_ShortcutTreeItem(LC_ShortcutTreeItem* parent, QAction* action, LC_ShortcutInfo*shortcutInfo);
    LC_ShortcutTreeItem(LC_ShortcutTreeItem* parent,QIcon ic, const QString &name, const QString &desc);

    virtual ~LC_ShortcutTreeItem();

    QIcon getIcon();
    QString getName();
    LC_ShortcutInfo* getShortcutInfo();
    LC_ShortcutTreeItem* addChild(QAction* action, LC_ShortcutInfo *shortcutInfo);
    void addChild(LC_ShortcutTreeItem* child) {childItems << child;};
    bool isModified(){ return group? false:shortcutInfo->isModified();};
    bool hasCollision(){ return group? false:shortcutInfo->hasCollision();};

    LC_ShortcutTreeItem *parent() const;
    const QList<LC_ShortcutTreeItem *> &getChildItems() const;
    LC_ShortcutTreeItem *child(int row);
    int row() const;
    int childCount() const;
    bool isMatched() const;
    void setMatched(bool val){matched = val;};
    bool isGroup() const;
    QString getShortcutViewString();
    void clearShortcut();
    void resetShortcutToDefault();
protected:
    // parent item
    LC_ShortcutTreeItem *parentItem = nullptr;
    // children of this item
    QList<LC_ShortcutTreeItem*> childItems;
    LC_ShortcutInfo* shortcutInfo;
    QIcon icon;
    QString text;
    QString description;
    bool matched {false};
    bool group {false};
};

#endif // LC_SHORTCUTTREEITEM_H
