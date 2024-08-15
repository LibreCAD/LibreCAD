/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#include "rs_layer.h"
#include "rs_layerlist.h"


/**
 * Default Constructor. You must call init manually before using 
 * this class.
 */
QG_LayerBox::QG_LayerBox(QWidget* parent)
        : QComboBox(parent) {

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
    layerList.sort();
    this->layerList = &layerList;

    if (showUnchanged) {
        addItem(tr("- Unchanged -"));
	}

    for (unsigned i=0; i<layerList.count(); ++i) {
        RS_Layer* lay = layerList.at(i);
        if (lay && (lay->getName()!="ByBlock" || showByBlock)) {
            addItem(lay->getName());
        }
    }

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotLayerChanged(int)));

    setCurrentIndex(0);

    slotLayerChanged(currentIndex());
}



/**
 * Sets the layer shown in the combobox to the given layer.
 */
void QG_LayerBox::setLayer(RS_Layer& layer) {
    currentLayer = &layer;

    //if (layer.getName()=="ByBlock" && showByBlock) {
    //    setCurrentItem(0);
    //} else {

    int i = findText(layer.getName());
    setCurrentIndex(i);
    //}

    //if (currentItem()!=7+(int)showByBlock*2) {
    slotLayerChanged(currentIndex());
    //}
}



/**
 * Sets the layer shown in the combobox to the given layer.
 */
void QG_LayerBox::setLayer(QString& layer) {

    //if (layer.getName()=="ByBlock" && showByBlock) {
    //    setCurrentItem(0);
    //} else {
    int i = findText(layer);
    setCurrentIndex(i);
    //}

    //if (currentItem()!=7+(int)showByBlock*2) {
    slotLayerChanged(currentIndex());
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

    currentLayer = layerList->find(itemText(index));

    //printf("Current color is (%d): %s\n",
    //       index, currentLayer.name().latin1());

    emit layerChanged(currentLayer);
}


