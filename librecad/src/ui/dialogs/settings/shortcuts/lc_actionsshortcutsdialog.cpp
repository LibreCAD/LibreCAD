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

#include "lc_actionsshortcutsdialog.h"

#include <QMessageBox>
#include <QScrollBar>

#include "lc_actiongroupmanager.h"
#include "lc_shortcutsstorage.h"
#include "lc_shortcutstreemodel.h"
#include "lc_shortcuttreeitem.h"
#include "ui_lc_actionsshortcutsdialog.h"
#include "lc_widgets_common.h"
#include "rs_settings.h"

// fixme - general application-wise shortcuts dispatching and keyboards support
// todo - complex sequences conflicts? How to ensure that conflicts will not occur if key sequences overlapped partially?

LC_ActionsShortcutsDialog::LC_ActionsShortcutsDialog(
    QWidget *parent, LC_ActionGroupManager *pManager)
    : LC_Dialog(parent, "Shortcuts")
    , ui(new Ui::LC_ActionsShortcutsDialog)
    ,m_actionGroupManager(pManager){
    ui->setupUi(this);
    createMappingModel();
    initTreeView();

    connect(ui->btnRecord, &LC_ShortcutButton::keySequenceChanged,this, &LC_ActionsShortcutsDialog::onKeySequenceChanged);
    connect(ui->btnRecord, &QPushButton::toggled,this, &LC_ActionsShortcutsDialog::onRecordButtonToggled);
    connect(ui->leFilter, &QLineEdit::textChanged, this, &LC_ActionsShortcutsDialog::onFilteringMaskChanged);
    connect(ui->cbMatchHighlight, &QCheckBox::clicked, this, &LC_ActionsShortcutsDialog::onFilteringMaskChanged);
    connect( ui->tvMappingsTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LC_ActionsShortcutsDialog::onTreeViewSelectionChanged);

    connect(ui->tvMappingsTree, &QTreeView::clicked, this, &LC_ActionsShortcutsDialog::onTreeClicked);
    connect(ui->tvMappingsTree, &QTreeView::doubleClicked, this, &LC_ActionsShortcutsDialog::onTreeDoubleClicked);
    connect(ui->btnReset, &QPushButton::clicked, this, &LC_ActionsShortcutsDialog::onResetCurrentItemClicked);
    connect(ui->btnResetAll, &QPushButton::clicked, this, &LC_ActionsShortcutsDialog::onResetAllClicked);
    connect(ui->pbClear, &QPushButton::clicked, this, &LC_ActionsShortcutsDialog::onClearClicked);
    connect(ui->pbImport, &QPushButton::clicked, this, &LC_ActionsShortcutsDialog::onImportClicked);
    connect(ui->pbExport, &QPushButton::clicked, this, &LC_ActionsShortcutsDialog::onExportClicked);


    ui->lblMessage->setTextFormat(Qt::RichText);
    QPalette palette = ui->lblMessage->palette();
    palette.setColor(QPalette::Active, QPalette::WindowText, Qt::red);
    ui->lblMessage->setPalette(palette);
    connect(ui->lblMessage, &QLabel::linkActivated, this, &LC_ActionsShortcutsDialog::showConflicts);
    ui->leFilter->setFocus();
    ui->gbShortcut->setVisible(false);
}

LC_ActionsShortcutsDialog::~LC_ActionsShortcutsDialog(){
    delete ui;
}

void LC_ActionsShortcutsDialog::initTreeView(){
    LC_ShortcutsTreeView* treeView = ui->tvMappingsTree;

    treeView->setup(m_mappingTreeModel);
    QHeaderView *pTreeHeader{treeView->header()};
    pTreeHeader->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    pTreeHeader->setStretchLastSection(true);
    pTreeHeader->hide();
    treeView->setUniformRowHeights(true);
    treeView->setAlternatingRowColors(false);
    treeView->setSelectionMode(QAbstractItemView::NoSelection);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setFocusPolicy(Qt::NoFocus);
    treeView->setMinimumHeight(180);

    treeView->setDragDropMode(QAbstractItemView::NoDragDrop);
    treeView->setDragEnabled(false);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setAcceptDrops(false);
    treeView->setDropIndicatorShown(false);
    treeView->setExpandsOnDoubleClick(true);

#ifndef DONT_FORCE_WIDGETS_CSS
    treeView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif
    // todo - do we need context menu there?
    // treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    // connect(treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(QPoint)));
    treeView->expandAll();
}

