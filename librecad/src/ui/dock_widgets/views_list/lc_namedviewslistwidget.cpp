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
    : LC_GraphicViewAwareWidget(parent)
    , ui(new Ui::LC_NamedViewsListWidget){
    ui->setupUi(this);
    setWindowTitle(title);

    ui->leFilterMask->setVisible(false);

    initToolbar();
    loadOptions();
    createModel();
    updateButtonsState();
    updateWidgetSettings();
}

LC_NamedViewsListWidget::~LC_NamedViewsListWidget(){
    delete ui;
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
    QItemSelectionModel *pModel = ui->tvTable->selectionModel();
    bool enable = pModel->hasSelection();
    bool singleSelection = pModel->selectedRows().size() == 1;
    bool notPrintPreview = false;
    if (m_graphicView != nullptr){
       notPrintPreview = !m_graphicView->isPrintPreview();
    }
    ui->tbUpdateView->setEnabled(enable && singleSelection && notPrintPreview);
    ui->tbRestoreView->setEnabled(enable && singleSelection);
    ui->tbRenameView->setEnabled(enable && singleSelection);
    ui->tbRemoveView->setEnabled(enable);

    bool hasViewsList = m_currentViewList != nullptr;
    ui->tbAddView->setEnabled(hasViewsList && notPrintPreview);
    if (m_namedViewsButton != nullptr) {
        QAction *restoreAction = m_namedViewsButton->defaultAction();
        restoreAction->setEnabled(hasViewsList && m_currentViewList->count() > 0);
    }
    if (m_saveViewAction != nullptr){
        m_saveViewAction->setEnabled(hasViewsList && notPrintPreview);
    }
}

namespace {
// the default icon size
    constexpr int ICON_WIDTH = 24;
}

void LC_NamedViewsListWidget::createModel() {
    m_viewsModel  = new LC_NamedViewsModel(m_options, this);

    QTableView *tableView = ui->tvTable;
    tableView->setModel(m_viewsModel);
    tableView->setShowGrid(true);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setFocusPolicy(Qt::NoFocus);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setMinimumHeight(60);

    QHeaderView *horizontalHeader {tableView->horizontalHeader()};
    horizontalHeader->setMinimumSectionSize(ICON_WIDTH + 4);
    horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->hide();

    QHeaderView *verticalHeader = tableView->verticalHeader();
    verticalHeader->setOffset(2);
    verticalHeader->hide();

    tableView->setColumnWidth(m_viewsModel->translateColumn(LC_NamedViewsModel::ICON_TYPE), ICON_WIDTH);
#ifndef DONT_FORCE_WIDGETS_CSS
    tableView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif


    connect(tableView, &QTableView::customContextMenuRequested, this, &LC_NamedViewsListWidget::onCustomContextMenu);
    connect(tableView, &QTableView::clicked, this, &LC_NamedViewsListWidget::slotTableClicked);
    connect(tableView, &QTableView::doubleClicked, this, &LC_NamedViewsListWidget::onTableDoubleClicked);

    connect( tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &LC_NamedViewsListWidget::onTableSelectionChanged);


    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void LC_NamedViewsListWidget::setGraphicView(RS_GraphicView *gv) {
    LC_ViewList *viewsList = nullptr;
    if (gv != nullptr && gv->getGraphic() != nullptr) {
        RS_Graphic *graphic = gv->getGraphic();
        loadFormats(graphic);
        viewsList = graphic->getViewList();
        m_viewport = gv->getViewPort();
    }
    else {
        m_viewport = nullptr;
    }
    m_graphicView = gv;
    setViewsList(viewsList);
}

void LC_NamedViewsListWidget::loadFormats(const RS_Graphic *graphic) {
    m_linearFormat = graphic->getLinearFormat();
    m_angleFormat = graphic->getAngleFormat();
    m_precision = graphic->getLinearPrecision();
    m_anglePrecision = graphic->getAnglePrecision();
    drawingUnit = graphic->getUnit();
}

void LC_NamedViewsListWidget::setViewsList(LC_ViewList *viewsList) {
    m_currentViewList = viewsList;
    updateData(false);
    if (nullptr != viewsList && m_viewsModel->count() > 0){
        selectView(m_currentViewList->at(0));
    }
}

void LC_NamedViewsListWidget::reload() {
    if (m_graphicView != nullptr) {
        RS_Graphic *graphic = m_graphicView->getGraphic();
        loadFormats(graphic);
    }
    updateData(true);
}

void LC_NamedViewsListWidget::refresh() {
    updateData(true);
}

void LC_NamedViewsListWidget::updateData(bool restoreSelectionIfPossible) {
    int selectedRow = getSingleSelectedRow();
    m_viewsModel->setViewsList(m_currentViewList, m_linearFormat, m_angleFormat, m_precision, m_anglePrecision, drawingUnit);
    restoreSingleSelectedRow(restoreSelectionIfPossible, selectedRow);
    updateButtonsState();
    if (m_options->showColumnIconType){
        ui->tvTable->setColumnWidth(m_viewsModel->translateColumn(LC_NamedViewsModel::ICON_TYPE), ICON_WIDTH);
    }
    emit viewListChanged(m_viewsModel->count());
}

void LC_NamedViewsListWidget::restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow) const {
    if (restoreSelectionIfPossible && selectedRow > 0){
        int itemsCount = m_viewsModel->count();
        if (itemsCount > 0) {
            ui->tvTable->clearSelection();
            if (selectedRow >= itemsCount){
                selectedRow = itemsCount - 1;
            }
            ui->tvTable->selectRow(selectedRow);
        }
    }
}

