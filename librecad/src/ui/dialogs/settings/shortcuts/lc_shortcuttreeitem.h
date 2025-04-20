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

#include "lc_shortcutinfo.h"

class LC_ShortcutTreeItem{
public:
    LC_ShortcutTreeItem(LC_ShortcutTreeItem* parent, QAction* action, LC_ShortcutInfo*shortcutInfo);
    LC_ShortcutTreeItem(LC_ShortcutTreeItem* parent,QIcon ic, const QString &name, const QString &desc);
    virtual ~LC_ShortcutTreeItem();
    QIcon getIcon();
    QString getName();
    LC_ShortcutInfo* getShortcutInfo();
    LC_ShortcutTreeItem* addChild(QAction* action, LC_ShortcutInfo *shortcutInfo);
    void addChild(LC_ShortcutTreeItem* child) {m_childItems << child;};
    bool isModified(){ return m_group? false:m_shortcutInfo->isModified();};
    bool hasCollision(){ return m_group? false:m_shortcutInfo->hasCollision();};
    LC_ShortcutTreeItem *parent() const;
    const QList<LC_ShortcutTreeItem *> &getChildItems() const;
    LC_ShortcutTreeItem *child(int row);
    int row() const;
    int childCount() const;
    bool isMatched() const;
    void setMatched(bool val){m_matched = val;};
    bool isGroup() const;
    QString getShortcutViewString();
    void clearShortcut();
    void resetShortcutToDefault();
private:
    // parent item
    LC_ShortcutTreeItem *m_parentItem = nullptr;
    // children of this item
    QList<LC_ShortcutTreeItem*> m_childItems;
    LC_ShortcutInfo* m_shortcutInfo;
    QIcon m_icon;
    QString m_text;
    QString m_description;
    bool m_matched {false};
    bool m_group {false};
};

#endif // LC_SHORTCUTTREEITEM_H
