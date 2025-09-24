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

#include "lc_dimstylestreemodel.h"

#include <QFont>

#include "lc_dimstyleitem.h"

LC_DimStyleTreeModel::LC_DimStyleTreeModel(QObject* parent, const QList<LC_DimStyleItem*>& items,
                                           bool highlightCurrent) : QAbstractItemModel(parent),
                                                                    m_highlightCurrentItem{highlightCurrent} {
    m_rootItem = std::make_unique<LC_DimStyleItem>();
    buildTree(items);
}

QVariant LC_DimStyleTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    LC_DimStyleItem* item = getItemForIndex(index);
    if (item == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            if (item->isOverrideItem()) {
                return QObject::tr("[Override]");
            }
            else {
                int usageCount = item->usageCount();
                auto displayName = item->displayName();
                if (item->isActive()) {
                    // displayName = "--> " + displayName;
                    displayName = "[A] " + displayName;
                }
                if (usageCount > 0) {
                    return displayName + " (" + QString::number(item->usageCount()) + ")";
                }
                return displayName;
            }
        }
        case Qt::FontRole: {
            if (item->isEntityStyleItem()) {
                QFont font;
                font.setBold(true);
                if (item->isActive()) {
                    font.setItalic(true);
                }
                return font;
            }
            else if (item->isActive()) {
                QFont font;
                if (m_highlightCurrentItem) {
                    font.setBold(true);
                }
                font.setItalic(true);
                return font;
            }
            break;
        }
        default:
            break;
    }
    return QVariant();
}

