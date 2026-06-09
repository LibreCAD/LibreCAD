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

#include "lc_layertreeview.h"

#include <QDragEnterEvent>
#include <QPainter>
#include <QStyledItemDelegate>

#include "lc_layertreeitem.h"
#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"
#include "lc_layertreewidget.h"
#include "rs_debug.h"

/**
 * Delegate for painting grid lines within tree view - rect around cells for Name column.
 * @brief The GridDelegate class
 */
class LayerTreeGridDelegate:public QStyledItemDelegate {
public:
    explicit LayerTreeGridDelegate(LC_LayerTreeView *parent = nullptr, LC_LayerTreeModel* tm = nullptr):QStyledItemDelegate(parent){
        if (parent != nullptr) {
            m_treeView = parent;
            m_treeModel = tm;
            m_options = tm->getOptions();
            const auto palette = parent->palette();
            m_gridColor = palette.color(QPalette::Button);
            m_customHoverTextColor = palette.color(QPalette::WindowText);
            m_hoverBackgrounColor = palette.color(QPalette::Button);
        }
    }

    enum TextColorMode {
        AutoContrast,    // Auto-calculate for readability
        CustomColor,     // Use specified color
        UsePalette       // Use from widget palette
    };
    QColor getTextColor(TextColorMode textColorMode, QColor hoverBackground, const QStyleOptionViewItem &opt) const{
        QColor textColor;
        switch (textColorMode) {
            case AutoContrast:
                textColor = calculateTextColor(hoverBackground);
                break;
            case CustomColor:
                textColor = m_customHoverTextColor;
                break;
            case UsePalette:
            default:
                // Use the palette's text color (default behavior)
                textColor = opt.palette.color(QPalette::Text);
                break;
        }
        return textColor;
    }

    QColor calculateTextColor(const QColor &background) const
    {
        // Calculate perceived brightness
        const double brightness = (0.299 * background.red() +
                            0.587 * background.green() +
                            0.114 * background.blue()) / 255.0;

        return (brightness > 0.5) ? Qt::black : Qt::white;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        const int col = index.column();
        const int actualCol = m_treeModel->translateColumn(col);
        // NOTE - Here we setup the color for howevered item highligting. Under Win10 - it works for all styles (Fusion, Windows, windows11)
        // EXCEPT! windowsvista - there it seems native highligh color is used that IGNORES such setup :(
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        bool isHovered = (opt.state & QStyle::State_MouseOver) &&
                      (opt.state & QStyle::State_Enabled);
        bool isSelected = opt.state & QStyle::State_Selected;

        if (isHovered && !isSelected) {
            opt.backgroundBrush = m_hoverBackgrounColor;

            QColor textColor = getTextColor(AutoContrast, m_hoverBackgrounColor, opt);

            QPalette palette = opt.palette;
            palette.setColor(QPalette::Text, textColor);
            palette.setColor(QPalette::WindowText, textColor);
            palette.setColor(QPalette::HighlightedText, textColor);
            palette.setColor(QPalette::Highlight, m_hoverBackgrounColor);
            palette.setColor(QPalette::AlternateBase, m_hoverBackgrounColor);

            opt.palette = palette;
            opt.state |= QStyle::State_Selected; // Enable highlight text rendering
            opt.backgroundBrush = QBrush(m_hoverBackgrounColor);
        }
        QStyledItemDelegate::paint(painter, opt, index);
        if (col >= 0){
            const LC_LayerTreeItem *layerItem = m_treeModel->getItemForIndex(index);
            const bool nonVirtualItem = layerItem != nullptr && !layerItem->isVirtual();
            if (actualCol == LC_LayerTreeModel::COLUMN_COLOR_SAMPLE && nonVirtualItem) {
                m_treeView->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, painter, m_treeView);
                QRect colorRect = option.rect;
                const int originalWidth = colorRect.width();
                const int newWidth = colorRect.height() - 8;
                // center color box in cell
                const int widthDelta = originalWidth - newWidth;
                const int leftDelta = widthDelta / 2;
                const int rightDelta = widthDelta - leftDelta;

                colorRect.adjust(leftDelta, 4, -rightDelta, -4);

                painter->fillRect(colorRect, Qt::black);
                colorRect.adjust(1, 1, -1, -1);
                const auto color = layerItem->getLayer()->getPen().getColor();
                painter->fillRect(colorRect, color);
            }
            bool drawGrid = m_options->showGrid;
            if (drawGrid) {
                if (actualCol == LC_LayerTreeModel::COLUMN_NAME){
                    if (nonVirtualItem){
                        drawGrid =  m_treeModel->isFlatMode();
                    }
                }
                if (drawGrid){
                    const QColor color = m_gridColor;
                    painter->save();
                    painter->setPen(color);
                    painter->drawRect(option.rect);
                    painter->restore();
                }
            }
        }
    }

