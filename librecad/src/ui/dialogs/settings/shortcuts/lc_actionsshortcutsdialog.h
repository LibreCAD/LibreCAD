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
#include "lc_dialog.h"

namespace Ui {
    class LC_ActionsShortcutsDialog;
}

class LC_ActionsShortcutsDialog : public LC_Dialog{
    Q_OBJECT

public:
    explicit LC_ActionsShortcutsDialog(QWidget *parent, LC_ActionGroupManager *pManager);
    ~LC_ActionsShortcutsDialog() override;
    void accept() override;
    void reject() override;
protected slots:
    void onKeySequenceChanged(const QKeySequence &key);
    void onFilteringMaskChanged();
    void onTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onResetCurrentItemClicked();
    void onResetAllClicked();
    void showConflicts();
    void onClearClicked();
    void onImportClicked();
    void onExportClicked();
    void onRecordButtonToggled(bool on);
protected:
    Ui::LC_ActionsShortcutsDialog *ui;
    LC_ShortcutsTreeModel *mappingTreeModel;
    LC_ActionGroupManager *actionGroupManager;
    LC_ShortcutTreeItem* currentItem = nullptr;
    int selectedRow = -1;
    int selectedParentRow = -1;
    QKeySequence editingKeySequence;
    QString keySequenceToEditString(const QKeySequence &sequence) const;
    void initTreeView();
    void createMappingModel();
    void onTreeDoubleClicked(QModelIndex index);
    void onTreeClicked(QModelIndex itemIndex);
    void selectItem(LC_ShortcutTreeItem *item, int selectedRow, int parentRow);
    void editItem(LC_ShortcutTreeItem *item);
    bool keySequenceIsValid(const QKeySequence &sequence) const;
    void rebuildModel(bool restoreSelection);
    static QFlags<QFileDialog::Option> getFileDialogOptions();
    static void showIOInfoDialog(bool forImport, bool ok, const QString &message);
    bool checkHasCollisions(LC_ShortcutInfo *shortcutInfo);
    void applyRecordedKeySequence();
    void reportSaveResult(int saveResult) const;
    void reportLoadResult(int loadResult) const;

    void doSelectItem(const QModelIndex &itemIndex);
};

#endif // LC_ACTIONSSHORTCUTSDIALOG_H
