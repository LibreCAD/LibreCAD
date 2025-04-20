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

#include "lc_dlgnamedviewslistoptions.h"

#include "lc_namedviewslistoptions.h"
#include "ui_lc_dlgnamedviewslistoptions.h"

LC_DlgNamedViewsListOptions::LC_DlgNamedViewsListOptions(LC_NamedViewsListOptions* opts, QWidget *parent)
    : LC_Dialog(parent,"NamedViewsListOptions")
    , ui(new Ui::LC_DlgNamedViewsListOptions),m_options{opts}{
    ui->setupUi(this);

    ui->cbShowTooltip->setChecked(m_options->showViewInfoToolTip);
    ui->cbShowViewTypeIcon->setChecked(m_options->showColumnIconType);
    ui->cbSilentUpdate->setChecked(m_options->duplicatedNameReplacesSilently);
    ui->cbRemovalConfirmation->setChecked(m_options->askForDeletionConfirmation);
    ui->cbSingleClickRestore->setChecked(m_options->restoreViewBySingleClick);
    ui->cbDoubleClickPolicy->setCurrentIndex(m_options->doubleClickPolicy);
    ui->cbColumnGridType->setChecked(m_options->showColumnGridType);
    ui->cbColumnUCSType->setChecked(m_options->showColumnUCSType);
    ui->cbColumnUCSDetails->setChecked(m_options->showColumnUCSDetails);
    ui->cbShowColumnVIewDetails->setChecked(m_options->showColumnViewDetails);

    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_DlgNamedViewsListOptions::validate);
}

LC_DlgNamedViewsListOptions::~LC_DlgNamedViewsListOptions(){
    delete ui;
}

void LC_DlgNamedViewsListOptions::languageChange() {
    ui->retranslateUi(this);
}

void LC_DlgNamedViewsListOptions::validate() const {
    m_options->showColumnIconType = ui->cbShowViewTypeIcon->isChecked();
    m_options->restoreViewBySingleClick = ui->cbSingleClickRestore->isChecked();
    m_options->duplicatedNameReplacesSilently = ui->cbSilentUpdate->isChecked();
    m_options->showViewInfoToolTip = ui->cbShowTooltip->isChecked();
    m_options->askForDeletionConfirmation = ui->cbRemovalConfirmation->isChecked();
    m_options->doubleClickPolicy = ui->cbDoubleClickPolicy->currentIndex();
    m_options->showColumnGridType = ui->cbColumnGridType->isChecked();
    m_options->showColumnUCSType = ui->cbColumnUCSType->isChecked();
    m_options->showColumnUCSDetails = ui->cbColumnUCSDetails->isChecked();
    m_options->showColumnViewDetails = ui->cbShowColumnVIewDetails->isChecked();
}
