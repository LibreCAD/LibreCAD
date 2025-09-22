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

#include "lc_dlgnewwidget.h"

#include <QMessageBox>

#include "ui_lc_dlgnewwidget.h"

LC_DlgNewWidget::LC_DlgNewWidget(QWidget *parent, bool forMenu, QStringList* existingWidgetsList)
    : LC_Dialog(parent, forMenu ? "DlgNewMenu" : "DlgNewToolbar")
    , ui(new Ui::LC_DlgNewWidget)
    , m_existingWidgetsList{existingWidgetsList}
    , m_forMenu{forMenu}
{
    ui->setupUi(this);
    if (forMenu) {
        setWindowTitle("New Menu");
        ui->lblWidgetName->setText(tr("Menu Name:"));
        ui->cbClearActions->setVisible(true);
        ui->leWidgetName->setToolTip(tr("Unique name of menu to create."));
    }
    else {
        setWindowTitle("New Toolbar");
        ui->lblWidgetName->setText(tr("Toolbar Name:"));
        ui->leWidgetName->setToolTip(tr("Unique name of toolbar to create."));
        ui->cbClearActions->setVisible(false);
    }
}

QString LC_DlgNewWidget::askForNewWidgetName(QWidget* parent, bool forMenu, QStringList* existingWidgetsList, bool& clearActionsList) {
    auto dlg = new LC_DlgNewWidget(parent, forMenu, existingWidgetsList);
    QString result;
    if (dlg->exec() == QDialog::Accepted) {
        clearActionsList = dlg->ui->cbClearActions->isChecked();
        result = dlg->ui->leWidgetName->text().trimmed();
        result = result.replace("/", "-");
    }
    delete dlg;
    return result;
}

void LC_DlgNewWidget::accept() {
    QString name = ui->leWidgetName->text().trimmed();
    name = name.replace("/", "-");
    if (name.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Please enter a name."));
    }
    else if (m_existingWidgetsList->contains(name)) {
            QMessageBox::critical(this, tr("Error"), tr("Name is not unique. Please enter unique name."));
    }
    else {
        LC_Dialog::accept();
    }
}

LC_DlgNewWidget::~LC_DlgNewWidget(){
    delete ui;
}
