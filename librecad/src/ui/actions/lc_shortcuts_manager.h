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

#ifndef LIBRECAD_LC_SHORTCUTS_MANAGER_H
#define LIBRECAD_LC_SHORTCUTS_MANAGER_H

#include <QString>
#include "lc_shortcutinfo.h"

class LC_ShortcutsManager {
public:
    LC_ShortcutsManager();

    int saveShortcuts(QMap<QString, LC_ShortcutInfo*> &shortcuts, QMap<QString, QAction *> &actionsMap) const;
    int saveShortcuts(const QString &fileName, const QList<LC_ShortcutInfo *> &items) const;
    void assignShortcutsToActions(const QMap<QString, QAction *> &map,
                                 std::vector<LC_ShortcutInfo> &shortcutsList) const;
    int loadShortcuts(const QString &filename, QMap<QString, QKeySequence> *result) const;
    int loadShortcuts(QMap<QString, QAction *> &map) const;
    QString getShortcutsMappingsFolder() const;
    void updateActionTooltips(const QMap<QString, QAction *> &actionsMap) const;
protected:

    static const char* PROPERTY_SHORTCUT_BACKUP;
    void applyShortcutsMapToActionsMap(QMap<QString, LC_ShortcutInfo *> &shortcuts, QMap<QString, QAction *> &actionsMap) const;
    void updateActionShortcutTooltips(const QMap<QString, QAction *> &map, bool enable) const;
    QString strippedActionText(QString s) const;
    QString getDefaultShortcutsFileName() const;
    void applyKeySequencesMapToActionsMap(QMap<QString, QKeySequence> &shortcuts, QMap<QString, QAction *> &actionsMap) const;
};
#endif //LIBRECAD_LC_SHORTCUTS_MANAGER_H
