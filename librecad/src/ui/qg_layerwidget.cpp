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
#include "qg_actionhandler.h"
#include "qc_applicationwindow.h"

#include <QBitmap>
#include <QScrollBar>
#include <QTableView>
#include <QHeaderView>
#include <QToolButton>
#include <QMenu>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include "rs_debug.h"

QG_LayerModel::QG_LayerModel(QObject * parent) : QAbstractTableModel(parent) {
    layerVisible = QIcon(":/icons/visible.svg");
    layerHidden = QIcon(":/icons/invisible.svg");
    layerDefreeze = QIcon(":/icons/unlocked.svg");
    layerFreeze = QIcon(":/icons/locked.svg");
    layerPrint = QIcon(":/icons/print.svg");
    layerNoPrint = QIcon(":/icons/noprint.svg");
    layerConstruction = QIcon(":/icons/construction_layer.svg");
    layerNoConstruction = QIcon(":/icons/noconstruction.svg");
}



int QG_LayerModel::rowCount ( const QModelIndex & /*parent*/ ) const {
    return listLayer.size();
}



QModelIndex QG_LayerModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}



QModelIndex QG_LayerModel::index ( int row, int column, const QModelIndex & /*parent*/ ) const {
    if ( row >= listLayer.size() || row < 0)
        return QModelIndex();
    return createIndex ( row, column);
}



void QG_LayerModel::setLayerList(RS_LayerList* ll) {
	/* since 4.6 the recommended way is to use begin/endResetModel()
	 * TNick <nicu.tofan@gmail.com>
	 */
    beginResetModel();
    listLayer.clear();
    if (ll == NULL) {
        endResetModel();
        return;
    }
    for (unsigned i=0; i < ll->count(); ++i) {
        listLayer.append(ll->at(i));
    }
    std::sort( listLayer.begin(), listLayer.end(), [](const RS_Layer *s1, const RS_Layer *s2)-> bool{
        return s1->getName() < s2->getName();
    } );
//called to force redraw
    endResetModel();
}



RS_Layer *QG_LayerModel::getLayer( int row ) {
    if ( row >= listLayer.size() || row < 0)
        return NULL;
    return listLayer.at(row);
}



QModelIndex QG_LayerModel::getIndex (RS_Layer * lay) {
    int row = listLayer.indexOf(lay);
    if (row<0)
        return QModelIndex();
    return createIndex ( row, NAME);
}



QVariant QG_LayerModel::data ( const QModelIndex & index, int role ) const {
    if (!index.isValid() || index.row() >= listLayer.size())
        return QVariant();

    RS_Layer* layer {listLayer.at(index.row())};
    int col {index.column()};

    switch (role) {
    case Qt::DecorationRole:
        switch (col){
        case VISIBLE:
            if (!layer->isFrozen()) {
                return layerVisible;
            }
            return layerHidden;

        case LOCKED:
            if (!layer->isLocked()) {
                return layerDefreeze;
            }
            return layerFreeze;

        case PRINT:
            if( !layer->isPrint()) {
                return layerNoPrint;
            }
            return layerPrint;

        case CONSTRUCTION:
            if( !layer->isConstruction()) {
                return layerNoConstruction;
            }
            return layerConstruction;

        default:
            break;
        }
        break;

    case Qt::DisplayRole:
        if (NAME == col) {
            return layer->getName();
        }
        break;

    case Qt::BackgroundColorRole:
        if( COLOR_SAMPLE == col) {
            return layer->getPen().getColor().toQColor();
        }
        break;

    }

    return QVariant();
}



/**
 * Constructor.
 */
