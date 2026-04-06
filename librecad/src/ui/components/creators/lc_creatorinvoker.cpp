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
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "lc_actiongroupmanager.h"
#include "lc_dialog.h"
#include "lc_dlg_menu_assigner.h"
#include "lc_dlg_widget_creator.h"
#include "qc_applicationwindow.h"
#include "qg_graphicview.h"
#include "rs_settings.h"

LC_CreatorInvoker::LC_CreatorInvoker(QC_ApplicationWindow *appWin, LC_ActionGroupManager *actionGroupManager)
    :m_appWindow{appWin}, m_actionGroupManager(actionGroupManager) {
    loadMenuActivators();
}

void createCustomMenuForFirstRunIfNeeded() {
    const bool firstLoad = LC_GET_BOOL("FirstLoad", true);
    if (firstLoad) {
        QStringList list;
        list << "ZoomAuto";
        QSettings settings;
        const auto menuName = "AutoZoom";
        const auto key = QString("CustomMenus/%1").arg(menuName);
        settings.setValue(key, list);

        LC_MenuActivator zoomActivator("", false, false, false, LC_MenuActivator::MIDDLE, LC_MenuActivator::DBL_CLICK, false,
                                       RS2::EntityUnknown);
        zoomActivator.update();
        const auto shortcut = zoomActivator.getShortcut();
        const auto activatorKey = QString("Activators/%1").arg(shortcut);

        settings.setValue(activatorKey, menuName);
    }
}

void LC_CreatorInvoker::createCustomToolbars(const bool showToolTips) {
    m_showToolbarTooltips = showToolTips;
    QSettings settings;
    settings.beginGroup("CustomToolbarsVisibility");
    const QStringList visibleToolbars = settings.value("VisibleList").toStringList();
    settings.endGroup();

    settings.beginGroup("CustomToolbars");
    const QStringList &customToolbars = settings.childKeys();
    for (const QString& key : customToolbars) {
        QList<QAction*> actionsList;
        auto actionNames = settings.value(key).toStringList();
        bool hasActions = false;
        for (const QString& actionName : actionNames) {
            if ("" == actionName){
                actionsList.push_back(nullptr);
            }
            else{
                const QAction* action = getAction(actionName);
                if (action != nullptr){
                    actionsList.push_back(getAction(actionName));
                    hasActions = true;
                }
            }
        }
        if (hasActions) {
            auto* toolbar = new QToolBar(key, m_appWindow);
            toolbar->setObjectName(key);
            if (m_showToolbarTooltips) {
                toolbar->setToolTip(tr("Toolbar: %1 (Custom)").arg(key));
            }
            for (const auto action: actionsList){
                if (action == nullptr){
                    toolbar->addSeparator();
                }
                else{
                    toolbar->addAction(action);
                }
            }
            connect(toolbar, &QToolBar::visibilityChanged, this, &LC_CreatorInvoker::onCustomToolbarVisibilityChanged);
            bool visible = visibleToolbars.contains(key);
            // LC_ERR << "Custom TB creation " << key << (visible ? " VISIBLE" : " NOT_VISIBLE");
            toolbar->setVisible(visible);
            m_appWindow->addToolBar(toolbar);
            // due to some reasons, visibilty TRUE event comes even if toolbar is set as invisible (probably as part of add toolbar).
            // therefore, we forcefully disable toolbar, if necessary, after some delay.
            QTimer::singleShot(5, [toolbar, visible] {
                toolbar->setVisible(visible);
            });
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
    m_appWindow->recreateToolbarsMenu();
}

void LC_CreatorInvoker::createToolbar(const QString &toolbarName, const QStringList& actionNames, const int areaIndex) const {
    auto toolbar = m_appWindow->findChild<QToolBar *>(toolbarName);

    if (toolbar) {
        toolbar->clear();
    }
    else {
        toolbar = new QToolBar(toolbarName, m_appWindow);
        toolbar->setObjectName(toolbarName);
        if (m_showToolbarTooltips) {
            toolbar->setToolTip(tr("Toolbar: %1 (Custom)").arg(toolbarName));
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
        if ("" == key){
            toolbar->addSeparator();
        }
        else{
            toolbar->addAction(getAction(key));
        }
    }

    connect(toolbar, &QToolBar::visibilityChanged, this, &LC_CreatorInvoker::onCustomToolbarVisibilityChanged);
}

void LC_CreatorInvoker::destroyToolbar(const QString &toolbarName) const {
    const auto toolbar = m_appWindow->findChild<QToolBar *>(toolbarName);
    toolbar->setVisible(false);
    disconnect(toolbar, &QToolBar::visibilityChanged, this, &LC_CreatorInvoker::onCustomToolbarVisibilityChanged);
    delete toolbar;
}

void LC_CreatorInvoker::onCustomToolbarVisibilityChanged(const bool visible) {
    const auto toolbar = dynamic_cast<QToolBar*>(sender());
    if (toolbar != nullptr) {
        const QString toolbarName = toolbar->objectName();
        QSettings settings;
        settings.beginGroup("CustomToolbarsVisibility");
        QStringList visibleToolbars = settings.value("VisibleList").toStringList();

        const qsizetype size = visibleToolbars.count();
        int idx = -1;
        for (qsizetype i = 0; i < size; i++) {
            const QString& s = visibleToolbars.at(i);
            if (s == toolbarName) {
                idx = i;
                break;
            }
        }
        if (visible) {
            if (idx == -1) {
                visibleToolbars << toolbarName;
            }
        }
        else {
            if (idx != -1) {
                visibleToolbars.remove(idx);
            }
        }
        settings.setValue("VisibleList", visibleToolbars);
        settings.endGroup();

        // LC_ERR << "TB_Visible " << toolbarName << (visible ? " Yes" : " no");
    }
}

void LC_CreatorInvoker::invokeMenuCreator() {
    auto dlg = LC_DlgWidgetCreator(m_appWindow, true, m_actionGroupManager);
    dlg.exec();
    loadMenuActivators();
}

bool LC_CreatorInvoker::getMenuActionsForMouseEvent(const QMouseEvent* event, const RS_Entity* entity, QStringList& actions) {
    const LC_MenuActivator* activatorForEntityType {nullptr};
    const LC_MenuActivator* activatorForAnyEntity{nullptr};
    const LC_MenuActivator* activatorForEitherEntity{nullptr};
    const LC_MenuActivator* activatorForNoEntity{nullptr};
    const bool hasEntity = entity != nullptr;

    RS2::EntityType entityType = RS2::EntityUnknown;
    if (hasEntity) {
        entityType = entity->rtti();
    }

    for (const auto a: m_menuActivators) {
        if (a->isEventApplicable(event)) {
            const RS2::EntityType activatorEntityType = a->getEntityType();
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
                activatorForNoEntity = a;
            }
        }
    }

    const LC_MenuActivator*  activatorToUse {nullptr};

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
        const QString menuName = activatorToUse->getMenuName();

        const QSettings settings;
        const auto widget = QString("CustomMenus/%1").arg(menuName);
        const auto value = settings.value(widget);
        if (value.isValid()) {
            const QStringList sList = value.toStringList();
            actions.clear();
            actions.append(sList);
        }
    }
    return mayInvokeDefaultMenu;
}

bool LC_CreatorInvoker::isDefaultMenuInvokerEvent(const QMouseEvent* event) {
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

QAction *LC_CreatorInvoker::getAction(const QString &key) const {
    return m_actionGroupManager->getActionByName(key);
}
