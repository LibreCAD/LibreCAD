/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_inputtextdialog.h"
#include "ui_lc_inputtextdialog.h"

LC_InputTextDialog::LC_InputTextDialog(QWidget *parent)
    : LC_Dialog(parent, "InputDlg")
    , ui(new Ui::LC_InputTextDialog){
    ui->setupUi(this);
}

LC_InputTextDialog::~LC_InputTextDialog(){
    delete ui;
}

QString LC_InputTextDialog::getText(QWidget *parent, const QString &title, const QString &label,
                       const QStringList &options,
                       bool allowsEditing,
                       const QString &text, bool *ok){
    LC_InputTextDialog dlg(parent);

    dlg.setWindowTitle(title);
    dlg.ui->lblLabel->setText(label);

    dlg.ui->cbInput->setEditable(allowsEditing);
    dlg.ui->cbInput->setCurrentText(text);
    dlg.ui->cbInput->addItems(options);

    dlg.ui->cbInput->setFocus();

    if (dlg.exec() == Accepted){
        *ok = true;
        return dlg.ui->cbInput->currentText();
    }
    else{
        *ok = false;
        return "";
    }
}

int LC_InputTextDialog::selectId(QWidget *parent, const QString &title, const QString &label,
                          const QList<QPair<int, QString>> &options, bool *ok){
    LC_InputTextDialog dlg(parent);

    dlg.setWindowTitle(title);
    dlg.ui->lblLabel->setText(label);

    auto cb_input = dlg.ui->cbInput;
    cb_input->setEditable(false);
    for (auto p: options) {
        cb_input->addItem(p.second, p.first);
    }

    if (dlg.exec() == Accepted){
        *ok = true;
        return cb_input->itemData(cb_input->currentIndex()).toInt();
    }
    else{
        *ok = false;
        return -1;
    }
}