int LC_NamedViewsListWidget::getSingleSelectedRow() const {
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    qsizetype selectedSize = selectedIndexes.size();
    int selectedRow = -1;
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            QModelIndex  selectedIndex = selectedIndexes.at(0);
            selectedRow = selectedIndex.row();
        }
    }
    return selectedRow;
}

void LC_NamedViewsListWidget::invokeOptionsDialog() {
    int selectedRow = getSingleSelectedRow();
    LC_DlgNamedViewsListOptions dlg = LC_DlgNamedViewsListOptions(m_options, this);
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        m_options->save();
        updateData(false);
        restoreSingleSelectedRow(true, selectedRow);
    }
}

void LC_NamedViewsListWidget::updateView() {
    LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr){
        updateExistingView(selectedView);
    }
}

void LC_NamedViewsListWidget::invokeView() {
    LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr){
        restoreView(selectedView);
    }
}

void LC_NamedViewsListWidget::removeAllViews() {
    if (m_currentViewList->count() > 0) {
        bool remove = false;
        int result = QMessageBox::question(this, tr("Delete View"),
                                           tr("Are you sure to delete ALL views?\n Warning: this action can NOT be undone!"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        remove = result == QMessageBox::Yes;

        if (remove){
            m_currentViewList->clear();
            refresh();
        }
    }
}

void LC_NamedViewsListWidget::removeView() {
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    qsizetype selectedSize = selectedIndexes.size();
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            QModelIndex selectedIndex = selectedIndexes.at(0);
            if (selectedIndex.isValid()) {
                LC_View *selectedView = m_viewsModel->getItemForIndex(selectedIndex);
                if (selectedView != nullptr) {
                    bool remove = false;
                    if (m_options->askForDeletionConfirmation) {
                        QString viewName = selectedView->getName();
                        int result = QMessageBox::question(this, tr("Delete View"),
                                                           tr("Are you sure to delete view\n \"%1\"?\n Warning: this action can NOT be undone!").arg(viewName),
                                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                        remove = result == QMessageBox::Yes;
                    }
                    else{
                        remove = true;
                    }
                    if (remove) {
                        removeExistingView(selectedView);
                        refresh();
                    }
                }
            }
        }
        else{
            QList<LC_View*> viewsToRemove;
            for (int i=0; i < selectedSize; i++){
                const QModelIndex &index = selectedIndexes.at(i);
                if (index.isValid()){
                    auto view  = m_viewsModel->getItemForIndex(index);
                    viewsToRemove.push_back(view);
                }
            }

            if (m_options->askForDeletionConfirmation){
                QString viewsName = "";
                for (auto v: viewsToRemove){
                    viewsName += "\n";
                    viewsName += v->getName();
                }
                int result = QMessageBox::question(this, tr("Delete Views"), tr("Are you sure to delete views %1?\nWarning: this action can NOT be undone!").arg(viewsName), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
                if (result == QMessageBox::Yes){
                    for (auto v: viewsToRemove){
                        removeExistingView(v);
                    }
                    refresh();
                }
            }
            else{
                for (auto v: viewsToRemove){
                    removeExistingView(v);
                }
                refresh();
            }
        }
    }
}

void LC_NamedViewsListWidget::renameView() {
    LC_View* selectedView = getSelectedView();
    if (selectedView != nullptr){
        renameExistingView(selectedView);
    }
}

void LC_NamedViewsListWidget::addNewView() {
    bool ok;
    QString defaultName = "";
    bool tryCreate = true;
    while (tryCreate) {
        QString text = QInputDialog::getText(this, tr("New View"),
                                             tr("View name:"), QLineEdit::Normal,
                                             defaultName, &ok);
        if (ok) {
            text = text.trimmed();
            if (text.isEmpty()) {
                QMessageBox::warning(this, tr("View Creation"), tr("Empty name of View is not allowed"), QMessageBox::Close, QMessageBox::Close);
            } else {
                LC_View *existingView = m_currentViewList->find(text);
                if (existingView == nullptr) {
                        doCreateNewView(text);
                    tryCreate = false;
                } else {
                    if (m_options->duplicatedNameReplacesSilently) {
                        updateExistingView(existingView);
                        tryCreate = false;
                    } else {
                        QMessageBox::warning(this, tr("View Creation"), tr("View with provided name already exists"), QMessageBox::Close, QMessageBox::Close);
                    }
                }
            }
        }
        else{
            tryCreate = false;
        }
    }
}

void LC_NamedViewsListWidget::renameExistingView(LC_View *selectedView) {
    QString viewName = selectedView->getName();
    bool ok;
    bool tryRename = true;
    while (tryRename) {
        QString text = QInputDialog::getText(this, tr("Rename View"),
                                             tr("View name:"), QLineEdit::Normal,
                                             viewName, &ok);
        if (ok) {
            text = text.trimmed();
            if (text.isEmpty()) {
                QMessageBox::warning(this, tr("View Rename"), tr("Empty name of View is not allowed"), QMessageBox::Close, QMessageBox::Close);
            } else {
                LC_View *existingView = m_currentViewList->find(text);
                if (existingView == nullptr) {
                    renameExistingView(text, selectedView);
                    tryRename = false;
                } else if (existingView != selectedView) {
                    QMessageBox::warning(this, tr("View Rename"), tr("View with provided name already exists, select another one"), QMessageBox::Close,
                                         QMessageBox::Close);
                }
            }
        }
        else{
            tryRename = false;
        }
    }
}

void LC_NamedViewsListWidget::loadOptions() {
    if (m_options == nullptr){
        m_options = new LC_NamedViewsListOptions();
    }
    m_options->load();
}

void LC_NamedViewsListWidget::doCreateNewView(const QString& name) {
    auto viewToCreate = m_viewport->createNamedView(name);
    m_currentViewList->addNew(viewToCreate);
    refresh();
}

void LC_NamedViewsListWidget::doUpdateView(LC_View *view) {
    m_viewport->updateNamedView(view);
    m_currentViewList->edited(view);
    refresh();
}

void LC_NamedViewsListWidget::onUcsListChanged() const {
    updateViewsUCSNames();
}

void LC_NamedViewsListWidget::updateViewsUCSNames() const {
    if (m_graphicView != nullptr) {
        RS_Graphic *graphic = m_graphicView->getGraphic();
        if (graphic != nullptr) {
            LC_UCSList *ucsList = graphic->getUCSList();
            m_viewsModel->updateViewsUCSNames(ucsList);
        }
        else{
//            viewsModel->clear();
        }
    }
}

void LC_NamedViewsListWidget::onTableSelectionChanged([[maybe_unused]] const QItemSelection &selected,
                                                      [[maybe_unused]] const QItemSelection &deselected) const {
    updateButtonsState();
}

void LC_NamedViewsListWidget::onCustomContextMenu([[maybe_unused]] const QPoint &pos){

    if (m_currentViewList == nullptr){
        return;
    }
    auto *contextMenu = new QMenu(this);
   /* auto *caption = new QLabel(tr("Views Menu"), this);
    QPalette palette;
    palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
    palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
    caption->setPalette(palette);
    caption->setAlignment(Qt::AlignCenter);*/


    using ActionMemberFunc = void (LC_NamedViewsListWidget::*)();
    const auto addActionFunc = [this, &contextMenu](const QString& iconName, const QString& name, ActionMemberFunc func) {
        contextMenu->addAction(QIcon(":/icons/" + iconName + ".lci"), name, this, func);
    };

    QModelIndex index = ui->tvTable->indexAt(pos);
    bool notInPrintPreview = !m_graphicView->isPrintPreview();
    if (index.isValid()){
        qsizetype selectedItemsCount = ui->tvTable->selectionModel()->selectedRows().size();
        if (selectedItemsCount > 1){
            if (notInPrintPreview) {
                addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu->addSeparator();
            }
            addActionFunc("remove", tr("R&emove Selected Views"), &LC_NamedViewsListWidget::removeView);
        }
        else {
            if (notInPrintPreview) {
                addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu->addSeparator();
            }
            addActionFunc("nview_visible", tr("&Restore View"), &LC_NamedViewsListWidget::invokeView);
            addActionFunc("rename_active_block", tr("Re&name View"), &LC_NamedViewsListWidget::renameView);
            addActionFunc("nview_update",tr("&Update View"), &LC_NamedViewsListWidget::updateView);
            addActionFunc("remove", tr("R&emove View"), &LC_NamedViewsListWidget::removeView);
            contextMenu->addSeparator();
            addActionFunc("close_all", tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    else{
        // click is not on item
        if (notInPrintPreview) {
            addActionFunc("nview_add", tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
        }
        if (m_currentViewList->count() > 0){
            if (notInPrintPreview) {
                contextMenu->addSeparator();
            }
            addActionFunc("close_all",tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void LC_NamedViewsListWidget::slotTableClicked(const QModelIndex& modelIndex) {
    if (!modelIndex.isValid()) {
        return;
    }
    if (m_options->restoreViewBySingleClick) {
        LC_View *view = m_viewsModel->getItemForIndex(modelIndex);
        if (view == nullptr) {
            return;
        } else {
            restoreView(view);
        }
    }
}

void LC_NamedViewsListWidget::onTableDoubleClicked() {
    LC_View *view = getSelectedView();
    if (view != nullptr){
        switch (m_options->doubleClickPolicy){
            case LC_NamedViewsListOptions::NOTHING:{
                break;
            }
            case LC_NamedViewsListOptions::RENAME:{
                renameExistingView(view);
                break;
            }
            case LC_NamedViewsListOptions::UPDATE:{
                updateExistingView(view);
                break;
            }
            case LC_NamedViewsListOptions::INVOKE:{
                restoreView(view);
                break;
            }
            default:
                break;
        }
    }
}

LC_View *LC_NamedViewsListWidget::getSelectedView() const {
    LC_View* result = nullptr;
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        if (m_currentViewList != nullptr) {
            result = m_viewsModel->getItemForIndex(selectedItemIndex);
        }
    }
    return result;
}

QModelIndex LC_NamedViewsListWidget::getSelectedItemIndex() const {
    QModelIndex result;
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

void LC_NamedViewsListWidget::removeExistingView(LC_View *view) const {
    m_currentViewList->remove(view);
}

void LC_NamedViewsListWidget::renameExistingView(const QString &newName, LC_View *view) {
    m_currentViewList->rename(view, newName);
    refresh();
}

void LC_NamedViewsListWidget::updateExistingView(LC_View *view) {
    doUpdateView(view);
}

void LC_NamedViewsListWidget::restoreView(int index) {
    if (m_currentViewList != nullptr) {
        LC_View* view = m_currentViewList->at(index);
        if (view != nullptr){
            restoreView(view);
        }
    }
}

void LC_NamedViewsListWidget::restoreSelectedView() {
    invokeView();
}

void LC_NamedViewsListWidget::restoreView(const QString &name) {
    if (m_currentViewList != nullptr) {
        LC_View* view = m_currentViewList->find(name);
        if (view != nullptr){
            restoreView(view);
            selectView(view);
        }
    }
}

void LC_NamedViewsListWidget::selectView(const LC_View *view) const {
    QModelIndex index = m_viewsModel->getIndexForView(view);
    if (index.isValid()){
        ui->tvTable->clearSelection();
        ui->tvTable->selectRow(index.row());
    }
}

void LC_NamedViewsListWidget::restoreView(LC_View *view) {
   if (view->isForPaperView()){
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
   else{
       if (m_graphicView->isPrintPreview()){
           auto* mdiWindow = LC_UI::findParentOfType<QC_MDIWindow>(m_graphicView);
           if (mdiWindow != nullptr) {
               QC_MDIWindow *parentWindow = mdiWindow->getParentWindow();
               if (parentWindow != nullptr) {
                   QC_ApplicationWindow::getAppWindow()->activateWindow(parentWindow);
               }
           }
       }
       m_viewport->restoreView(view);
   }
}

void LC_NamedViewsListWidget::fillViewsList(QList<LC_View *> &list) const {
    if (m_currentViewList != nullptr){
        m_viewsModel->fillViewsList(list);
    }
}

QIcon LC_NamedViewsListWidget::getViewTypeIcon(LC_View *view) const {
    return m_viewsModel->getTypeIcon(view);
}

QWidget *LC_NamedViewsListWidget::createSelectionWidget(QAction* saveAction, QAction* defaultAction) {
    m_namedViewsButton = new LC_NamedViewsButton(this);
    m_namedViewsButton->setDefaultAction(defaultAction);
    m_saveViewAction = saveAction;
    return m_namedViewsButton;
}

void LC_NamedViewsListWidget::updateWidgetSettings() const {
    LC_GROUP("Widgets"); {
        bool flatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        int iconSize = LC_GET_INT("DockWidgetsIconSize", 16);

        QSize size(iconSize, iconSize);

        QList<QToolButton *> widgets = this->findChildren<QToolButton *>();
        foreach(QToolButton *w, widgets) {
            w->setAutoRaise(flatIcons);
            w->setIconSize(size);
        }
    }
    LC_GROUP_END();
}
