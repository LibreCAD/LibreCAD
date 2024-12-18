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

#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QTimer>
#include <QStyleHints>
#include <QContextMenuEvent>
#include "lc_namedviewslistwidget.h"
#include "rs_graphic.h"
#include "ui_lc_namedviewslistwidget.h"
#include "rs_debug.h"
#include "lc_dlgnamedviewslistoptions.h"
#include "lc_namedviewsbutton.h"
#include "qc_applicationwindow.h"

LC_NamedViewsListWidget::LC_NamedViewsListWidget(const QString& title, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LC_NamedViewsListWidget){
    ui->setupUi(this);
    setWindowTitle(title);

    ui->leFilterMask->setVisible(false);

    initToolbar();
    loadOptions();
    createModel();
    updateButtonsState();
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
    if (graphicView != nullptr){
       notPrintPreview = !graphicView->isPrintPreview();
    }
    ui->tbUpdateView->setEnabled(enable && singleSelection && notPrintPreview);
    ui->tbRestoreView->setEnabled(enable && singleSelection);
    ui->tbRenameView->setEnabled(enable && singleSelection);
    ui->tbRemoveView->setEnabled(enable);

    bool hasViewsList = currentViewList != nullptr;
    ui->tbAddView->setEnabled(hasViewsList && notPrintPreview);
    if (namedViewsButton != nullptr) {
        QAction *restoreAction = namedViewsButton->defaultAction();
        restoreAction->setEnabled(hasViewsList && currentViewList->count() > 0);
    }
    if (saveViewAction != nullptr){
        saveViewAction->setEnabled(hasViewsList && notPrintPreview);
    }
}

namespace {
// the default icon size
    constexpr static int ICON_WIDTH = 24;
}

