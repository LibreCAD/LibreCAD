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

#include "lc_namedviewslistwidget.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>

#include "lc_dlgnamedviewslistoptions.h"
#include "lc_graphicviewport.h"
#include "lc_namedviewsbutton.h"
#include "lc_namedviewslistoptions.h"
#include "lc_namedviewsmodel.h"
#include "lc_uiutils.h"
#include "lc_viewslist.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "ui_lc_namedviewslistwidget.h"

LC_NamedViewsListWidget::LC_NamedViewsListWidget(const QString& title, QWidget* parent)
    : LC_GraphicViewAwareWidget(parent), ui(new Ui::LC_NamedViewsListWidget) {
    ui->setupUi(this);
    setWindowTitle(title);
    initToolbar();
    loadOptions();
    createModel();
    updateButtonsState();
    updateWidgetSettings();
}

LC_NamedViewsListWidget::~LC_NamedViewsListWidget() {
    delete ui;
    delete m_options;
}

void LC_NamedViewsListWidget::initToolbar() const {
    connect(ui->tbSettings, &QToolButton::clicked, this, &LC_NamedViewsListWidget::invokeOptionsDialog);
    connect(ui->tbAddView, &QToolButton::clicked, this, &LC_NamedViewsListWidget::addNewView);
    connect(ui->tbRestoreView, &QToolButton::clicked, this, &LC_NamedViewsListWidget::invokeView);
    connect(ui->tbUpdateView, &QToolButton::clicked, this, &LC_NamedViewsListWidget::updateView);
    connect(ui->tbRemoveView, &QToolButton::clicked, this, &LC_NamedViewsListWidget::removeView);
    connect(ui->tbRenameView, &QToolButton::clicked, this, &LC_NamedViewsListWidget::renameView);
}

void LC_NamedViewsListWidget::updateButtonsState() const {
    const QItemSelectionModel* pModel = ui->tvTable->selectionModel();
    const bool enable = pModel->hasSelection();
    const bool singleSelection = pModel->selectedRows().size() == 1;
    bool notPrintPreview = false;
    if (m_graphicView != nullptr) {
        notPrintPreview = !m_graphicView->isPrintPreview();
    }
    ui->tbUpdateView->setEnabled(enable && singleSelection && notPrintPreview);
    ui->tbRestoreView->setEnabled(enable && singleSelection);
    ui->tbRenameView->setEnabled(enable && singleSelection);
    ui->tbRemoveView->setEnabled(enable);

    const bool hasViewsList = m_currentViewList != nullptr;
    ui->tbAddView->setEnabled(hasViewsList && notPrintPreview);
    if (m_namedViewsButton != nullptr) {
        QAction* restoreAction = m_namedViewsButton->defaultAction();
        restoreAction->setEnabled(hasViewsList && m_currentViewList->count() > 0);
    }
    if (m_saveViewAction != nullptr) {
        m_saveViewAction->setEnabled(hasViewsList && notPrintPreview);
    }
}

class LC_ViewsTableItemDelegate : public LC_TableItemDelegateBase {
public:
    explicit LC_ViewsTableItemDelegate(LC_MouseTrackingTableView* parent, LC_NamedViewsModel* model, LC_NamedViewsListOptions* options) : LC_TableItemDelegateBase(parent) {
        m_model = model;
        m_options = options;
        auto palette = parent->palette();
        // m_hoverRowBackgroundColor = palette.color(QPalette::AlternateBase);
        m_hoverRowBackgroundColor = palette.color(QPalette::Mid);
    }

    void doPaint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyledItemDelegate::paint(painter, option, index);
        const bool drawGrid = m_options != nullptr && m_options->showGrid;
        if (drawGrid) {
            drawHorizontalGridLine(painter, option);
        }
    }
private:
    LC_NamedViewsModel* m_model;
    LC_NamedViewsListOptions* m_options;
};


