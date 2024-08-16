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

#include <QHeaderView>
#include <QToolButton>
#include <QMenu>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QContextMenuEvent>
#include <QMap>
#include <QTreeView>
#include <QtWidgets>

#include "lc_layerdialog_ex.h"
#include "lc_layertreeitem.h"
#include "lc_layertreemodel_options.h"
#include "lc_layertreeoptionsdialog.h"
#include "lc_layertreeview.h"
#include "lc_layertreewidget.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "lc_flexlayout.h"

/**
 * Constructor.
 */
LC_LayerTreeWidget::LC_LayerTreeWidget(
    QG_ActionHandler *ah, QWidget *parent, const char *name, Qt::WindowFlags f):QWidget(parent, f){

    setObjectName(name);
    actionHandler = ah;
    layerList = nullptr;

    // TODO - should this flag be persistent? Let's keep it transient so far, let's see

    flatListMode = false;

    QLayout *layFiltering = initFilterAndSettingsSection();

    initTreeModel();

    layerTreeView = initTreeView();

    QLayout *layButtons = initButtonsBar();
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addLayout(layFiltering);
    lay->addLayout(layButtons);
    QSizePolicy policy = layerTreeView->sizePolicy();
    policy.setVerticalStretch(1);
    layerTreeView->setSizePolicy(policy);
    lay->addWidget(layerTreeView);
    this->setLayout(lay);
}

void LC_LayerTreeWidget::initTreeModel(){
    auto *options = new LC_LayerTreeModelOptions();
    options->load();
    layerTreeModel = new LC_LayerTreeModel(this, options);
    layerTreeModel->setFlatMode(flatListMode);
}

/**
 * UI initialization of TreeView. Model should be already created
 * @return
 */
LC_LayerTreeView *LC_LayerTreeWidget::initTreeView(){

    auto *treeView = new LC_LayerTreeView(this);
    treeView->setup(layerTreeModel);

    QHeaderView *pTreeHeader{treeView->header()};
    pTreeHeader->setMinimumSectionSize(LC_LayerTreeModel::ICONWIDTH + 4);
    pTreeHeader->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    pTreeHeader->setStretchLastSection(true);
    pTreeHeader->hide();

    treeView->setColumnWidth(LC_LayerTreeModel::VISIBLE, LC_LayerTreeModel::ICONWIDTH);
    treeView->setColumnWidth(LC_LayerTreeModel::VISIBLE, LC_LayerTreeModel::ICONWIDTH);
    treeView->setColumnWidth(LC_LayerTreeModel::LOCKED, LC_LayerTreeModel::ICONWIDTH);
    treeView->setColumnWidth(LC_LayerTreeModel::PRINT, LC_LayerTreeModel::ICONWIDTH);
    treeView->setColumnWidth(LC_LayerTreeModel::CONSTRUCTION, LC_LayerTreeModel::ICONWIDTH);
    treeView->setColumnWidth(LC_LayerTreeModel::COLOR_SAMPLE, LC_LayerTreeModel::ICONWIDTH);

    treeView->setUniformRowHeights(true);
    treeView->setAlternatingRowColors(false);
    treeView->setSelectionMode(QAbstractItemView::NoSelection);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setFocusPolicy(Qt::NoFocus);
    treeView->setMinimumHeight(65);
    //treeView->setMinimumHeight(140);


    treeView->setDragDropMode(QAbstractItemView::InternalMove);
    treeView->setDragEnabled(true);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setAcceptDrops(true);
    treeView->setDropIndicatorShown(true);
    treeView->setExpandsOnDoubleClick(false);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    treeView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");

    connect(treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(QPoint)));
    connect(treeView, &QTreeView::clicked, this, &LC_LayerTreeWidget::slotTreeClicked);
    connect(treeView, &QTreeView::doubleClicked, this, &LC_LayerTreeWidget::slotTreeDoubleClicked);

    return treeView;
}

/**
 * UI initialization of filtering section
 * @brief LC_LayerTreeWidget::initFilterAndSettingsSection
 * @return
 */
QLayout *LC_LayerTreeWidget::initFilterAndSettingsSection(){
    auto *layFiltering = new QHBoxLayout;

    // lineEdit to filter layer list with RegEx
    matchLayerName = new QLineEdit(this);
    matchLayerName->setReadOnly(false);
    matchLayerName->setPlaceholderText(tr("Filter"));
    matchLayerName->setClearButtonEnabled(true);
    matchLayerName->setToolTip(tr("Looking for matching layer names"));
    connect(matchLayerName, &QLineEdit::textChanged, this, &LC_LayerTreeWidget::slotFilteringMaskChanged);

    // TODO - in general, it is possible to use persistent settings for the state, yet not sure it is reasonable
    matchModeCheckBox = new QCheckBox(this);
    matchModeCheckBox->setText(tr("Highlight Mode"));
    matchModeCheckBox->setChecked(true);
    connect(matchModeCheckBox, &QCheckBox::clicked, this, &LC_LayerTreeWidget::slotFilteringMaskChanged);

    layFiltering->addWidget(matchLayerName);
    layFiltering->addWidget(matchModeCheckBox);

    // settings button
    // const QSize minButSize(28, 28);
    auto* but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/settings.svg"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Settings"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::invokeSettingsDialog);
    layFiltering->addWidget(but);

    return layFiltering;
}

/**
 * UI initialization for actions bar
 * TODO - in general, it's possible to use separate widget for button bar and manage the state there.
 * Lets eave it for now as it is in order to minify affecting codebase, probably will refactor later
 */
QLayout *LC_LayerTreeWidget::initButtonsBar(){
//    auto *layButtons = new QHBoxLayout;
    auto *layButtons = new LC_FlexLayout(0,5,5);
    QToolButton *but;
//    const QSize minButSize(28, 28);

    // show all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/visible.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::showAllLayers);
    layButtons->addWidget(but);

    // hide all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/invisible.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Hide all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::hideAllLayers);
    layButtons->addWidget(but);

    // toggle secondary layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_vertical.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show Secondary Layers"));
    but->setCheckable(true);
    but->setChecked(true); // visible by default
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::toggleSecondaryLayersVisibility);
    layButtons->addWidget(but);
    btnShowSecondaryLayers = but;

    // Visible only active
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/select_all.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show Active Layer Only"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::showActiveLayerOnly);
    layButtons->addWidget(but);

//    layButtons->addStretch(1);

    // expand all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/order.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Expand All"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::expandAllLayers);
    layButtons->addWidget(but);
    btnExpandAll = but;

    // collapse all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/upmost.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Collapse All"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::collapseAllLayers);
    layButtons->addWidget(but/*, 10*/);
    btnCollapseAll = but;

    // expand all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_aligned.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Collapse Secondary"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::collapseSecondaryLayers);
    layButtons->addWidget(but);
    btnCollapseSecondary = but;

//    layButtons->addStretch(1);

    // unlock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/unlocked.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Unlock all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::unlockAllLayers);
    layButtons->addWidget(but);

    // lock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/locked.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Lock all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::lockAllLayers);
    layButtons->addWidget(but);

    // add layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/add.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Add a layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::addLayer);
    layButtons->addWidget(but);

    // add dimensional layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_horizontal.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Add dimensions Layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::addDimensionalLayerForActiveLayer);

    btnAddDimensional = but;
    layButtons->addWidget(but);

    // remove layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/remove.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Remove layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::removeActiveLayers);
    layButtons->addWidget(but);

    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/rename_active_block.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Modify layer attributes / rename"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::editActiveLayer);
    layButtons->addWidget(but);

//    layButtons->addStretch(10);

    // add separator line
    // auto *vFrame = new QFrame;
    // vFrame->setFrameShape(QFrame::VLine);
    // layButtons->addWidget(vFrame);

    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/down.svg"));
//    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Flat List Mode)"));
    but->setCheckable(true);
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::toggleFlatView);

    layButtons->addWidget(but);
    btnListMode = but;
    updateToolBarButtons();
    return layButtons;
}

/**
 * Sets the RS_LayerList this layer widget should show. If layer list does not equal to previously
 * set one, that leads to complete rebuild of underlying model
 */
