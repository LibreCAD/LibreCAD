/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_TABLEITEMDELEGATEBASE_H
#define LC_TABLEITEMDELEGATEBASE_H
#include <QStyledItemDelegate>

class QTableView;

class LC_TableItemDelegateBase : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit LC_TableItemDelegateBase(QTableView* parent);
public slots:
    void onHoverIndexChanged(const QModelIndex& index);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
protected:
    int m_hoverRow = -1;
    QColor m_hoverRowBackgroundColor = Qt::red;
    QColor m_gridColor;
    QTableView* m_tableView {nullptr};
    void paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void drawHorizontalGridLine(QPainter* painter, const QStyleOptionViewItem& option) const;
    virtual void doPaint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif
