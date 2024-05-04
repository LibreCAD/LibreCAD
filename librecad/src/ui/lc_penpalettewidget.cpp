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

#include <QCheckBox>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QScrollBar>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QTimer>

#include "lc_peninforegistry.h"
#include "lc_penitem.h"
#include "lc_penpalettemodel.h"
#include "lc_penpaletteoptionsdialog.h"
#include "lc_penpalettewidget.h"
#include "qc_applicationwindow.h"
#include "qg_graphicview.h"
#include "qg_pentoolbar.h"
#include "rs_graphic.h"
#include "rs_modification.h"
#include "ui_lc_penpalettewidget.h"
#include "lc_flexlayout.h"

/**
 * Delegate used to paint underline lines for table grid
 */
class LC_PenPaletteGridDelegate:public QStyledItemDelegate {
public:
    explicit LC_PenPaletteGridDelegate(QTableView *parent = nullptr, LC_PenPaletteOptions* options = nullptr):QStyledItemDelegate(parent){
        this->options = options;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override{
        QStyledItemDelegate::paint(painter, option, index);
        bool draw = options != nullptr;
        if (draw){
            QColor color = options->itemsGridColor;
            painter->save();
            painter->setPen(color);
            painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
            painter->restore();
        }
    }

private:
    LC_PenPaletteOptions* options {nullptr};
};

/**
 * Overall initialization
 * @param title
 * @param parent
 */
LC_PenPaletteWidget::LC_PenPaletteWidget(const QString& title, QWidget* parent) :
    QWidget(parent),
    Ui::LC_PenPaletteWidget(){

    setupUi(this);

    // make buttons flexible
    auto *layButtonsFlex = new LC_FlexLayout(2, 6, 6);
    layButtonsFlex->fillFromLayout(layButtons);
    int buttonsPosition = gridLayout->indexOf(layButtons);
    QLayoutItem *pItem = gridLayout->takeAt(buttonsPosition);
    delete pItem;

    int settingsWidgetPosition = gridLayout->indexOf(tbSettings);
    QLayoutItem *pLayoutItem = gridLayout->takeAt(settingsWidgetPosition);
    delete pLayoutItem;

    gridLayout->addLayout(layButtonsFlex, 0, 0, 1, 1);
    gridLayout->addWidget(tbSettings, 0,1,1,1);
    gridLayout->setAlignment(tbSettings,Qt::AlignTop);

    // make controls flexible

    auto *layPenColorFlex = new LC_FlexLayout(2, 6, 6, 45);
    layPenColorFlex->fillFromLayout(layPenColor);
    layPenColorFlex->fillFromLayout(layTypeWidth);
    layPenColorFlex->setSoftBreakItems({2, 4, 6});
    layPenColorFlex->setFullWidthItems({1});
    gridLayout->addLayout(layPenColorFlex,4,0,1, 2);

    setWindowTitle(title);

    // load generic options
    auto options = new LC_PenPaletteOptions();
    options->loadFromSettings();

    // load pens data from storage
    penPaletteData = new LC_PenPaletteData(options);
    bool itemsLoaded = penPaletteData->loadItems();
    if (!itemsLoaded){
        // todo... potentially, it is possible to show dialog there - yet probably it's better stay with default pens..
    }

    penPaletteModel = new LC_PenPaletteModel(options, penPaletteData);

    initPenEditor();
    initFilteringSection();
    initTableView();
    initToolBar();

    // set first pen item
    if (penPaletteModel->rowCount(QModelIndex())>0){
        LC_PenItem* item = penPaletteModel->getPen(0);
        penPaletteModel->setActivePen(item);
    }
    else{
        tbRemove->setEnabled(false);
    }
}

/**
 * Just initialization of button handlers
 */
void LC_PenPaletteWidget::initToolBar() const{
    connect(tbEditApplyToSelection, &QToolButton::clicked, this, &LC_PenPaletteWidget::applyEditorPenToSelection);
    connect(tbEditSelectEntities, &QToolButton::clicked, this, &LC_PenPaletteWidget::selectEntitiesWithAttributesPenByPenEditor);
    connect(tbEditSelectEntitiesByResolvedPen, &QToolButton::clicked, this, &LC_PenPaletteWidget::selectEntitiesWithDrawingPenByPenEditor);
    connect(tbEditSelectFromEntity, &QToolButton::clicked, this, &LC_PenPaletteWidget::fillPenEditorBySelectedEntityAttributesPen);
    connect(tbEditSelectResolvedFromEntity, &QToolButton::clicked, this, &LC_PenPaletteWidget::fillPenEditorBySelectedEntityDrawingPen);
    connect(tbEditApplyFromCurrent, &QToolButton::clicked, this, &LC_PenPaletteWidget::fillPenEditorByPenToolBarPen);
    connect(tbSetAsCurrent, &QToolButton::clicked, this, &LC_PenPaletteWidget::applyEditorPenToPenToolBar);
    connect(tbEditSave, &QToolButton::clicked, this, &LC_PenPaletteWidget::createOrUpdatePenItem);

    connect(tbRemove, &QToolButton::clicked, this, &LC_PenPaletteWidget::removeActivePenItem);
    connect(tbPickFromActiveLayer, &QToolButton::clicked, this, &LC_PenPaletteWidget::fillPenEditorByActiveLayer);
    connect(tbApplyToActiveLayer, &QToolButton::clicked, this, &LC_PenPaletteWidget::applyEditorPenToActiveLayer);
    connect(tbUpdateCurrentPenByActiveLayer, &QToolButton::clicked, this, &LC_PenPaletteWidget::updatePenToolbarByActiveLayer);
    connect(tbSettings, &QToolButton::clicked, this, &LC_PenPaletteWidget::invokeOptionsDialog);
}

/**
 * setup of table view
 */
void LC_PenPaletteWidget::initTableView(){
    tableView->setModel(penPaletteModel);

    QHeaderView *horizontalHeader = tableView->horizontalHeader();
    horizontalHeader->setMinimumSectionSize(24);
    horizontalHeader->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->hide();

    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setFocusPolicy(Qt::NoFocus);
    tableView->setMinimumHeight(140);    
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QHeaderView *verticalHeader = tableView->verticalHeader();
    verticalHeader->setOffset(2);
    verticalHeader->hide();

    tableView->setColumnWidth(penPaletteModel ->translateColumn(LC_PenPaletteModel::COLOR_ICON), LC_PenPaletteModel::ICON_WIDTH);
    tableView->setColumnWidth(penPaletteModel ->translateColumn(LC_PenPaletteModel::COLOR_NAME), LC_PenPaletteModel::ICON_WIDTH * 5);
    tableView->setColumnWidth(penPaletteModel ->translateColumn(LC_PenPaletteModel::TYPE_ICON), LC_PenPaletteModel::ICON_WIDTH);
    tableView->setColumnWidth(penPaletteModel ->translateColumn(LC_PenPaletteModel::WIDTH_ICON), LC_PenPaletteModel::ICON_WIDTH);

    connect(tableView, &QTableView::clicked, this, &LC_PenPaletteWidget::onTableClicked);
    connect(tableView, &QTableView::customContextMenuRequested, this, &LC_PenPaletteWidget::onTableViewContextMenuInvoked);
    connect( tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &LC_PenPaletteWidget::onTableSelectionChanged);


    connect(penPaletteModel, &LC_PenPaletteModel::modelChange, this, &LC_PenPaletteWidget::onModelChanged);
    connect(penPaletteData, &LC_PenPaletteData::modelDataChange, this, &LC_PenPaletteWidget::onPersistentItemsChanged);

    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    tableView->setItemDelegate(new LC_PenPaletteGridDelegate(tableView, penPaletteModel->getOptions()));
}

/**
 * Handler for changes of persistent model (pen data).
 * Here save of pen data model occurs - and if file may not be opened for save, the user is prompted to change the option for correct file location.
 * If the user decide not to change options with file - file will not be saved, and this logic will be involved on next modification of model.
 * If there will not be modifications and the user will exist the application - well, all not saved changes will be lost.
 *
 */
void LC_PenPaletteWidget::onPersistentItemsChanged(){
    bool itemsSaved = penPaletteData ->saveItems();
    while (!itemsSaved){
        bool showOptions = invokeUnableToSavePenDataDialog();
        if (showOptions){
            invokeOptionsDialog(true);
            itemsSaved = penPaletteData ->saveItems();
        }
        else{ // user skipped options, so we still cant' save data - yet that's to user. Will ask next time, however
            itemsSaved = true;
        }
    }
}

/**
 * setup of editor area
 */
void LC_PenPaletteWidget::initPenEditor(){
    cbColor->init(true, true);
    cbWidth->init(true, true);
    cbType->init(true, true);
    connect(lePenName, &QLineEdit::textChanged, this, &LC_PenPaletteWidget::onPenEditorChanged);
    connect(lePenName, &QLineEdit::returnPressed, this, &LC_PenPaletteWidget::createOrUpdatePenItem);
    connect(cbColor, SIGNAL(currentIndexChanged(int)),this, SLOT(onPenEditorColorChanged(int)));
    connect(cbWidth, SIGNAL(currentIndexChanged(int)),this, SLOT(onPenEditorWidthChanged(int)));
    connect(cbType, SIGNAL(currentIndexChanged(int)),this, SLOT(onPenEditorLineTypeChanged(int)));
}
/**
 * Filtering section initialization
 */
void LC_PenPaletteWidget::initFilteringSection(){
    // restore mode for filter
    cbHighlightMode->setChecked(penPaletteModel->getOptions()->filterIsInHighlightMode);
    // add handlers
    connect(leFilterMask, &QLineEdit::textChanged, this, &LC_PenPaletteWidget::filterMaskChanged);
    connect(cbHighlightMode, &QCheckBox::clicked, this, &LC_PenPaletteWidget::filterMaskChanged);
}


/**
 * Context menu for table view
 * @param pos
 */
void LC_PenPaletteWidget::onTableViewContextMenuInvoked([[maybe_unused]] const QPoint &pos){
    int itemsCount = penPaletteModel->rowCount(QModelIndex());
    int selectedItemsCount = tableView->selectionModel()->selectedRows().size();
    if (itemsCount >0 && selectedItemsCount > 0){
        auto contextMenu = std::make_unique<QMenu>(this);
        QLabel *caption = new QLabel(tr("Pens Menu"), this);
        QPalette palette;
        palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
        palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
        caption->setPalette(palette);
        caption->setAlignment(Qt::AlignCenter);
        typedef void (LC_PenPaletteWidget::*MemFn)();
        auto addAction = [&contextMenu, this](const std::pair<QString, MemFn>& item) {
            auto* action = contextMenu->addAction(item.first);
            connect(action, &QAction::triggered, this, item.second);
// #if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
//             connect(action, &QAction::triggered, this, time.second);
// #else
//             connect(action, SIGNAL(triggered()), this, time.second);
// #endif
        };
        auto addActions = [&addAction](std::initializer_list<std::pair<QString, MemFn>> menuEntries){
            for (const auto& menuEntry: menuEntries)
                addAction(menuEntry);
        };

        if (selectedItemsCount == 1){
            addActions({ {tr("&Apply Pen To Selection"), &LC_PenPaletteWidget::applySelectedPenToSelection},
                        {tr("&Set As Current Pen"), &LC_PenPaletteWidget::applySelectedPenItemToPenToolBar},
                        {tr("&Apply Pen To Active Layer"), &LC_PenPaletteWidget::applySelectedPenItemToActiveLayer},
                        {tr("&Select Entities With Attributes Pen"), &LC_PenPaletteWidget::selectEntitiesWithAttributesPenBySelectedPenItem},
                        {tr("&Select Entities With Drawing Pen"), &LC_PenPaletteWidget::selectEntitiesWithDrawingPenBySelectedPenItem}});

            contextMenu->addSeparator();
            addActions({{tr("&Edit Pen"), &LC_PenPaletteWidget::editSelectedPenItem},
                        {tr("&Remove Pen"), &LC_PenPaletteWidget::removeSelectedPenItem}});
        }
        else{ // for multiselect - only rename
            addAction({tr("&Remove Pens"), &LC_PenPaletteWidget::removeSelectedPenItems});
        }
        contextMenu->exec(QCursor::pos());
    }
}

/**
 * Handles changes in the model to properly update remove button
 */
void LC_PenPaletteWidget::onModelChanged(){
    int count = penPaletteModel->rowCount(QModelIndex());
    bool hasActivePen  = penPaletteModel->getActivePen() != nullptr;
    tbRemove->setEnabled((count > 0) && hasActivePen);
}

/**
 * Setter for current window
 * @param mdiWindow
 */
void LC_PenPaletteWidget::setMdiWindow(QC_MDIWindow* mdiWindow){
    mdi_win = mdiWindow;
}

/**
 * Handler for table item activation
 * @param modelIndex
 */
void LC_PenPaletteWidget::onTableClicked(QModelIndex modelIndex){
    if (!modelIndex.isValid()) {
        return;
    }

    // increase clicks count. Double click should be processed additionally, as QT does not work well if both clicked and doubleClicked
    // slots are used for table view.  So on first click - we'll activate item, and start the time for potential double click processing
    clicksCount ++;

    if (clicksCount ==1){

        // if we're in first call, we'll activate clicked row and pen item

        LC_PenItem* pen = penPaletteModel->getPen(modelIndex.row());
        if (pen == nullptr)
            return;

        // just make pen active and put it into editor
        penPaletteModel->setActivePen(pen);
        fillPenEditorByPenItem(pen);

        // invoke double click processing via one-shot timer. If double click will occur - we'll handle this on timer's call
        QStyleHints *styleHints = QGuiApplication::styleHints();
        int interval = styleHints->mouseDoubleClickInterval();
        QTimer::singleShot(interval, this, &LC_PenPaletteWidget::doDoubleClick);
    }

}

void LC_PenPaletteWidget::doDoubleClick(){
    if (clicksCount == 2){
        // that's actual double click, call appropriate method
        onTableRowDoubleClicked();
    }
    // hm... it seems that it was single click where timer was fired.. so clear the counter
    clicksCount = 0;
}

/**
 * handler for double click on table view item
 * @param index
 */
void LC_PenPaletteWidget::onTableRowDoubleClicked(){
    // execute command specified by option
    switch (penPaletteModel->getOptions()->doubleClickOnTableMode){
        case LC_PenPaletteOptions::DOUBLE_CLICK_DOES_NOTHING:
            break;
        case LC_PenPaletteOptions::DOUBLE_CLICK_SELECT_ENTITIES_BY_ATTRIBUTES_PEN:
            selectEntitiesWithAttributesPenBySelectedPenItem();
            break;
        case LC_PenPaletteOptions::DOUBLE_CLICK_SELECT_ENTITIES_BY_DRAWING_PEN:
            selectEntitiesWithAttributesPenBySelectedPenItem();
            break;
    }
};

/**
 * Handler for table selection change
 * @param selected
 * @param deselected
 */
void LC_PenPaletteWidget::onTableSelectionChanged(
    const QItemSelection &selected,
        const QItemSelection &deselected){
    QModelIndex index;
    QItemSelectionModel *selectionModel {tableView->selectionModel()};

        foreach (index, selected.indexes()) {
              selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Select);
        }

