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

#include <QDragEnterEvent>

#include "lc_layertreewidget.h"
#include "lc_layertreeview.h"
#include "rs_debug.h"

LC_LayerTreeView::LC_LayerTreeView(QWidget *parent):QTreeView(parent){
}

void LC_LayerTreeView::setup(LC_LayerTreeModel *treeModel){
    setModel(treeModel);
    auto* delegate = new LayerTreeGridDelegate(this, treeModel);
    setItemDelegate(delegate);
}

void LC_LayerTreeView::dragLeaveEvent(QDragLeaveEvent *event) {
     RS_DEBUG->print(RS_Debug::D_WARNING, "dragLeaveEvent");
     event->accept();
     QModelIndex dropIndex = QModelIndex();
     auto* widget = (LC_LayerTreeWidget*)parentWidget();
     widget->onDropEvent(dropIndex , LC_LayerTreeWidget::InvalidDrop);

}

void LC_LayerTreeView::dragEnterEvent(QDragEnterEvent *event) {

     RS_DEBUG->print(RS_Debug::D_WARNING, "dragEnterEvent");

    // we disable drag&drop here rather than on model and flags() level
    LC_LayerTreeModel *layerTreeModel = getTreeModel();
    bool dragDropEnabled = layerTreeModel->getOptions()->dragDropEnabled;
    if (!dragDropEnabled){
        return;
    }

    QModelIndex dropIndex = indexAt(event->position().toPoint());
    auto* widget = dynamic_cast<LC_LayerTreeWidget*>(parentWidget());
    if( !dropIndex.isValid() )
        return;
    widget->onDragEnterEvent(dropIndex);

    event->setDropAction( Qt::MoveAction );
    event->accept();
}

    void LC_LayerTreeView::dropEvent(QDropEvent *event){
        RS_DEBUG->print(RS_Debug::D_WARNING, "dropEvent");
        if (!event){
            return;
        }
        event->accept();

        QModelIndex dropIndex = indexAt(event->position().toPoint());
        auto* widget = dynamic_cast<LC_LayerTreeWidget*>(parentWidget());
        if (!widget){
            return;
        }

        int dropIndicator = dropIndicatorPosition();

        if( !dropIndex.isValid() ){
            // drop to empty space on viewpoint? Need to verify this case more
            switch (dropIndicator){
               case QAbstractItemView::OnViewport:
                widget->onDropEvent(dropIndex, LC_LayerTreeWidget::OnViewport);
                break;
            default:
                widget->onDropEvent(dropIndex, LC_LayerTreeWidget::InvalidDrop);
            }
            return;
        }

        //bool bAbove = false; // boolean for the case when you are above an item

        LC_LayerTreeWidget::DropIndicatorPosition position{};
        if (!dropIndex.parent().isValid() && dropIndex.row() != -1)
        {
            switch (dropIndicator)
            {
            case QAbstractItemView::AboveItem:
                // manage a boolean for the case when you are above an item
                //bAbove = true;
                position = LC_LayerTreeWidget::AboveItem;
                break;
            case QAbstractItemView::BelowItem:
                position = LC_LayerTreeWidget::BelowItem;
                // something when being below an item
                break;
            case QAbstractItemView::OnItem:
                position = LC_LayerTreeWidget::OnItem;
                // you're on an item, maybe add the current one as a child
                break;
            case QAbstractItemView::OnViewport:
                // you are not on your tree
                position = LC_LayerTreeWidget::OnViewport;
                break;
            default:
                break;
            }
        }
        widget->onDropEvent(dropIndex, position);
    }

QStringList LC_LayerTreeView::saveTreeExpansionState(){
    QStringList treeExpansionState;
    LC_LayerTreeModel *layerTreeModel = getTreeModel();
    foreach (QModelIndex index, layerTreeModel->getPersistentIndexList()) {
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
void LC_LayerTreeView::restoreTreeExpansionState(QStringList treeExpansionState){
    LC_LayerTreeModel *layerTreeModel = getTreeModel();
    this->setUpdatesEnabled(false);
    applyExpandState(treeExpansionState, layerTreeModel->index(0, 0, QModelIndex()));
    this->setUpdatesEnabled(true);
    treeExpansionState.clear();
}

LC_LayerTreeModel *LC_LayerTreeView::getTreeModel() const{
    auto* layerTreeModel = dynamic_cast<LC_LayerTreeModel *>(model());
    return layerTreeModel;
}

/** Utility method for recursive expansion of the tree
 * @brief LC_LayerTreeWidget::expandChildren
 * @param index
 */
void LC_LayerTreeView::expandChildren(const QModelIndex &index){
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
void LC_LayerTreeView::applyExpandState(
    QStringList &expandedItems, QModelIndex startIndex){
    LC_LayerTreeModel* layerTreeModel = getTreeModel();
        foreach (QString item, expandedItems) {
            QModelIndexList matches = layerTreeModel->match(startIndex, Qt::UserRole, item);
                foreach (QModelIndex index, matches) {
                    this->setExpanded(index, true);
                    applyExpandState(expandedItems, layerTreeModel->index(0, 0, index));
                }
        }
}
