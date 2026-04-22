/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_dlg_widget_creator.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <locale>

#include "lc_actiongroup.h"
#include "lc_actiongroupmanager.h"
#include "lc_dlg_menu_assigner.h"
#include "lc_dlg_new_widget.h"
#include "lc_menuactivator.h"
#include "lc_settingsexporter.h"
#include "rs_settings.h"
#include "ui_lc_dlg_widget_creator.h"

namespace {
    const QString g_menuNameSeparator = " | ";
}

LC_DlgWidgetCreator::LC_DlgWidgetCreator(QWidget *parent, const bool forMenu, LC_ActionGroupManager* actionGroupManager)
    : LC_Dialog(parent, forMenu ? "MenuCreator" : "ToolbarCreator")
    , ui(new Ui::LC_DlgWidgetCreator), m_forMenu{forMenu}, m_actionGroupManager{actionGroupManager}{
    ui->setupUi(this);

    ui->gbMenuAssignment->setVisible(forMenu);
    ui->gbToolbarPlacement->setVisible(!forMenu);

    const bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
    ui->btnDelete->setAutoRaise(autoRaiseButtons);
    ui->btnSave->setAutoRaise(autoRaiseButtons);
    ui->btnNew->setAutoRaise(autoRaiseButtons);

    if (forMenu) {
        setWindowTitle(tr("Custom Menu Creator"));
        ui->lblWidgetTypeName->setText(tr("Menu Name:"));
        ui->btnDelete->setToolTip(tr("Destroy Menu"));
        ui->btnNew->setToolTip(tr("New Menu"));
        ui->cbWidgetName->setToolTip(tr("Name of Custom Nenu"));
        ui->lblDialogHint->setText(tr("Define a custom menu by specifying the set of used actions. "
                  "Menu will be shown as a popup in the drawing area as soon as menu invocation shortcut is invoked. "));
        connect(ui->pbMenuAssign, &QPushButton::clicked, this, &LC_DlgWidgetCreator::onAssignMenu);
        connect(ui->pbMenuUnassign, &QPushButton::clicked, this, &LC_DlgWidgetCreator::onUnAssignMenu);

        ui->pbExportToFile->setToolTip(tr("Export custom menus setup to file"));
        ui->pbImportFromFile->setToolTip(tr("Import custom  menus setup from file"));
    }
    else {
        setWindowTitle(tr("Custom Toolbar Creator"));
        ui->lblWidgetTypeName->setText(tr("Toolbar Name:"));
        ui->btnDelete->setToolTip(tr("Destroy toolbar"));
        ui->btnNew->setToolTip(tr("New Toolbar"));
        ui->cbWidgetName->setToolTip(tr("Name of Custom Toolbar"));
        ui->lblDialogHint->setText(tr("Define a custom toolbar by specifying the set of used actions. "
              "Once created, the custom toolbar will behave exactly as built-in ones."));

        int area = LC_GET_ONE_INT("Defaults", "NewToolBarArea", 0);
        if (area < 0 || area > 3) {
            area = 0;
        }
        ui->cbTBPlacementArea->setCurrentIndex(area);

        ui->pbExportToFile->setToolTip(tr("Export custom toolbars setup to file"));
        ui->pbImportFromFile->setToolTip(tr("Import custom  toolbars setup from file"));

        connect(ui->cbTBPlacementArea, &QComboBox::currentIndexChanged, this, &LC_DlgWidgetCreator::onToolbarPlacementIndexChanged);
    }

    connect(ui->pbExportToFile, &QPushButton::clicked, this, &LC_DlgWidgetCreator::exportConfiguration);
    connect(ui->pbImportFromFile, &QPushButton::clicked, this, &LC_DlgWidgetCreator::importConfiguration);

    ui->cbCategories->setCurrentIndex(-1);
    ui->cbCategories->addItem(QObject::tr("All"), "_all");
    foreach (auto ag, m_actionGroupManager->allGroupsList()) {
        ui->cbCategories->addItem(ag->getIcon(), ag->getTitle(), ag->getName());
    }

    ui->alChosenActions->setDragDropMode(QAbstractItemView::InternalMove);
    ui->alOfferredActions->setSortingEnabled(true);

    connect(ui->cbCategories, &QComboBox::activated, this, &LC_DlgWidgetCreator::onCategoryActivated);

    connect(ui->btnAdd, &QPushButton::released, this, &LC_DlgWidgetCreator::addChosenAction);
    connect(ui->btnAddSeparator, &QPushButton::released, this, &LC_DlgWidgetCreator::addSeparator);
    connect(ui->btnRemove,  &QPushButton::released, this, &LC_DlgWidgetCreator::removeChosenAction);
    connect(ui->btnRemoveAll,  &QPushButton::released, this, &LC_DlgWidgetCreator::removeAllChosenActions);

    connect(ui->alOfferredActions, &ActionList::itemDoubleClicked,this, &LC_DlgWidgetCreator::addChosenActionForItem);
    connect(ui->alChosenActions, &ActionList::itemDoubleClicked,this, &LC_DlgWidgetCreator::removeChosenActionForItem);

    connect(ui->cbWidgetName, &QComboBox::currentIndexChanged, this, &LC_DlgWidgetCreator::loadWidgetActions);

    connect(ui->btnNew, &QToolButton::released, this, &LC_DlgWidgetCreator::newWidget);
    connect(ui->btnSave, &QToolButton::released, this, &LC_DlgWidgetCreator::saveWidget);
    connect(ui->btnDelete, &QToolButton::released, this, &LC_DlgWidgetCreator::destroyWidget);

    reloadCustomWidgets();
}

