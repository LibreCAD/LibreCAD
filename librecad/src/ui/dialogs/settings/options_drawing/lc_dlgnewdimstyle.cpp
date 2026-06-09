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

void LC_DlgNewDimStyle::onUsedForChanged(const int index) {
    const bool forAllDimensions = index == 0;
    ui->leStyleName->setEnabled(forAllDimensions);
    if (forAllDimensions) {
        ui->leStyleName->selectAll();
    }
    if (!forAllDimensions) {
        const int currentBasedOnIdx = ui->cbBasedOnStyle->currentIndex();
        const auto dimStyleItem = m_dimItemsListModel->getItemAtRow(currentBasedOnIdx);
        if (!dimStyleItem->isBaseStyle()) {
            const auto majorDimStyleItem = dimStyleItem->parentItem();
            if (majorDimStyleItem != nullptr) { // this is style for specific dim type
                const int newIndex = m_dimItemsListModel->getItemIndex(majorDimStyleItem);
                ui->cbBasedOnStyle->blockSignals(true);
                ui->cbBasedOnStyle->setCurrentIndex(newIndex);
                ui->cbBasedOnStyle->blockSignals(false);
            }
        }
    }
    switch (index) {
        case 0:
            m_dimType = RS2::EntityUnknown;
            break;
        case 1:
            m_dimType = RS2::EntityDimLinear;
            break;
        case 2:
            m_dimType = RS2::EntityDimAngular;
            break;
        case 3:
            m_dimType = RS2::EntityDimRadial;
            break;
        case 4:
            m_dimType = RS2::EntityDimDiametric;
            break;
        case 5:
            m_dimType = RS2::EntityDimOrdinate;
            break;
        case 6:
            m_dimType = RS2::EntityDimLeader;
            break;
        default:
            m_dimType = RS2::EntityUnknown;
    }
}

void LC_DlgNewDimStyle::onBasedOnChanged(const int index) {
    const int currentUsedForMode = ui->cbUseFor->currentIndex();
    if (currentUsedForMode != 0) {
        ui->cbUseFor->setCurrentIndex(0);
        m_nameWasEntered = false;
    }
    m_baseDimStyle = m_dimItemsListModel->getItemAtRow(index);
    if (!m_nameWasEntered) {
        const QString newStyleName = "Copy of " + m_baseDimStyle->displayName();
        ui->leStyleName->setText(newStyleName);
        ui->leStyleName->selectAll();
    }
}

void LC_DlgNewDimStyle::onStyleNameTextChanged(const QString&) {
    m_nameWasEntered = true;
}

void LC_DlgNewDimStyle::setup(const LC_DimStyleItem* initialStyle, const QList<LC_DimStyleItem*>& items) {
    QList<LC_DimStyleItem*> sortedItems;
    sortedItems.append(items);
    // fixme - sand - sort items by base name!
    m_dimItemsListModel = new LC_StylesListModel(this, sortedItems, false);
    m_dimItemsListModel->sort(0, Qt::SortOrder::AscendingOrder);
    ui->cbBasedOnStyle->setModel(m_dimItemsListModel);
    const int initialIndex = m_dimItemsListModel->getItemIndex(initialStyle);
    ui->cbBasedOnStyle->setCurrentIndex(initialIndex);
    ui->leStyleName->selectAll();
    ui->leStyleName->setFocus();
}

QString LC_DlgNewDimStyle::getStyleName() const {
    switch (m_dimType) {
        case RS2::EntityUnknown:
            return ui->leStyleName->text();
        default: {
            const QString baseStyleName = m_baseDimStyle->dimStyle()->getName();
            QString resultingName = LC_DimStyle::getStyleNameForBaseAndType(baseStyleName, m_dimType);
            return resultingName;
        }
    }
}

void LC_DlgNewDimStyle::onAccept() {
    const QString newName = getStyleName();
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
