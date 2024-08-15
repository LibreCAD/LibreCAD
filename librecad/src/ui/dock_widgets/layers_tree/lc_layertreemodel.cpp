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

#include <QColor>

#include "lc_layertreemodel.h"
#include "lc_layertreemodel_options.h"

LC_LayerTreeModel::LC_LayerTreeModel(QObject * parent, LC_LayerTreeModelOptions *ops) :QAbstractItemModel(parent) {
    layerVisible = QIcon(":/icons/visible.svg");
    layerHidden = QIcon(":/icons/invisible.svg");
    layerDefreeze = QIcon(":/icons/unlocked.svg");
    layerFreeze = QIcon(":/icons/locked.svg");
    layerPrint = QIcon(":/icons/print.svg");
    layerNoPrint = QIcon(":/icons/noprint.svg");
    layerConstruction = QIcon(":/icons/construction_layer.svg");
    layerNoConstruction = QIcon(":/icons/noconstruction.svg");

    iconLayerVirtual = QIcon(":/icons/copy.svg");
    iconLayerDimensional = QIcon(":/icons/dim_horizontal.svg");
    iconLayerActual = QIcon(":/icons/line_rectangle.svg");
    iconLayerAlternatePosition = QIcon(":/icons/rotate.svg");
    iconLayerInformationalNotes = QIcon(":/icons/mtext.svg");

    rootItem = new LC_LayerTreeItem();

    options = ops;
}
/**
 * Returns amount of child for item defined by given index.
 * if index is invalid, returns number of top-level items
 * @param parent
 * @return
 */
int LC_LayerTreeModel::rowCount ( const QModelIndex & parent ) const {

    LC_LayerTreeItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItemForIndex(parent);

    int result = parentItem->childCount();
    return result;
}
/**
 * Count of columns in tree view. Depends on the setting - whether icons for types should be shown or not
 * @param parent
 * @return
 */
int LC_LayerTreeModel::columnCount([[maybe_unused]] const QModelIndex &parent) const{
    int result = LAST;
    if (options->hideLayerTypeIcons){
        result = LAST-1;
    }
    return result;
}

/**
 * Returns parent index for given item
 * @param index
 * @return
 */
QModelIndex LC_LayerTreeModel::parent ( const QModelIndex & index ) const {
    if (index.isValid()){
        LC_LayerTreeItem *childItem = getItemForIndex(index);
        LC_LayerTreeItem *parentItem = childItem->parent();

        if (parentItem != rootItem){
            return createIndex(parentItem->row(), 0, parentItem);
        }
    }
    return QModelIndex();
}
/**
 * Returns model index for given values
 * @param row
 * @param column
 * @param parent
 * @return
 */
QModelIndex LC_LayerTreeModel::index( int row, int column, const QModelIndex & parent ) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    LC_LayerTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItemForIndex(parent);

    if (parentItem->isInValid()){
        return QModelIndex();
    }

    LC_LayerTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

/**
 * Major method that fully rebuilds the model based on provided list of layers.
 * First, throws away old root, than sorts the list of layers alphabetically,
 * restores layers hierarchy
 * @brief LC_LayerTreeModel::setLayerList
 * @param ll
 */
void LC_LayerTreeModel::setLayerList(RS_LayerList* ll) {
    /* since 4.6 the recommended way is to use begin/endResetModel()
     * TNick <nicu.tofan@gmail.com>
     */
    beginResetModel();

    rootItem -> invalidate();
    delete rootItem;

    rootItem = new LC_LayerTreeItem();

    if (ll == nullptr) {
        endResetModel();
        return;
    }

    // prepare the list that we'll use for restoring hierarchy
    QList<RS_Layer*> listLayer;
    for (unsigned i=0; i < ll->count(); ++i) {
        listLayer.append(ll->at(i));
    }

    // sort layers alphabetically
    std::sort( listLayer.begin(), listLayer.end(), [](const RS_Layer *s1, const RS_Layer *s2)-> bool{
        return s1->getName() < s2->getName();
    } );

    RS_Layer* activeLayer = ll->getActive();

    // restore hierarchy
    rebuildModel(listLayer, activeLayer);

    // compute flags for virtual items based on flags on children
    rootItem->updateCalculatedFlagsForDescendentVirtualLayers();

    // ensure that items on each layer are sorted properly
    rootItem -> sortChildren();

    //called to force redraw
    endResetModel();

    emitDataChanged();
}
/**
 * utility method to force reset view indexes - avoiding of "collapseSecondary" flickering
 * @brief LC_LayerTreeModel::reset
 */
void LC_LayerTreeModel::reset(){
     beginResetModel();
     //called to force redraw
     endResetModel();
     emitDataChanged();
}

/**
 * Internal method for building layers tree hierarchy from flat list of layers.
 * For restoring hierarchy, it is expected that layer levels are stored directly
 * within names of layers and some common separator string is used between layer levels.
 *
 * It is expected that the incoming list is properly sorted already.
 *
 * In addition to building the hierarchy, filtering/highlight marking according to
 * filtering regexp is performed, and if there is active layer, the path to it
 * from the root is marked.
 *
 *
 * @brief LC_LayerTreeModel::rebuildModel
 * @param listLayer
 * @param activeLayer
 */

