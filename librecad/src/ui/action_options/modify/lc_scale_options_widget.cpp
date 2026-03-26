/****************************************************************************
**
*Options widget for ModifyScale m_action

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

#include "lc_scale_options_widget.h"

#include "lc_action_modify_scale.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_scale_options_widget.h"

LC_ScaleOptionsWidget::LC_ScaleOptionsWidget(): ui(new Ui::LC_ScaleOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbUseCurrentLayerClicked);

    connect(ui->cbExplicitFactor, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbExplicitFactorClicked);
    connect(ui->cbIsotrpic, &QCheckBox::clicked, this, &LC_ScaleOptionsWidget::cbIsotropicClicked);
    connect(ui->leFactorX, &QLineEdit::editingFinished, this, &LC_ScaleOptionsWidget::onFactorXEditingFinished);
    connect(ui->leFactorY, &QLineEdit::editingFinished, this, &LC_ScaleOptionsWidget::onFactorYEditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_ScaleOptionsWidget::onCopiesNumberValueChanged);
}

void LC_ScaleOptionsWidget::updateUI(const int mode, [[maybe_unused]]const QVariant* value) {
    if (mode == 0) {
        // update on SetTargetPoint
        const QString factorX = fromDouble(m_action->getFactorX());

        ui->leFactorX->blockSignals(true);
        ui->leFactorY->blockSignals(true);

        ui->leFactorX->setText(factorX);
        if (ui->cbIsotrpic) {
            ui->leFactorY->setText(factorX);
        }
        else {
            const QString factorY = fromDouble(m_action->getFactorY());
            ui->leFactorY->setText(factorY);
        }

        ui->leFactorX->blockSignals(false);
        ui->leFactorY->blockSignals(false);
    }
    else {
        // do nothing
    }
}

void LC_ScaleOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyScale*>(a);

    bool useCurrentLayer = m_action->isUseCurrentLayer();
    bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    bool keepOriginals = m_action->isKeepOriginals();
    bool useMultipleCopies = m_action->isUseMultipleCopies();
    int copiesNumber = m_action->getCopiesNumber();

    bool explicitFactor = m_action->isExplicitFactor();
    bool isotrophic = m_action->isIsotropicScaling();
    QString factorX = fromDouble(m_action->getFactorX());
    QString factorY = fromDouble(m_action->getFactorY());

    LC_GuardedSignalsBlocker({
        ui->cbCurrentAttr,
        ui->cbKeepOriginals,
        ui->cbCurrentLayer,
        ui->cbMultipleCopies,
        ui->sbNumberOfCopies,
        ui->leFactorX,
        ui->leFactorY,
        ui->cbIsotrpic,
        ui->cbExplicitFactor
    });

    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);
    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);

    ui->leFactorX->setText(factorX);
    ui->leFactorY->setText(factorY);
    ui->cbExplicitFactor->setChecked(explicitFactor);
    ui->leFactorX->setEnabled(!explicitFactor);
    ui->leFactorY->setEnabled(!explicitFactor && !isotrophic);

    ui->cbIsotrpic->setChecked(isotrophic);
    ui->leFactorY->setEnabled(!isotrophic && !explicitFactor);
}

void LC_ScaleOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::onCopiesNumberValueChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::cbExplicitFactorClicked(const bool val) {
    m_action->setExplicitFactor(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::cbIsotropicClicked(const bool val) {
    m_action->setIsotropicScaling(val);
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::onFactorXEditingFinished() {
    auto val = ui->leFactorX->text();
    double factor = 1.;
    if (toDouble(val, factor, 0.0, false)) {
        m_action->setFactorX(factor);
    }
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::onFactorYEditingFinished() {
    auto val = ui->leFactorY->text();
    double factor = 1.;
    if (toDouble(val, factor, 0.0, false)) {
        m_action->setFactorY(factor);
    }
    m_action->updateOptions();
}

void LC_ScaleOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
