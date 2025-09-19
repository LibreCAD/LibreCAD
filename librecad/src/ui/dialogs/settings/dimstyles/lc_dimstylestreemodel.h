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

#ifndef LC_DIMSTYLETREEMODEL_H
#define LC_DIMSTYLETREEMODEL_H
#include <QAbstractItemModel>

#include "lc_dimstyle.h"

class LC_DimStyleItem;

class LC_DimStyleTreeModel: public QAbstractItemModel{
public:
    LC_DimStyleTreeModel(QObject* parent, const QList<LC_DimStyleItem*>& items, bool highlightCurrentItem);
    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    LC_DimStyleItem* getItemForIndex(const QModelIndex& index) const;
    void collectUnsavedItems(QList<LC_DimStyleItem*>& items) const;
    void setUsageCount(bool val) {m_showUsageCount = val;}
    LC_DimStyleItem* getActiveStyleItem();
    LC_DimStyleItem* getEntityStyleItem();
    LC_DimStyleItem* getStandardItem();
    LC_DimStyleItem* findByName(const QString& chars);
    void addItem(LC_DimStyleItem* item);
    void collectItemsForBaseStyleItem(QList<LC_DimStyleItem*>* list, LC_DimStyleItem* baseItem);
    void collectAllItemsForStyle(LC_DimStyle* lc_dim_style, QList<LC_DimStyleItem*>* list);
    void collectItemsForBaseStyleName(const QString& chars, QList<LC_DimStyleItem*>* list);
    void emitDataChanged();
    void removeItem(LC_DimStyleItem* item);
    void setActiveStyleItem(const QModelIndex& model_index);
    void setEntityItem(LC_DimStyleItem* item);
    void setEntityItem(const QModelIndex& model_index);
    void cleanup(bool deleteDimStyles);
    void collectAllStyleItems(QList<LC_DimStyleItem*>& items);
    void mergeWith(const QList<LC_DimStyle*>& list);
    int itemsCount();
    LC_DimStyleItem*  rootItem() {return m_rootItem.get();}
private:
    void addToParent(LC_DimStyleItem* item);
    void buildTree(const QList<LC_DimStyleItem*>& list);
    std::unique_ptr<LC_DimStyleItem> m_rootItem;
    bool m_showUsageCount {true};
    int m_itemsCount{0};
    bool m_highlightCurrentItem {false};
};

#endif // LC_DIMSTYLETREEMODEL_H
