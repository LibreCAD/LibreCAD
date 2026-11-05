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

#include "rs_layerlist.h"

#include<iostream>

#include <QObject>
#include <QSet>
#include <QtAlgorithms>

#include "rs_debug.h"
#include "rs_layer.h"
#include "rs_layerlistlistener.h"

namespace {
bool byName(const RS_Layer* l0, const RS_Layer* l1) {
    return l0->getName() < l1->getName();
}

QString nameKey(const QString& name) {
    return name.normalized(QString::NormalizationForm_C);
}
} // namespace

/**
 * Default constructor.
 */
RS_LayerList::RS_LayerList() {
    m_activeLayer = nullptr;
    setModified(false);
}

/**
 * Frees the layers, after telling the listeners that still listen (see
 * RS_LayerListListener::layerListDestroyed()): a dock attached to the list has a pointer to it, and
 * rows for the layers. The listeners are unlisted one at a time, oldest first, each before it is told,
 * so one that unregisters itself in the callback, or deletes another, cannot make the loop skip or
 * repeat one.
 */
RS_LayerList::~RS_LayerList() {
    QList<RS_Layer*> removed;
    removed.swap(m_layers);
    m_layerSet.clear();
    m_byName.clear();
    m_activeLayer = nullptr;
    m_layerListListeners.drain([](RS_LayerListListener* listener) {
        listener->layerListDestroyed();
    });
    qDeleteAll(removed);
}

/**
 * Removes all layers in the layerlist and frees them: the list owns its
 * layers, as remove() and add() (for a rejected duplicate) always assumed.
 * Listeners are told once the list is empty and before the layers are freed,
 * as remove() does for one layer: a dock's table model lists layers by
 * pointer, and reading a freed one crashed (#2969).
 */
void RS_LayerList::clear() {
    QList<RS_Layer*> removed;
    removed.swap(m_layers);
    m_layerSet.clear();
    m_byName.clear();
    m_activeLayer = nullptr;
    setModified(true);
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerListCleared();
    }
    qDeleteAll(removed);
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
 * Activates the layer of the given name, see activate(RS_Layer*, bool).
 *
 * @param name
 * @param notify Notify listeners.
 * @return true if the layer is the active layer now
 */
bool RS_LayerList::activate(const QString& name, const bool notify) {
    RS_DEBUG->print("RS_LayerList::activate: %s, notify: %d begin",
                    name.toLatin1().data(), static_cast<int>(notify));

    const bool activated = activate(find(name), notify);
    RS_DEBUG->print("RS_LayerList::activate: %s end", name.toLatin1().data());
    return activated;
}

/**
 * Activates the given layer, as a layer is made current by choice.
 *
 * A hidden layer can be current: it stays hidden, and so do the entities drawn on it (#3229).
 * A locked layer is not made current, since what is drawn goes to the current layer and a
 * locked layer must not change. (The current layer is locked only when no other layer is
 * unlocked, see keepCurrentLayerUnlocked(); activating it again, as editing its pen does to tell
 * the listeners, works.)
 *
 * @param layer a layer of this list
 * @param notify Notify listeners.
 * @return true if the layer is the active layer now
 */
bool RS_LayerList::activate(RS_Layer* layer, const bool notify) {
    RS_DEBUG->print("RS_LayerList::activate notify: %d begin", static_cast<int>(notify));
    if (!canActivate(layer)) {
        return false;
    }
    setActive(layer, notify);
    RS_DEBUG->print("RS_LayerList::activate end");
    return true;
}

/**
 * @return true if activate() makes the layer the active one: it is a layer of this list, and it
 * is not locked or is the active layer already
 */
bool RS_LayerList::canActivate(RS_Layer* layer) const {
    return m_layerSet.contains(layer) && (layer == m_activeLayer || !layer->isLocked());
}

/**
 * Makes the given layer of this list the active one whatever its flags, as a drawing was
 * saved with it.
 *
 * @param layer a layer of this list
 * @param notify Notify listeners.
 */
void RS_LayerList::setActive(RS_Layer* layer, const bool notify) {
    if (!m_layerSet.contains(layer)) {
        return;
    }
    m_activeLayer = layer;

    if (notify) {
        fireLayerActivated();
    }
}

/**
 * Makes the layer a drawing was saved with current, as it is opened. It is current whatever its
 * flags, hidden or not, but a locked one gives way to a layer that can be current, as settleActiveLayer()
 * says. Listeners are notified.
 *
 * @param layer a layer of this list
 */
