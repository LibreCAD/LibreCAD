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

#include "lc_shortcutstreemodel.h"
#include "lc_shortcuttreeitem.h"
#include "lc_actiongroup.h"
#include "rs_debug.h"

LC_ShortcutsTreeModel::LC_ShortcutsTreeModel(QObject *parent):QAbstractItemModel(parent) {
    filteringRegexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

LC_ShortcutsTreeModel::~LC_ShortcutsTreeModel() {
    delete rootItem;
    qDeleteAll(shortcuts);
}

void LC_ShortcutsTreeModel::rebuildModel(LC_ActionGroupManager *pManager){
    beginResetModel();
    auto* root = new LC_ShortcutTreeItem(nullptr, QIcon(), "", "");

    const QList<LC_ActionGroup *> &groupsList = pManager->allGroupsList();

    for (LC_ActionGroup* group: groupsList){
        if (group->isActionMappingsMayBeConfigured()) {
            auto *groupChild = new LC_ShortcutTreeItem(root, group->getIcon(), group->getDescription(),
                                                       group->getDescription());

            for (QAction *action: group->actions()) {

                const QVariant &configurable = action->property(LC_ShortcutInfo::PROPERTY_ACTION_SHORTCUT_CONFIGURABLE);
                if (configurable.isValid()){
                    if (!configurable.toBool()){
                        continue;
                    }
                }

                QString actionName = action->objectName();
                bool hasRegexpMatch = false;
                if (filterForConflicts) {
                } else if (hasRegexp) {
                    int pos = 0;
                    QString actionText = action->text().remove("&").toLower();
                    hasRegexpMatch = filteringRegexp.match(actionText, pos).hasMatch();

                    if (regexpHighlightMode) {
                        // we'll highlight it later based on the flag
                    } else { // in filtering mode, skip if no match
                        if (hasRegexpMatch) {
                            // don't need it anymore for highlight mode, so we'll clear it
                            hasRegexpMatch = false;
                        } else {
                            continue; // skip this layer at all as it was not matched
                        }
                    }
                }

                auto *shortcutInfo = shortcuts[actionName];
                if (shortcutInfo == nullptr) {
                    shortcutInfo = new LC_ShortcutInfo(actionName, action->shortcut());
                    shortcuts[actionName] = shortcutInfo;
                }
                if (filterForConflicts) {
                    if (shortcutInfo->hasCollision()) {
//                        LC_ERR << shortcutInfo->getName();
                    }
                    else{
                        continue;
                    }
                }
                LC_ShortcutTreeItem *child = groupChild->addChild(action, shortcutInfo);
                child->setMatched(hasRegexpMatch);
            }
            if (groupChild->childCount() > 0) {
                root->addChild(groupChild);
            } else {
                delete groupChild;
            }
        }
    }

    delete rootItem;
    setRootItem(root);
    endResetModel();
    dataChanged(QModelIndex(), QModelIndex());
}

Qt::ItemFlags LC_ShortcutsTreeModel::flags(const QModelIndex &index) const {
    return QAbstractItemModel::flags(index);
}

int LC_ShortcutsTreeModel::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    return LAST;
}

int LC_ShortcutsTreeModel::rowCount(const QModelIndex &parent) const {
    LC_ShortcutTreeItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItemForIndex(parent);

    int result = parentItem->childCount();
    return result;
}

int LC_ShortcutsTreeModel::translateColumn(int column) const{
    return column;
}

QModelIndexList LC_ShortcutsTreeModel::getPersistentIndexList() {
    return persistentIndexList();
}

QVariant LC_ShortcutsTreeModel::data(const QModelIndex &index, int role) const {
    if (index.isValid()) {
        LC_ShortcutTreeItem *mappingItem = getItemForIndex(index);
        int col = translateColumn(index.column());

        switch (role) {
            case Qt::DecorationRole: {
                if (col == ICON) {
                    return mappingItem->getIcon();
                }
                break;
            }
            case Qt::UserRole: {
                return mappingItem->getName();
            }
            case Qt::DisplayRole: {
                switch (col){
                    case NAME:
                        return mappingItem->getName();
                    case SHORTCUT:
                        return mappingItem->getShortcutViewString();
                    default:
                        break;
                }
                break;
            }
            case Qt::BackgroundRole:
                if (mappingItem->isGroup()){  // background for virtual layer
//                return options->virtualLayerBgColor;
                    return QColor(0xf5f5f5);
                }
                break;

            case Qt::FontRole: {
                QFont font;
                if (mappingItem->isModified()){
                    font.setItalic(true);
                    if (SHORTCUT == col){
                        font.setBold(true);
                    }
                    return font;
                }
                else if (mappingItem ->hasCollision()){
                    font.setBold(true);
                    return font;
                }
                break;
            }
            case Qt::ForegroundRole:{
                if (mappingItem -> isMatched()){ // highlighting of items that are matched by filter regexpt
                    return QColorConstants::Blue/*options->matchedItemColor*/ ;
                }

                if (mappingItem->hasCollision()){
                    return QColorConstants::Red;
                }
                break;
            }

/*        case Qt::ToolTipRole:{
            if (options->showToolTips){ // show tooltip with full name of layer
                if (NAME == col) {
                    QString displayName =  layerItem->getFullName();
                    return displayName;
                }
            }
            break;
        }*/

            default:
                // do nothing;
                break;
        }
    }

    return QVariant();
}