void LC_NamedViewsListWidget::createModel() {
    m_viewsModel = new LC_NamedViewsModel(m_options, this);

    const auto tableView = ui->tvTable;
    tableView->setModel(m_viewsModel);
    tableView->setShowGrid(false);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setFocusPolicy(Qt::NoFocus);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setMinimumHeight(60);

    QHeaderView* verticalHeader = tableView->verticalHeader();
    const QFontMetrics fm(font());
    m_itemHeight = fm.height() + 6;
    verticalHeader->setDefaultSectionSize(m_itemHeight);
    verticalHeader->setOffset(2);
    verticalHeader->hide();

    QHeaderView* horizontalHeader{tableView->horizontalHeader()};
    horizontalHeader->setMinimumSectionSize(m_itemHeight);
    horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->hide();
    tableView->setColumnWidth(m_viewsModel->translateColumn(LC_NamedViewsModel::ICON_TYPE), m_itemHeight);

    // tableView->setShowGrid(true); // fixme - sand - add to options!
#ifndef DONT_FORCE_WIDGETS_CSS
    tableView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif

    connect(tableView, &QTableView::customContextMenuRequested, this, &LC_NamedViewsListWidget::onCustomContextMenu);
    connect(tableView, &QTableView::clicked, this, &LC_NamedViewsListWidget::slotTableClicked);
    connect(tableView, &QTableView::doubleClicked, this, &LC_NamedViewsListWidget::onTableDoubleClicked);

    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LC_NamedViewsListWidget::onTableSelectionChanged);

    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->setTrackingItemDelegate(new LC_ViewsTableItemDelegate(tableView, m_viewsModel, m_options));
}

void LC_NamedViewsListWidget::setGraphicView(RS_GraphicView* gv) {
    LC_ViewList* viewsList = nullptr;
    if (gv != nullptr && gv->getGraphic() != nullptr) {
        RS_Graphic* graphic = gv->getGraphic();
        viewsList = graphic->getViewList();
        m_viewport = gv->getViewPort();
    }
    else {
        m_viewport = nullptr;
    }
    m_graphicView = gv;
    setViewsList(viewsList);
}

void LC_NamedViewsListWidget::setViewsList(LC_ViewList* viewsList) {
    m_currentViewList = viewsList;
    updateData(false);
    if (nullptr != viewsList && m_viewsModel->count() > 0) {
        selectView(m_currentViewList->at(0));
    }
}

void LC_NamedViewsListWidget::reload() {
    updateData(true);
}

void LC_NamedViewsListWidget::refresh() {
    updateData(true);
}

QLayout* LC_NamedViewsListWidget::getTopLevelLayout() const {
    return ui->gridLayout;
}

void LC_NamedViewsListWidget::updateData(const bool restoreSelectionIfPossible) {
    const int selectedRow = getSingleSelectedRow();
    LC_Formatter* formatter = nullptr;
    if (m_viewport != nullptr) {
        formatter = m_viewport->getFormatter();
    }
    m_viewsModel->setViewsList(m_currentViewList, formatter);
    restoreSingleSelectedRow(restoreSelectionIfPossible, selectedRow);
    updateButtonsState();
    if (m_options->showColumnIconType) {
        ui->tvTable->setColumnWidth(m_viewsModel->translateColumn(LC_NamedViewsModel::ICON_TYPE), m_itemHeight);
    }
    emit viewListChanged(m_viewsModel->count());
}

void LC_NamedViewsListWidget::restoreSingleSelectedRow(const bool restoreSelectionIfPossible, int selectedRow) const {
    if (restoreSelectionIfPossible && selectedRow > 0) {
        const int itemsCount = m_viewsModel->count();
        if (itemsCount > 0) {
            ui->tvTable->clearSelection();
            if (selectedRow >= itemsCount) {
                selectedRow = itemsCount - 1;
            }
            ui->tvTable->selectRow(selectedRow);
        }
    }
}

int LC_NamedViewsListWidget::getSingleSelectedRow() const {
    const QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    const qsizetype selectedSize = selectedIndexes.size();
    int selectedRow = -1;
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            const QModelIndex selectedIndex = selectedIndexes.at(0);
            selectedRow = selectedIndex.row();
        }
    }
    return selectedRow;
}

void LC_NamedViewsListWidget::invokeOptionsDialog() {
    auto dlg = LC_DlgNamedViewsListOptions(m_options, this);
    const int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted) {
        const int selectedRow = getSingleSelectedRow();
        m_options->save();
        updateData(false);
        restoreSingleSelectedRow(true, selectedRow);
    }
}

void LC_NamedViewsListWidget::updateView() {
    LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr) {
        updateExistingView(selectedView);
    }
}

