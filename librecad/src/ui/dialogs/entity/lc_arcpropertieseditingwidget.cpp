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

#include "lc_arcpropertieseditingwidget.h"

#include "rs_arc.h"
#include "ui_lc_arcpropertieseditingwidget.h"

LC_ArcPropertiesEditingWidget::LC_ArcPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_ArcPropertiesEditingWidget){
    ui->setupUi(this);

    connect(ui->leCenterX, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leCenterY, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onRadiusEditingFinished);
    connect(ui->leAngle1, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onAngle1EditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onAngle2EditingFinished);
    connect(ui->cbReversed, &QCheckBox::toggled, this, &LC_ArcPropertiesEditingWidget::onReversedToggled);
}

LC_ArcPropertiesEditingWidget::~LC_ArcPropertiesEditingWidget(){
    delete ui;
}

void LC_ArcPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Arc*>(entity);

    toUI(m_entity->getCenter(), ui->leCenterX, ui->leCenterY);
    toUIValue(m_entity->getRadius(), ui->leRadius);
    toUIAngleDeg(m_entity->getAngle1(), ui->leAngle1);
    toUIAngleDeg(m_entity->getAngle2(), ui->leAngle2);
    toUIBool(m_entity->isReversed(), ui->cbReversed);
}

void LC_ArcPropertiesEditingWidget::onCenterEditingFinished() {
    m_entity->setCenter(toWCS(ui->leCenterX, ui->leCenterY, m_entity->getCenter()));
}

void LC_ArcPropertiesEditingWidget::onRadiusEditingFinished() {
    m_entity->setRadius(toWCSValue(ui->leRadius, m_entity->getRadius()));
}

void LC_ArcPropertiesEditingWidget::onAngle1EditingFinished() {
    m_entity->setAngle1(toWCSAngle(ui->leAngle1, m_entity->getAngle1()));
}

void LC_ArcPropertiesEditingWidget::onAngle2EditingFinished() {
    m_entity->setAngle2(toWCSAngle(ui->leAngle2, m_entity->getAngle2()));
}

void LC_ArcPropertiesEditingWidget::onReversedToggled([[maybe_unused]]bool checked) {
    if (m_entity->isReversed() != ui->cbReversed->isChecked()) {
        m_entity->revertDirection();
    }
}

void LC_ArcPropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickRadius,  "radius", ui->leRadius);
    pickAngleSetup(ui->tbPickStartAngle,  "startAngle", ui->leAngle1);
    pickAngleSetup(ui->tbPickEndAngle, "endAngle",  ui->leAngle2);
}
