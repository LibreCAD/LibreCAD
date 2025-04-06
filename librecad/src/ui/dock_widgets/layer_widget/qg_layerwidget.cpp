/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
**
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

#include <QBitmap>
#include <QBoxLayout>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>

#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "qg_layerwidget.h"

#include "lc_flexlayout.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "lc_widgets_common.h"

QG_LayerModel::QG_LayerModel(QObject * parent) : QAbstractTableModel(parent) {
    m_iconLayerVisible = QIcon(":/icons/visible.lci");
    //layerHidden = QIcon(":/icons/invisible.lci");
    m_iconLayerHidden = QIcon(":/icons/not_visible.lci");
    m_iconLayerDefreeze = QIcon(":/icons/unlocked.lci");
    m_iconLayerFreeze = QIcon(":/icons/locked.lci");
    m_iconLayerPrint = QIcon(":/icons/print.lci");
    m_iconLayerNoPrint = QIcon(":/icons/noprint.lci");
    m_iconLayerConstruction = QIcon(":/icons/construction_layer.lci");
    m_iconLayerNoConstruction = QIcon(":/icons/noconstruction.lci");
}

int QG_LayerModel::rowCount ( const QModelIndex & /*parent*/ ) const {
    return m_listLayer.size();
}

QModelIndex QG_LayerModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex QG_LayerModel::index ( int row, int column, const QModelIndex & /*parent*/ ) const {
    if ( row >= m_listLayer.size() || row < 0)
        return QModelIndex();
    return createIndex ( row, column);
}

void QG_LayerModel::setLayerList(RS_LayerList* ll) {
    /* since 4.6 the recommended way is to use begin/endResetModel()
     * TNick <nicu.tofan@gmail.com>
     */
    beginResetModel();
    m_listLayer.clear();
    if (ll == nullptr) {
        endResetModel();
        return;
    }
    for (unsigned i=0; i < ll->count(); ++i) {
        m_listLayer.append(ll->at(i));
    }
    setActiveLayer(ll->getActive());
    std::sort( m_listLayer.begin(), m_listLayer.end(), [](const RS_Layer *s1, const RS_Layer *s2)-> bool{
        return s1->getName() < s2->getName();
    } );

    //called to force redraw
    endResetModel();
}

RS_Layer *QG_LayerModel::getLayer(int row) const {
    if ( row >= m_listLayer.size() || row < 0) {
        return nullptr;
    }
    return m_listLayer.at(row);
}