void LC_LayerTreeModel::rebuildModel(QList<RS_Layer*> &listLayer, RS_Layer* activeLayer){

    int alternatePositionLayerNameSuffixLen = options->alternatePositionLayerNameSuffix.length();
    int informationLayerNameSuffixLen = options->informationalLayerNameSuffix.length();
    int dimensionalLayerNameSuffixLen = options->dimensionalLayerNameSuffix.length();

    int number = 0;

    int layersCount = listLayer.count();

    // simply walk by all items in the given list

    while (number < layersCount) {

        RS_Layer * layer = listLayer.at(number);

        QString layerFullName = layer->getName();
        QString layerName = layerFullName;

        // check how layer name is matched to filtering regexp

        bool hasRegexpMatch = false;
        if (hasRegexp){
            int pos = 0;
            hasRegexpMatch =filteringRegexp.match(layerName, pos).hasMatch();

            if (regexpHighlightMode){
                // we'll highlight it later based on the flag
            }
            else{ // in filtering mode, skip if no match
                if (hasRegexpMatch){
                    // don't need it anymore for highlight mode, so we'll clear it
                    hasRegexpMatch = false;
                }
                else{
                    number++;
                    continue; // skip this layer at all as it was not matched
                }
            }
        }
        // split by parts using defined separator of layer levels
        QStringList nameParts = layerFullName.split(options->layerLevelSeparator, Qt::KeepEmptyParts);
        int partsCount = nameParts.count();

        LC_LayerTreeItem* virtualRoot = rootItem;

        if (flatMode){
            // do nothing here, no need to restore tree structure
        }
        else{

            // it's expected that actual layer should be on the last position/
            // Potentially, there might be a clash due to incorrect naming
            // so upper layer in the chain will be not virtual
            // however, even such situation is handled correctly by the view...
            // Also, due to sort order of the incoming list, non-virtual parent layer will be inserted earlier
            for (int indent = 0; indent < partsCount-1; ++indent){
                QString subName = nameParts.at(indent);
                virtualRoot = virtualRoot -> getOrCreateVirtualChild(subName);
                setupDisplayNames(virtualRoot);
                // TODO - is it a place for settings-controlled policy - whether highlights root or not...???
                virtualRoot->setMatched(hasRegexpMatch);
            }
            if (partsCount > 0){
                layerName = nameParts.at(partsCount-1);
            }
        }

        QString primaryLayerName = "";

        // check whether it's helper layer (dimensions, info, alternate position) so we try to find primary layer for this (without _pos)

        int type = LC_LayerTreeItem::NORMAL;

        // determining level item type, name and position based on naming.
        // TODO - actually, a more flexible and generic policy may be used there, and so it will allow to
        // support additional types of layers... however, not sure that it is necessary at the moment

        if (layerName.endsWith(options->alternatePositionLayerNameSuffix)){
            type = LC_LayerTreeItem::ALTERNATE_POSITION;
            QString mainName =  layerName.left(layerName.length() - alternatePositionLayerNameSuffixLen);
            primaryLayerName = mainName;
            if (!flatMode){
              layerName = mainName;
            }
        }
        else if (layerName.endsWith(options->informationalLayerNameSuffix)){
            type = LC_LayerTreeItem::INFORMATIONAL;
            QString mainName = layerName.left(layerName.length() - informationLayerNameSuffixLen);
            primaryLayerName = mainName;
            if (!flatMode){
               layerName = mainName;
            }
        }
        else if (layerName.endsWith(options->dimensionalLayerNameSuffix)){
            type = LC_LayerTreeItem::DIMENSIONAL;
            QString mainName = layerName.left(layerName.length() - dimensionalLayerNameSuffixLen);
            primaryLayerName = mainName;
            if (!flatMode){
                layerName = mainName;
            }
        }

        LC_LayerTreeItem* primaryLayerItem = nullptr;
        if (primaryLayerName.length() > 0){
            // here we assume that primary layer is one of previous children of the current parent.
            // this is reasonable, since we rely on suffix, and primary layer should be already
            // added to the tree due to initial sorting of layers in the incoming layers list
            primaryLayerItem = virtualRoot->findPrimaryLayerChild(primaryLayerName);
        }

        if (primaryLayerItem != nullptr){
            if (!flatMode){
                // this item will be inserted as child of primary level
                virtualRoot = primaryLayerItem;
            }
        }

        LC_LayerTreeItem* layerItem = virtualRoot->createLayerChild(layerName, layer);
        layerItem->setLayerType(type);

        if ("0" == layerFullName){
            // just store zero layer check as flag there in order to reduce further comparisons
            layerItem->markAsZero();
        }

        // yes, primary layer will be both parent (for normal mode) and primary item.
        // however, that's fine since it might be that primary level for specific type
        // does not exist at all, yet parent will be always set (at least, root one)
        layerItem->setPrimaryItem(primaryLayerItem);


        int indent = layerItem->getIndent();
        if (indent > maxIndent){
            maxIndent = indent;
        }

        // prepare display names (for tree item and hint) of layer

        setupDisplayNames(layerItem);

        // if we're here and flag is matched, we are in highlight mode, always false for filtering mode
        layerItem->setMatched(hasRegexpMatch);

        // go up and mark all parent items up to the root as active path items
        if (layer == activeLayer){
            layerItem->setActiveLayer(true);
            layerItem->markAsActivePath();
        }
        number++;
    }
}

/**
 * Method calculates display name (that will be shown in UI) for given item as well as full path to the item
 * @param item layer tree item
 */
void LC_LayerTreeModel::setupDisplayNames(LC_LayerTreeItem* item){
    QString layerName = item->getName();

    QString displayName = layerName.trimmed();
    // based on settings, display name may be indented or not
    if (options->showIndentedName){
        int ident = item->getIndent() - 1;
        if (ident > 0){
           displayName = layerName.rightJustified(ident*options->identSize + layerName.length(), ' ', false);
           if (options->hideLayerTypeIcons){
               displayName = restoreNamePart(displayName, item->getLayerType()  );
           }
        }
    }
    item->setDisplayName(displayName);

    // prepare full path of node for showing in tooltip
    QString fullName = "";
    LC_LayerTreeItem* parent = item->parent();
    if (parent != nullptr){
        QString parentPath = parent->getFullName();
        if (parentPath.length()>0){
             fullName = parentPath.append(options->layerLevelSeparator).append(item->getName());
        }
        else{
            fullName = item->getName();
        }
    }
    item->setFullName(fullName);
}

