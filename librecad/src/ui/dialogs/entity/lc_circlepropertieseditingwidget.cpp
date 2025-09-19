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

#include "lc_circlepropertieseditingwidget.h"

#include "rs_circle.h"
#include "ui_lc_circlepropertieseditingwidget.h"

LC_CirclePropertiesEditingWidget::LC_CirclePropertiesEditingWidget(QWidget* parent)
    : LC_EntityPropertiesEditorWidget(parent)
      , ui(new Ui::LC_CirclePropertiesEditingWidget) {
    ui->setupUi(this);

    connect(ui->leCenterX, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leCenterY, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_CirclePropertiesEditingWidget::onRadiusEditingFinished);
}

LC_CirclePropertiesEditingWidget::~LC_CirclePropertiesEditingWidget(){
    delete ui;
}

void LC_CirclePropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Circle*>(entity);

    toUI(m_entity->getCenter(), ui->leCenterX, ui->leCenterY);
    toUIValue(m_entity->getRadius(), ui->leRadius);
}

void LC_CirclePropertiesEditingWidget::onCenterEditingFinished() {
    m_entity->setCenter(toWCS(ui->leCenterX, ui->leCenterY, m_entity->getCenter()));
}

void LC_CirclePropertiesEditingWidget::onRadiusEditingFinished() {
    m_entity->setRadius(toWCSValue(ui->leRadius, m_entity->getRadius()));
}

void LC_CirclePropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickRadius, "radius", ui->leRadius);
}