QModelIndex QG_LayerModel::getIndex (RS_Layer * lay) const {
    int row = m_listLayer.indexOf(lay);
    if (row<0) {
        return {};
    }
    return createIndex (row, NAME);
}
QVariant QG_LayerModel::data ( const QModelIndex & index, int role ) const{
    if (!index.isValid() || index.row() >= m_listLayer.size())
        return QVariant();

    RS_Layer *layer{m_listLayer.at(index.row())};
    int col{index.column()};

    switch (role) {
        case Qt::DecorationRole:
            switch (col) {
                case VISIBLE:
                    if (!layer->isFrozen()) {
                        return m_iconLayerVisible;
                    }
                    return m_iconLayerHidden;

                case LOCKED:
                    if (!layer->isLocked()) {
                        return m_iconLayerDefreeze;
                    }
                    return m_iconLayerFreeze;

                case PRINT:
                    if (!layer->isPrint()) {
                        return m_iconLayerNoPrint;
                    }
                    return m_iconLayerPrint;

                case CONSTRUCTION:
                    if (!layer->isConstruction()) {
                        return m_iconLayerNoConstruction;
                    }
                    return m_iconLayerConstruction;

                default:
                    break;
            }
            break;

        case Qt::DisplayRole:
            if (NAME == col) {
                return layer->getName();
            }
            break;

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        case Qt::BackgroundRole:
#else
    case Qt::BackgroundColorRole:
#endif
            if (COLOR_SAMPLE == col) {
                return layer->getPen().getColor().toQColor();
            }
            break;

        case Qt::FontRole:
            if (NAME == col) {
                if (m_activeLayer && m_activeLayer == layer) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
            break;
        default:
            break;
    }

    return QVariant();
}

/**
 * Constructor.
 */
QG_LayerWidget::QG_LayerWidget(QG_ActionHandler *ah, QWidget *parent, const char *name, Qt::WindowFlags f)
    : QWidget(parent, f){
    setObjectName(name);
    m_actionHandler = ah;
    m_layerList = nullptr;
    m_showByBlock = false;
    m_lastLayer = nullptr;

    m_layerModel = new QG_LayerModel(this);
    m_layerView = new QTableView(this);
    m_layerView->setModel(m_layerModel);
    m_layerView->setShowGrid(true);
    m_layerView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_layerView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_layerView->setFocusPolicy(Qt::NoFocus);
    // layerView->setMinimumHeight(140);
    m_layerView->setMinimumHeight(60);
    QHeaderView *pHeader {m_layerView->horizontalHeader()};
    pHeader->setMinimumSectionSize( QG_LayerModel::ICONWIDTH + 4);
    pHeader->setStretchLastSection(true);
    pHeader->hide();
    m_layerView->setColumnWidth(QG_LayerModel::VISIBLE, QG_LayerModel::ICONWIDTH);
    m_layerView->setColumnWidth(QG_LayerModel::VISIBLE, QG_LayerModel::ICONWIDTH);
    m_layerView->setColumnWidth(QG_LayerModel::LOCKED, QG_LayerModel::ICONWIDTH);
    m_layerView->setColumnWidth(QG_LayerModel::PRINT, QG_LayerModel::ICONWIDTH);
    m_layerView->setColumnWidth(QG_LayerModel::CONSTRUCTION, QG_LayerModel::ICONWIDTH);
    m_layerView->setColumnWidth(QG_LayerModel::COLOR_SAMPLE, QG_LayerModel::ICONWIDTH);
    m_layerView->verticalHeader()->hide();

#ifndef DONT_FORCE_WIDGETS_CSS
    layerView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);

    auto *layButtons = new LC_FlexLayout(0,5,5);

    // auto* layButtons = new QHBoxLayout;
    QToolButton* but;
    // show all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/visible.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show all layers"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersDefreezeAll);
    layButtons->addWidget(but);
    // hide all layer:
    but = new QToolButton(this);
//    but->setIcon(QIcon(":/icons/invisible.lci"));
    but->setIcon(QIcon(":/icons/not_visible.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Hide all layers"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersFreezeAll);
    layButtons->addWidget(but);
    // unlock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/unlocked.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Unlock all layers"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersUnlockAll);
    layButtons->addWidget(but);
    // lock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/locked.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Lock all layers"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersLockAll);
    layButtons->addWidget(but);
    // add layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/add.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Add a layer"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersAdd);
    layButtons->addWidget(but);
    // remove layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/remove.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Remove layer"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersRemove);
    layButtons->addWidget(but);
    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/rename_active_block.lci"));
    // but->setMinimumSize(minButSize);
    but->setToolTip(tr("Modify layer attributes / rename"));
    connect(but, &QToolButton::clicked, m_actionHandler, &QG_ActionHandler::slotLayersEdit);
    layButtons->addWidget(but);

    // lineEdit to filter layer list with RegEx
    m_matchLayerName = new QLineEdit(this);
    m_matchLayerName->setReadOnly(false);
    m_matchLayerName->setPlaceholderText(tr("Filter"));
    m_matchLayerName->setClearButtonEnabled(true);
    m_matchLayerName->setToolTip(tr("Looking for matching layer names"));
    connect(m_matchLayerName, &QLineEdit::textChanged, this, &QG_LayerWidget::slotUpdateLayerList);

    lay->addWidget(m_matchLayerName);
    lay->addLayout(layButtons);
    lay->addWidget(m_layerView);
    this->setLayout(lay);

    connect(m_layerView, &QTableView::clicked, this, &QG_LayerWidget::slotActivated);
    connect(m_layerView->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &QG_LayerWidget::slotSelectionChanged);

    updateWidgetSettings();
}

/**
 * Sets the layerlist this layer widget should show.
 *
 * @param showByBlock true: show the layer with the name "ByBlock" if
 *                    it exists.
 *                    false: don't show special layer "ByBlock"
 */
void QG_LayerWidget::setLayerList(RS_LayerList* layerList, bool showByBlock) {
    m_layerList = layerList;
    m_showByBlock = showByBlock;
    update();
}

void QG_LayerWidget::updateFiltering(){
    if (! m_matchLayerName->text().isEmpty()) {
        slotUpdateLayerList();
    }
}

void QG_LayerWidget::layerAdded(RS_Layer* layer){
    update();   // 1st apply the new layer to the view
    updateFiltering();
    activateLayer(layer);
    update();   // update again, if new layer is last row, the height was wrong
}

void QG_LayerWidget::layerEdited(RS_Layer *rs_layer){
    update();
    updateFiltering();
}

void QG_LayerWidget::layerRemoved(RS_Layer *rs_layer){
    update();
    updateFiltering();
    update();
    activateLayer(m_layerList->at(0));
}

/**
 * @brief getActiveName
 * @return the name of the active layer
 */
QString QG_LayerWidget::getActiveName() const{
    if (m_layerList != nullptr) {
        RS_Layer *activeLayer = m_layerList->getActive();
        if (activeLayer != nullptr) {
            return activeLayer->getName();
        }
    }
    return QString();
}

/**
 * Updates the layer box from the layers in the graphic.
 */
void QG_LayerWidget::update() {
    RS_DEBUG->print("QG_LayerWidget::update() begin");

    if (!m_layerView) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerView");
        return;
    }
    int yPos = m_layerView->verticalScrollBar()->value();
    m_layerView->resizeRowsToContents();
    m_layerView->verticalScrollBar()->setValue(yPos);

    m_layerModel->setLayerList(m_layerList); // allow a null layerList; this clears the widget

    if (m_layerList == nullptr) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QG_LayerWidget::update: nullptr layerList");
        return;
    }

    RS_DEBUG->print("QG_LayerWidget::update: reactivating current layer");

    RS_Layer* activeLayer = m_layerList->getActive();
    if (activeLayer == nullptr) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QG_LayerWidget::update: nullptr activeLayer");
        m_layerModel->setActiveLayer(nullptr);
        return;
    }

    if (m_lastLayer == nullptr) {
        RS_DEBUG->print(RS_Debug::D_NOTICE, "QG_LayerWidget::update: nullptr lastLayer");
        m_lastLayer = activeLayer;
    }

    restoreSelections();

    RS_DEBUG->print("QG_LayerWidget::update(): OK");
}