// FIXME - check for duplicate in the same parent
/**
 * Perform conversion of item denoted by index to specified layer type
 * @param selectedIndex model index for item
 * @param toLayerType new layer type
 * @return whether item is converted or not
 */
bool LC_LayerTreeModel::convertToType(QModelIndex &selectedIndex, int toLayerType){
    bool result = false;
    if (selectedIndex.isValid()){
        LC_LayerTreeItem *layerItem = getItemForIndex(selectedIndex);//
        if (!layerItem->isZero()){
            if (!layerItem -> isVirtual()){
              result = doConvertToType(layerItem, toLayerType);
            }
        }
    }
    return result;
}

/**
 * Utility method for performing layer type conversion. Renames given layer according to naming convention for
 * given layer type
 * @param layerItem layer tree item
 * @param toLayerType new layer type
 * @return true if renamed
 */
bool LC_LayerTreeModel::doConvertToType(LC_LayerTreeItem *layerItem , int toLayerType){
    RS_Layer* layer = layerItem->getLayer();
    QString layerName = layerItem->getName();

    QString originalItemName = cleanupLayerName(layerName);

    // if changing the type will lead to name duplication, we'll not prohibit rename, but mark new layer name as a copy
    QString prefix = options->copiedNamePathPrefix;
    QString suffix = options->copiedNamePathSuffix;

    QString newItemName = findNewUniqueName(layerItem->parent(), originalItemName, prefix, suffix, toLayerType);
    QString newName = restoreNamePart(newItemName, toLayerType);
    QString path = createItemPathString(layerItem, false, true, newName);

    layer->setName(path);

    return true;
}

/**
 * Utility method that removes suffixes specific for secondary layer type (if any) and returns logical layer name
 * @param layerName inner layer name (with suffix)
 * @return logical name without suffix or provided name if suffix is absent
 */
QString LC_LayerTreeModel::cleanupLayerName(QString &layerName) const{
    QString mainName = layerName;
    if (layerName.endsWith(options->alternatePositionLayerNameSuffix)){
        mainName = layerName.left(layerName.length() - options->alternatePositionLayerNameSuffix.length());
    }
    else if (layerName.endsWith(options->informationalLayerNameSuffix)){
        mainName = layerName.left(layerName.length() - options->informationalLayerNameSuffix.length());
    }
    else if (layerName.endsWith(options->dimensionalLayerNameSuffix)){
        mainName = layerName.left(layerName.length() - options->dimensionalLayerNameSuffix.length());
    }
    return mainName;
}

/**
 * Recreates internal name of the layer item based on logical name according to naming convention for given layer type.
 * @param name logical name of layer
 * @param layerType type of the layer
 * @return internal name of the layer that takes into consideration naming convention for layer type
 */
QString LC_LayerTreeModel::restoreNamePart(QString name, int layerType){
    switch (layerType){
    case LC_LayerTreeItem::VIRTUAL:
    case LC_LayerTreeItem::NORMAL:
        return name;
    case LC_LayerTreeItem::ALTERNATE_POSITION:
        return name.append(options->alternatePositionLayerNameSuffix);
    case LC_LayerTreeItem::DIMENSIONAL:
        return name.append(options->dimensionalLayerNameSuffix);
    case LC_LayerTreeItem::INFORMATIONAL:
        return name.append(options->informationalLayerNameSuffix);
    default:
        return name; // tmp - or empty string?
    }
}
/**
 * Generates string that represents full path to the item based on the list of items that forms that path.
 * Default naming convention is used.
 * If layer name should be alternated, alternative layer name is used for the last element of created path
 * @param itemsPathAsList list of layer that forms the path
 * @param alternateSourceName alternative layer of the last layer in the path
 * @param alternativeName indicates that name should be alternated
 * @return generated full name
 */
QString LC_LayerTreeModel::generateLayersPathString(QList<LC_LayerTreeItem*>  itemsPathAsList, bool alternateSourceName, QString &alternativeName){
    return doGenerateLayersPathString(itemsPathAsList, alternateSourceName, alternativeName, options->layerLevelSeparator);
}

/**
 * Utility method for path generation using provided layer separator.
 * @param itemsPathAsList list of items that represents items path. Root is the last element in the list
 * @param alternateSourceName flag that indicates that deepest child in the path should have alternative name
 * @param alternativeName   name that should be used for the last item in the path (deepest child)
 * @param usingLayerLayerSeparator separator that used for path elements
 * @return
 */
QString LC_LayerTreeModel::doGenerateLayersPathString(QList<LC_LayerTreeItem*>  &itemsPathAsList, bool alternateSourceName, QString &alternativeName,
                                                      QString &usingLayerLayerSeparator){
    int count = itemsPathAsList.size();
    QString result = "";
    // element with list.length() index is model root node, no name there
    // root is the last item in the list

    int startIndex = count-2;
    bool skipPrimary = false;
    for (int i = startIndex; i>=0; i--){
        // since we create artificial item ident for secondary layer, we may need to skip it
        if (skipPrimary){
            skipPrimary = false;
        }
        else{
            LC_LayerTreeItem* item = itemsPathAsList.at(i);
            QString name = item->getName();
            QString restoredPartName = restoreNamePart(name, item->getLayerType());
            result = result.append(restoredPartName);
            if (i > 0){
                result = result.append(usingLayerLayerSeparator);
            }
            // skip primary on the next step, if needed
            if (item->getPrimaryItem() != nullptr){
                skipPrimary = true;
            }}
    }

    if (alternateSourceName){
        // move to settings
        if (result.length() > 0){
             result = result.append(usingLayerLayerSeparator);
         }
         result = result.append(alternativeName);

    }
    return result;
}

