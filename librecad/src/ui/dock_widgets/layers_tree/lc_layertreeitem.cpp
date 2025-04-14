/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
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

#include "lc_layertreeitem.h"
#include "rs_layer.h"

LC_LayerTreeItem::LC_LayerTreeItem(LC_LayerTreeItem *parent):
    m_parentItem { parent}{
}

LC_LayerTreeItem::LC_LayerTreeItem(const QString &itemName, RS_Layer *actualLayer, LC_LayerTreeItem *parent){
    m_parentItem = parent;
    m_layer = actualLayer;
    name = itemName;
    m_indent = parent->getIndent() + 1;
}

LC_LayerTreeItem::~LC_LayerTreeItem(){
    qDeleteAll(m_childItems);
}

void LC_LayerTreeItem::appendChild(LC_LayerTreeItem *item){
    m_childItems.append(item);
}

LC_LayerTreeItem *LC_LayerTreeItem::child(int row) const {
    return m_childItems.value(row);
}

int LC_LayerTreeItem::childCount() const{
    return m_childItems.count();
}

int LC_LayerTreeItem::childCountAll() const{
    int result = childCount();
    if (isVirtual()){
        const int count = m_childItems.length();
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = m_childItems.at(i);
            result = result + (child->childCountAll());
        }
    }
    return result;
}

LC_LayerTreeItem *LC_LayerTreeItem::parent() const {
    return m_parentItem;
}

int LC_LayerTreeItem::row() const{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<LC_LayerTreeItem *>(this));
    return 0;
}

bool LC_LayerTreeItem::isVirtual() const{
    return this->m_layer == nullptr;
}
/**
 * Sort children of this item alphabetically, and recursively sort descendants
 */
void LC_LayerTreeItem::sortChildren(){
    int count = m_childItems.length();

    if (count > 0){
        std::stable_sort(m_childItems.begin(), m_childItems.end(), [](LC_LayerTreeItem *l0, LC_LayerTreeItem *l1) -> bool{
            return l0->getName() < l1->getName();
        });

        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = m_childItems.at(i);
            child->sortChildren();
        }
    }
}
/**
 * Recursively calculates virtual flags for virtual layers, based on flags for their children.
 * First ask children to calculate their flags, and based on them calculates own values.
 * AND boolean operation is used, so it means that flag for virtual layer is true if
 * value of this flag is true for all children of the item.
 */
void LC_LayerTreeItem::updateCalculatedFlagsForDescendentVirtualLayers(){
    if (isVirtual()){
        int count = m_childItems.length();
        m_virtualConstruction = true;
        m_virtualLocked = true;
        m_virtualPrint = true;
        m_virtualVisible = true;
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = m_childItems.at(i);
            child->updateCalculatedFlagsForDescendentVirtualLayers();

            m_virtualConstruction &= child->isConstruction();
            m_virtualVisible = m_virtualVisible && child->isVisible();
            m_virtualPrint &= child->isPrint();
            m_virtualLocked &= child->isLocked();
        }
    }
}
/**
 * Collects list of layers from descendent children (recursively) according to provided condition
 * @param result resulting list
 * @param acceptor condition for acceptance
 * @param includeSelf defines whether this item should be also checked for the condition, if false - only child items will be processed
 */
void LC_LayerTreeItem::collectLayers(QList<RS_Layer *> &result, LC_LayerTreeItemAcceptor *acceptor, bool includeSelf){
    if (includeSelf){
        if (m_layer){
            if (acceptor->acceptLayerTreeItem(this)){
                result << m_layer;
            }
        }
    }

    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        if (child->isVirtual()){
            child->collectLayers(result, acceptor);
        } else {
            if (acceptor->acceptLayerTreeItem(child)){
                result << child->getLayer();
                child->collectLayers(result, acceptor);
            }
        }
    }
}

/**
 * Collects list of descendent children (recursively) for given conditions
 * @param result result of children
 * @param acceptor condition to check
 * @param includeSelf flag whether this item should be included or not.
 */
void LC_LayerTreeItem::collectDescendantChildren(QList<LC_LayerTreeItem *> &result, LC_LayerTreeItemAcceptor *acceptor, bool includeSelf){
    if (includeSelf){
        result << this;
    }

    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        if (acceptor->acceptLayerTreeItem(child)){
            result << child;
        }
        if (child->childCount() > 0){
            child->collectDescendantChildren(result, acceptor);
        }
    }
}

/**
 * Finds LayerTreeItem that holds provided layer
 * @param source layer
 * @return item with given layer
 */
LC_LayerTreeItem *LC_LayerTreeItem::findItemWithLayer(RS_Layer *source){
    if (m_layer == source){
        return this;
    }
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        LC_LayerTreeItem *res = child->findItemWithLayer(source);
        if (res != nullptr){
            return res;
        }
    }
    return nullptr;
}

/**
 * Collects items that forms path given item to the root (root is the last element of the list)
 * @param result resulting path
 * @param includeSelf should this item be part of the path or not
 */
void LC_LayerTreeItem::collectPathToParent(QList<LC_LayerTreeItem *> &result, bool includeSelf){
    if (includeSelf){
        result << this;
    }
    LC_LayerTreeItem *parent = m_parentItem;
    while (parent != nullptr) {
        result << parent;
        parent = parent->parent();
    }
}

/**
 * Checks whether this item is descendant of given one
 * @param item potential parent
 * @return true if given item is part of the path from root to this item
 */
bool LC_LayerTreeItem::isDescendantOf(const LC_LayerTreeItem *item) const {
    bool result = false;
    LC_LayerTreeItem *parent = m_parentItem;
    while (parent != nullptr) {
        if (parent == item){
            result = true;
            break;
        }
        parent = parent->parent();
    }
    return result;
}

