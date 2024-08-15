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

#ifndef LC_ACTIONSSHORTCUTSDIALOG_H
#define LC_ACTIONSSHORTCUTSDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QItemSelection>
#include "lc_shortcutstreemodel.h"
#include "lc_actiongroupmanager.h"
#include "lc_shortcutstreeview.h"

namespace Ui {
    class LC_ActionsShortcutsDialog;
}

class LC_ActionsShortcutsDialog : public QDialog{
    Q_OBJECT

public:
    explicit LC_ActionsShortcutsDialog(QWidget *parent, QMap<QString, QAction *> &map, LC_ActionGroupManager *pManager);
    ~LC_ActionsShortcutsDialog() override;
    void accept() override;
    void reject() override;
protected slots:
    void setKeySequence(const QKeySequence &key);
    void slotFilteringMaskChanged();
    void onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onResetCurrentItemClicked();
    void onResetAllClicked();
    void showConflicts();
    void onClearClicked();
    void onImportClicked();
    void onExportClicked();
protected:
    Ui::LC_ActionsShortcutsDialog *ui;
    LC_ShortcutsTreeModel *mappingTreeModel;
    QMap<QString, QAction*> &actionsMap;
    LC_ActionGroupManager *actionGroupManager;
    LC_ShortcutTreeItem* currentItem = nullptr;
    QString keySequenceToEditString(const QKeySequence &sequence) const;
    void initTreeView();
    void createMappingModel();
    void slotTreeDoubleClicked(QModelIndex index);
    void slotTreeClicked(QModelIndex itemIndex);
    void selectItem(LC_ShortcutTreeItem *item);
    void editItem(LC_ShortcutTreeItem *item);
    bool keySequenceIsValid(const QKeySequence &sequence) const;
    void rebuildModel();
    void saveDialogPosition() const;
    void loadDialogPosition();
    static QFlags<QFileDialog::Option> getFileDialogOptions();
    static void showIOInfoDialog(bool forImport, bool ok, const QString &message);
    bool checkHasCollisions(LC_ShortcutInfo *shortcutInfo);
};

#endif // LC_ACTIONSSHORTCUTSDIALOG_H