void RS_LayerList::restoreActive(RS_Layer* layer) {
    if (!m_layerSet.contains(layer)) {
        return;
    }
    m_activeLayer = layer;
    settleActiveLayer();
    fireLayerActivated();
}

/**
 * Makes the active layer one that can be: a drawing has one, and it is not locked while another layer
 * is not. A drawing is opened with no active layer, or with a locked one (AutoCAD lets a locked layer
 * be current and a file may say so); the replacement is the one a removed current layer gets,
 * shown layers first. Listeners are not notified.
 */
void RS_LayerList::settleActiveLayer() {
    ensureActiveLayer();
    if (m_activeLayer != nullptr && m_activeLayer->isLocked()) {
        if (RS_Layer* better = bestLayerToActivate(m_activeLayer, false)) {
            m_activeLayer = better;
        }
    }
}

/**
 * @brief sort by layer names
 */
void RS_LayerList::sort() {
    std::stable_sort(m_layers.begin(), m_layers.end(), byName);
    m_sortedAt = RS_Layer::nameChanges();
}

void RS_LayerList::fireLayerAdded(RS_Layer* layer) const {
    for (const auto l : m_layerListListeners) {
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
        if (m_sortedAt == RS_Layer::nameChanges()) {
            // still sorted: the place a stable sort would move the layer to
            m_layers.insert(std::upper_bound(m_layers.cbegin(), m_layers.cend(), layerToAdd, byName), layerToAdd);
        }
        else {
            m_layers.append(layerToAdd);
            this->sort();
        }
        m_layerSet.insert(layerToAdd);
        m_byName.insert(nameKey(layerToAdd->getName()), layerToAdd);
        // notify listeners
        fireLayerAdded(layerToAdd);
        setModified(true);

        // if there was no active layer so far, activate this one, locked or not: there is no other
        if (m_activeLayer == nullptr) {
            setActive(layerToAdd);
        }
    }
    else {
        // if there was no active layer so far, activate this one.
        if (m_activeLayer == nullptr) {
            setActive(existingLayer);
        }
        if (existingLayer == layerToAdd) {
            // already listed: nothing to merge, and it must not be freed
            return;
        }

        existingLayer->freeze(layerToAdd->isFrozen());
        existingLayer->lock(layerToAdd->isLocked());
        existingLayer->setPrint(layerToAdd->isPrint());
        existingLayer->setConverted(layerToAdd->isConverted());
        existingLayer->setConstruction(layerToAdd->isConstruction());
        existingLayer->visibleInLayerList(layerToAdd->isVisibleInLayerList());
        existingLayer->setPen(layerToAdd->getPen());

        delete layerToAdd;
    }
}

void RS_LayerList::fireLayerRemoved(RS_Layer* layer) const {
    for (const auto l : m_layerListListeners) {
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
    m_layerSet.remove(layerToRemove);
    m_byName.clear(); // find() indexes the layers that are left

    setModified(true);

    // Select a survivor before notifying listeners.
    if (m_activeLayer == layerToRemove) {
        m_activeLayer = nullptr;
        ensureActiveLayer();
    }

    fireLayerRemoved(layerToRemove);

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
    fireLayerEdited(layer);
}

void RS_LayerList::fireLayerEdited(RS_Layer* layer) {
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerEdited(layer);
    }
    setModified(true);
}

/**
 * @return Pointer to the layer with the given name or
 * \p nullptr if no such layer was found.
 */
RS_Layer* RS_LayerList::find(const QString& name) {
    // fixme - sand - merge - copypaste
    // NFC-normalize both sides so a layer round-tripped through tools that
    // emit decomposed (NFD) Unicode still matches a composed (NFC) lookup.
    if (m_indexedAt != RS_Layer::nameChanges() || m_byName.size() != m_layers.size()) {
        // Last to first, so that of two layers of one name the first is found.
        m_byName.clear();
        for (auto it = m_layers.crbegin(); it != m_layers.crend(); ++it) {
            m_byName.insert(nameKey((*it)->getName()), *it);
        }
        m_indexedAt = RS_Layer::nameChanges();
    }
    return m_byName.value(nameKey(name));
}

/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(const QString& name) {
    // fixme - sand - merge - copypaste
    const QString k = name.normalized(QString::NormalizationForm_C);
    int ret = 0;
    for (auto l : m_layers) {
        if (l->getName().normalized(QString::NormalizationForm_C) == k) {
            return ret;
        }
        ret++;
    }
    return -1;
}

