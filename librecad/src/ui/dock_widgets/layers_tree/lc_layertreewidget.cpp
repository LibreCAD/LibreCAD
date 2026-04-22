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

#include "lc_layertreewidget.h"

#include <QtWidgets>

#include "lc_exportlayersdialogservice.h"
#include "lc_flexlayout.h"
#include "lc_layerdialog_ex.h"
#include "lc_layersexporter.h"
#include "lc_layertreeitem.h"
#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"
#include "lc_layertreeoptionsdialog.h"
#include "lc_layertreeview.h"
#include "lc_widgets_common.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_selection.h"
/**
 * Constructor.
 */
LC_LayerTreeWidget::LC_LayerTreeWidget(const QG_ActionHandler* ah, QWidget* parent, const char* name, const Qt::WindowFlags f):
    LC_GraphicViewAwareWidget(parent, name, f) {

    m_actionHandler = ah;
    m_layerList = nullptr;

    // TODO - should this flag be persistent? Let's keep it transient so far, let's see

    m_flatListMode = false;

    QLayout *layFiltering = initFilterAndSettingsSection();

    initTreeModel();

    m_layerTreeView = initTreeView();

    QLayout *layButtons = initButtonsBar();
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 0, 2, 2);
    lay->setSpacing(0);
    lay->addLayout(layFiltering);
    lay->addLayout(layButtons);
    QSizePolicy policy = m_layerTreeView->sizePolicy();
    policy.setVerticalStretch(1);
    m_layerTreeView->setSizePolicy(policy);
    lay->addWidget(m_layerTreeView);
    this->setLayout(lay);

    updateWidgetSettings();
}

void LC_LayerTreeWidget::initTreeModel(){
    auto *options = new LC_LayerTreeModelOptions();
    options->load();
    m_layerTreeModel = new LC_LayerTreeModel(this, options);
    m_layerTreeModel->setFlatMode(m_flatListMode);
}

/**
 * UI initialization of TreeView. Model should be already created
 * @return
 */
LC_LayerTreeView *LC_LayerTreeWidget::initTreeView(){

    auto *treeView = new LC_LayerTreeView(this);
    treeView->setup(m_layerTreeModel);

    const QFontMetrics fm(font());
    const int itemHeight = fm.height() + 6;

    QHeaderView *headerView{treeView->header()};
    headerView->setMinimumSectionSize(itemHeight);
    headerView->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    headerView->setStretchLastSection(true);
    headerView->hide();

    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_VISIBLE, itemHeight);
    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_VISIBLE, itemHeight);
    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_LOCKED, itemHeight);
    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_PRINT, itemHeight);
    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_CONSTRUCTION, itemHeight);
    treeView->setColumnWidth(LC_LayerTreeModel::COLUMN_COLOR_SAMPLE, itemHeight);

    treeView->setUniformRowHeights(true);
    treeView->setAlternatingRowColors(false);
    treeView->setSelectionMode(QAbstractItemView::NoSelection);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setFocusPolicy(Qt::NoFocus);
    treeView->setMinimumHeight(65);

    treeView->setDragDropMode(QAbstractItemView::InternalMove);
    treeView->setDragEnabled(true);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setAcceptDrops(true);
    treeView->setDropIndicatorShown(true);
    treeView->setExpandsOnDoubleClick(false);

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

