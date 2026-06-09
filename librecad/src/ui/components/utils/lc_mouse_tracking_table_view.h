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

#ifndef LC_MOUSETRACKINGTABLEVIEW_H
#define LC_MOUSETRACKINGTABLEVIEW_H
#include <QTableView>

#include "lc_tableitem_delegate_base.h"

class LC_MouseTrackingTableView: public QTableView {
    Q_OBJECT
public:
    explicit LC_MouseTrackingTableView(QWidget* parent);
    void setTrackingItemDelegate(LC_TableItemDelegateBase* delegate);
signals:
    void hoverIndexChanged(const QModelIndex&);
protected:
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
};

#endif