        foreach (index, deselected.indexes()) {
                selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Deselect);
        }
}

/**
 * Displays options dialogs and applies changes, if necessary
 */
void LC_PenPaletteWidget::invokeOptionsDialog(bool focusOnFile){
    LC_PenPaletteOptions * options = penPaletteModel->getOptions();
    LC_PenPaletteOptionsDialog dlg = LC_PenPaletteOptionsDialog(this, options, focusOnFile);
    QString oldFileName = options->pensFileName;
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->saveToSettings();
        penPaletteModel->update(true);
        update();
        if (!focusOnFile){ // this is normal invocation via settings button
            QString newFileName = options->pensFileName;
            if (oldFileName != newFileName){
                // next time, we'll try to read pens from new file. It's safer to do this rather
                // than save pen items to new file (yet this is still may be possible if the user will
                // change pens data and so save attempt will be performed to new file...
                // potentially, it is possible just to save new file in settings and use current file util restart
                // yet this is also quite a messy way... So let's ask for restart.
                QMessageBox::warning( this, tr("Pen palette"),
                                      tr("Location of pens file is changed, please restart the application so new pens file will be used.\n\n"
                                         "Please note that if you'll save pen via editor without restart, current pens from palette will be saved "
                                         "in the new file and therefore existing content of it will be overridden."),
                                      QMessageBox::Ok,
                                      Qt::NoButton);
            }
        }
    }
}

