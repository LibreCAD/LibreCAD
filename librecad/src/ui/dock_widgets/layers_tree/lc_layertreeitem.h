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

class RS_Layer;
class LC_LayerTreeItem;

/**
 * Utility class for defining policy for accepting specific items.
 * Used for searching/collecting items in the tree
 */
class LC_LayerTreeItemAcceptor{
public:
    virtual ~LC_LayerTreeItemAcceptor()= default;
    virtual bool acceptLayerTreeItem([[maybe_unused]] LC_LayerTreeItem* item) const {
        return true;
    }
};

/**
 * Tree Item used to maintain hierarchical structure of layers.
 * May hold reference to the layer - if no layer there, it's just a "virtual layer" used as layers group
 */
class LC_LayerTreeItem{
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

    LC_LayerTreeItem(const QString &name, RS_Layer *actualLayer , LC_LayerTreeItem *parent = nullptr);
    ~LC_LayerTreeItem();

    void appendChild(LC_LayerTreeItem *child);

    LC_LayerTreeItem *child(int row) const;
    int childCount() const;
    int childCountAll() const;
    int row() const;
    LC_LayerTreeItem *parent() const;
    void sortChildren();
    bool isVirtual() const;
    bool isVisible() const;
    bool isLocked() const;
    bool isPrint() const;
    bool isConstruction() const;
    bool isNotFrozen() const;
    QString getName();
    QString getDisplayName() const {return m_displayName;};
    void setDisplayName(QString &newName){m_displayName = newName;};
    RS_Layer* getLayer() const;
    void updateCalculatedFlagsForDescendentVirtualLayers();
    LC_LayerTreeItem* createLayerChild(const QString &childName, RS_Layer* childLayer);
    LC_LayerTreeItem* getOrCreateVirtualChild(const QString &targetChildName);
    int getIndent() const { return m_indent;};
    void collectLayers(QList<RS_Layer*> &result, LC_LayerTreeItemAcceptor* acceptor, bool acceptSelf=false);
    void collectDescendantChildren(QList<LC_LayerTreeItem*> &result, LC_LayerTreeItemAcceptor* acceptor, bool includeSelf = false);
    void markAsActivePath();
    void rebuildActivePath(RS_Layer* layerToBeActive);
    bool isPartOfActivePath() const;
    int getLayerType() const {return m_layerType;};
    void setLayerType(int value);
    LC_LayerTreeItem* getPrimaryItem() const;
    void setPrimaryItem(LC_LayerTreeItem* item);
    LC_LayerTreeItem* findPrimaryLayerChild(const QString &childNameToFind) const;
    void setMatched(bool enabled) {m_matched = enabled;};
    bool isMatched() const {return m_matched;};
    bool isActiveLayer() const {return m_activeLayer;};
    void setActiveLayer(bool value){m_activeLayer = value;};
    void invalidate();
    bool isInValid() const;
    bool hasChildOfType(int type) const;
    bool hasChildWithName(const QString& nameToFind, int layerTypeToFind) const;
    void collectPathToParent(QList<LC_LayerTreeItem*> &result, bool includeSelf);
    bool isDescendantOf(const LC_LayerTreeItem* item) const;
    bool isZero() const {return m_zero_layer;}
    void markAsZero(){m_zero_layer = true;}
    QString getFullName(){return m_fullName;};
    void setFullName(QString &name){m_fullName = name;};
private:

    LC_LayerTreeItem* findItemWithLayer(RS_Layer* source);
    // children of this item
    QList<LC_LayerTreeItem*> m_childItems;
    // parent item
    LC_LayerTreeItem *m_parentItem = nullptr;
    // reference to primary item - used by secondary (informational, dimensional, alternative pos) layers
    LC_LayerTreeItem *m_primaryItem {nullptr};
    // actual layer, if any
    RS_Layer* m_layer {nullptr};
    // part of raw parsed name
    QString name;
    // name used to display in UI
    QString m_displayName;
    // full path to the item
    QString m_fullName;
    // flat that containing layer is "0"
    bool m_zero_layer {false};
    // several calculated fields used for virtual layers
    bool m_virtualVisible {false};
    bool m_virtualLocked {false};
    bool m_virtualPrint{false};
    bool m_virtualConstruction{false};
    // current indent of the item
    int m_indent {0};
    // flag that this item is part of the path from root to active layer
    bool m_partOfActivePath {false};
    // type of layer
    int m_layerType {NORMAL};
    // flag that indicates that item matched filtering condition
    bool m_matched {false};
    // flag that indicates that this is active layer
    bool m_activeLayer {false};
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
        bool result = item->isNotFrozen();
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