/**
 * Method generates unique name for child of given destination item based on given name candidate and expected layer type.
 * If destination item already has children with provided name/layer type, new name is generated iteratively by
 * adding given prefix, suffix and counter (until no duplicate will be found).
 *
 * @param destination layer item where duplicated name should be avoided
 * @param name candidate for child layer name
 * @param copyPrefix prefix that will be used as part of the name if duplicate is found
 * @param copySuffix suffix that will be used as part of the name if duplicate is found
 * @param layerType layer type used for name creation
 * @return new unique name for the layer
 */
QString  LC_LayerTreeModel::findNewUniqueName(LC_LayerTreeItem* destination, QString &name, QString &copyPrefix, QString &copySuffix, int layerType){
   QString result = name;
   int i = 0;

   QString itemName(name);
   result = itemName;

   while (destination->hasChildWithName(result, layerType)){
       QString nameCopyTry(name);
       result = nameCopyTry.append(copyPrefix).append(QString::number(i)).append(copySuffix);
       i++;
   }   
   return result;
}

/**
 * Creates path string for given item. Actually, it restores part of raw layer name for this item.
 * First collects the path from item to parent, and than generates path string
 * @param item item for which path should be generated
 * @param includeSelf indicates whether the name of the item should be also included into the path
 * @param alternateSourceName  flag that indicates that instead of current name of the given item alternative one should be used
 * @param newItemName name that should be used in created path instead of the name of given item, if name if item should be replaced in the path
 * @return full path string
 */
QString LC_LayerTreeModel::createItemPathString(LC_LayerTreeItem* item, bool includeSelf, bool alternateSourceName, QString &newItemName){
    QList<LC_LayerTreeItem*> sourcePath;
    item->collectPathToParent(sourcePath, includeSelf);
    QString result = generateLayersPathString(sourcePath, alternateSourceName, newItemName);
    return result;
}
/**
 * Support of drag&drop operation - moving the source item to the destination item (so the source item will be child of
 * the destination item.
 * Calculates the prefix for the subtree of source item, and renames corresponding layers by replacing source prefix
 * in the name to the name prefix that corresponds to the destination item
 * @param source source layer tree item
 * @param destination destination item to which source will be moved as a child
 * @return true if
 */
bool LC_LayerTreeModel::performReStructure(LC_LayerTreeItem*  source, LC_LayerTreeItem*  destination){

    bool layersModified = false;
    bool allowRestructure = isValidRestructure(source, destination);
    if (allowRestructure){
        QString sourceName = source->getName();

        // prefix of names that corresponds to the source item
        QString originalLayersNamePathToBeReplaced = createItemPathString(source, true, false, sourceName);

        // check whether within destination item there is an item with the same name as for source
        QString prefix = options->copiedNamePathPrefix;
        QString suffix = options->copiedNamePathSuffix;
        QString newSourceName = findNewUniqueName(destination, sourceName, prefix, suffix, source->getLayerType());

        // prefix of layer names that corresponds to the destination item
        QString replacingLayersNamePath = createItemPathString(destination, true, true, newSourceName);

        // collect layers for children of the source
        QList<LC_LayerTreeItem *> sourceLayersToRename;
        LC_LayerTreeItemAcceptor acceptAllAcceptor;
        source->collectDescendantChildren(sourceLayersToRename, &acceptAllAcceptor, true);

        // rename collected layers by replacing source prefix to destination prefix in their names
        layersModified = renameLayers(sourceLayersToRename, originalLayersNamePathToBeReplaced, replacingLayersNamePath);
    }
    return layersModified;
}

/**
 * Utility method for layers rename operation. For each level of given list of layer tree items, creates new name of the layer by
 * replacing old prefix of layer's name to the new one.
 * @param layersList list of layers to rename
 * @param fromNamePrefix old prefix name to be replace by new one
 * @param toNamePrefix new prefix name that will replace old one
 * @return map of where the key is layer and value is new name of the layer
 */
QHash<RS_Layer*, QString> LC_LayerTreeModel::prepareLayerRename(QList<LC_LayerTreeItem*> &layersList, QString &fromNamePrefix, QString &toNamePrefix){
    QHash<RS_Layer*, QString> result;
    int layersCount = layersList.length();
    int fromNamePrefixLen = fromNamePrefix.size();
    for (int i = 0; i< layersCount; i++){
        LC_LayerTreeItem* layerItem = layersList.at(i);
        RS_Layer* layer = layerItem->getLayer();
        if (layer){
            QString name = layer->getName();
            if (name.startsWith(fromNamePrefix)){
                // ensure that only first occurrence (prefix) will be replaced there
                QString newName = name.replace(name.indexOf(fromNamePrefix), fromNamePrefixLen, toNamePrefix);
                result.insert(layer, newName);
            }
        }
        else{
            continue;
        }
    }
    return result;
}
/**
 * Renames the list of layers in given list of tree items by replacing old prefix of layer name to new one
 * @param layersList list of layer tree items
 * @param fromNamePrefix old prefix of the name to be replace by new one
 * @param toNamePrefix new prefix of the layer's name that replaces old one
 * @return true if at least one layer was renamed
 */
bool LC_LayerTreeModel::renameLayers(QList<LC_LayerTreeItem*> layersList, QString &fromNamePrefix, QString &toNamePrefix){

    QHash<RS_Layer*, QString> layersMap = prepareLayerRename(layersList, fromNamePrefix, toNamePrefix);
    bool result = renameLayersMap(layersMap);
    return result;
}

/**
 * Perform renames of layers stored in provided map
 * @param layersMap key is layer, value - new name of layer
 * @return true if at least one layer is renamed
 */
