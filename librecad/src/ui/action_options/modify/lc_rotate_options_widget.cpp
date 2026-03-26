/****************************************************************************
**
* Options widget for ModifyRotate action

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
#include "lc_rotate_options_widget.h"

#include "lc_action_modify_rotate.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_rotate_options_widget.h"

LC_RotateOptionsWidget::LC_RotateOptionsWidget(): ui(new Ui::LC_RotateOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbUseCurrentLayerClicked);

    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbFreeAngleClicked);
    connect(ui->cbRelativeAngle, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbRelativeAngleClicked);
    connect(ui->cbFreeRefAngle, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbFreeRefAngleClicked);
    connect(ui->cbCenterPointFirst, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::cbCenterPointFirstClicked);
    connect(ui->cbTwoRotations, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::onTwoRotationsClicked);
    connect(ui->cbAbsoluteRefAngle, &QCheckBox::clicked, this, &LC_RotateOptionsWidget::onAbsoluteRefAngleClicked);

    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_RotateOptionsWidget::onCopiesNumberValueChanged);

    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_RotateOptionsWidget::onAngleEditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_RotateOptionsWidget::onRefPointAngleEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
    pickAngleSetup("angle2", ui->tbPickAngle2, ui->leAngle2);
}

LC_RotateOptionsWidget::~LC_RotateOptionsWidget() {
    delete ui;
}

void LC_RotateOptionsWidget::updateUI(const int mode, [[maybe_unused]]const QVariant* value) {
    switch (mode) {
        case UPDATE_ANGLE: {
            // update on SetTargetPoint
            const QString angle = fromDouble(m_action->getCurrentAngleDegrees());

            ui->leAngle->blockSignals(true);
            ui->leAngle->setText(angle);
            ui->leAngle->blockSignals(false);

            ui->leAngle->update();
            break;
        }
        case DISABLE_SECOND_ROTATION: {
            allowSecondRotationUI(false);
            break;
        }
        case ENABLE_SECOND_ROTATION: {
            allowSecondRotationUI(true);
            break;
        }
        case UPDATE_ANGLE2: {
            // update on SetTargetPoint
            const QString angle2 = fromDouble(m_action->getCurrentAngle2Degrees());

            ui->leAngle2->blockSignals(true);
            ui->leAngle2->setText(angle2);
            ui->leAngle2->blockSignals(false);

            ui->leAngle2->update();
            break;
        }
        default:
            break;
    }
}

void LC_RotateOptionsWidget::allowSecondRotationUI(const bool enable) const {
    const bool enableSecondAngle = enable && !ui->cbFreeRefAngle->isChecked();
    ui->leAngle2->setEnabled(enableSecondAngle);
    ui->tbPickAngle2->setEnabled(enableSecondAngle);
    ui->cbTwoRotations->setEnabled(enable);
    ui->cbFreeRefAngle->setEnabled(enable);
    ui->cbAbsoluteRefAngle->setEnabled(enable);
}

void LC_RotateOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyRotate*>(a);

    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool keepOriginals = m_action->isKeepOriginals();
    const bool useMultipleCopies = m_action->isUseMultipleCopies();
    const int copiesNumber = m_action->getCopiesNumber();

    const bool twoRotations = m_action->isRotateAlsoAroundReferencePoint();
    const bool freeAngle = m_action->isFreeAngle();
    const bool relativeAngle = m_action->isRelativeAngle();
    const bool centerPointFirst = m_action->isCenterPointFirst();
    const bool freeRefAngle = m_action->isFreeRefPointAngle();
    const bool absoluteRefAngle = m_action->isRefPointAngleAbsolute();
    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const QString angle2 = fromDouble(RS_Math::rad2deg(m_action->getRefPointAngle()));

    LC_GuardedSignalsBlocker({
        ui->cbCurrentAttr,
        ui->cbKeepOriginals,
        ui->cbCurrentLayer,
        ui->cbMultipleCopies,
        ui->sbNumberOfCopies,
        ui->leAngle,
        ui->leAngle2,
        ui->cbFreeAngle,
        ui->cbRelativeAngle,
        ui->cbCenterPointFirst,
        ui->cbFreeRefAngle,
        ui->cbTwoRotations,
        ui->cbAbsoluteRefAngle
    });

    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);
    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);
    ui->leAngle->setText(angle);

    ui->cbFreeAngle->setChecked(freeAngle);
    ui->leAngle->setEnabled(!freeAngle);
    ui->tbPickAngle->setEnabled(!freeAngle);
    ui->cbRelativeAngle->setEnabled(freeAngle);
    ui->cbRelativeAngle->setChecked(relativeAngle);

    ui->cbCenterPointFirst->setChecked(centerPointFirst);

    ui->cbFreeRefAngle->setChecked(freeRefAngle);
    if (ui->cbTwoRotations->isChecked()) {
        ui->leAngle2->setEnabled(!freeRefAngle);
        ui->tbPickAngle2->setEnabled(!freeRefAngle);
    }

    ui->leAngle2->setText(angle2);
    ui->cbAbsoluteRefAngle->setChecked(absoluteRefAngle);

    allowSecondRotationUI(twoRotations);
    ui->cbTwoRotations->setEnabled(true);
    ui->cbTwoRotations->setChecked(twoRotations);
}

void LC_RotateOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::onCopiesNumberValueChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbFreeAngleClicked(const bool val) {
    m_action->setFreeAngle(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbRelativeAngleClicked(const bool val) {
    m_action->setRelativeAngle(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbCenterPointFirstClicked(const bool val) {
    m_action->setCenterPointFirst(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::cbFreeRefAngleClicked(const bool val) {
    m_action->setFreeRefPointAngle(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::onAbsoluteRefAngleClicked(const bool val) {
    m_action->setRefPointAngleAbsolute(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::onTwoRotationsClicked(const bool val) {
    m_action->setRotateAlsoAroundReferencePoint(val);
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::onRefPointAngleEditingFinished() {
    const auto val = ui->leAngle2->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setRefPointAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_RotateOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