/**
 * Creates new pen item or updates existing one (based on pen's name).
 * If name was not changed - pen will be updated, otherwise - new one will be created.
 */
void LC_PenPaletteWidget::createOrUpdatePenItem(){
    QString penName = lePenName->text();
    if (!penName.isEmpty()){
        QString actualPenName = penName.trimmed();

        // try to find the pen with such name
        LC_PenItem* penItem = penPaletteModel -> findPenForName(actualPenName);
        bool existingPen = true;
        if (penItem == nullptr){
            // no pen name found, need to create
            penItem = penPaletteModel->createNewItem(penName);
            existingPen = false;
        }

        RS2::LineType lineType = RS2::LineType::LineTypeUnchanged;
        int lineTypeIndex = cbType->currentIndex();
        if (lineTypeIndex > 0){ // 0 is unchanged
            lineType = cbType->getLineType();
        }
        penItem->setLineType(lineType);

        RS2::LineWidth width = RS2::LineWidth::WidthUnchanged;
        int widthIndex = cbWidth -> currentIndex();
        if (widthIndex > 0){ // 0 is unchanged
            width = cbWidth -> getWidth();
        }

        penItem->setLineWidth(width);

        RS_Color color = LC_PenInfoRegistry::createUnchangedColor();

        int colorIndex = cbColor -> currentIndex();
        if (colorIndex > 0){
            color = cbColor->getColor();
        }

        penItem -> setColor(color);

        // store pen in model
        if (existingPen){
            penPaletteModel-> itemEdited(penItem);
        }
        else{
            penPaletteModel -> addItem(penItem);
        }

        // cleanup pen editor and make it unchanged
        markEditingPenChanged(false);
    }
}
/**
 * Applies pen item that is currently selected in the table to selected entities in drawing
 */
void LC_PenPaletteWidget::applySelectedPenToSelection(){
    LC_PenItem* item = getSelectedPenItem();
    if (item != nullptr){
        RS2::LineType lineType = item->getLineType();
        RS2::LineWidth width = item->getLineWidth();
        RS_Color color = item->getColor();
        bool colorCheck = !LC_PenInfoRegistry::isUnchangedColor(color);

        doApplyPenAttributesToSelection(lineType, width, color, colorCheck);

        redrawDrawing();
    }
}

