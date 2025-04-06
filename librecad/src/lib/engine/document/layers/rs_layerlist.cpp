/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include<iostream>

#include "rs_debug.h"
#include "rs_layerlist.h"
#include "rs_layer.h"
#include "rs_layerlistlistener.h"

/**
 * Default constructor.
 */
RS_LayerList::RS_LayerList() {
    m_activeLayer = nullptr;
    setModified(false);
}

/**
 * Removes all layers in the layerlist.
 */
void RS_LayerList::clear() {
    m_layers.clear();
    setModified(true);
}

QList<RS_Layer*>::iterator RS_LayerList::begin() {
    return m_layers.begin();
}

QList<RS_Layer*>::iterator RS_LayerList::end() {
    return m_layers.end();
}

QList<RS_Layer*>::const_iterator RS_LayerList::begin() const {
    return m_layers.begin();
}

QList<RS_Layer*>::const_iterator RS_LayerList::end() const {
    return m_layers.end();
}

/**
 * Activates the given layer.
 *
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(const QString& name, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate: %s, notify: %d begin",
                    name.toLatin1().data(), notify);

    activate(find(name), notify);
    /*
    if (activeLayer==nullptr) {
        RS_DEBUG->print("activeLayer is nullptr");
} else {
        RS_DEBUG->print("activeLayer is %s", activeLayer->getName().latin1());
}
    */

    RS_DEBUG->print("RS_LayerList::activate: %s end", name.toLatin1().data());
}

/**
 * Activates the given layer.
 *
 * @param notify Notify listeners.
 */
void RS_LayerList::activate(RS_Layer* layer, bool notify) {
    RS_DEBUG->print("RS_LayerList::activate notify: %d begin", notify);

    /*if (layer) {
        RS_DEBUG->print("RS_LayerList::activate: %s",
                        layer->getName().latin1());
} else {
        RS_DEBUG->print("RS_LayerList::activate: nullptr");
}*/

    m_activeLayer = layer;

    if (notify) {
        fireLayerActivated();
    }

    RS_DEBUG->print("RS_LayerList::activate end");
}

/**
 * @brief sort by layer names
 */
void RS_LayerList::sort() {
    std::stable_sort(m_layers.begin(), m_layers.end(), [](const RS_Layer* l0, const RS_Layer* l1)-> bool
    {
        return l0->getName() < l1->getName();
    });
}

void RS_LayerList::fireLayerAdded(RS_Layer* layer) {
    for (int i = 0; i < m_layerListListeners.size(); ++i) {
        RS_LayerListListener* l = m_layerListListeners.at(i);
        l->layerAdded(layer);
    }
}

/**
 * Adds a layer to the layer list.
 * If there is already a layer with the same name, no layer is
 * added. In that case the layer passed to the method will be deleted!
 * If no layer was active so far, the new layer becomes the active one.
 *
 * Listeners are notified.
 */
void RS_LayerList::add(RS_Layer* layerToAdd) {
    RS_DEBUG->print("RS_LayerList::addLayer()");

    if (layerToAdd == nullptr) {
        return;
    }

    // check if layer already exists:
    RS_Layer* existingLayer = find(layerToAdd->getName());
    if (existingLayer == nullptr) {
        m_layers.append(layerToAdd);
        this->sort();
        // notify listeners
        fireLayerAdded(layerToAdd);
        setModified(true);

        // if there was no active layer so far, activate this one.
        if (m_activeLayer == nullptr) {
            activate(layerToAdd);
        }
    }
    else {
        // if there was no active layer so far, activate this one.
        if (m_activeLayer == nullptr) {
            activate(existingLayer);
        }

        existingLayer->freeze(layerToAdd->isFrozen());
        existingLayer->lock(layerToAdd->isLocked());
        existingLayer->setPrint(layerToAdd->isPrint());
        existingLayer->setConverted(layerToAdd->isConverted());
        existingLayer->setConstruction(layerToAdd->isConstruction());
        existingLayer->visibleInLayerList(layerToAdd->isVisibleInLayerList());
        existingLayer->setPen(layerToAdd->getPen());

        delete layerToAdd;
        layerToAdd = nullptr;
    }
}

