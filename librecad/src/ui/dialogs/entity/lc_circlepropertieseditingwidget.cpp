/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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
#include <limits>
#include <QDoubleValidator>

#include "lc_circlepropertieseditingwidget.h"

#include "rs_circle.h"
#include "ui_lc_circlepropertieseditingwidget.h"

LC_CirclePropertiesEditingWidget::LC_CirclePropertiesEditingWidget(QWidget* parent)
    : LC_EntityPropertiesEditorWidget(parent)
      , ui(new Ui::LC_CirclePropertiesEditingWidget) {
    ui->setupUi(this);

    // Set up validators for input fields
    auto coordValidator = new QDoubleValidator(this);
    ui->leCenterX->setValidator(coordValidator);
    ui->leCenterY->setValidator(coordValidator);

    auto positiveValidator = new QDoubleValidator(0.0, std::numeric_limits<double>::max(), 10, this);
    ui->leRadius->setValidator(positiveValidator);
    ui->leDiameter->setValidator(positiveValidator);

    connect(ui->leCenterX, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leCenterY, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onRadiusEditingFinished);
    connect(ui->leDiameter, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onDiameterEditingFinished);
    connect(ui->leRadius, &QLineEdit::textChanged, this, &LC_CirclePropertiesEditingWidget::onRadiusTextChanged);
    connect(ui->leDiameter, &QLineEdit::textChanged, this, &LC_CirclePropertiesEditingWidget::onDiameterTextChanged);
}

LC_CirclePropertiesEditingWidget::~LC_CirclePropertiesEditingWidget(){
    delete ui;
}

void LC_CirclePropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Circle*>(entity);

    ui->leRadius->blockSignals(true);
    ui->leDiameter->blockSignals(true);
    toUI(m_entity->getCenter(), ui->leCenterX, ui->leCenterY);
    toUIValue(m_entity->getRadius(), ui->leRadius);
    toUIValue(m_entity->getRadius() * 2, ui->leDiameter);
    ui->leRadius->blockSignals(false);
    ui->leDiameter->blockSignals(false);
}

void LC_CirclePropertiesEditingWidget::onCenterEditingFinished() {
    m_entity->setCenter(toWCS(ui->leCenterX, ui->leCenterY, m_entity->getCenter()));
}

void LC_CirclePropertiesEditingWidget::onRadiusEditingFinished() {
    double newRadius = toWCSValue(ui->leRadius, m_entity->getRadius());
    if (newRadius > 0.0) {  // Ensure positive radius
        m_entity->setRadius(newRadius);
        ui->leDiameter->blockSignals(true);
        toUIValue(newRadius * 2, ui->leDiameter);
        ui->leDiameter->blockSignals(false);
    } else {
        // Revert to current value if invalid
        toUIValue(m_entity->getRadius(), ui->leRadius);
    }
}

void LC_CirclePropertiesEditingWidget::onDiameterEditingFinished() {
    double newDiameter = toWCSValue(ui->leDiameter, m_entity->getRadius() * 2);
    double newRadius = newDiameter / 2.0;
    if (newRadius > 0.0) {  // Ensure positive radius
        m_entity->setRadius(newRadius);
        ui->leRadius->blockSignals(true);
        toUIValue(newRadius, ui->leRadius);
        ui->leRadius->blockSignals(false);
    } else {
        // Revert to current value if invalid
        toUIValue(m_entity->getRadius() * 2, ui->leDiameter);
    }
}

void LC_CirclePropertiesEditingWidget::onRadiusTextChanged(const QString &text) {
    int pos = 0;
    if (ui->leRadius->validator()->validate(const_cast<QString&>(text), pos) == QValidator::Acceptable) {
        bool ok;
        double value = text.toDouble(&ok);
        if (ok && value > 0.0) {
            ui->leDiameter->blockSignals(true);
            toUIValue(value * 2, ui->leDiameter);
            ui->leDiameter->blockSignals(false);
        }
    }
}

void LC_CirclePropertiesEditingWidget::onDiameterTextChanged(const QString &text) {
    int pos = 0;
    if (ui->leDiameter->validator()->validate(const_cast<QString&>(text), pos) == QValidator::Acceptable) {
        bool ok;
        double value = text.toDouble(&ok);
        if (ok && value > 0.0) {
            ui->leRadius->blockSignals(true);
            toUIValue(value / 2.0, ui->leRadius);
            ui->leRadius->blockSignals(false);
        }
    }
}

void LC_CirclePropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickRadius, "radius", ui->leRadius);
    pickDistanceSetup(ui->tbPickDiameter, "diameter", ui->leDiameter);
}
