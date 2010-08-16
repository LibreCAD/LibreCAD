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

#include "qg_layerbox.h"

//#include <qpainter.h>


/**
 * Default Constructor. You must call init manually before using 
 * this class.
 */
QG_LayerBox::QG_LayerBox(QWidget* parent, const char* name)
        : QComboBox(parent, name) {

    showByBlock = false;
	showUnchanged = false;
	unchanged = false;
    layerList = NULL;
    currentLayer = NULL;
}



/**
 * Destructor
 */
QG_LayerBox::~QG_LayerBox() {}



/**
 * Initialisation (called manually only once).
 *
 * @param layerList Layer list which provides the layer names that are 
 *                  available.
 * @param showByBlock true: Show attribute ByBlock.
 */
void QG_LayerBox::init(RS_LayerList& layerList, 
		bool showByBlock, bool showUnchanged) {
    this->showByBlock = showByBlock;
	this->showUnchanged = showUnchanged;
    this->layerList = &layerList;

    if (showUnchanged) {
        insertItem(tr("- Unchanged -"));
	}

    for (uint i=0; i<layerList.count(); ++i) {
        RS_Layer* lay = layerList.at(i);
        if (lay!=NULL && (lay->getName()!="ByBlock" || showByBlock)) {
            insertItem(lay->getName());
        }
    }

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotLayerChanged(int)));

    setCurrentItem(0);

    slotLayerChanged(currentItem());
}



/**
 * Sets the layer shown in the combobox to the given layer.
 */
void QG_LayerBox::setLayer(RS_Layer& layer) {
    currentLayer = &layer;

    //if (layer.getName()=="ByBlock" && showByBlock) {
    //    setCurrentItem(0);
    //} else {

    setCurrentText(layer.getName());
    //}

    //if (currentItem()!=7+(int)showByBlock*2) {
    slotLayerChanged(currentItem());
    //}
}



/**
 * Sets the layer shown in the combobox to the given layer.
 */
void QG_LayerBox::setLayer(RS_String& layer) {

    //if (layer.getName()=="ByBlock" && showByBlock) {
    //    setCurrentItem(0);
    //} else {
    setCurrentText(layer);
    //}

    //if (currentItem()!=7+(int)showByBlock*2) {
    slotLayerChanged(currentItem());
    //}
}



/**
 * Called when the color has changed. This method 
 * sets the current color to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual color.
 */
void QG_LayerBox::slotLayerChanged(int index) {
    //currentLayer.resetFlags();

	if (index==0 && showUnchanged) {
		unchanged = true;
	}
	else {
		unchanged = false;
	}

    currentLayer = layerList->find(text(index));

    //printf("Current color is (%d): %s\n",
    //       index, currentLayer.name().latin1());

    emit layerChanged(currentLayer);
}