#ifndef DONT_FORCE_WIDGETS_CSS
    treeView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif

    connect(treeView, &QTreeView::customContextMenuRequested, this, &LC_LayerTreeWidget::onCustomContextMenu);
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
    layFiltering->setContentsMargins(0, 0, 0, 0);
    layFiltering->setSpacing(2);

    // lineEdit to filter layer list with RegEx
    m_matchLayerName = new QLineEdit(this);
    m_matchLayerName->setReadOnly(false);
    m_matchLayerName->setPlaceholderText(tr("Filter"));
    m_matchLayerName->setClearButtonEnabled(true);
    m_matchLayerName->setToolTip(tr("Looking for matching layer names"));
    connect(m_matchLayerName, &QLineEdit::textChanged, this, &LC_LayerTreeWidget::slotFilteringMaskChanged);

    // TODO - in general, it is possible to use persistent settings for the state, yet not sure it is reasonable
    m_matchModeCheckBox = new QCheckBox(this);
    m_matchModeCheckBox->setText(tr("Highlight Mode"));
    m_matchModeCheckBox->setChecked(true);
    connect(m_matchModeCheckBox, &QCheckBox::clicked, this, &LC_LayerTreeWidget::slotFilteringMaskChanged);

    layFiltering->addWidget(m_matchLayerName);
    layFiltering->addWidget(m_matchModeCheckBox);

    // settings button
    auto* but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/settings.lci"));
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
    auto *layButtons = new LC_FlexLayout(0,3,3);
    layButtons->setContentsMargins(0,0,0,1);
    // show all layer:
    auto but = new QToolButton(this);
    // but->setIcon(QIcon(":/icons/visible.lci"));
    but->setIcon(QIcon(":/icons/visible_all.lci"));
    but->setToolTip(tr("Show all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::showAllLayers);
    layButtons->addWidget(but);

    // hide all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/not_visible_all.lci"));
    but->setToolTip(tr("Hide all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::hideAllLayers);
    layButtons->addWidget(but);

    // toggle secondary layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_vertical.lci"));
    but->setToolTip(tr("Show Secondary Layers"));
    but->setCheckable(true);
    but->setChecked(true); // visible by default
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::toggleSecondaryLayersVisibility);
    layButtons->addWidget(but);
    m_btnShowSecondaryLayers = but;

    // Visible only active
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/select_all.lci"));
    but->setToolTip(tr("Show Active Layer Only"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::showActiveLayerOnly);
    layButtons->addWidget(but);

    // expand all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/order.lci"));

    but->setToolTip(tr("Expand All"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::expandAllLayers);
    layButtons->addWidget(but);
    m_btnExpandAll = but;

    // collapse all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/upmost.lci"));
    but->setToolTip(tr("Collapse All"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::collapseAllLayers);
    layButtons->addWidget(but/*, 10*/);
    m_btnCollapseAll = but;

    // expand all layers
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_aligned.lci"));
    but->setToolTip(tr("Collapse Secondary"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::collapseSecondaryLayers);
    layButtons->addWidget(but);
    m_btnCollapseSecondary = but;

    // unlock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/unlocked.lci"));
    but->setToolTip(tr("Unlock all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::unlockAllLayers);
    layButtons->addWidget(but);

    // lock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/locked.lci"));
    but->setToolTip(tr("Lock all layers"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::lockAllLayers);
    layButtons->addWidget(but);

    // add layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/add.lci"));
    but->setToolTip(tr("Add a layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::addLayer);
    layButtons->addWidget(but);

    // add dimensional layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/dim_horizontal.lci"));
    but->setToolTip(tr("Add dimensions Layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::addDimensionalLayerForActiveLayer);

    m_btnAddDimensional = but;
    layButtons->addWidget(but);

    // remove layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/remove.lci"));
    but->setToolTip(tr("Remove layer"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::removeActiveLayers);
    layButtons->addWidget(but);

    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/rename_active_block.lci"));
    but->setToolTip(tr("Modify layer attributes / rename"));
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::editActiveLayer);
    layButtons->addWidget(but);

    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/down.lci"));
    but->setToolTip(tr("Flat List Mode)"));
    but->setCheckable(true);
    connect(but, &QToolButton::clicked, this, &LC_LayerTreeWidget::toggleFlatView);

    layButtons->addWidget(but);
    m_btnListMode = but;
    updateToolBarButtons();
    return layButtons;
}

/**
 * Sets the RS_LayerList this layer widget should show. If layer list does not equal to previously
 * set one, that leads to complete rebuild of underlying model
 */
void LC_LayerTreeWidget::setLayerList(RS_LayerList *ll){
    if (m_layerList != nullptr) {
        m_layerList->removeListener(this);
    }
    m_layerList = ll;

    if (ll != nullptr){
        m_layerList->addListener(this);
    }

    update();
}

/**
 * Updates the layer box from the layers in the graphic.
 * Primary method used to rebuild model for modifications and
 * updating the UI.
 */
void LC_LayerTreeWidget::update() const {

    RS_DEBUG->print("QG_LayerWidget::update() begin");

    if (m_layerTreeView == nullptr){
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerView");
        return;
    }

    if (m_layerList == nullptr){
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QG_LayerWidget::update: nullptr layerList");
        m_layerTreeModel->setLayerList(nullptr);
        return;
    }

    if (m_withinSelfActivation){
        // layer is activated by our widget, not externally via listener, so we need just modify active
        // path for currently active layer and do not rebuild existing model
        m_layerTreeModel->proceedActiveLayerChanged(m_layerList);
    } else {
        // complete rebuild of the model and update of UI
        const int yPos = m_layerTreeView->verticalScrollBar()->value();
        m_layerTreeView->verticalScrollBar()->setValue(yPos);

        const QStringList treeExpansionState = m_layerTreeView->saveTreeExpansionState();

        // model will be rebuilt as result of setting layer list
        m_layerTreeModel->setLayerList(m_layerList);

        if (!treeExpansionState.isEmpty()){
            m_layerTreeView->restoreTreeExpansionState(treeExpansionState);
        }

        m_layerTreeView->viewport()->update();
    }

    RS_DEBUG->print("QG_LayerWidget::update(): OK");
}

/**
 * Activates the given layer and makes it the active  layer in the layers list.
 */
void LC_LayerTreeWidget::activateLayer(RS_Layer *layer) const {
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if ((layer == nullptr) || (m_layerList == nullptr)){
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layer or layerList");
        return;
    }

    m_graphic->activateLayer(layer, false);
    update();
    m_layerTreeView->viewport()->update();

    //update active layer name in main window status bar
    QC_ApplicationWindow::getAppWindow()->slotUpdateActiveLayer();

    RS_DEBUG->print("QG_LayerWidget::activateLayer() end");
}

//---------------- three expanded state control ----------------------

void LC_LayerTreeWidget::collapseAllLayers() const {
    m_layerTreeView->collapseAll();
}

void LC_LayerTreeWidget::expandAllLayers() const {
    m_layerTreeView->expandAll();
}

void LC_LayerTreeWidget::expandItems(const int depth) const {
    if (depth == -1){
        m_layerTreeView->expandAll();
    } else {
        m_layerTreeView->expandToDepth(depth);
    }
}
/**
 * Collapsing all secondary items  - all that are not normal or virtual
 */
void LC_LayerTreeWidget::collapseSecondaryLayers() const {
    m_layerTreeView->setUpdatesEnabled(false);
        foreach (QModelIndex index, m_layerTreeModel->getPersistentIndexList()) {
            if (m_layerTreeView->isExpanded(index)){
                const LC_LayerTreeItem *childItem = m_layerTreeModel->getItemForIndex(index);
                const int type = childItem->getLayerType();
                if (type == RS_Layer::LayerType::NORMAL){
                    m_layerTreeView->collapse(index);
                }
            }
        }
    m_layerTreeView->setUpdatesEnabled(true);
    m_layerTreeView->viewport()->update();
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
void LC_LayerTreeWidget::slotTreeDoubleClicked(const QModelIndex &index /*const QString& layerName*/){
    if (index.isValid()){
        const LC_LayerTreeItem *layerItem = m_layerTreeModel->getItemForIndex(index);
        if (layerItem->isVirtual()){ // for virtual layer, we expand children
            // From Qt 5.13, another method might be used
            //  layerTreeView -> expandRecursively(index,1);

            // TODO - think about expansion/collapsing on double click
             m_layerTreeView->setUpdatesEnabled(false);
            m_layerTreeView->expandChildren(index);
            m_layerTreeView->setUpdatesEnabled(true);
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
void LC_LayerTreeWidget::slotTreeClicked(const QModelIndex &layerIdx /*const QString& layerName*/){

    if (!layerIdx.isValid() || m_layerList == nullptr){
        return;
    }

    auto *layerItem = static_cast<LC_LayerTreeItem *>(layerIdx.internalPointer());

    // use translated column index to ensure proper column detection
    const int column = m_layerTreeModel->translateColumn(layerIdx.column());

    const QList<RS_Layer *> layers;
    QList<RS_Layer *> layersToDisable = layers;
    QList<RS_Layer *> layersToEnable = layers;

    const auto acceptAll = LC_LayerTreeItemAcceptor();

    if (layerItem->isVirtual()){
        switch (column) {
            case LC_LayerTreeModel::COLUMN_VISIBLE: {
                // we use slightly different logic for visibility check
                // comparing to other modes.
                // here we rely on the current state of node

                const bool isVisible = layerItem->isVisible();
                if (isVisible){
                    // we'd like to disable layer - so we should disable its secondary children too
                    layerItem->collectLayers(layersToDisable, acceptAll);
                } else { // we enable this level, sub-virtual layers and normal layers - yet without sub-layers
                    // Therefore we select all child except secondary
                    layerItem->collectLayers(layersToEnable, QG_LayerTreeItemAcceptorSecondary(false));
                }
                manageLayersVisibilityFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::COLUMN_LOCKED:{
                 if (layerItem->isLocked()){
                     layerItem->collectLayers(layersToDisable, acceptAll);
                 }
                 else{
                     layerItem->collectLayers(layersToEnable, acceptAll);
                 }
                manageLayersLockFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::COLUMN_PRINT:{
                if (layerItem->isPrint()){
                    layerItem->collectLayers(layersToDisable, acceptAll);
                }
                else{
                    layerItem->collectLayers(layersToEnable, acceptAll);
                }
                manageLayersPrintFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::COLUMN_CONSTRUCTION: {
                if (layerItem->isConstruction()){
                    layerItem->collectLayers(layersToDisable, acceptAll);
                }
                else{
                    layerItem->collectLayers(layersToEnable, acceptAll);
                }
                manageLayersConstructionFlag(layersToEnable, layersToDisable, false);
                break;
            }
            case LC_LayerTreeModel::COLUMN_NAME: {
                // just convenient way to expand children of this layer
                m_layerTreeView->expand(layerIdx);
                break;
            }
            default:
                break;
        }
    } else { // real layer
        RS_Layer *lay = layerItem->getLayer();
        if (lay == nullptr) {
            return;
        }

        if (column == LC_LayerTreeModel::COLUMN_NAME){
            // Layer activation is here

            // set flag that activation of layer initiated by the widget.
            // this will prevent complete re-build of the model as soon as
            // corresponding listener will be invoked, so only the active path
            // to selected layer will be updated.

            m_withinSelfActivation = true;
            m_graphic->activateLayer(lay, true);
            m_withinSelfActivation = false;

            return;
        }

        // handling clicks on flags
        const int layerType = layerItem->getLayerType();
        const bool normalLayerWithChildren = layerType == RS_Layer::LayerType::NORMAL && layerItem->childCount() > 0;

        switch (column) {
            case LC_LayerTreeModel::COLUMN_VISIBLE: {
                if (normalLayerWithChildren){
                    const bool isVisible = layerItem->isVisible();
                    if (isVisible){
                        // we'd like to disable layer - so we should disable its secondary children
                        layerItem->collectLayers(layersToDisable, acceptAll);
                        layersToDisable << lay;
                        manageLayersVisibilityFlag(layersToEnable, layersToDisable, false);
                    } else {
                        // we enable ony the layer itself, without touching it secondary children
                        m_actionHandler->setCurrentAction(RS2::ActionLayersToggleView, lay);
                    }
                } else {
                    m_actionHandler->setCurrentAction(RS2::ActionLayersToggleView, lay);
                }
                break;
            }
            case LC_LayerTreeModel::COLUMN_LOCKED:
                if (normalLayerWithChildren){
                    if (layerItem->isLocked()){
                        layerItem->collectLayers(layersToDisable, acceptAll, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, acceptAll, true);
                    }
                    manageLayersLockFlag(layersToEnable, layersToDisable, false);
                }
                else{
                    m_actionHandler->setCurrentAction(RS2::ActionLayersToggleLock, lay);
                }
                break;
            case LC_LayerTreeModel::COLUMN_PRINT:
                if (normalLayerWithChildren){
                    if (layerItem->isPrint()){
                        layerItem->collectLayers(layersToDisable, acceptAll, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, acceptAll, true);
                    }
                    manageLayersPrintFlag(layersToEnable, layersToDisable, false);
                }
                else{
                    m_actionHandler->setCurrentAction(RS2::ActionLayersTogglePrint, lay);
                }
                break;
            case LC_LayerTreeModel::COLUMN_CONSTRUCTION:
                if (normalLayerWithChildren){
                    if (layerItem->isConstruction()){
                        layerItem->collectLayers(layersToDisable, acceptAll, true);
                    }
                    else{
                        layerItem->collectLayers(layersToEnable, acceptAll, true);
                    }
                    manageLayersConstructionFlag(layersToEnable, layersToDisable, false);
                }
                else {
                    m_actionHandler->setCurrentAction(RS2::ActionLayersToggleConstruction, lay);
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
void LC_LayerTreeWidget::slotFilteringMaskChanged() const {
    const QString mask = m_matchLayerName->text();
    const bool highlightMode = m_matchModeCheckBox->isChecked();
    m_layerTreeModel->setFilteringRegexp(mask, highlightMode);
    update();
}

/**
 * Shows a context menu for the layer widget. Launched with a right click.
 * Menu is dynamic and is based on the particular item on which click is performed
 */
void LC_LayerTreeWidget::onCustomContextMenu(const QPoint &point){

    if (m_actionHandler != nullptr){
        auto *contextMenu = new QMenu(this);
        /*auto *caption = new QLabel(tr("Layer Menu"), this);
        QPalette palette;
        palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
        palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
        caption->setPalette(palette);
        caption->setAlignment(Qt::AlignCenter);*/

        // Actions for all layers:

        using ActionMemberFunc = void (LC_LayerTreeWidget::*)();
        const auto addActionFunc = [this, &contextMenu](const QString& iconName, const QString& name, ActionMemberFunc func) {
            contextMenu->addAction(QIcon(":/icons/" + iconName + ".lci"), name, this, func);
        };

        const QModelIndex index = m_layerTreeView->indexAt(point);

        if (index.isValid()){
            LC_LayerTreeItem *layerItem = m_layerTreeModel->getItemForIndex(index);
            const int layerType = layerItem->getLayerType();
            const bool isVirtual = layerItem->isVirtual();
            const QString title = "Layer: " + layerItem->getName();
            contextMenu->setTitle(title);

            if (isVirtual){
                addActionFunc("add", tr("&Add Child Layer"), &LC_LayerTreeWidget::addChildLayerForSelectedItem);
                addActionFunc("rename_active_block", tr("&Rename"), &LC_LayerTreeWidget::renameVirtualLayer);
                addActionFunc("remove", tr("&Remove Layers (Sub-Tree)"),  &LC_LayerTreeWidget::removeLayersForSelectedItem);
                contextMenu->addSeparator();
                addActionFunc("copy", tr("&Copy Structure (Sub-Tree)"), &LC_LayerTreeWidget::createLayerCopy);
                addActionFunc("paste", tr("&Duplicate Content (Sub-Tree)"), &LC_LayerTreeWidget::createLayerDuplicate);
                // TODO - should we take care of virtual layer's visibility somehow?
                addActionFunc("deselect_layer", tr("&Select Entities (Sub-Tree)"), &LC_LayerTreeWidget::selectLayersEntities);
            } else {

                const bool nonZeroLayer = !layerItem->isZero();

                addActionFunc("rename_active_block", tr("&Edit Layer &Attributes"), &LC_LayerTreeWidget::editSelectedLayer);
                if (nonZeroLayer){
                    addActionFunc("remove", tr("&Remove Layer"), &LC_LayerTreeWidget::removeLayersForSelectedItem);
                }
                contextMenu->addSeparator();

                if (layerType == RS_Layer::LayerType::NORMAL){
                    if (nonZeroLayer){
                        bool hasItems = false;

                        if (!layerItem->hasChildOfType(RS_Layer::LayerType::DIMENSIONAL)){
                            addActionFunc("dim_horizontal", tr("&Add Dimensions Sub-Layer"), &LC_LayerTreeWidget::addDimensionalLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!layerItem->hasChildOfType(RS_Layer::LayerType::INFORMATIONAL)){
                            addActionFunc("mtext", tr("&Add Info Sub-Layer"), &LC_LayerTreeWidget::addInformationalLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!layerItem->hasChildOfType(RS_Layer::LayerType::ALTERNATE_POSITION)){
                             addActionFunc("rotate", tr("&Add Alternative View Sub-Layer"),
                                                   &LC_LayerTreeWidget::addAddAlternativePositionLayerForSelectedItem);
                            hasItems = true;
                        }
                        if (!m_flatListMode){
                            if (layerItem->childCount() > 0){
                                addActionFunc("remove", tr("&Remove Sub-layers"), &LC_LayerTreeWidget::removeChildLayersForSelected);
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
                    if (layerType != RS_Layer::LayerType::DIMENSIONAL){
                        contextMenu->addAction(tr("Convert to Dimensional Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToDimensional);
                    }
                    if (layerType != RS_Layer::LayerType::INFORMATIONAL){
                        contextMenu->addAction(tr("Convert to Info Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToInformational);
                    }
                    if (layerType != RS_Layer::LayerType::ALTERNATE_POSITION){
                        contextMenu->addAction(tr("Convert to Alternative Position Layer"), this, &LC_LayerTreeWidget::convertLayerTypeToAlternativePosition);
                    }
                    contextMenu->addSeparator();
                }

                if (layerItem->isVisible() && !layerItem->isLocked()){
                    addActionFunc("deselect_layer",tr("&Select Layer's Entities"), &LC_LayerTreeWidget::selectLayersEntities);
                }
                contextMenu->addSeparator();
                addActionFunc("copy", tr("&Create Layer Copy"), &LC_LayerTreeWidget::createLayerCopy);
                addActionFunc("paste", tr("&Duplicate Layer With Content"),  &LC_LayerTreeWidget::createLayerDuplicate);

                contextMenu->addSeparator();
                if (!layerItem->isLocked()){
                    addActionFunc("move_copy", tr("Move Selection to Layer"),  &LC_LayerTreeWidget::moveSelectionToLayer);
                    addActionFunc("duplicate", tr("Duplicate Selection to Layer"),  &LC_LayerTreeWidget::duplicateSelectionToLayer);
                    contextMenu->addSeparator();
                }
            }
            contextMenu->addSeparator();
        } else {
            // click is not on item
            addActionFunc("add", tr("&Add Layer"), &LC_LayerTreeWidget::addLayer);
        }
        addActionFunc("visible", tr("&Freeze Others Layers"), &LC_LayerTreeWidget::hideOtherThanSelectedLayers);
        addActionFunc("visible_all",   tr("&Defreeze All Layers"),  &LC_LayerTreeWidget::showAllLayers);
        addActionFunc("not_visible_all",tr("&Freeze All Layers"), &LC_LayerTreeWidget::hideAllLayers);
        addActionFunc("unlocked", tr("&Unlock All Layers"), &LC_LayerTreeWidget::unlockAllLayers);
        addActionFunc("locked", tr("&Lock All Layers"), &LC_LayerTreeWidget::lockAllLayers);
        addActionFunc("print", tr("Enable &Printing All Layers"),  &LC_LayerTreeWidget::printAllLayers);
        addActionFunc("noprint", tr("&Disable Printing All Layers"), &LC_LayerTreeWidget::noPrintAllLayers);
        contextMenu->addSeparator();
        if (index.isValid()) {
            addActionFunc("layer_export_single",tr("&Export Single Layer"), &LC_LayerTreeWidget::exportSelectedLayer);
            if (!m_flatListMode){
                const LC_LayerTreeItem *layerItem = m_layerTreeModel->getItemForIndex(index);
                if (layerItem->childCount() > 0){
                    addActionFunc("layer_export_selected", tr("&Export Layer Sub-Tree"), &LC_LayerTreeWidget::exportLayerSubTree);
                }
            }
        }
        addActionFunc("layer_export_visible", tr("Export &Visible Layer(s)"),  &LC_LayerTreeWidget::exportVisibleLayers);
        contextMenu->addSeparator();
        addActionFunc("close_all", tr("&Find And Remove Empty Layers"), &LC_LayerTreeWidget::removeEmptyLayers);
        contextMenu->exec(QCursor::pos());
        delete contextMenu;
    }
//     e->accept();
}

// ------------- Layers Visibility Control

/**
 * Makes all layers visible
 */
void LC_LayerTreeWidget::showAllLayers()  {
    m_btnShowSecondaryLayers->setChecked(true);
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToShow = m_layerTreeModel->collectLayers(acceptAll);
    const QList<RS_Layer *> layersToHide;
    manageLayersVisibilityFlag(layersToShow, layersToHide, false);
}

/**
 * Hides all layers except actual one
 */
void LC_LayerTreeWidget::showActiveLayerOnly() const {
    if (nullptr != m_layerList){
        const auto activeLayer = m_graphic->getActiveLayer();
        if (activeLayer != nullptr){
            const QG_LayerTreeItemAcceptorSameLayerAs acceptAllExcept(activeLayer, true);
            const QList<RS_Layer *> layersToHide = m_layerTreeModel->collectLayers(acceptAllExcept);
            QList<RS_Layer *> layersToShow;
            layersToShow << activeLayer;

            manageLayersVisibilityFlag(layersToShow, layersToHide, false);
        }
    }
}
/**
 * Makes all layers invisible
 */
void LC_LayerTreeWidget::hideAllLayers() {
    m_btnShowSecondaryLayers->setChecked(false);
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToShow;
    const QList<RS_Layer *> layersToHide = m_layerTreeModel->collectLayers(acceptAll);
    manageLayersVisibilityFlag(layersToShow, layersToHide, false);
}

/**
 * Hides all layers except selected layer (it might be different from actual one)
 */
void LC_LayerTreeWidget::hideOtherThanSelectedLayers()  {
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        const LC_LayerTreeItemAcceptor acceptAll;
        QList<RS_Layer *> layersToHide = m_layerTreeModel->collectLayers(acceptAll);

        QList<RS_Layer *> layersToShow;
        currentItem->collectLayers(layersToShow, acceptAll, true);
        const int count = layersToShow.count();
        for (int i = 0; i< count; i++){
            RS_Layer* layer = layersToShow.at(i);
            layersToHide.removeAll(layer);
        }
        if (count > 0){
            m_graphic->activateLayer(layersToShow.at(0), false);
        }
        manageLayersVisibilityFlag(layersToShow, layersToHide, false);
    }
}
/**
 * Toggles visibility for secondary layers (dimensions, info and alternative position)
 */
void LC_LayerTreeWidget::toggleSecondaryLayersVisibility() const {
    LC_LayerTreeItem *root = m_layerTreeModel->getRoot();
    QList<LC_LayerTreeItem *> secondaryItems;
    const QG_LayerTreeItemAcceptorSecondary acceptor;
    root->collectDescendantChildren(secondaryItems, acceptor, false);

    const bool showSecondary = m_btnShowSecondaryLayers->isChecked();
    const int count = secondaryItems.count();

    if (count > 0){
        QList<RS_Layer *> layersToEnable;
        QList<RS_Layer *> layersToDisable;

        for (int i = 0; i < count; i++) {
            const LC_LayerTreeItem *layerItem = secondaryItems.at(i);

            RS_Layer *layer = layerItem->getLayer();
            const LC_LayerTreeItem *primaryLayerItem = layerItem->getPrimaryItem();

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
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToLock;
    const QList<RS_Layer *> layersToUnlock = m_layerTreeModel->collectLayers(acceptAll);
    manageLayersLockFlag(layersToLock, layersToUnlock, false);
}
/**
 * All matched layers become locked
 */
void LC_LayerTreeWidget::lockAllLayers(){
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToLock = m_layerTreeModel->collectLayers(acceptAll);
    const QList<RS_Layer *> layersToUnlock;
    manageLayersLockFlag(layersToLock, layersToUnlock, false);
}
/**
 * All matched layers will be printable
 */
void LC_LayerTreeWidget::printAllLayers(){
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToPrint = m_layerTreeModel->collectLayers(acceptAll);
    const QList<RS_Layer *> layersToNoPrint;
    manageLayersPrintFlag(layersToPrint, layersToNoPrint, false);
}

/**
 * All matched layers will be not printable
 */
void LC_LayerTreeWidget::noPrintAllLayers(){
    const LC_LayerTreeItemAcceptor acceptAll;
    const QList<RS_Layer *> layersToPrint;
    const QList<RS_Layer *> layersToNoPrint= m_layerTreeModel->collectLayers(acceptAll);
    manageLayersPrintFlag(layersToPrint, layersToNoPrint, false);
}

/**
 * Collects layers with no entities and allows the user to remove them via layer removal dialog
 */
void LC_LayerTreeWidget::removeEmptyLayers(){

    // first we inspect all entities ao collect found layers in set
    QSet<RS_Layer*> foundLayers;
    for (const auto en: *m_document) {
        RS_Layer *l = en->getLayer(true);
        if (l != nullptr){
            foundLayers.insert(l);
        }
    }

    // compare layers provided by layers list with set of found layers
    QList<RS_Layer*> layersWithNoEntities;
    const unsigned int layersCount = m_layerList->count();
    for (unsigned int i = 0; i< layersCount; i++){
        RS_Layer* l = m_layerList->at(i);
        if (foundLayers.contains(l)){
            // do nothing, some entities are found on this layer
        }
        else{
            layersWithNoEntities << l;
        }
    }

    const int layersWithNoEntitiesCount = layersWithNoEntities.count();
    if (layersWithNoEntitiesCount > 0){ // empty layers found
        bool emptyLayerIsFiltered = false;

        // collect list of tree items for removal. Items are needed for proper showing display name in removal dialog...
        // TODO - still need to think about layer removal dialog and layer identification there. Probably it's better to show full name instead of display name

        QList<LC_LayerTreeItem*> itemsToRemove;
        for (int i = 0; i < layersWithNoEntitiesCount; i++){
            RS_Layer* l = layersWithNoEntities.at(i);
            LC_LayerTreeItem* item = m_layerTreeModel->getItemForLayer(l);
            if (item == nullptr){
                if (m_layerTreeModel->isRegexpApplied()){
                    // this means that there is regexp and layer is filtered...
                    emptyLayerIsFiltered = true;
                }
            }
            else{
                itemsToRemove<<item;
            }
        }

        if (emptyLayerIsFiltered){ // not too good to remove something not visible, so let the user one more chance to check
            const QString title(QMessageBox::tr("Remove empty layers"));
            const QString text = QMessageBox::tr("Layer(s) without entities found, yet they are filtered and not visible.\n\nClear filtering mask and repeat.");
            QMessageBox msgBox(QMessageBox::Information, title, text, QMessageBox::Ok);
            msgBox.exec();
        }
        else{// do actual removal
            doRemoveLayerItems(itemsToRemove);
        }
    }
    else{ // no layers with no entities, just show message about that
        const QString title(QMessageBox::tr("Remove empty layers"));
        const QString text = QMessageBox::tr("No layers without entities found, nothing to remove.");

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
    m_layerTreeView->viewport()->update();
}

void LC_LayerTreeWidget::layerRemoved(RS_Layer *){
    RS_DEBUG->print("LC_LayerTreeWidget::layerRemoved()");
    update();
    activateLayer(m_layerList->at(0));
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
void LC_LayerTreeWidget::onDragEnterEvent(const QModelIndex &dropIndex) const {
    // save source item for which drag is invoked
    LC_LayerTreeItem *currentlyDraggingItem = m_layerTreeModel->getItemForIndex(dropIndex);
    // LC_ERR << "Drag Enter for " << currentlyDraggingItem->getName() << "  " << currentlyDraggingItem->getFullName();

    m_layerTreeModel->setCurrentlyDraggingItem(currentlyDraggingItem);
}
/**
 * On Drop event.
 * @param dropIndex
 * @param position
 */
void LC_LayerTreeWidget::onDropEvent(const QModelIndex &dropIndex, const DropIndicatorPosition position) const {
    RS_DEBUG->print(RS_Debug::D_WARNING, "onDropEvent");
    LC_LayerTreeItem *currentlyDraggingItem = m_layerTreeModel->getCurrentlyDraggingItem();
    if (currentlyDraggingItem != nullptr){
        LC_LayerTreeItem *destinationItem = nullptr;
        if (dropIndex.isValid()){ // going to drop on other item
            destinationItem = m_layerTreeModel->getItemForIndex(dropIndex);

        } else if (position == DropIndicatorPosition::OnViewport){
            // drop is on viewport, so we'll add source to root item
            destinationItem = m_layerTreeModel->getRoot();
        }

        if (destinationItem != nullptr){
            // do actual rename
            const bool layersRenamed = m_layerTreeModel->performReStructure(currentlyDraggingItem, destinationItem);
            if (layersRenamed){
                m_layerTreeView->expand(dropIndex);
                m_layerList->fireLayerEdited(nullptr);
            }
        }
        m_layerTreeModel->setCurrentlyDraggingItem(nullptr);
    }
}

//---------------- context menu processing ----------------------

//--------------- Adding layers -----------------------

/**
 * Creates new information layer for selected item
 */
void LC_LayerTreeWidget::addInformationalLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(RS_Layer::LayerType::INFORMATIONAL);
}

/**
 * Creates new alternative position layer for selected item
 */
void LC_LayerTreeWidget::addAddAlternativePositionLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(RS_Layer::LayerType::ALTERNATE_POSITION);
}

/**
 * creates nwe dimensional layer for selected item
 */
void LC_LayerTreeWidget::addDimensionalLayerForSelectedItem(){
    doAddSecondaryLevelForSelectedItem(RS_Layer::LayerType::DIMENSIONAL);
}

/**
 * Adds secondary layer with required type
 * @param layerType new layer type to be created
 */
void LC_LayerTreeWidget::doAddSecondaryLevelForSelectedItem(const int layerType){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
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
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        invokeLayerAddDialog(currentItem, RS_Layer::NOT_DEFINED_LAYER_TYPE);
    }
}

/**
 * Adds new layer
 */
void LC_LayerTreeWidget::addLayer(){
    invokeLayerAddDialog(nullptr, RS_Layer::NOT_DEFINED_LAYER_TYPE);
}
/**
 * Adds dimensional layer for currently active layer, if that layer is normal.
 * If dimensional layer already exists - just shows a dialog, otherwise - invokes normal layer creation flow
 */
void LC_LayerTreeWidget::addDimensionalLayerForActiveLayer(){
    if (nullptr != m_layerList){
        const auto activeLayer = m_graphic->getActiveLayer();
        if (activeLayer != nullptr){
            LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                if (currentItem->getLayerType() == RS_Layer::LayerType::NORMAL){
                    constexpr int newLayerType = RS_Layer::LayerType::DIMENSIONAL;
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
    if (nullptr != m_layerList){
        const auto activeLayer = m_graphic->getActiveLayer();
        if (activeLayer != nullptr){
            LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForLayer(activeLayer);
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
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        invokeLayerEditOrRenameDialog(currentItem, true);
    }
}
/**
 * Renames virtual layer (and so, renames all descendants of it accordingly)
 */
void LC_LayerTreeWidget::renameVirtualLayer(){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
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
void LC_LayerTreeWidget::moveOrDuplicateSelectionToSelectedItemLayer(const bool duplicate, const QString &title){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        const LC_LayerTreeItem *layerItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        if (!layerItem->isLocked()){
            if (!layerItem->isVirtual()){
                const bool resolvePens = QMessageBox::question(this, title,
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
    doConvertSelectedItemLayerToNewType(RS_Layer::LayerType::DIMENSIONAL);
}

/**
 * Converts selected layer to informational layer type
 */
void LC_LayerTreeWidget::convertLayerTypeToInformational(){
    doConvertSelectedItemLayerToNewType(RS_Layer::LayerType::INFORMATIONAL);
}

/**
* Converts selected layer to alternative position layer type
*/
void LC_LayerTreeWidget::convertLayerTypeToAlternativePosition(){
    doConvertSelectedItemLayerToNewType(RS_Layer::LayerType::ALTERNATE_POSITION);
}

/**
 * Converts secondary layer to normal type
 */
void LC_LayerTreeWidget::convertLayerTypeToNormal(){
    doConvertSelectedItemLayerToNewType(RS_Layer::LayerType::NORMAL);
}

/**
 * Utility method for actual conversion of selected item to new layer type.
 * Just relies on the model for renames, and fires model update
 * @param newType
 */
void LC_LayerTreeWidget::doConvertSelectedItemLayerToNewType(const int newType) const {
    const QModelIndex selectedItem = getSelectedItemIndex();
    const bool converted = m_layerTreeModel->convertToType(selectedItem, newType);
    if (converted){
        m_layerList->fireLayerEdited(nullptr);
    }
}

//----------- Layers removal --------
/**
 * Removes layers (including descendants) for selected item
 */
void LC_LayerTreeWidget::removeLayersForSelectedItem(){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        doRemoveLayersFromSource(currentItem, false);
    }
}

/**
 * Removes descendants for selected layer item. Selected layer survives
 */
void LC_LayerTreeWidget::removeChildLayersForSelected(){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        doRemoveLayersFromSource(currentItem, true);
    }
}

/**
 * Removes active layer (as well as it descendants)
 */
void LC_LayerTreeWidget::removeActiveLayers(){
    if (nullptr != m_layerList){
        const auto activeLayer = m_graphic->getActiveLayer();
        if (activeLayer != nullptr){
            LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                doRemoveLayersFromSource(currentItem, false);
            }
        }
    }
}

void LC_LayerTreeWidget::removeActiveLayer(bool removeWithChildren){
    if (nullptr != m_layerList){
        const auto activeLayer = m_graphic->getActiveLayer();
        if (activeLayer != nullptr){
            LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForLayer(activeLayer);
            if (currentItem != nullptr){
                if (removeWithChildren) {
                    doRemoveLayersFromSource(currentItem, false);
                }
                else {
                    QList<LC_LayerTreeItem*> layersToRemove;
                    layersToRemove.push_back(currentItem);
                    doRemoveLayerItems(layersToRemove);
                }
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
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        doCreateLayersCopy(selectedItemIndex, false);
    }
}

/**
 * Creates a copy of selected item and its descendants, and also creates copies for all entities that belongs
 * to original layers and add these entities' copies to appropriate layers copies
 */
void LC_LayerTreeWidget::createLayerDuplicate(){
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()){
        doCreateLayersCopy(selectedItemIndex, true);
    }
}
/**
 * Switches between hierarchical tree and flat view for layers list
 */
void LC_LayerTreeWidget::toggleFlatView(){
    m_flatListMode = !m_flatListMode;
    m_layerTreeModel->setFlatMode(m_flatListMode);
    update();
    updateToolBarButtons();
    // TODO - add settings for expansion depth?
    constexpr int expandDepth = 0;
    expandItems(expandDepth);
}
/**
 * Utility method that updates visibility/state for buttons in toolbar according to current view mode
 */
void LC_LayerTreeWidget::updateToolBarButtons() const {
    m_btnCollapseSecondary->setVisible(!m_flatListMode);
    m_btnCollapseAll->setVisible(!m_flatListMode);
    m_btnExpandAll->setVisible(!m_flatListMode);
    m_btnListMode->setChecked(m_flatListMode);
    m_btnAddDimensional-> setVisible(!m_flatListMode);
}

/**
 * Returns model index for currently selected item
 * @return
 */
QModelIndex LC_LayerTreeWidget::getSelectedItemIndex() const {
    QModelIndex result;
    const QModelIndexList selectedIndexes = m_layerTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.size() == 1){ // only one selected item is expected
        result = selectedIndexes.at(0);
    }
    return result;
}

/**
 * Utility method that returns list of visible layers for selected item and its descendants
 * @return list of layers
 */
QList<RS_Layer *> LC_LayerTreeWidget::collectLayersForSelectedItem() const {
    QList<RS_Layer *> layersList;
    const QModelIndex selectedIndex = getSelectedItemIndex();
    if (selectedIndex.isValid()){
        LC_LayerTreeItem *layerItem = m_layerTreeModel->getItemForIndex(selectedIndex);
        const QG_LayerTreeItemAcceptorVisible acceptVisible;
        layerItem->collectLayers(layersList, acceptVisible, true);
    }
    return layersList;
}

/**
 * Utility method that deselects entities on locked layer. Actually, similar code is located in specific actions.
 * Included there for consistency and reducing dependencies to other codebase
 * TODO - in general, a refactoring and creation of some utility within the engine is reasonable for such kind of code
 * @param layer
 */
void LC_LayerTreeWidget::deselectEntitiesOnLockedLayer(RS_Layer *layer) const {
    if (layer == nullptr) {
        return;
    }
    if (!layer->isLocked()) {
        return;
    }

    RS_Selection::unselectLayer(m_document, m_graphicView->getViewPort(), layer);
    redrawView();
}

/**
 * Utility method that deselects entities on given layer.
 * @param layer
 */
void LC_LayerTreeWidget::deselectEntities(RS_Layer *layer) const {
    if (layer == nullptr) {
        return;
    }
    RS_Selection::unselectLayer(m_document, m_graphicView->getViewPort(), layer);
    redrawView();
}

/**
 * Either makes provide layers visible and invisible, or just toggles visibility flag for given layers.
 * @param layersToEnable list of layers to make visible
 * @param layersToDisable  list of layers to hide
 * @param toggleMode - flag whether toggle mode should be called internally. if true, list in layersToDisable is ignored
 */
void LC_LayerTreeWidget::manageLayersVisibilityFlag(const QList<RS_Layer *> &layersToEnable, const QList<RS_Layer *> &layersToDisable, const bool toggleMode) const {
    if (m_graphicView != nullptr){
        if (toggleMode){
            m_graphic->toggleFreezeLayers(layersToEnable);
        } else {
            m_graphic->setFreezeLayers(layersToEnable, layersToDisable);
        }
        m_document->updateInserts();
        m_document->calculateBorders();
    }
}
/**
 * Either makes provide layers locked and unlocked, or just toggles lock flag for given layers.
 * @param layersToLockOrToggle
 * @param layersToUnlock
 * @param toggleMode - flag whether toggle mode should be called internally. if true, list in layersToDisable is ignored
 */
void LC_LayerTreeWidget::manageLayersLockFlag(const QList<RS_Layer *> &layersToLockOrToggle, const QList<RS_Layer *> &layersToUnlock, const bool toggleMode){
    int count = 0;
    QList<RS_Layer *> layersForEntitiesUpdate;
    if (toggleMode){
        m_graphic->toggleLockLayers(layersToLockOrToggle);
        count = layersToLockOrToggle.count();
        layersForEntitiesUpdate = layersToLockOrToggle;
    } else {
        m_graphic->setLockLayers(layersToUnlock, layersToLockOrToggle);
        count = layersToUnlock.count();
        layersForEntitiesUpdate = layersToUnlock;
    }

    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer != nullptr){
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
void LC_LayerTreeWidget::manageLayersConstructionFlag(const QList<RS_Layer *> &layersToBeConstruction, const QList<RS_Layer *> &layersNonConstruction, const bool toggleMode){
    QList<RS_Layer *> layersForEntitiesUpdate;
     if (toggleMode){
        m_graphic->toggleConstructionLayers(layersToBeConstruction);
        layersForEntitiesUpdate = layersToBeConstruction;
    } else {
         m_graphic->setConstructionLayers(layersNonConstruction, layersToBeConstruction);
         layersForEntitiesUpdate = layersNonConstruction;
    }
    const int count = layersForEntitiesUpdate.count();
    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer!= nullptr){
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
void LC_LayerTreeWidget::manageLayersPrintFlag(const QList<RS_Layer *> &layersToPrint, const QList<RS_Layer *> &layersNoPrint, const bool toggleMode){
    QList<RS_Layer *> layersForEntitiesUpdate;
    if (toggleMode){
       m_graphic->togglePrintLayers(layersToPrint);
       layersForEntitiesUpdate = layersToPrint;
    }
    else{
        m_graphic->setPrintLayers(layersNoPrint, layersToPrint);
        layersForEntitiesUpdate = layersNoPrint;
    }
    const int count = layersForEntitiesUpdate.count();
    for (int i = 0; i < count; i++) {
        RS_Layer *layer = layersForEntitiesUpdate.at(i);
        if (layer != nullptr){
            deselectEntities(layer);
        }
    }
}

/**
 * Selects entities that belongs to provide list of layers
 * @param layers layers on which entities should be selected
 */
void LC_LayerTreeWidget::doSelectLayersEntities(QList<RS_Layer *> &layers) const {
    const RS_Selection sel(m_graphicView);
    sel.selectIfMatched(m_document->getEntityList(), true, [layers](const RS_Entity* en)-> bool {
        if (en != nullptr) {
            const auto entityLayer = en->getLayer(false);
            if (entityLayer != nullptr && !entityLayer->isLocked() && en->isVisible()) {
                return layers.contains(entityLayer);
            }
        }
        return false;
    });

    redrawView();
}

/**
 * Creates a copy of layers structure denoted by given modelIndex (selected level plus it descendants)
 * @param sourceIndex model index for topmost item to copy
 * @param duplicateEntities if true, entities on copied layers will be also duplicated
 */
void LC_LayerTreeWidget::doCreateLayersCopy(const QModelIndex &sourceIndex, bool duplicateEntities){
    QHash<RS_Layer *, RS_Layer *> layersMap = m_layerTreeModel->createLayersCopy(sourceIndex);
    if (!layersMap.empty()) {
        m_graphic->undoableModify(m_graphicView->getViewPort(), [this, layersMap,duplicateEntities](LC_DocumentModificationBatch& ctx)-> bool {
                                      QHashIterator<RS_Layer*, RS_Layer*> iter{layersMap};
                                      while (iter.hasNext()) {
                                          iter.next();
                                          const RS_Layer* sourceLayer = iter.key();
                                          RS_Layer* copyLayer   = iter.value();
                                          copyLayerAttributes(copyLayer, sourceLayer);
                                          if (duplicateEntities) {
                                              duplicateLayerEntities(sourceLayer, copyLayer, ctx);
                                          }
                                          // that's pretty ugly that there are no batch operations in LayerList....
                                          // it may lead to exceeding invocation of listeners
                                          m_layerList->add(copyLayer); // fixme - sand - add batch invocation for layer's list modification!!
                                      }
                                      return true;
                                  }
                                  // fixme - selection - UNDO END
            );

        // just additional invocation of listeners on layer list...
        m_layerList->fireLayerEdited(nullptr);
    }
}

/**
 * Method creates duplicates of entities on source layer on copy layer
 * @param sourceLayer  layer which is copied
 * @param copyLayer  created copy of source layer
 * @param ctx
 */
void LC_LayerTreeWidget::duplicateLayerEntities(const RS_Layer *sourceLayer, RS_Layer *copyLayer, LC_DocumentModificationBatch& ctx) const {
    for (const auto entity: *m_document) {
        const RS_Layer *layer = entity->getLayer(true);
        if (layer != nullptr && layer == sourceLayer){
            RS_Entity *clone = entity->clone();
            clone->setLayer(copyLayer);
            ctx += clone;
        }
    }
}

/**
 * Utility method that copies attributes of source layer to its copy
 * @param copyLayer layer to which attributes should be copied
 * @param sourceLayer source layer
 */
void LC_LayerTreeWidget::copyLayerAttributes(RS_Layer *copyLayer, const RS_Layer *sourceLayer){
    copyLayer->setConstruction(sourceLayer->isConstruction());
    copyLayer->setConverted(sourceLayer->isConverted());
    copyLayer->setPrint(sourceLayer->isPrint());

    const RS_Pen sourcePen = sourceLayer->getPen();
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

void LC_LayerTreeWidget::redrawView() const {
    if (m_graphicView != nullptr){
        m_graphicView->redraw();
    }
}

// Fixme - actually this operations are good candidates for moving them into separate actions
// that may be invoked  not only from layers widget... at least, moving to active layer may be...
/**
 * Moves or copies currently selected entities to the layer that is stored by provided tree item.
 * If items are copied, selection will be on duplicated entities, or remains on the same entities if they are moved.
 * Optional resolving of pens allows saving selected items appearance (for "By Layer" values) so such entities will
 * look on target layer in the same way as in the source one.
 * @param layerItem - item that denotes layer
 * @param duplicate - if true, entities will be copied, otherwise - moved between layers
 * @param resolvePens - if true, pen of original entity is resolved first and assigned to the entity that will be on target layer
 */
void LC_LayerTreeWidget::doMoveSelectionToLayer(const LC_LayerTreeItem* layerItem, const bool duplicate, bool resolvePens) const {
    QList<RS_Entity*> selectedEntities;
    if (m_document->collectSelected(selectedEntities)) {
        RS_Layer *targetLayer = layerItem->getLayer();
        bool removeOriginals = !duplicate;
        m_document->undoableModify(m_graphicView->getViewPort(),
                                   [ removeOriginals, targetLayer, resolvePens, selectedEntities](LC_DocumentModificationBatch& ctx)-> bool {
                                       for (const auto en : selectedEntities) {
                                           // iterate over all entities
                                           if (!en->isParentSelected()) {
                                               const RS_Layer* l = en->getLayer(true);
                                               if (l != nullptr && l != targetLayer) {
                                                   RS_Entity* clone = en->clone();
                                                   if (resolvePens) {
                                                       // resolve pen in original entities, so "by layer" and "by block" values will be replaced by resolved values
                                                       RS_Pen resolvedPen = en->getPen(true);
                                                       // assigning resolved pen back to the entity's copy
                                                       clone->setPen(resolvedPen);
                                                   }

                                                   clone->setLayer(targetLayer);
                                                   ctx += clone;
                                               }
                                           }
                                       }

                                       if (removeOriginals) {
                                           ctx -= selectedEntities;
                                       }
                                       return true;
                                   }, [removeOriginals, selectedEntities](const LC_DocumentModificationBatch& ctx, RS_Document* doc)-> void {
                                       if (!removeOriginals) {
                                           doc->select(selectedEntities, false);
                                       }
                                       doc->select(ctx.entitiesToAdd);
                                   });
    }
    redrawView();
}

/**
 * Removes layers starting from provide source
 * @param source starting point for removal
 * @param removeChildrenOnly - if true, only children are removed, if false - source will be removed too
 */
void LC_LayerTreeWidget::doRemoveLayersFromSource(LC_LayerTreeItem *source, const bool removeChildrenOnly){
    const LC_LayerTreeItemAcceptor acceptAllAcceptor;
    QList<LC_LayerTreeItem *> itemsToRemove;

    // prepare list of layers to be removed
    source->collectDescendantChildren(itemsToRemove, acceptAllAcceptor, !removeChildrenOnly);

    doRemoveLayerItems(itemsToRemove);
}

/**
 * Removes list of layers denoted by the list of items. Virtual items are skipped, and confirmation dialog is shown
 * @param itemsToRemove  items to remove layers
 */
void LC_LayerTreeWidget::doRemoveLayerItems(QList<LC_LayerTreeItem *> &itemsToRemove){
    QStringList layerNames;
    QList<RS_Layer *> layersToRemove;
    for (const auto item: itemsToRemove) {
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
        const int dialogResult = invokeLayersRemovalDialog(layerNames);
        if (dialogResult == QMessageBox::Ok){
            doRemoveLayers(layersToRemove);
        }
    }
}

/**
 * Performs actual removal of provided list of layers
 * @param layers list of layers to remove
 */
void LC_LayerTreeWidget::doRemoveLayers(QList<RS_Layer *> &layers) const {
    for (const auto layer: layers) {
        RS_Graphic *graphic = m_document->getGraphic();
        if (graphic != nullptr){
            graphic->removeLayer(layer);
        }
    }
    m_document->updateInserts();
    m_document->calculateBorders();
    m_layerList->setModified(true);
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
int LC_LayerTreeWidget::invokeLayersRemovalDialog(const QStringList &layerNames){
    // layers added, show confirmation dialog

    const QString title(QMessageBox::tr("Remove %n layer(s)", "", layerNames.size()));
    const QStringList text_lines = {QMessageBox::tr("Listed layers and all entities on them will be removed."), "",
                              QMessageBox::tr("Warning: this action can NOT be undone!"),};
    QStringList detail_lines = {QMessageBox::tr("Layers for removal:"), "",};
    detail_lines << layerNames;

    QMessageBox msgBox(QMessageBox::Warning, title, text_lines.join("\n"), QMessageBox::Ok | QMessageBox::Cancel, this);
    msgBox.setDetailedText(detail_lines.join("\n"));

    const int result = msgBox.exec();
    return result;
}
/**
 * Executes layer generic layer edit dialog. If dialog accepted, applies editing results
 * @param pItem layer item for editing
 * @param edit true - editing mode, false - virtual layer rename mode
 */
void LC_LayerTreeWidget::invokeLayerEditOrRenameDialog(LC_LayerTreeItem *pItem, bool edit){
    auto dlg = LC_LayerDialogEx(this, QMessageBox::tr("Layer DialogEx"), m_layerTreeModel, pItem, m_layerList);

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
    const auto originalName = QString(pItem->getName());
    const QString path = m_layerTreeModel->generateLayersDisplayPathString(pItem);

    dlg.setLayerName(originalName);
    dlg.setParentPath(path);
    dlg.setLayer(layer);

    const int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
      if (edit){ // do editing for layer
          // first apply generic attributes
          if (layer != nullptr) {
              const RS_Pen pen = dlg.getPen();
              layer->setPen(pen);
              layer->setConstruction(dlg.isConstruction());
          }

          // handle rename
          const QString layerName = dlg.getLayerName();
          const int editedLayerType = dlg.getEditedLayerType();
          if (originalName != layerName || editedLayerType != pItem->getLayerType()){ // layer is also renamed
              m_layerTreeModel->renamePrimaryLayer(pItem, layerName, editedLayerType);
          }
          m_layerList->fireLayerEdited(nullptr);
      }
      else{ // just rename virtual layer
          QString newName = dlg.getLayerName();
          const bool renamed = m_layerTreeModel -> renameVirtualLayer(pItem, newName);
          if (renamed){
              m_layerList->fireLayerEdited(nullptr);
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
    auto dlg = LC_LayerDialogEx(this, QMessageBox::tr("Layer DialogEx"), m_layerTreeModel, parentItem, m_layerList);

    // setup dialog first
    int dialogMode = LC_LayerDialogEx::MODE_ADD_CHILD_LAYER;
    switch (layerType){
        case RS_Layer::NOT_DEFINED_LAYER_TYPE:{
            if (parentItem == nullptr){
                dialogMode = LC_LayerDialogEx::MODE_ADD_LAYER;
            }
            else{
                dialogMode = LC_LayerDialogEx::MODE_ADD_CHILD_LAYER;
            }
            break;
        }
        case RS_Layer::LayerType::DIMENSIONAL:
        case RS_Layer::LayerType::INFORMATIONAL:
        case RS_Layer::LayerType::ALTERNATE_POSITION:{
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
        path = m_layerTreeModel->generateLayersDisplayPathString(parentItem);
    }

    dlg.setLayerName(name);
    dlg.setParentPath(path);
    dlg.setLayerType(layerType);

    // temporary layer for editing
    auto* tmpLayer = new RS_Layer("");

    // if layer type is provided - we'll use default pen for it
    RS_Pen defaultPen = m_layerTreeModel->getOptions() ->getDefaultPen(layerType == RS_Layer::NOT_DEFINED_LAYER_TYPE ? RS_Layer::LayerType::NORMAL : layerType);
    auto penCopy = RS_Pen(defaultPen);

    tmpLayer->setPen(penCopy);
    dlg.setLayer(tmpLayer);

    int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
      QString layerName = dlg.getLayerName();
      int editedLayerType = dlg.getEditedLayerType();

      // check for duplicate name is performed by the dialog on validate, so here we'll simply create a layer
      QString newLayerName = m_layerTreeModel->createFullLayerName(parentItem, layerName, editedLayerType, true);

      RS_Pen pen = dlg.getPen();
      tmpLayer->setConstruction(dlg.isConstruction());
      tmpLayer->setName(newLayerName);
      tmpLayer->setPen(pen);
      m_layerList->add(tmpLayer); // listeners will be invoked there
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
    LC_LayerTreeModelOptions* options = m_layerTreeModel->getOptions();
    auto dlg = LC_LayerTreeOptionsDialog(this, options);
    const int dialogResult = dlg.exec();
    if (dialogResult == QDialog::Accepted){
        options->save();
        update();
    }
}

void LC_LayerTreeWidget::setGraphicView(RS_GraphicView *gview){
    m_graphicView = gview;

    if (gview == nullptr) {
        setLayerList(nullptr);
        m_document = nullptr;
        m_graphic = nullptr;
    }
    else {
        const auto doc        = gview->getDocument();
        m_graphic = gview->getGraphic(true);
        setLayerList(m_graphic->getLayerList());
        m_document      = doc;
    }
}

void LC_LayerTreeWidget::exportSelectedLayer()  {
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()) {
        const LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        if (!currentItem->isVirtual()) {
            LC_LayersExportOptions exportOptions;
            const auto layerToExport = currentItem->getLayer();
            exportOptions.layers.push_back(layerToExport);
            const auto sourceGraphic = m_document->getGraphic();
            LC_ExportLayersService::exportLayers(exportOptions, sourceGraphic);
        }
    }
}

void LC_LayerTreeWidget::exportLayersList(QList<RS_Layer*> layersToExport) const {
    if (!layersToExport.isEmpty()) {
        LC_LayersExportOptions exportOptions;
        for (const auto layer: layersToExport) {
            exportOptions.layers.push_back(layer);
        }
        const auto sourceGraphic = m_document->getGraphic();
        LC_ExportLayersService::exportLayers(exportOptions, sourceGraphic);
    }
}

void LC_LayerTreeWidget::exportLayerSubTree()  {
    const QModelIndex selectedItemIndex = getSelectedItemIndex();
    if (selectedItemIndex.isValid()) {
        LC_LayerTreeItem *currentItem = m_layerTreeModel->getItemForIndex(selectedItemIndex);
        const auto acceptAll = LC_LayerTreeItemAcceptor();
        QList<RS_Layer*> layersToExport;
        currentItem->collectLayers(layersToExport, acceptAll, true);
        exportLayersList(layersToExport);
    }
}

void LC_LayerTreeWidget::exportVisibleLayers()  {
    const auto layerItem = m_layerTreeModel->getRoot();
    const QG_LayerTreeItemAcceptorVisible acceptVisible;
    QList<RS_Layer*> layersToExport;
    layerItem->collectLayers(layersToExport, acceptVisible, false);
    exportLayersList(layersToExport);
}

QLayout* LC_LayerTreeWidget::getTopLevelLayout() const {
    return layout();
}