/**
 * Checks whether one of direct children has given name and layer type
 * @param nameToFind name to check
 * @param layerTypeToFind layer type to check
 * @return true if child is present, false otherwise
 */
bool LC_LayerTreeItem::hasChildWithName(const QString &nameToFind, int layerTypeToFind) const {
    bool result = false;
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        if (layerTypeToFind == child->getLayerType()){
            if (child->getName() == nameToFind){
                result = true;
                break;
            }
        }
    }
    return result;
}

/**
 * Check whether one direct children has given layer type
 * @param type layer type
 * @return  true if direct child with given type exist
 */
bool LC_LayerTreeItem::hasChildOfType(int type) const {
    bool result = false;
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        if (child->getLayerType() == type){
            result = true;
            break;
        }
    }
    return result;
}

void LC_LayerTreeItem::setLayerType(int value){
    m_layerType = value;
}

/**
 * If layer is secondary, this is the reference to the normal layer (if any) that corresponds to this item
 * @return primary layer item
 */
LC_LayerTreeItem *LC_LayerTreeItem::getPrimaryItem() const {
    return m_primaryItem;
}

void LC_LayerTreeItem::setPrimaryItem(LC_LayerTreeItem *item){
    m_primaryItem = item;
}

/**
 * Marks all parent items from this item to the root as part of active path
 */
void LC_LayerTreeItem::markAsActivePath(){
    m_partOfActivePath = true;
    LC_LayerTreeItem *parent = m_parentItem;
    while (parent != nullptr) {
        parent->m_partOfActivePath = true;
        parent = parent->parent();
    }
}

bool LC_LayerTreeItem::isPartOfActivePath() const{
    return m_partOfActivePath;
}

/**
 * Modifies path to active layer. First, clear existing active path, than find the item that holds provided layer
 * and marks all parents of that item as part of the active path
  *
  * @param layerToBeActive layer that should be active
  */
void LC_LayerTreeItem::rebuildActivePath(RS_Layer *layerToBeActive){

    // just too lazy to use recursion there for one time cleanup

    // removing previous active path
    // here we simply go down by current active path, if any,
    // by search in the deep
    QList<LC_LayerTreeItem *> items = m_childItems;
    bool found = true;
    while (found) {
        int count = items.length();
        found = false;
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = items.at(i);
            if (child->isPartOfActivePath()){
                child->m_partOfActivePath = false;
                child->setActiveLayer(false);
                items = child->m_childItems;
                found = true;
                break;
            }
        }
    }

    auto *itemForActiveLayer = findItemWithLayer(layerToBeActive);
    if (itemForActiveLayer != nullptr){ // we got one, mark the path
        itemForActiveLayer->setActiveLayer(true);
        itemForActiveLayer->markAsActivePath();
    }
}

/**
 * Creates child item with given name and layer
 * @param childName name of child item
 * @param childLayer layer that will be hold in child
 * @return created item
 */
LC_LayerTreeItem *LC_LayerTreeItem::createLayerChild(const QString &childName, RS_Layer *childLayer){
    auto *layerItem = new LC_LayerTreeItem(childName, childLayer, this);
    appendChild(layerItem);
    return layerItem;
}

/**
 * Finds child of this item with given name and normal layer type
 * @param childNameToFind child name
 * @return item or nullptr if not found
 */
LC_LayerTreeItem *LC_LayerTreeItem::findPrimaryLayerChild(const QString &childNameToFind) const {
    LC_LayerTreeItem *result = nullptr;
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        int type = child->getLayerType();
        if (type == NORMAL){ // virtual is supported there to handle not proper layer names convention, actually
            QString childName = child->getName();
            if (childNameToFind == childName){
                result = child;
                break;
            }
        }
    }
    return result;
}

/**
 * Inspects children and returns child which is virtual had has given name. If no such child present, new child
 * is created and returned
 * @param targetChildName name of child layer
 * @return virtual child with given name
 */
LC_LayerTreeItem *LC_LayerTreeItem::getOrCreateVirtualChild(const QString &targetChildName){
    LC_LayerTreeItem *result = nullptr;
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        QString childName = child->getName();
        if (targetChildName == childName){
            result = child;
            break;
        }
    }
    if (result == nullptr){
        result = new LC_LayerTreeItem(targetChildName, nullptr, this);
        result->setLayerType(VIRTUAL);
        appendChild(result);
    }
    return result;
}

bool LC_LayerTreeItem::isVisible() const {
    if (isVirtual()){
        return m_virtualVisible;
    } else {
        return !m_layer->isFrozen();
    }
}

bool LC_LayerTreeItem::isLocked() const {
    if (isVirtual()){
        return m_virtualLocked;
    } else {
        return m_layer->isLocked();
    }
}

bool LC_LayerTreeItem::isPrint() const {
    if (isVirtual()){
        return m_virtualPrint;
    } else {
        return m_layer->isPrint();
    }
}

bool LC_LayerTreeItem::isConstruction() const {
    if (isVirtual()){
        return m_virtualConstruction;
    } else {
        return m_layer->isConstruction();
    }
}

bool LC_LayerTreeItem::isNotFrozen() const {
    return !m_layer->isFrozen();
}

QString LC_LayerTreeItem::getName() {return name;}

RS_Layer *LC_LayerTreeItem::getLayer() const {return this->m_layer;}

/**
 *  utility method that is used to mark item as invalid
 */
void LC_LayerTreeItem::invalidate(){
    m_layerType = -1;
    int count = m_childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = m_childItems.at(i);
        child->invalidate();
    }
}

bool LC_LayerTreeItem::isInValid() const{
    return m_layerType < 0;
}