/**
 * Applies pen from editor to selected entities in drawing
 */
void LC_PenPaletteWidget::applyEditorPenToSelection(){
    RS2::LineType lineType;
    int lineTypeIndex = cbType->currentIndex();
    lineType = RS2::LineType::LineTypeUnchanged;
    if (lineTypeIndex > 0){ // not unchanged
        lineType = cbType->getLineType();
    }
    RS2::LineWidth width = RS2::LineWidth::WidthUnchanged;
    int widthIndex = cbWidth->currentIndex();
    if (widthIndex > 0){ // 0 is unchanged
        width = cbWidth->getWidth();
    }

    RS_Color color = LC_PenInfoRegistry::createUnchangedColor();
    int colorIndex = cbColor->currentIndex();
    bool colorCheck = false;
    if (colorIndex > 0){ // 0 is unchanged
        color = cbColor->getColor();
        colorCheck = true;
    }

    doApplyPenAttributesToSelection(lineType, width, color, colorCheck);

    redrawDrawing();
}

/**
 * Actual method for applying of pen to selection. Basically, this is really similar to Change Attributes action,
 * pen attributes are applied to selected entities via RS_Modification  (so internally, this is like normal
 * change of attributes)
 * @param lineType
 * @param width
 * @param color
 * @param modifyColor
 */
void LC_PenPaletteWidget::doApplyPenAttributesToSelection(RS2::LineType lineType, RS2::LineWidth width, RS_Color color, bool modifyColor){

    auto graphic = mdi_win->getGraphic();
    QG_GraphicView *graphicView = mdi_win->getGraphicView();
    RS_Document* container = mdi_win->getDocument();

    if (graphic){
        RS_AttributesData data;
        data.pen = RS_Pen(color, width, lineType);
        data.layer = "0";
        data.changeColor = modifyColor;
        data.changeLineType = lineType != RS2::LineTypeUnchanged;
        data.changeWidth = width != RS2::WidthUnchanged;
        data.changeLayer = false;

        RS_Modification m(*container, graphicView);
        m.changeAttributes(data);
    }
}

/**
 * Picks pen from selected entities attributes and fill pen editor by them.
 * Pen is not resolved, so it corresponds pen stored in selected entity
 */
void LC_PenPaletteWidget::fillPenEditorBySelectedEntityAttributesPen(){
    doFillPenEditorBySelectedEntity(false);
}

/**
 * Picks pen from selected entities and fill pen editor by them.
 * Pen is resolved, so it corresponds to the pen that is used for actual drawing of selected entity.
 */
void LC_PenPaletteWidget::fillPenEditorBySelectedEntityDrawingPen(){
    doFillPenEditorBySelectedEntity(true);
}

/**
 * Utility method for filling pen editor by selected entity pen
 * First it checks whether only one entity is selected, an if so - extracts pen's attributes
 * @param resolvePenOnEntitySelect flag that indicates whether resolved pen should be applied to the editor (if true) or just pen from entity's attributes
 */
void LC_PenPaletteWidget::doFillPenEditorBySelectedEntity(bool resolvePenOnEntitySelect){
    // first we collect selected entitites
    QList<RS_Entity *> selectedEntities;
    auto graphic = mdi_win->getGraphic();
    foreach (auto e, graphic->getEntityList()) {
        if (e->isSelected()){
            selectedEntities << e;
        }
    }
    if (selectedEntities.size() != 1){ // only one entity selected is expected
        showEntitySelectionInfoDialog();
    } else {
        RS_Entity *entity = selectedEntities.at(0);

        // retrieve pen to apply
        RS_Pen pen = entity->getPen(resolvePenOnEntitySelect);

        // apply pen to editor controls
        doFillPenEditorByPen(pen);
    }
}

/**
 * Select entities with attributes pen that matched to pen info that is selected in the table
 */
void LC_PenPaletteWidget::selectEntitiesWithAttributesPenBySelectedPenItem(){
    doSelectEntitiesBySelectedPenItem(false, false);
}

/**
 * Select entities with drawing pen that matches pen info that is selected in the table
 */
void LC_PenPaletteWidget::selectEntitiesWithDrawingPenBySelectedPenItem(){
    doSelectEntitiesBySelectedPenItem(true, true);
}

/**
 * Selects entities that matches to pen item that is selected in the table
 * @param resolvePens - flag that indicates that entity's pen should be resolved
 * @param resolveLayers - flat that indicates that entity's layer should be resolved
 */
void LC_PenPaletteWidget::doSelectEntitiesBySelectedPenItem(bool resolvePens, bool resolveLayers){
    // obtain selected item
    LC_PenItem* item = getSelectedPenItem();
    if (item != nullptr){
        // prepare pen attributes
        RS2::LineType lineType = item->getLineType();
        RS2::LineWidth lineWidth = item->getLineWidth();
        RS_Color color = item->getColor();
        bool colorCheck = !LC_PenInfoRegistry::isUnchangedColor(color);

        // do actual selection of entities by pen
        doSelectEntitiesThatMatchToPenAttributes(lineType, lineWidth, color, colorCheck, resolvePens, resolveLayers);
    }
}

/**
 * Selects entities with pen attributes as it is set in editor pen
 */
void LC_PenPaletteWidget::selectEntitiesWithAttributesPenByPenEditor(){
    bool resolvePens = false;
    bool resolveLayers  = false;
    doSelectEntitiesByPenEditor(resolvePens, resolveLayers);
}

/**
 * Selects entities with drawing pen attributes as it is set in editor pen
 */
void LC_PenPaletteWidget::selectEntitiesWithDrawingPenByPenEditor(){
    bool resolvePens = true;
    bool resolveLayers  = true;
    doSelectEntitiesByPenEditor(resolvePens, resolveLayers);
}

/**
 * Common method for selecting entities by pen attributes specified by editor's pen
 * @param resolvePens - true if entity's pen should be resolved, false otherwise
 * @param resolveLayers  - true entity's layer should be resolve, false otherwise
 */