/**
 * @return Index of the given layer in the layer list or -1 if the layer
 * was not found.
 */
int RS_LayerList::getIndex(RS_Layer* layer) const {
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
    for (auto* l : std::as_const(m_layerListListeners)) {
            l->layerToggled(layer);
    }
}

/**
 * Locks or unlocks the given layer; locking the current layer is handled as
 * keepCurrentLayerUnlocked() says.
 * Listeners are notified.
 */
RS_LayerList::CurrentLayerLock RS_LayerList::toggleLock(RS_Layer* layer) {
    if (layer == nullptr) {
        return {};
    }

    const bool currentWasLocked = m_activeLayer != nullptr && m_activeLayer->isLocked();
    layer->toggleLock();
    const CurrentLayerLock outcome = keepCurrentLayerUnlocked(currentWasLocked);
    setModified(true);

    // Notify listeners:
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerToggledLock(layer);
    }
    if (outcome.wasReplaced()) {
        fireActiveLayerReplaced();
    }
    return outcome;
}

/**
 * What the lock controls (the lock icon of the Layer List and the Layer Tree, the property sheet's
 * Locked, Lock all layers) ask before they offer to lock a layer: locking the current layer needs
 * another layer that is not locked to take its place, see keepCurrentLayerUnlocked(); without one
 * the lock is refused, so the control is disabled rather than refusing when it is used.
 *
 * @return true if toggleLock() locks the layer: it is a layer of this list, it is not locked, and
 * it is not the current layer or another layer is not locked
 */
bool RS_LayerList::canLock(const RS_Layer* layer) const {
    if (layer == nullptr || layer->isLocked() || !m_layerSet.contains(const_cast<RS_Layer*>(layer))) {
        return false;
    }
    if (layer != m_activeLayer) {
        return true;
    }
    return bestLayerToActivate(layer, false) != nullptr;
}

/**
 * @return what a lock control that canToggleLock() disabled says in its tool tip
 */
QString RS_LayerList::lockUnavailableText() {
    return QObject::tr("The current layer cannot be locked: no other layer is unlocked to take its place.");
}

/**
 * @return true if toggleLock() changes the layer: it unlocks a locked layer of this list, and
 * locks an unlocked one when canLock() says it can
 */
bool RS_LayerList::canToggleLock(const RS_Layer* layer) const {
    if (layer == nullptr || !m_layerSet.contains(const_cast<RS_Layer*>(layer))) {
        return false;
    }
    return layer->isLocked() || canLock(layer);
}

/**
 * @return true if locking the given layers together (a group of the Layer Tree, the layers a
 * lock icon stands for) locks at least one of them
 */
bool RS_LayerList::canLockAny(const QList<RS_Layer*>& layers) const {
    for (const RS_Layer* layer : layers) {
        if (canLock(layer)) {
            return true;
        }
    }
    return false;
}

/**
 * @return true if lockAll() locks a layer: one that the Layer List dock lists and that canLock()
 * says can be locked
 */
bool RS_LayerList::canLockAll() const {
    for (const RS_Layer* layer : std::as_const(m_layers)) {
        if (layer->isVisibleInLayerList() && canLock(layer)) {
            return true;
        }
    }
    return false;
}

/**
 * The Toggle Layer Lock action (menu, toolbar, Layer List menu) toggles the layers selected in the
 * layer list, or the current layer when none is selected.
 *
 * @return true if it changes one of them: it unlocks a locked layer or locks one that canLock()
 */
bool RS_LayerList::canToggleLockOfSelection() const {
    bool anySelected = false;
    for (const RS_Layer* layer : std::as_const(m_layers)) {
        if (layer->isVisibleInLayerList() && layer->isSelectedInLayerList()) {
            anySelected = true;
            if (canToggleLock(layer)) {
                return true;
            }
        }
    }
    return !anySelected && canToggleLock(m_activeLayer);
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
    for (const auto l : std::as_const(m_layerListListeners)) {
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
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerToggledConstruction(layer);
    }
}

void RS_LayerList::fireLayerToggled() {
    setModified(true);
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerToggled(nullptr);
    }
}

