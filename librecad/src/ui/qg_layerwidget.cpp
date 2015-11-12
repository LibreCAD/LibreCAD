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

QG_LayerModel::QG_LayerModel(QObject * parent) : QAbstractTableModel(parent) {
    layerVisible = QIcon(":/ui/visibleblock.png");
    layerHidden = QIcon(":/ui/hiddenblock.png");
    layerDefreeze = QIcon(":/ui/unlockedlayer.png");
    layerFreeze = QIcon(":/ui/lockedlayer.png");
    layerPrint = QIcon(":/ui/fileprint.png");
    layerConstruction = QIcon(":/ui/constructionlayer.png");
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
	/* since 4.6 the recomended way is to use begin/endResetModel()
	 * TNick <nicu.tofan@gmail.com>
	 */
#if QT_VERSION >= 0x040600
    beginResetModel();
#endif
    listLayer.clear();
    if (ll == NULL) {
#if QT_VERSION >= 0x040600
        endResetModel();
#else
        reset();
#endif
        return;
    }
    for (unsigned i=0; i < ll->count(); ++i) {
        listLayer.append(ll->at(i));
    }
    std::sort( listLayer.begin(), listLayer.end(), [](const RS_Layer *s1, const RS_Layer *s2)-> bool{
        return s1->getName() < s2->getName();
    } );
//called to force redraw
#if QT_VERSION >= 0x040600
    endResetModel();
#else
    reset();
#endif
}


RS_Layer *QG_LayerModel::getLayer( int row ){
    if ( row >= listLayer.size() || row < 0)
        return NULL;
    return listLayer.at(row);
}

QModelIndex QG_LayerModel::getIndex (RS_Layer * lay){
    int row = listLayer.indexOf(lay);
    if (row<0)
        return QModelIndex();
    return createIndex ( row, NAME);
}

QPixmap createColorSampleForLayer(RS_Layer* layer)
{
	QPixmap pixmap(QSize(20,20));
	{
		pixmap.fill(layer->getPen().getColor().toQColor());
	}

	return pixmap;
}

