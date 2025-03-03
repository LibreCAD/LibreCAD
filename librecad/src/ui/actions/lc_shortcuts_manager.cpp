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

#include <QApplication>
#include <QDir>
#include "lc_shortcuts_manager.h"
#include "rs_settings.h"
#include "lc_shortcutsstorage.h"
#include "rs_debug.h"

const char* LC_ShortcutsManager::PROPERTY_SHORTCUT_BACKUP = "tooltip.original";

LC_ShortcutsManager::LC_ShortcutsManager() {}

int LC_ShortcutsManager::saveShortcuts(
    QMap<QString, LC_ShortcutInfo *> &shortcuts, QMap<QString, QAction *> &actionsMap) const {

    applyShortcutsMapToActionsMap(shortcuts, actionsMap);
    updateActionTooltips(actionsMap);
    
    QString defaultShortcutsFileName = getDefaultShortcutsFileName();

    int saveResult = LC_ShortcutsStorage::saveShortcuts(defaultShortcutsFileName, shortcuts.values(), false);
    return saveResult;
}

int LC_ShortcutsManager::loadShortcuts(QMap<QString, QAction *> &actionsMap) const {
    QString defaultFileName = getDefaultShortcutsFileName();
    QMap<QString, QKeySequence> shortcuts = QMap<QString, QKeySequence>();
    int loadResult = loadShortcuts(defaultFileName, &shortcuts);
    if (loadResult == LC_ShortcutsStorage::OK){
        applyKeySequencesMapToActionsMap(shortcuts, actionsMap);
    }
    updateActionTooltips(actionsMap);
    return loadResult;
}


int LC_ShortcutsManager::saveShortcuts(const QString &fileName, const QList<LC_ShortcutInfo *> &shortcutsList) const {
    int result = LC_ShortcutsStorage::saveShortcuts(fileName, shortcutsList);
    return result;
}

int LC_ShortcutsManager::loadShortcuts(const QString &filename, QMap<QString, QKeySequence> *result) const{
   return LC_ShortcutsStorage::loadShortcuts(filename, result);
}

void LC_ShortcutsManager::updateActionTooltips(const QMap<QString, QAction *> &actionsMap) const {
    bool showShortcutsInActionsTooltips = LC_GET_ONE_BOOL("Appearance","ShowKeyboardShortcutsInTooltips", true);
    updateActionShortcutTooltips(actionsMap, showShortcutsInActionsTooltips);
}

void LC_ShortcutsManager::applyShortcutsMapToActionsMap(QMap<QString, LC_ShortcutInfo*> &shortcuts, QMap<QString, QAction *> &actionsMap) const{
    for (auto [key, shortcut] : shortcuts.asKeyValueRange()){
        QAction* action = actionsMap[key];
        if (action != nullptr){
            action->setShortcut(shortcut->getKey());
        }
    }
}

void LC_ShortcutsManager::applyKeySequencesMapToActionsMap(QMap<QString, QKeySequence> &shortcuts, QMap<QString, QAction *> &actionsMap) const{
    for (auto [name, shortcut] : shortcuts.asKeyValueRange()){
        QAction* action = actionsMap[name];
        if (action != nullptr){
            action->setShortcut(shortcut);
        }
    }
}

void LC_ShortcutsManager::assignShortcutsToActions(const QMap<QString, QAction *> &map,
                                                    std::vector<LC_ShortcutInfo> &shortcutsList) const {
    for (const LC_ShortcutInfo &a: shortcutsList){
        QAction* createdAction = map[a.getName()];
        if (createdAction != nullptr){
            const QList<QKeySequence> &list = a.getKeysList();
            if (list.isEmpty()) {
                const QKeySequence &sequence = a.getKey();
                if (sequence != QKeySequence::UnknownKey) {
                    createdAction->setShortcut(sequence);
                }
            } else { // todo - support for future
                createdAction->setShortcuts(list);
            }
        }
    }
}

void LC_ShortcutsManager::updateActionShortcutTooltips(const QMap<QString, QAction *> &map, bool enable) const {
    if (enable){
        for (auto [key, action]: map.asKeyValueRange()) {
            if (action != nullptr) {
                if (!action->shortcut().isEmpty()) {
                    // action uses a custom tooltip. Backup so that we can restore it later.
                    QString tooltip = action->property(PROPERTY_SHORTCUT_BACKUP).toString();
                    if (tooltip.isEmpty()) {
                        tooltip = action->toolTip();
                        action->setProperty(PROPERTY_SHORTCUT_BACKUP, action->toolTip());
                    }
                    QColor shortcutTextColor = QApplication::palette().color(QPalette::ToolTipText);
                    QString shortCutTextColorName;
                    if (shortcutTextColor.value() == 0) {
                        shortCutTextColorName = "gray";  // special handling for black because lighter() does not work there [QTBUG-9343].
                    } else {
                        int factor = (shortcutTextColor.value() < 128) ? 150 : 50;
                        shortCutTextColorName = shortcutTextColor.lighter(factor).name();
                    }
                    action->setToolTip(QString(
                        "<p style='white-space:pre'>%1&nbsp;&nbsp;<code style='color:%2; font-size:small'>%3</code></p>")
                                           .arg(tooltip, shortCutTextColorName,
                                                action->shortcut().toString(QKeySequence::NativeText)));
                }
            }
            else{
                // if we are here, it means that there is a key for which no action was registered in actions map.
                // it might be either due to incorrect data in settings file, or if the action was created somewhere in
                // code that is outside actions factory.
                // in any case, just ignore that key
                LC_ERR << "Action is null in map, key "  << key;
            }
        }
    }
    else{
        for (QAction* action: map){
            action->setToolTip(action->property(PROPERTY_SHORTCUT_BACKUP).toString());
            action->setProperty(PROPERTY_SHORTCUT_BACKUP, QVariant()); // todo - is it really necessary to remove it?
        }
    }
}

/* guesses a descriptive text from a text suited for a menu entry
   This is equivalent to QActions internal qt_strippedText()
*/
QString LC_ShortcutsManager::strippedActionText(QString s) const{
    s.remove( QString::fromLatin1("..."));
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}

QString LC_ShortcutsManager::getShortcutsMappingsFolder() const {
    QString result = LC_GET_ONE_STR("Paths","ShortcutsMappings", QDir::homePath());
    return result;
}

QString LC_ShortcutsManager::getDefaultShortcutsFileName() const {
    QString path =  getShortcutsMappingsFolder() + "/default.lcs";
    return QDir::toNativeSeparators(path);
}
