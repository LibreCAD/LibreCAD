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

#include "qg_layerwidget.h"

#include <QAbstractTableModel>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QScrollBar>
#include <QToolButton>

#include "lc_actiongroupmanager.h"
#include "lc_flexlayout.h"
#include "lc_mouse_tracking_table_view.h"
#include "lc_tableitem_delegate_base.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

QG_LayerModel::QG_LayerModel(QObject * parent) : QAbstractItemModel(parent) {
    m_iconLayerVisible = QIcon(":/icons/visible.lci");
    m_iconLayerHidden = QIcon(":/icons/not_visible.lci");
    m_iconLayerDefreeze = QIcon(":/icons/unlocked.lci");
    m_iconLayerFreeze = QIcon(":/icons/locked.lci");
    m_iconLayerPrint = QIcon(":/icons/print.lci");
    m_iconLayerNoPrint = QIcon(":/icons/noprint.lci");
    m_iconLayerConstruction = QIcon(":/icons/construction_layer.lci");
    m_iconLayerNoConstruction = QIcon(":/icons/noconstruction.lci");
}

int QG_LayerModel::rowCount ( const QModelIndex & parent) const {
    if (!parent.isValid()) {
        return m_listLayer.size();
    }
    return 0;
}

QModelIndex QG_LayerModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex QG_LayerModel::index (const int row, const int column, const QModelIndex & /*parent*/ ) const {
    if ( row >= m_listLayer.size() || row < 0) {
        return QModelIndex();
    }
    return createIndex ( row, column);
}

