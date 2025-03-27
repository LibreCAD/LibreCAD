/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "lc_ucslistwidget.h"
#include "ui_lc_ucslistwidget.h"
#include "qc_mdiwindow.h"
#include "qc_applicationwindow.h"
#include "lc_dlgucslistoptions.h"
#include "lc_ucslistbutton.h"
#include "lc_graphicviewport.h"
#include "rs_settings.h"
#include "lc_widgets_common.h"

LC_UCSListWidget::LC_UCSListWidget(const QString& title, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_UCSListWidget){
    ui->setupUi(this);
    setWindowTitle(title);

    ui->leFilterMask->setVisible(false);

    initToolbar();
    loadOptions();
    createModel();
    updateButtonsState();
    updateWidgetSettings();
}

LC_UCSListWidget::~LC_UCSListWidget(){
    delete ui;
}

void LC_UCSListWidget::initToolbar() const {
    connect(ui->tbSettings, &QToolButton::clicked, this, &LC_UCSListWidget::invokeOptionsDialog);
    connect(ui->tbRestore, &QToolButton::clicked, this, &LC_UCSListWidget::activateUCS);
    connect(ui->tbPreview, &QToolButton::clicked, this, &LC_UCSListWidget::previewUCS);
    connect(ui->tbUpdate, &QToolButton::clicked, this, &LC_UCSListWidget::saveCurrentUCS);
    connect(ui->tbRemove, &QToolButton::clicked, this, &LC_UCSListWidget::removeUCS);
    connect(ui->tbRename, &QToolButton::clicked, this, &LC_UCSListWidget::editUCS);
}

void LC_UCSListWidget::updateButtonsState() const {
    QItemSelectionModel *pModel = ui->tvTable->selectionModel();
    bool enable = pModel->hasSelection();
    bool singleSelection = pModel->selectedRows().size() == 1;
    bool notPrintPreview = false;
    if (graphicView != nullptr){
        notPrintPreview = !graphicView->isPrintPreview();
    }
    bool isUCS = false;
    if (singleSelection){
        LC_UCS* ucs = ucsListModel->getItemForIndex(pModel->selectedRows().at(0));
        if (ucs != nullptr){
            isUCS = ucs->isUCS();
        }
    }

    bool hasNoActiveUCS = ucsListModel->getActiveUCS() == nullptr;
    ui->tbUpdate->setEnabled(notPrintPreview && hasNoActiveUCS);
    ui->tbRestore->setEnabled(enable && singleSelection);
    ui->tbPreview->setEnabled(enable && singleSelection);
    ui->tbRename->setEnabled(enable && singleSelection && isUCS);
    ui->tbRemove->setEnabled(enable && isUCS);

    bool hasUCSList = currentUCSList != nullptr;
    ui->tbAdd->setEnabled(hasUCSList && notPrintPreview);

    int count = ucsListModel->count();
    bool hasUCSs = count > 1;

    if (ucsListButton != nullptr) {
        ucsListButton->enableSubActions(hasUCSs);
        QAction *restoreAction = ucsListButton->defaultAction();
        restoreAction->setEnabled(hasUCSs);
    }

    if (applyUCSAction != nullptr){
        // do nothing so far
    }
}

namespace {
// the default icon size
    constexpr static int ICON_WIDTH = 24;
}