void RS_LayerList::fireLayerRemoved(RS_Layer* layer) {
    for (int i = 0; i < m_layerListListeners.size(); ++i) {
        RS_LayerListListener* l = m_layerListListeners.at(i);
        l->layerRemoved(layer);
    }
}

/**
 * Removes a layer from the list.
 * Listeners are notified after the layer was removed from
 * the list but before it gets deleted.
 */
void RS_LayerList::remove(RS_Layer* layerToRemove) {
    RS_DEBUG->print("RS_LayerList::removeLayer()");
    if (layerToRemove == nullptr) {
        return;
    }

    // here the layer is removed from the list but not deleted
    m_layers.removeOne(layerToRemove);

    fireLayerRemoved(layerToRemove);

    setModified(true);

    // activate an other layer if necessary:
    if (m_activeLayer == layerToRemove) {
        activate(m_layers.first());
    }

    // now it's save to delete the layer
    delete layerToRemove;
}

/**
 * Changes a layer's attributes. The attributes of layer 'layer'
 * are copied from layer 'source'.
 * Listeners are notified.
 */
void RS_LayerList::edit(RS_Layer* layer, const RS_Layer& source) {
    if (layer == nullptr) {
        return;
    }
    *layer = source;
    fireEdit(layer);
}

void RS_LayerList::fireEdit(RS_Layer* layer) {
    for (auto l : m_layerListListeners) {
        l->layerEdited(layer);
    }
    setModified(true);
}

/**
 * @return Pointer to the layer with the given name or
 * \p nullptr if no such layer was found.
 */
RS_Layer* RS_LayerList::find(const QString& name) {
    RS_Layer* ret = nullptr;
    for (auto l : m_layers) {
        if (l->getName() == name) {
            ret = l;
            break;
        }
    }
    return ret;
}

/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(const QString& name) {
    int ret = 0;
    for (auto l : m_layers) {
        if (l->getName() == name) {
            return ret;
        }
        else {
            ret++;
        }
    }
    return -1;
}

/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(RS_Layer* layer) {
    //RS_DEBUG->print("RS_LayerList::find begin");
    return m_layers.indexOf(layer);
}

/**
 * Switches on / off the given layer.
 * Listeners are notified.
 */
void RS_LayerList::toggle(const QString& name) {
    toggle(find(name));
}

/**
 * Switches on / off the given layer.
 * Listeners are notified.
 */
void RS_LayerList::toggle(RS_Layer* layer) {
    if (layer == nullptr) {
        return;
    }

    // set flags
    layer->toggle();
    setModified(true);

    // Notify listeners:
    for (auto* l : m_layerListListeners) {
        l->layerToggled(layer);
    }
}

/**
 * Locks or unlocks the given layer.
 * Listeners are notified.
 */
void RS_LayerList::toggleLock(RS_Layer* layer) {
    if (layer == nullptr) {
        return;
    }

    layer->toggleLock();
    setModified(true);

    // Notify listeners:
    for (auto l : m_layerListListeners) {
        l->layerToggledLock(layer);
    }
}

/**
 * Switch printing for the given layer on / off.
 * Listeners are notified.
 */
void RS_LayerList::togglePrint(RS_Layer* layer) {
    if (layer == nullptr) {
        return;
    }

    layer->togglePrint();
    setModified(true);

    // Notify listeners:
    for (auto l : m_layerListListeners) {
        l->layerToggledPrint(layer);
    }
}

/**
 * Switch construction attribute for the given layer on / off.
 * Listeners are notified.
 */
void RS_LayerList::toggleConstruction(RS_Layer* layer) {
    if (layer == nullptr) {
        return;
    }

    layer->toggleConstruction();
    setModified(true);

    // Notify listeners:
    for (auto l : m_layerListListeners) {
        l->layerToggledConstruction(layer);
    }
}

void RS_LayerList::fireLayerToggled() {
    setModified(true);
    for (auto l : m_layerListListeners) {
        l->layerToggled(nullptr);
    }
}

