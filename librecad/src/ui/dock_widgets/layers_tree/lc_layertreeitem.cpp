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

LC_LayerTreeItem::LC_LayerTreeItem(LC_LayerTreeItem *parent):
    parentItem { parent}
{
}

LC_LayerTreeItem::LC_LayerTreeItem(QString &itemName, RS_Layer *actualLayer, LC_LayerTreeItem *parent){
    parentItem = parent;
    layer = actualLayer;
    name = itemName;
    indent = parent->getIndent() + 1;
}

LC_LayerTreeItem::~LC_LayerTreeItem(){
    qDeleteAll(childItems);
}

void LC_LayerTreeItem::appendChild(LC_LayerTreeItem *item){
    childItems.append(item);
}

LC_LayerTreeItem *LC_LayerTreeItem::child(int row){
    return childItems.value(row);
}

int LC_LayerTreeItem::childCount() const{
    return childItems.count();
}

int LC_LayerTreeItem::childCountAll() const{
    int result = childCount();
    if (isVirtual()){
        int count = childItems.length();
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = childItems.at(i);
            result = result + (child->childCountAll());
        }
    }
    return result;
}

LC_LayerTreeItem *LC_LayerTreeItem::parent(){
    return parentItem;
}

int LC_LayerTreeItem::row() const{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<LC_LayerTreeItem *>(this));
    return 0;
}

bool LC_LayerTreeItem::isVirtual() const{
    return this->layer == nullptr;
}
/**
 * Sort children of this item alphabetically, and recursively sort descendants
 */
void LC_LayerTreeItem::sortChildren(){
    int count = childItems.length();

    if (count > 0){
        std::stable_sort(childItems.begin(), childItems.end(), [](LC_LayerTreeItem *l0, LC_LayerTreeItem *l1) -> bool{
            return l0->getName() < l1->getName();
        });

        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = childItems.at(i);
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
        int count = childItems.length();
        virtualConstruction = true;
        virtualLocked = true;
        virtualPrint = true;
        virtualVisible = true;
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = childItems.at(i);
            child->updateCalculatedFlagsForDescendentVirtualLayers();

            virtualConstruction &= child->isConstruction();
            virtualVisible = virtualVisible && child->isVisible();
            virtualPrint &= child->isPrint();
            virtualLocked &= child->isLocked();
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
        if (this->layer){
            if (acceptor->acceptLayerTreeItem(this)){
                result << this->layer;
            }
        }
    }

    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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

    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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
    if (this->layer == source){
        return this;
    }
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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
    LC_LayerTreeItem *parent = parentItem;
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
bool LC_LayerTreeItem::isDescendantOf(LC_LayerTreeItem *item){
    bool result = false;
    LC_LayerTreeItem *parent = parentItem;
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
bool LC_LayerTreeItem::hasChildWithName(QString &nameToFind, int layerTypeToFind){
    bool result = false;
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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
bool LC_LayerTreeItem::hasChildOfType(int type){
    bool result = false;
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
        if (child->getLayerType() == type){
            result = true;
            break;
        }
    }
    return result;
}

void LC_LayerTreeItem::setLayerType(int value){
    layerType = value;
}
/**
 * If layer is secondary, this is the reference to the normal layer (if any) that corresponds to this item
 * @return primary layer item
 */
LC_LayerTreeItem *LC_LayerTreeItem::getPrimaryItem(){
    return primaryItem;
}

void LC_LayerTreeItem::setPrimaryItem(LC_LayerTreeItem *item){
    primaryItem = item;
}
/**
 * Marks all parent items from this item to the root as part of active path
 */
void LC_LayerTreeItem::markAsActivePath(){
    partOfActivePath = true;
    LC_LayerTreeItem *parent = parentItem;
    while (parent != nullptr) {
        parent->partOfActivePath = true;
        parent = parent->parent();
    }
}

bool LC_LayerTreeItem::isPartOfActivePath() const{
    return partOfActivePath;
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
    QList<LC_LayerTreeItem *> items = childItems;
    bool found = true;
    while (found) {
        int count = items.length();
        found = false;
        for (int i = 0; i < count; i++) {
            LC_LayerTreeItem *child = items.at(i);
            if (child->isPartOfActivePath()){
                child->partOfActivePath = false;
                child->setActiveLayer(false);
                items = child->childItems;
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
LC_LayerTreeItem *LC_LayerTreeItem::createLayerChild(QString &childName, RS_Layer *childLayer){
    auto *layerItem = new LC_LayerTreeItem(childName, childLayer, this);
    appendChild(layerItem);
    return layerItem;
}

/**
 * Finds child of this item with given name and normal layer type
 * @param childNameToFind child name
 * @return item or nullptr if not found
 */
LC_LayerTreeItem *LC_LayerTreeItem::findPrimaryLayerChild(QString &childNameToFind){
    LC_LayerTreeItem *result = nullptr;
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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
 * Inspects children and returns child which is virtual had has given name. If no such child present, new child is
 * is created and returned
 * @param targetChildName name of child layer
 * @return virtual child with given name
 */
LC_LayerTreeItem *LC_LayerTreeItem::getOrCreateVirtualChild(QString &targetChildName){
    LC_LayerTreeItem *result = nullptr;
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
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

bool LC_LayerTreeItem::isVisible(){
    if (isVirtual()){
        return virtualVisible;
    } else {
        return !layer->isFrozen();
    }
}

bool LC_LayerTreeItem::isLocked(){
    if (isVirtual()){
        return virtualLocked;
    } else {
        return layer->isLocked();
    }
}

bool LC_LayerTreeItem::isPrint(){
    if (isVirtual()){
        return virtualPrint;
    } else {
        return layer->isPrint();
    }
}

bool LC_LayerTreeItem::isConstruction(){
    if (isVirtual()){
        return virtualConstruction;
    } else {
        return layer->isConstruction();
    }
}

QString LC_LayerTreeItem::getName() {return name;}

RS_Layer *LC_LayerTreeItem::getLayer(){return this->layer;}

/**
 *  utility method that is used to mark item as invalid
 */
void LC_LayerTreeItem::invalidate(){
        layerType = -1;
    int count = childItems.length();
    for (int i = 0; i < count; i++) {
        LC_LayerTreeItem *child = childItems.at(i);
        child->invalidate();
    }
}

bool LC_LayerTreeItem::isInValid() const{
    return layerType < 0;
}

