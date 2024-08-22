/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <QStyledItemDelegate>
#include <QPainter>
#include "lc_shortcutstreeview.h"

class LC_ShortcutsTreeGridDelegate:public QStyledItemDelegate {
public:
    explicit LC_ShortcutsTreeGridDelegate(LC_ShortcutsTreeView *parent = nullptr, LC_ShortcutsTreeModel* treeModel = nullptr):QStyledItemDelegate(parent){
        if (parent){
            this->treeModel = treeModel;
        }
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        QStyledItemDelegate::paint(painter, option, index);
        int col = index.column();

        if (col > 0){
            bool draw = true;

            LC_ShortcutTreeItem *shortcutItem = treeModel->getItemForIndex(index);
            if (shortcutItem != nullptr){
                if (shortcutItem->isGroup()){
                    draw = false;
                }
                else if (index.column() != LC_ShortcutsTreeModel::SHORTCUT) {
                    draw = false;
                }
            }
            if (draw){
                /*LC_LayerTreeModelOptions* options = treeModel->getOptions();
                QColor color = options->itemsGridColor;*/
                auto color = QColor(0xd3d3d3);
                painter->save();
                painter->setPen(color);
                painter->drawRect(option.rect);
                painter->restore();
            }
        }
    }

private:
    LC_ShortcutsTreeModel* treeModel{nullptr};
};

LC_ShortcutsTreeView::LC_ShortcutsTreeView(QWidget *parent):QTreeView(parent) {}

void LC_ShortcutsTreeView::setup(LC_ShortcutsTreeModel *treeModel) {
    setModel(treeModel);
    auto* delegate = new LC_ShortcutsTreeGridDelegate(this, treeModel);
    setItemDelegate(delegate);
}

// todo - duplicated code from LayerTreeView. Most probably, need separate tree widget that supports expanded state

QStringList LC_ShortcutsTreeView::saveTreeExpansionState(){
    QStringList treeExpansionState;
    LC_ShortcutsTreeModel *treeModel = getTreeModel();
        for (QModelIndex index : treeModel->getPersistentIndexList()) {
            if (this->isExpanded(index)){
                treeExpansionState << index.data(Qt::UserRole).toString();
            }
        }

    return treeExpansionState;
}

/**
 * Restores previously saved state of expanded tree items
 * @brief LC_LayerTreeWidget::restoreTreeExpansionState
 * @param treeExpansionState
 */
void LC_ShortcutsTreeView::restoreTreeExpansionState(QStringList treeExpansionState){
    LC_ShortcutsTreeModel *layerTreeModel = getTreeModel();
    this->setUpdatesEnabled(false);
    applyExpandState(treeExpansionState, layerTreeModel->index(0, 0, QModelIndex()));
    this->setUpdatesEnabled(true);
    treeExpansionState.clear();
}

LC_ShortcutsTreeModel *LC_ShortcutsTreeView::getTreeModel() const{
    auto* result = dynamic_cast<LC_ShortcutsTreeModel *>(model());
    return result;
}

/** Utility method for recursive expansion of the tree
 * @brief LC_LayerTreeWidget::expandChildren
 * @param index
 */
void LC_ShortcutsTreeView::expandChildren(const QModelIndex &index){
    if (!index.isValid()){
        return;
    }
    expand(index);

    int childCount = index.model()->rowCount(index);
    for (int i = 0; i < childCount; i++) {
        const QModelIndex &child = model()->index(i, 0, index);
        // Recursively call the function for each child node.
        expandChildren(child);
    }
}

/**
 * Utility method for recursive expanding nodes starting from
 * given index based on the list of expanded items
 * @brief LC_LayerTreeWidget::applyExpandState
 * @param expandedItems
 * @param startIndex
 */
void LC_ShortcutsTreeView::applyExpandState(
    QStringList &expandedItems, QModelIndex startIndex){
    LC_ShortcutsTreeModel* treeModel = getTreeModel();
        for (QString item: expandedItems) {
            QModelIndexList matches = treeModel->match(startIndex, Qt::UserRole, item);
                for (QModelIndex index: matches) {
                    this->setExpanded(index, true);
                    applyExpandState(expandedItems, treeModel->index(0, 0, index));
                }
        }
}
