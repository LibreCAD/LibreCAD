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

#include "lc_dlgwidgetcreator.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <locale>

#include "lc_actiongroup.h"
#include "lc_actiongroupmanager.h"
#include "lc_creatorinvoker.h"
#include "lc_dlgmenuassigner.h"
#include "lc_dlgnewwidget.h"
#include "lc_menuactivator.h"
#include "rs_settings.h"
#include "ui_lc_dlgwidgetcreator.h"

namespace {
    const QString g_menuNameSeparator = " | ";
}

LC_DlgWidgetCreator::LC_DlgWidgetCreator(QWidget *parent, bool forMenu, LC_ActionGroupManager* actionGroupManager)
    : LC_Dialog(parent, forMenu ? "MenuCreator" : "ToolbarCreator")
    , ui(new Ui::LC_DlgWidgetCreator), m_forMenu{forMenu}, m_actionGroupManager{actionGroupManager}{
    ui->setupUi(this);

    ui->gbMenuAssignment->setVisible(forMenu);
    ui->gbToolbarPlacement->setVisible(!forMenu);

    bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
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

        connect(ui->cbTBPlacementArea, &QComboBox::currentIndexChanged, this, &LC_DlgWidgetCreator::onToolbarPlacementIndexChanged);
    }

    ui->cbCategories->setCurrentIndex(-1);
    ui->cbCategories->addItem(QObject::tr("All"), "_all");
    foreach (auto ag, m_actionGroupManager->allGroupsList()) {
        ui->cbCategories->addItem(ag->getIcon(), ag->getTitle(), ag->getName());
    }

    ui->alChosenActions->setDragDropMode(QAbstractItemView::InternalMove);
    ui->alOfferredActions->setSortingEnabled(true);

    connect(ui->cbCategories, &QComboBox::activated, this, &LC_DlgWidgetCreator::onCategoryActivated);

    connect(ui->btnAdd, &QPushButton::released, this, &LC_DlgWidgetCreator::addChosenAction);
    connect(ui->btnRemove,  &QPushButton::released, this, &LC_DlgWidgetCreator::removeChosenAction);
    connect(ui->btnRemoveAll,  &QPushButton::released, this, &LC_DlgWidgetCreator::removeAllChosenActions);

    connect(ui->alOfferredActions, &ActionList::itemDoubleClicked,this, &LC_DlgWidgetCreator::addChosenActionForItem);
    connect(ui->alChosenActions, &ActionList::itemDoubleClicked,this, &LC_DlgWidgetCreator::removeChosenActionForItem);

    connect(ui->cbWidgetName, &QComboBox::currentIndexChanged, this, &LC_DlgWidgetCreator::loadWidgetActions);

    connect(ui->btnNew, &QToolButton::released, this, &LC_DlgWidgetCreator::newWidget);
    connect(ui->btnSave, &QToolButton::released, this, &LC_DlgWidgetCreator::saveWidget);
    connect(ui->btnDelete, &QToolButton::released, this, &LC_DlgWidgetCreator::destroyWidget);

    setCategoryAll();
    loadCustomWidgets();

    ui->cbWidgetName->setCurrentIndex(0);
    updateSaveAndDeleteButtons();
}

void LC_DlgWidgetCreator::updateSaveAndDeleteButtons() {
    int widgetsCount = ui->cbWidgetName->count();
    auto hasItems = widgetsCount > 0;
    ui->btnDelete->setEnabled(hasItems);
    ui->btnSave->setEnabled(hasItems);
}

LC_DlgWidgetCreator::~LC_DlgWidgetCreator(){
    delete ui;
}

void LC_DlgWidgetCreator::setCategoryAll() {
    ui->cbCategories->setCurrentIndex(0);
    onCategoryActivated(0);
}


void LC_DlgWidgetCreator::addChosenAction() {
    QListWidgetItem *item = ui->alOfferredActions->currentItem();
    if (item) {
        ui->alChosenActions->addItem(item->clone());
        delete item;
    }
}

void LC_DlgWidgetCreator::addChosenActionForItem(QListWidgetItem *item) {
    ui->alChosenActions->addItem(item->clone());
    delete item;
}