LC_DlgWidgetCreator::~LC_DlgWidgetCreator(){
    delete ui;
    qDeleteAll(m_menuActivators);
}

void LC_DlgWidgetCreator::reloadCustomWidgets() {
    setCategoryAll();
    loadCustomWidgets();

    ui->cbWidgetName->setCurrentIndex(0);
    updateSaveAndDeleteButtons();
}

void LC_DlgWidgetCreator::updateSaveAndDeleteButtons() const {
    const int widgetsCount = ui->cbWidgetName->count();
    const auto hasItems = widgetsCount > 0;
    ui->btnDelete->setEnabled(hasItems);
    ui->btnSave->setEnabled(hasItems);
}

void LC_DlgWidgetCreator::setCategoryAll() const {
    ui->cbCategories->setCurrentIndex(0);
    onCategoryActivated(0);
}

void LC_DlgWidgetCreator::addChosenAction() const {
    const QListWidgetItem *item = ui->alOfferredActions->currentItem();
    if (item != nullptr) {
        ui->alChosenActions->addItem(item->clone());
        delete item;
    }
}

void LC_DlgWidgetCreator::addSeparator() const {
    const auto item = new QListWidgetItem("----------", nullptr);
    item->setWhatsThis("");
    ui->alChosenActions->addItem(item);
}

void LC_DlgWidgetCreator::addChosenActionForItem(const QListWidgetItem *item) const {
    ui->alChosenActions->addItem(item->clone());
    delete item;
}

void LC_DlgWidgetCreator::removeAllChosenActions() const {
    const int count = ui->alChosenActions->count();
    for (int i = count - 1; i >= 0; i--) {
        const auto item = ui->alChosenActions->item(i);
        removeChosenActionForItem(item);
    }
}

void LC_DlgWidgetCreator::removeChosenAction() const {
    QListWidgetItem *item = ui->alChosenActions->currentItem();
    if (item != nullptr) {
        removeChosenActionForItem(item);
    }
}

void LC_DlgWidgetCreator::removeChosenActionForItem(QListWidgetItem *item) const {
    if (!item->whatsThis().isEmpty()){
        const auto categoryData = ui->cbCategories->currentData(Qt::UserRole);
        const QString category = categoryData.toString();
        bool mayAddToOfferedActions = false;
        if (category == "_all") {
            mayAddToOfferedActions = true;
        }
        else {
            const auto itemCategoryData = item->data(Qt::UserRole);
            const auto itemCategory = itemCategoryData.toString();
            if (category == itemCategory) {
                mayAddToOfferedActions = true;
            }
        }
        if (mayAddToOfferedActions) {
            // here we try to check that there is no such item in the list of offered actions - so we avoid duplication
            // that may occur after reloading list of offered actions and chosen actions.
            // NOTE:: this means that unique text (action names) is expected!!!
            const auto existingItems = ui->alOfferredActions->findItems(item->text(), Qt::MatchExactly);
            if (existingItems.isEmpty()) {
                ui->alOfferredActions->addItem(item->clone());
            }
        }
    }

    const int row =  ui->alChosenActions->row(item);
    if (row > 0) {
        ui->alChosenActions->removeItemWidget(item);
    }
    delete item;
}

