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

#include "lc_shortcuttreeitem.h"

LC_ShortcutTreeItem::LC_ShortcutTreeItem(LC_ShortcutTreeItem *parent, const QAction *action, LC_ShortcutInfo *shortcutInfo) {
    m_parentItem  = parent;
    m_text = action->text().remove("&");
    m_icon = action->icon();
    m_description = action->toolTip();
    m_shortcutInfo = shortcutInfo;
    m_group = false;
}

LC_ShortcutTreeItem::LC_ShortcutTreeItem(LC_ShortcutTreeItem *parent, const QIcon  &ic, const QString &name, const QString &desc) :
   m_parentItem{parent}, m_shortcutInfo{nullptr}, m_icon{ic}, m_text{name}, m_description{desc}, m_group{true}{
}

LC_ShortcutTreeItem::~LC_ShortcutTreeItem() {
    qDeleteAll(m_childItems);
}

QIcon LC_ShortcutTreeItem::getIcon() {
    return m_icon;
}

QString LC_ShortcutTreeItem::getName() {
    return m_text;
}

void LC_ShortcutTreeItem::clearShortcut() const {
    m_shortcutInfo->clear();
}

void LC_ShortcutTreeItem::resetShortcutToDefault() const {
    m_shortcutInfo->resetToDefault();
}

LC_ShortcutInfo* LC_ShortcutTreeItem::getShortcutInfo() const {
    return m_shortcutInfo;
}

LC_ShortcutTreeItem *LC_ShortcutTreeItem::parent() const {
    return m_parentItem;
}

int LC_ShortcutTreeItem::row() const{
    if (m_parentItem != nullptr) {
        return m_parentItem->m_childItems.indexOf(const_cast<LC_ShortcutTreeItem *>(this));
    }
    return 0;
}

int LC_ShortcutTreeItem::childCount() const{
    return m_childItems.count();
}

bool LC_ShortcutTreeItem::isMatched() const {
    return m_matched;
}

bool LC_ShortcutTreeItem::isGroup() const {
    return m_group;
}

const QList<LC_ShortcutTreeItem *> &LC_ShortcutTreeItem::getChildItems() const {
    return m_childItems;
}

LC_ShortcutTreeItem* LC_ShortcutTreeItem::addChild(const QAction *action, LC_ShortcutInfo *shortcutInfo) {
    auto* child = new LC_ShortcutTreeItem(this, action, shortcutInfo);
    m_childItems<<child;
    return child;
}

LC_ShortcutTreeItem *LC_ShortcutTreeItem::child(const int row) const {
    return m_childItems.value(row);
}

QString LC_ShortcutTreeItem::getShortcutViewString() const {
    if (m_group){
        return "";
    }
    return m_shortcutInfo->getKeyAsString();
}
