/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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
#include <QItemSelection>

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
        PRINT,
        CONSTRUCTION,
		COLOR_SAMPLE,
        NAME,
        LAST
    };

    // the default icon size
    constexpr static int ICONWIDTH = 24;

	QG_LayerModel(QObject * parent = nullptr);
	~QG_LayerModel() = default;
    Qt::ItemFlags flags (const QModelIndex & index) const override
    {
        if (index.column() == 5)
            return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
        else
            return Qt::ItemIsEnabled;
    }
    int columnCount(const QModelIndex &/*parent*/) const  override {return LAST;}
    int rowCount ( const QModelIndex & parent = {} ) const override;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    QModelIndex parent ( const QModelIndex & index ) const override;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
    void setLayerList(RS_LayerList* ll);
    RS_Layer *getLayer( int row ) const;
    QModelIndex getIndex (RS_Layer * lay) const;

    RS_Layer* getActiveLayer() const
    {
        return activeLayer;
    }
    void setActiveLayer(RS_Layer* l)
    {
        activeLayer = l;
    }

private:
    QList<RS_Layer*> listLayer;
    QIcon layerVisible;
    QIcon layerHidden;
    QIcon layerDefreeze;
    QIcon layerFreeze;
    QIcon layerPrint;
    QIcon layerNoPrint;
    QIcon layerConstruction;
    QIcon layerNoConstruction;
    RS_Layer* activeLayer {nullptr};
};


/**
 * This is the Qt implementation of a widget which can view a
 * layer list and provides a user interface for basic layer actions.
 */
class QG_LayerWidget: public QWidget, public RS_LayerListListener {
    Q_OBJECT

public:
  QG_LayerWidget(QG_ActionHandler *ah, QWidget *parent,
                 const char *name = nullptr, Qt::WindowFlags f = {});
  virtual ~QG_LayerWidget() = default;

  void setLayerList(RS_LayerList *layerList, bool showByBlock);

  void update();
  void activateLayer(RS_Layer *layer, bool updateScroll = true);

  void layerActivated(RS_Layer *layer) override { activateLayer(layer);}
  void layerAdded(RS_Layer *layer) override;
  void layerEdited(RS_Layer *) override { update(); }
  void layerRemoved(RS_Layer *) override {
        update();
        activateLayer(layerList->at(0));
    }
    void layerToggled(RS_Layer*) override {
        update();
    }
    void layerToggledLock(RS_Layer*) override {
        update();
    }
    void layerToggledPrint(RS_Layer*) override {
        update();
    }
    void layerToggledConstruction(RS_Layer*) override {
        update();
    }

    QLineEdit* getMatchLayerName() {
        return matchLayerName;
    }
    /**
     * @brief getActiveName
     * @return the name of the active layer
     */
    QString getActiveName() const;

signals:
    void escape();

public slots:
    void slotActivated(QModelIndex layerIdx);
    void slotSelectionChanged(
        const QItemSelection &selected,
        const QItemSelection &deselected);
    void slotUpdateLayerList();
    void activateLayer(int row);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    virtual void keyPressEvent(QKeyEvent* e) override;

private:
    RS_LayerList* layerList;
    bool showByBlock = false;
    QLineEdit* matchLayerName;
    QTableView* layerView;
    QG_LayerModel *layerModel;
    RS_Layer* lastLayer;
    QG_ActionHandler* actionHandler;

    void restoreSelections();
};

#endif