void LC_DlgWidgetCreator::onUnAssignMenu([[maybe_unused]]bool checked) {
    const int menuIndex = ui->cbWidgetName->currentIndex();
    if (menuIndex == -1) {
        return;
    }
    const QString menuName = ui->cbWidgetName->currentData().toString();
    const LC_MenuActivator* activator = findMenuActivator(menuName);
    if (activator != nullptr) {
        const auto questionResult = QMessageBox::question(this, tr("Unassign menu"),
                                                    tr(
                                                        "Are you sure you'd like to unassign \"%1\" menu? Note: Just an invocation shortcut will be "
                                                        "removed and menu will not be deleted.").arg(menuName),
                                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (QMessageBox::Yes == questionResult) {
            LC_GROUP("Activators");
            LC_REMOVE(activator->getShortcut());
            LC_GROUP_END();

            const QString newDisplayName = createMenuItemDisplayName(menuName, nullptr);
            ui->cbWidgetName->setItemText(menuIndex, newDisplayName);
            removeMenuActivator(menuName);

            ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
            ui->pbMenuUnassign->setEnabled(false);
        }
    }
}

void LC_DlgWidgetCreator::onAssignMenu([[maybe_unused]]bool checked) {
    const int menuIndex = ui->cbWidgetName->currentIndex();
    if (menuIndex != -1) {
        const QString menuName = ui->cbWidgetName->currentData().toString();
        if (menuName.isEmpty()) {
            return;
        }
        LC_MenuActivator* activator = findMenuActivator(menuName);
        LC_MenuActivator* copyToEdit = nullptr;
        if (activator == nullptr) {
            copyToEdit = new LC_MenuActivator();
        }
        else {
            copyToEdit = activator->getCopy();
        }
        copyToEdit->setMenuName(menuName);

        auto* dlgMenuAssigner = new LC_DlgMenuAssigner(this, copyToEdit, &m_menuActivators);
        if (dlgMenuAssigner->exec() == Accepted) {
            copyToEdit->update();
            const QString shortcutView = copyToEdit->getShortcutView();
            ui->lblMenuShortcut->setText(shortcutView);

            const QString newDisplayName = createMenuItemDisplayName(menuName, copyToEdit);

            LC_GROUP("Activators");
            const auto shortcut = copyToEdit->getShortcut();
            if (activator != nullptr) {
                const auto originalShortcut = activator->getShortcut();
                LC_REMOVE(originalShortcut);
                LC_SET(shortcut, menuName);
                copyToEdit->copyTo(*activator);
                delete copyToEdit;
            }
            else {
                LC_SET(shortcut, menuName);
                m_menuActivators.push_back(copyToEdit);
            }

            LC_GROUP_END();

            auto it = m_menuActivators.begin();
            // unassign other menu if there is shortcut override, so ensure that only one is valid
            while (it != m_menuActivators.end()) {
                const auto value = *it;
                auto oldMenuName = value->getMenuName();
                if (value->getShortcut() == shortcut && oldMenuName != menuName) {
                    m_menuActivators.erase(it);
                    delete value;
                    break;
                }
                ++it;
            }
            // update display names for menus in combobox
            const int count = ui->cbWidgetName->count();
            for (int i = 0; i < count; ++i) {
                const QString  itemMenuName = ui->cbWidgetName->itemData(i).toString();
                QString displayName = createMenuItemDisplayName(itemMenuName);
                ui->cbWidgetName->setItemText(i, displayName);
            }

            ui->cbWidgetName->setItemText(menuIndex, newDisplayName);
            ui->pbMenuUnassign->setEnabled(true);
        }
        else {
            delete copyToEdit;
        }
    }
}

void LC_DlgWidgetCreator::onCategoryActivated(const int index) const {
    const auto categoryData = ui->cbCategories->itemData(index, Qt::UserRole);
    const QString category = categoryData.toString();
    if (category == "_all") {
        auto actionMap = m_actionGroupManager->getActionsMap();
        ui->alOfferredActions->clear();
        ui->alOfferredActions->fromActionMap(actionMap);
        return;
    }

    if (!m_actionGroupManager->hasActionGroup(category)) {
        return;
    }

    ui->alOfferredActions->clear();
    const auto chosenActions = getChosenActionNames();
    const auto actionGroup = m_actionGroupManager->getActionGroup(category);
    if (actionGroup != nullptr) {
        for (const auto action: actionGroup->actions()) {
            if (!chosenActions.contains(action->objectName())) {
                ui->alOfferredActions->addActionItem(action);
            }
        }
    }
}

void LC_DlgWidgetCreator::onWidgetNameIndexChanged(const int index) {
    loadWidgetActions(index);
}

void LC_DlgWidgetCreator::onToolbarPlacementIndexChanged(const int index) {
    LC_SET_ONE("Defaults", "NewToolBarArea", index);
}

void LC_DlgWidgetCreator::loadWidgetActions(const int index) {
    const QString key = ui->cbWidgetName->itemData(index).toString();
    // w_key = key;
    const QSettings settings;
    const auto widget = QString("%1/%2").arg(getSettingsGroupName()).arg(key);
    QStringList actionNamesList = settings.value(widget).toStringList();

    if (actionNamesList.isEmpty()) {
        return;
    }

    if (m_forMenu) {
        ui->pbMenuAssign->setEnabled(true);
    }

    ui->btnDelete->setEnabled(true);

    ui->alChosenActions->clear();
    ui->alOfferredActions->clear();

    foreach(auto str, actionNamesList) {
        if ("" == str){
            addSeparator();
        }
        else{
            const QAction* action = m_actionGroupManager->getActionByName(str);
            if (action != nullptr) {
                ui->alChosenActions->addActionItem(action);
            }
        }
    }
    setCategoryAll();

    if (m_forMenu) {
        const auto activator = findMenuActivator(key);
        if (activator == nullptr) {
            ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
            ui->pbMenuUnassign->setEnabled(false);
        }
        else {
            const QString shortCut = activator->getShortcutView();
            ui->lblMenuShortcut->setText(shortCut);
            ui->pbMenuUnassign->setEnabled(true);
        }
    }
}
void LC_DlgWidgetCreator::newWidget() {
    bool clearActionsList = false;
    QStringList  widgetsList;
    const int count = ui->cbWidgetName->count();
    for (int i= 0; i < count;i++) {
        QString name = ui->cbWidgetName->itemData(i).toString();
        widgetsList.push_back(name);
    }
    const QString newName = LC_DlgNewWidget::askForNewWidgetName(this, m_forMenu, &widgetsList, clearActionsList);
    if (!newName.isEmpty()) {
         if (clearActionsList) {
             ui->alChosenActions->clear();
             setCategoryAll();
         }
        QString displayName = newName;
        if (m_forMenu) {
            displayName = createMenuItemDisplayName(newName, nullptr);
        }
        ui->cbWidgetName->addItem(displayName, newName);
        const int idx = ui->cbWidgetName->count() - 1;
        ui->cbWidgetName->setCurrentIndex(idx);
        ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
        ui->pbMenuUnassign->setEnabled(false);
    }
    updateSaveAndDeleteButtons();
}

void LC_DlgWidgetCreator::saveWidget() {
    const int index = ui->cbWidgetName->currentIndex();
    if (index == -1) {
        return;
    }
    QString widgetName = ui->cbWidgetName->currentText();
    if (m_forMenu) {
        widgetName = ui->cbWidgetName->currentData().toString();
    }

    const QStringList chosenActionNamesList = getChosenActionNames();
    if (!chosenActionNamesList.isEmpty() && !widgetName.isEmpty()) {
        QSettings settings;
        const auto widget = QString("%1/%2").arg(getSettingsGroupName()).arg(widgetName);
        settings.setValue(widget, chosenActionNamesList);
        const int areaIndex = ui->cbTBPlacementArea->currentIndex();
        emit widgetCreationRequest(widgetName, chosenActionNamesList, areaIndex);
    }
}


void LC_DlgWidgetCreator::destroyWidget() {
    const int index = ui->cbWidgetName->currentIndex();
    if (index == -1) {
        return;
    }
    auto widgetName = ui->cbWidgetName->currentText();
    QString widgetTypeName;
    if (m_forMenu) {
        widgetName = ui->cbWidgetName->currentData().toString();
        widgetTypeName = tr("menu");
    }
    else {
        widgetTypeName = tr("toolbar");
    }

    if (QMessageBox::Yes == QMessageBox::question(this, tr("Remove %1").arg(widgetTypeName),
        tr("Are you sure you'd like to remove  %2\"%1\"?").arg(widgetName).arg(widgetTypeName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {

        ui->cbWidgetName->removeItem(index);
        QSettings settings;
        settings.remove(QString("%1/%2").arg(getSettingsGroupName()).arg(widgetName));
        if (m_forMenu) {
            removeMenuActivator(widgetName);
        }
        emit widgetDestroyRequest(widgetName);
        ui->alChosenActions->clear();
    }
    updateSaveAndDeleteButtons();
}

QStringList LC_DlgWidgetCreator::getChosenActionNames() const {
    QStringList sList;
    for (int i = 0; i < ui->alChosenActions->count(); ++i) {
        const auto listWidgetItem = ui->alChosenActions->item(i);
        sList << listWidgetItem->whatsThis();
    }
    return sList;
}

QString LC_DlgWidgetCreator::getSettingsGroupName() const {
    return m_forMenu ? "CustomMenus" : "CustomToolbars";
}

QString LC_DlgWidgetCreator::createMenuItemDisplayName(const QString& key, const LC_MenuActivator* activator) {
    QString message = key;
    if (activator == nullptr) {
        message.append(g_menuNameSeparator).append(tr("NOT ASSIGNED"));
    }
    else {
        message.append(g_menuNameSeparator).append(activator->getShortcutView());
    }
    return message;
}

QString LC_DlgWidgetCreator::createMenuItemDisplayName(const QString& key) {
    const auto activator = findMenuActivator(key);
    return createMenuItemDisplayName(key, activator);
}

void LC_DlgWidgetCreator::loadCustomWidgets() {
    if (m_forMenu) {
        LC_GROUP("Activators");
        qDeleteAll(m_menuActivators);

        auto activators = LC_CHILD_KEYS();
        for (const auto &key : std::as_const(activators)) {
            LC_MenuActivator* activator = LC_MenuActivator::fromShortcut(key);
            if (activator != nullptr) {
                QString menuName = LC_GET_STR(key);
                activator->setMenuName(menuName);
                m_menuActivators.push_back(activator);
            }
        }
        LC_GROUP_END();

        LC_GROUP(getSettingsGroupName());
        auto widgets = LC_CHILD_KEYS();

        std::vector<std::pair<QString, LC_MenuActivator*>> activatorsList;
        for (auto key : std::as_const(widgets)) {
            auto activator = findMenuActivator(key);
            std::pair<QString, LC_MenuActivator*> pair(key, activator);
            activatorsList.push_back(pair);
        }

        // sort by event, so activators for different entities yet for the same event will be located
        // together in the list
        std::sort(activatorsList.begin(), activatorsList.end(),
              [](const std::pair<QString, LC_MenuActivator*>& a, const std::pair<QString, LC_MenuActivator*>& b) {
                  const LC_MenuActivator* a1 = a.second;
                  const LC_MenuActivator* a2 = b.second;
                  if (a1 == nullptr) {
                      if (a2 != nullptr) {
                          return true;
                      }
                      return false;
                  }
                  if (a2 == nullptr) {
                      return false;
                  }
                  const auto event1 = a1->getEventView();
                  const auto event2 = a2->getEventView();
                  if (event1 == event2) {
                      const bool ent1 = a1->isEntityRequired();
                      const bool ent2 = a2->isEntityRequired();
                      if (ent1 != ent2) {
                          return ent1;
                      }
                      return a1->getEntityType() < a2->getEntityType();
                  }
                  return event1 < event2;
              });

        for (const auto & [key, activator]: activatorsList) {
            QString message = createMenuItemDisplayName(key, activator);
            ui->cbWidgetName->addItem(message, key);

        }
        LC_GROUP_END();
    }
    else {
        LC_GROUP(getSettingsGroupName());
        ui->cbWidgetName->clear();
        auto widgets = LC_CHILD_KEYS();

        for (const auto &key : std::as_const(widgets)) {
            ui->cbWidgetName->addItem(key, key);
        }
        LC_GROUP_END();
    }
}

LC_MenuActivator* LC_DlgWidgetCreator::findMenuActivator(const QString& menuName) {
    for (const auto a: std::as_const(m_menuActivators)) {
        if (a->getMenuName() == menuName) {
            return a;
        }
    }
    return nullptr;
}

void LC_DlgWidgetCreator::removeMenuActivator(const QString& menuName) {
    const auto activator = findMenuActivator(menuName);
    if (activator != nullptr) {
        m_menuActivators.removeOne(activator);
        LC_GROUP("Activators");
        LC_REMOVE(activator->getShortcut());
        LC_GROUP_END();

        delete activator;
    }
}

void LC_DlgWidgetCreator::exportConfiguration() {
    LC_SettingsExporter exporter;
    exporter.exportCustomWidgetSettings(this, m_forMenu);
}

void LC_DlgWidgetCreator::importConfiguration() {
    LC_SettingsExporter exporter;
    exporter.importCustomWidgetSettings(this, m_forMenu);
    reloadCustomWidgets();
}