QModelIndex LC_DimStyleTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    LC_DimStyleItem* parentItem = parent.isValid() ? getItemForIndex(parent) : m_rootItem.get();
    LC_DimStyleItem* childItem = parentItem->child(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex LC_DimStyleTreeModel::parent(const QModelIndex& index) const {
    if (index.isValid()) {
        LC_DimStyleItem* childItem = getItemForIndex(index);
        if (childItem != nullptr) {
            LC_DimStyleItem* parentItem = childItem->parentItem();
            if (parentItem != m_rootItem.get()) {
                return createIndex(parentItem->row(), 0, parentItem);
            }
        }
    }
    return QModelIndex();
}

int LC_DimStyleTreeModel::rowCount(const QModelIndex& parent) const {
    LC_DimStyleItem* parentItem;
    if (parent.isValid()) {
        parentItem = getItemForIndex(parent);
    }
    else {
        parentItem = m_rootItem.get();
    }

    if (parentItem != nullptr) {
        int result = parentItem->childCount();
        return result;
    }
    return 0;
}

int LC_DimStyleTreeModel::columnCount([[maybe_unused]]const QModelIndex& parent) const {
    return 1;
}

LC_DimStyleItem* LC_DimStyleTreeModel::getItemForIndex(const QModelIndex& index) const {
    LC_DimStyleItem* result = nullptr;
    if (index.isValid()) {
        result = static_cast<LC_DimStyleItem*>(index.internalPointer());
    }
    return result;
}

void LC_DimStyleTreeModel::collectUnsavedItems(QList<LC_DimStyleItem*>& items) const {
    int childCount = m_rootItem->childCount();
    // created styles will be under the root. If they are on further level - it means that it's overrides...
    for (int i = 0; i < childCount; i++) {
        auto child = m_rootItem->child(i);
        if (child->isUnsaved()) {
            items << child;
        }
    }
}

LC_DimStyleItem* LC_DimStyleTreeModel::getActiveStyleItem() {
    return m_rootItem->findActive();
}

LC_DimStyleItem* LC_DimStyleTreeModel::getEntityStyleItem() {
    return m_rootItem->findEntityStyleItem();
}

LC_DimStyleItem* LC_DimStyleTreeModel::getStandardItem() {
    return m_rootItem->child(0);
}

LC_DimStyleItem* LC_DimStyleTreeModel::findByName(const QString& name) {
    return m_rootItem->findByName(name);
}

void LC_DimStyleTreeModel::collectItemsForBaseStyleItem(QList<LC_DimStyleItem*>* list, LC_DimStyleItem* baseItem) {
    if (baseItem != nullptr) {
        list->push_back(baseItem);
        int childCount = baseItem->childCount();
        for (int i = 0; i < childCount; i++) {
            LC_DimStyleItem* childItem = baseItem->child(i);
            list->push_back(childItem);
        }
    }
}

void LC_DimStyleTreeModel::collectAllItemsForStyle(LC_DimStyle* styleItem, QList<LC_DimStyleItem*>* list) {
    QString baseName;
    RS2::EntityType type;
    LC_DimStyle::parseStyleName(styleItem->getName(), baseName, type);
    LC_DimStyleItem* baseItem = m_rootItem->findBaseStyleItem(baseName);
    collectItemsForBaseStyleItem(list, baseItem);
}

void LC_DimStyleTreeModel::collectItemsForBaseStyleName(const QString& baseStyleName, QList<LC_DimStyleItem*>* list) {
    LC_DimStyleItem* baseItem = m_rootItem->findBaseStyleItem(baseStyleName);
    collectItemsForBaseStyleItem(list, baseItem);
}

void LC_DimStyleTreeModel::emitDataChanged() {
    beginResetModel();
    endResetModel();
    dataChanged(QModelIndex(), QModelIndex());
}

void LC_DimStyleTreeModel::removeItem(LC_DimStyleItem* item) {
    item->parentItem()->removeChild(item);
    delete item;
    m_itemsCount--;
    emitDataChanged();
}

void LC_DimStyleTreeModel::setActiveStyleItem(const QModelIndex& index) {
    LC_DimStyleItem* item = getItemForIndex(index);
    if (item != nullptr) {
        LC_DimStyleItem* currentlyActiveItem = m_rootItem->findActive();
        if (currentlyActiveItem != nullptr) {
            currentlyActiveItem->setActive(false);
        }
        item->setActive(true);
    }
    emitDataChanged();
}

void LC_DimStyleTreeModel::setEntityItem(LC_DimStyleItem* item) {
    if (item != nullptr) {
        LC_DimStyleItem* currentlyActiveItem = m_rootItem->findEntityStyleItem();
        if (currentlyActiveItem != nullptr) {
            currentlyActiveItem->setEntityStyleItem(false);
        }
        item->setEntityStyleItem(true);
    }
    emitDataChanged();
}

void LC_DimStyleTreeModel::setEntityItem(const QModelIndex& index) {
    LC_DimStyleItem* item = getItemForIndex(index);
    setEntityItem(item);
}

void LC_DimStyleTreeModel::cleanup(bool deleteDimStyles) {
    m_rootItem->cleanup(deleteDimStyles);
    m_itemsCount = 0;
}

void LC_DimStyleTreeModel::collectAllStyleItems(QList<LC_DimStyleItem*>& items) {
    m_rootItem->collectChildren(items);
}

void LC_DimStyleTreeModel::mergeWith(const QList<LC_DimStyle*>& list) {
    for (const auto i : list) {
        auto incomingDimStyle = i;
        QString itemName = incomingDimStyle->getName();
        auto existingItem = findByName(itemName);
        if (existingItem != nullptr) {
            // style with such name already exists...
            auto existingDimStyle = existingItem->dimStyle();
            // todo - sand - potentially, some policy may be used there and that policy will define how to
            // handle duplicates of styles. For example, it's possible to make a copy.
            // However, for now we'll simply update existing style be values from incoming style.
            incomingDimStyle->copyTo(existingDimStyle);

            delete incomingDimStyle;
        }
        else {
            auto varsItem = new LC_DimStyleItem(incomingDimStyle, 0, false);
            addToParent(varsItem);
        }
    }
    emitDataChanged();
}

int LC_DimStyleTreeModel::itemsCount() {
    return m_itemsCount;
}

void LC_DimStyleTreeModel::addItem(LC_DimStyleItem* item) {
    if (item->isBaseStyle()) {
        m_rootItem->appendChild(item);
        m_itemsCount++;
    }
    else {
        addToParent(item);
    }
    emitDataChanged();
}

void LC_DimStyleTreeModel::addToParent(LC_DimStyleItem* item) {
    QString baseName = item->baseName();
    LC_DimStyleItem* baseItem = m_rootItem->findBaseStyleItem(baseName);
    if (baseItem != nullptr) {
        baseItem->appendChild(item);
    }
    else {
        m_rootItem->appendChild(item);
    }
    m_itemsCount++;
}

void LC_DimStyleTreeModel::buildTree(const QList<LC_DimStyleItem*>& list) {
    // quick and dirty implementation that assumes that there are 2 levels of styles.
    // first one - basic styles
    // add basic styles to the root item and remove them from original list
    QList<LC_DimStyleItem*> listCopy = list;
    QMutableListIterator<LC_DimStyleItem*> i(listCopy);
    while (i.hasNext()) {
        LC_DimStyleItem* item = i.next();
        if (item->isBaseStyle()) {
            m_rootItem->appendChild(item);
            m_itemsCount++;
            i.remove();
        }
    }

    // second layer - items for specific type of dimension
    QMutableListIterator<LC_DimStyleItem*> s(listCopy);
    while (s.hasNext()) {
        LC_DimStyleItem* item = s.next();
        addToParent(item);
    }
}
