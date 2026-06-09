/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_paste_to_points_options_widget.h"

#include "lc_action_edit_paste_to_points.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_paste_to_points_options_widget.h"

LC_PasteToPointsOptionsWidget::LC_PasteToPointsOptionsWidget():ui(new Ui::LC_PasteToPointsOptionsWidget) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PasteToPointsOptionsWidget::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_PasteToPointsOptionsWidget::onFactorEditingFinished);
    connect(ui->cbRemovePoint, &QCheckBox::clicked, this, &LC_PasteToPointsOptionsWidget::onRemovePointsClicked);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

LC_PasteToPointsOptionsWidget::~LC_PasteToPointsOptionsWidget() {
    delete ui;
}

void LC_PasteToPointsOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionPasteToPoints*>(a);

    const QString angle = fromDouble(RS_Math::rad2deg(m_action->getAngle()));
    const QString factor = fromDouble(m_action->getScaleFactor());
    const bool removePoints = m_action->isRemovePointAfterPaste();

    LC_GuardedSignalsBlocker({ui->leAngle, ui->leFactor,  ui->cbRemovePoint});
    ui->leAngle->setText(angle);
    ui->leFactor->setText(factor);
    ui->cbRemovePoint->setChecked(removePoints);
}

void LC_PasteToPointsOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angle;
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setAngle(RS_Math::deg2rad(angle));
    }
    m_action->updateOptions();
}

void LC_PasteToPointsOptionsWidget::onFactorEditingFinished() {
    const auto val = ui->leFactor->text();
    double y;
    if (toDouble(val, y, 1.0, true)) {
        m_action->setScaleFactor(y);
        ui->leFactor->setText(fromDouble(y));
    }
    m_action->updateOptions();
}

void LC_PasteToPointsOptionsWidget::onRemovePointsClicked(const bool clicked) {
    m_action->setRemovePointAfterPaste(clicked);
    m_action->updateOptions();
}

void LC_PasteToPointsOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
