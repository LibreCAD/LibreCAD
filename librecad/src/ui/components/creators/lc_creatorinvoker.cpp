/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024
 Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)

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

#include "lc_creatorinvoker.h"

#include <QMenu>
#include <QMouseEvent>
#include <QSettings>
#include <QToolBar>
#include <QVBoxLayout>

#include "lc_actiongroupmanager.h"
#include "lc_dialog.h"
#include "lc_dlgmenuassigner.h"
#include "lc_dlgwidgetcreator.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_settings.h"

LC_CreatorInvoker::LC_CreatorInvoker(QC_ApplicationWindow *appWin, LC_ActionGroupManager *actionGroupManager)
    :m_appWindow{appWin}, m_actionGroupManager(actionGroupManager) {
    loadMenuActivators();
}

void createCustomMenuForFirstRunIfNeeded() {
    bool firstLoad = LC_GET_BOOL("FirstLoad", true);
    if (firstLoad) {
        QStringList list;
        list << "ZoomAuto";
        QSettings settings;
        auto menuName = "AutoZoom";
        auto key = QString("CustomMenus/%1").arg(menuName);
        settings.setValue(key, list);

        LC_MenuActivator zoomActivator("",false, false, false, LC_MenuActivator::MIDDLE, LC_MenuActivator::DOUBLE_CLICK, false, RS2::EntityUnknown);
        zoomActivator.update();
        auto shortcut = zoomActivator.getShortcut();
        auto activatorKey = QString("Activators/%1").arg(shortcut);

        settings.setValue(activatorKey, menuName);
    }
}

void LC_CreatorInvoker::createCustomToolbars(bool showToolTips) {
    m_showToolbarTooltips = showToolTips;
    QSettings settings;
    settings.beginGroup("CustomToolbars");
    const QStringList &customToolbars = settings.childKeys();

    for (const QString& key : customToolbars) {
        QList<QAction*> actionsList;
        auto actionNames = settings.value(key).toStringList();
        for (const QString& actionName : actionNames) {
            QAction* action = getAction(actionName);
            if (action != nullptr)
                actionsList.push_back(getAction(actionName));
        }
        if (!actionsList.empty()) {
            auto* toolbar = new QToolBar(key, m_appWindow);
            toolbar->setObjectName(key);
            if (m_showToolbarTooltips) {
                toolbar->setToolTip(tr("Toolbar: %1 (Custom)").arg(key));
            }
            toolbar->addActions(actionsList);
            m_appWindow->addToolBar(toolbar);
        }
    }

    settings.endGroup();

    createCustomMenuForFirstRunIfNeeded();
}

void LC_CreatorInvoker::invokeToolbarCreator() {
    auto dlg = LC_DlgWidgetCreator(m_appWindow, false, m_actionGroupManager);

    connect(&dlg, &LC_DlgWidgetCreator::widgetCreationRequest, this, &LC_CreatorInvoker::createToolbar);
    connect(&dlg, &LC_DlgWidgetCreator::widgetDestroyRequest, this, &LC_CreatorInvoker::destroyToolbar);

    dlg.exec();
    dlg.deleteLater();
}

void LC_CreatorInvoker::createToolbar(const QString &toolbar_name, const QStringList& actionNames, int areaIndex) {
    auto toolbar = m_appWindow->findChild<QToolBar *>(toolbar_name);

    if (toolbar) {
        toolbar->clear();
    }
    else {
        toolbar = new QToolBar(toolbar_name, m_appWindow);
        toolbar->setObjectName(toolbar_name);
        if (m_showToolbarTooltips) {
            toolbar->setToolTip(tr("Toolbar: %1 (Custom)").arg(toolbar_name));
        }
        Qt::ToolBarArea area;
        switch (areaIndex) {
            case 0:
                area = Qt::BottomToolBarArea;
                break;
            case 1:
                area = Qt::LeftToolBarArea;
                break;
            case 2:
                area = Qt::RightToolBarArea;
                break;
            case 3:
                area = Qt::TopToolBarArea;
                break;
            default:
                area = Qt::BottomToolBarArea;
                break;
        }
        m_appWindow->addToolBar(area, toolbar);
    }

    for(const auto &key: actionNames) {
        toolbar->addAction(getAction(key));
    }
}