QG_LayerWidget::QG_LayerWidget(QG_ActionHandler* ah, QWidget* parent,
                               const char* name, Qt::WindowFlags f)
        : QWidget(parent, f) {

    setObjectName(name);
    actionHandler = ah;
	layerList = nullptr;
    showByBlock = false;
	lastLayer = nullptr;

    layerModel = new QG_LayerModel(this);
    layerView = new QTableView(this);
    layerView->setModel(layerModel);
    layerView->setShowGrid(true);
    layerView->setSelectionMode(QAbstractItemView::SingleSelection);
    layerView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layerView->setFocusPolicy(Qt::NoFocus);
    layerView->setMinimumHeight(140);
    QHeaderView *pHeader {layerView->horizontalHeader()};
    pHeader->setMinimumSectionSize( QG_LayerModel::ICONWIDTH + 4);
    pHeader->setStretchLastSection(true);
    pHeader->hide();
    layerView->setColumnWidth(QG_LayerModel::VISIBLE, QG_LayerModel::ICONWIDTH);
    layerView->setColumnWidth(QG_LayerModel::VISIBLE, QG_LayerModel::ICONWIDTH);
    layerView->setColumnWidth(QG_LayerModel::LOCKED, QG_LayerModel::ICONWIDTH);
    layerView->setColumnWidth(QG_LayerModel::PRINT, QG_LayerModel::ICONWIDTH);
    layerView->setColumnWidth(QG_LayerModel::CONSTRUCTION, QG_LayerModel::ICONWIDTH);
    layerView->setColumnWidth(QG_LayerModel::COLOR_SAMPLE, QG_LayerModel::ICONWIDTH);
    layerView->verticalHeader()->hide();

	QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);

	QHBoxLayout* layButtons = new QHBoxLayout;
    QToolButton* but;
    const QSize minButSize(28,28);
    // show all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/visible.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersDefreezeAll()));
    layButtons->addWidget(but);
    // hide all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/invisible.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Hide all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersFreezeAll()));
    layButtons->addWidget(but);
    // unlock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/unlocked.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Unlock all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersUnlockAll()));
    layButtons->addWidget(but);
    // lock all layers:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/locked.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Lock all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersLockAll()));
    layButtons->addWidget(but);
    // add layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/add.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Add a layer"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersAdd()));
    layButtons->addWidget(but);
    // remove layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/remove.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Remove the current layer"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersRemove()));
    layButtons->addWidget(but);
    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/rename_active_block.svg"));
    but->setMinimumSize(minButSize);
    but->setToolTip(tr("Modify layer attributes / rename"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersEdit()));
    layButtons->addWidget(but);

    // lineEdit to filter layer list with RegEx
    matchLayerName = new QLineEdit(this);
    matchLayerName->setReadOnly(false);
    matchLayerName->setPlaceholderText("Filter");
    matchLayerName->setClearButtonEnabled(true);
    matchLayerName->setToolTip(tr("Looking for matching layer names"));
    connect(matchLayerName, SIGNAL( textChanged(QString) ), this, SLOT( slotUpdateLayerList() ) );

    //lay->addWidget(caption);
    lay->addWidget(matchLayerName);
    lay->addLayout(layButtons);
    lay->addWidget(layerView);
	this->setLayout(lay);

    connect(layerView, SIGNAL(pressed(QModelIndex)), this, SLOT(slotActivated(QModelIndex)));
}



/**
 * Sets the layerlist this layer widget should show.
 *
 * @param showByBlock true: show the layer with the name "ByBlock" if
 *                    it exists.
 *                    false: don't show special layer "ByBlock"
 */
void QG_LayerWidget::setLayerList(RS_LayerList* layerList, bool showByBlock) {
    this->layerList = layerList;
    this->showByBlock = showByBlock;
    if (layerList != NULL) {
        this->layerList->setLayerWitget(this);
    }
    update();
}



void QG_LayerWidget::layerAdded(RS_Layer* layer)
{
    update();   // 1st apply the new layer to the view
    if (! matchLayerName->text().isEmpty()) {
        slotUpdateLayerList();
    }
    activateLayer(layer);
    update();   // update again, if new layer is last row, the height was wrong
}



/**
 * @brief getActiveName
 * @return the name of the active layer
 */
QString QG_LayerWidget::getActiveName() const
{
    if(layerList){
        RS_Layer* p=layerList->getActive();
        if(p) return p->getName();
    }
    return QString();
}



/**
 * Updates the layer box from the layers in the graphic.
 */
void QG_LayerWidget::update() {

    RS_DEBUG->print("QG_LayerWidget::update() begin");

    if (!layerView) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerView");
        return;
    }
    int yPos = layerView->verticalScrollBar()->value();
    layerView->resizeRowsToContents();
    layerView->verticalScrollBar()->setValue(yPos);

    if (!layerList) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr layerList");
        return;
    }
    layerModel->setLayerList(layerList);

    RS_DEBUG->print("QG_LayerWidget::update: reactivating current layer");

    RS_Layer* activeLayer = layerList->getActive();
    if (!activeLayer) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::update: nullptr activeLayer");
        return;
    }
    activateLayer(activeLayer);

    if (!lastLayer) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_LayerWidget::update: nullptr lastLayer");
        lastLayer = activeLayer;
    }

    RS_DEBUG->print("QG_LayerWidget::update(): OK");
}



/**
 * Activates the given layer and makes it the active
 * layer in the layerlist.
 */
void QG_LayerWidget::activateLayer(RS_Layer* layer, bool updateScroll) {
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if (!layer || !layerList) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layer or layerList");
        return;
    }

