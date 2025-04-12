/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef LC_LAYERWIDGETTREEVIEW_H
#define LC_LAYERWIDGETTREEVIEW_H

#include <QStyledItemDelegate>
#include <QTreeView>

class LC_LayerTreeModel;

class LC_LayerTreeView:public QTreeView{
public:
    LC_LayerTreeView(QWidget *parent = nullptr);
    void setup(LC_LayerTreeModel *treeModel);
    QStringList saveTreeExpansionState() const;
    void restoreTreeExpansionState(QStringList treeExpansionState);
    void expandChildren(const QModelIndex &index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyExpandState(QStringList &expandedItems, const QModelIndex& startIndex);
    LC_LayerTreeModel *getTreeModel() const;
};


#endif // QG_LAYERWIDGETTREEVIEW_H