/**
 * Freezes or defreezes all layers.
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_LayerList::freezeAll(bool freeze) {
    for (unsigned l = 0; l < count(); l++) {
        if (at(l)->isVisibleInLayerList()) {
            at(l)->freeze(freeze);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::fireLayerActivated() {
    for (auto l : m_layerListListeners) {
        l->layerActivated(m_activeLayer);
    }
}

/**
 * Locks or unlocks all layers.
 *
 * @param lock true: lock, false: unlock
 */
void RS_LayerList::lockAll(bool lock) {
    for (unsigned l = 0; l < count(); l++) {
        if (at(l)->isVisibleInLayerList()) {
            at(l)->lock(lock);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::toggleLockMulti(QList<RS_Layer*> toggleLayers) {
    int count = toggleLayers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = toggleLayers.at(i);
        if (layer) {
            layer->toggleLock();
        }
    }

    fireLayerToggled();
}

void RS_LayerList::togglePrintMulti(QList<RS_Layer*> toggleLayers) {
    int count = toggleLayers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = toggleLayers.at(i);
        if (layer) {
            layer->togglePrint();
        }
    }
    fireLayerToggled();
}

void RS_LayerList::toggleConstructionMulti(QList<RS_Layer*> toggleLayers) {
    int count = toggleLayers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = toggleLayers.at(i);
        if (layer) {
            layer->toggleConstruction();
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setFreezeMulti(QList<RS_Layer*> layersEnable, QList<RS_Layer*> layersDisable) {
    int countUnFreeze = layersEnable.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersEnable.at(i);
        if (layer) {
            layer->freeze(false);
        }
    }
    int countFreeze = layersDisable.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersDisable.at(i);
        if (layer) {
            layer->freeze(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setLockMulti(QList<RS_Layer*> layersToUnlock, QList<RS_Layer*> layersToLock) {
    int countUnFreeze = layersToUnlock.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersToUnlock.at(i);
        if (layer) {
            layer->lock(false);
        }
    }
    int countFreeze = layersToLock.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersToLock.at(i);
        if (layer) {
            layer->lock(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setPrintMulti(QList<RS_Layer*> layersNoPrint, QList<RS_Layer*> layersPrint) {
    int countUnFreeze = layersNoPrint.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersNoPrint.at(i);
        if (layer) {
            layer->setPrint(false);
        }
    }
    int countFreeze = layersPrint.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersPrint.at(i);
        if (layer) {
            layer->setPrint(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setConstructionMulti(QList<RS_Layer*> layersNoConstruction, QList<RS_Layer*> layersConstruction) {
    int countUnFreeze = layersNoConstruction.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersNoConstruction.at(i);
        if (layer) {
            layer->setConstruction(false);
        }
    }
    int countFreeze = layersConstruction.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersConstruction.at(i);
        if (layer) {
            layer->setConstruction(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::toggleFreezeMulti(QList<RS_Layer*> toggleLayers) {
    int count = toggleLayers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = toggleLayers.at(i);
        if (layer) {
            layer->toggle();
        }
    }
    fireLayerToggled();
}

/**
 * adds a LayerListListener to the list of listeners. Listeners
 * are notified when the layer list changes.
 *
 * Typical listeners are: layer list widgets, pen toolbar, graphic view
 */
void RS_LayerList::addListener(RS_LayerListListener* listener) {
    // ensure that listener is added only once
    if (listener == nullptr) {
        return;
    }
    for (auto const l : m_layerListListeners) {
        if (l == listener) {
            return;
        }
    }
    m_layerListListeners.append(listener);
}

/**
 * removes a LayerListListener from the list of listeners.
 */
void RS_LayerList::removeListener(RS_LayerListListener* listener) {
    m_layerListListeners.removeOne(listener);
}

/**
 * Dumps the layers to stdout.
 */
std::ostream& operator <<(std::ostream& os, RS_LayerList& l) {
    os << "Layerlist: \n";
    for (unsigned i = 0; i < l.count(); i++) {
        os << *(l.at(i)) << "\n";
    }
    return os;
}

/**
 * Sets the layer lists modified status to 'm'.
 * Listeners are notified.
 */
void RS_LayerList::setModified(bool m) {
    m_modified = m;
}

// notify that list is updated via listerners
void RS_LayerList::slotUpdateLayerList() {
    setModified(true);
}