void LC_DlgWidgetCreator::removeAllChosenActions() {
    int count = ui->alChosenActions->count();
    for (int i = count - 1; i >= 0; i--) {
        auto item = ui->alChosenActions->item(i);
        removeChosenActionForItem(item);
    }
}

void LC_DlgWidgetCreator::removeChosenAction() {
    QListWidgetItem *item = ui->alChosenActions->currentItem();
    if (item != nullptr) {
        removeChosenActionForItem(item);
    }
}

void LC_DlgWidgetCreator::removeChosenActionForItem(QListWidgetItem *item) {
    auto categoryData = ui->cbCategories->currentData(Qt::UserRole);
    QString category = categoryData.toString();
    bool mayAddToOfferedActions = false;
    if (category == "_all") {
        mayAddToOfferedActions = true;
    }
    else {
        auto itemCategoryData = item->data(Qt::UserRole);
        auto itemCategory = itemCategoryData.toString();
        if (category == itemCategory) {
            mayAddToOfferedActions = true;
        }
    }
    if (mayAddToOfferedActions) {
        // here we try to check that there is no such item in the list of offered actions - so we avoid duplication
        // that may occur after reloading list of offered actions and chosen actions.
        // NOTE:: this means that unique text (action names) is expected!!!
        auto existingItems = ui->alOfferredActions->findItems(item->text(), Qt::MatchExactly);
        if (existingItems.isEmpty()) {
            ui->alOfferredActions->addItem(item->clone());
        }
    }

    int row =  ui->alChosenActions->row(item);
    if (row > 0) {
        ui->alChosenActions->removeItemWidget(item);
    }
    delete item;
}