//    if (layer->getName() == layerList->getActive()->getName()) {
//        return;
//    }
    layerList->activate(layer);

    if (!layerModel) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: nullptr layerModel");
        return;
    }
    QModelIndex idx = layerModel->getIndex(layer);

    if (!idx.model() || !layerView) {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_LayerWidget::activateLayer: invalid layer or nullptr layerView");
        return;
    }
    layerView->setCurrentIndex(idx);

    if (!updateScroll) {
        int yPos = layerView->verticalScrollBar()->value();
        layerView->verticalScrollBar()->setValue(yPos);
    }

    //update active layer name in mainwindow status bar
    QC_ApplicationWindow::getAppWindow()->slotUpdateActiveLayer();

    RS_DEBUG->print("QG_LayerWidget::activateLayer() end");
}



/**
 * Called when the user activates (highlights) a layer.
 */
void QG_LayerWidget::slotActivated(QModelIndex layerIdx /*const QString& layerName*/) {
    if (!layerIdx.isValid() || layerList==NULL) {
        return;
    }

    RS_Layer* lay = layerModel->getLayer(layerIdx.row());
    if (lay == 0)
        return;

    if (layerIdx.column() == QG_LayerModel::NAME) {
        layerList->activate(lay, true);
        return;
    }

    switch(layerIdx.column()){
    case QG_LayerModel::VISIBLE:
        actionHandler->toggleVisibility(lay);
        break;
    case QG_LayerModel::LOCKED:
        actionHandler->toggleLock(lay);
        break;
    case QG_LayerModel::PRINT:
        actionHandler->togglePrint(lay);
        break;
    case QG_LayerModel::CONSTRUCTION:
        actionHandler->toggleConstruction(lay);
        break;
    default:
        break;
    }
}



/**
 * Called when reg-expresion matchLayerName->text changed
 */
void QG_LayerWidget::slotUpdateLayerList() {
    QRegExp rx("");
	int pos=0;
    QString  s, n;

    n=matchLayerName->text();
    rx.setPattern(n);
    rx.setPatternSyntax(QRegExp::WildcardUnix);

    for (unsigned int i=0; i<layerList->count() ; i++) {
        s=layerModel->getLayer(i)->getName();
		int f=rx.indexIn(s, pos);
		if ( !f ) {
            layerView->showRow(i);
            layerModel->getLayer(i)->visibleInLayerList(true);
        } else {
            layerView->hideRow(i);
            layerModel->getLayer(i)->visibleInLayerList(false);
        }
    }
}

/**
 * Shows a context menu for the layer widget. Launched with a right click.
 */
void QG_LayerWidget::contextMenuEvent(QContextMenuEvent *e) {

    if (actionHandler) {
        QMenu* contextMenu = new QMenu(this);
        QLabel* caption = new QLabel(tr("Layer Menu"), this);
        QPalette palette;
        palette.setColor(caption->backgroundRole(), RS_Color(0,0,0));
        palette.setColor(caption->foregroundRole(), RS_Color(255,255,255));
        caption->setPalette(palette);
        caption->setAlignment( Qt::AlignCenter );
        contextMenu->addAction( tr("&Defreeze all Layers"), actionHandler,
                                 SLOT(slotLayersDefreezeAll()), 0);
        contextMenu->addAction( tr("&Freeze all Layers"), actionHandler,
                                 SLOT(slotLayersFreezeAll()), 0);
        contextMenu->addAction( tr("&Unlock all Layers"), actionHandler,
                                 SLOT(slotLayersUnlockAll()), 0);
        contextMenu->addAction( tr("&Lock all Layers"), actionHandler,
                                 SLOT(slotLayersLockAll()), 0);
        contextMenu->addSeparator();
        contextMenu->addAction( tr("&Add Layer"), actionHandler,
                                 SLOT(slotLayersAdd()), 0);
        contextMenu->addAction( tr("&Remove Layer"), actionHandler,
                                 SLOT(slotLayersRemove()), 0);
        contextMenu->addAction( tr("Edit Layer &Attributes"), actionHandler,
                                 SLOT(slotLayersEdit()), 0);
        contextMenu->addAction( tr("Toggle Layer &Visibility"), actionHandler,
                                 SLOT(slotLayersToggleView()), 0);
        contextMenu->addAction( tr("Toggle Layer &Printing"), actionHandler,
                                 SLOT(slotLayersTogglePrint()), 0);
        contextMenu->addAction( tr("Toggle &Construction Layer"), actionHandler,
                                 SLOT(slotLayersToggleConstruction()), 0);
        contextMenu->exec(QCursor::pos());
        delete contextMenu;
    }

    e->accept();
}


/**
 * Escape releases focus.
 */
void QG_LayerWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {

    case Qt::Key_Escape:
        emit escape();
        break;

    default:
        QWidget::keyPressEvent(e);
        break;
    }
}


void QG_LayerWidget::activateLayer(int row)
{
    auto layer = layerModel->getLayer(row);
    if (layer)
        layerList->activate(layer, true);
    else
        qWarning("activateLayer: row %d doesn't exist", row);
}