void QG_LayerModel::setLayerList(const RS_LayerList* ll) {
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

RS_Layer *QG_LayerModel::getLayer(const int row) const {
    if ( row >= m_listLayer.size() || row < 0) {
        return nullptr;
    }
    return m_listLayer.at(row);
}

QModelIndex QG_LayerModel::getIndex (RS_Layer * lay) const {
    const int row = m_listLayer.indexOf(lay);
    if (row<0) {
        return {};
    }
    return createIndex (row, COLUMN_NAME);
}
QVariant QG_LayerModel::data( const QModelIndex & index, const int role ) const{
    if (!index.isValid() || index.row() >= m_listLayer.size()) {
        return QVariant();
    }

    const RS_Layer *layer{m_listLayer.at(index.row())};
    const int col{index.column()};

    switch (role) {
        case Qt::DecorationRole:
            switch (col) {
                case COLUMN_VISIBLE:
                    if (!layer->isFrozen()) {
                        return m_iconLayerVisible;
                    }
                    return m_iconLayerHidden;

                case COLUMN_LOCKED:
                    if (!layer->isLocked()) {
                        return m_iconLayerDefreeze;
                    }
                    return m_iconLayerFreeze;

                case COLUMN_PRINT:
                    if (!layer->isPrint()) {
                        return m_iconLayerNoPrint;
                    }
                    return m_iconLayerPrint;

                case COLUMN_CONSTRUCTION:
                    if (!layer->isConstruction()) {
                        return m_iconLayerNoConstruction;
                    }
                    return m_iconLayerConstruction;

                default:
                    break;
            }
            break;

        case Qt::DisplayRole:
            if (COLUMN_NAME == col) {
                return layer->getName();
            }
            break;
        case Qt::BackgroundRole:
            if (COLUMN_COLOR_SAMPLE == col) {
                return layer->getPen().getColor().toQColor();
            }
            break;
        case Qt::FontRole:
            if (COLUMN_NAME == col) {
                if ((m_activeLayer != nullptr) && m_activeLayer == layer) {
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

class LC_LayerTableItemDelegate : public LC_TableItemDelegateBase {
public:
    explicit LC_LayerTableItemDelegate(QTableView* parent, QG_LayerModel* model) : LC_TableItemDelegateBase(parent) {
        m_model = model;
        auto palette = parent->palette();
        m_gridColor = palette.color(QPalette::Button);
        m_hoverRowBackgroundColor = palette.color(QPalette::AlternateBase);
    }

    void doPaint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        const int col = index.column();
        if (col  == QG_LayerModel::Columns::COLUMN_COLOR_SAMPLE) {
            const auto layer = m_model->getLayer(index.row());
            if (layer != nullptr){
                QRect colorRect = option.rect;
                const int originalWidth = colorRect.width();
                const int newWidth = colorRect.height() - 8;
                // center color box in cell
                const int widthDelta = originalWidth - newWidth;
                const int leftDelta = widthDelta / 2;
                const int rightDelta = widthDelta - leftDelta;

                colorRect.adjust(leftDelta+1, 6, -rightDelta-1, -6);

                painter->fillRect(colorRect, Qt::black);
                colorRect.adjust(1, 1, -1, -1);
                const auto color = layer->getPen().getColor();
                painter->fillRect(colorRect, color);
            }
        }
        else {
            QStyledItemDelegate::paint(painter, option, index);
        }
        const bool drawGrid = true;
        if (drawGrid) {
           drawHorizontalGridLine(painter, option);
        }
    }
private:
    QG_LayerModel* m_model;
};

void QG_LayerWidget::addToolbarButton(LC_FlexLayout* layButtons, const RS2::ActionType actionType) {
    QAction* action = m_actionGroupManager->getActionByType(actionType);
    if (action != nullptr) {
        const auto button = new QToolButton(this);
        button->setDefaultAction(action);
        layButtons->addWidget(button);
    }
}

/**
 * Constructor.
 */
QG_LayerWidget::QG_LayerWidget(LC_ActionGroupManager* actionGroupManager, const QG_ActionHandler *ah, QWidget *parent, const char *name, const Qt::WindowFlags f)
    : LC_GraphicViewAwareWidget(parent, name, f){
    m_actionHandler = ah;
    m_actionGroupManager = actionGroupManager;
    m_layerList = nullptr;
    m_showByBlock = false;
    m_lastLayer = nullptr;

    m_layerModel = new QG_LayerModel(this);
    m_layerView = new LC_MouseTrackingTableView(this);
    m_layerView->setModel(m_layerModel);

    m_layerView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_layerView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_layerView->setFocusPolicy(Qt::NoFocus);
    m_layerView->setMinimumHeight(60);

    QHeaderView* verticalHeader = m_layerView->verticalHeader();
    const QFontMetrics fm(font());
    const int itemHeight = fm.height() + 6;
    verticalHeader->setDefaultSectionSize(itemHeight);

    QHeaderView *horizontalHeader = m_layerView->horizontalHeader();
    horizontalHeader->setMinimumSectionSize(itemHeight);
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->hide();

    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_VISIBLE, itemHeight);
    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_VISIBLE, itemHeight);
    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_LOCKED, itemHeight);
    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_PRINT, itemHeight);
    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_CONSTRUCTION, itemHeight);
    m_layerView->setColumnWidth(QG_LayerModel::COLUMN_COLOR_SAMPLE, itemHeight);
    m_layerView->verticalHeader()->hide();

    m_layerView->setTrackingItemDelegate(new LC_LayerTableItemDelegate(m_layerView, m_layerModel));

    m_layerView->setShowGrid(false); // fixme - sand - add to options!*/

#ifndef DONT_FORCE_WIDGETS_CSS
    m_layerView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 1, 2, 2);
    lay->setSpacing(1);

    auto *layButtons = new LC_FlexLayout(0,3,3);
    layButtons->setContentsMargins(0, 0, 0, 0);

    addToolbarButton(layButtons, RS2::ActionLayersDefreezeAll);
    addToolbarButton(layButtons, RS2::ActionLayersFreezeAll);
    addToolbarButton(layButtons, RS2::ActionLayersUnlockAll);
    addToolbarButton(layButtons, RS2::ActionLayersLockAll);
    addToolbarButton(layButtons, RS2::ActionLayersAdd);
    addToolbarButton(layButtons, RS2::ActionLayersRemove);
    addToolbarButton(layButtons, RS2::ActionLayersEdit);

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
 * @param layerList
 * @param showByBlock true: show the layer with the name "ByBlock" if
 *                    it exists.
 *                    false: don't show special layer "ByBlock"
 */
