/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#include "comboboxoption.h"
#include "ui_comboboxoption.h"

ComboBoxOption::ComboBoxOption(QWidget* parent) :
    QFrame(parent),
    ui(new Ui::ComboBoxOption){
    ui->setupUi(this);
    ui->pushButton->setDisabled(true);
    connect(ui->pushButton, &QPushButton::released, this, &ComboBoxOption::saveIndexAndEmitOption);
    connect(ui->comboBox, &QComboBox::activated, this, &ComboBoxOption::setButtonState);
}

ComboBoxOption::~ComboBoxOption(){
    delete ui;
}

void ComboBoxOption::setTitle(const QString& title) const {
    ui->groupBox->setTitle(title);
}

void ComboBoxOption::setOptionsList(const QStringList& options) const {
    ui->comboBox->addItems(options);
}

void ComboBoxOption::setCurrentOption(const QString& option){
    int index = ui->comboBox->findText(option);
    ui->comboBox->setCurrentIndex(index);
    m_lastSavedIndex = index;
}

void ComboBoxOption::saveIndexAndEmitOption(){
    ui->pushButton->setDisabled(true);
    int index = ui->comboBox->currentIndex();
    QString option = ui->comboBox->itemText(index);
    m_lastSavedIndex = index;
    emit optionToSave(option);
}

void ComboBoxOption::setButtonState(int index){
    ui->pushButton->setDisabled((m_lastSavedIndex == index) ? true : false);
}