void LC_PenPaletteWidget::doSelectEntitiesByPenEditor(bool resolvePens, bool resolveLayers){
    RS2::LineType lineType;
    int lineTypeIndex = cbType->currentIndex();
    lineType = RS2::LineTypeUnchanged;
    if (lineTypeIndex > 0){ // not unchanged
        lineType = cbType->getLineType();
    }
    RS2::LineWidth width = RS2::WidthUnchanged;
    int widthIndex = cbWidth->currentIndex();
    if (widthIndex > 0){ // not unchanged
        width = cbWidth->getWidth();
    }

    RS_Color color = LC_PenInfoRegistry::createUnchangedColor();
    int colorIndex = cbColor->currentIndex();
    bool colorCheck = false;
    if (colorIndex > 0){ // not unchanged
        color = cbColor->getColor();
        colorCheck = true;
    }

    // do actual selection
    doSelectEntitiesThatMatchToPenAttributes(lineType, width, color, colorCheck, resolvePens, resolveLayers);
}

/**
 * Inner method of selecting entities that matches to given pen attributes.
 * It take cares of "unchanged" values that acts as "any value is suitable" condition for entity selection.
 * Due to this, it is possible have quite a powerful selection functionality and select entities that match the pen only partially - say, entities with
 * specified color only and so on.
 *
 * From the logic point of view, entities are selected using logical AND logic - i.e entities are selected only if their pens matches ALL specified attributes.
 * Without a doubt, it is easy to add also "OR" logic of match there, however, it might be quite confusing for the user...
 *
 * @param lineType - target line type
 * @param width - target line width
 * @param color - target color
 * @param colorCheck - flag that indicates that color should be checked during entity match
 * @param resolvePens - indicates whether entity's pen should be resolved or not for match
 * @param resolveLayers - indicates whether entity's layer should resolved on not for match
 */
void LC_PenPaletteWidget::doSelectEntitiesThatMatchToPenAttributes(
    const RS2::LineType &lineType, const RS2::LineWidth &width, const RS_Color &color, bool colorCheck, bool resolvePens, bool resolveLayers) const{
    auto graphic = mdi_win->getGraphic();
    int selectedCount = 0;
    int hasEntitiesOnFrozenLayers = false;
    int hasEntitiesOnLockedLayers = false;
        foreach (auto e, graphic->getEntityList()) {

            // based on parameter, we'll use either entity's attributes pen - or resolved pen that is actually used for drawing
            RS_Pen pen = e->getPen(resolvePens);

            if (lineType != RS2::LineTypeUnchanged){
                // test linetype match only if target line type is not "unchanged" (which means "any")
                if (pen.getLineType() != lineType){
                    continue;
                }
            }

            if (width != RS2::WidthUnchanged){
                // test line width  match only if target line width is not "unchanged" (which means "any")
                if (width != pen.getWidth()){
                    continue;
                }
            }

            if (colorCheck){
                // check for color if it is requested by parameter
                // todo - check and verify equality implementation... should we take care of flags there?
                if (color != pen.getColor()){
                    continue;
                }
            }

            // additional check - if entity is matched, might it be that it is on layer which is hidden or frozen?
            RS_Layer *layer = e->getLayer(resolveLayers);
            if (layer->isFrozen()){
                hasEntitiesOnFrozenLayers = true;
            }
            if (layer->isLocked()){
                hasEntitiesOnLockedLayers = true;
            } else {
                // we are on normal layer and entity is matched - so simply select it
                e->setSelected(true);
                selectedCount++;
            }
        }

    if (selectedCount == 0){
        // no entities are actually selected
        if (penPaletteModel->getOptions()->showNoSelectionMessage){
            // based by options, we'll let know the user that we can't select
            showNoSelectionDialog(hasEntitiesOnFrozenLayers, hasEntitiesOnLockedLayers);
        }
    } else {
        // just update drawing with selected entities
        redrawDrawing();
    }
}

/**
 * Updates actual pen in pen toolbar by the pen of current active layer.
 * This is just nice convenient shortcut and  usability improvement.
 * The pen tool bar's pen is updated on active layer change - yet only if the user didn't modify actual pen manually.
 * Therefore, if it is necessary to return active pen to "by layer" state, it's necessary to do this manually and set all 3 attributes of pen there.
 * This method eliminates that and simply sets the pen of active layer to pen toolbar in one click.
 */
void LC_PenPaletteWidget::updatePenToolbarByActiveLayer(){
    if (!isVisible())
        return;
    QG_PenToolBar *penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
    if (penToolBar != nullptr){
        if (layerList != nullptr){
            RS_Layer* layer = layerList->getActive();
            if (layer != nullptr){
                RS_Pen layerPen = layer->getPen();
                penToolBar->setLayerLineType(layerPen.getLineType(), true);
                penToolBar->setLayerWidth(layerPen.getWidth(), true);
                penToolBar->setLayerColor(layerPen.getColor(), true);
            }
        }
    }
}

/**
 *  Applies attributes of pen in the editor to the pen toolbar.
 *  Only attributes that are not "unchanged" are applied.
 */
void LC_PenPaletteWidget::applyEditorPenToPenToolBar(){
    QG_PenToolBar *penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();

    if (penToolBar != nullptr){

        if (layerList != nullptr){
            RS_Layer* layer = layerList->getActive();
            if (layer != nullptr){
                RS_Pen layerPen = layer->getPen();
                int lineTypeIndex = cbType->currentIndex();
                if (lineTypeIndex > 0){ // it is not unchanged value
                    RS2::LineType lineType = cbType->getLineType();

                    // todo... mmm - what is semantic of "by block" in pen toolbar?
                    if (lineType == RS2::LineType::LineByLayer){
                        // we set pen from currently active layer
                        penToolBar->setLayerLineType(layerPen.getLineType(), true);
                    }
                    else{
                        // set linetype from editor
                        penToolBar->setLineType(lineType);
                    }
                }

                int widthIndex = cbWidth->currentIndex();
                if (widthIndex > 0){ // this is not "unchanged" value, so apply it
                    RS2::LineWidth width = cbWidth->getWidth();
                    if (width == RS2::LineWidth::WidthByLayer){
                        // for by layer we use pen from active layer
                        penToolBar->setLayerWidth(layerPen.getWidth(), true);
                    }
                    else{
                        // set width from editor
                        penToolBar->setWidth(width);
                    }
                }

                int colorIndex = cbColor->currentIndex();
                if (colorIndex > 0){ // again, proceed if color is not "unchanged"
                    RS_Color color = cbColor->getColor();

                    if (color.isByLayer()){
                        penToolBar->setLayerColor(layerPen.getColor(), true );
                    }
                    else {
                        penToolBar->setColor(color);
                    }
                }
            }
      }
  }
}