void LC_LayerTreeWidget::setLayerList(RS_LayerList *ll){
    RS_DEBUG->print("LC_LayerTreeWidget::setLayerList()");
    if (ll == nullptr){
        this->layerList = nullptr;
        update();
    }
    else{
        if (ll != this->layerList){
           this->layerList = ll;
          update();
        }
    }
}

/**
 * Updates the layer box from the layers in the graphic.
 * Primary method used to rebuild model for modifications and
 * updating the UI.
 */
void LC_LayerTreeWidget::update(){

    RS_DEBUG->print("QG_LayerWidget::update() begin");

    if (!layerTreeView){
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerView");
        return;
    }

    if (!layerList){
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QG_LayerWidget::update: nullptr layerList");
        return;
    }

    if (withinSelfActivation){
        // layer is activated by our widget, not externally via listener, so we need just modify active
        // path for currently active layer and do not rebuild existing model
        layerTreeModel->proceedActiveLayerChanged(layerList);
    } else {
        // complete rebuild of the model and update of UI
        int yPos = layerTreeView->verticalScrollBar()->value();
        layerTreeView->verticalScrollBar()->setValue(yPos);

        QStringList treeExpansionState = layerTreeView->saveTreeExpansionState();

        // model will be rebuilt as result of setting layer list
        layerTreeModel->setLayerList(layerList);

        if (!treeExpansionState.isEmpty()){
            layerTreeView->restoreTreeExpansionState(treeExpansionState);
        }

        layerTreeView->viewport()->update();
    }

    RS_DEBUG->print("QG_LayerWidget::update(): OK");
}

/**
 * Activates the given layer and makes it the active  layer in the layers list.
 */
void LC_LayerTreeWidget::activateLayer(RS_Layer *layer){
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if (!layer || !layerList){
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layer or layerList");
        return;
    }

    layerList->activate(layer, false);
    update();
    layerTreeView->viewport()->update();

    //update active layer name in main window status bar
    QC_ApplicationWindow::getAppWindow()->slotUpdateActiveLayer();

    RS_DEBUG->print("QG_LayerWidget::activateLayer() end");
}

//---------------- three expanded state control ----------------------

void LC_LayerTreeWidget::collapseAllLayers(){
    layerTreeView->collapseAll();
}

void LC_LayerTreeWidget::expandAllLayers(){
    layerTreeView->expandAll();
}

void LC_LayerTreeWidget::expandItems(int depth){
    if (depth == -1){
        layerTreeView->expandAll();
    } else {
        layerTreeView->expandToDepth(depth);
    }
}
/**
 * Collapsing all secondary items  - all that are not normal or virtual
 */
void LC_LayerTreeWidget::collapseSecondaryLayers(){
    layerTreeView->setUpdatesEnabled(false);
        foreach (QModelIndex index, layerTreeModel->getPersistentIndexList()) {
            if (layerTreeView->isExpanded(index)){
                LC_LayerTreeItem *childItem = layerTreeModel->getItemForIndex(index);
                int type = childItem->getLayerType();
                if (type == LC_LayerTreeItem::NORMAL){
                    layerTreeView->collapse(index);
                }
            }
        }    
    layerTreeView->setUpdatesEnabled(true);
    layerTreeView->viewport()->update();
//    QStringList expansion = layerTreeView->saveTreeExpansionState();
//    layerTreeModel->reset();
//    layerTreeView->restoreTreeExpansionState(expansion);
}

//-------------- Processing clicks on treeView

/**
 * Expand items by double click. Custom implementation, since double click for normal layer items
 * will bring edit dialog, and double click on virtual layer item will fully expand all its descendants
 * @brief LC_LayerTreeWidget::onTreeDoubleClicked
 * @param index
 */
void LC_LayerTreeWidget::slotTreeDoubleClicked(QModelIndex index /*const QString& layerName*/){
    if (index.isValid()){
        LC_LayerTreeItem *layerItem = layerTreeModel->getItemForIndex(index);
        if (layerItem->isVirtual()){ // for virtual layer, we expand children
            // From Qt 5.13, another method might be used
            //  layerTreeView -> expandRecursively(index,1);

            // TODO - think about expansion/collapsing on double click
             layerTreeView->setUpdatesEnabled(false);
            layerTreeView->expandChildren(index);
            layerTreeView->setUpdatesEnabled(true);
        } else {
            // for normal layer - we'll invoke editing
            editSelectedLayer();
        }
    }
}

/**
 * Called when the user activates (highlights) a layer.
 * Method is used for toggling flag states for layers, as well as for activation of specific layer
 * Clicks on virtual layer items will affect descendent items flags and click on Name expands children of that virtual layer.
 * Clicks on real layer items works in the way similar to original layer list.
 */
