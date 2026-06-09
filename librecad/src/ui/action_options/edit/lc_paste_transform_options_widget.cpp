/****************************************************************************
**
* Options widget for pen transform action

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

#include "lc_paste_transform_options_widget.h"

#include "lc_action_edit_paste_transform.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_paste_transform_options_widget.h"

LC_PasteTransformOptionsWidget::LC_PasteTransformOptionsWidget(): ui(new Ui::LC_PasteTransformOptionsWidget) {
    ui->setupUi(this);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PasteTransformOptionsWidget::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_PasteTransformOptionsWidget::onFactorEditingFinished);
    connect(ui->leArraySpacingX, &QLineEdit::editingFinished, this, &LC_PasteTransformOptionsWidget::onArraySpacingXEditingFinished);
    connect(ui->leArraySpacingY, &QLineEdit::editingFinished, this, &LC_PasteTransformOptionsWidget::onArraySpacingYEditingFinished);
    connect(ui->leArrayAngle, &QLineEdit::editingFinished, this, &LC_PasteTransformOptionsWidget::onArrayAngleEditingFinished);
    connect(ui->cbArray, &QCheckBox::clicked, this, &LC_PasteTransformOptionsWidget::onArrayClicked);
    connect(ui->sbArrayX, &QSpinBox::valueChanged, this, &LC_PasteTransformOptionsWidget::onArrayXCountChanged);
    connect(ui->sbArrayY, &QSpinBox::valueChanged, this, &LC_PasteTransformOptionsWidget::onArrayYCountChanged);
    connect(ui->cbSameAngles, &QCheckBox::clicked, this, &LC_PasteTransformOptionsWidget::cbSameAnglesClicked);

    pickDistanceSetup("spacingX", ui->tbPickArraySpacingX, ui->leArraySpacingX);
    pickDistanceSetup("spacingY", ui->tbPickArraySpacingY, ui->leArraySpacingY);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickAngleSetup("arrayAngle", ui->tbPickArrayAngle, ui->leArrayAngle);
}

LC_PasteTransformOptionsWidget::~LC_PasteTransformOptionsWidget() {
    delete ui;
}

void LC_PasteTransformOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionEditPasteTransform*>(a);

    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const QString factor = fromDouble(m_action->getFactor());
    const bool isArray = m_action->isArrayCreated();
    const int arrayXCount = m_action->getArrayXCount();
    const int arrayYCount = m_action->getArrayYCount();
    const QString arrayXSpacing = fromDouble(m_action->getArraySpacingX());
    const QString arrayYSpacing = fromDouble(m_action->getArraySpacingY());
    const bool sameAngles = m_action->isSameAngles();
    const QString arrayAngle = fromDouble(RS_Math::rad2deg(m_action->getArrayAngle()));

    LC_GuardedSignalsBlocker({
        ui->leAngle,
        ui->leFactor,
        ui->cbArray,
        ui->sbArrayX,
        ui->sbArrayY,
        ui->leArraySpacingX,
        ui->leArraySpacingY,
        ui->leArrayAngle,
        ui->cbSameAngles
    });

    ui->leAngle->setText(angle);
    ui->leFactor->setText(factor);

    ui->cbArray->setChecked(isArray);
    ui->sbArrayX->setEnabled(isArray);
    ui->sbArrayY->setEnabled(isArray);
    ui->leArraySpacingX->setEnabled(isArray);
    ui->leArraySpacingY->setEnabled(isArray);
    ui->leArrayAngle->setEnabled(isArray);
    ui->cbSameAngles->setEnabled(isArray);

    ui->tbPickArraySpacingX->setEnabled(isArray);
    ui->tbPickArraySpacingY->setEnabled(isArray);
    ui->tbPickArrayAngle->setEnabled(isArray);

    if (isArray) {
        const bool differentAngles = !ui->cbSameAngles->isChecked();
        ui->leAngle->setEnabled(differentAngles);
        ui->tbPickAngle->setEnabled(differentAngles);
    }
    else {
        ui->leAngle->setEnabled(true);
        ui->tbPickAngle->setEnabled(true);
    }

    ui->sbArrayX->setValue(arrayXCount);
    ui->sbArrayY->setValue(arrayYCount);
    ui->leArraySpacingX->setText(arrayXSpacing);
    ui->leArraySpacingY->setText(arrayYSpacing);

    ui->leArrayAngle->setText(arrayAngle);
    if (ui->cbSameAngles->isChecked()) {
        ui->leAngle->setText(arrayAngle);
    }

    ui->cbSameAngles->setChecked(sameAngles);
    if (sameAngles) {
        ui->leAngle->setText(ui->leArrayAngle->text());
        const bool enable = !ui->cbArray->isChecked();
        ui->leAngle->setEnabled(enable);
    }
    else {
        ui->leAngle->setEnabled(true);
    }
}

void LC_PasteTransformOptionsWidget::onArrayXCountChanged(const int value) {
    m_action->setArrayXCount(value);
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onArrayYCountChanged(const int value) {
    m_action->setArrayYCount(value);
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
}

void LC_PasteTransformOptionsWidget::onFactorEditingFinished() {
    const auto val = ui->leFactor->text();
    double y;
    if (toDouble(val, y, 1.0, true)) {
        m_action->setFactor(y);
    }
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onArraySpacingXEditingFinished() {
    const auto val = ui->leArraySpacingX->text();
    double y;
    if (toDouble(val, y, 1.0, true)) {
        m_action->setArraySpacingX(y);
    }
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onArraySpacingYEditingFinished() {
    const auto val = ui->leArraySpacingY->text();
    double y;
    if (toDouble(val, y, 1.0, true)) {
        m_action->setArraySpacingY(y);
    }
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onArrayAngleEditingFinished() {
    const auto val = ui->leArrayAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setArrayAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::onArrayClicked(const bool clicked) {
    m_action->setArrayCreated(clicked);
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::cbSameAnglesClicked(const bool value) {
    m_action->setSameAngles(value);
    m_action->updateOptions();
}

void LC_PasteTransformOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