void QG_LayerWidget::setLayerList(RS_LayerList* layerList, const bool showByBlock) {
    if (m_layerList != nullptr) {
        m_layerList->removeListener(this);
    }

    m_layerList = layerList;
    m_showByBlock = showByBlock;

    if (layerList != nullptr) {
        m_layerList->addListener(this);
    }
    updateWidget();
}

void QG_LayerWidget::updateFiltering(){
    if (! m_matchLayerName->text().isEmpty()) {
        slotUpdateLayerList();
    }
}

void QG_LayerWidget::layerAdded(RS_Layer* layer){
    updateWidget();   // 1st apply the new layer to the view
    updateFiltering();
    activateLayer(layer);
    updateWidget();   // update again, if new layer is last row, the height was wrong
}

void QG_LayerWidget::layerEdited([[maybe_unused]]RS_Layer *layer){
    updateWidget();
    updateFiltering();
}

void QG_LayerWidget::layerRemoved([[maybe_unused]]RS_Layer *layer){
    updateWidget();
    updateFiltering();
    updateWidget();
    activateLayer(m_layerList->at(0));
}

/**
 * @brief getActiveName
 * @return the name of the active layer
 */
QString QG_LayerWidget::getActiveName() const{
    if (m_layerList != nullptr) {
        const RS_Layer *activeLayer = m_layerList->getActive();
        if (activeLayer != nullptr) {
            return activeLayer->getName();
        }
    }
    return QString();
}

/**
 * Updates the layer box from the layers in the graphic.
 */
void QG_LayerWidget::updateWidget() {
    RS_DEBUG->print("QG_LayerWidget::update() begin");

    if (m_layerView == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerView");
        return;
    }
    const int yPos = m_layerView->verticalScrollBar()->value();
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

void QG_LayerWidget::restoreSelections() const {
    QItemSelectionModel* selectionModel = m_layerView->selectionModel();
    for (auto* layer: *m_layerList) {
        if (layer == nullptr) {
            continue;
        }
        if (!layer->isVisibleInLayerList()) {
            continue;
        }
        if (!layer->isSelectedInLayerList()) {
            continue;
        }

        QModelIndex idx = m_layerModel->getIndex(layer);
        QItemSelection selection(idx, idx);
        selectionModel->select(selection, QItemSelectionModel::Select);
    }
}

/**
 * Activates the given layer and makes it the active
 * layer in the layerlist.
 */
void QG_LayerWidget::activateLayer(RS_Layer* layer, const bool updateScroll) const {
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if (layer == nullptr || m_graphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layer or layerList");
        return;
    }

    m_graphic->activateLayer(layer);

    if (m_layerModel == nullptr) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layerModel");
        return;
    }
    const QModelIndex idx = m_layerModel->getIndex(layer);

    if ((idx.model() == nullptr) || (m_layerView == nullptr)) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: invalid layer or nullptr layerView");
        return;
    }

    // remember selected status of the layer
    const bool selected = layer->isSelectedInLayerList();

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
        const int yPos = m_layerView->verticalScrollBar()->value();
        m_layerView->verticalScrollBar()->setValue(yPos);
    }

    //update active layer name in mainwindow status bar
    QC_ApplicationWindow::getAppWindow()->slotUpdateActiveLayer();

    RS_DEBUG->print("QG_LayerWidget::activateLayer() end");
}

/**
 * Called when the user activates (highlights) a layer.
 */