void LC_ActionsShortcutsDialog::createMappingModel()  {
    m_mappingTreeModel = new LC_ShortcutsTreeModel(this);
    m_mappingTreeModel->rebuildModel(m_actionGroupManager);
}

void LC_ActionsShortcutsDialog::onTreeViewSelectionChanged(const QItemSelection &selected, [[maybe_unused]] const QItemSelection &deselected){
    const QModelIndexList &indexesList = selected.indexes();
    if (!indexesList.isEmpty()){
        const QModelIndex &firstIndex = indexesList.first();
        doSelectItem(firstIndex);
    }
}

void LC_ActionsShortcutsDialog::onResetCurrentItemClicked(){
   if (m_currentItem != nullptr){
       if (ui->btnRecord->isChecked()){
           ui->leKeySequence->setText(m_currentItem->getShortcutViewString());
       }
       else {
           m_currentItem->resetShortcutToDefault();
           ui->leKeySequence->setText(m_currentItem->getShortcutViewString());
           checkHasCollisions(m_currentItem->getShortcutInfo());
           rebuildModel(true);
       }
   }
};

void LC_ActionsShortcutsDialog::onClearClicked() {
    if (m_currentItem != nullptr){
        m_currentItem->clearShortcut();
        ui->leKeySequence->setText("");
        m_editingKeySequence = QKeySequence();
        checkHasCollisions(m_currentItem->getShortcutInfo());
        rebuildModel(true);
    }
}

void LC_ActionsShortcutsDialog::onImportClicked() {
    QFileDialog::Options options = getFileDialogOptions();

    const QString &dir = m_actionGroupManager->getShortcutsMappingsFolder();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file to save mapping"), dir, "LibreCAD Shortcuts (*.lcs)", nullptr,options);

    QMap<QString, QKeySequence> shortcutsMap;
    int loadResult = m_actionGroupManager->loadShortcuts(fileName, &shortcutsMap);

    reportLoadResult(loadResult);

    if (loadResult == LC_ShortcutsStorage::OK){
        // todo - what is more convenient? replace completely by imported or merge existing and imported shortcuts? Provide confirmation policy dialog for this?
        m_mappingTreeModel->applyShortcuts(shortcutsMap, false);
        rebuildModel(false);
    }
    shortcutsMap.clear();
}

void LC_ActionsShortcutsDialog::onExportClicked() {
    QFileDialog::Options options = getFileDialogOptions();

    const QString &dir = m_actionGroupManager->getShortcutsMappingsFolder();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select file to save mapping"), dir, "LibreCAD Shortcuts (*.lcs)", nullptr, options);

    QList<LC_ShortcutInfo*> shortcutsList;
    m_mappingTreeModel->collectShortcuts(shortcutsList);

    int saveResult = m_actionGroupManager->saveShortcuts(shortcutsList, fileName);
    shortcutsList.clear();
    reportSaveResult(saveResult);
}

void LC_ActionsShortcutsDialog::reportLoadResult(int loadResult) const {
    switch (loadResult){
        case LC_ShortcutsStorage::OK:{
            showIOInfoDialog(true, true, tr("Shortcuts mappings were loaded successfully."));
            break;
        }
        case LC_ShortcutsStorage::ERROR_FILE:{
            showIOInfoDialog(true, false, tr("Can't read provided file - please check that it is exists and it is allowed to read from it."));
            break;
        }
        case LC_ShortcutsStorage::ERROR_WRONG_DTD:{
            showIOInfoDialog(true, false, tr("Shortcuts mappings were not imported.\n\n "
                                             "Provided file does exists, however it does not contain LibreCAD shortcuts mapping."));
            break;
        }
        case LC_ShortcutsStorage::ERROR_PARSE:{
            showIOInfoDialog(true, false, tr("Shortcuts mappings were not imported. \n\n"
                                             "XML parsing error occurred during file processing."));
            break;
        }
        default:
            break;
    }
}

