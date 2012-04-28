/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#ifndef QG_LAYERWIDGET_H
#define QG_LAYERWIDGET_H

#include <QWidget>
#include <QIcon>
#include <QAbstractTableModel>

#include "rs_layerlistlistener.h"
#include "rs_layerlist.h"

class QG_ActionHandler;
class QTableView;
class QLineEdit;

/**
 * Implementation of a model to use in QG_LayerWidget
 */
class QG_LayerModel: public QAbstractTableModel {
public:
    enum {
        VISIBLE,
        LOCKED,
        HelpLayer,
        NAME,
        LAST
    };
    QG_LayerModel(QObject * parent = 0);
    ~QG_LayerModel();
    Qt::ItemFlags flags ( const QModelIndex & /*index*/ ) {
            return Qt::ItemIsSelectable|Qt::ItemIsEnabled;}
    int columnCount(const QModelIndex &/*parent*/) const {return LAST;}
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QModelIndex parent ( const QModelIndex & index ) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    void setLayerList(RS_LayerList* ll);
    RS_Layer *getLayer( int row );
    QModelIndex getIndex (RS_Layer * lay);

private:
    QList<RS_Layer*> listLayer;
    QIcon layerVisible;
    QIcon layerHidden;
    QIcon layerDefreeze;
    QIcon layerFreeze;
    QIcon helpLayer;
};


/**
 * This is the Qt implementation of a widget which can view a
 * layer list and provides a user interface for basic layer actions.
 */
class QG_LayerWidget: public QWidget, public RS_LayerListListener {
    Q_OBJECT

public:
    QG_LayerWidget(QG_ActionHandler* ah, QWidget* parent,
                   const char* name=0, Qt::WFlags f = 0);
    ~QG_LayerWidget();

    void setLayerList(RS_LayerList* layerList, bool showByBlock);

    void update();
    void activateLayer(RS_Layer* layer);

    virtual void layerActivated(RS_Layer* layer) {
        activateLayer(layer);
    }
    virtual void layerAdded(RS_Layer* layer) {
        update();
        activateLayer(layer);
    }
    virtual void layerEdited(RS_Layer*) {
        update();
    }
   virtual void layerRemoved(RS_Layer*) {
        update();
        activateLayer(layerList->at(0));
    }
    virtual void layerToggled(RS_Layer*) {
        update();
    }

signals:
    void escape();

public slots:
    void slotActivated(QModelIndex layerIdx);
    void slotUpdateLayerList();

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    virtual void keyPressEvent(QKeyEvent* e);

private:
    RS_LayerList* layerList;
    bool showByBlock;
    QLineEdit* matchLayerName;
    QTableView* layerView;
    QG_LayerModel *layerModel;
    RS_Layer* lastLayer;   
    QG_ActionHandler* actionHandler;
};

#endif
