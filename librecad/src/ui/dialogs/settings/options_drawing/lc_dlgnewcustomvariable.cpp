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

#include "lc_dlgnewcustomvariable.h"

#include <QMessageBox>

#include "ui_lc_dlgnewcustomvariable.h"

LC_DlgNewCustomVariable::LC_DlgNewCustomVariable(QWidget *parent)
    : LC_Dialog(parent, "NewCustomVarDlg")
    , ui(new Ui::LC_DlgNewCustomVariable){
    ui->setupUi(this);
    ui->leVarName->setFocus();
}

LC_DlgNewCustomVariable::~LC_DlgNewCustomVariable(){
    delete ui;
}

void LC_DlgNewCustomVariable::accept() {
    m_varName = ui->leVarName->text();
    m_varValue = ui->leVarValue->text();
    bool uniquePropertyName = !m_existingPropertyNames->contains(m_varName, Qt::CaseInsensitive);

    if (uniquePropertyName) {
        LC_Dialog::accept();
    }
    else {
        QMessageBox::critical(this, tr("New Custom Property"), tr("Property with provided name already exists. Please specify unique name."));
    }
}
