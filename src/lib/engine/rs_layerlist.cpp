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


#include "rs_layerlist.h"



/**
 * Default constructor.
 */
RS_LayerList::RS_LayerList() {
    layers.setAutoDelete(false);
    layerListListeners.setAutoDelete(false);
    activeLayer = NULL;
	setModified(false);
}



/**
 * Removes all layers in the layerlist.
 */
void RS_LayerList::clear() {
    layers.clear();
	setModified(true);
}



/**
 * Activates the given layer.
 * 
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(const RS_String& name, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate: %s, notify: %d begin", name.latin1(), notify);

    activate(find(name), notify);
    /*
    if (activeLayer==NULL) {
        RS_DEBUG->print("activeLayer is NULL");
} else {
        RS_DEBUG->print("activeLayer is %s", activeLayer->getName().latin1());
}
    */

    RS_DEBUG->print("RS_LayerList::activate: %s end", name.latin1());
}



/**
 * Activates the given layer.
 * 
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(RS_Layer* layer, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate notify: %d begin", notify);

    /*if (layer!=NULL) {
        RS_DEBUG->print("RS_LayerList::activate: %s",
                        layer->getName().latin1());
} else {
        RS_DEBUG->print("RS_LayerList::activate: NULL");
}*/

    activeLayer = layer;

    if (notify) {
       for (uint i=0; i<layerListListeners.count(); ++i) {
           RS_LayerListListener* l = layerListListeners.at(i);

           l->layerActivated(activeLayer);
		   RS_DEBUG->print("RS_LayerList::activate listener notified");
       }
    }

    RS_DEBUG->print("RS_LayerList::activate end");
}



/**
 * Adds a layer to the layer list.
 * If there is already a layer with the same name, no layer is 
 * added. In that case the layer passed to the methode will be deleted!
 * If no layer was active so far, the new layer becomes the active one.
 *
 * Listeners are notified.
 */
void RS_LayerList::add(RS_Layer* layer) {
    RS_DEBUG->print("RS_LayerList::addLayer()");

    if (layer==NULL) {
        return;
    }

    // check if layer already exists:
    RS_Layer* l = find(layer->getName());
    if (l==NULL) {
        layers.append(layer);

        // notify listeners
        for (uint i=0; i<layerListListeners.count(); ++i) {
            RS_LayerListListener* l = layerListListeners.at(i);
            l->layerAdded(layer);
        }
		setModified(true);

        // if there was no active layer so far, activate this one.
        if (activeLayer==NULL) {
            activate(layer);
        }
    } else {
        // if there was no active layer so far, activate this one.
        if (activeLayer==NULL) {
            activate(l);
        }

		l->setPen(layer->getPen());

        delete layer;
        layer = NULL;
    }
}



/**
 * Removes a layer from the list.
 * Listeners are notified after the layer was removed from 
 * the list but before it gets deleted.
 */
void RS_LayerList::remove(RS_Layer* layer) {
    RS_DEBUG->print("RS_LayerList::removeLayer()");
    if (layer==NULL) {
        return;
    }

    // here the layer is removed from the list but not deleted
    layers.remove(layer);

    for (uint i=0; i<layerListListeners.count(); ++i) {
        RS_LayerListListener* l = layerListListeners.at(i);
        l->layerRemoved(layer);
    }
		
	setModified(true);

    // activate an other layer if necessary:
    if (activeLayer==layer) {
        activate(layers.first());
    }

    // now it's save to delete the layer
    delete layer;
}



/**
 * Changes a layer's attributes. The attributes of layer 'layer'
 * are copied from layer 'source'.
 * Listeners are notified.
 */
void RS_LayerList::edit(RS_Layer* layer, const RS_Layer& source) {
    if (layer==NULL) {
        return;
    }

    *layer = source;

    for (uint i=0; i<layerListListeners.count(); ++i) {
        RS_LayerListListener* l = layerListListeners.at(i);

        l->layerEdited(layer);
    }
	
	setModified(true);
}



/**
 * @return Pointer to the layer with the given name or
 * \p NULL if no such layer was found.
 */
RS_Layer* RS_LayerList::find(const RS_String& name) {
    //RS_DEBUG->print("RS_LayerList::find begin");

    RS_Layer* ret = NULL;

    for (RS_Layer* l=layers.first();
            l!=NULL;
            l=layers.next()) {

        if (l->getName()==name) {
            ret = l;
        }
    }

    //RS_DEBUG->print("RS_LayerList::find end");

    return ret;
}



/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(const RS_String& name) {
    //RS_DEBUG->print("RS_LayerList::find begin");

    int ret = -1;
	int i = 0;

    for (RS_Layer* l=layers.first();
            l!=NULL;
            l=layers.next()) {

        if (l->getName()==name) {
            ret = i;
			break;
        }
		i++;
    }

    //RS_DEBUG->print("RS_LayerList::find end");

    return ret;
}


/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(RS_Layer* layer) {
    //RS_DEBUG->print("RS_LayerList::find begin");

    int ret = -1;
	int i = 0;

    for (RS_Layer* l=layers.first();
            l!=NULL;
            l=layers.next()) {

        if (l==layer) {
            ret = i;
			break;
        }
		i++;
    }

    //RS_DEBUG->print("RS_LayerList::find end");

    return ret;
}


/**
 * Switches on / off the given layer. 
 * Listeners are notified.
 */
void RS_LayerList::toggle(const RS_String& name) {
    toggle(find(name));
}



/**
 * Switches on / off the given layer. 
 * Listeners are notified.
 */
void RS_LayerList::toggle(RS_Layer* layer) {
    if (layer==NULL) {
        return;
    }

    layer->toggle();

    // Notify listeners:
    for (uint i=0; i<layerListListeners.count(); ++i) {
        RS_LayerListListener* l = layerListListeners.at(i);
        l->layerToggled(layer);
    }
}



/**
 * Locks or unlocks the given layer. 
 * Listeners are notified.
 */
void RS_LayerList::toggleLock(RS_Layer* layer) {
    if (layer==NULL) {
        return;
    }

    layer->toggleLock();

    // Notify listeners:
    for (uint i=0; i<layerListListeners.count(); ++i) {
        RS_LayerListListener* l = layerListListeners.at(i);
        l->layerToggled(layer);
    }
}



/**
 * Freezes or defreezes all layers.
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_LayerList::freezeAll(bool freeze) {

    for (uint l=0; l<count(); l++) {
        at(l)->freeze(freeze);
    }

    for (uint i=0; i<layerListListeners.count(); ++i) {
        RS_LayerListListener* l = layerListListeners.at(i);
        l->layerToggled(NULL);
    }
}



/**
 * adds a LayerListListener to the list of listeners. Listeners
 * are notified when the layer list changes.
 *
 * Typical listeners are: layer list widgets, pen toolbar, graphic view
 */
void RS_LayerList::addListener(RS_LayerListListener* listener) {
    layerListListeners.append(listener);
}



/**
 * removes a LayerListListener from the list of listeners. 
 */
void RS_LayerList::removeListener(RS_LayerListListener* listener) {
    layerListListeners.remove(listener);
}



/**
 * Dumps the layers to stdout.
 */
std::ostream& operator << (std::ostream& os, RS_LayerList& l) {

    os << "Layerlist: \n";
    for (uint i=0; i<l.count(); i++) {
        os << *(l.at(i)) << "\n";
    }

    return os;
}