void QG_LayerWidget::restoreSelections() {
    QItemSelectionModel* selectionModel = m_layerView->selectionModel();
    for (auto* layer: *m_layerList) {
        if (!layer) continue;
        if (!layer->isVisibleInLayerList()) continue;
        if (!layer->isSelectedInLayerList()) continue;

        QModelIndex idx = m_layerModel->getIndex(layer);
        QItemSelection selection(idx, idx);
        selectionModel->select(selection, QItemSelectionModel::Select);
    }
}

/**
 * Activates the given layer and makes it the active
 * layer in the layerlist.
 */
void QG_LayerWidget::activateLayer(RS_Layer* layer, bool updateScroll) {
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if (!layer || !m_layerList) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layer or layerList");
        return;
    }

    m_layerList->activate(layer);

    if (!m_layerModel) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layerModel");
        return;
    }
    QModelIndex idx = m_layerModel->getIndex(layer);

    if (!idx.model() || !m_layerView) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: invalid layer or nullptr layerView");
        return;
    }

    // remember selected status of the layer
    bool selected = layer->isSelectedInLayerList();

    m_layerView->setCurrentIndex(idx);
    m_layerModel->setActiveLayer(layer);
    m_layerView->viewport()->update();

    // restore selected status of the layer
    QItemSelectionModel::SelectionFlag selFlag;
    if (selected) {
        selFlag = QItemSelectionModel::Select;
    } else {
        selFlag = QItemSelectionModel::Deselect;
    }
    layer->selectedInLayerList(selected);
    m_layerView->selectionModel()->select(QItemSelection(idx, idx), selFlag);

    if (!updateScroll) {
        int yPos = m_layerView->verticalScrollBar()->value();
        m_layerView->verticalScrollBar()->setValue(yPos);
    }

    //update active layer name in mainwindow status bar
    QC_ApplicationWindow::getAppWindow()->slotUpdateActiveLayer();

    RS_DEBUG->print("QG_LayerWidget::activateLayer() end");
}

/**
 * Called when the user activates (highlights) a layer.
 */
void QG_LayerWidget::slotActivated(QModelIndex layerIdx /*const QString& layerName*/) {
    if (!layerIdx.isValid() || m_layerList==nullptr) {
        return;
    }

    RS_Layer* lay = m_layerModel->getLayer(layerIdx.row());
    if (lay == 0)
        return;

    if (layerIdx.column() == QG_LayerModel::NAME) {
        m_layerList->activate(lay, true);
        return;
    }

    switch (layerIdx.column()) {
        case QG_LayerModel::VISIBLE:
            m_actionHandler->toggleVisibility(lay);
            break;
        case QG_LayerModel::LOCKED:
            m_actionHandler->toggleLock(lay);
            break;
        case QG_LayerModel::PRINT:
            m_actionHandler->togglePrint(lay);
            break;
        case QG_LayerModel::CONSTRUCTION:
            m_actionHandler->toggleConstruction(lay);
            break;
        default:
            break;
    }
}


