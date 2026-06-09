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

#include "lc_ucs_create_options_widget.h"

#include "lc_actionucscreate.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_ucs_create_options_widget.h"

LC_UCSCreateOptionsWidget::LC_UCSCreateOptionsWidget(): ui(new Ui::LC_UCSCreateOptionsWidget) {
    ui->setupUi(this);
    connect(ui->cbFreeAngle, &QCheckBox::clicked, this, &LC_UCSCreateOptionsWidget::cbFreeAngleClicked);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_UCSCreateOptionsWidget::onAngleEditingFinished);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

LC_UCSCreateOptionsWidget::~LC_UCSCreateOptionsWidget() {
    delete ui;
}

void LC_UCSCreateOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionUCSCreate*>(a);

    const bool freeAngle = !m_action->isFixedAngle();
    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));

    LC_GuardedSignalsBlocker({ui->leAngle, ui->cbFreeAngle});

    ui->leAngle->setText(angle);
    ui->cbFreeAngle->setChecked(freeAngle);
    const bool enableAngle = !freeAngle;
    ui->leAngle->setEnabled(enableAngle);
    ui->tbPickAngle->setEnabled(enableAngle);
}

void LC_UCSCreateOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_UCSCreateOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_UCSCreateOptionsWidget::cbFreeAngleClicked(const bool val) {
    m_action->setFixedAngle(!val);
    m_action->updateOptions();
}