/**
 * Fills pen editor by pen attributes from pen toolbar.
 */
void LC_PenPaletteWidget::fillPenEditorByPenToolBarPen(){
    QG_PenToolBar* penToolBar  = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
    if (penToolBar != nullptr){
        doFillPenEditorByPen(penToolBar->getPen());
    }
}

/**
 * Removes pen info item that is currently active (if any) from the model and underlying storage
 */
void LC_PenPaletteWidget::removeActivePenItem(){
    LC_PenItem* activePenItem = penPaletteModel->getActivePen();
    if (activePenItem != nullptr){
        doRemovePenItem(activePenItem);
    }
}

/**
 * Do actual removal of specific pen item from the model and underlying storage
 * @param penItem pen to remove
 */
void LC_PenPaletteWidget::doRemovePenItem(LC_PenItem *penItem){
    QString penName = penItem->getName();
    // invoke confirmation dialog
    int dialogResult = invokeItemRemovalDialog(penName);
    if (dialogResult == QMessageBox::Ok){
        // remove in model
        penPaletteModel->removeItem(penItem);
        updateModel();
    }
}

/**
 * Removes several pen items
 * @param penItems items to remove
 */
void LC_PenPaletteWidget::doRemovePenItems(QList<LC_PenItem *> &penItems){
    // ask for confirmation
    int dialogResult = invokeItemMultiRemovalDialog(penItems);
    if (dialogResult == QMessageBox::Ok){
    // remove each item
        int count = penItems.count();
        for (int i=0; i< count; i++) {
            LC_PenItem* item = penItems.at(i);
            penPaletteModel->removeItem(item);
        }
        updateModel();
    }
}
/**
 * Removes pen item that is currently selected in the table
 */
void LC_PenPaletteWidget::removeSelectedPenItem(){
    LC_PenItem* item = getSelectedPenItem();
    if (item != nullptr){
        doRemovePenItem(item);
    }
}

/**
 * Removes pen items that are currently selected in the table
 */
void LC_PenPaletteWidget::removeSelectedPenItems(){
    QList<LC_PenItem*> items = getSelectedPenItems();
    if (!items.isEmpty()){
        doRemovePenItems(items);
    }
}

/**
 * Perform editing of selected pen item - just make the pen active and
 * fills pen editor by attributes of selected pen item
 */
void LC_PenPaletteWidget::editSelectedPenItem(){
    LC_PenItem* item = getSelectedPenItem();
    if (item != nullptr){
        penPaletteModel->setActivePen(item);
        fillPenEditorByPenItem(item);
    }
}

/**
 * Applies attributes from selected pen item in the pens table to pen toolbar.
 * Only attributes that are not with 'unchanged' value are applied.
 */
void LC_PenPaletteWidget::applySelectedPenItemToPenToolBar(){
    LC_PenItem* item = getSelectedPenItem();
    if (item != nullptr){
        QG_PenToolBar *penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();

        if (penToolBar != nullptr){
            if (layerList != nullptr){
                // retrieve active layer
                RS_Layer* layer = layerList->getActive();
                if (layer != nullptr){
                    RS_Pen layerPen = layer->getPen();

                    // handle line type
                    RS2::LineType lineType = item->getLineType();
                    if (lineType != RS2::LineType::LineTypeUnchanged){ // not unchanged
                        if (lineType == RS2::LineType::LineByLayer){
                            // apply attributes from layer if it is by layer
                            penToolBar->setLayerLineType(layerPen.getLineType(), true);
                        }
                        else{
                            // apply attribute from pen
                            penToolBar->setLineType(lineType);
                        }
                    }

                    // handle line width
                    RS2::LineWidth width = item->getLineWidth();
                    if (width!=RS2::LineWidth::WidthUnchanged){
                        if (width == RS2::LineWidth::WidthByLayer){
                            // by layer, we rely on layer' attributes
                            penToolBar->setLayerWidth(layerPen.getWidth(), true);
                        }
                        else{
                            // set width from editor
                            penToolBar->setWidth(width);
                        }
                    }

                    // handle color
                    RS_Color color = item->getColor();
                    if (!LC_PenInfoRegistry::isUnchangedColor(color)){
                        if (color.isByLayer()){
                            // for color by layer we pick actual color from active layer
                            penToolBar->setLayerColor(layerPen.getColor(), true );
                        }
                        else {
                            // set color from editor
                            penToolBar->setColor(color);
                        }
                    }
                }
            }
        }
    }
}

/**
 * Here we apply attributes of selected pen item in pens table to current active layer.
 */
void LC_PenPaletteWidget::applySelectedPenItemToActiveLayer(){
    LC_PenItem *selectedPenItem = getSelectedPenItem();
    if (selectedPenItem != nullptr){
        if (layerList != nullptr){
            RS_Layer *layer = layerList->getActive();
            if (layer != nullptr){
                RS_Pen layerPen = layer->getPen();

                RS_Pen penCopy = createPenByPenItem(layerPen, selectedPenItem);

                layer->setPen(penCopy);
                layerList->activate(layer, true);
            }
        }
        redrawDrawing();
    }
}

/**
 * Utility method that returns pen item that is currently selected in pens table.
 * Selected pen may differ from active pen. Method is used by operations that
 * expect existence of only one selected item.
 * @return selected pen item or null if none
 */
LC_PenItem *LC_PenPaletteWidget::getSelectedPenItem(){
    LC_PenItem *selectedPenItem = nullptr;
    QModelIndex selectedIndex = getSelectedItemIndex();
    if (selectedIndex.isValid()){
        selectedPenItem = penPaletteModel->getItemForIndex(selectedIndex);
    }
    return selectedPenItem;
}

/**
 * Returns list of pen items selected in pens table
 * @return list of selected items (empty list if none)
 */