void LC_ActionsShortcutsDialog::reportSaveResult(int saveResult) const {
    switch (saveResult){
        case LC_ShortcutsStorage::OK:{
            showIOInfoDialog(true, true, tr("Shortcuts mappings were saved successfully."));
            break;
        }
        case LC_ShortcutsStorage::ERROR_FILE:{
            showIOInfoDialog(true, false, tr("Can't write to provided file - please check that it is allowed to write to it."));
            break;
        }
        case LC_ShortcutsStorage::ERROR_GENERATE:{
            showIOInfoDialog(true, false, tr("Shortcuts mappings were not imported. \n\n"
                                             "Some error occurred during XML generation."));
            break;
        }
        default:
            break;
    }
}

void LC_ActionsShortcutsDialog::onResetAllClicked() {
    m_mappingTreeModel->resetAllToDefault();
    m_mappingTreeModel->setFilterForConflicts(false);
    rebuildModel(false);
}

void LC_ActionsShortcutsDialog::onTreeDoubleClicked(QModelIndex index ){
    if (index.isValid()){
        LC_ShortcutTreeItem *item = m_mappingTreeModel->getItemForIndex(index);
        const QModelIndex &parentIndex = m_mappingTreeModel->parent(index);
        selectItem(item, index.row(), parentIndex.row());
        if (item->isGroup()){ //  expand children if group
            ui->tvMappingsTree -> expandRecursively(index,1);
        } else {
            // for normal mapping - we'll invoke editing
            editItem(item);
        }
    }
}

void LC_ActionsShortcutsDialog::onTreeClicked(QModelIndex itemIndex ){
    if (itemIndex.isValid()) {
        doSelectItem(itemIndex);
    }
}

void LC_ActionsShortcutsDialog::doSelectItem(const QModelIndex &itemIndex) {
    LC_ShortcutTreeItem *layerItem = m_mappingTreeModel->getItemForIndex(itemIndex);
    const QModelIndex &parentIndex = m_mappingTreeModel->parent(itemIndex);
    selectItem(layerItem, itemIndex.row(), parentIndex.row());
}

void LC_ActionsShortcutsDialog::onFilteringMaskChanged(){
    QString mask = ui->leFilter->text();
    bool highlightMode = ui->cbMatchHighlight->isChecked();
    m_mappingTreeModel->setFilteringRegexp(mask, highlightMode);
    rebuildModel(false);
}

void LC_ActionsShortcutsDialog::onRecordButtonToggled(bool on) {
   if (!on){ // end recording
       applyRecordedKeySequence();
   }
}

void LC_ActionsShortcutsDialog::showIOInfoDialog(bool forImport,bool ok, const QString &message){
    QString title = forImport ? tr("Import Shortcuts Mapping") : tr("Export Shortcuts Mapping");
    QMessageBox msgBox(ok? QMessageBox::Information : QMessageBox::Warning, title, message, QMessageBox::Ok);
    msgBox.exec();
}

QFileDialog::Options LC_ActionsShortcutsDialog::getFileDialogOptions() {
    bool useQtFileDialog = LC_GET_ONE_BOOL("Defaults", "UseQtFileOpenDialog");
    QFileDialog::Options options;
    options.setFlag(QFileDialog::DontUseNativeDialog, useQtFileDialog);
    return options;
}

void LC_ActionsShortcutsDialog::selectItem(LC_ShortcutTreeItem *item, int row, int parentRow) {
    if (item == nullptr || item->isGroup()){
        ui->gbShortcut->setVisible(false);
        m_currentItem = nullptr;
        m_selectedRow = -1;
        m_selectedParentRow = -1;
    }
    else{
        ui->gbShortcut->setVisible(true);
        ui->lblActionName->setText(item->getName());
        ui->lblGroupName->setText(item->parent()->getName());
        ui->lblActionIcon->setText("");
        int height = ui->lblActionName->height();
        ui->lblActionIcon->setPixmap(item->getIcon().pixmap(height,height));
        ui->leKeySequence->setText(item->getShortcutViewString());
        m_currentItem = item;
        m_selectedRow = row;
        m_selectedParentRow = parentRow;
    }
}

void LC_ActionsShortcutsDialog::editItem([[maybe_unused]]LC_ShortcutTreeItem *item) {
    ui->btnRecord->setChecked(true);
    ui->btnRecord->setFocus();
}