bool LC_LayerTreeModel::renameLayersMap(const QHash<RS_Layer *, QString> &layersMap) const{
    bool layersRenamed = false;
    QHashIterator<RS_Layer *, QString> iter{layersMap};
    while (iter.hasNext()) {
        iter.next();
        RS_Layer *layer = iter.key();
        QString  newName = iter.value();
        layer->setName(newName);
        layersRenamed = true;
     }
    return layersRenamed;
}

/**
 * Method evaluates whether given source item may be moved as child item of the destination item.
 * That check is used in (flag()) to control drag&drop as well as by restructuring method.
 * @param source source item
 * @param destination destination item to which source will added as child
 * @return true if moving source to destination is allowed, false otherwise
 */
bool LC_LayerTreeModel::isValidRestructure(LC_LayerTreeItem*  source, LC_LayerTreeItem*  destination) const{

    if (source == nullptr){ // should not be possible, yet still
        return false;
    }

    // prohibit dropping into own parent, allow dropping to viewport
    if (!destination->parent()){
        if (source->parent() == rootItem){ // avoid duplication of top-level items
            return false;
        }
       return true;
    }

    // do not allow drop to "0" layer.
    if (destination->isZero()){
        return false;
    }

    // don't let to drop on secondary layers
    // tbd- the alternative position layer potentially hold dimensions too... ?

    int destinationLayerType = destination->getLayerType();
    if (destinationLayerType == LC_LayerTreeItem::DIMENSIONAL ||
        destinationLayerType == LC_LayerTreeItem::INFORMATIONAL ||
        destinationLayerType == LC_LayerTreeItem::ALTERNATE_POSITION){
        return false;
    }

    // don't let to drop on existing parent
    if (source -> parent() == destination){
        return false;
    }

     // don't let drop item to one of own descendants
     if (destination->isDescendantOf(source)){
         return false;
     }

     // don't let dropping virtual layer to normal one...
     // actually, it's possible in general, yet it will lead to
     // situation where normal item as normal children. That's
     // in general possible, yet looks rather like naming convention
     // error, so we'd rather prohibit it there

     bool destinationVirtual = destination -> isVirtual();

     if (source->isVirtual()){
         return destinationVirtual; // virtual layer may be dropped on virtual only
     }
     else{
         int sourceItemType = source-> getLayerType();
         if (sourceItemType == LC_LayerTreeItem::NORMAL){
             // check that we'll drop it to virtual only
             return destinationVirtual;
         }

         // secondary item
         // let's drop it everywhere so far?
         // allow dragging items except for "0" layer, yet we already checked for "0"
         return true;
     }
}
/**
 * Get active layer from layer list and update the model to mark active layer paths accordingly
 * @param ll
 */
void LC_LayerTreeModel::proceedActiveLayerChanged(RS_LayerList* ll) {
    if (ll == nullptr)
        return;
    RS_Layer* activeLayer = ll->getActive();
    // remark items for active layer (without rebuilding entire model)
    rootItem->rebuildActivePath(activeLayer);
}

 /**
  * Utility method for retrieving underlying tree item for given model index
  * @param index model index
  * @return corresponding layer item
  */
LC_LayerTreeItem* LC_LayerTreeModel::getItemForIndex(const QModelIndex &index) const{
    LC_LayerTreeItem* result = nullptr;
    if (index.isValid()){
      result = static_cast<LC_LayerTreeItem*>(index.internalPointer());
    }
    return result;
}

/**
 * Adjusted given column in treeview taking into consideration whether layer type icons are shown or not.
 * Utility method to simply column/related processing
 * @param column column from model index
 * @return column to use in comparisons
 */
int LC_LayerTreeModel::translateColumn(int column) const{
    int result = column;
    if (options->hideLayerTypeIcons){
        result = column + 1;
    }
    return result;
}

/**
 * Overridden method of the model
 * @param index model index
 * @param role data role
 * @return data for given index and role
 */
QVariant LC_LayerTreeModel::data ( const QModelIndex & index, int role ) const {
    if (!index.isValid())
        return QVariant();

    LC_LayerTreeItem *layerItem = getItemForIndex(index);
    int col = translateColumn(index.column());

    switch (role) {
    case Qt::DecorationRole: {
        switch (col) {
            case EMPTY: // show layer type icon
                if (layerItem->isVirtual()){
                    return iconLayerVirtual;
                } else {
                    int layerType = layerItem->getLayerType();
                    switch (layerType) {
                        case LC_LayerTreeItem::NORMAL:
                            return iconLayerActual;
                        case LC_LayerTreeItem::DIMENSIONAL:
                            return iconLayerDimensional;
                        case LC_LayerTreeItem::INFORMATIONAL:
                            return iconLayerInformationalNotes;
                        case LC_LayerTreeItem::ALTERNATE_POSITION:
                            return iconLayerAlternatePosition;
                        default: {
                            // we should not be there, it means invalid/unsupported layer type
                            // TODO - flag error
                        }
                    }
                }
                break;

            case VISIBLE: // layer's visibility icon
                if (layerItem->isVisible()){
                    return layerVisible;
                }
                return layerHidden;

            case LOCKED: // layer lock status
                if (!layerItem->isLocked()){
                    return layerDefreeze;
                }
                return layerFreeze;

            case PRINT: // layer print status
                if (!layerItem->isPrint()){
                    return layerNoPrint;
                }
                return layerPrint;

            case CONSTRUCTION: // construction flat
                if (!layerItem->isConstruction()){
                    return layerNoConstruction;
                }
                return layerConstruction;

            default:
                break;
        }
        break;
    }

    case Qt::UserRole:
        return layerItem->getName();

    case Qt::DisplayRole:
        if (NAME == col) { // display name of item
            QString displayName =  layerItem->getDisplayName();
            return displayName;
        }
        break;

    case Qt::BackgroundRole:
        if (layerItem->isVirtual()){  // background for virtual layer
            return options->virtualLayerBgColor;
        }
        else {
            if(COLOR_SAMPLE == col) {  // layer of color pen
                RS_Layer* layer = layerItem->getLayer();
                if (layer){
                    return layer->getPen().getColor().toQColor();
                }
            }
            else if (layerItem->isActiveLayer()){
                return options->activeLayerBgColor;
            }            
        }
        break;

    case Qt::FontRole:
        if (NAME == col) {
            QFont font;
            // todo - potentially, that may be also controlled by settings

            // mark active part items as bold
            if (layerItem->isPartOfActivePath()){
                font.setBold(true);
            }
            // mark virtual layers by italic
            if (layerItem->isVirtual()){
                font.setItalic(true);
            }
            return font;
        }
        break;

    case Qt::ForegroundRole:{

            if (layerItem -> isMatched()){ // highlighting of items that are matched by filter regexpt
                return options->matchedItemColor;
            }

// FIXME - layer color by settings
//            RS_Layer* layer = layerItem->getLayer();
//            if (layer){
//               return layer->getPen().getColor().toQColor();
//            }
        break;
    }

    case Qt::ToolTipRole:{
        if (options->showToolTips){ // show tooltip with full name of layer
            if (NAME == col) {
                QString displayName =  layerItem->getFullName();
                return displayName;
            }
        }
        break;
    }

    default:
        // do nothing;
        break;
    }

    return QVariant();
}
/**
 * Overridden flags method of the model
 * @param index
 * @return
 */
