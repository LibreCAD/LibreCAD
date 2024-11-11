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

#include "lc_optionswidgetsholder.h"
#include "ui_lc_optionswidgetsholder.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "lc_shortcuts_manager.h"

LC_OptionsWidgetsHolder::LC_OptionsWidgetsHolder(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_OptionsWidgetsHolder){
    ui->setupUi(this);
    ui->snapOptionsHolder->setLocatedOnLeft(true);
}

LC_OptionsWidgetsHolder::~LC_OptionsWidgetsHolder(){
    delete ui;
}

LC_SnapOptionsWidgetsHolder *LC_OptionsWidgetsHolder::getSnapOptionsHolder() {
    return ui->snapOptionsHolder;
}

#define DEBUG_WIDGETS_COUNT_

void LC_OptionsWidgetsHolder::addOptionsWidget(QWidget *optionsWidget) {
    if (optionsWidget != nullptr) {
        if (hasActionIcon) {
            ui->vCurrentActionLine->setVisible(true);
        }
        ui->wOptionsWidgetsContainer->layout()->addWidget(optionsWidget);
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &list = ui->wOptionsWidgetsContainer->children();
        if (!list.isEmpty()) {
            int size = list.size();
            LC_ERR << "OPTION WIDGETS: " << size;
        }
#endif
    }
}

void LC_OptionsWidgetsHolder::removeOptionsWidget(QWidget *optionsWidget) {
    ui->vCurrentActionLine->setVisible(false);
    if (optionsWidget != nullptr) {
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &list = ui->wOptionsWidgetsContainer->children();
        LC_ERR << "OPTION WIDGETS BEFORE: " << list.size();
#endif

        ui->wOptionsWidgetsContainer->layout()->removeWidget(optionsWidget);
        optionsWidget->deleteLater();
#ifdef DEBUG_WIDGETS_COUNT
        const QObjectList &listAfter = ui->wOptionsWidgetsContainer->children();
        LC_ERR << "OPTION WIDGETS AFTER: " << listAfter.size();
#endif
    }
}

void LC_OptionsWidgetsHolder::clearActionIcon() {
   auto i  = QIcon();
   hasActionIcon = false;
   doSetIcon(i,"");
}

void LC_OptionsWidgetsHolder::setCurrentQAction(QAction *a) {
    QIcon icon;
    QString text="";
    if (LC_GET_ONE_BOOL("Appearance", "ShowActionIconInOptions", true)){
        icon = a->icon();
        text = LC_ShortcutsManager::getPlainActionToolTip(a);
        hasActionIcon = true;
    }
    else{
        icon  = QIcon();
        hasActionIcon = false;
    }
    doSetIcon(icon, text);
}

void LC_OptionsWidgetsHolder::doSetIcon(const QIcon &icon, const QString& text) {
    ui->lCurrentAction->setPixmap(icon.pixmap(iconSize));
    ui->lCurrentAction->setToolTip(tr("Current Action:")+ " " + text);
}