void LC_UCSListWidget::createModel() {
    ucsListModel  = new LC_UCSListModel(options, this);

    QTableView *tableView = ui->tvTable;
    tableView->setModel(ucsListModel);
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

#ifndef DONT_FORCE_WIDGETS_CSS
    tableView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif

    connect(tableView, &QTableView::customContextMenuRequested, this, &LC_UCSListWidget::onCustomContextMenu);
    connect(tableView, &QTableView::clicked, this, &LC_UCSListWidget::slotTableClicked);
    connect(tableView, &QTableView::doubleClicked, this, &LC_UCSListWidget::onTableDoubleClicked);

    connect( tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &LC_UCSListWidget::onTableSelectionChanged);

    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void LC_UCSListWidget::setGraphicView(RS_GraphicView *gv,QMdiSubWindow *w) {
    LC_UCSList *ucsList = nullptr;
    if (currentUCSList != nullptr) {
        currentUCSList->removeListener(this);
    }
    if (graphicView != nullptr){
        disconnect(graphicView, &RS_GraphicView::ucsChanged, this, &LC_UCSListWidget::onViewUCSChanged);
    }
    if (gv != nullptr && gv->getGraphic() != nullptr) {
        RS_Graphic *graphic = gv->getGraphic();
        loadFormats(graphic);
        ucsList = graphic->getUCSList();
        ucsList->addListener(this);
        viewport = gv->getViewPort();
        connect(gv, &RS_GraphicView::ucsChanged, this, &LC_UCSListWidget::onViewUCSChanged);
    }
    else{
        viewport = nullptr;
    }
    graphicView = gv;
    window = w;
    setUCSList(ucsList);
}

void LC_UCSListWidget::loadFormats(RS_Graphic *graphic) {
    linearFormat = graphic->getLinearFormat();
    angleFormat = graphic->getAngleFormat();
    precision = graphic->getLinearPrecision();
    anglePrecision = graphic->getAnglePrecision();
    drawingUnit = graphic->getUnit();
}

void LC_UCSListWidget::setUCSList(LC_UCSList *viewsList) {
    currentUCSList = viewsList;
    updateData(false);
    if (nullptr != viewsList && ucsListModel->count() > 0){
        // fixme - sand - ucs complete
//        selectUCS(currentUCSList->at(0));
    }
}

void LC_UCSListWidget::onViewUCSChanged(LC_UCS *ucs) {
    if (ucs == nullptr){
        return;
    }
    ucsListModel->markActive(ucs);
    ui->tvTable->repaint();
    selectUCS(ucs);
    if (ucsStateWidget != nullptr){
        QIcon typeIcon = getUCSTypeIcon(ucs);
        QString name = ucs->getName();
        if (name.isEmpty()){
            name = tr("<No name>");
        }
        QString info = ucsListModel->getUCSInfo(ucs);
        ucsStateWidget->update(typeIcon, name, info);
    }
    bool isometric = ucs->isIsometric();
    RS2::IsoGridViewType isoType = ucs->getIsoGridViewType();
    if (viewport->isGridIsometric() != isometric || viewport->getIsoViewType() != isoType) {
        QC_ApplicationWindow::getAppWindow()->updateGridViewActions(isometric, isoType);
    }
}

void LC_UCSListWidget::reload() {
    if (graphicView != nullptr) {
        RS_Graphic *graphic = graphicView->getGraphic();
        loadFormats(graphic);
    }
    updateData(true);
}

void LC_UCSListWidget::refresh() {
    updateData(true);
}

void LC_UCSListWidget::updateData(bool restoreSelectionIfPossible) {
    int selectedRow = getSingleSelectedRow();
    ucsListModel->setUCSList(currentUCSList, linearFormat, angleFormat, precision, anglePrecision, drawingUnit);
    restoreSingleSelectedRow(restoreSelectionIfPossible, selectedRow);
    updateButtonsState();
    if (options->showColumnTypeIcon){
        ui->tvTable->setColumnWidth(ucsListModel->translateColumn(LC_UCSListModel::ICON_TYPE), ICON_WIDTH);
    }
    emit ucsListChanged();
}

void LC_UCSListWidget::restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow) {
    if (restoreSelectionIfPossible && selectedRow > 0){
        int itemsCount = ucsListModel->count();
        if (itemsCount > 0) {
            ui->tvTable->clearSelection();
            if (selectedRow >= itemsCount){
                selectedRow = itemsCount - 1;
            }
            ui->tvTable->selectRow(selectedRow);
        }
    }
}

int LC_UCSListWidget::getSingleSelectedRow() const {
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    qsizetype selectedSize = selectedIndexes.size();
    int selectedRow = -1;
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            auto  selectedIndex = selectedIndexes.at(0);
            selectedRow = selectedIndex.row();
        }
    }
    return selectedRow;
}