void LC_CreatorInvoker::destroyToolbar(const QString &toolbar_name) {
    auto toolbar = m_appWindow->findChild<QToolBar *>(toolbar_name);
    delete toolbar;
}

void LC_CreatorInvoker::invokeMenuCreator() {
    auto dlg = LC_DlgWidgetCreator(m_appWindow, true, m_actionGroupManager);
    dlg.exec();
    loadMenuActivators();
}

bool LC_CreatorInvoker::getMenuActionsForMouseEvent(QMouseEvent* event, RS_Entity* entity, QStringList& actions) {
    LC_MenuActivator* activatorForEntityType {nullptr};
    LC_MenuActivator* activatorForAnyEntity{nullptr};
    LC_MenuActivator* activatorForEitherEntity{nullptr};
    LC_MenuActivator* activatorForNoEntity{nullptr};
    bool hasEntity = entity != nullptr;

    RS2::EntityType entityType = RS2::EntityUnknown;
    if (hasEntity) {
        entityType = entity->rtti();
    }

    for (auto a: m_menuActivators) {
        if (a->isEventApplicable(event)) {
            RS2::EntityType activatorEntityType = a->getEntityType();
            if (a->isEntityRequired()) {
                if (hasEntity) {
                    if (activatorEntityType == RS2::EntityUnknown) {
                        activatorForAnyEntity = a;
                    }
                    else if (activatorEntityType == entityType) {
                        activatorForEntityType = a;
                    }
                    else if (activatorEntityType == RS2::EntityGraphic) {
                        activatorForEitherEntity = a;
                    }
                }
                else {
                    if (activatorEntityType == RS2::EntityGraphic) {
                        activatorForEitherEntity = a;
                    }
                }
            }
            else {
                if (hasEntity) {
                    continue;
                }
                else {
                    activatorForNoEntity = a;
                }
            }
        }
    }

    LC_MenuActivator*  activatorToUse {nullptr};

    if (hasEntity) {
        if (activatorForEntityType != nullptr) {
            activatorToUse = activatorForEntityType;
        }
        else if (activatorForAnyEntity != nullptr) {
            activatorToUse = activatorForAnyEntity;
        }
        else if (activatorForEitherEntity != nullptr) {
            activatorToUse = activatorForEitherEntity;
        }
    }
    else {
        if (activatorForNoEntity != nullptr) {
            activatorToUse = activatorForNoEntity;
        }
        else if (activatorForEitherEntity != nullptr) {
            activatorToUse = activatorForEitherEntity;
        }
    }

    bool mayInvokeDefaultMenu = false;

    if (activatorToUse == nullptr) {
        if (isDefaultMenuInvokerEvent(event)) {
            mayInvokeDefaultMenu = true;
        }
    }
    else {
        QString menuName = activatorToUse->getMenuName();

        QSettings settings;
        auto widget = QString("CustomMenus/%1").arg(menuName);
        auto value = settings.value(widget);
        if (value.isValid()) {
            QStringList s_list = value.toStringList();
            actions.clear();
            actions.append(s_list);
        }
    }
    return mayInvokeDefaultMenu;
}

bool LC_CreatorInvoker::isDefaultMenuInvokerEvent(QMouseEvent* event) {
    return event->modifiers() == Qt::NoModifier && event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonRelease;
}

void LC_CreatorInvoker::loadMenuActivators() {
    if (!m_menuActivators.empty()) {
        qDeleteAll(m_menuActivators);
        m_menuActivators.clear();
    }

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
}

QAction *LC_CreatorInvoker::getAction(const QString &key) {
    return m_actionGroupManager->getActionByName(key);
}