void LC_NamedViewsListWidget::invokeView() {
    const LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr) {
        restoreView(selectedView);
    }
}

void LC_NamedViewsListWidget::removeAllViews() {
    if (m_currentViewList->count() > 0) {
        bool remove = false;
        const int result = QMessageBox::question(this, tr("Delete View"),
                                                 tr("Are you sure to delete ALL views?\n Warning: this action can NOT be undone!"),
                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        remove = result == QMessageBox::Yes;

        if (remove) {
            m_currentViewList->clear();
            refresh();
        }
    }
}

void LC_NamedViewsListWidget::removeView() {
    const QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    const qsizetype selectedSize = selectedIndexes.size();
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            const QModelIndex selectedIndex = selectedIndexes.at(0);
            if (selectedIndex.isValid()) {
                LC_View* selectedView = m_viewsModel->getItemForIndex(selectedIndex);
                if (selectedView != nullptr) {
                    bool remove = false;
                    if (m_options->askForDeletionConfirmation) {
                        const QString viewName = selectedView->getName();
                        const int result = QMessageBox::question(this, tr("Delete View"),
                                                                 tr(
                                                                     "Are you sure to delete view\n \"%1\"?\n Warning: this action can NOT be undone!")
                                                                .arg(viewName), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                        remove = result == QMessageBox::Yes;
                    }
                    else {
                        remove = true;
                    }
                    if (remove) {
                        removeExistingView(selectedView);
                        refresh();
                    }
                }
            }
        }
        else {
            QList<LC_View*> viewsToRemove;
            for (int i = 0; i < selectedSize; i++) {
                const QModelIndex& index = selectedIndexes.at(i);
                if (index.isValid()) {
                    const auto view = m_viewsModel->getItemForIndex(index);
                    viewsToRemove.push_back(view);
                }
            }

            if (m_options->askForDeletionConfirmation) {
                QString viewsName = "";
                for (const auto v : viewsToRemove) {
                    viewsName += "\n";
                    viewsName += v->getName();
                }
                const int result = QMessageBox::question(this, tr("Delete Views"),
                                                         tr("Are you sure to delete views %1?\nWarning: this action can NOT be undone!").
                                                         arg(viewsName), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (result == QMessageBox::Yes) {
                    for (const auto v : viewsToRemove) {
                        removeExistingView(v);
                    }
                    refresh();
                }
            }
            else {
                for (const auto v : viewsToRemove) {
                    removeExistingView(v);
                }
                refresh();
            }
        }
    }
}

void LC_NamedViewsListWidget::renameView() {
    LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr) {
        renameExistingView(selectedView);
    }
}

void LC_NamedViewsListWidget::addNewView() {
    bool ok = false;
    const QString defaultName = "";
    bool tryCreate = true;
    while (tryCreate) {
        QString text = QInputDialog::getText(this, tr("New View"), tr("View name:"), QLineEdit::Normal, defaultName, &ok);
        if (ok) {
            text = text.trimmed();
            if (text.isEmpty()) {
                QMessageBox::warning(this, tr("View Creation"), tr("Empty name of View is not allowed"), QMessageBox::Close,
                                     QMessageBox::Close);
            }
            else {
                LC_View* existingView = m_currentViewList->find(text);
                if (existingView == nullptr) {
                    doCreateNewView(text);
                    tryCreate = false;
                }
                else {
                    if (m_options->duplicatedNameReplacesSilently) {
                        updateExistingView(existingView);
                        tryCreate = false;
                    }
                    else {
                        QMessageBox::warning(this, tr("View Creation"), tr("View with provided name already exists"), QMessageBox::Close,
                                             QMessageBox::Close);
                    }
                }
            }
        }
        else {
            tryCreate = false;
        }
    }
}

