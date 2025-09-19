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

#include "lc_dlgnewdimstyle.h"

#include <QMessageBox>

#include "lc_dimstyle.h"
#include "lc_dimstyleitem.h"
#include "lc_dimstyleslistmodel.h"
#include "ui_lc_dlgnewdimstyle.h"

LC_DlgNewDimStyle::LC_DlgNewDimStyle(QWidget *parent)
    : LC_Dialog(parent, "NewDimStyle")
    , ui(new Ui::LC_DlgNewDimStyle){
    ui->setupUi(this);

    connect(ui->cbUseFor, &QComboBox::currentIndexChanged, this, &LC_DlgNewDimStyle::onUsedForChanged);
    connect(ui->cbBasedOnStyle, &QComboBox::currentIndexChanged, this, &LC_DlgNewDimStyle::onBasedOnChanged);
    connect(ui->leStyleName, &QLineEdit::textEdited, this, &LC_DlgNewDimStyle::onStyleNameTextChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_DlgNewDimStyle::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

LC_DlgNewDimStyle::~LC_DlgNewDimStyle(){
    delete ui;
}

void LC_DlgNewDimStyle::onUsedForChanged(int index) {
    bool forAllDimensions = index == 0;
    ui->leStyleName->setEnabled(forAllDimensions);
    if (forAllDimensions) {
        ui->leStyleName->selectAll();
    }
    if (!forAllDimensions) {
        int currentBasedOnIdx = ui->cbBasedOnStyle->currentIndex();
        auto dimStyleItem = m_dimItemsListModel->getItemAtRow(currentBasedOnIdx);
        if (!dimStyleItem->isBaseStyle()) {
            auto majorDimStyleItem = dimStyleItem->parentItem();
            if (majorDimStyleItem != nullptr) { // this is style for specific dim type
                int newIndex = m_dimItemsListModel->getItemIndex(majorDimStyleItem);
                ui->cbBasedOnStyle->blockSignals(true);
                ui->cbBasedOnStyle->setCurrentIndex(newIndex);
                ui->cbBasedOnStyle->blockSignals(false);
            }
        }
    }
    switch (index) {
        case 0:
            dimType = RS2::EntityUnknown;
            break;
        case 1:
            dimType = RS2::EntityDimLinear;
            break;
        case 2:
            dimType = RS2::EntityDimAngular;
            break;
        case 3:
            dimType = RS2::EntityDimRadial;
            break;
        case 4:
            dimType = RS2::EntityDimDiametric;
            break;
        case 5:
            dimType = RS2::EntityDimOrdinate;
            break;
        case 6:
            dimType = RS2::EntityDimLeader;
            break;
        default:
            dimType = RS2::EntityUnknown;
    }
}

void LC_DlgNewDimStyle::onBasedOnChanged(int index) {
    int currentUsedForMode = ui->cbUseFor->currentIndex();
    if (currentUsedForMode != 0) {
        ui->cbUseFor->setCurrentIndex(0);
        nameWasEntered = false;
    }
    baseDimStyle = m_dimItemsListModel->getItemAtRow(index);
    if (!nameWasEntered) {
        QString newStyleName = "Copy of " + baseDimStyle->displayName();
        ui->leStyleName->setText(newStyleName);
        ui->leStyleName->selectAll();
    }
}

void LC_DlgNewDimStyle::onStyleNameTextChanged(const QString&) {
    nameWasEntered = true;
}

void LC_DlgNewDimStyle::setup(LC_DimStyleItem* initialStyle, QList<LC_DimStyleItem*>& items) {
    QList<LC_DimStyleItem*> sortedItems;
    sortedItems.append(items);
    // fixme - sand - sort items by base name!
    m_dimItemsListModel = new LC_StylesListModel(this, sortedItems, false);
    m_dimItemsListModel->sort(0, Qt::SortOrder::AscendingOrder);
    ui->cbBasedOnStyle->setModel(m_dimItemsListModel);
    int initialIndex = m_dimItemsListModel->getItemIndex(initialStyle);
    ui->cbBasedOnStyle->setCurrentIndex(initialIndex);
    ui->leStyleName->selectAll();
    ui->leStyleName->setFocus();
}

QString LC_DlgNewDimStyle::getStyleName() const {
    switch (dimType) {
        case RS2::EntityUnknown:
            return ui->leStyleName->text();
        default: {
            QString baseStyleName = baseDimStyle->dimStyle()->getName();
            QString resultingName = LC_DimStyle::getStyleNameForBaseAndType(baseStyleName, dimType);
            return resultingName;
        }
    }
}

void LC_DlgNewDimStyle::onAccept() {
    QString newName = getStyleName();
    if (newName.isEmpty()) {
        QMessageBox::warning(this, tr("New Dimension Style"), tr("Empty name of style is not allowed."));
        return;
    }
    if (m_dimItemsListModel->findByName(newName) != nullptr) {
        QMessageBox::warning(this, tr("New Dimension Style"), tr("Provided name of dimension style (%1) is not unique! Please enter another one.").arg(newName));
        return;
    }
    LC_Dialog::accept();
}