void LC_NamedViewsListWidget::createModel() {
    viewsModel  = new LC_NamedViewsModel(options, this);

    QTableView *tableView = ui->tvTable;
    tableView->setModel(viewsModel);
    tableView->setShowGrid(true);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setFocusPolicy(Qt::NoFocus);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setMinimumHeight(60);

    QHeaderView *pHeader {tableView->horizontalHeader()};
    pHeader->setMinimumSectionSize( ICON_WIDTH + 4);
    pHeader->setStretchLastSection(true);
    pHeader->hide();

    QHeaderView *verticalHeader = tableView->verticalHeader();
    verticalHeader->setOffset(2);
    verticalHeader->hide();

    tableView->setColumnWidth(viewsModel->translateColumn(LC_NamedViewsModel::ICON), ICON_WIDTH);

    tableView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");


    connect(tableView, &QTableView::customContextMenuRequested, this, &LC_NamedViewsListWidget::onCustomContextMenu);
    connect(tableView, &QTableView::clicked, this, &LC_NamedViewsListWidget::slotTableClicked);
    connect(tableView, &QTableView::doubleClicked, this, &LC_NamedViewsListWidget::onTableDoubleClicked);

    connect( tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &LC_NamedViewsListWidget::onTableSelectionChanged);


    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void LC_NamedViewsListWidget::setGraphicView(RS_GraphicView *gv,
                                             QMdiSubWindow *w) {
    LC_ViewList *viewsList = nullptr;
    if (gv != nullptr && gv->getGraphic() != nullptr) {
        RS_Graphic *graphic = gv->getGraphic();
        linearFormat = graphic->getLinearFormat();
        precision = graphic->getLinearPrecision();
        drawingUnit = graphic->getUnit();
        viewsList = graphic->getViewList();
    }
    graphicView = gv;
    window = w;
    setViewsList(viewsList);
}

void LC_NamedViewsListWidget::setViewsList(LC_ViewList *viewsList) {
    currentViewList = viewsList;
    updateData(false);
    if (nullptr != viewsList && viewsModel->count() > 0){
        selectView(currentViewList->at(0));
    }
}

void LC_NamedViewsListWidget::refresh() {
    updateData(true);
}

void LC_NamedViewsListWidget::updateData(bool restoreSelectionIfPossible) {
    int selectedRow = getSingleSelectedRow();
    viewsModel->setViewsList(currentViewList, linearFormat, precision, drawingUnit);
    restoreSingleSelectedRow(restoreSelectionIfPossible, selectedRow);
    updateButtonsState();
    if (options->showTypeIcon){
        ui->tvTable->setColumnWidth(viewsModel->translateColumn(LC_NamedViewsModel::ICON),ICON_WIDTH);
    }
    emit viewListChanged(viewsModel->count());
}

void LC_NamedViewsListWidget::restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow) {
    if (restoreSelectionIfPossible && selectedRow > 0){
        int itemsCount = viewsModel->count();
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
    LC_DlgNamedViewsListOptions dlg = LC_DlgNamedViewsListOptions(options, this);
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->save();
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
    bool remove = false;
    if (currentViewList->count() > 0) {
        int result = QMessageBox::question(this, tr("Delete View"),
                                           tr("Are you sure to delete ALL views?\n Warning: this action can NOT be undone!"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        remove = result == QMessageBox::Yes;

        if (remove){
            currentViewList->clear();
            refresh();
        }
    }
}

void LC_NamedViewsListWidget::removeView() {
    bool remove = false;
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    qsizetype selectedSize = selectedIndexes.size();
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            QModelIndex selectedIndex = selectedIndexes.at(0);
            if (selectedIndex.isValid()) {
                LC_View *selectedView = viewsModel->getItemForIndex(selectedIndex);
                if (selectedView != nullptr) {
                    if (options->askForDeletionConfirmation) {
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
                    auto view  = viewsModel->getItemForIndex(index);
                    viewsToRemove.push_back(view);
                }
            }

            if (options->askForDeletionConfirmation){
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
                LC_View *existingView = currentViewList->find(text);
                if (existingView == nullptr) {
                    doCreateNewView(text);
                    tryCreate = false;
                } else {
                    if (options->duplicatedNameReplacesSilently) {
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
                LC_View *existingView = currentViewList->find(text);
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
    if (options == nullptr){
        options = new LC_NamedViewsListOptions();
    }
    options->load();
}

void LC_NamedViewsListWidget::doCreateNewView(QString name) {
    auto* viewToCreate = new LC_View(name);
    doUpdateViewByGraphicView(viewToCreate);
    currentViewList->addNew(viewToCreate);
    refresh();
}

void LC_NamedViewsListWidget::doUpdateView(LC_View *view) {
    doUpdateViewByGraphicView(view);
    currentViewList->edited(view);
    refresh();
}

void LC_NamedViewsListWidget::doUpdateViewByGraphicView(LC_View *view) const {
    view->setForPaperView(graphicView->isPrintPreview());

    int width = graphicView->getWidth();
    int height = graphicView->getHeight();

    double x = graphicView->toGraphX(width);
    double y = graphicView->toGraphY(height);

    double x0 = graphicView->toGraphX(0);
    double y0 = graphicView->toGraphY(0);

    view->setCenter({(x + x0) / 2.0, (y + y0) / 2.0, 0});
    view->setSize({(x - x0), (y - y0), 0});

    view->setTargetPoint({0, 0, 0});
}

void LC_NamedViewsListWidget::onTableSelectionChanged([[maybe_unused]] const QItemSelection &selected,
                                                      [[maybe_unused]] const QItemSelection &deselected){
    updateButtonsState();
}

void LC_NamedViewsListWidget::onCustomContextMenu([[maybe_unused]] const QPoint &pos){

    if (currentViewList == nullptr){
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
    const auto addActionFunc = [this, &contextMenu](const QString& name, ActionMemberFunc func) {
        contextMenu->addAction(name, this, func);
    };

    QModelIndex index = ui->tvTable->indexAt(pos);
    bool notInPrintPreview = !graphicView->isPrintPreview();
    if (index.isValid()){
        int selectedItemsCount = ui->tvTable->selectionModel()->selectedRows().size();
        if (selectedItemsCount > 1){
            if (notInPrintPreview) {
                addActionFunc(tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu->addSeparator();
            }
            addActionFunc(tr("R&emove Selected Views"), &LC_NamedViewsListWidget::removeView);
        }
        else {
            if (notInPrintPreview) {
                addActionFunc(tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
                contextMenu->addSeparator();
            }
            addActionFunc(tr("&Restore View"), &LC_NamedViewsListWidget::invokeView);
            addActionFunc(tr("Re&name View"), &LC_NamedViewsListWidget::renameView);
            addActionFunc(tr("&Update View"), &LC_NamedViewsListWidget::updateView);
            addActionFunc(tr("R&emove View"), &LC_NamedViewsListWidget::removeView);
            contextMenu->addSeparator();
            addActionFunc(tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    else{
        // click is not on item
        if (notInPrintPreview) {
            addActionFunc(tr("&Add View"), &LC_NamedViewsListWidget::addNewView);
        }
        if (currentViewList->count() > 0){
            if (notInPrintPreview) {
                contextMenu->addSeparator();
            }
            addActionFunc(tr("Remove A&ll Views"), &LC_NamedViewsListWidget::removeAllViews);
        }
    }
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void LC_NamedViewsListWidget::slotTableClicked(QModelIndex modelIndex) {
    if (!modelIndex.isValid()) {
        return;
    }
    if (options->restoreViewBySingleClick) {
        LC_View *view = viewsModel->getItemForIndex(modelIndex);
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
        switch (options->doubleClickPolicy){
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

LC_View *LC_NamedViewsListWidget::getSelectedView() {
    LC_View* result = nullptr;
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        if (currentViewList != nullptr) {
            result = viewsModel->getItemForIndex(selectedItemIndex);
        }
    }
    return result;
}

QModelIndex LC_NamedViewsListWidget::getSelectedItemIndex(){
    QModelIndex result;
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

void LC_NamedViewsListWidget::removeExistingView(LC_View *view) {
    currentViewList->remove(view);
}

void LC_NamedViewsListWidget::renameExistingView(QString newName, LC_View *view) {
    currentViewList->rename(view, newName);
    refresh();
}

void LC_NamedViewsListWidget::updateExistingView(LC_View *view) {
    doUpdateView(view);
}

void LC_NamedViewsListWidget::restoreView(int index) {
    if (currentViewList != nullptr) {
        LC_View* view = currentViewList->at(index);
        if (view != nullptr){
            restoreView(view);
        }
    }
}

void LC_NamedViewsListWidget::restoreSelectedView() {
    invokeView();
}

void LC_NamedViewsListWidget::restoreView(const QString &name) {
    if (currentViewList != nullptr) {
        LC_View* view = currentViewList->find(name);
        if (view != nullptr){
            restoreView(view);
            selectView(view);
        }
    }
}

void LC_NamedViewsListWidget::selectView(LC_View *view) {
    QModelIndex index = viewsModel->getIndexForView(view);
    if (index.isValid()){
        ui->tvTable->clearSelection();
        ui->tvTable->selectRow(index.row());
    }
}

void LC_NamedViewsListWidget::restoreView(LC_View *view) {
   RS_Vector center = view->getCenter();
   RS_Vector size = view->getSize();
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
       if (graphicView->isPrintPreview()){
           QC_MDIWindow* mdiWindow = qobject_cast<QC_MDIWindow*>(window);
           QC_MDIWindow *parentWindow = mdiWindow->getParentWindow();
           if (parentWindow != nullptr) {
               QC_ApplicationWindow::getAppWindow()->activateWindow(parentWindow);
               panZoomGraphicView(center, size);
           }
       }
       else {
           panZoomGraphicView(center, size);
       }
   }
}

void LC_NamedViewsListWidget::panZoomGraphicView(const RS_Vector &center, const RS_Vector &size) {
    const RS_Vector halfSize = size / 2;
    RS_Vector v1 = center - halfSize;
    RS_Vector v2 = center + halfSize;
    graphicView->zoomWindow(v1, v2, true);
}

void LC_NamedViewsListWidget::fillViewsList(QList<LC_View *> &list) {
    if (currentViewList != nullptr){
        viewsModel->fillViewsList(list);
    }
}

QIcon LC_NamedViewsListWidget::getViewTypeIcon(LC_View *view) {
    return viewsModel->getTypeIcon(view);
}

QWidget *LC_NamedViewsListWidget::createSelectionWidget(QAction* saveAction, QAction* defaultAction) {
    namedViewsButton = new LC_NamedViewsButton(this);
    namedViewsButton->setDefaultAction(defaultAction);
    saveViewAction = saveAction;
    return namedViewsButton;
}