void LC_LayerTreeWidget::slotTreeClicked(QModelIndex layerIdx /*const QString& layerName*/){

    if (!layerIdx.isValid() || layerList == nullptr){
        return;
    }

    auto *layerItem = static_cast<LC_LayerTreeItem *>(layerIdx.internalPointer());

    // use translated column index to ensure proper column detection
    int column = layerTreeModel->translateColumn(layerIdx.column());

    QList<RS_Layer *> layers;
    QList<RS_Layer *> layersToDisable = layers;
    QList<RS_Layer *> layersToEnable = layers;
    LC_LayerTreeItemAcceptor ACCEPT_ALL = LC_LayerTreeItemAcceptor();

    if (layerItem->isVirtual()){
        switch (column) {
            case LC_LayerTreeModel::VISIBLE: {
                // we use slightly different logic for visibility check
                // comparing to other modes.
                // here we rely on the current state of node

                bool isVisible = layerItem->isVisible();
                if (isVisible){
                    // we'd like to disable layer - so we should disable its secondary children too
                    layerItem->collectLayers(layersToDisable, &ACCEPT_ALL);
                } else { // we enable this level, sub-virtual layers and normal layers - yet without sub-layers
                    // Therefore we select all child except secondary
                    QG_LayerTreeItemAcceptorSecondary ACCEPT_SECONDARY = QG_LayerTreeItemAcceptorSecondary(false);
                    layerItem->collectLayers(layersToEnable, &ACCEPT_SECONDARY);
                }
                manageLayersVisibilityFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::LOCKED:{
                 if (layerItem->isLocked()){
                     layerItem->collectLayers(layersToDisable, &ACCEPT_ALL);
                 }
                 else{
                     layerItem->collectLayers(layersToEnable, &ACCEPT_ALL);
                 }
                manageLayersLockFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::PRINT:{
                if (layerItem->isPrint()){
                    layerItem->collectLayers(layersToDisable, &ACCEPT_ALL);
                }
                else{
                    layerItem->collectLayers(layersToEnable, &ACCEPT_ALL);
                }
                manageLayersPrintFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::CONSTRUCTION: {
                if (layerItem->isConstruction()){
                    layerItem->collectLayers(layersToDisable, &ACCEPT_ALL);
                }
                else{
                    layerItem->collectLayers(layersToEnable, &ACCEPT_ALL);
                }
                manageLayersConstructionFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::NAME: {
                // just convenient way to expand children of this layer
                layerTreeView->expand(layerIdx);
                break;
            }
            default:
                break;
        }
    } else { // real layer
        RS_Layer *lay = layerItem->getLayer();
        if (lay == nullptr)
            return;

        if (column == LC_LayerTreeModel::NAME){
            // Layer activation is here

            // set flag that activation of layer initiated by the widget.
            // this will prevent complete re-build of the model as soon as
            // corresponding listener will be invoked, so only the active path
            // to selected layer will be updated.

            withinSelfActivation = true;
            layerList->activate(lay, true);
            withinSelfActivation = false;

            return;
        }

        // handling clicks on flags
        int layerType = layerItem->getLayerType();
        bool normalLayerWithChildren = layerType == LC_LayerTreeItem::NORMAL && layerItem->childCount() > 0;

        switch (column) {
            case LC_LayerTreeModel::VISIBLE: {
                if (normalLayerWithChildren){
                    bool isVisible = layerItem->isVisible();
                    if (isVisible){
                        // we'd like to disable layer - so we should disable its secondary children
                        layerItem->collectLayers(layersToDisable, &ACCEPT_ALL);
                        layersToDisable << lay;
                        manageLayersVisibilityFlag(layersToEnable, layersToDisable, false);
                    } else {
                        // we enable ony the layer itself, without touching it secondary children
                        actionHandler->toggleVisibility(lay);
                    }
                } else {
                    actionHandler->toggleVisibility(lay);
                }
                break;
            }
            case LC_LayerTreeModel::LOCKED:
                if (normalLayerWithChildren){
                    if (layerItem->isLocked()){
                        layerItem->collectLayers(layersToDisable, &ACCEPT_ALL, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, &ACCEPT_ALL, true);
                    }
                    manageLayersLockFlag(layersToEnable, layersToDisable, false);
                }
                else{
                    actionHandler->toggleLock(lay);
                }
                break;
            case LC_LayerTreeModel::PRINT:
                if (normalLayerWithChildren){
                    if (layerItem->isPrint()){
                        layerItem->collectLayers(layersToDisable, &ACCEPT_ALL, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, &ACCEPT_ALL, true);
                    }
                    manageLayersPrintFlag(layersToEnable, layersToDisable, false);
                }
                else{
                    actionHandler->togglePrint(lay);
                }
                break;
            case LC_LayerTreeModel::CONSTRUCTION:
                if (normalLayerWithChildren){
                    if (layerItem->isConstruction()){
                        layerItem->collectLayers(layersToDisable, &ACCEPT_ALL, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, &ACCEPT_ALL, true);
                    }
                    manageLayersConstructionFlag(layersToEnable, layersToDisable, false);
                }
                else {
                    actionHandler->toggleConstruction(lay);
                }
                break;
            default:
                break;
        }
    }
}

// ----------  Filtering mask
/**
 * Called when reg-expression matchLayerName->text or match mode check box changed.
 * Simply notifies model about filtering change and updates model and ui
 */
void LC_LayerTreeWidget::slotFilteringMaskChanged(){
    QString mask = matchLayerName->text();
    bool highlightMode = matchModeCheckBox->isChecked();
    layerTreeModel->setFilteringRegexp(mask, highlightMode);
    update();
}

/**
 * Shows a context menu for the layer widget. Launched with a right click.
 * Menu is dynamic and is based on the particular item on which click is performed
 */
void LC_LayerTreeWidget::onCustomContextMenu(const QPoint &point){

    if (actionHandler){
        auto *contextMenu = new QMenu(this);
        auto *caption = new QLabel(tr("Layer Menu"), this);
        QPalette palette;
        palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
        palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
        caption->setPalette(palette);
        caption->setAlignment(Qt::AlignCenter);

        // Actions for all layers:

        QModelIndex index = layerTreeView->indexAt(point);

        if (index.isValid()){
            LC_LayerTreeItem *layerItem = layerTreeModel->getItemForIndex(index);
            int layerType = layerItem->getLayerType();
            bool isVirtual = layerItem->isVirtual();
            QString title = "Layer: " + layerItem->getName();
            contextMenu->setTitle(title);

            if (isVirtual){
                contextMenu->addAction(tr("&Add Child Layer"), this, &LC_LayerTreeWidget::addChildLayerForSelectedItem);
                contextMenu->addAction(tr("&Rename"), this, &LC_LayerTreeWidget::renameVirtualLayer);
                contextMenu->addAction(tr("&Remove Layers (Sub-Tree)"), this, &LC_LayerTreeWidget::removeLayersForSelectedItem);
                contextMenu->addSeparator();
                contextMenu->addAction(tr("&Copy Structure (Sub-Tree)"), this, &LC_LayerTreeWidget::createLayerCopy);
                contextMenu->addAction(tr("&Duplicate Content (Sub-Tree)"), this, &LC_LayerTreeWidget::createLayerDuplicate);
                // TODO - should we take care of virtual layer's visibility somehow?
                contextMenu->addAction(tr("&Select Entities (Sub-Tree)"), this, &LC_LayerTreeWidget::selectLayersEntities);
            } else {

                bool NON_ZERO_LAYER = !layerItem->isZero();

                contextMenu->addAction(tr("&Edit Layer &Attributes"), this, &LC_LayerTreeWidget::editSelectedLayer);
                if (NON_ZERO_LAYER){
                    contextMenu->addAction(tr("&Remove Layer"), this, &LC_LayerTreeWidget::removeLayersForSelectedItem);
                }
                contextMenu->addSeparator();

                if (layerType == LC_LayerTreeItem::NORMAL){
                    if (NON_ZERO_LAYER){
                        bool hasItems = false;

                        if (!layerItem->hasChildOfType(LC_LayerTreeItem::DIMENSIONAL)){
                            contextMenu->addAction(tr("&Add Dimensions Sub-Layer"), this, &LC_LayerTreeWidget::addDimensionalLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!layerItem->hasChildOfType(LC_LayerTreeItem::INFORMATIONAL)){
                            contextMenu->addAction(tr("&Add Info Sub-Layer"), this, &LC_LayerTreeWidget::addInformationalLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!layerItem->hasChildOfType(LC_LayerTreeItem::ALTERNATE_POSITION)){
                            contextMenu->addAction(tr("&Add Alternative View Sub-Layer"), this,
                                                   &LC_LayerTreeWidget::addAddAlternativePositionLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!flatListMode){
                            if (layerItem->childCount() > 0){
                                contextMenu->addAction(tr("&Remove Sub-layers"), this, &LC_LayerTreeWidget::removeChildLayersForSelected);
                                hasItems = true;
                            }
                        }
                        if (hasItems){
                            contextMenu->addSeparator();
                        }
                        if (layerItem->childCount() == 0){
                            contextMenu->addAction(tr("Convert to Dimensional Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToDimensional);
                            contextMenu->addAction(tr("Convert to Info Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToInformational);
                            contextMenu->addAction(tr("Convert to Alternative Position Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToAlternativePosition);
                            contextMenu->addSeparator();
                        }

                    }
                } else {
                    contextMenu->addAction(tr("Convert to Normal Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToNormal);
                    if (layerType != LC_LayerTreeItem::DIMENSIONAL){
                        contextMenu->addAction(tr("Convert to Dimensional Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToDimensional);
                    }
                    if (layerType != LC_LayerTreeItem::INFORMATIONAL){
                        contextMenu->addAction(tr("Convert to Info Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToInformational);
                    }
                    if (layerType != LC_LayerTreeItem::ALTERNATE_POSITION){
                        contextMenu->addAction(tr("Convert to Alternative Position Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToAlternativePosition);
                    }
                    contextMenu->addSeparator();
                }

                if (layerItem->isVisible() && !layerItem->isLocked()){
                    contextMenu->addAction(tr("&Select Layer's Entities"), this, &LC_LayerTreeWidget::selectLayersEntities);
                }
                contextMenu->addSeparator();
                contextMenu->addAction(tr("&Create Layer Copy"), this, &LC_LayerTreeWidget::createLayerCopy);
                contextMenu->addAction(tr("&Duplicate Layer With Content"), this, &LC_LayerTreeWidget::createLayerDuplicate);

                contextMenu->addSeparator();
                if (!layerItem->isLocked()){
                    contextMenu->addAction(tr("Move Selection to Layer"), this, &LC_LayerTreeWidget::moveSelectionToLayer);
                    contextMenu->addAction(tr("Duplicate Selection to Layer"), this, &LC_LayerTreeWidget::duplicateSelectionToLayer);
                    contextMenu->addSeparator();
                }
            }
            contextMenu->addSeparator();
        } else {
            // click is not on item
            contextMenu->addAction(tr("&Add Layer"), this, &LC_LayerTreeWidget::addLayer);
        }
        contextMenu->addAction(tr("&Freeze Others Layers"), this, &LC_LayerTreeWidget::hideOtherThanSelectedLayers);
        contextMenu->addAction(tr("&Defreeze All Layers"), this, &LC_LayerTreeWidget::showAllLayers);
        contextMenu->addAction(tr("&Freeze All Layers"), this, &LC_LayerTreeWidget::hideAllLayers);
        contextMenu->addAction(tr("&Unlock All Layers"), this, &LC_LayerTreeWidget::unlockAllLayers);
        contextMenu->addAction(tr("&Lock All Layers"), this, &LC_LayerTreeWidget::lockAllLayers);
        contextMenu->addAction(tr("Enable &Printing All Layers"), this, &LC_LayerTreeWidget::printAllLayers);
        contextMenu->addAction(tr("&Disable Printing All Layers"), this, &LC_LayerTreeWidget::noPrintAllLayers);
        contextMenu->addSeparator();
        contextMenu->addAction(tr("&Find And Remove Empty Layers"), this, &LC_LayerTreeWidget::removeEmptyLayers);

        /// TODO - check whether these action is needed. Actually, it is possible to support them,
        // yet it is necessary to refactor these actions for proper selection of layer
        // plus they are not part of release - probably will restore them later
        //
        //contextMenu->addAction(tr("Export &Visible Layer(s)"), actionHandler, &QG_ActionHandler::slotLayersExportVisible, 0);
        //contextMenu->addAction(tr("&Export Selected Layer(s)"), actionHandler,
        //                                &QG_ActionHandler::slotLayersExportSelected, 0);

        contextMenu->exec(QCursor::pos());
        delete contextMenu;
    }
//     e->accept();
}

// ------------- Layers Visibility Control

/**
 * Makes all layers visible
 */
void LC_LayerTreeWidget::showAllLayers(){
    btnShowSecondaryLayers->setChecked(true);
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToShow = layerTreeModel->collectLayers(&ACCEPT_ALL);
    QList<RS_Layer *> layersToHide;
    manageLayersVisibilityFlag(layersToShow, layersToHide, false);
}

/**
 * Hides all layers except actual one
 */
void LC_LayerTreeWidget::showActiveLayerOnly(){
    if (nullptr != layerList){
        RS_Layer *activeLayer = layerList->getActive();
        if (activeLayer){
            QG_LayerTreeItemAcceptorSameLayerAs ACCEPT_ALL_EXCEPT(activeLayer, true);
            QList<RS_Layer *> layersToHide = layerTreeModel->collectLayers(&ACCEPT_ALL_EXCEPT);
            QList<RS_Layer *> layersToShow;
            layersToShow << activeLayer;

            manageLayersVisibilityFlag(layersToShow, layersToHide, false);
        }
    }
}
/**
 * Makes all layers invisible
 */
void LC_LayerTreeWidget::hideAllLayers(){
    btnShowSecondaryLayers->setChecked(false);
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToShow;
    QList<RS_Layer *> layersToHide = layerTreeModel->collectLayers(&ACCEPT_ALL);
    manageLayersVisibilityFlag(layersToShow, layersToHide, false);
}

/**
 * Hides all layers except selected layer (it might be different from actual one)
 */
void LC_LayerTreeWidget::hideOtherThanSelectedLayers(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        LC_LayerTreeItemAcceptor ACCEPT_ALL;
        QList<RS_Layer *> layersToHide = layerTreeModel->collectLayers(&ACCEPT_ALL);

        QList<RS_Layer *> layersToShow;
        currentItem->collectLayers(layersToShow, &ACCEPT_ALL, true);
        int count = layersToShow.count();
        for (int i = 0; i< count; i++){
            RS_Layer* layer = layersToShow.at(i);
            layersToHide.removeAll(layer);
        }
        if (count > 0){
            layerList->activate(layersToShow.at(0), false);
        }
        manageLayersVisibilityFlag(layersToShow, layersToHide, false);
    }
}
/**
 * Toggles visibility for secondary layers (dimensions, info and alternative position)
 */
void LC_LayerTreeWidget::toggleSecondaryLayersVisibility(){
    LC_LayerTreeItem *root = layerTreeModel->getRoot();
    QList<LC_LayerTreeItem *> secondaryItems;
    QG_LayerTreeItemAcceptorSecondary acceptor;
    root->collectDescendantChildren(secondaryItems, &acceptor, false);

    bool showSecondary = btnShowSecondaryLayers->isChecked();
    int count = secondaryItems.count();

    if (count > 0){
        QList<RS_Layer *> layersToEnable;
        QList<RS_Layer *> layersToDisable;

        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *layerItem = secondaryItems.at(i);

            RS_Layer *layer = layerItem->getLayer();
            LC_LayerTreeItem *primaryLayerItem = layerItem->getPrimaryItem();

            if (primaryLayerItem != nullptr){
                // the logic here is straightforward
                // we enable secondary layer only if primary layer is enabled.
                // so we'll not show dimensions layer if primary normal layer is not visible...
                if (showSecondary && primaryLayerItem->isVisible()){
                    layersToEnable << layer;
                } else {
                    layersToDisable << layer;
                }
            } else {
                if (showSecondary){
                    layersToEnable << layer;
                } else {
                    layersToDisable << layer;
                }
            }
        }
        manageLayersVisibilityFlag(layersToEnable, layersToDisable, false);
    }
}

// ----------- Lock/unlock layers
/**
 * All layers included into view become unlocked (affects only matched layers)
 */
void LC_LayerTreeWidget::unlockAllLayers(){
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToLock;
    QList<RS_Layer *> layersToUnlock = layerTreeModel->collectLayers(&ACCEPT_ALL);
    manageLayersLockFlag(layersToLock, layersToUnlock, false);
}
/**
 * All matched layers become locked
 */
void LC_LayerTreeWidget::lockAllLayers(){
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToLock = layerTreeModel->collectLayers(&ACCEPT_ALL);
    QList<RS_Layer *> layersToUnlock;
    manageLayersLockFlag(layersToLock, layersToUnlock, false);
}
/**
 * All matched layers will be printable
 */
void LC_LayerTreeWidget::printAllLayers(){
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToPrint = layerTreeModel->collectLayers(&ACCEPT_ALL);
    QList<RS_Layer *> layersToNoPrint;
    manageLayersPrintFlag(layersToPrint, layersToNoPrint, false);
}

/**
 * All matched layers will be not printable
 */
void LC_LayerTreeWidget::noPrintAllLayers(){
    LC_LayerTreeItemAcceptor ACCEPT_ALL;
    QList<RS_Layer *> layersToPrint;
    QList<RS_Layer *> layersToNoPrint= layerTreeModel->collectLayers(&ACCEPT_ALL);
    manageLayersPrintFlag(layersToPrint, layersToNoPrint, false);
}

/**
 * Collects layers with no entities and allows the user to remove them via layer removal dialog
 */
void LC_LayerTreeWidget::removeEmptyLayers(){

    // first we inspect all entities ao collect found layers in set
    QSet<RS_Layer*> foundLayers;
    for (auto en: *document) {
        RS_Layer *l = en->getLayer(true);
        if (l != nullptr){
            foundLayers.insert(l);
        }
    }

    // compare layers provided by layers list with set of found layers
    QList<RS_Layer*> layersWithNoEntities;
    unsigned int layersCount = layerList->count();
    for (unsigned int i = 0; i< layersCount; i++){
        RS_Layer* l = layerList->at(i);
        if (foundLayers.contains(l)){
            // do nothing, some entities are found on this layer
        }
        else{
            layersWithNoEntities << l;
        }
    }

    int layersWithNoEntitiesCount = layersWithNoEntities.count();
    if (layersWithNoEntitiesCount > 0){ // empty layers found
        bool emptyLayerIsFiltered = false;

        // collect list of tree items for removal. Items are needed for proper showing display name in removal dialog...
        // TODO - still need to think about layer removal dialog and layer identification there. Probably it's better to show full name instead of display name

        QList<LC_LayerTreeItem*> itemsToRemove;
        for (int i = 0; i < layersWithNoEntitiesCount; i++){
            RS_Layer* l = layersWithNoEntities.at(i);
            LC_LayerTreeItem* item = layerTreeModel->getItemForLayer(l);
            if (item == nullptr){
                if (layerTreeModel->isRegexpApplied()){
                    // this means that there is regexp and layer is filtered...
                    emptyLayerIsFiltered = true;
                }
            }
            else{
                itemsToRemove<<item;
            }
        }

        if (emptyLayerIsFiltered){ // not too good to remove something not visible, so let the user one more chance to check
            QString title(QMessageBox::tr("Remove empty layers"));
            QString text = QMessageBox::tr("Layer(s) without entities found, yet they are filtered and not visible.\n\nClear filtering mask and repeat.");
            QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok);
            msgBox.exec();
        }
        else{// do actual removal
            doRemoveLayerItems(itemsToRemove);
        }
    }
    else{ // no layers with no entities, just show message about that
        QString title(QMessageBox::tr("Remove empty layers"));
        QString text = QMessageBox::tr("No layers without entities found, nothing to remove.");

        QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok);
        msgBox.exec();
    }
}

/**
 * Escape releases focus.
 */
void LC_LayerTreeWidget::keyPressEvent(QKeyEvent *e){
    switch (e->key()) {
        case Qt::Key_Escape:
            emit escape();
            break;
        default:
            QWidget::keyPressEvent(e);
            break;
    }
}

//--------------- LayerListener methods

void LC_LayerTreeWidget::layerActivated(RS_Layer *layer){
    RS_DEBUG->print("LC_LayerTreeWidget::layerActivated()");
    activateLayer(layer);
}

void LC_LayerTreeWidget::layerAdded([[maybe_unused]] RS_Layer *layer){
    RS_DEBUG->print("QG_LayerWidget::layerAdded() begin");
    update();
}

void LC_LayerTreeWidget::layerEdited(RS_Layer *){
    RS_DEBUG->print("LC_LayerTreeWidget::layerEdited()");
    update();
    layerTreeView->viewport()->update();
}

void LC_LayerTreeWidget::layerRemoved(RS_Layer *){
    RS_DEBUG->print("LC_LayerTreeWidget::layerRemoved()");
    update();
    activateLayer(layerList->at(0));
}

void LC_LayerTreeWidget::layerToggled(RS_Layer *){
    RS_DEBUG->print("LC_LayerTreeWidget::layerToggled()");
    update();
}

void LC_LayerTreeWidget::layerToggledLock(RS_Layer *){
    update();
}

void LC_LayerTreeWidget::layerToggledPrint(RS_Layer *){
    update();
}

void LC_LayerTreeWidget::layerToggledConstruction(RS_Layer *){
    update();
}
// --------- Drag & Drop support ---
/**
 * Drag start event = just saving the item which starts dragging
 * @param dropIndex
 */
void LC_LayerTreeWidget::onDragEnterEvent(QModelIndex dropIndex){
    // save source item for which drag is invoked
    LC_LayerTreeItem *currentlyDraggingItem = layerTreeModel->getItemForIndex(dropIndex);
    layerTreeModel->setCurrentlyDraggingItem(currentlyDraggingItem);
}
/**
 * On Drop event.
 * @param dropIndex
 * @param position
 */
void LC_LayerTreeWidget::onDropEvent(QModelIndex dropIndex, DropIndicatorPosition position){
    RS_DEBUG->print(RS_Debug::D_WARNING, "onDropEvent");
    LC_LayerTreeItem *currentlyDraggingItem = layerTreeModel->getCurrentlyDraggingItem();
    if (currentlyDraggingItem){
        LC_LayerTreeItem *destinationItem = nullptr;
        if (dropIndex.isValid()){ // going to drop on other item
            destinationItem = layerTreeModel->getItemForIndex(dropIndex);

        } else if (position == LC_LayerTreeWidget::OnViewport){
            // drop is on viewport, so we'll add source to root item
            destinationItem = layerTreeModel->getRoot();
        }

        if (destinationItem != nullptr){
            // do actual rename
            bool layersRenamed = layerTreeModel->performReStructure(currentlyDraggingItem, destinationItem);
            if (layersRenamed){
                layerTreeView->expand(dropIndex);
                layerList->fireEdit(nullptr);
            }
        }
        layerTreeModel->setCurrentlyDraggingItem(nullptr);
    }
}

//---------------- context menu processing ----------------------

//--------------- Adding layers -----------------------

/**
 * Creates new information layer for selected item
 */
void LC_LayerTreeWidget::addInformationalLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(LC_LayerTreeItem::INFORMATIONAL);
}

/**
 * Creates new alternative position layer for selected item
 */
void LC_LayerTreeWidget::addAddAlternativePositionLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(LC_LayerTreeItem::ALTERNATE_POSITION);
}

/**
 * creates nwe dimensional layer for selected item
 */
void LC_LayerTreeWidget::addDimensionalLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(LC_LayerTreeItem::DIMENSIONAL);
}

/**
 * Adds secondary layer with required type
 * @param layerType new layer type to be created
 */
void LC_LayerTreeWidget::doAddSecondaryLevelForSelectedItem(int layerType){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        if (!currentItem->isVirtual()){
            // check for duplicate first
            if (!currentItem->hasChildOfType(layerType)){
                invokeLayerAddDialog(currentItem, layerType);
            }
            else{ // duplicate found
                QMessageBox::information(parentWidget(),
                                         QMessageBox::tr("Add Layer"),
                                         QMessageBox::tr("Such child layer already exist for \n[%1].\n"
                                                         "Please specify a different name.")
                                             .arg(currentItem->getName()),
                                         QMessageBox::Ok);
            }
        }
    }
}


/**
 * utility method for adding secondary levels for selected item
 */
void LC_LayerTreeWidget::addChildLayerForSelectedItem(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        invokeLayerAddDialog(currentItem, LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE);
    }
}

/**
 * Adds new layer
 */
void LC_LayerTreeWidget::addLayer(){
    invokeLayerAddDialog(nullptr, LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE);
}
/**
 * Adds dimensional layer for currently active layer, if that layer is normal.
 * If dimensional layer already exists - just shows a dialog, otherwise - invokes normal layer creation flow
 */
void LC_LayerTreeWidget::addDimensionalLayerForActiveLayer(){
    if (nullptr != layerList){
        RS_Layer *activeLayer = layerList->getActive();
        if (activeLayer){
            LC_LayerTreeItem *currentItem = layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                if (currentItem->getLayerType() == LC_LayerTreeItem::NORMAL){
                    int newLayerType = LC_LayerTreeItem::DIMENSIONAL;
                    if (!currentItem->hasChildOfType(newLayerType)){
                        invokeLayerAddDialog(currentItem, newLayerType);
                    }
                    else{
                        QMessageBox::information(parentWidget(),
                                                 QMessageBox::tr("Add Layer"),
                                                 QMessageBox::tr("Such child layer already exist for \n[%1].\n")
                                                     .arg(currentItem->getName()),
                                                 QMessageBox::Ok);
                    }
                }
                else{
                    QMessageBox::information(parentWidget(),
                                             QMessageBox::tr("Add Layer"),
                                             QMessageBox::tr("Dimensional layer may be added only for normal active layer.\n")
                                                 .arg(currentItem->getName()),
                                             QMessageBox::Ok);
                }
            }
        }
    }
}

//---------  Editing Layers -----------------

/**
 * Invokes editing dialog for active layer
 */
void LC_LayerTreeWidget::editActiveLayer(){
    if (nullptr != layerList){
        RS_Layer *activeLayer = layerList->getActive();
        if (activeLayer){
            LC_LayerTreeItem *currentItem = layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                invokeLayerEditOrRenameDialog(currentItem, true);
            }
        }
    }
}

/**
 * Invokes editing dialog for selected layer (called from context menu, so selected layer may differ from actual one
 */
void LC_LayerTreeWidget::editSelectedLayer(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        invokeLayerEditOrRenameDialog(currentItem, true);
    }
}
/**
 * Renames virtual layer (and so, renames all descendants of it accordingly)
 */
void LC_LayerTreeWidget::renameVirtualLayer(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        if (currentItem->isVirtual()){
            invokeLayerEditOrRenameDialog(currentItem, false);
        }
    }
}

// ----------- Entities selection related
/**
 * Selects all entities that belongs to selected layer (and it descendants)
 */
void LC_LayerTreeWidget::selectLayersEntities(){
    QList<RS_Layer *> layersList = collectLayersForSelectedItem();
    if (!layersList.isEmpty()){
        doSelectLayersEntities(layersList);
    }
}
/**
 * Moves currently selected entities to selected layer. Layer of original entities is changed to selected layer.
 */
void LC_LayerTreeWidget::moveSelectionToLayer(){
    moveOrDuplicateSelectionToSelectedItemLayer(false, QMessageBox::tr("Move Selection"));
}

/**
 * Creates duplicates for selected entities and add these duplicated entities to selected layer.
 */
void LC_LayerTreeWidget::duplicateSelectionToLayer(){
    moveOrDuplicateSelectionToSelectedItemLayer(true, QMessageBox::tr("Duplicate Selection"));
}

/**
 * Utility method that identifies item for current index, as user whether pens should be resolved and
 * delegates actual processing to doMoveSelectionToLayer method where selected entities are moved or
 * copied to the provided layer
 * @param duplicate if true, duplicate should be performed, if false - move of entities
 * @param title title for user choice dialog
 */
void LC_LayerTreeWidget::moveOrDuplicateSelectionToSelectedItemLayer(bool duplicate, const QString &title){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *layerItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        if (!layerItem->isLocked()){
            if (!layerItem->isVirtual()){
                bool resolvePens = QMessageBox::question(this, title,
                                                         QMessageBox::tr("Replace \"By Layer\" value to source layers values?\n\n"
                                                                         "If Yes - entities with \"By Layer\" pens will look on new layer exactly as on previous layers and "
                                                                         "\"By Layer\" value will be replaced by resolved pens.\n\n"
                                                                         "If No - \"By Layer\" values remains and so pen of target layer will define pen for such entities."),
                                                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
                doMoveSelectionToLayer(layerItem, duplicate, resolvePens);
            }
        }
    }
}

// --------------- Layer type conversion --------------
/**
 * Converts selected layer to dimensional layer type
 */
void LC_LayerTreeWidget::convertLayerTypeToDimensional(){
    doConvertSelectedItemLayerToNewType(LC_LayerTreeItem::DIMENSIONAL);
}

/**
 * Converts selected layer to informational layer type
 */
void LC_LayerTreeWidget::convertLayerTypeToInformational(){
    doConvertSelectedItemLayerToNewType(LC_LayerTreeItem::INFORMATIONAL);
}

/**
* Converts selected layer to alternative position layer type
*/
void LC_LayerTreeWidget::convertLayerTypeToAlternativePosition(){
    doConvertSelectedItemLayerToNewType(LC_LayerTreeItem::ALTERNATE_POSITION);
}

/**
 * Converts secondary layer to normal type
 */
void LC_LayerTreeWidget::convertLayerTypeToNormal(){
    doConvertSelectedItemLayerToNewType(LC_LayerTreeItem::NORMAL);
}

/**
 * Utility method for actual conversion of selected item to new layer type.
 * Just relies on the model for renames, and fires model update
 * @param newType
 */
void LC_LayerTreeWidget::doConvertSelectedItemLayerToNewType(int newType){
    QModelIndex selectedItem = getSelectedItemIndex();
    bool converted = layerTreeModel->convertToType(selectedItem, newType);
    if (converted){
        layerList->fireEdit(nullptr);
    }
}

//----------- Layers removal --------
/**
 * Removes layers (including descendants) for selected item
 */
void LC_LayerTreeWidget::removeLayersForSelectedItem(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        doRemoveLayersFromSource(currentItem, false);
    }
}

/**
 * Removes descendants for selected layer item. Selected layer survives
 */
void LC_LayerTreeWidget::removeChildLayersForSelected(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = layerTreeModel->getItemForIndex(selectedItemIndex);
        doRemoveLayersFromSource(currentItem, true);
    }
}

/**
 * Removes active layer (as well as it descendants)
 */
void LC_LayerTreeWidget::removeActiveLayers(){
    if (nullptr != layerList){
        RS_Layer *activeLayer = layerList->getActive();
        if (activeLayer){
            LC_LayerTreeItem *currentItem = layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                doRemoveLayersFromSource(currentItem, false);
            }
        }
    }
}

//---------------- Layer Copy and Duplicate --------------
/**
 * Creates copy of selected layer as it descendants. Copy of layers (with name rename, if needed) as well as
 * layer's attributes are created, yet no entities from source layers are copied
 */
void LC_LayerTreeWidget::createLayerCopy(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        doCreateLayersCopy(selectedItemIndex, false);
    }
}

/**
 * Creates a copy of selected item and its descendants, and also creates copies for all entities that belongs
 * to original layers and add these entities' copies to appropriate layers copies
 */
void LC_LayerTreeWidget::createLayerDuplicate(){
    QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        doCreateLayersCopy(selectedItemIndex, true);
    }
}
/**
 * Switches between hierarchical tree and flat view for layers list
 */
void LC_LayerTreeWidget::toggleFlatView(){
    flatListMode = !flatListMode;
    layerTreeModel->setFlatMode(flatListMode);
    update();
    updateToolBarButtons();
    // TODO - add settings for expansion depth?
    int expandDepth = 0;
    expandItems(expandDepth);
}
/**
 * Utility method that updates visibility/state for buttons in toolbar according to current view mode
 */
void LC_LayerTreeWidget::updateToolBarButtons(){
    btnCollapseSecondary->setVisible(!flatListMode);
    btnCollapseAll->setVisible(!flatListMode);
    btnExpandAll->setVisible(!flatListMode);
    btnListMode->setChecked(flatListMode);
    btnAddDimensional-> setVisible(!flatListMode);
}

/**
 * Returns model index for currently selected item
 * @return
 */
QModelIndex LC_LayerTreeWidget::getSelectedItemIndex(){
    QModelIndex result;
    QModelIndexList selectedIndexes = layerTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

/**
 * Utility method that returns list of visible layers for selected item and its descendants
 * @return list of layers
 */
QList<RS_Layer *> LC_LayerTreeWidget::collectLayersForSelectedItem(){
    QList<RS_Layer *> layersList;
    QModelIndex selectedIndex = getSelectedItemIndex();
    if (selectedIndex.isValid()){
        LC_LayerTreeItem *layerItem = layerTreeModel->getItemForIndex(selectedIndex);
        QG_LayerTreeItemAcceptorVisible ACCEPT_VISIBLE;
        layerItem->collectLayers(layersList, &ACCEPT_VISIBLE, true);
    }
    return layersList;
}

/**
 * Utility method that deselects entities on locked layer. Actually, similar code is located in specific actions.
 * Included there for consistency and reducing dependencies to other codebase
 * TODO - in general, a refactoring and creation of some utility within the engine is reasonable for such kind of code
 * @param layer
 */
void LC_LayerTreeWidget::deselectEntitiesOnLockedLayer(RS_Layer *layer){
    if (!layer) return;
    if (!layer->isLocked()) return;

    for (auto e: *document) {
        if (e && e->isVisible() && e->getLayer() == layer){
            if (view){
                view->deleteEntity(e);
            }
            e->setSelected(false);
            if (view){
                view->drawEntity(e);
            }
        }
    }
}
/**
 * Utility method that deselects entities on given layer.
 * @param layer
 */
void LC_LayerTreeWidget::deselectEntities(RS_Layer *layer){
    if (!layer) return;

    for (auto e: *document) {
        if (e && e->isVisible() && e->getLayer() == layer){
            if (view){
                view->deleteEntity(e);
            }
            if (view){
                view->drawEntity(e);
            }
        }
    }
}

/**
 * Either makes provide layers visible and invisible, or just toggles visibility flag for given layers.
 * @param layersToEnable list of layers to make visible
 * @param layersToDisable  list of layers to hide
 * @param toggleMode - flag whether toggle mode should be called internally. if true, list in layersToDisable is ignored
 */
void LC_LayerTreeWidget::manageLayersVisibilityFlag(QList<RS_Layer *> &layersToEnable, QList<RS_Layer *> &layersToDisable, bool toggleMode){
    if (view){
        if (toggleMode){
            layerList->toggleFreezeMulti(layersToEnable);
        } else {
            layerList->setFreezeMulti(layersToEnable, layersToDisable);
        }
        document->updateInserts();
        document->calculateBorders();
    }
}
/**
 * Either makes provide layers locked and unlocked, or just toggles lock flag for given layers.
 * @param layersToEnable list of layers to lock
 * @param layersToDisable  list of layers to unlock
 * @param toggleMode - flag whether toggle mode should be called internally. if true, list in layersToDisable is ignored
 */

void LC_LayerTreeWidget::manageLayersLockFlag(QList<RS_Layer *> &layersToLockOrToggle, QList<RS_Layer *> &layersToUnlock, bool toggleMode){
    int count;
    QList<RS_Layer *> layersForEntitiesUpdate;
    if (toggleMode){
        layerList->toggleLockMulti(layersToLockOrToggle);
        count = layersToLockOrToggle.count();
        layersForEntitiesUpdate = layersToLockOrToggle;
    } else {
        layerList->setLockMulti(layersToUnlock, layersToLockOrToggle);
        count = layersToUnlock.count();
        layersForEntitiesUpdate = layersToUnlock;
    }

    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer){
            deselectEntitiesOnLockedLayer(layer);
        }
    }
}

/**
 * Toggles construction flag for given list of layers. It may be changed to set/unset logic later
 * @param layersToBeConstruction list of layers to lock
 * @param layersNonConstruction  not used so far
 * @param toggleMode - not used
 */
void LC_LayerTreeWidget::manageLayersConstructionFlag(QList<RS_Layer *> &layersToBeConstruction, QList<RS_Layer *> &layersNonConstruction, bool toggleMode){
    QList<RS_Layer *> layersForEntitiesUpdate;
     if (toggleMode){
        layerList->toggleConstructionMulti(layersToBeConstruction);
        layersForEntitiesUpdate = layersToBeConstruction;
    } else {
         layerList->setConstructionMulti(layersNonConstruction, layersToBeConstruction);
         layersForEntitiesUpdate = layersNonConstruction;
    }
    int count = layersForEntitiesUpdate.count();
    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer){
            deselectEntities(layer);
        }
    }
}
/**
* Toggles print flag for given list of layers. It may be changed to set/unset logic later
* @param layersToPrint list of layers to lock
* @param layersNoPrint  not used so far
* @param toggleMode - not used
*/
void LC_LayerTreeWidget::manageLayersPrintFlag(QList<RS_Layer *> &layersToPrint, QList<RS_Layer *> &layersNoPrint, bool toggleMode){
    QList<RS_Layer *> layersForEntitiesUpdate;
    if (toggleMode){
       layerList->togglePrintMulti(layersToPrint);
        layersForEntitiesUpdate = layersToPrint;
    }
    else{
        layerList->setPrintMulti(layersNoPrint, layersToPrint);
        layersForEntitiesUpdate = layersNoPrint;
    }
    int count = layersForEntitiesUpdate.count();
    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer){
            deselectEntities(layer);
        }
    }
}