void LC_DlgWidgetCreator::onUnAssignMenu([[maybe_unused]]bool checked) {
    int menuIndex = ui->cbWidgetName->currentIndex();
    if (menuIndex == -1) {
        return;
    }
    QString menuName = ui->cbWidgetName->currentData().toString();
    LC_MenuActivator* activator = findMenuActivator(menuName);
    if (activator != nullptr) {
        auto questionResult = QMessageBox::question(this, tr("Unassign menu"),
                                                    tr(
                                                        "Are you sure you'd like to unassign \"%1\" menu? Note: Just an invocation shortcut will be "
                                                        "removed and menu will not be deleted.").arg(menuName),
                                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (QMessageBox::Yes == questionResult) {
            LC_GROUP("Activators");
            LC_REMOVE(activator->getShortcut());
            LC_GROUP_END();

            QString newDisplayName = createMenuItemDisplayName(menuName, nullptr);
            ui->cbWidgetName->setItemText(menuIndex, newDisplayName);
            removeMenuActivator(menuName);

            ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
            ui->pbMenuUnassign->setEnabled(false);
        }
    }
}

void LC_DlgWidgetCreator::onAssignMenu([[maybe_unused]]bool checked) {
    int menuIndex = ui->cbWidgetName->currentIndex();
    if (menuIndex != -1) {
        QString menuName = ui->cbWidgetName->currentData().toString();
        if (menuName.isEmpty()) {
            return;
        }
        LC_MenuActivator* activator = findMenuActivator(menuName);
        LC_MenuActivator* copyToEdit;
        if (activator == nullptr) {
            copyToEdit = new LC_MenuActivator();
        }
        else {
            copyToEdit = activator->getCopy();
        }
        copyToEdit->setMenuName(menuName);

        auto* dlgMenuAssigner = new LC_DlgMenuAssigner(this, copyToEdit, &m_menuActivators);
        if (dlgMenuAssigner->exec() == QDialog::Accepted) {
            copyToEdit->update();
            QString shortcutView = copyToEdit->getShortcutView();
            ui->lblMenuShortcut->setText(shortcutView);

            QString newDisplayName = createMenuItemDisplayName(menuName, copyToEdit);

            LC_GROUP("Activators");
            auto shortcut = copyToEdit->getShortcut();
            if (activator != nullptr) {
                auto originalShortcut = activator->getShortcut();
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
                auto value = *it;
                auto oldMenuName = value->getMenuName();
                if (value->getShortcut() == shortcut && oldMenuName != menuName) {
                    m_menuActivators.erase(it);
                    delete value;
                    break;
                }
                ++it;
            }
            // update display names for menus in combobox
            int count = ui->cbWidgetName->count();
            for (int i = 0; i < count; ++i) {
                QString  itemMenuName = ui->cbWidgetName->itemData(i).toString();
                QString displayName = createMenuItemDisplayName(itemMenuName);
                ui->cbWidgetName->setItemText(i, displayName);
            }

            ui->cbWidgetName->setItemText(menuIndex, newDisplayName);
            ui->pbMenuUnassign->setEnabled(true);
        }
        else {
            delete copyToEdit;
        };
    }
}

void LC_DlgWidgetCreator::onCategoryActivated(int index) {
    auto categoryData = ui->cbCategories->itemData(index, Qt::UserRole);
    QString category = categoryData.toString();
    if (category == "_all") {
        auto a_map = m_actionGroupManager->getActionsMap();
        ui->alOfferredActions->clear();
        ui->alOfferredActions->fromActionMap(a_map);
        return;
    }

    if (!m_actionGroupManager->hasActionGroup(category)) {
        return;
    }

    ui->alOfferredActions->clear();
    auto chosen_actions = getChosenActions();
    auto action_group = m_actionGroupManager->getActionGroup(category);
    if (action_group != nullptr)
        for (auto action: action_group->actions()) {
            if (!chosen_actions.contains(action->objectName())) {
                ui->alOfferredActions->addActionItem(action);
            }
        }
}

void LC_DlgWidgetCreator::onWidgetNameIndexChanged(int index) {
    loadWidgetActions(index);
}

void LC_DlgWidgetCreator::onToolbarPlacementIndexChanged(int index) {
       LC_SET_ONE("Defaults", "NewToolBarArea", index);
}

void LC_DlgWidgetCreator::loadWidgetActions(int index) {
    QString key = ui->cbWidgetName->itemData(index).toString();
    // w_key = key;
    QSettings settings;
    auto widget = QString("%1/%2").arg(getSettingsGroupName()).arg(key);
    QStringList s_list = settings.value(widget).toStringList();

    if (s_list.isEmpty()) {
        return;
    }

    if (m_forMenu) {
        ui->pbMenuAssign->setEnabled(true);
    }

    ui->btnDelete->setEnabled(true);

    ui->alChosenActions->clear();
    ui->alOfferredActions->clear();

    foreach(auto str, s_list) {
        QAction* action = m_actionGroupManager->getActionByName(str);
        if (action != nullptr) {
            ui->alChosenActions->addActionItem(action);
        }
    }
    setCategoryAll();

    if (m_forMenu) {
        auto activator = findMenuActivator(key);
        if (activator == nullptr) {
            ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
            ui->pbMenuUnassign->setEnabled(false);
        }
        else {
            QString shortCut = activator->getShortcutView();
            ui->lblMenuShortcut->setText(shortCut);
            ui->pbMenuUnassign->setEnabled(true);
        }
    }
}
void LC_DlgWidgetCreator::newWidget() {
    bool clearActionsList = false;
    QStringList  widgetsList;
    int count = ui->cbWidgetName->count();
    for (int i= 0; i < count;i++) {
        QString name = ui->cbWidgetName->itemData(i).toString();
        widgetsList.push_back(name);
    }
    QString newName = LC_DlgNewWidget::askForNewWidgetName(this, m_forMenu, &widgetsList, clearActionsList);
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
        int idx = ui->cbWidgetName->count() - 1;
        ui->cbWidgetName->setCurrentIndex(idx);
        ui->lblMenuShortcut->setText(tr("NOT ASSIGNED"));
        ui->pbMenuUnassign->setEnabled(false);
    }
    updateSaveAndDeleteButtons();
}

