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

#include "lc_mouse_tracking_table_view.h"

#include <QMouseEvent>

#include "lc_tableitem_delegate_base.h"

LC_MouseTrackingTableView::LC_MouseTrackingTableView(QWidget* parent) :QTableView(parent){
    setMouseTracking(true);
}

void LC_MouseTrackingTableView::mouseMoveEvent(QMouseEvent* event) {
    const QModelIndex index = indexAt(event->pos());
    emit hoverIndexChanged(index);
    QTableView::mouseMoveEvent(event);
}

void LC_MouseTrackingTableView::leaveEvent(QEvent* event) {
    emit hoverIndexChanged(QModelIndex());
    viewport()->repaint();
    QTableView::leaveEvent(event);
}

void LC_MouseTrackingTableView::enterEvent(QEnterEvent* event) {
    viewport()->repaint();
    QTableView::enterEvent(event);
}

void LC_MouseTrackingTableView::setTrackingItemDelegate(LC_TableItemDelegateBase* delegate) {
    setItemDelegate(delegate);
    if (delegate != nullptr) {
        connect(this, &LC_MouseTrackingTableView::hoverIndexChanged, delegate, &LC_TableItemDelegateBase::onHoverIndexChanged);
    }
    setSelectionBehavior(QAbstractItemView::SelectRows);
}