/**
 * Selects entities that belongs to provide list of layers
 * @param layers layers on which entities should be selected
 */
void LC_LayerTreeWidget::doSelectLayersEntities(QList<RS_Layer *> &layers){
    // NOTE:  actually, the more correct location for this logic is RS_Selection class or something like that...
    // yet leave it for now here to reduce amount of codebase modifications.

    for (auto en: *document) {
        if (en && en->isVisible() && !en->isSelected() && (!(en->getLayer() && en->getLayer()->isLocked()))){
            RS_Layer *l = en->getLayer(true);
            if (l != nullptr && layers.contains(l)){
                if (view){
                    view->deleteEntity(en);
                }
                en->setSelected(true);
                if (view){
                    view->drawEntity(en);
                }
            }
        }
    }

    RS_DIALOGFACTORY->updateSelectionWidget(document->countSelected(), document->totalSelectedLength());
}

/**
 * Creates a copy of layers structure denoted by given modelIndex (selected level plus it descendants)
 * @param sourceIndex model index for topmost item to copy
 * @param duplicateEntities if true, entities on copied layers will be also duplicated
 */
void LC_LayerTreeWidget::doCreateLayersCopy(QModelIndex sourceIndex, bool duplicateEntities){
    QHash<RS_Layer *, RS_Layer *> layersMap = layerTreeModel->createLayersCopy(sourceIndex);
    if (layersMap.count() > 0){
        QHashIterator<RS_Layer *, RS_Layer *> iter{layersMap};
        while (iter.hasNext()) {
            iter.next();
            RS_Layer *sourceLayer = iter.key();
            RS_Layer *copyLayer = iter.value();
            copyLayerAttributes(copyLayer, sourceLayer);
            if (duplicateEntities){
                duplicateLayerEntities(sourceLayer, copyLayer);
            }
            // that's pretty ugly that there are no batch operations in LayerList....
            // it may lead to exceeding invocation of listeners
            layerList->add(copyLayer);
        }
        // just additional invocation of listeners on layer list...
        layerList->fireEdit(nullptr);
    }
}