void LC_DlgWidgetCreator::saveWidget() {
    int index = ui->cbWidgetName->currentIndex();
    if (index == -1) {
        return;
    }
    QString widgetName = ui->cbWidgetName->currentText();
    if (m_forMenu) {
        widgetName = ui->cbWidgetName->currentData().toString();
    }



    QStringList a_list = getChosenActions();
    if (!a_list.isEmpty() && !widgetName.isEmpty()) {
        QSettings settings;
        auto widget = QString("%1/%2").arg(getSettingsGroupName()).arg(widgetName);
        settings.setValue(widget, a_list);
        int areaIndex = ui->cbTBPlacementArea->currentIndex();
        emit widgetCreationRequest(widgetName, a_list, areaIndex);
    }
}

void LC_DlgWidgetCreator::destroyWidget() {
    int index = ui->cbWidgetName->currentIndex();
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

QStringList LC_DlgWidgetCreator::getChosenActions() {
    QStringList s_list;
    for (int i = 0; i < ui->alChosenActions->count(); ++i) {
        s_list << ui->alChosenActions->item(i)->whatsThis();
    }
    return s_list;
}

QString LC_DlgWidgetCreator::getSettingsGroupName() {
    return m_forMenu ? "CustomMenus" : "CustomToolbars";
}

QString LC_DlgWidgetCreator::createMenuItemDisplayName(QString key, LC_MenuActivator* activator) {
    QString message = key;
    if (activator == nullptr) {
        message.append(g_menuNameSeparator).append(tr("NOT ASSIGNED"));
    }
    else {
        message.append(g_menuNameSeparator).append(activator->getShortcutView());
    }
    return message;
}

QString LC_DlgWidgetCreator::createMenuItemDisplayName(QString key) {
    auto activator = findMenuActivator(key);
    return createMenuItemDisplayName(key, activator);
}

void LC_DlgWidgetCreator::loadCustomWidgets() {
    if (m_forMenu) {
        LC_GROUP("Activators");
        auto activators = LC_CHILD_KEYS();

        for (auto key : activators) {
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
        for (auto key : widgets) {
            auto activator = findMenuActivator(key);
            std::pair<QString, LC_MenuActivator*> pair(key, activator);
            activatorsList.push_back(pair);
        }

        // sort by event, so activators for different entities yet for the same event will be located
        // together in the list
        std::sort(activatorsList.begin(), activatorsList.end(),
              [](std::pair<QString, LC_MenuActivator*>& a, std::pair<QString, LC_MenuActivator*>& b) {
                  LC_MenuActivator* a1 = a.second;
                  LC_MenuActivator* a2 = b.second;
                  if (a1 == nullptr) {
                      if (a2 != nullptr) {
                          return true;
                      }
                      else {
                          return false;
                      }
                  }
                  else if (a2 == nullptr) {
                      return false;
                  }
                  else {
                      auto event1 = a1->getEventView();
                      auto event2 = a2->getEventView();
                      if (event1 == event2) {
                          bool ent1 = a1->isEntityRequired();
                          bool ent2 = a2->isEntityRequired();
                          if (ent1 != ent2) {
                              return ent1;
                          }
                          else {
                              return a1->getEntityType() < a2->getEntityType();
                          }
                      }
                      else {
                          return event1 < event2;
                      }
                  }
              });

        for (const auto &p: activatorsList) {
            auto key = p.first;
            auto activator = p.second;
            QString message = createMenuItemDisplayName(key, activator);
            ui->cbWidgetName->addItem(message, key);

        }
        LC_GROUP_END();
    }
    else {
        LC_GROUP(getSettingsGroupName());
        auto widgets = LC_CHILD_KEYS();

        for (auto key : widgets) {
            ui->cbWidgetName->addItem(key, key);
        }
        LC_GROUP_END();
    }
}

LC_MenuActivator* LC_DlgWidgetCreator::findMenuActivator(const QString& menuName) {
    for (auto a: m_menuActivators) {
        if (a->getMenuName() == menuName) {
            return a;
        }
    }
    return nullptr;
}

void LC_DlgWidgetCreator::removeMenuActivator(const QString& menuName) {
    auto activator = findMenuActivator(menuName);
    if (activator != nullptr) {
        m_menuActivators.removeOne(activator);
        LC_GROUP("Activators");
        LC_REMOVE(activator->getShortcut());
        LC_GROUP_END();

        delete activator;
    }
}
