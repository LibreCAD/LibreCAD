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

#include "lc_shortcutstreeview.h"

#include <QPainter>
#include <QStyledItemDelegate>

#include "lc_shortcutstreemodel.h"
#include "lc_shortcuttreeitem.h"

class LC_ShortcutsTreeGridDelegate:public QStyledItemDelegate {
public:
    explicit LC_ShortcutsTreeGridDelegate(LC_ShortcutsTreeView *parent = nullptr, LC_ShortcutsTreeModel* treeModel = nullptr):QStyledItemDelegate(parent){
        if (parent != nullptr){
            this->m_treeModel = treeModel;
        }
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        QStyledItemDelegate::paint(painter, option, index);
        const int col = index.column();

        if (col > 0){
            bool draw = true;

            const LC_ShortcutTreeItem *shortcutItem = m_treeModel->getItemForIndex(index);
            if (shortcutItem != nullptr){
                if (shortcutItem->isGroup()){
                    draw = false;
                }
                else if (index.column() != LC_ShortcutsTreeModel::SHORTCUT) {
                    draw = false;
                }
            }
            if (draw){
                // const auto color = option.palette.color(QPalette::Mid);
                const auto color = option.palette.color(QPalette::Window);
                painter->save();
                painter->setPen(color);
                painter->drawRect(option.rect);
                painter->restore();
            }
        }
    }

private:
    LC_ShortcutsTreeModel* m_treeModel{nullptr};
};

LC_ShortcutsTreeView::LC_ShortcutsTreeView(QWidget *parent):QTreeView(parent) {}

void LC_ShortcutsTreeView::setup(LC_ShortcutsTreeModel *treeModel) {
    setModel(treeModel);
    auto* delegate = new LC_ShortcutsTreeGridDelegate(this, treeModel);
    setItemDelegate(delegate);
}

// todo - duplicated code from LayerTreeView. Most probably, need separate tree widget that supports expanded state

QStringList LC_ShortcutsTreeView::saveTreeExpansionState() const {
    QStringList treeExpansionState;
    const LC_ShortcutsTreeModel *treeModel = getTreeModel();
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
    const LC_ShortcutsTreeModel *layerTreeModel = getTreeModel();
    this->setUpdatesEnabled(false);
    applyExpandState(treeExpansionState, layerTreeModel->index(0, 0, QModelIndex()));
    this->setUpdatesEnabled(true);
    treeExpansionState.clear();
}

LC_ShortcutsTreeModel *LC_ShortcutsTreeView::getTreeModel() const{
    auto* result = static_cast<LC_ShortcutsTreeModel *>(model());
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

    const int childCount = index.model()->rowCount(index);
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
    QStringList &expandedItems, const QModelIndex& startIndex){
    const LC_ShortcutsTreeModel* treeModel = getTreeModel();
        for (const QString &item: expandedItems) {
            QModelIndexList matches = treeModel->match(startIndex, Qt::UserRole, item);
                for (QModelIndex index: std::as_const(matches)) {
                    this->setExpanded(index, true);
                    applyExpandState(expandedItems, treeModel->index(0, 0, index));
                }
        }
}
