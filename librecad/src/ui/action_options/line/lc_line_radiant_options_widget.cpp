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

#include "lc_line_radiant_options_widget.h"

#include "lc_action_draw_line_radiant.h"
#include "lc_guarded_signals_blocker.h"
#include "rs_settings.h"
#include "ui_lc_line_radiant_options_widget.h"

LC_LineRadiantOptionsWidget::LC_LineRadiantOptionsWidget(): ui(new Ui::LC_LineRadiantOptionsWidget){
    ui->setupUi(this);
    connect(ui->cbLengthType, &QComboBox::currentIndexChanged, this, &LC_LineRadiantOptionsWidget::onLengthTypeIndexChanged);
    connect(ui->cbPointSelector, &QComboBox::currentIndexChanged, this, &LC_LineRadiantOptionsWidget::onActivePointIndexChanged);
    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_LineRadiantOptionsWidget::onXEditingFinished);
    connect(ui->leY, &QLineEdit::editingFinished, this, &LC_LineRadiantOptionsWidget::onYEditingFinished);
    connect(ui->leLength, &QLineEdit::editingFinished, this, &LC_LineRadiantOptionsWidget::onLengthEditingFinished);

    connectInteractiveInputButton(ui->tbPickX, LC_ActionContext::InteractiveInputInfo::POINT_X, "pointX");
    ui->leX->setProperty("_tagHolder", "pointX");

    connectInteractiveInputButton(ui->tbPickY, LC_ActionContext::InteractiveInputInfo::POINT_Y, "pointY");
    ui->leY->setProperty("_tagHolder", "pointY");

    connectInteractiveInputButton(ui->tbPickLength, LC_ActionContext::InteractiveInputInfo::DISTANCE, "length");
    ui->leLength->setProperty("_tagHolder", "length");

    connectInteractiveInputButton(ui->tbPickPoint, LC_ActionContext::InteractiveInputInfo::POINT, "farPoint");
}

LC_LineRadiantOptionsWidget::~LC_LineRadiantOptionsWidget(){
    delete ui;
    m_action = nullptr;
}

void LC_LineRadiantOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawLineRadiant *>(a);
    const double len = m_action->getLength();
    const int lengthType = m_action->getLenghType();
    const LC_ActionDrawLineRadiant::RadiantIdx pointId = m_action->getActiveRadiantIndex();
    const RS_Vector activePoint = m_action->getActiveRadiant();

    LC_GuardedSignalsBlocker({ui->cbPointSelector, ui->leX, ui->leY, ui->cbLengthType, ui->leLength});

    const bool lenValueEnabled = lengthType != 3;
    ui->leLength->setEnabled(lenValueEnabled);
    ui->leLength->setText(fromDouble(len));
    ui->tbPickLength->setEnabled(lenValueEnabled);
    ui->cbLengthType->setCurrentIndex(lengthType);

    const QString pointX = fromDouble(activePoint.x);
    const QString pointY = fromDouble(activePoint.y);
    ui->leX->setText(pointX);
    ui->leY->setText(pointY);
    ui->cbPointSelector->setCurrentIndex(pointId);
}

void LC_LineRadiantOptionsWidget::onActivePointIndexChanged(int index) {
    const auto pointIdx = static_cast<LC_ActionDrawLineRadiant::RadiantIdx>(index);
    m_action->setActiveRadiantIndex(pointIdx);
    m_action->updateOptions();
}

void LC_LineRadiantOptionsWidget::onLengthTypeIndexChanged(int index) {
    const auto lengthType = static_cast<LC_ActionDrawLineRadiant::LenghtType>(index);
    m_action->setLengthType(lengthType);
    m_action->updateOptions();
}

void LC_LineRadiantOptionsWidget::onXEditingFinished() {
    const QString val = ui->leX->text();
    double value;
    if (toDouble(val, value, 0.0, false)) {
        m_action->setActiveX(value);
        m_action->updateOptions();
    }
    else {
        // fixme - mark control as invalid visual? Create special editor with validator?
        ui->leX->blockSignals(true);
        ui->leX->setText(fromDouble(m_action->getActiveX()));
        ui->leX->blockSignals(false);
    }
}

void LC_LineRadiantOptionsWidget::onYEditingFinished() {
    const QString val = ui->leY->text();
    double value;
    if (toDouble(val, value, 0.0, false)) {
        m_action->setActiveY(value);
        m_action->updateOptions();
    }
    else {
        ui->leY->blockSignals(true);
        ui->leY->setText(fromDouble(m_action->getActiveY()));
        ui->leY->blockSignals(false);
    }
}

void LC_LineRadiantOptionsWidget::onLengthEditingFinished() {
    const QString val = ui->leLength->text();
    double value;
    if (toDouble(val, value, 0.0, false)) {
        m_action->setLength(value);
        m_action->updateOptions();
    }
    else {
        ui->leLength->blockSignals(true);
        ui->leLength->setText(fromDouble(m_action->getLength()));
        ui->leLength->blockSignals(false);
    }
}

void LC_LineRadiantOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