/**
 * Method creates duplicates of entities on source layer on copy layer
 * @param sourceLayer  layer which is copied
 * @param copyLayer  created copy of source layer
 */
void LC_LayerTreeWidget::duplicateLayerEntities(RS_Layer *sourceLayer, RS_Layer *copyLayer){
    // TODO - what about UNDO?
    for (auto en: *document) {
        RS_Layer *l = en->getLayer(true);
        if (l != nullptr && l == sourceLayer){
            RS_Entity *duplicateEntity = en->clone();
            duplicateEntity->setLayer(copyLayer);
            document->addEntity(duplicateEntity);
        }
    }
}
/**
 * Utility method that copies attributes of source layer to its copy
 * @param copyLayer layer to which attributes should be copied
 * @param sourceLayer source layer
 */
void LC_LayerTreeWidget::copyLayerAttributes(RS_Layer *copyLayer, RS_Layer *sourceLayer){
    copyLayer->setConstruction(sourceLayer->isConstruction());
    copyLayer->setConverted(sourceLayer->isConverted());
    copyLayer->setPrint(sourceLayer->isPrint());

    RS_Pen sourcePen = sourceLayer->getPen();
    RS_Pen pen;

    pen.setColor(sourcePen.getColor());
    pen.setWidth(sourcePen.getWidth());
    pen.setLineType(sourcePen.getLineType());
    pen.setAlpha(sourcePen.getAlpha());
    pen.setScreenWidth(sourcePen.getScreenWidth());
    pen.setDashOffset(sourcePen.dashOffset());

    // TODO - check whether flags should be actually copied there
    pen.setFlags(sourcePen.getFlags());

    copyLayer->setPen(pen);
    copyLayer->freeze(sourceLayer->isFrozen());
}

