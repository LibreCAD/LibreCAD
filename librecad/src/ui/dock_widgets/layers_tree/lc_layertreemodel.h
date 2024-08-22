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

#ifndef LC_LAYERTREEMODEL_H
#define LC_LAYERTREEMODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QItemSelection>
#include <QRegularExpression>
#include <QWidget>

#include "lc_layertreeitem.h"
#include "lc_layertreemodel_options.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

/**
 * Model used by layers tree
 */
class LC_LayerTreeModel:public QAbstractItemModel {
    Q_OBJECT

public:
    // columns for treeview
    enum {
        EMPTY, VISIBLE, LOCKED, PRINT, CONSTRUCTION, COLOR_SAMPLE, NAME, LAST
    };
    // the default icon size
    static constexpr int ICONWIDTH = 24;
    explicit LC_LayerTreeModel(QObject *parent, LC_LayerTreeModelOptions *options);
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    void setLayerList(RS_LayerList *ll);
    void proceedActiveLayerChanged(RS_LayerList *ll);
    QList<RS_Layer *> collectLayers(LC_LayerTreeItemAcceptor *acceptor);
    QModelIndexList getPersistentIndexList();
    LC_LayerTreeItem *getRoot(){return rootItem;};
    void setFilteringRegexp(QString &reqgexp, bool highlightMode);
    Qt::DropActions supportedDropActions() const override{return Qt::CopyAction | Qt::MoveAction;}
    void setCurrentlyDraggingItem(LC_LayerTreeItem *item);
    LC_LayerTreeItem *getCurrentlyDraggingItem();
    void setFlatMode(bool mode);
    bool performReStructure(LC_LayerTreeItem *source, LC_LayerTreeItem *destination);
    bool convertToType(QModelIndex &index, int toLayerType);
    QHash<RS_Layer *, RS_Layer *> createLayersCopy(QModelIndex &selectedIndex);
    LC_LayerTreeItem *getItemForIndex(const QModelIndex &index) const;
    LC_LayerTreeItem *getItemForLayer(RS_Layer *layer) const;
    bool renameVirtualLayer(LC_LayerTreeItem *pItem, QString &qString);
    void renamePrimaryLayer(LC_LayerTreeItem *layerItem, QString newName, int newLayerType);
    QString generateLayersDisplayPathString(LC_LayerTreeItem* item);
    QStringList getLayersListForRenamedVirtualLayer(LC_LayerTreeItem *virtualLayerItem, QString &newName);
    QStringList getLayersListForRenamedPrimary(LC_LayerTreeItem* source, QString &newSourceName, int newLayerType);
    QString createFullLayerName(LC_LayerTreeItem *treeItem, QString &layerName, int layerType, bool newLayer);
    int translateColumn(int column) const;
    LC_LayerTreeModelOptions* getOptions() {return options;};
    void reset();
    bool isRegexpApplied() const{return hasRegexp;};
private:
    void rebuildModel(QList<RS_Layer *> &layersList, RS_Layer *activeLayer);
    bool isValidRestructure(LC_LayerTreeItem *source, LC_LayerTreeItem *destination) const;
    bool doConvertToType(LC_LayerTreeItem *layerItem, int toLayerType);
    QHash<RS_Layer *, RS_Layer *> doCreateLayersCopy(LC_LayerTreeItem *source, bool includeChildren, int newLayerType);
    bool renameLayers(QList<LC_LayerTreeItem *> layersList, QString &fromNamePrefix, QString &toNamePrefix);
    void emitDataChanged();
    QString generateLayersPathString(QList<LC_LayerTreeItem *> items, bool alternateName, QString &alternativeName);
    QString restoreNamePart(QString name, int layerType);
    void setupDisplayNames(LC_LayerTreeItem *item);
    QString findNewUniqueName(LC_LayerTreeItem *destination, QString &name, QString &copyPrefix, QString &copySuffix, int layerType);
    QString createItemPathString(LC_LayerTreeItem *item, bool includeSelf, bool alternateItemName, QString &newItemName);
    void createFirstLayerCopy(QHash<RS_Layer *, RS_Layer *> &result, LC_LayerTreeItem *source, int newLayerType);
    void doCreateChildLayersCopy(QHash<RS_Layer *, RS_Layer *> &result, LC_LayerTreeItem *source, QString &oldPrefix, QString &newPrefix);
    QString createFirstCopiedItemNew(LC_LayerTreeItem *source, int newLayerType);
    QString cleanupLayerName(QString &layerName) const;
    QString doGenerateLayersPathString(QList<LC_LayerTreeItem *> &itemsPathAsList, bool alternateSourceName, QString &alternativeName,
                                       QString &usingLayerLayerSeparator);

    QHash<RS_Layer *, QString> doGetVirtualLayerRenameLayersMap(LC_LayerTreeItem *source, QString &newSourceName);
    QHash<RS_Layer *, QString> prepareLayerRename(QList<LC_LayerTreeItem *> &layersList, QString &fromNamePrefix, QString &toNamePrefix);
    bool renameLayersMap(const QHash<RS_Layer *, QString> &layersMap) const;
    QHash<RS_Layer *, QString> doGetPrimaryLayerRenameLayersMap(LC_LayerTreeItem *source, QString &newSourceName, int newLayerType);

    QIcon layerVisible;
    QIcon layerHidden;
    QIcon layerDefreeze;
    QIcon layerFreeze;
    QIcon layerPrint;
    QIcon layerNoPrint;
    QIcon layerConstruction;
    QIcon layerNoConstruction;
    QIcon iconLayerVirtual;
    QIcon iconLayerDimensional;
    QIcon iconLayerAlternatePosition;
    QIcon iconLayerInformationalNotes;
    QIcon iconLayerActual;

    int maxIndent{0};

    // filtering/highlight regexp value
    QRegularExpression filteringRegexp{""};

    // flat that controls whether regexp should be applied
    bool hasRegexp{false};

    //  controls whether items matched to regexp are just highlighted or filtered
    bool regexpHighlightMode{true};

    // current item during drag&drop operation
    LC_LayerTreeItem *currentlyDraggingItem{nullptr};

    // root item for layers hierarchy
    LC_LayerTreeItem *rootItem = nullptr;

    bool flatMode{false};

    LC_LayerTreeModelOptions* options = nullptr;
    void copyChildrenLayers(LC_LayerTreeItem *parent, int newParentLayerType, QHash<RS_Layer *, RS_Layer *> &result);
};

#endif // QG_LAYERTREEMODEL_H
