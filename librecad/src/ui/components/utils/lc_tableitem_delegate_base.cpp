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

#include "lc_tableitem_delegate_base.h"

#include <QPainter>
#include <QTableView>

LC_TableItemDelegateBase::LC_TableItemDelegateBase(QTableView* parent):QStyledItemDelegate(parent) {
    m_tableView = parent;
}

void LC_TableItemDelegateBase::onHoverIndexChanged(const QModelIndex& index) {
    if (index.isValid()) {
        m_hoverRow = index.row();
    }
    else {
        m_hoverRow = -1;
    }
}
void LC_TableItemDelegateBase::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    paintBackground(painter, option, index);
    doPaint(painter, option, index);
}

void LC_TableItemDelegateBase::paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.isValid()) {
        const int row = index.row();
        if (row == m_hoverRow && m_hoverRow >= 0) {
            painter->fillRect(option.rect, m_hoverRowBackgroundColor);
        }
        else {
            QStyleOptionViewItem copy = option;
            copy.state &= ~QStyle::State_Selected;
            m_tableView->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &copy, painter, m_tableView);
        }
    }
}

void LC_TableItemDelegateBase::drawHorizontalGridLine(QPainter* painter, const QStyleOptionViewItem& option) const {
    const QColor color = m_gridColor;
    painter->save();
    painter->setPen(color);
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->restore();
}

void LC_TableItemDelegateBase::doPaint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyledItemDelegate::paint(painter, option, index);
}
