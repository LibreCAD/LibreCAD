/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_LAYERTREEITEM_H
#define LC_LAYERTREEITEM_H

#include <QVariant>
#include "rs_layer.h"


class LC_LayerTreeItem;

/**
 * Utility class for defining policy for accepting specific items.
 * Used for searching/collecting items in the tree
 */
class LC_LayerTreeItemAcceptor{
public:
    virtual ~LC_LayerTreeItemAcceptor()= default;
    virtual bool acceptLayerTreeItem([[maybe_unused]] LC_LayerTreeItem* item) const
    {
        return true;
    }
};

/**
 * Tree Item used to maintain hierarchical structure of layers.
 * May hold reference to the layer - if no layer there, it's just a "virtual layer" used as layers group
 */
class LC_LayerTreeItem
{
public:

    static constexpr int NOT_DEFINED_LAYER_TYPE = -1;
    // Layer types
    enum {
        VIRTUAL,
        NORMAL,
        DIMENSIONAL,
        INFORMATIONAL,
        ALTERNATE_POSITION
    };


    explicit LC_LayerTreeItem(LC_LayerTreeItem *parent = nullptr);

    LC_LayerTreeItem(QString &name, RS_Layer *actualLayer , LC_LayerTreeItem *parent = nullptr);
    ~LC_LayerTreeItem();

    void appendChild(LC_LayerTreeItem *child);

    LC_LayerTreeItem *child(int row);
    int childCount() const;
    int childCountAll() const;
    int row() const;
    LC_LayerTreeItem *parent();
    void sortChildren();
    bool isVirtual() const;
    bool isVisible();
    bool isLocked();
    bool isPrint();
    bool isConstruction();
    QString getName();
    QString getDisplayName() const {return displayName;};
    void setDisplayName(QString &newName){displayName = newName;};
    RS_Layer* getLayer();
    void updateCalculatedFlagsForDescendentVirtualLayers();
    LC_LayerTreeItem* createLayerChild(QString &childName, RS_Layer* childLayer);
    LC_LayerTreeItem* getOrCreateVirtualChild(QString &targetChildName);
    int getIndent() const { return indent;};
    void collectLayers(QList<RS_Layer*> &result, LC_LayerTreeItemAcceptor* acceptor, bool acceptSelf=false);
    void collectDescendantChildren(QList<LC_LayerTreeItem*> &result, LC_LayerTreeItemAcceptor* acceptor, bool includeSelf = false);
    void markAsActivePath();
    void rebuildActivePath(RS_Layer* layerToBeActive);
    bool isPartOfActivePath() const;
    int getLayerType() const {return layerType;};
    void setLayerType(int value);
    LC_LayerTreeItem* getPrimaryItem();
    void setPrimaryItem(LC_LayerTreeItem* item);
    LC_LayerTreeItem* findPrimaryLayerChild(QString &childNameToFind);
    void setMatched(bool enabled) {matched = enabled;};
    bool isMatched() const {return matched;};
    bool isActiveLayer() const {return activeLayer;};
    void setActiveLayer(bool value){activeLayer = value;};
    void invalidate();
    bool isInValid() const;
    bool hasChildOfType(int type);
    bool hasChildWithName(QString& nameToFind, int layerTypeToFind);
    void collectPathToParent(QList<LC_LayerTreeItem*> &result, bool includeSelf);
    bool isDescendantOf(LC_LayerTreeItem* item);
    bool isZero() const {return zero_layer;}
    void markAsZero(){zero_layer = true;}
    QString getFullName(){return fullName;};
    void setFullName(QString &name){fullName = name;};
private:

    LC_LayerTreeItem* findItemWithLayer(RS_Layer* source);
    // children of this item
    QList<LC_LayerTreeItem*> childItems;
    // parent item
    LC_LayerTreeItem *parentItem = nullptr;
    // reference to primary item - used by secondary (informational, dimensional, alternative pos) layers
    LC_LayerTreeItem *primaryItem {nullptr};
    // actual layer, if any
    RS_Layer* layer {nullptr};
    // part of raw parsed name
    QString name;
    // name used to display in UI
    QString displayName;
    // full path to the item
    QString fullName;
    // flat that containing layer is "0"
    bool zero_layer {false};
    // several calculated fields used for virtual layers
    bool virtualVisible {false};
    bool virtualLocked {false};
    bool virtualPrint{false};
    bool virtualConstruction{false};
    // current indent of the item
    int indent {0};
    // flag that this item is part of the path from root to active layer
    bool partOfActivePath {false};
    // type of layer
    int layerType {NORMAL};
    // flag that indicates that item matched filtering condition
    bool matched {false};
    // flag that indicates that this is active layer
    bool activeLayer {false};
};


class QG_LayerTreeItemAcceptorByType: public virtual LC_LayerTreeItemAcceptor{
public:
    explicit QG_LayerTreeItemAcceptorByType(int ltype){
        layerType = ltype;
    }

    bool acceptLayerTreeItem(LC_LayerTreeItem* item) const override{
        int ltype = item -> getLayerType();
        bool result = ltype == layerType;
        return result;
    }
private:
    int layerType = 0;
};

class QG_LayerTreeItemAcceptorVisible: public virtual LC_LayerTreeItemAcceptor{
public:
    bool acceptLayerTreeItem(LC_LayerTreeItem* item) const override{
        bool result = !item->getLayer()->isFrozen();
        return result;
    }
};

class QG_LayerTreeItemAcceptorSameLayerAs: public virtual LC_LayerTreeItemAcceptor{
public:

    explicit QG_LayerTreeItemAcceptorSameLayerAs(RS_Layer* l){
        layer = l;
    }

    explicit QG_LayerTreeItemAcceptorSameLayerAs(RS_Layer* l, bool notEqualsMode){
        layer = l;
        notOperation = notEqualsMode;
    }

    bool acceptLayerTreeItem(LC_LayerTreeItem* item) const override{
        bool result = item->getLayer() == layer;
        if (notOperation){
            result = !result;
        }
        return result;
    }
private:
    RS_Layer* layer = nullptr;
    bool notOperation {false};
};

class QG_LayerTreeItemAcceptorSecondary: public virtual LC_LayerTreeItemAcceptor{
public:
    QG_LayerTreeItemAcceptorSecondary() = default;
    explicit QG_LayerTreeItemAcceptorSecondary(bool includeItems){
        includeMode = includeItems;
    }

    bool acceptLayerTreeItem(LC_LayerTreeItem* item) const override{
        int ltype = item -> getLayerType();
        bool result = true;

        if (includeMode){
            result = (ltype == LC_LayerTreeItem::DIMENSIONAL) || (ltype == LC_LayerTreeItem::ALTERNATE_POSITION) || (ltype == LC_LayerTreeItem::INFORMATIONAL);
        }
        else{
            if (item -> getPrimaryItem() != nullptr){
                result = (ltype != LC_LayerTreeItem::DIMENSIONAL) && (ltype != LC_LayerTreeItem::ALTERNATE_POSITION) && (ltype != LC_LayerTreeItem::INFORMATIONAL);
            }
        }
        return result;
    }
private:
    bool includeMode {true};
};

#endif // QG_LAYERTREEITEM_H