void LC_UCSListWidget::invokeOptionsDialog() {
    int selectedRow = getSingleSelectedRow();
    LC_DlgUCSListOptions dlg = LC_DlgUCSListOptions(options, this);
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->save();
        updateData(false);
        restoreSingleSelectedRow(true, selectedRow);
        graphicView->loadSettings();
    }
}

void LC_UCSListWidget::saveCurrentUCS() {
    viewport->extractUCS(); // fixme -sand - ucs - hm... to much logic under the hood of viewport, huh?
}

void LC_UCSListWidget::activateUCS() {
    LC_UCS* ucs = getSelectedUCS();
    if (ucs != nullptr){
        applyUCS(ucs);
    }
}

void LC_UCSListWidget::previewUCS(){
    LC_UCS* ucs = getSelectedUCS();
    if (ucs != nullptr){
        previewExistingUCS(ucs);
    }
}

void LC_UCSListWidget::previewExistingUCS(LC_UCS* ucs){
    graphicView->highlightUCSLocation(ucs);
}

void LC_UCSListWidget::removeAllUCSs() {
    bool remove = false;
    if (currentUCSList->count() > 1) {
        int result = QMessageBox::question(this, tr("Delete All UCS"),
                                           tr("Are you sure to delete ALL UCS?\n Warning: this action can NOT be undone!"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        remove = result == QMessageBox::Yes;

        if (remove){
            currentUCSList->clear();
            refresh();
        }
    }
}

void LC_UCSListWidget::removeUCS() {
    bool remove = false;
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    qsizetype selectedSize = selectedIndexes.size();
    if (selectedSize > 0) {
        if (selectedSize == 1) {
            QModelIndex selectedIndex = selectedIndexes.at(0);
            if (selectedIndex.isValid()) {
                LC_UCS *selectedUCS = ucsListModel->getItemForIndex(selectedIndex);
                if (selectedUCS != nullptr) {
                    if (selectedUCS->isUCS()) { // don't allow to delete wcs
                        if (options->askForDeletionConfirmation) {
                            QString viewName = selectedUCS->getName();
                            int result = QMessageBox::question(this, tr("Delete UCS"),
                                                               tr("Are you sure to delete UCS\n \"%1\"?\n Warning: this action can NOT be undone!").arg(
                                                                   viewName),
                                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                            remove = result == QMessageBox::Yes;
                        } else {
                            remove = true;
                        }
                        if (remove) {
                            removeExistingUCS(selectedUCS);
                            refresh();
                        }
                    }
                }
            }
        }
        else{
            QList<LC_UCS*> ucsToRemove;
            for (int i=0; i < selectedSize; i++){
                const QModelIndex &index = selectedIndexes.at(i);
                if (index.isValid()){
                    auto u  = ucsListModel->getItemForIndex(index);
                    if (u->isUCS()) {
                        ucsToRemove.push_back(u);
                    }
                }
            }

            if (options->askForDeletionConfirmation){
                QString ucsName = "";
                for (auto v: ucsToRemove){
                    ucsName += "\n";
                    ucsName += v->getName();
                }
                int result = QMessageBox::question(this, tr("Delete UCSs"),
                                                   tr("Are you sure to delete UCS %1?\nWarning: this action can NOT be undone!").arg(ucsName),
                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (result == QMessageBox::Yes){
                    for (auto v: ucsToRemove){
                        removeExistingUCS(v);
                    }
                    refresh();
                }
            }
            else{
                for (auto v: ucsToRemove){
                    removeExistingUCS(v);
                }
                refresh();
            }
        }
    }
}

void LC_UCSListWidget::editUCS() {
    LC_UCS* ucs = getSelectedUCS();
    if (ucs != nullptr){
        if (ucs->isUCS()) {
            renameExistingUCS(ucs);
        }
    }
}

void LC_UCSListWidget::setWCS() {
    viewport->applyUCS(currentUCSList->getWCS());
}


void LC_UCSListWidget::renameExistingUCS(LC_UCS *selectedUCS) {
    QString viewName = selectedUCS->getName();
    bool ok;
    bool tryRename = true;
    while (tryRename) {
        QString text = QInputDialog::getText(this, tr("Rename UCS"),
                                             tr("UCS name:"), QLineEdit::Normal,
                                             viewName, &ok);
        if (ok) {
            text = text.trimmed();
            if (text.isEmpty()) {
                tryRename = false;
            } else {
                LC_UCS *existingView = currentUCSList->find(text);
                if (existingView == nullptr) {
                    renameExistingUCS(text, selectedUCS);
                    tryRename = false;
                } else if (existingView != selectedUCS) {
                    QMessageBox::warning(this, tr("Rename UCS"),
                                         tr("UCS with provided name already exists, select another one"),
                                         QMessageBox::Close,
                                         QMessageBox::Close);
                }
            }
        }
        else{
            tryRename = false;
        }
    }
}

void LC_UCSListWidget::loadOptions() {
    if (options == nullptr){
        options = new LC_UCSListOptions();
    }
    options->load();
}

void LC_UCSListWidget::onTableSelectionChanged([[maybe_unused]] const QItemSelection &selected,
                                                      [[maybe_unused]] const QItemSelection &deselected){
    updateButtonsState();
}

void LC_UCSListWidget::onCustomContextMenu([[maybe_unused]] const QPoint &pos){

    if (currentUCSList == nullptr){
        return;
    }
    auto *contextMenu = new QMenu(this);

    using ActionMemberFunc = void (LC_UCSListWidget::*)();
    const auto addActionFunc = [this, &contextMenu](const QString& name, ActionMemberFunc func) {
        contextMenu->addAction(name, this, func);
    };

    QModelIndex index = ui->tvTable->indexAt(pos);
    bool notInPrintPreview = !graphicView->isPrintPreview();
    bool hasUCS = currentUCSList->count() > 1;
    bool hasNoActiveUCS = ucsListModel->getActiveUCS() == nullptr;
    if (index.isValid()){
        int selectedItemsCount = ui->tvTable->selectionModel()->selectedRows().size();
        if (selectedItemsCount > 1){
            if (notInPrintPreview) {
                contextMenu->addAction(createUCSAction);
                contextMenu->addSeparator();
                if (hasNoActiveUCS){
                    addActionFunc(tr("&Save UCS"), &LC_UCSListWidget::saveCurrentUCS);
                }
            }
            addActionFunc(tr("&Update UCS"), &LC_UCSListWidget::saveCurrentUCS);
            addActionFunc(tr("R&emove Selected UCSs"), &LC_UCSListWidget::removeUCS);
        }
        else {
            if (notInPrintPreview) {
                contextMenu->addAction(createUCSAction);
                if (hasNoActiveUCS){
                    addActionFunc(tr("&Save UCS"), &LC_UCSListWidget::saveCurrentUCS);
                }
            }
            LC_UCS *ucsForIndex = ucsListModel->getItemForIndex(index);
            if (ucsForIndex != nullptr){
                if (hasUCS) {
                    contextMenu->addSeparator();
                    addActionFunc(tr("&Apply UCS"), &LC_UCSListWidget::activateUCS);
                    addActionFunc(tr("&Preview UCS"), &LC_UCSListWidget::previewUCS);
                }
                if(ucsForIndex->isUCS()) {
                    addActionFunc(tr("&Rename UCS"), &LC_UCSListWidget::editUCS);
                    addActionFunc(tr("R&emove UCS"), &LC_UCSListWidget::removeUCS);
                }
            }
            if (hasUCS) {
                contextMenu->addSeparator();
                addActionFunc(tr("Remove A&ll UCSs"), &LC_UCSListWidget::removeAllUCSs);
            }
        }
    }
    else{
        // click is not on item
        if (notInPrintPreview) {
            contextMenu->addAction(createUCSAction);
            if (hasNoActiveUCS){
                addActionFunc(tr("&Save UCS"), &LC_UCSListWidget::saveCurrentUCS);
            }
        }
        if (hasUCS){
            if (notInPrintPreview) {
                contextMenu->addSeparator();
            }
            addActionFunc(tr("Remove A&ll UCSs"), &LC_UCSListWidget::removeAllUCSs);
        }
    }
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

void LC_UCSListWidget::slotTableClicked(QModelIndex modelIndex) {
    if (!modelIndex.isValid()) {
        return;
    }
    if (options->restoreViewBySingleClick) {
        LC_UCS *view = ucsListModel->getItemForIndex(modelIndex);
        if (view == nullptr) {
            return;
        } else {
            applyUCS(view);
        }
    }
}

void LC_UCSListWidget::onTableDoubleClicked() {
    LC_UCS *view = getSelectedUCS();
    if (view != nullptr){
        switch (options->doubleClickPolicy){
            case LC_UCSListOptions::DoubleClickPolicy::DO_NOTHING:{
                break;
            }
            case LC_UCSListOptions::DoubleClickPolicy::EDIT_UCS:{
                renameExistingUCS(view);
                break;
            }
            case LC_UCSListOptions::DoubleClickPolicy::APPLY_UCS:{
                applyUCS(view);
                break;
            }
            case LC_UCSListOptions::DoubleClickPolicy::SHOW_MARKER:{
                previewExistingUCS(view);
                break;
            }
            default:
                break;
        }
    }
}

LC_UCS *LC_UCSListWidget::getSelectedUCS() {
    LC_UCS* result = nullptr;
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        if (currentUCSList != nullptr) {
            result = ucsListModel->getItemForIndex(selectedItemIndex);
        }
    }
    return result;
}

QModelIndex LC_UCSListWidget::getSelectedItemIndex(){
    QModelIndex result;
    QModelIndexList selectedIndexes = ui->tvTable->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

void LC_UCSListWidget::removeExistingUCS(LC_UCS *ucs) {
    currentUCSList->remove(ucs);
}

void LC_UCSListWidget::renameExistingUCS(QString newName, LC_UCS *ucs) {
    currentUCSList->rename(ucs, newName);
    refresh();
}


void LC_UCSListWidget::selectUCS(LC_UCS *view) {
    QModelIndex index = ucsListModel->getIndexForUCS(view);
    if (index.isValid()){
        ui->tvTable->clearSelection();
        ui->tvTable->selectRow(index.row());
    }
}

void LC_UCSListWidget::applyUCS(LC_UCS *ucs) {
    viewport->applyUCS(ucs);
}

void LC_UCSListWidget::fillUCSList(QList<LC_UCS *> &list) {
    if (currentUCSList != nullptr){
        ucsListModel->fillUCSsList(list);
    }
}

QIcon LC_UCSListWidget::getUCSTypeIcon(LC_UCS *view) {
    return ucsListModel->getTypeIcon(view);
}

QWidget *LC_UCSListWidget::createSelectionWidget(QAction* createAction, QAction* defaultAction) {
    ucsListButton = new LC_UCSListButton(this);
    ucsListButton->setDefaultAction(defaultAction);
    applyUCSAction = defaultAction;
    createUCSAction = createAction;
    ui->tbAdd->setDefaultAction(createAction);
    return ucsListButton;
}

QModelIndex LC_UCSListWidget::getIndexForUCS(LC_UCS *u) {
    return ucsListModel->getIndexForUCS(u);
}

void LC_UCSListWidget::applyUCSByIndex(QModelIndex index) {
    if (index.isValid()){
        LC_UCS* ucs = ucsListModel->getItemForIndex(index);
        if (ucs != nullptr){
            applyUCS(ucs);
        }
    }
}

LC_UCS *LC_UCSListWidget::getActiveUCS() {
    return ucsListModel->getActiveUCS();
}

void LC_UCSListWidget::setStateWidget(LC_UCSStateWidget *stateWidget) {
    ucsStateWidget = stateWidget;
}

void LC_UCSListWidget::updateWidgetSettings(){
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