/**
 * Freezes or defreezes all layers that the Layer List dock lists; its filter
 * leaves the other layers as they are. Freezing leaves the active layer as it
 * is ("Hide all layers except current").
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_LayerList::freezeAll(const bool freeze) {
    for (unsigned l = 0; l < count(); l++) {
        if (at(l)->isVisibleInLayerList() && !(freeze && at(l) == m_activeLayer)) {
            at(l)->freeze(freeze);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::fireLayerActivated() {
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->layerActivated(m_activeLayer);
    }
}

/**
 * Locks or unlocks all layers that the Layer List dock lists. Locking leaves the current layer
 * unlocked, with every layer locked ("Lock all layers" locks all but the current layer, as "Hide all
 * layers except current" hides all but it); with a layer the dock does not list unlocked, that one
 * becomes current, as keepCurrentLayerUnlocked() says.
 *
 * @param lock true: lock, false: unlock
 */
RS_LayerList::CurrentLayerLock RS_LayerList::lockAll(const bool lock) {
    const bool currentWasLocked = m_activeLayer != nullptr && m_activeLayer->isLocked();
    for (unsigned l = 0; l < count(); l++) {
        if (at(l)->isVisibleInLayerList()) {
            at(l)->lock(lock);
        }
    }
    const CurrentLayerLock outcome = keepCurrentLayerUnlocked(currentWasLocked);
    fireLayerToggled();
    if (outcome.wasReplaced()) {
        fireActiveLayerReplaced();
    }
    return outcome;
}

RS_LayerList::CurrentLayerLock RS_LayerList::toggleLockMulti(const QList<RS_Layer*>& layers) {
    const bool currentWasLocked = m_activeLayer != nullptr && m_activeLayer->isLocked();
    const int count = layers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = layers.at(i);
        if (layer != nullptr    ) {
            layer->toggleLock();
        }
    }

    const CurrentLayerLock outcome = keepCurrentLayerUnlocked(currentWasLocked);
    fireLayerToggled();
    if (outcome.wasReplaced()) {
        fireActiveLayerReplaced();
    }
    return outcome;
}

void RS_LayerList::togglePrintMulti(const QList<RS_Layer*>& layers) {
    const int count = layers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = layers.at(i);
        if (layer != nullptr) {
            layer->togglePrint();
        }
    }
    fireLayerToggled();
}

void RS_LayerList::toggleConstructionMulti(const QList<RS_Layer*>& layers) {
    const int count = layers.count();
    for (int i = 0; i < count; i++) {
        RS_Layer* layer = layers.at(i);
        if (layer != nullptr) {
            layer->toggleConstruction();
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setFreezeMulti(const QList<RS_Layer*>& layersEnable, const QList<RS_Layer*>& layersDisable) {
    const int countUnFreeze = layersEnable.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersEnable.at(i);
        if (layer != nullptr) {
            layer->freeze(false);
        }
    }
    const int countFreeze = layersDisable.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersDisable.at(i);
        if (layer != nullptr) {
            layer->freeze(true);
        }
    }
    fireLayerToggled();
}

RS_LayerList::CurrentLayerLock RS_LayerList::setLockMulti(const QList<RS_Layer*>& layersToUnlock,
                                                          const QList<RS_Layer*>& layersToLock) {
    const bool currentWasLocked = m_activeLayer != nullptr && m_activeLayer->isLocked();
    const int countUnFreeze = layersToUnlock.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersToUnlock.at(i);
        if (layer != nullptr) {
            layer->lock(false);
        }
    }
    const int countFreeze = layersToLock.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersToLock.at(i);
        if (layer != nullptr) {
            layer->lock(true);
        }
    }
    const CurrentLayerLock outcome = keepCurrentLayerUnlocked(currentWasLocked);
    fireLayerToggled();
    if (outcome.wasReplaced()) {
        fireActiveLayerReplaced();
    }
    return outcome;
}

