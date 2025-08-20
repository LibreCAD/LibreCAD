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

#include <QFont>
#include "lc_dimstyleslistmodel.h"
#include "lc_dimstyle.h"
#include "lc_dimstyleitem.h"

LC_StylesListModel::LC_StylesListModel(QObject* parent, const QList<LC_DimStyleItem*>& items, bool showUsageCount)
    : QAbstractListModel{parent},
      m_items{items} {
    m_showUsagesCount = showUsageCount;
}

LC_StylesListModel::LC_StylesListModel(QObject* parent, RS_Graphic* g, RS2::EntityType dimensionType):QAbstractListModel{parent},
    m_showUsagesCount{false}{
    createModel(g, dimensionType);
}

int LC_StylesListModel::rowCount([[maybe_unused]]const QModelIndex& parent) const { return m_items.size(); }

QVariant LC_StylesListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    int row = index.row();
    LC_DimStyleItem* item = m_items[row];
    switch (role) {
        case Qt::DisplayRole: {
            if (m_showUsagesCount) {
                return item->displayName() + " (" + QString::number(item->usageCount()) + ")";
            }
            else {
                return item->displayName();
            }
        }
        case Qt::FontRole: {
            if (item->isActive()) {
                QFont font;
                font.setBold(true);
                font.setItalic(true);
                return font;
            }
            return QVariant();
        }
        default:
            return QVariant();
    }
}

void LC_StylesListModel::addItem(LC_DimStyleItem* item) {
    m_items << item;
    emitDataChanged();
}

LC_DimStyleItem* LC_StylesListModel::getActiveStyleItem() {
    size_t size = m_items.size();
    for (size_t i = 0; i < size; ++i) {
        LC_DimStyleItem *item = m_items[i];
        if (item->isActive()) {
            return item;
        };
    }
    return nullptr;
}

LC_DimStyleItem* LC_StylesListModel::getItemForIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return nullptr;
    }
    return m_items[index.row()];
}

LC_DimStyleItem* LC_StylesListModel::getItemAtRow(int row) const {
    return m_items[row];
}

LC_DimStyleItem* LC_StylesListModel::getStandardItem() const {
    return m_items[0];
}

void LC_StylesListModel::setActiveStyleItem(const QModelIndex& index) {
   int row = index.row();
   int size = m_items.size();
    for (int i = 0; i < size; ++i) {
        LC_DimStyleItem *item = m_items[i];
        item->setActive(i == row);
    }
    emitDataChanged();
}

int LC_StylesListModel::getActiveStyleItemIndex() {
    size_t size = m_items.size();
    for (size_t i = 0; i < size; ++i) {
        LC_DimStyleItem *item = m_items[i];
        if (item->isActive()) {
            return i;
        };
    }
    return 0;
}

void LC_StylesListModel::emitDataChanged() {
    dataChanged(QModelIndex(), QModelIndex());
}

LC_DimStyleItem* LC_StylesListModel::findByName(const QString& name) {
    size_t size = m_items.size();
    for (size_t i = 0; i < size; ++i) {
        LC_DimStyleItem *item = m_items[i];
        auto itemName = item->dimStyle()->getName();
        if (itemName == name) {
            return item;
        };
    }
    return nullptr;
}

void LC_StylesListModel::removeItem(LC_DimStyleItem* item) {
    if (item != nullptr) {
        m_items.removeOne(item);

        if (item->isActive()) {
            m_items[0]->setActive(true);
        }
        delete item;
        emitDataChanged();
    }
}

void LC_StylesListModel::mergeWith(QList<LC_DimStyle*>& list) {
    for (const auto i: list) {
        auto incomingDimStyle = i;
        QString itemName = incomingDimStyle->getName();
        auto existingItem = findByName(itemName);
        if (existingItem != nullptr) { // style with such name already exists...
            auto existingDimStyle = existingItem->dimStyle();
            // todo - sand - potentially, some policy may be used there and that policy will define how to
            // handle duplicates of styles. For example, it's possible to make a copy.
            // However, for now we'll simply update existing style be values from incoming style.
            incomingDimStyle->copyTo(existingDimStyle);

            delete incomingDimStyle;
        }
        else {
            auto varsItem = new LC_DimStyleItem(incomingDimStyle, 0, false);
            m_items << varsItem;
        }
    }

    list.clear();
    emitDataChanged();
}

LC_StylesListModel* LC_StylesListModel::getFlatItemsListModel() {
    return this; // fixme - quick tmp test approach
}

int LC_StylesListModel::getItemIndex(LC_DimStyleItem* itemToFind) {
    size_t size = m_items.size();
    for (size_t i = 0; i < size; ++i) {
        LC_DimStyleItem *item = m_items[i];
        if (item == itemToFind) {
            return i;
        };
    }
    return 0;
}

void LC_StylesListModel::collectItemsForBaseStyleName(const QString &baseName, QList<LC_DimStyleItem*>* list) {
    for (const auto dsi: m_items) {
        if (baseName == dsi->baseName()) {
            list->push_back(dsi);
        }
    }
}

void LC_StylesListModel::collectItemsForStyle(LC_DimStyle* dimStyle, QList<LC_DimStyleItem*>* list) {
    QString baseName;
    RS2::EntityType styleFor;
    LC_DimStyle::parseStyleName(dimStyle->getName(), baseName, styleFor);

    collectItemsForBaseStyleName(baseName, list);
}

// todo - think whether some sorting is needed for dimension styles. Actually, its hardly possible that amount of
// style will be significant, yet still....
void LC_StylesListModel::sort([[maybe_unused]]int column,[[maybe_unused]] Qt::SortOrder order) {
    /*emit layoutAboutToBeChanged({QModelIndex()}, QAbstractItemModel::VerticalSortHint);
    std::sort(items.begin(), items.end(), [column,order](const LC_DimStyle* a, const LC_DimStyle* b)-> bool
    {
        return a->getName() > b->getName();
        //const auto aV = a.at(column);
        const auto bV = b.at(column);
        if (order == Qt::AscendingOrder)
            return aV == bV ? a.at(2) < b.at(2) : aV < bV;
        return aV == bV ? a.at(2) > b.at(2) : aV > bV;#1#
    });
    emit layoutChanged({QModelIndex()}, QAbstractItemModel::VerticalSortHint);*/
}

void LC_StylesListModel::cleanup() {
    qDeleteAll(m_items);
}

void LC_StylesListModel::createModel(RS_Graphic* g, [[maybe_unused]]RS2::EntityType dimensionType) {
    QString defaultDimStyleName = g->getDefaultDimStyleName();
    LC_DimStyle* styleThatIsDefault = g->getDimStyleByName(defaultDimStyleName);

    auto dimStylesList = g->getDimStyleList();
    auto dimStyles = dimStylesList->getStylesList();

    for (const auto dimStyle : *dimStyles) {
        QString styleName = dimStyle->getName();
        QString baseName;
        RS2::EntityType entityType = RS2::EntityUnknown;

        LC_DimStyle::parseStyleName(styleName, baseName, entityType);
        // skip artificial style from vars and skip type-specific styles
        if (!dimStyle->isFromVars() && entityType == RS2::EntityUnknown) {
            LC_DimStyle* ds = dimStyle->getCopy();
            bool defaultStyle = styleThatIsDefault == dimStyle;
            auto item = new LC_DimStyleItem(ds, 0, defaultStyle);
            m_items << item;
        }
    }
}