QModelIndex LC_ShortcutsTreeModel::parent(const QModelIndex &index) const {
    if (index.isValid()){
        LC_ShortcutTreeItem *childItem = getItemForIndex(index);
        if (childItem != nullptr) {
            LC_ShortcutTreeItem *parentItem = childItem->parent();
            if (parentItem != rootItem) {
                return createIndex(parentItem->row(), 0, parentItem);
            }
        }
    }
    return QModelIndex();
}

QModelIndex LC_ShortcutsTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    LC_ShortcutTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItemForIndex(parent);

    LC_ShortcutTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

LC_ShortcutTreeItem* LC_ShortcutsTreeModel::getItemForIndex(const QModelIndex &index) const{
    LC_ShortcutTreeItem* result = nullptr;
    if (index.isValid()){
        result = static_cast<LC_ShortcutTreeItem*>(index.internalPointer());
    }
    return result;
}

void LC_ShortcutsTreeModel::setFilteringRegexp(QString &regexp, bool highlightMode) {
    filteringRegexp.setPattern(regexp);
    //filteringRegexp.setPatternSyntax(QRegExp::WildcardUnix);
    hasRegexp = !regexp.trimmed().isEmpty();
    regexpHighlightMode = highlightMode;
}

void LC_ShortcutsTreeModel::setRootItem(LC_ShortcutTreeItem *rootItem) {
    LC_ShortcutsTreeModel::rootItem = rootItem;
}

void LC_ShortcutsTreeModel::resetAllToDefault() {
    for (LC_ShortcutInfo* shortcut:shortcuts){
        shortcut->resetToDefault();
        shortcut->setCollision(false);
    }
    dataChanged(QModelIndex(), QModelIndex());
}

bool LC_ShortcutsTreeModel::checkForCollisions(LC_ShortcutInfo *shortcutInfo) {
    bool hasCollisions = false;
    for (auto currentShortcut: shortcuts){
        currentShortcut->setCollision(false);
    }
    if (shortcutInfo != nullptr) { // simple check, for this shortcut only
        QKeySequence keyToTest = shortcutInfo->getKey();
        for (auto *shortcut: shortcuts) {
            if (shortcut != shortcutInfo) {
                if (shortcut->hasTheSameKey(keyToTest)) {
                    shortcut->setCollision(true);
                    hasCollisions = true;
                }
            }
        }
        shortcutInfo->setCollision(hasCollisions);
    }
    else { // checking for all possible duplicates
        for (auto currentShortcut: shortcuts){
            if (currentShortcut->hasCollision()){
                continue;
            }
            QKeySequence keyToTest = currentShortcut->getKey();
            for (auto anotherShortcut: shortcuts){
                if (anotherShortcut != currentShortcut){
                    if (anotherShortcut->hasTheSameKey(keyToTest)){
                        hasCollisions = true;
                        currentShortcut->setCollision(true);
                        anotherShortcut->setCollision(true);
//                        LC_ERR << currentShortcut->getName() << " and " << anotherShortcut->getName();
                    }
                }
            }
        }
    }
    return hasCollisions;
}

const QMap<QString, LC_ShortcutInfo *> &LC_ShortcutsTreeModel::getShortcuts() const {
    return shortcuts;
}

void LC_ShortcutsTreeModel::setFilterForConflicts(bool filter) {
    filterForConflicts = filter;
}

void LC_ShortcutsTreeModel::collectShortcuts(QList<LC_ShortcutInfo *> &items, bool includeEmpty) {
    for (auto shortcut : shortcuts){
        if (shortcut->hasKey() || includeEmpty){
            items << shortcut;
        }
    }
}

/**
 * applies given map of shortcuts mapping to stored map
 * @param map
 * @param replace - defines whether provided shortcuts should be merged into existing map or whether they should replace them.
 * if replace is true, original shortcuts keys are cleared (to no shortcut state) first, otherwise original shortcuts survive if
 *  there is no corresponding key in provided map
 */
void LC_ShortcutsTreeModel::applyShortcuts(const QMap<QString, QKeySequence> &map, bool replace) {
    if (replace){
        for (auto shortcut: shortcuts){
            shortcut->clear();
        }
    }
    for (auto [key, value] : map.asKeyValueRange()) {
        LC_ShortcutInfo* existingShortcut = shortcuts[key];
        if (existingShortcut != nullptr){
            existingShortcut->setKey(value);
        }
        else{
            // todo - basically, this is not truly possible situation - it means that there is mapping to old action that was removed (or some trash in xml)
            // todo - not sure that should be handled somehow, so let's just skip such entries for now
        }
    }
}

bool LC_ShortcutsTreeModel::isModified() {
    for (auto shortcut: shortcuts){
        if (shortcut->isModified()){
            return true;
        }
    }
    return false;
}
