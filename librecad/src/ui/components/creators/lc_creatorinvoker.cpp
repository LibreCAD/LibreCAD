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

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMenu>
#include <QSettings>
#include <QToolBar>
#include <QVBoxLayout>

#include "lc_actiongroupmanager.h"
#include "lc_dialog.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "widgetcreator.h"

LC_CreatorInvoker::LC_CreatorInvoker(QC_ApplicationWindow *appWin, LC_ActionGroupManager *actionGroupManager)
    :m_appWindow{appWin}, m_actionGroupManager(actionGroupManager) {
}

void LC_CreatorInvoker::createCustomToolbars() {
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
            toolbar->addActions(actionsList);
            m_appWindow->addToolBar(toolbar);
        }
    }

    settings.endGroup();
}

void LC_CreatorInvoker::invokeToolbarCreator() {
    auto tb_creator = m_appWindow->findChild<QDialog *>("Toolbar Creator");
    if (tb_creator) {
        tb_creator->raise();
        tb_creator->activateWindow();
        return;
    }

    auto dlg = new LC_Dialog(m_appWindow, "ToolbarCreator");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Toolbar Creator"));
    dlg->setObjectName("Toolbar Creator");

    auto toolbarCreator = new WidgetCreator(dlg, m_actionGroupManager);
    toolbarCreator->addCustomWidgets("CustomToolbars");

    connect(toolbarCreator, &WidgetCreator::widgetToCreate, this, &LC_CreatorInvoker::createToolbar);
    connect(toolbarCreator, &WidgetCreator::widgetToDestroy, this, &LC_CreatorInvoker::destroyToolbar);

    auto layout = new QVBoxLayout;
    layout->addWidget(toolbarCreator);
    dlg->setLayout(layout);

    dlg->show();
}

void LC_CreatorInvoker::createToolbar(const QString &toolbar_name) {
    QSettings settings;
    auto tb = QString("CustomToolbars/%1").arg(toolbar_name);
    auto a_list = settings.value(tb).toStringList();

    auto toolbar = m_appWindow->findChild<QToolBar *>(toolbar_name);

    if (toolbar) {
        toolbar->clear();
    }
    else {
        toolbar = new QToolBar(toolbar_name, m_appWindow);
        toolbar->setObjectName(toolbar_name);
        m_appWindow->addToolBar(Qt::BottomToolBarArea, toolbar);
    }

    QStringList &list = a_list;
    for(const auto &key: list) {
        toolbar->addAction(getAction(key));
    }
}

void LC_CreatorInvoker::destroyToolbar(const QString &toolbar_name) {
    auto toolbar = m_appWindow->findChild<QToolBar *>(toolbar_name);
    delete toolbar;
}

void LC_CreatorInvoker::invokeMenuCreator() {
    auto menu_creator = m_appWindow->findChild<QDialog *>("Menu Creator");
    if (menu_creator) {
        menu_creator->raise();
        menu_creator->activateWindow();
        return;
    }

    auto dlg = new LC_Dialog(m_appWindow, "MenuCreator");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Menu Creator"));
    auto layout = new QVBoxLayout;
    auto widgetCreator = new WidgetCreator(dlg, m_actionGroupManager, true);
    widgetCreator->addCustomWidgets("CustomMenus");

    connect(widgetCreator, &WidgetCreator::widgetToDestroy, this, &LC_CreatorInvoker::destroyMenu);
    connect(widgetCreator, &WidgetCreator::widgetToAssign, this, &LC_CreatorInvoker::invokeMenuAssigner);
    connect(widgetCreator, &WidgetCreator::widgetToUpdate, this, &LC_CreatorInvoker::updateMenu);

    layout->addWidget(widgetCreator);
    dlg->setLayout(layout);
    dlg->show();
}

