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

#include "lc_multilinetexteditdialog.h"

#include "ui_lc_multilinetexteditdialog.h"

LC_MultilineTextEditDialog::LC_MultilineTextEditDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::LC_MultilineTextEditDialog) {
    ui->setupUi(this);

    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
}

LC_MultilineTextEditDialog::~LC_MultilineTextEditDialog() {
    delete ui;
}

void LC_MultilineTextEditDialog::setReadOnly(const bool value) const {
    ui->plainTextEdit->setReadOnly(value);
    if (value) {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    }
    else {
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    }
}

void LC_MultilineTextEditDialog::setText(const QString& text) const {
    ui->plainTextEdit->setPlainText(text);
}

QString LC_MultilineTextEditDialog::getText() const {
    return ui->plainTextEdit->toPlainText();
}

void LC_MultilineTextEditDialog::on_buttonBox_clicked(QAbstractButton* button) {
    switch (ui->buttonBox->buttonRole(button)) {
        case QDialogButtonBox::AcceptRole: {
            accept();
            break;
        }
        case QDialogButtonBox::RejectRole: {
            reject();
            break;
        }
        default:
            break;
    }
}
