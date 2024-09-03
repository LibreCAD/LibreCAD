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

LC_ShortcutTreeItem::LC_ShortcutTreeItem(LC_ShortcutTreeItem *parent, QAction *action, LC_ShortcutInfo *shortcutInfo) {
    parentItem  = parent;
    text = action->text().remove("&");
    icon = action->icon();
    description = action->toolTip();
    this->shortcutInfo = shortcutInfo;
    group = false;
}

LC_ShortcutTreeItem::LC_ShortcutTreeItem(LC_ShortcutTreeItem *parent, QIcon ic, const QString &name, const QString &desc) {
    group = true;
    parentItem = parent;
    text = name;
    this->description = desc;
    this->icon = ic;
}

LC_ShortcutTreeItem::~LC_ShortcutTreeItem() {
    qDeleteAll(childItems);
}

QIcon LC_ShortcutTreeItem::getIcon() {
    return icon;
}

QString LC_ShortcutTreeItem::getName() {
    return text;
}

void LC_ShortcutTreeItem::clearShortcut(){
    shortcutInfo->clear();
}

void LC_ShortcutTreeItem::resetShortcutToDefault(){
    shortcutInfo->resetToDefault();
}

LC_ShortcutInfo* LC_ShortcutTreeItem::getShortcutInfo() {
    return shortcutInfo;
}

LC_ShortcutTreeItem *LC_ShortcutTreeItem::parent() const {
    return parentItem;
}

int LC_ShortcutTreeItem::row() const{
    if (parentItem != nullptr) {
        return parentItem->childItems.indexOf(const_cast<LC_ShortcutTreeItem *>(this));
    }
    return 0;
}

int LC_ShortcutTreeItem::childCount() const{
    return childItems.count();
}

bool LC_ShortcutTreeItem::isMatched() const {
    return matched;
}

bool LC_ShortcutTreeItem::isGroup() const {
    return group;
}

const QList<LC_ShortcutTreeItem *> &LC_ShortcutTreeItem::getChildItems() const {
    return childItems;
}

LC_ShortcutTreeItem* LC_ShortcutTreeItem::addChild(QAction *act, LC_ShortcutInfo *scInfo) {
    auto* child = new LC_ShortcutTreeItem(this, act, scInfo);
    childItems<<child;
    return child;
}

LC_ShortcutTreeItem *LC_ShortcutTreeItem::child(int row){
    return childItems.value(row);
}

QString LC_ShortcutTreeItem::getShortcutViewString() {
    if (group){
        return "";
    }
    return shortcutInfo->getKeyAsString();
}