Qt::ItemFlags LC_LayerTreeModel::flags(const QModelIndex & index) const{
    if (!index.isValid()){
        if (flatMode){
            return {};
        } else {
            // in normal (not flat) mode we'll support of dropping on viewport, and there the index will be invalid
            return Qt::ItemIsDropEnabled;
        }
    }

    int col = translateColumn(index.column());

    if (col == NAME) {
        Qt::ItemFlags result = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        LC_LayerTreeItem *layerItem = getItemForIndex(index);

        bool dragEnabled = !flatMode && options->dragDropEnabled; // disable drag&drop in flat mode

        if (dragEnabled){
            // allow dragging items except for "0" layer.
            if (layerItem->isZero()){
                dragEnabled= false;
            }

            if (dragEnabled){
                result = result | Qt::ItemIsDragEnabled;
            }
        }

        // take care of drag&drop
        if (currentlyDraggingItem != nullptr && !flatMode){
            // check whether it is allowed to drop currently dragging item on this item
            bool dropEnabled = isValidRestructure(currentlyDraggingItem, layerItem);
            if (dropEnabled){
                result = result  | Qt::ItemIsDropEnabled;
            }
        }
        return result;
    }
    else
        return Qt::ItemIsEnabled;
}
/**
 * Utility method that returns list of descendent layers based on provided acceptance conditions
 * @param acceptor condition for selecting layers
 * @return list of collected layers
 */
QList<RS_Layer*> LC_LayerTreeModel::collectLayers(LC_LayerTreeItemAcceptor* acceptor){
    QList<RS_Layer*> result;
    rootItem->collectLayers(result, acceptor);
    return result;
}

/**
 * Utility method that exposed inherited method - used for selection state control
 * @return
 */
QModelIndexList LC_LayerTreeModel::getPersistentIndexList(){
    return persistentIndexList();
}

/**
 *  Updates regexp expression as well as regexp mode (highlight or filter).
 *  Method sets fields only and does not refresh the model.
 * @param regexp regexp string from filtering box
 * @param highlightMode  mode of filter, if true - items are highlighted, if false - filtered
 */
void LC_LayerTreeModel::setFilteringRegexp(QString &regexp, bool highlightMode){
//   Actually, this is a bit ugly setter as it does not force model rebuild.However, update will be subsequent
//   call... ok so far, it should not be called externally other than from widget.

    filteringRegexp.setPattern(regexp);
    //filteringRegexp.setPatternSyntax(QRegExp::WildcardUnix);
    hasRegexp = !regexp.trimmed().isEmpty();
    regexpHighlightMode = highlightMode;
}

/**
 * Utility method to notify that model is changed
 */
void LC_LayerTreeModel::emitDataChanged(){
   dataChanged(QModelIndex(), QModelIndex());
}

/**
 * Tries to find item that holds given layer
 * @param layer target layer
 * @return tree item that stores target level
 */
LC_LayerTreeItem* LC_LayerTreeModel::getItemForLayer(RS_Layer* layer) const{
    QG_LayerTreeItemAcceptorSameLayerAs ACCEPT_SAME(layer);
    QList<LC_LayerTreeItem*> items;
    rootItem->collectDescendantChildren(items, &ACCEPT_SAME, false);

    LC_LayerTreeItem* result = nullptr;
    if (items.size() == 1){ // only 1 result expected to be returned.
        result = items.at(0);
    }
    return result;
}

/**
 * Creates copy of layers sub-tree starting from item identified by given index
 * @param selectedIndex model index for item
 * @return map of layers. Key - original Layer, Value - new layer (copy)
 */
QHash<RS_Layer*, RS_Layer*> LC_LayerTreeModel::createLayersCopy(QModelIndex& selectedIndex){
    QHash<RS_Layer*, RS_Layer*> result;
    if (selectedIndex.isValid()){
        LC_LayerTreeItem *source = getItemForIndex(selectedIndex);
        result = doCreateLayersCopy(source, true, source->getLayerType());
    }
    return result;
}

/**
 * Created copy(s) of single layer or layer with descendants
 * @param source source item
 * @param includeChildren flag that defined whether copy for child layers should be created
 * @param newLayerType new layer type
 * @return map of layers, key is original layer, value - original layer's copy
 */
