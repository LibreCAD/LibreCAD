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
#include "ui_lc_dlgucslistoptions.h"

LC_DlgUCSListOptions::LC_DlgUCSListOptions(LC_UCSListOptions* opts, QWidget *parent)
    : LC_Dialog(parent, "UCSListOptions")
    , ui(new Ui::LC_DlgUCSListOptions), options{opts}
{
    ui->setupUi(this);


    ui->cbShowTypeIcon->setChecked(options->showColumnTypeIcon);
    ui->cbShowPositionAndAngle->setChecked(options->showColumnPositionAndAngle);
    ui->cbShowGridType->setChecked(options->showColumnGridType);
    ui->cbShowTooltip->setChecked(options->showViewInfoToolTip);

    ui->cbSilentUpdate->setChecked(options->duplicatedNameReplacesSilently);
    ui->cbShowPositionAndAngle->setChecked(options->showColumnPositionAndAngle);
    ui->cbRemovalConfirmation->setChecked(options->askForDeletionConfirmation);
    ui->cbSingleClickRestore->setChecked(options->restoreViewBySingleClick);
    ui->cbDoubleClickPolicy->setCurrentIndex(options->doubleClickPolicy);
    ui->cbUCSApplyPolicy->setCurrentIndex(options->ucsApplyingPolicy);

    ui->sbHighlightBlinkCount->setValue(options->highlightBlinksCount);
    ui->sbHighlightBlinkDelay->setValue(options->highlightBlinksDelay);

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
    options->showColumnTypeIcon = ui->cbShowTypeIcon->isChecked();
    options->showColumnPositionAndAngle = ui->cbShowPositionAndAngle->isChecked();
    options->showColumnGridType= ui->cbShowGridType->isChecked();
    options->showViewInfoToolTip = ui->cbShowTooltip->isChecked();
    options->restoreViewBySingleClick = ui->cbSingleClickRestore->isChecked();
    options->duplicatedNameReplacesSilently = ui->cbSilentUpdate->isChecked();
    options->askForDeletionConfirmation = ui->cbRemovalConfirmation->isChecked();
    options->doubleClickPolicy = ui->cbDoubleClickPolicy->currentIndex();
    options->ucsApplyingPolicy = ui->cbUCSApplyPolicy->currentIndex();
    options->highlightBlinksCount = ui->sbHighlightBlinkCount->value();
    options->highlightBlinksDelay = ui->sbHighlightBlinkDelay->value();
}