QList<LC_PenItem *> LC_PenPaletteWidget::getSelectedPenItems(){
    QList<LC_PenItem *> result;
    QModelIndexList selectedIndexes = tableView->selectionModel()->selectedRows();
    int count = selectedIndexes.size();
    for (int i = 0; i < count; i++){
        QModelIndex index = selectedIndexes.at(i);
        LC_PenItem *selectedPenItem = penPaletteModel->getItemForIndex(index);
        if (selectedPenItem != nullptr){
            result << selectedPenItem;
        }
    }    
    return result;
}

/**
 * Utility method that returns only one selected pen item
 * @return selected item (or null if no selection or more than one item is selected)
 */
QModelIndex LC_PenPaletteWidget::getSelectedItemIndex(){
    QModelIndex result;
    QModelIndexList selectedIndexes = tableView->selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

/**
 * Fills fiels of pen' editor by attributes of pen for active layer
 */
void LC_PenPaletteWidget::fillPenEditorByActiveLayer(){
    if (layerList != nullptr){
        RS_Layer *layer = layerList->getActive();
        if (layer != nullptr){
            RS_Pen pen = layer->getPen();
            doFillPenEditorByPen(pen);
        }
    }
}

/**
 * Creates real RS_Pen by attributes in editor pen.
 * For original pen passed, a copy of it created and all attributes of pen from pen editor (except "unchanged" values) are applied to pen's copy.
 * @param originalPen original pen to which attributes from pen editor will be applied
 * @return copy of original pen with applied pen attributes from editor
 */
RS_Pen LC_PenPaletteWidget::createPenByEditor(const RS_Pen &originalPen){
    RS_Pen penCopy = RS_Pen(originalPen);

    // apply linetype
    int lineTypeIndex = cbType->currentIndex();
    if (lineTypeIndex > 0){ // not unchanged
        RS2::LineType lineType = cbType->getLineType();
        penCopy.setLineType(lineType);
    }

    // apply line width
    int widthIndex = cbWidth->currentIndex();
    if (widthIndex > 0){ // not unchanged
        RS2::LineWidth width = cbWidth->getWidth();
        penCopy.setWidth(width);
    }

    // apply color
    int colorIndex = cbColor->currentIndex();
    if (colorIndex > 0){
        RS_Color color = cbColor->getColor();
        penCopy.setColor(color);
    }

    return penCopy;
}

/**
 * Creates real RS_Pen by pen attributes stored in provided item.
 * For original pen passed, a copy of it created and all attributes of pen from pen editor (except "unchanged" values) are applied to pen's copy.
 *
 * @param originalPen original pen to which attributes will be applied
 * @param item pen item that holds pen attributes
 * @return copy of original pen with applied pen attributes
 */
RS_Pen LC_PenPaletteWidget::createPenByPenItem(RS_Pen &originalPen, LC_PenItem *item){
    RS_Pen penCopy = RS_Pen(originalPen);

    // apply linetype
    RS2::LineType lineType = item->getLineType();
    if (lineType != RS2::LineType::LineTypeUnchanged){ // not unchanged
        penCopy.setLineType(lineType);
    }

    // apply line width
    RS2::LineWidth lineWidth = item->getLineWidth();
    if (lineWidth != RS2::LineWidth::WidthUnchanged){
        penCopy.setWidth(lineWidth);
    }

    // apply color
    RS_Color color = item->getColor();
    if (!LC_PenInfoRegistry::isUnchangedColor(color)){
        penCopy.setColor(color);
    }
    return penCopy;
}

/**
 * Applies pen attributes that are in pen editor to active layer.
 */
void LC_PenPaletteWidget::applyEditorPenToActiveLayer(){
    if (layerList != nullptr){
        RS_Layer* layer = layerList->getActive();
        if (layer != nullptr){
            // original pen of layer
            RS_Pen layerPen = layer->getPen();

            // we create a copy of layer's pen with applied attributes from editor
            RS_Pen penCopy = createPenByEditor(layerPen);

            // apply pen to the layer
            layer->setPen(penCopy);

            // activate layer again for refreshing layers view
            layerList->activate(layer, true);
        }
    }
    redrawDrawing();
}

/**
 * Handles changes in filtering section (text or mode flag).
 * Simpy stores the value of filtering mode in setting and updates table model accordingly
 */
void LC_PenPaletteWidget::filterMaskChanged(){
    QString mask = leFilterMask->text();
    bool highlightMode = cbHighlightMode->isChecked();

    // storing filtering mode in persistent settings
    LC_PenPaletteOptions *options = penPaletteModel->getOptions();
    options->filterIsInHighlightMode = highlightMode;
    options->saveToSettings();

    // update model
    penPaletteModel->setFilteringRegexp(mask);
    updateModel();
}


/**
 * Updates table view and pens model
 */
void LC_PenPaletteWidget::updateModel(){
    // complete rebuild of the model and update of UI
    int yPos = tableView->verticalScrollBar()->value();
    tableView->verticalScrollBar()->setValue(yPos);
    penPaletteModel->update(false);
    tableView->viewport()->update();
}

/**
 * Fills pen editor by attributes of provided pen item
 * @param pen pen item with attributes
 */
void LC_PenPaletteWidget::fillPenEditorByPenItem(LC_PenItem *pen){
    // set flag that we're in setup of editor to disable handling of data change signals from editor's controls
    inEditorControlsSetup = true;

    // setup editor
    QString name = pen->getName();
    lePenName->setText(name);
    RS_Color color = pen->getColor();
    RS2::LineWidth width = pen->getLineWidth();
    RS2::LineType lineType = pen->getLineType();
    doUpdatePenEditorByPenAttributes(color, width, lineType);

    // allow processing of data change signals from editor controls again
    inEditorControlsSetup = false;
}

/**
 *  Fill pen editor by attributes from given pen
 * @param pen pen with attributes
 */
void LC_PenPaletteWidget::doFillPenEditorByPen(RS_Pen pen){    
    lePenName->setText("");
    RS_Color color = pen.getColor();
    RS2::LineWidth width = pen.getWidth();
    RS2::LineType lineType = pen.getLineType();
    doUpdatePenEditorByPenAttributes(color, width, lineType);
}


/**
 * Setup controls of pen editors by provided pen attributes
 * @param color color
 * @param width line width
 * @param lineType line type
 */
void LC_PenPaletteWidget::doUpdatePenEditorByPenAttributes(const RS_Color &color, RS2::LineWidth &width, RS2::LineType &lineType){

    // item with 0 index for all comboboxes in editor is used for "unchanged" value due to editor controls setup

    // setup color
    if (LC_PenInfoRegistry::isUnchangedColor(color)){
        cbColor->setCurrentIndex(0); // 0 is for unchanged. Potentially that might be incorporated into colorbox...
    }
    else{      
        cbColor->setColor(color);    
    }

    // setup linetype
    if (RS2::WidthUnchanged == width){
        cbWidth->setCurrentIndex(0); // 0 is for unchanged value
    } else {
        cbWidth->setWidth(width);
    }

    // setup line width
    if (lineType == RS2::LineTypeUnchanged){
        cbType->setCurrentIndex(0); // 0 is for unchanged value
    }
    else {
        cbType->setLineType(lineType);
    }
    // focus on name editor
    lePenName->setFocus();

    // disable save button in editor until something will be changed
    markEditingPenChanged(false);
}

/**
 * Handles changes in pen editor controls and enable save button if needed
 * @param changed flag whether pen change occurred
 */
void LC_PenPaletteWidget::markEditingPenChanged(bool changed){
    editorChanged = changed;
    if (changed){
        // first check that we're not invoked from editor setup
        if (!inEditorControlsSetup){
            QString penName = lePenName->text();
            if (!penName.isEmpty()){
                tbEditSave->setEnabled(true);
            }
            // as user started the pen editing, remove active pen in table
            penPaletteModel->setActivePen(nullptr);
            tableView->update();
        }
    } else {
        tbEditSave->setEnabled(false);
    }    
}

/**
 * Handlers for controls chane in pen' editor
 */
void LC_PenPaletteWidget::onPenEditorChanged(){
    markEditingPenChanged(true);
}

void LC_PenPaletteWidget::onPenEditorColorChanged([[maybe_unused]] int index){
    markEditingPenChanged(true);
}

void LC_PenPaletteWidget::onPenEditorWidthChanged([[maybe_unused]] int index){
    markEditingPenChanged(true);
}

void LC_PenPaletteWidget::onPenEditorLineTypeChanged([[maybe_unused]] int index){
    markEditingPenChanged(true);
}

/**
 * setter for layers list
 * @param ll
 */
void LC_PenPaletteWidget::setLayerList(RS_LayerList *ll) {
    layerList = ll;
}


/**
 * Escape releases focus.
 */
void LC_PenPaletteWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {

    case Qt::Key_Escape:
        emit escape();
        break;

    default:
        QWidget::keyPressEvent(e);
        break;
    }
}