QHash<RS_Layer*, RS_Layer*> LC_LayerTreeModel::doCreateLayersCopy(LC_LayerTreeItem* source, bool includeChildren, int newLayerType){
   QHash<RS_Layer*, RS_Layer*> result;

   if (source->isVirtual()){
       if (includeChildren){ // need to create copies for children too
           copyChildrenLayers(source, newLayerType, result);
       }
   }
   else { // just create copy of layer
       createFirstLayerCopy(result, source, newLayerType); // copy single non-virtual layer
       if (newLayerType == LC_LayerTreeItem::NORMAL){ // if normal layer - we may have secondary ones
           copyChildrenLayers(source, newLayerType, result);
       }
   }
   return result;
}
/**
 * Creates layers copy for children of given parent item
 * @param parent parent item
 * @param newParentLayerType type of parent item
 * @param result map with result (original/copy layers)
 */
void LC_LayerTreeModel::copyChildrenLayers(LC_LayerTreeItem *parent, int newParentLayerType, QHash<RS_Layer *, RS_Layer *> &result){
    int childCount = parent->childCount();
    if (childCount > 0){
        QString newLayerNamePart = createFirstCopiedItemNew(parent, newParentLayerType);
        QString namePrefixReplaceTo = createItemPathString(parent, false, true, newLayerNamePart);
        QString sourceName = parent->getName();
        QString namePrefixReplaceFrom = createItemPathString(parent, true, false, sourceName);
        doCreateChildLayersCopy(result, parent, namePrefixReplaceFrom, namePrefixReplaceTo);
    }
}

/**
 * Creates copy of the very top layer in the copied sub-tree. Generates unique name for the layer, based on it
 * prepares full name of the layer, creates layer and add original/copy pair to the result map
 * @param result results map
 * @param source source tree item
 * @param newLayerType new layer type
 */
void LC_LayerTreeModel::createFirstLayerCopy(QHash<RS_Layer*,RS_Layer*> &result, LC_LayerTreeItem* source, int newLayerType){

    QString newLayerNamePart = createFirstCopiedItemNew(source, newLayerType);
    QString newLayerWithType = restoreNamePart(newLayerNamePart, newLayerType);
    QString newLayerName = createItemPathString(source, false, true, newLayerWithType);

    RS_Layer* sourceLayer = source->getLayer();
    auto* newLayer = new RS_Layer(newLayerName);
    result.insert(sourceLayer, newLayer);
}

/**
 * Creates a copy of child layers. Collect all descendants of given item, than iterates over layers and prepares new
 * name of layer's copy (by replacing old prefix of path to new one).
 * New name is set for each copy and original/copy layers pair is added to the provided map.
 *
 * @param result map of layers (old->new)
 * @param source source item, copies will be created for it's children
 * @param oldPrefix old layers names prefix that should be replaced on copy
 * @param newPrefix new prefix that will be used instead of ald ones in names of copied layers
 */
void LC_LayerTreeModel::doCreateChildLayersCopy(QHash<RS_Layer*, RS_Layer*>&result, LC_LayerTreeItem* source, QString &oldPrefix, QString &newPrefix){

    QList<RS_Layer*> childLayers;
    LC_LayerTreeItemAcceptor ACCEPT_ALL;

    source->collectLayers(childLayers, &ACCEPT_ALL, false);

    for (auto const& layer: childLayers){
         QString layerName = layer->getName();
         if (layerName.startsWith(oldPrefix)){
             // replace only first occurrence
             QString newLayerName;
             newLayerName = layerName.replace(layerName.indexOf(oldPrefix), oldPrefix.size(), newPrefix);
             auto* newLayer = new RS_Layer(newLayerName);
             result.insert(layer, newLayer);
         }
    }
}

/**
 * Generates name of the topmost layer in the sub-tree that should be copied.
 * Named is generated to ensure that there will not be duplicating item names among source item siblings
 * @param source source item
 * @param newLayerType new layer type
 * @return name of the layer
 */
QString LC_LayerTreeModel::createFirstCopiedItemNew(LC_LayerTreeItem* source, int newLayerType){
    QString itemName = source->getName();
    LC_LayerTreeItem *parent = source->parent();
    QString prefix = options->copiedNamePathPrefix;
    QString suffix = options->copiedNamePathSuffix;
    QString newItemName = findNewUniqueName(parent, itemName, prefix, suffix, newLayerType);
    return newItemName;
}

/**
 * Renames provided virtual layer item (and it descendants, accordingly)
 * @param source source tree item
 * @param newSourceName new name of item
 * @return true if renamed
 */
bool LC_LayerTreeModel::renameVirtualLayer(LC_LayerTreeItem *source, QString &newSourceName){
    QHash<RS_Layer *, QString> layersMapToRename = doGetVirtualLayerRenameLayersMap(source, newSourceName);
    bool result = renameLayersMap(layersMapToRename);
    return result;
}

/**
 * Layer that generates full display path for the layer. At the moment, it uses layer level separator as it is
 * specified in options, however, some normalized form of name that does not depend on current separator
 * potentially may be used
 * @param item source item
 * @return  display name
 */
// TODO - decide how to show the path - as original underlying layer name or as part of names...
QString LC_LayerTreeModel::generateLayersDisplayPathString(LC_LayerTreeItem *item){
    QList<LC_LayerTreeItem*> sourcePathItems;
    item->collectPathToParent(sourcePathItems, false);
    QString prefix = "";
    QString separator = options->layerLevelSeparator;
    QString result = doGenerateLayersPathString(sourcePathItems, false, prefix, separator);
    return result;
}

/**
 * Returns map of layers that will be be affected by rename of virtual layers (actually, descendants of virtual layer that
 * is renamed
 * @param source source item for virtual layer
 * @param newSourceName new name for virtual layer item
 * @return map of layers and new name for them (after rename)
 */