// TODO - actually this operations are good candidates for moving them into separate actions
// that may be invoked  not only from layers widget... at least, moving to active layer may be...
/**
 * Moves or copies currently selected entities to the layer that is stored by provided tree item.
 * If items are copied, selection will be on duplicated entities, or remains on the same entities if they are moved.
 * Optional resolving of pens allows saving selected items appearance (for "By Layer" values) so such entities will
 * look on target layer in the same way as in the source one.
 * @param layerItem - item that denotes layer
 * @param duplicate - if true, entities will be copied, otherwise - moved between layers
 * @param resolvePend - if true, pen of original entity is resolved first and assigned to the entity that will be on target layer
 */
void LC_LayerTreeWidget::doMoveSelectionToLayer(LC_LayerTreeItem* layerItem, bool duplicate, bool resolvePens){
    RS_Layer *targetLayer = layerItem->getLayer();

    // using if there is just for a bit better performance
    if (duplicate){
        for (auto en: *document) { // iterate over all entities
            if (en != nullptr){
                if (en->isVisible() && en->isSelected() && !en->isParentSelected()){
                    RS_Layer *l = en->getLayer(true);
                    if (l != nullptr && l != targetLayer){ // don't move to itself
                        if (view){
                            view->deleteEntity(en);
                        }
                        en->setSelected(false);
                        if (view){
                            view->drawEntity(en);
                        }
                        RS_Entity *duplicateEntity = en->clone();
                        if (resolvePens){
                            // resolve pen in original entities, so "by layer" and "by block" values will be replaced by resolved values
                            RS_Pen resolvedPen = en->getPen(true);
                            // assigning resolved pen back to the entity's copy
                            duplicateEntity ->setPen(resolvedPen);
                        }
                        // switch selection to newly duplicated entity
                        duplicateEntity->setSelected(true);
                        duplicateEntity->setLayer(targetLayer);
                        document->addEntity(duplicateEntity);
                    }
                }
            }
        }
    } else {
        for (auto en: *document) {
            if (en != nullptr){
                if (en->isVisible() && en->isSelected() && !en->isParentSelected()){
                    RS_Layer *l = en->getLayer(true);
                    if (l != nullptr && l != targetLayer){
                        if (view){
                            view->deleteEntity(en);
                        }
                        if (resolvePens){
                            // before changing the layer, resolve pen and
                            // set it back to the entity.
                            RS_Pen resolvedPen = en->getPen(true);
                            en ->setPen(resolvedPen);
                        }
                        en->setLayer(targetLayer);
                        if (view){
                            view->drawEntity(en);
                        }
                    }
                }
            }
        }
    }
}
// TBD - Undo support?
/**
 * Removes layers starting from provide source
 * @param source starting point for removal
 * @param removeChildrenOnly - if true, only children are removed, if false - source will be removed too
 */
