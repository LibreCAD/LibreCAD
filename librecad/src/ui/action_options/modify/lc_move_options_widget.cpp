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

#include "lc_move_options_widget.h"

#include "lc_action_modify_move.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_move_options_widget.h"

LC_MoveOptionsWidget::LC_MoveOptionsWidget(): ui(new Ui::LC_MoveOptionsWidget) {
    ui->setupUi(this);
    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_MoveOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_MoveOptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_MoveOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_MoveOptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_MoveOptionsWidget::onCopiesCountChanged);
}

void LC_MoveOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyMove*>(a);

    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const int copiesNumber = m_action->getCopiesNumber();
    const bool keepOriginals = m_action->isKeepOriginals();
    const bool useMultipleCopies = m_action->isUseMultipleCopies();
    LC_GuardedSignalsBlocker({ui->cbCurrentAttr, ui->cbKeepOriginals, ui->cbCurrentLayer, ui->cbMultipleCopies, ui->sbNumberOfCopies});

    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);
    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
}

void LC_MoveOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_MoveOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_MoveOptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_MoveOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_MoveOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_MoveOptionsWidget::onCopiesCountChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}