QVariant QG_LayerModel::data ( const QModelIndex & index, int role ) const {
    if (!index.isValid() || index.row() >= listLayer.size())
        return QVariant();

    RS_Layer* lay = listLayer.at(index.row());

    if (role ==Qt::DecorationRole) {
        switch(index.column()){
        case VISIBLE:
            if (!lay->isFrozen()) {
                return layerVisible;
            }
            return layerHidden;
        case LOCKED:
            if (!lay->isLocked()) {
                return layerDefreeze;
            }
            return layerFreeze;
        case PRINT:
            return layerPrint.pixmap(QSize(20,20),
                                     lay->isPrint() ? QIcon::Normal : QIcon::Disabled,
                                     QIcon::On);
        case CONSTRUCTION:
            return layerConstruction.pixmap(QSize(14,14),
                                            lay->isConstruction() ? QIcon::Normal : QIcon::Disabled,
                                            QIcon::On);

		case COLOR_SAMPLE:
		{
			return createColorSampleForLayer(lay);
		}

        default:
            break;

        }

    }
    if (role ==Qt::DisplayRole && index.column() == NAME) {
        return lay->getName();
    }
//Other roles:
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
    layerView->setModel (layerModel);
    layerView->setShowGrid (false);
    layerView->setSelectionMode(QAbstractItemView::SingleSelection);
    layerView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layerView->setFocusPolicy(Qt::NoFocus);
    layerView->setMinimumHeight(140);
    layerView->setColumnWidth(QG_LayerModel::VISIBLE, 18);
    layerView->setColumnWidth(QG_LayerModel::LOCKED, 18);
    layerView->setColumnWidth(QG_LayerModel::PRINT, 24);
    layerView->setColumnWidth(QG_LayerModel::CONSTRUCTION, 18);
	layerView->setColumnWidth(QG_LayerModel::COLOR_SAMPLE, 24);
    layerView->verticalHeader()->hide();
    layerView->horizontalHeader()->setStretchLastSection(true);
    layerView->horizontalHeader()->hide();

	QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setSpacing ( 0 );
    lay->setContentsMargins(2, 2, 2, 2);

	QHBoxLayout* layButtons = new QHBoxLayout;
    QToolButton* but;
	const QSize minButSize(22,22);
    // show all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":ui/visiblelayer.png"));
	but->setMinimumSize(minButSize);
    but->setToolTip(tr("Show all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersDefreezeAll()));
    layButtons->addWidget(but);
    // hide all layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":ui/hiddenlayer.png"));
	but->setMinimumSize(minButSize);
    but->setToolTip(tr("Hide all layers"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersFreezeAll()));
    layButtons->addWidget(but);
    // add layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":ui/layeradd.png"));
	but->setMinimumSize(minButSize);
    but->setToolTip(tr("Add a layer"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersAdd()));
    layButtons->addWidget(but);
    // remove layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":ui/layerremove.png"));
	but->setMinimumSize(minButSize);
    but->setToolTip(tr("Remove the current layer"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersRemove()));
    layButtons->addWidget(but);
    // rename layer:
    but = new QToolButton(this);
    but->setIcon(QIcon(":ui/layeredit.png"));
	but->setMinimumSize(minButSize);
    but->setToolTip(tr("Modify layer attributes / rename"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotLayersEdit()));
    layButtons->addWidget(but);

    // lineEdit to filter layer list with RegEx
    matchLayerName = new QLineEdit(this);
    matchLayerName->setReadOnly(false);
    //matchLayerName->setText("*");
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

    int yPos = layerView->verticalScrollBar()->value();

    RS_Layer* activeLayer;
    if (layerList) {
        activeLayer = layerList->getActive();
    } else {
        activeLayer = NULL;
    }

    layerModel->setLayerList(layerList);

    if (layerList==NULL) {
        RS_DEBUG->print("QG_LayerWidget::update() abort");
        return;
    }

    RS_DEBUG->print("QG_LayerWidget::update() reactivating current layer");

    RS_Layer* l = lastLayer;
    activateLayer(activeLayer);
    lastLayer = l;
    layerView->resizeRowsToContents();
    layerView->verticalScrollBar()->setValue(yPos);

    RS_DEBUG->print("QG_LayerWidget::update() end");
}


/**
 * Activates the given layer and makes it the active
 * layer in the layerlist.
 */
void QG_LayerWidget::activateLayer(RS_Layer* layer, bool updateScroll) {
    RS_DEBUG->print("QG_LayerWidget::activateLayer() begin");

    if (layer==NULL || layerList==NULL) {
        return;
    }
    int yPos = layerView->verticalScrollBar()->value();


    layerList->activate(layer);

    layerList->activate(layer);
    QModelIndex idx = layerModel->getIndex (layer);

    layerView->setCurrentIndex ( idx );
	if (!updateScroll)
        layerView->verticalScrollBar()->setValue(yPos);

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

    RS_Layer * lay = layerModel->getLayer( layerIdx.row() );
    if (lay == 0)
        return;

    if (layerIdx.column() == QG_LayerModel::NAME) {
        lastLayer = layerList->getActive();
        layerList->activate(lay);
        lastLayer = layerList->getActive();
        layerList->activate(lay, true);
        return;
    }

    RS_Layer* l = layerList->getActive();
    layerList->activate(lay, true);
    switch(layerIdx.column()){
    case QG_LayerModel::VISIBLE:
        actionHandler->slotLayersToggleView();
        break;
    case QG_LayerModel::LOCKED:
        actionHandler->slotLayersToggleLock();
        break;
    case QG_LayerModel::PRINT:
        actionHandler->slotLayersTogglePrint();
        break;
    case QG_LayerModel::CONSTRUCTION:
        actionHandler->slotLayersToggleConstruction();
        break;
    default:
        activateLayer(l);
        return;
    }
    activateLayer(l, false);
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
#if QT_VERSION < 0x040600
    rx.setPatternSyntax(QRegExp::Wildcard);
#else
    rx.setPatternSyntax(QRegExp::WildcardUnix);
#endif

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