void LC_LayerTreeWidget::doRemoveLayersFromSource(LC_LayerTreeItem *source, bool removeChildrenOnly){
    LC_LayerTreeItemAcceptor acceptAllAcceptor;
    QList<LC_LayerTreeItem *> itemsToRemove;

    // prepare list of layers to be removed
    source->collectDescendantChildren(itemsToRemove, &acceptAllAcceptor, !removeChildrenOnly);

    doRemoveLayerItems(itemsToRemove);

}

/**
 * Removes list of layers denoted by the list of items. Virtual items are skipped, and confirmation dialog is shown
 * @param itemsToRemove  items to remove layers
 */
void LC_LayerTreeWidget::doRemoveLayerItems(QList<LC_LayerTreeItem *> &itemsToRemove){
    QStringList layerNames;
    QList<RS_Layer *> layersToRemove;
    for (auto item: itemsToRemove) {
        if (!item->isVirtual()){
            if (item->isZero()){ // don't let removing zero layer
                QMessageBox::information(this, QMessageBox::tr("Remove Layer"), QMessageBox::tr("Layer \"0\" can never be removed."), QMessageBox::Ok);
            } else {
                if (item->isLocked()){
                    // well, the original LayersWidget allows to remove locked layers...
                    // so we'll do so and will not treat this separately
                }
                layerNames << item->getDisplayName();
                layersToRemove << item->getLayer();
            }
        }
    }

    // display layers removal dialog if needed
    if (!layerNames.empty()){
        int dialogResult = invokeLayersRemovalDialog(layerNames);
        if (dialogResult == QMessageBox::Ok){
            doRemoveLayers(layersToRemove);
        }
    }
}