/**
 * Utility method that forces redraw of graphic view
 */
void LC_PenPaletteWidget::redrawDrawing() const{mdi_win->getGraphicView()->redraw(RS2::RedrawDrawing);}

/**
 * Confirmation dialog for single pen removal
 * @param penName name of pen to be removed
 * @return dialog execution result. If QMessageBox::Ok - remove confirmed
 */
int LC_PenPaletteWidget::invokeItemRemovalDialog(QString &penName){
    QString title(QMessageBox::tr("Remove pen"));
    QStringList text_lines = {QMessageBox::tr("Pen will be removed from palette, drawing entities will not be affected."), "",
                              QMessageBox::tr("Are you sure you'd like to proceed?"),};
    QStringList detail_lines = {QMessageBox::tr("Pen for removal:"), "",};
    detail_lines << penName;

    QMessageBox msgBox(QMessageBox::Warning, title, text_lines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDetailedText(detail_lines.join("\n"));

    int result = msgBox.exec();
    return result;
}

/**
 * Displays confirmation dialog for several items removal
 * @param penItems list of pen names to remove
 * @return result of the dialog execution. If QMessageBox::Ok - remove confirmed
 */
int LC_PenPaletteWidget::invokeItemMultiRemovalDialog(QList<LC_PenItem*> &penItems){
    QString title(QMessageBox::tr("Remove pens"));
    QStringList text_lines = {QMessageBox::tr("Pens will be removed from palette, drawing entities will not be affected."), "",
                              QMessageBox::tr("Are you sure you'd like to proceed?"),};
    QStringList detail_lines = {QMessageBox::tr("Pens for removal:"), "",};
    int count = penItems.count();
    for (int i = 0; i < count;i++) {
        QString penName = penItems.at(i)->getName();
        detail_lines << penName;
    }

    QMessageBox msgBox(QMessageBox::Warning, title, text_lines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDetailedText(detail_lines.join("\n"));

    int result = msgBox.exec();
    return result;
}

/**
 * Displays dialog which is shown if there are no items to select by pen
 *
 * @param hasOnFrozenLayers flag that entities that match pen are present on frozen layers
 * @param hasOnLockedLayers flag that entities that match pen are present on locked layers
 */
void LC_PenPaletteWidget::showNoSelectionDialog(bool hasOnFrozenLayers, bool hasOnLockedLayers) const{
    QString title(QMessageBox::tr("Select Entities "));
    QStringList text_lines = {QMessageBox::tr("There are no entities that matches pen on visible layers."),
                              (hasOnFrozenLayers) ? QMessageBox::tr("Such entities exist on frozen layers.\n"):"",
                              (hasOnLockedLayers) ? QMessageBox::tr("Such entities exist on locked layers.\n"):"",
                              QMessageBox::tr("Please use different pen attributes."),};
    QMessageBox msgBox(QMessageBox::Warning, title, text_lines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.exec();
}

/**
 * Displays message box notifying the user that only one entity should be selected for picking pen attributes
 */
void LC_PenPaletteWidget::showEntitySelectionInfoDialog(){
    QString title(QMessageBox::tr("Set pen by entity"));
    QString text = QMessageBox::tr("Please select only one entity to pick pen setting.");
    QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok);
    msgBox.exec();
}

/**
 * Displays dialog if error occurred during saving pens data and prompt the user to change location of pens file
 * @return true if user would like to change location
 */
bool LC_PenPaletteWidget::invokeUnableToSavePenDataDialog(){
    QString title(QMessageBox::tr("Saving Pens Data"));
    QString text = QMessageBox::tr("Unable to save pens data to specified pens file. Would you like to specify correct path to the file?");
    QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Yes | QMessageBox::No);
    int dlgResult = msgBox.exec();
    bool result = dlgResult == QMessageBox::Yes;
    return result;
}







