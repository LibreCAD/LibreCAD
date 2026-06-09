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

#include "lc_rotate_2_options_widget.h"

#include "lc_action_modify_rotate_twice.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_rotate_2_options_widget.h"

// todo - potentially, instead of specifying secondary angle it is possible to let the user enter the sum of angles
// todo - and calculate secondary angle based on angle1 and that sum.
// todo - Such approach will simplify ui (as sum defines resulting angle of entity) - yet will break compatibility with
// todo - previous versions. I'm not sure that this is popular command, yet still...
// fixme - think and decide which way of setting secondary angle is more convenient...

LC_Rotate2OptionsWidget::LC_Rotate2OptionsWidget(): ui(new Ui::LC_Rotate2OptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbMultipleCopies, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbMultipleCopiesClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbCurrentLayer, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->cbSameAngleForCopies, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbSameAngleForCopiesClicked);
    connect(ui->cbAnglesMirrored, &QCheckBox::clicked, this, &LC_Rotate2OptionsWidget::cbAnglesMirroredClicked);
    connect(ui->leAngle1, &QLineEdit::editingFinished, this, &LC_Rotate2OptionsWidget::onAngle1EditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_Rotate2OptionsWidget::onAngle2EditingFinished);
    connect(ui->sbNumberOfCopies, &QSpinBox::valueChanged, this, &LC_Rotate2OptionsWidget::onCopiesCountChanged);

    // fixme - remove later if the control will not be reused for some other flag
    ui->cbSameAngleForCopies->hide();

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle1);
    pickAngleSetup("angle2", ui->tbPickAngle2, ui->leAngle2);
}

LC_Rotate2OptionsWidget::~LC_Rotate2OptionsWidget() {
    delete ui;
}

void LC_Rotate2OptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionModifyRotateTwice*>(a);

    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool keepOriginals = m_action->isKeepOriginals();
    const bool useMultipleCopies = m_action->isUseMultipleCopies();
    const int copiesNumber = m_action->getCopiesNumber();
    const bool sameAngle = m_action->isUseSameAngle2ForCopies();
    const bool mirrorAngles = m_action->isMirrorAngles();
    const QString angle1 = fromDouble(RS_Math::rad2deg(m_action->getAngle1()));
    const QString angle2 = fromDouble(RS_Math::rad2deg(m_action->getAngle2()));

    LC_GuardedSignalsBlocker({
        ui->cbCurrentAttr,
        ui->cbKeepOriginals,
        ui->cbCurrentLayer,
        ui->cbMultipleCopies,
        ui->sbNumberOfCopies,
        ui->cbSameAngleForCopies,
        ui->leAngle1,
        ui->leAngle2,
        ui->cbAnglesMirrored
    });

    ui->cbMultipleCopies->setChecked(useMultipleCopies);
    ui->sbNumberOfCopies->setEnabled(useMultipleCopies);
    ui->sbNumberOfCopies->setValue(copiesNumber);
    ui->cbCurrentLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbKeepOriginals->setChecked(keepOriginals);

    ui->cbSameAngleForCopies->setChecked(sameAngle);

    ui->cbAnglesMirrored->setChecked(mirrorAngles);
    ui->leAngle2->setEnabled(!mirrorAngles);
    ui->tbPickAngle2->setEnabled(!mirrorAngles);

    ui->leAngle1->setText(angle1);
    ui->leAngle2->setText(angle2);
}

void LC_Rotate2OptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::cbMultipleCopiesClicked(const bool val) {
    m_action->setUseMultipleCopies(val);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::onCopiesCountChanged(int number) {
    if (number < 1) {
        number = 1;
    }
    m_action->setCopiesNumber(number);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::onAngle1EditingFinished() {
    const auto val = ui->leAngle1->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle1(RS_Math::deg2rad(angle));
        const bool anglesMirrored = ui->cbAnglesMirrored->isChecked();
        if (anglesMirrored) {
            m_action->setAngle2(RS_Math::deg2rad(-angle));
        }
    }
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::onAngle2EditingFinished() {
    const QString& val = ui->leAngle2->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle2(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::cbSameAngleForCopiesClicked(const bool val) {
    m_action->setUseSameAngle2ForCopies(val);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::cbAnglesMirroredClicked(const bool checked) {
    m_action->setMirrorAngles(checked);
    m_action->updateOptions();
}

void LC_Rotate2OptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
