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

#include "lc_dlgucslistoptions.h"

#include "lc_ucslistoptions.h"
#include "ui_lc_dlgucslistoptions.h"

LC_DlgUCSListOptions::LC_DlgUCSListOptions(LC_UCSListOptions* opts, QWidget *parent)
    : LC_Dialog(parent, "UCSListOptions")
    , ui(new Ui::LC_DlgUCSListOptions), m_options{opts}{
    ui->setupUi(this);


    ui->cbShowTypeIcon->setChecked(m_options->showColumnTypeIcon);
    ui->cbShowPositionAndAngle->setChecked(m_options->showColumnPositionAndAngle);
    ui->cbShowGridType->setChecked(m_options->showColumnGridType);
    ui->cbShowTooltip->setChecked(m_options->showViewInfoToolTip);

    ui->cbSilentUpdate->setChecked(m_options->duplicatedNameReplacesSilently);
    ui->cbShowPositionAndAngle->setChecked(m_options->showColumnPositionAndAngle);
    ui->cbRemovalConfirmation->setChecked(m_options->askForDeletionConfirmation);
    ui->cbSingleClickRestore->setChecked(m_options->restoreViewBySingleClick);
    ui->cbDoubleClickPolicy->setCurrentIndex(m_options->doubleClickPolicy);
    ui->cbUCSApplyPolicy->setCurrentIndex(m_options->ucsApplyingPolicy);

    ui->sbHighlightBlinkCount->setValue(m_options->highlightBlinksCount);
    ui->sbHighlightBlinkDelay->setValue(m_options->highlightBlinksDelay);

    ui->cbSilentUpdate->setVisible(false); // hide for now, reserve for future extension

    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_DlgUCSListOptions::validate);
}

LC_DlgUCSListOptions::~LC_DlgUCSListOptions(){
    delete ui;
}

void LC_DlgUCSListOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_DlgUCSListOptions::validate() {
    m_options->showColumnTypeIcon = ui->cbShowTypeIcon->isChecked();
    m_options->showColumnPositionAndAngle = ui->cbShowPositionAndAngle->isChecked();
    m_options->showColumnGridType= ui->cbShowGridType->isChecked();
    m_options->showViewInfoToolTip = ui->cbShowTooltip->isChecked();
    m_options->restoreViewBySingleClick = ui->cbSingleClickRestore->isChecked();
    m_options->duplicatedNameReplacesSilently = ui->cbSilentUpdate->isChecked();
    m_options->askForDeletionConfirmation = ui->cbRemovalConfirmation->isChecked();
    m_options->doubleClickPolicy = ui->cbDoubleClickPolicy->currentIndex();
    m_options->ucsApplyingPolicy = ui->cbUCSApplyPolicy->currentIndex();
    m_options->highlightBlinksCount = ui->sbHighlightBlinkCount->value();
    m_options->highlightBlinksDelay = ui->sbHighlightBlinkDelay->value();
}