void LC_NamedViewsListWidget::renameExistingView(LC_View* selectedView) {
    const QString viewName = selectedView->getName();
    bool ok = false;
    bool tryRename = true;
    while (tryRename) {
        QString text = QInputDialog::getText(this, tr("Rename View"), tr("View name:"), QLineEdit::Normal, viewName, &ok);
        if (ok) {
            text = text.trimmed();
            if (text.isEmpty()) {
                QMessageBox::warning(this, tr("View Rename"), tr("Empty name of View is not allowed"), QMessageBox::Close,
                                     QMessageBox::Close);
            }
            else {
                const LC_View* existingView = m_currentViewList->find(text);
                if (existingView == nullptr) {
                    renameExistingView(text, selectedView);
                    tryRename = false;
                }
                else if (existingView != selectedView) {
                    QMessageBox::warning(this, tr("View Rename"), tr("View with provided name already exists, select another one"),
                                         QMessageBox::Close, QMessageBox::Close);
                }
            }
        }
        else {
            tryRename = false;
        }
    }
}

void LC_NamedViewsListWidget::loadOptions() {
    if (m_options == nullptr) {
        m_options = new LC_NamedViewsListOptions();
    }
    m_options->load();
}

void LC_NamedViewsListWidget::doCreateNewView(const QString& name) {
    const auto viewToCreate = m_viewport->createNamedView(name);
    m_currentViewList->addNew(viewToCreate);
    refresh();
}

void LC_NamedViewsListWidget::doUpdateView(LC_View* view) {
    m_viewport->updateNamedView(view);
    m_currentViewList->edited(view);
    refresh();
}

void LC_NamedViewsListWidget::onUcsListChanged() const {
    updateViewsUCSNames();
}

void LC_NamedViewsListWidget::updateViewsUCSNames() const {
    if (m_graphicView != nullptr) {
        RS_Graphic* graphic = m_graphicView->getGraphic();
        if (graphic != nullptr) {
            LC_UCSList* ucsList = graphic->getUCSList();
            m_viewsModel->updateViewsUCSNames(ucsList);
        }
        else {
            //            viewsModel->clear();
        }
    }
}

void LC_NamedViewsListWidget::onTableSelectionChanged([[maybe_unused]] const QItemSelection& selected,
                                                      [[maybe_unused]] const QItemSelection& deselected) const {
    updateButtonsState();
}

