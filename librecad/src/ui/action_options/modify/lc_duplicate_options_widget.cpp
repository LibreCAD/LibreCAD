/****************************************************************************
**
* Options widget for "Duplicate" action.

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
**********************************************************************/
#include "lc_duplicate_options_widget.h"

#include "lc_action_modify_duplicate.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_duplicate_options_widget.h"

LC_DuplicateOptionsWidget::LC_DuplicateOptionsWidget() : ui(new Ui::LC_DuplicateOptionsWidget) {
    ui->setupUi(this);
    connect(ui->leOffsetX, &QLineEdit::editingFinished, this, &LC_DuplicateOptionsWidget::onOffsetXEditingFinished);
    connect(ui->leOffsetY, &QLineEdit::editingFinished, this, &LC_DuplicateOptionsWidget::onOffsetYEditingFinished);
    connect(ui->cbInPlace, &QCheckBox::clicked, this, &LC_DuplicateOptionsWidget::onInPlaceClicked);
    connect(ui->cbPen, &QComboBox::currentIndexChanged, this, &LC_DuplicateOptionsWidget::onPenModeIndexChanged);
    connect(ui->cbLayer, &QComboBox::currentIndexChanged, this, &LC_DuplicateOptionsWidget::onLayerModeIndexChanged);

    pickDistanceSetup("offsetX", ui->tbPickOffsetX, ui->leOffsetX);
    pickDistanceSetup("offsetY", ui->tbPickOffsetY, ui->leOffsetY);
}

LC_DuplicateOptionsWidget::~LC_DuplicateOptionsWidget() {
    delete ui;
    m_action = nullptr;
}

void LC_DuplicateOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_DuplicateOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyDuplicate*>(a);

    const QString ofX = fromDouble(m_action->getOffsetX());
    const QString ofY = fromDouble(m_action->getOffsetY());
    const bool inplace = m_action->isDuplicateInPlace();
    const int penMode = m_action->getPenMode();
    const int layerMode = m_action->getLayerMode();

    LC_GuardedSignalsBlocker({ui->cbInPlace, ui->cbLayer, ui->cbLayer, ui->leOffsetX, ui->leOffsetY});

    ui->leOffsetX->setText(ofX);
    ui->leOffsetY->setText(ofY);
    ui->leOffsetX->setEnabled(!inplace);
    ui->leOffsetY->setEnabled(!inplace);
    ui->tbPickOffsetX->setEnabled(!inplace);
    ui->tbPickOffsetY->setEnabled(!inplace);
    ui->cbPen->setEnabled(!inplace);
    ui->cbLayer->setEnabled(!inplace);
    ui->cbInPlace->setChecked(inplace);
    ui->cbPen->setCurrentIndex(penMode);
    ui->cbLayer->setCurrentIndex(layerMode);
}

void LC_DuplicateOptionsWidget::onOffsetXEditingFinished() {
    const QString& val = ui->leOffsetX->text();double value;
    if (toDouble(val, value, 0, false)) {
        m_action->setOffsetX(value);
    }
    m_action->updateOptions();
}

void LC_DuplicateOptionsWidget::onInPlaceClicked(const bool value) const {
    m_action->setDuplicateInPlace(value);
    m_action->updateOptions();
}

void LC_DuplicateOptionsWidget::onOffsetYEditingFinished() {
    const QString& val = ui->leOffsetY->text();
    double value;
    if (toDouble(val, value, 0, false)) {
        m_action->setOffsetY(value);
    }
    m_action->updateOptions();
}

void LC_DuplicateOptionsWidget::onPenModeIndexChanged(const int mode) const {
    m_action->setPenMode(mode);
    m_action->updateOptions();
}

void LC_DuplicateOptionsWidget::onLayerModeIndexChanged(const int mode) const {
    m_action->setLayerMode(mode);
    m_action->updateOptions();
}
