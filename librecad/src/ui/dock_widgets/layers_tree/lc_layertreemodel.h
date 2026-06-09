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

#include <QAbstractItemModel>
#include <QIcon>
#include <QRegularExpression>

class LC_LayerTreeItem;
class LC_LayerTreeItemAcceptor;
class RS_Layer;
class RS_LayerList;
struct LC_LayerTreeModelOptions;

/**
 * Model used by layers tree
 */
class LC_LayerTreeModel:public QAbstractItemModel {
    Q_OBJECT
public:
    // columns for treeview
    enum Columns {
        COLUMN_EMPTY, COLUMN_VISIBLE, COLUMN_LOCKED, COLUMN_PRINT, COLUMN_CONSTRUCTION, COLUMN_COLOR_SAMPLE, COLUMN_NAME, COLUMN_LAST
    };
    // the default icon size
    static constexpr int ICONWIDTH = 21;
    explicit LC_LayerTreeModel(QObject *parent, LC_LayerTreeModelOptions *options);
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    void setLayerList(const RS_LayerList *ll);
    void proceedActiveLayerChanged(const RS_LayerList *ll) const;
    QList<RS_Layer *> collectLayers(const LC_LayerTreeItemAcceptor &acceptor) const;
    QModelIndexList getPersistentIndexList() const;
    LC_LayerTreeItem *getRoot() const {return m_rootItem;}
    void setFilteringRegexp(const QString &regexp, bool highlightMode);
    Qt::DropActions supportedDropActions() const override{return Qt::CopyAction | Qt::MoveAction;}
    void setCurrentlyDraggingItem(LC_LayerTreeItem *item);
    LC_LayerTreeItem *getCurrentlyDraggingItem() const;
    void setFlatMode(bool mode);
    bool isFlatMode() const {return m_flatMode;}
    bool performReStructure(LC_LayerTreeItem *source, LC_LayerTreeItem *destination);
    bool convertToType(const QModelIndex &index, int toLayerType);
    QHash<RS_Layer *, RS_Layer *> createLayersCopy(const QModelIndex &selectedIndex);
    LC_LayerTreeItem *getItemForIndex(const QModelIndex &index) const;
    LC_LayerTreeItem *getItemForLayer(RS_Layer *layer) const;
    bool renameVirtualLayer(LC_LayerTreeItem *source, QString &newSourceName);
    void renamePrimaryLayer(LC_LayerTreeItem *layerItem, const QString& newName, int newLayerType);
    QString generateLayersDisplayPathString(LC_LayerTreeItem* item) const;
    QStringList getLayersListForRenamedVirtualLayer(LC_LayerTreeItem *source, QString &newName);
    QStringList getLayersListForRenamedPrimary(LC_LayerTreeItem* source, const QString &newSourceName, int newLayerType);
    QString createFullLayerName(LC_LayerTreeItem *treeItem, const QString &layerName, int layerType, bool newLayer);
    int translateColumn(int column) const;
    LC_LayerTreeModelOptions* getOptions() const {return m_options;}
    void reset();
    bool isRegexpApplied() const{return m_hasRegexp;}
private:
    void rebuildModel(const QList<RS_Layer *> &layersList, const RS_Layer *activeLayer);
    bool isValidRestructure(const LC_LayerTreeItem *source, const LC_LayerTreeItem *destination) const;
    bool doConvertToType(LC_LayerTreeItem *layerItem, int toLayerType);
    QHash<RS_Layer *, RS_Layer *> doCreateLayersCopy(LC_LayerTreeItem *source, bool includeChildren, int newLayerType);
    bool renameLayers(const QList<LC_LayerTreeItem *>& layersList, const QString &fromNamePrefix, const QString &toNamePrefix);
    void emitDataChanged();
    QString generateLayersPathString(const QList<LC_LayerTreeItem *>& itemsPathAsList, bool alternateName, const QString &shouldAlternate) const;
    QString restoreNamePart(QString name, int layerType) const;
    void setupDisplayNames(LC_LayerTreeItem *item) const;
    QString findNewUniqueName(const LC_LayerTreeItem *destination, const QString &name, const QString &copyPrefix, const QString &copySuffix, int layerType);
    QString createItemPathString(LC_LayerTreeItem *item, bool includeSelf, bool alternateSourceName, const QString &newItemName);
    void createFirstLayerCopy(QHash<RS_Layer *, RS_Layer *> &result, LC_LayerTreeItem *source, int newLayerType);
    void doCreateChildLayersCopy(QHash<RS_Layer *, RS_Layer *> &result, LC_LayerTreeItem *source, const QString &oldPrefix, const QString &newPrefix);
    QString createFirstCopiedItemNew(LC_LayerTreeItem *source, int newLayerType);
    QString cleanupLayerName(const QString &layerName) const;
    QString doGenerateLayersPathString(const QList<LC_LayerTreeItem *> &itemsPathAsList, bool alternateSourceName, const QString &alternativeName,
                                       const QString &usingLayerLayerSeparator) const;
    QHash<RS_Layer *, QString> doGetVirtualLayerRenameLayersMap(LC_LayerTreeItem *source, QString &newSourceName);
    QHash<RS_Layer *, QString> prepareLayerRename(const QList<LC_LayerTreeItem *> &layersList, const QString &fromNamePrefix, const QString &toNamePrefix);
    bool renameLayersMap(const QHash<RS_Layer *, QString> &layersMap) const;
    QHash<RS_Layer *, QString> doGetPrimaryLayerRenameLayersMap(LC_LayerTreeItem *source, const QString &newSourceName, int newLayerType);

    QIcon m_iconLayerVisible;
    QIcon m_iconLayerHidden;
    QIcon m_iconLayerDefreeze;
    QIcon m_iconLayerFreeze;
    QIcon m_iconLayerPrint;
    QIcon m_iconLayerNoPrint;
    QIcon m_iconLayerConstruction;
    QIcon m_iconLayerNoConstruction;
    QIcon m_iconLayerVirtual;
    QIcon m_iconLayerDimensional;
    QIcon m_iconLayerAlternatePosition;
    QIcon m_iconLayerInformationalNotes;
    QIcon m_iconLayerActual;

    int m_maxIndent{0};

    // filtering/highlight regexp value
    QRegularExpression m_filteringRegexp{""};

    // flat that controls whether regexp should be applied
    bool m_hasRegexp{false};

    //  controls whether items matched to regexp are just highlighted or filtered
    bool m_regexpHighlightMode{true};

    // current item during drag&drop operation
    LC_LayerTreeItem *m_currentlyDraggingItem{nullptr};

    // root item for layers hierarchy
    LC_LayerTreeItem *m_rootItem = nullptr;

    bool m_flatMode{false};

    LC_LayerTreeModelOptions* m_options = nullptr;
    void copyChildrenLayers(LC_LayerTreeItem *parent, int newParentLayerType, QHash<RS_Layer *, RS_Layer *> &result);
};

#endif