void LC_NamedViewsListWidget::onCustomContextMenu([[maybe_unused]] const QPoint& pos) {
    if (m_currentViewList == nullptr) {
        return;
    }
    auto contextMenu = QMenu(this);
    /* auto *caption = new QLabel(tr("Views Menu"), this);
     QPalette palette;
     palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
     palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
     caption->setPalette(palette);
     caption->setAlignment(Qt::AlignCenter);*/

    using ActionMemberFunc = void (LC_NamedViewsListWidget::*)();
    const auto addActionFunc = [this, &contextMenu](const QString& iconName, const QString& name, ActionMemberFunc func) {
        contextMenu.addAction(QIcon(":/icons/" + iconName + ".lci"), name, this, func);
    };

    const QModelIndex index = ui->tvTable->indexAt(pos);
    const bool notInPrintPreview = !m_graphicView->isPrintPreview();
    if (index.isValid()) {
        const qsizetype selectedItemsCount = ui->tvTable->selectionModel()->selectedRows().size();
        if (selectedItemsCount > 1) {
            if (notInPrintPreview) {
                addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu.addSeparator();
            }
            addActionFunc("remove", tr("R&emove Selected Views"), &LC_NamedViewsListWidget::removeView);
        }
        else {
            if (notInPrintPreview) {
                addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu.addSeparator();
            }
            addActionFunc("nview_visible", tr("&Restore View"), &LC_NamedViewsListWidget::invokeView);
            addActionFunc("rename_active_block", tr("Re&name View"), &LC_NamedViewsListWidget::renameView);
            addActionFunc("nview_update", tr("&Update View"), &LC_NamedViewsListWidget::updateView);
            addActionFunc("remove", tr("R&emove View"), &LC_NamedViewsListWidget::removeView);
            contextMenu.addSeparator();
            addActionFunc("close_all", tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    else {
        // click is not on item
        if (notInPrintPreview) {
            addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
        }
        if (m_currentViewList->count() > 0) {
            if (notInPrintPreview) {
                contextMenu.addSeparator();
            }
            addActionFunc("close_all", tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    contextMenu.exec(QCursor::pos());
}

void LC_NamedViewsListWidget::slotTableClicked(const QModelIndex& modelIndex) {
    if (!modelIndex.isValid()) {
        return;
    }
    if (m_options->restoreViewBySingleClick) {
        const LC_View* view = m_viewsModel->getItemForIndex(modelIndex);
        if (view == nullptr) {
            return;
        }
        restoreView(view);
    }
}

void LC_NamedViewsListWidget::onTableDoubleClicked() {
    LC_View* view = getSelectedView();
    if (view != nullptr) {
        switch (m_options->doubleClickPolicy) {
            case LC_NamedViewsListOptions::NOTHING: {
                break;
            }
            case LC_NamedViewsListOptions::RENAME: {
                renameExistingView(view);
                break;
            }
            case LC_NamedViewsListOptions::UPDATE: {
                updateExistingView(view);
                break;
            }
            case LC_NamedViewsListOptions::INVOKE: {
                restoreView(view);
                break;
            }
            default:
                break;
        }
    }
}

LC_View* LC_NamedViewsListWidget::getSelectedView() const {
    LC_View* result = nullptr;
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()) {
        if (m_currentViewList != nullptr) {
            result = m_viewsModel->getItemForIndex(selectedItemIndex);
        }
    }
    return result;
}

QModelIndex LC_NamedViewsListWidget::getSelectedItemIndex() const {
    QModelIndex result;
    const QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1) {
        // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

void LC_NamedViewsListWidget::removeExistingView(LC_View* view) const {
    m_currentViewList->remove(view);
}

void LC_NamedViewsListWidget::renameExistingView(const QString& newName, LC_View* view) {
    m_currentViewList->rename(view, newName);
    refresh();
}

void LC_NamedViewsListWidget::updateExistingView(LC_View* view) {
    doUpdateView(view);
}

void LC_NamedViewsListWidget::restoreView(const int index) {
    if (m_currentViewList != nullptr) {
        const LC_View* view = m_currentViewList->at(index);
        if (view != nullptr) {
            restoreView(view);
        }
    }
}

void LC_NamedViewsListWidget::restoreSelectedView() {
    invokeView();
}

void LC_NamedViewsListWidget::restoreView(const QString& name) {
    if (m_currentViewList != nullptr) {
        const LC_View* view = m_currentViewList->find(name);
        if (view != nullptr) {
            restoreView(view);
            selectView(view);
        }
    }
}

void LC_NamedViewsListWidget::selectView(const LC_View* view) const {
    const QModelIndex index = m_viewsModel->getIndexForView(view);
    if (index.isValid()) {
        ui->tvTable->clearSelection();
        ui->tvTable->selectRow(index.row());
    }
}

void LC_NamedViewsListWidget::restoreView(const LC_View* view) {
    if (view->isForPaperView()) {
        // fixme - sand - support of navigation to view in paperspace!!!
        QMessageBox::warning(this, tr("Restore View"), tr("Paper space view is not fully supported yet."), QMessageBox::Close,
                             QMessageBox::Close);
        /* todo - add support for navigation to view in paper space. So far it early to add support without proper support of paper space.
         * if (graphicView->isPrintPreview()){
             panZoomGraphicView(center, size); //  paper/scale should be proper positionted.
         }
         else{
             // switch to print-preview if available, or call print-preview action
             // zoom for print-preview
         }*/
    }
    else {
        if (m_graphicView->isPrintPreview()) {
            const auto* mdiWindow = LC_UI::findParentOfType<QC_MDIWindow>(m_graphicView);
            if (mdiWindow != nullptr) {
                QC_MDIWindow* parentWindow = mdiWindow->getParentWindow();
                if (parentWindow != nullptr) {
                    QC_ApplicationWindow::getAppWindow()->activateWindow(parentWindow);
                }
            }
        }
        m_viewport->restoreView(view);
    }
}

void LC_NamedViewsListWidget::fillViewsList(QList<LC_View*>& list) const {
    if (m_currentViewList != nullptr) {
        m_viewsModel->fillViewsList(list);
    }
}

QIcon LC_NamedViewsListWidget::getViewTypeIcon(const LC_View* view) const {
    return m_viewsModel->getTypeIcon(view);
}

QWidget* LC_NamedViewsListWidget::createSelectionWidget(QAction* saveViewAction, QAction* defaultAction) {
    m_namedViewsButton = new LC_NamedViewsButton(this);
    m_namedViewsButton->setDefaultAction(defaultAction);
    m_saveViewAction = saveViewAction;
    return m_namedViewsButton;
}
