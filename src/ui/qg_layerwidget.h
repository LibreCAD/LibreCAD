/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
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

#include <qwidget.h>
#include <q3listbox.h>
//Added by qt3to4:
#include <QPixmap>
#include <QContextMenuEvent>
#include <QKeyEvent>

#include "rs_layerlist.h"
#include "rs_layerlistlistener.h"

#include "qg_actionhandler.h"



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
    void highlightLayer(RS_Layer* layer);
    void highlightLayer(const QString& name);

    virtual void layerActivated(RS_Layer* layer) {
        highlightLayer(layer);
    }
    virtual void layerAdded(RS_Layer* layer) {
        update();
        highlightLayer(layer);
    }
    virtual void layerEdited(RS_Layer*) {
        update();
    }
    virtual void layerRemoved(RS_Layer*) {
        update();
        highlightLayer(layerList->at(0));
    }
    virtual void layerToggled(RS_Layer*) {
        update();
    }

signals:
	void escape();

public slots:
    void slotActivated(const QString& layerName);
	void slotMouseButtonClicked(int button, Q3ListBoxItem* item, 
		const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
	virtual void keyPressEvent(QKeyEvent* e);

private:
    RS_LayerList* layerList;
    bool showByBlock;
    Q3ListBox* listBox;
	RS_Layer* lastLayer;
    QPixmap pxmLayerStatus00;
    QPixmap pxmLayerStatus01;
    QPixmap pxmLayerStatus10;
    QPixmap pxmLayerStatus11;
    QPixmap pxmVisible;
    QPixmap pxmHidden;
    QPixmap pxmAdd;
    QPixmap pxmRemove;
    QPixmap pxmEdit;
    QPixmap pxmDefreezeAll;
    QPixmap pxmFreezeAll;
    QG_ActionHandler* actionHandler;
};

#endif