void LC_CreatorInvoker::invokeMenuAssigner(const QString &menu_name) {
    QSettings settings;
    settings.beginGroup("Activators");

    QDialog dlg;
    dlg.setWindowTitle(tr("Menu Assigner"));

    auto cb_1 = new QCheckBox("Double-Click");
    auto cb_2 = new QCheckBox("Right-Click");
    auto cb_3 = new QCheckBox("Ctrl+Right-Click");
    auto cb_4 = new QCheckBox("Shift+Right-Click");
    cb_1->setChecked(settings.value("Double-Click").toString() == menu_name);
    cb_2->setChecked(settings.value("Right-Click").toString() == menu_name);
    cb_3->setChecked(settings.value("Ctrl+Right-Click").toString() == menu_name);
    cb_4->setChecked(settings.value("Shift+Right-Click").toString() == menu_name);

    auto button_box = new QDialogButtonBox;
    button_box->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);

    connect(button_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto layout = new QVBoxLayout;
    dlg.setLayout(layout);

    auto frame = new QFrame;
    layout->addWidget(frame);

    auto f_layout = new QVBoxLayout;
    frame->setLayout(f_layout);

    f_layout->addWidget(cb_1);
    f_layout->addWidget(cb_2);
    f_layout->addWidget(cb_3);
    f_layout->addWidget(cb_4);
    f_layout->addWidget(button_box);

    if (dlg.exec()) {
        if (cb_1->isChecked())
            assignMenu("Double-Click", menu_name);
        else
            unassignMenu("Double-Click", menu_name);

        if (cb_2->isChecked())
            assignMenu("Right-Click", menu_name);
        else
            unassignMenu("Right-Click", menu_name);

        if (cb_3->isChecked())
            assignMenu("Ctrl+Right-Click", menu_name);
        else
            unassignMenu("Ctrl+Right-Click", menu_name);

        if (cb_4->isChecked())
            assignMenu("Shift+Right-Click", menu_name);
        else
            unassignMenu("Shift+Right-Click", menu_name);
    }
    settings.endGroup();
}

void LC_CreatorInvoker::unassignMenu(const QString &activator, const QString &menu_name) {
    QSettings settings;
    settings.beginGroup("Activators");

    if (settings.value(activator).toString() == menu_name) {
        settings.remove(activator);
    }
    settings.endGroup();

    m_appWindow->doForEachWindow([activator](QC_MDIWindow *win) {
        auto view = win->getGraphicView();
        view->destroyMenu(activator);
    });
}

void LC_CreatorInvoker::assignMenu(const QString &activator, const QString &menu_name) {
    QSettings settings;

    settings.beginGroup("Activators");
    settings.setValue(activator, menu_name);
    settings.endGroup();

    auto menu_key = QString("CustomMenus/%1").arg(menu_name);
    auto a_list = settings.value(menu_key).toStringList();

    m_appWindow->doForEachWindow([a_list, activator, menu_name, this](QC_MDIWindow *win) {
        auto graphicView = win->getGraphicView();
        auto menu = new QMenu(activator, graphicView);
        menu->setObjectName(menu_name);
        for (const auto& key: a_list) {
            auto action = getAction(key);
            if (action != nullptr) {
                menu->addAction(action);
            }
        }
        graphicView->setMenu(activator, menu);
    });
}

void LC_CreatorInvoker::updateMenu(const QString &menu_name) {
    QSettings settings;

    auto menu_key = QString("CustomMenus/%1").arg(menu_name);
    auto a_list = settings.value(menu_key).toStringList();

    settings.beginGroup("Activators");
    auto activators = settings.childKeys();

    for(const auto &activator: activators) {
        if (settings.value(activator).toString() == menu_name) {
            m_appWindow->doForEachWindow([activator, menu_name, a_list, this](QC_MDIWindow *win) {
                auto view = win->getGraphicView();
                auto menu = new QMenu(activator, view);
                menu->setObjectName(menu_name);
                for(const auto &key: a_list) {
                    menu->addAction(getAction(key));
                }
                view->setMenu(activator, menu);
            });
        }
    }
}

void LC_CreatorInvoker::destroyMenu(const QString &menu_name) {
    QSettings settings;
    settings.beginGroup("Activators");
    auto activators = settings.childKeys();

    for(const auto &activator: activators) {
        if (settings.value(activator).toString() == menu_name) {
            settings.remove(activator);
            m_appWindow->doForEachWindow([activator](QC_MDIWindow *win) {
                auto view = win->getGraphicView();
                view->destroyMenu(activator);
            });
        }
    }
    settings.endGroup();
}

void LC_CreatorInvoker::setupCustomMenuForNewGraphicsView(QG_GraphicView* view) {
    QSettings settings;
    settings.beginGroup("Activators");
    auto activators = settings.childKeys();
    settings.endGroup();
    // fixme - settings
    for (auto activator: activators) {
        auto menu_name = settings.value("Activators/" + activator).toString();
        auto path      = QString("CustomMenus/%1").arg(menu_name);
        auto a_list    = settings.value(path).toStringList();
        if (!a_list.isEmpty()) {
            auto menu      = new QMenu(activator, view);
            menu->setObjectName(menu_name);
            bool hasAction = false;
            foreach(auto key, a_list) {
                if (!key.isEmpty()) {
                    auto action = getAction(key);
                    if (action != nullptr) {
                        menu->QWidget::addAction(action);
                        hasAction = true;
                    }
                }
            }
            if (hasAction) {
                view->setMenu(activator, menu);
            }
            else {
                delete menu;
            }
        }
    }
}

QAction *LC_CreatorInvoker::getAction(const QString &key) {
    return m_actionGroupManager->getActionByName(key);
}
