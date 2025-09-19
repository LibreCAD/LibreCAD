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

#ifndef LC_DIMSTYLESLISTMODEL_H
#define LC_DIMSTYLESLISTMODEL_H
#include <qabstractitemmodel.h>

namespace RS2
{
    enum EntityType : unsigned;
}

class RS_Graphic;
class LC_DimStyleItem;
class LC_DimStyle;

class LC_StylesListModel: public QAbstractListModel {
public:
    LC_StylesListModel(QObject* parent, const QList<LC_DimStyleItem*>& items, bool showUsageCount = true);
    LC_StylesListModel(QObject* parent, RS_Graphic* g, RS2::EntityType dimensionType);
    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    void addItem(LC_DimStyleItem* item);
    LC_DimStyleItem* getActiveStyleItem();
    LC_DimStyleItem* getItemForIndex(const QModelIndex& index) const;
    LC_DimStyleItem* getItemAtRow(int row) const;
    LC_DimStyleItem* getStandardItem() const;
    void setActiveStyleItem(const QModelIndex& index);
    int getActiveStyleItemIndex();
    void emitDataChanged();
    LC_DimStyleItem* findByName(const QString& name);
    void removeItem(LC_DimStyleItem* item);
    QList<LC_DimStyleItem*>* getStyleItems() {
        return &m_items;
    }
    void mergeWith(QList<LC_DimStyle*>& list);
    LC_StylesListModel* getFlatItemsListModel();
    int getItemIndex(LC_DimStyleItem* initial_style);
    void collectItemsForBaseStyleName(const QString& baseName, QList<LC_DimStyleItem*>* list);
    void collectItemsForStyle(LC_DimStyle* dim_style, QList<LC_DimStyleItem*>* list);
    void sort(int column, Qt::SortOrder order) override;
    void cleanup();
private:
    QList<LC_DimStyleItem*> m_items;
    bool m_showUsagesCount{true};
    void createModel(RS_Graphic* g,RS2::EntityType dimensionType);
};

#endif // LC_DIMSTYLESLISTMODEL_H