QHash<RS_Layer *, QString>  LC_LayerTreeModel::doGetVirtualLayerRenameLayersMap(LC_LayerTreeItem *source, QString &newSourceName){
    QString sourceName = source->getName();

    // prefix path in layer names that will be replaced as result of rename
    QString originalLayersNamePathToBeReplaced = createItemPathString(source, true, false, sourceName);

    // prefix path in layer names that will be assigned as result of rename
    QString replacingLayersNamePath = createItemPathString(source, false, true, newSourceName);

    // collect descendants
    QList<LC_LayerTreeItem *> sourceLayersToRename;
    LC_LayerTreeItemAcceptor acceptAllAcceptor;
    source->collectDescendantChildren(sourceLayersToRename, &acceptAllAcceptor, true);

    // preparing layers map
    QHash<RS_Layer*, QString> layersToRename = prepareLayerRename(sourceLayersToRename, originalLayersNamePathToBeReplaced, replacingLayersNamePath);
    return layersToRename;
}

/**
 * Identifies layers that will be affected affected by renaming primary layer operation and returns a map of layers and
 * new names.
 * If layer type is not changed - secondary layers will be renamed accordingly to the primary layer rename. Otherwise,
 * only the primary layer itself will be renamed.
 * @param source item to be renamed
 * @param newSourceName  new name of layer (without sub-type suffix)
 * @param newLayerType   new type of source layer
 * @return key in the map - layer to be renamed, value - new name of layer
 */
QHash<RS_Layer *, QString>  LC_LayerTreeModel::doGetPrimaryLayerRenameLayersMap(LC_LayerTreeItem *source, QString &newSourceName, int newLayerType){
    QString sourceName = source->getName();
    int originalLayerType = source->getLayerType();

    bool typeChanged = newLayerType != originalLayerType;
    bool nameChanged = sourceName != newSourceName;

    QString originalLayersNamePathToBeReplaced = createItemPathString(source, true, false, sourceName);

    QList<LC_LayerTreeItem *> sourceLayersToRename;
    LC_LayerTreeItemAcceptor acceptAllAcceptor;

    QString newName = restoreNamePart(newSourceName, newLayerType);

    if (typeChanged){
          // we'll rename only the primary layer itself on type change
          sourceLayersToRename << source;
     }
    else if (nameChanged){
            // we'll rename primary layer and it's children
            source->collectDescendantChildren(sourceLayersToRename, &acceptAllAcceptor, true);
    }

    //  prefix in layer names that will be used after rename
    QString replacingLayersNamePath = createItemPathString(source, false, true, newName);

    QHash<RS_Layer*, QString> layersToRename = prepareLayerRename(sourceLayersToRename, originalLayersNamePathToBeReplaced, replacingLayersNamePath);
    return layersToRename;
}

/**
 * Returns list of current layer named for layers that will be affected by rename virtual layer operation
 * @param source virtual layer item
 * @param newSourceName new item name
 * @return list of names
 */
QStringList LC_LayerTreeModel::getLayersListForRenamedVirtualLayer(LC_LayerTreeItem *source, QString &newSourceName){
    QHash<RS_Layer *, QString> layersToRename = doGetVirtualLayerRenameLayersMap(source, newSourceName);
    QStringList result = layersToRename.values();
    return result;
}

/**
 * Returns list of layer names for layers that will be affected by renaming primary normal layer
 * @param source source item
 * @param newSourceName new name of item
 * @param newLayerType  new layer type
 * @return list of names
 */
QStringList LC_LayerTreeModel::getLayersListForRenamedPrimary(LC_LayerTreeItem* source, QString &newSourceName, int newLayerType){
    QHash<RS_Layer *, QString> layersToRename = doGetPrimaryLayerRenameLayersMap(source, newSourceName, newLayerType);
    QStringList result = layersToRename.values();
    return result;
}

/**
 * Creates full name for layer.
 * @param treeItem  source item with layer
 * @param layerName name of layer item
 * @param layerType type of layer item
 * @param newLayer true if name for new layer is generated, false if for existing
 * @return
 */
QString LC_LayerTreeModel::createFullLayerName(LC_LayerTreeItem *treeItem, QString &layerName, int layerType, bool newLayer){
    QString result;
    QString newLayerName = restoreNamePart(layerName, layerType);
    if (treeItem != nullptr){
        result = createItemPathString(treeItem, newLayer, true, newLayerName);
    } else {
        result = newLayerName;
    }

    return result;
}

/**
 * Renames primary layer, based on options - may also rename secondary layers for this layer
 * @param layerItem source item
 * @param newName new name of layer item
 * @param newLayerType  new type of layer
 */
void LC_LayerTreeModel::renamePrimaryLayer(LC_LayerTreeItem *layerItem, QString newName, int newLayerType){
    RS_Layer* layer = layerItem->getLayer();
    if (layerItem->childCount()==0){ // no secondary layers there, just rename this layer
        QString newLayerName = createFullLayerName(layerItem, newName, newLayerType, false);
        layer->setName(newLayerName);
    }
    else{ // as we have children, probably they should be renamed too
        if (options->renameSecondaryLayersOnPrimaryRename){
            QHash<RS_Layer *, QString> layersMapToRename = doGetPrimaryLayerRenameLayersMap(layerItem, newName, newLayerType);
            renameLayersMap(layersMapToRename);
        }
        else{
            QString newLayerName = createFullLayerName(layerItem, newName, newLayerType, false);
            layer->setName(newLayerName);
        }
    }
}

void LC_LayerTreeModel::setCurrentlyDraggingItem(LC_LayerTreeItem *item){currentlyDraggingItem = item;}

void LC_LayerTreeModel::setFlatMode(bool mode){flatMode = mode;}

LC_LayerTreeItem *LC_LayerTreeModel::getCurrentlyDraggingItem() {return currentlyDraggingItem;}