void RS_LayerList::setPrintMulti(const QList<RS_Layer*>& layersNoPrint, const QList<RS_Layer*>& layersPrint) {
    const int countUnFreeze = layersNoPrint.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersNoPrint.at(i);
        if (layer != nullptr) {
            layer->setPrint(false);
        }
    }
    const int countFreeze = layersPrint.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersPrint.at(i);
        if (layer != nullptr) {
            layer->setPrint(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::setConstructionMulti(const QList<RS_Layer*>& layersNoConstruction, const QList<RS_Layer*>& layersConstruction) {
    const int countUnFreeze = layersNoConstruction.count();
    for (int i = 0; i < countUnFreeze; i++) {
        RS_Layer* layer = layersNoConstruction.at(i);
        if (layer != nullptr) {
            layer->setConstruction(false);
        }
    }
    const int countFreeze = layersConstruction.count();
    for (int i = 0; i < countFreeze; i++) {
        RS_Layer* layer = layersConstruction.at(i);
        if (layer != nullptr) {
            layer->setConstruction(true);
        }
    }
    fireLayerToggled();
}

void RS_LayerList::toggleFreezeMulti(const QList<RS_Layer*>& layers) {
    QSet<RS_Layer*> toggled;
    for (RS_Layer* layer : layers) {
        if (layer != nullptr && !toggled.contains(layer)) {
            layer->toggle();
            toggled.insert(layer);
        }
    }
    fireLayerToggled();
}

/**
 * Makes a layer active when none of this list is: the active one was removed, or a drawing was
 * loaded without one. An active layer that is hidden or locked stays active, as hiding or locking
 * it does not change the current layer. The replacement is the first layer that can be made
 * current, shown ones first; when every layer is locked, the first shown one, else the first.
 * Listeners are not notified.
 */
void RS_LayerList::ensureActiveLayer() {
    if (m_layerSet.contains(m_activeLayer)) {
        return;
    }
    m_activeLayer = bestLayerToActivate(nullptr, true);
}

/**
 * @param except a layer that is not to be chosen, or nullptr
 * @param allowLocked whether a locked layer may be chosen when no unlocked one is left
 * @return the first layer of the list that is shown and not locked, else the first one that is not
 * locked, though hidden, else (when allowed) the first shown one, else the first; nullptr if none
 */
RS_Layer* RS_LayerList::bestLayerToActivate(const RS_Layer* except, const bool allowLocked) const {
    RS_Layer* best = nullptr;
    int bestRank = 4;
    for (RS_Layer* layer : std::as_const(m_layers)) {
        if (layer == except || (layer->isLocked() && !allowLocked)) {
            continue;
        }
        const int rank = (layer->isLocked() ? 2 : 0) + (layer->isFrozen() ? 1 : 0);
        if (rank < bestRank) {
            best = layer;
            bestRank = rank;
            if (rank == 0) {
                break;
            }
        }
    }
    return best;
}

/**
 * A locked layer must not change, and what is drawn goes to the current layer: so a locked layer is
 * not current. When a request to lock layers has locked the current layer, the first layer that is
 * shown and not locked (else one that is hidden) becomes current in its place (nothing is moved to
 * it: see RS_LayerListListener::activeLayerReplaced()); when no other layer is unlocked there is
 * none to take its place, and the current layer is not locked.
 *
 * @param wasLocked whether the current layer was locked before the request, as a drawing may be
 * opened that way: it is then left as it is
 * @return what became of the current layer; the caller notifies the listeners (about the toggles
 * first, then fireActiveLayerReplaced() when it was replaced)
 */
RS_LayerList::CurrentLayerLock RS_LayerList::keepCurrentLayerUnlocked(const bool wasLocked) {
    CurrentLayerLock outcome;
    if (m_activeLayer == nullptr || wasLocked || !m_activeLayer->isLocked()) {
        return outcome;
    }
    outcome.m_asked = m_activeLayer;
    if (RS_Layer* replacement = bestLayerToActivate(m_activeLayer, false)) {
        outcome.m_replacement = replacement;
        m_activeLayer = replacement;
    }
    else {
        m_activeLayer->lock(false);
    }
    return outcome;
}

void RS_LayerList::fireActiveLayerReplaced() {
    for (const auto l : std::as_const(m_layerListListeners)) {
        l->activeLayerReplaced(m_activeLayer);
    }
}

/**
 * adds a LayerListListener to the list of listeners. Listeners
 * are notified when the layer list changes.
 *
 * Typical listeners are: layer list widgets, pen toolbar, graphic view
 */
void RS_LayerList::addListener(RS_LayerListListener* listener) {
    // added only once; the listener is told which lists have it, and removes itself when destroyed
    m_layerListListeners.add(listener);
}

/**
 * removes a LayerListListener from the list of listeners.
 */
void RS_LayerList::removeListener(RS_LayerListListener* listener) {
    m_layerListListeners.remove(listener);
}

/**
 * Dumps the layers to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_LayerList& l) {
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
void RS_LayerList::setModified(const bool m) {
    m_modified = m;
}

// notify that list is updated via listerners
void RS_LayerList::slotUpdateLayerList() {
    setModified(true);
}