private:
    LC_LayerTreeView* m_treeView;
    LC_LayerTreeModel* m_treeModel{nullptr};
    QColor m_gridColor;
    QColor m_hoverBackgrounColor;
    QColor m_customHoverTextColor;
    LC_LayerTreeModelOptions* m_options;
};


LC_LayerTreeView::LC_LayerTreeView(QWidget *parent):QTreeView(parent){
}

void LC_LayerTreeView::setup(LC_LayerTreeModel *treeModel){
    setModel(treeModel);
    setMouseTracking(true);
    auto* delegate = new LayerTreeGridDelegate(this, treeModel);
    setItemDelegate(delegate);
}

void LC_LayerTreeView::dragLeaveEvent(QDragLeaveEvent *event) {
     RS_DEBUG->print(RS_Debug::D_WARNING, "dragLeaveEvent");
     event->accept();
     constexpr auto dropIndex = QModelIndex();
     const auto* widget = static_cast<LC_LayerTreeWidget*>(parentWidget());
     widget->onDropEvent(dropIndex , LC_LayerTreeWidget::InvalidDrop);
}

void LC_LayerTreeView::dragEnterEvent(QDragEnterEvent *event) {
     RS_DEBUG->print(RS_Debug::D_WARNING, "dragEnterEvent");

    // we disable drag&drop here rather than on model and flags() level
    const LC_LayerTreeModel *layerTreeModel = getTreeModel();
    const bool dragDropEnabled = layerTreeModel->getOptions()->dragDropEnabled;
    if (!dragDropEnabled){
        return;
    }

    const QModelIndex dropIndex = indexAt(event->position().toPoint());
    const auto* layerTree = static_cast<LC_LayerTreeWidget*>(parentWidget());
    if( !dropIndex.isValid()) {
        return;
    }
    layerTree->onDragEnterEvent(dropIndex);

    event->setDropAction( Qt::MoveAction );
    event->accept();
}

void LC_LayerTreeView::dropEvent(QDropEvent* event) {
    RS_DEBUG->print(RS_Debug::D_WARNING, "dropEvent");
    if (event == nullptr) {
        return;
    }
    event->accept();

    const QModelIndex dropIndex = indexAt(event->position().toPoint());
    const auto* widget = dynamic_cast<LC_LayerTreeWidget*>(parentWidget());
    if (widget == nullptr) {
        return;
    }

    const int dropIndicator = dropIndicatorPosition();

    if (!dropIndex.isValid()) {
        // drop to empty space on viewpoint? Need to verify this case more
        switch (dropIndicator) {
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
    if (!dropIndex.parent().isValid() && dropIndex.row() != -1) {
        switch (dropIndicator) {
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

QStringList LC_LayerTreeView::saveTreeExpansionState() const {
    QStringList treeExpansionState;
    const LC_LayerTreeModel *layerTreeModel = getTreeModel();
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
    const LC_LayerTreeModel *layerTreeModel = getTreeModel();
    this->setUpdatesEnabled(false);
    applyExpandState(treeExpansionState, layerTreeModel->index(0, 0, QModelIndex()));
    this->setUpdatesEnabled(true);
    treeExpansionState.clear();
}

LC_LayerTreeModel *LC_LayerTreeView::getTreeModel() const{
    auto* layerTreeModel = static_cast<LC_LayerTreeModel *>(model());
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
void LC_LayerTreeView::applyExpandState(QStringList &expandedItems, const QModelIndex& startIndex){
    const LC_LayerTreeModel* layerTreeModel = getTreeModel();
        foreach (QString item, expandedItems) {
            QModelIndexList matches = layerTreeModel->match(startIndex, Qt::UserRole, item);
                foreach (QModelIndex index, matches) {
                    this->setExpanded(index, true);
                    applyExpandState(expandedItems, layerTreeModel->index(0, 0, index));
                }
        }
}