/**
 * Performs actual removal of provided list of layers
 * @param layersToRemove list of layers to remove
 */
void LC_LayerTreeWidget::doRemoveLayers(QList<RS_Layer *> &layersToRemove){
    for (auto layer: layersToRemove) {
        RS_Graphic *graphic = document->getGraphic();
        if (graphic){
            graphic->removeLayer(layer);
        }
    }
    document->updateInserts();
    document->calculateBorders();
    layerList->setModified(true);
}
// ------------ Dialogs -------------
/**
 * Displays layers removal confirmation dialog and returns result of it execution
 *
 * Of course, method could be moved to DialogFactory... keeping it there for minimizing affecting other codebase
 *
 * @param layerNames list of layer names to be removed
 * @return result of the dialog execution
 */
int LC_LayerTreeWidget::invokeLayersRemovalDialog(QStringList &layerNames){
    // layers added, show confirmation dialog

    QString title(QMessageBox::tr("Remove %n layer(s)", "", layerNames.size()));
    QStringList text_lines = {QMessageBox::tr("Listed layers and all entities on them will be removed."), "",
                              QMessageBox::tr("Warning: this action can NOT be undone!"),};
    QStringList detail_lines = {QMessageBox::tr("Layers for removal:"), "",};
    detail_lines << layerNames;

    QMessageBox msgBox(QMessageBox::Warning, title, text_lines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDetailedText(detail_lines.join("\n"));

    int result = msgBox.exec();
    return result;
}
/**
 * Executes layer generic layer edit dialog. If dialog accepted, applies editing results
 * @param pItem layer item for editing
 * @param edit true - editing mode, false - virtual layer rename mode
 */
void LC_LayerTreeWidget::invokeLayerEditOrRenameDialog(LC_LayerTreeItem *pItem, bool edit){
    LC_LayerDialogEx dlg = LC_LayerDialogEx(this, QMessageBox::tr("Layer DialogEx"), layerTreeModel, pItem, layerList);

    RS_Layer* layer = nullptr;
    if (edit){
        dlg.setMode(LC_LayerDialogEx::MODE_EDIT_LAYER);
        layer = pItem->getLayer();
        dlg.setLayerType(pItem->getLayerType());
        dlg.setConstruction(layer->isConstruction());
    }
    else{
        dlg.setMode(LC_LayerDialogEx::MODE_RENAME_VIRTUAL);
    }
    QString originalName = QString(pItem->getName());
    QString path = layerTreeModel->generateLayersDisplayPathString(pItem);

    dlg.setLayerName(originalName);
    dlg.setParentPath(path);
    dlg.setLayer(layer);

    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
      QString newName = dlg.getLayerName();
      if (edit){ // do editing for layer
          // first apply generic attributes
          RS_Pen pen = dlg.getPen();
          layer->setPen(pen);
          layer->setConstruction(dlg.isConstruction());

          // handle rename
          QString layerName = dlg.getLayerName();
          if (originalName != layerName){ // layer is also renamed
              int editedLayerType = dlg.getEditedLayerType();
              layerTreeModel -> renamePrimaryLayer(pItem, layerName, editedLayerType);
          }
          layerList->fireEdit(nullptr);
      }
      else{ // just rename virtual layer
          bool renamed = layerTreeModel -> renameVirtualLayer(pItem, newName);
          if (renamed){
              layerList->fireEdit(nullptr);
          }
      }
    }
}

/**
 * Displays layer dialog for adding new layers and creates new layer base on dialog data.
 * @param parentItem if not null - parent item, so created layer will be under this parent
 * @param layerType - type of layer should be created. If LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE - allows to select type
 */
void LC_LayerTreeWidget::invokeLayerAddDialog(LC_LayerTreeItem *parentItem, int layerType){
    LC_LayerDialogEx dlg = LC_LayerDialogEx(this, QMessageBox::tr("Layer DialogEx"), layerTreeModel, parentItem, layerList);

    // setup dialog first
    int dialogMode = LC_LayerDialogEx::MODE_ADD_CHILD_LAYER;
    switch (layerType){
        case LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE:{
            if (parentItem == nullptr){
                dialogMode = LC_LayerDialogEx::MODE_ADD_LAYER;
            }
            else{
                dialogMode = LC_LayerDialogEx::MODE_ADD_CHILD_LAYER;
            }
            break;
        }
        case LC_LayerTreeItem::DIMENSIONAL:
        case LC_LayerTreeItem::INFORMATIONAL:
        case LC_LayerTreeItem::ALTERNATE_POSITION:{
            dialogMode = LC_LayerDialogEx::MODE_ADD_SECONDARY_LAYER;
            break;
        }
        default:{
            break;
        }
    }
    dlg.setMode(dialogMode);

    // prepare parent path
    QString name = "";
    QString path = "";
    if (parentItem != nullptr){
        name = QString(parentItem->getName());
        path = layerTreeModel->generateLayersDisplayPathString(parentItem);
    }

    dlg.setLayerName(name);
    dlg.setParentPath(path);
    dlg.setLayerType(layerType);

    // temporary layer for editing
    auto* tmpLayer = new RS_Layer("");

    // if layer type is provided - we'll use default pen for it
    RS_Pen defaultPen = layerTreeModel->getOptions() ->getDefaultPen(layerType == LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE ? LC_LayerTreeItem::NORMAL : layerType);
    RS_Pen penCopy = RS_Pen(defaultPen);

    tmpLayer->setPen(penCopy);
    dlg.setLayer(tmpLayer);

    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
      QString layerName = dlg.getLayerName();
      int editedLayerType = dlg.getEditedLayerType();

      // check for duplicate name is performed by the dialog on validate, so here we'll simply create a layer
      QString newLayerName = layerTreeModel->createFullLayerName(parentItem, layerName, editedLayerType, true);

      RS_Pen pen = dlg.getPen();
      tmpLayer->setConstruction(dlg.isConstruction());
      tmpLayer->setName(newLayerName);
      tmpLayer->setPen(pen);
      layerList->add(tmpLayer); // listeners will be invoked there
    }
    else{
        // dialog cancelled, so remove temporary layer
        delete tmpLayer;
    }
}

/**
 *  Displays settings dialog and if needed, saves setting and updates widget
 */
void LC_LayerTreeWidget::invokeSettingsDialog(){
    LC_LayerTreeModelOptions* options = layerTreeModel->getOptions();
    LC_LayerTreeOptionsDialog dlg = LC_LayerTreeOptionsDialog(this, options);
    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->save();
        update();
    }
}