void QG_LayerWidget::slotActivated(const QModelIndex& layerIdx /*const QString& layerName*/) const {
    if (!layerIdx.isValid() || m_layerList==nullptr) {
        return;
    }

    RS_Layer* lay = m_layerModel->getLayer(layerIdx.row());
    if (lay == nullptr) {
        return;
    }

    if (layerIdx.column() == QG_LayerModel::COLUMN_NAME) {
        m_graphic->activateLayer(lay, true);
        return;
    }

    switch (layerIdx.column()) {
        case QG_LayerModel::COLUMN_VISIBLE:
            m_actionHandler->setCurrentAction(RS2::ActionLayersToggleView, lay);
            break;
        case QG_LayerModel::COLUMN_LOCKED:
            m_actionHandler->setCurrentAction(RS2::ActionLayersToggleLock, lay);
            break;
        case QG_LayerModel::COLUMN_PRINT:
            m_actionHandler->setCurrentAction(RS2::ActionLayersTogglePrint, lay);
            break;
        case QG_LayerModel::COLUMN_CONSTRUCTION:
            m_actionHandler->setCurrentAction(RS2::ActionLayersToggleConstruction, lay);
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
    const QItemSelection &deselected) const {
    QModelIndex index;
    QItemSelectionModel *selectionModel {m_layerView->selectionModel()};

    foreach (index, selected.indexes()) {
        const auto layer = m_layerModel->getLayer(index.row());
        if (layer != nullptr) {
            layer->selectedInLayerList(true);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Select);
        }
    }

    foreach (index, deselected.indexes()) {
        const auto layer = m_layerModel->getLayer(index.row());
        if ((layer != nullptr) && layer->isVisibleInLayerList()) {
            layer->selectedInLayerList(false);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Deselect);
        }
    }
}


/**
 * Called when reg-expresion matchLayerName->text changed
 */
void QG_LayerWidget::slotUpdateLayerList() {
    const QRegularExpression rx = QRegularExpression::fromWildcard(m_matchLayerName->text());

    for (unsigned i = 0; i < m_layerList->count(); i++) {
        QString s = m_layerModel->getLayer(i)->getName();
        if (m_matchLayerName->text().isEmpty() || s.indexOf(rx) == 0) {
            m_layerView->showRow(i);
            m_layerModel->getLayer(i)->visibleInLayerList(true);
        }
        else {
            m_layerView->hideRow(i);
            m_layerModel->getLayer(i)->visibleInLayerList(false);
        }
    }

    restoreSelections();
}

void QG_LayerWidget::addMenuItem(QMenu* contextMenu, const RS2::ActionType actionType) const {
    const auto action = m_actionGroupManager->getActionByType(actionType);
    if (action != nullptr) {
        contextMenu->QWidget::addAction(action);
    }
}

QLayout* QG_LayerWidget::getTopLevelLayout() const {
    return layout();
}

/**
 * Shows a context menu for the layer widget. Launched with a right click.
 */
void QG_LayerWidget::contextMenuEvent(QContextMenuEvent *e) {
    const auto menu = new QMenu(this);
    addMenuItem(menu, RS2::ActionLayersDefreezeAll);
    addMenuItem(menu, RS2::ActionLayersFreezeAll);
    addMenuItem(menu, RS2::ActionLayersUnlockAll);
    addMenuItem(menu, RS2::ActionLayersLockAll);
    menu->addSeparator();
    // Actions for selected layers or, if nothing is selected, for active layer:
    addMenuItem(menu, RS2::ActionLayersToggleView);
    addMenuItem(menu, RS2::ActionLayersToggleLock);
    addMenuItem(menu, RS2::ActionLayersTogglePrint);
    addMenuItem(menu, RS2::ActionLayersToggleConstruction);
    addMenuItem(menu, RS2::ActionLayersRemove);
    menu->addSeparator();
    // Single layer actions:
    addMenuItem(menu, RS2::ActionLayersAdd);
    addMenuItem(menu, RS2::ActionLayersEdit);
    menu->addSeparator();
    addMenuItem(menu, RS2::ActionLayersExportSelected);
    addMenuItem(menu, RS2::ActionLayersExportVisible);
    menu->exec(QCursor::pos());
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

void QG_LayerWidget::activateLayer(const int row) const {
    const auto layer = m_layerModel->getLayer(row);
    if (layer != nullptr) {
        m_graphic->activateLayer(layer, true);
    }
    else {
        qWarning("activateLayer: row %d doesn't exist", row);
    }
}

void QG_LayerWidget::setGraphicView(RS_GraphicView *gview){
    if (gview == nullptr) {
        setLayerList(nullptr, false);
        m_graphic = nullptr;
    }
    else {
        const auto doc = gview->getDocument();
        const bool showByBlock = doc->rtti() == RS2::EntityBlock;
        m_graphic  = gview->getGraphic(true);
        const auto layerList = m_graphic->getLayerList();
        setLayerList(layerList, showByBlock);
    }
}