/**
 * Called on layers selection/deselection
 */
void QG_LayerWidget::slotSelectionChanged(
    const QItemSelection &selected,
    const QItemSelection &deselected){
    QModelIndex index;
    QItemSelectionModel *selectionModel {m_layerView->selectionModel()};

    foreach (index, selected.indexes()) {
        auto layer = m_layerModel->getLayer(index.row());
        if (layer) {
            layer->selectedInLayerList(true);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Select);
        }
    }

    foreach (index, deselected.indexes()) {
        auto layer = m_layerModel->getLayer(index.row());
        if (layer && layer->isVisibleInLayerList()) {
            layer->selectedInLayerList(false);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Deselect);
        }
    }
}


/**
 * Called when reg-expresion matchLayerName->text changed
 */
void QG_LayerWidget::slotUpdateLayerList() {
    QRegularExpression rx = QRegularExpression::fromWildcard(m_matchLayerName->text());

    for (unsigned i=0; i<m_layerList->count() ; i++) {
        QString s=m_layerModel->getLayer(i)->getName();
        if (m_matchLayerName->text().isEmpty() || s.indexOf(rx) == 0) {
            m_layerView->showRow(i);
            m_layerModel->getLayer(i)->visibleInLayerList(true);
        } else {
            m_layerView->hideRow(i);
            m_layerModel->getLayer(i)->visibleInLayerList(false);
        }
    }

    restoreSelections();
}

/**
 * Shows a context menu for the layer widget. Launched with a right click.
 */
void QG_LayerWidget::contextMenuEvent(QContextMenuEvent *e) {
    if (m_actionHandler) {
        auto contextMenu = std::make_unique<QMenu>(this);

        using ActionMemberFunc = void (QG_ActionHandler::*)();
        const auto addActionFunc = [this, &contextMenu](const QString& name, ActionMemberFunc func) {
            contextMenu->addAction(name, m_actionHandler, func);
        };

        // Actions for all layers:
        addActionFunc( tr("&Defreeze all Layers"), &QG_ActionHandler::slotLayersDefreezeAll);
        addActionFunc( tr("&Freeze all Layers"), &QG_ActionHandler::slotLayersFreezeAll);
        addActionFunc( tr("&Unlock all Layers"), &QG_ActionHandler::slotLayersUnlockAll);
        addActionFunc( tr("&Lock all Layers"), &QG_ActionHandler::slotLayersLockAll);
        contextMenu->addSeparator();
        // Actions for selected layers or,
        // if nothing is selected, for active layer:
        addActionFunc( tr("Toggle Layer &Visibility"), &QG_ActionHandler::slotLayersToggleView);
        addActionFunc( tr("Toggle Layer Loc&k"), &QG_ActionHandler::slotLayersToggleLock);
        addActionFunc( tr("Toggle Layer &Printing"), &QG_ActionHandler::slotLayersTogglePrint);
        addActionFunc( tr("Toggle &Construction Layer"), &QG_ActionHandler::slotLayersToggleConstruction);
        addActionFunc( tr("&Remove Layer"), &QG_ActionHandler::slotLayersRemove);
        contextMenu->addSeparator();
        // Single layer actions:
        addActionFunc( tr("&Add Layer"), &QG_ActionHandler::slotLayersAdd);
        addActionFunc( tr("Edit Layer &Attributes"), &QG_ActionHandler::slotLayersEdit);

        contextMenu->addSeparator();

        addActionFunc( tr("&Export Selected Layer(s)"), &QG_ActionHandler::slotLayersExportSelected);
        addActionFunc( tr("Export &Visible Layer(s)"), &QG_ActionHandler::slotLayersExportVisible);

        contextMenu->exec(QCursor::pos());
    }

    e->accept();
}

/**
 * Escape releases focus.
 */
void QG_LayerWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {

    case Qt::Key_Escape: {
        emit escape();
        break;
    }
    default:
        QWidget::keyPressEvent(e);
        break;
    }
}

void QG_LayerWidget::activateLayer(int row){
    auto layer = m_layerModel->getLayer(row);
    if (layer) {
        m_layerList->activate(layer, true);
    }
    else
        qWarning("activateLayer: row %d doesn't exist", row);
}

void QG_LayerWidget::updateWidgetSettings(){
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

void QG_LayerWidget::setDocumentAndView(RS_Document *doc, [[maybe_unused]]RS_GraphicView *gview){
    if (doc == nullptr) {
        setLayerList(nullptr, false);
    }
    else {
        bool showByBlock = doc->rtti() == RS2::EntityBlock;
        setLayerList(doc->getLayerList(), showByBlock);
    }
}