void LC_ActionsShortcutsDialog::rebuildModel(bool restoreSelection) {
    LC_ShortcutsTreeView* treeView = ui->tvMappingsTree;
    QScrollBar *verticalScrollBar = treeView->verticalScrollBar();
    int yPos = verticalScrollBar->value();

//    QStringList treeExpansionState = treeView->saveTreeExpansionState();
    m_mappingTreeModel->rebuildModel(m_actionGroupManager);
//        treeView->restoreTreeExpansionState(treeExpansionState);
//    }
    if (restoreSelection){
        QItemSelectionModel *selectionModel = treeView->selectionModel();
        const QModelIndex &parentIndex = m_mappingTreeModel->index(m_selectedParentRow, 0, QModelIndex());
        if (parentIndex.isValid()){
            const QModelIndex &index = m_mappingTreeModel->index(m_selectedRow, 0, parentIndex);
            selectionModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        else{
            selectItem(nullptr, -1, -1);
        }

    }
    else{
        selectItem(nullptr, -1, -1);
    }    
    treeView->expandAll();
    verticalScrollBar->setValue(yPos);
    treeView->viewport()->update();
}

void LC_ActionsShortcutsDialog::applyRecordedKeySequence(){
    if (m_currentItem != nullptr){
        LC_ShortcutInfo *shortcutInfo = m_currentItem->getShortcutInfo();
        shortcutInfo->setKey(m_editingKeySequence);
        if (shortcutInfo->isModified()){
            checkHasCollisions(shortcutInfo);
            rebuildModel(true);
        }
    }
    m_editingKeySequence = QKeySequence();
}

void LC_ActionsShortcutsDialog::onKeySequenceChanged(const QKeySequence &key) {
    if (keySequenceIsValid(key)) {
        ui->leKeySequence->setText(keySequenceToEditString(key));
        m_editingKeySequence = key;
    }
    else{
        ui->lblMessage->setText(tr("Invalid key sequence."));
    }
}

bool LC_ActionsShortcutsDialog::checkHasCollisions(LC_ShortcutInfo *shortcutInfo) {
    bool hasCollisions = m_mappingTreeModel-> checkForCollisions(shortcutInfo);
    if (hasCollisions){
        QString msg;
        if (shortcutInfo == nullptr){
            msg = tr("Resolve conflicts before saving.");
            m_mappingTreeModel->setFilterForConflicts(true);
        }
        else{
            msg = tr("Key sequence has potential conflicts. <a href=\"#conflicts\">Show.</a>");
        }
        ui->lblMessage->setText(msg);
    }
    else{
        m_mappingTreeModel->setFilterForConflicts(false);
        ui->lblMessage->setText("");
    }
    return hasCollisions;
}

void LC_ActionsShortcutsDialog::showConflicts(){
    m_mappingTreeModel->setFilterForConflicts(true);
    rebuildModel(false);
}

bool LC_ActionsShortcutsDialog::keySequenceIsValid(const QKeySequence &sequence) const {
    if (sequence.isEmpty()) {
        return false;
    }
    for (int i = 0; i < sequence.count(); ++i) {
        QKeyCombination keyCombination = sequence[i];
        if (keyCombination.toCombined() == Qt::Key_unknown)
            return false;
    }
    return true;
}

QString LC_ActionsShortcutsDialog::keySequenceToEditString(const QKeySequence &sequence) const {
    QString text = sequence.toString(QKeySequence::PortableText);
    return text;
}

void LC_ActionsShortcutsDialog::accept() {
    if (checkHasCollisions(nullptr)) {
        rebuildModel(false);
    }
    else{
        if (m_mappingTreeModel->isModified()) {
            QMap<QString, LC_ShortcutInfo *> shortcuts = m_mappingTreeModel->getShortcuts();
            int saveResult = m_actionGroupManager->saveShortcuts(shortcuts);
            // might be ugly to show OK result, as it is for default settings... think and review later
            if (saveResult != LC_ShortcutsStorage::OK) {
                reportSaveResult(saveResult);
            }
        }
        LC_Dialog::accept();
    }
}

void LC_ActionsShortcutsDialog::reject() {
    bool canClose = true;
    if (m_mappingTreeModel->isModified()){
        canClose = QMessageBox::Yes == QMessageBox::question(this, "Confirmation",
                                                             tr("Some mappings are modified.\nAre you sure you are going to discard changes?"),
                                                      QMessageBox::Yes | QMessageBox::No);
    }
    if (canClose) {
        LC_Dialog::reject();
    }
}
