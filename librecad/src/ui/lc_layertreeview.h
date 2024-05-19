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

#include <QPainter>
#include <QTreeView>
#include <qstyleditemdelegate.h>

#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"


class LC_LayerTreeView:public QTreeView{
public:
    LC_LayerTreeView(QWidget *parent = nullptr);
    void setup(LC_LayerTreeModel *treeModel);
    QStringList saveTreeExpansionState();
    void restoreTreeExpansionState(QStringList treeExpansionState);
    void expandChildren(const QModelIndex &index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyExpandState(QStringList &expandedItems, QModelIndex startIndex);
    LC_LayerTreeModel *getTreeModel() const;
};

/**
 * Delegate for painting grid lines within tree view - rect around cells for Name column.
 * @brief The GridDelegate class
 */
class LayerTreeGridDelegate:public QStyledItemDelegate {
public:
    explicit LayerTreeGridDelegate(LC_LayerTreeView *parent = nullptr, LC_LayerTreeModel* treeModel = nullptr):QStyledItemDelegate(parent){
        if (parent){
            this->treeModel = treeModel;
        }
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        QStyledItemDelegate::paint(painter, option, index);
        int col = index.column();

        if (col > 0){
            bool draw = true;
            if (col == LC_LayerTreeModel::NAME){
                LC_LayerTreeItem *layerItem = treeModel->getItemForIndex(index);
                if (layerItem && (!layerItem->isVirtual())){
                    draw = false;
                }
            }
            if (draw){
                LC_LayerTreeModelOptions* options = treeModel->getOptions();
                QColor color = options->itemsGridColor;
                painter->save();
                painter->setPen(color);
                painter->drawRect(option.rect);
                painter->restore();
            }
        }
    }

private:
    LC_LayerTreeModel* treeModel{nullptr};
};

#endif // QG_LAYERWIDGETTREEVIEW_H
