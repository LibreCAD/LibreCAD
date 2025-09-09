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

#include "lc_insertpropertieseditingwidget.h"

#include "rs_insert.h"
#include "rs_math.h"
#include "ui_lc_insertpropertieseditingwidget.h"

#define ALLOW_PICK_SCALE false

LC_InsertPropertiesEditingWidget::LC_InsertPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_InsertPropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->leInsertionPointX, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onInsertionPointEditingFinished);
    connect(ui->leInsertionPointY, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onInsertionPointEditingFinished);
    connect(ui->leScaleX, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onScaleEditingFinishedChanged);
    connect(ui->leScaleY, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onScaleEditingFinishedChanged);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->leRows, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onRowsEditingFinished);
    connect(ui->leCols, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onRowsEditingFinished);
    connect(ui->leColSpacing, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onSpacingEditingFinished);
    connect(ui->leRowSpacing, &QLineEdit::editingFinished, this, &LC_InsertPropertiesEditingWidget::onSpacingEditingFinished);
    if (!ALLOW_PICK_SCALE) {
        ui->tbPIckScaleX->setVisible(false);
        ui->tbPIckScaleY->setVisible(false);
    }
}

LC_InsertPropertiesEditingWidget::~LC_InsertPropertiesEditingWidget(){
    delete ui;
}

void LC_InsertPropertiesEditingWidget::onInsertionPointEditingFinished() {
    m_entity->setInsertionPoint(toWCS(ui->leInsertionPointX, ui->leInsertionPointY, m_entity->getInsertionPoint()));
}

void LC_InsertPropertiesEditingWidget::onScaleEditingFinishedChanged() {
    m_entity->setScale(toWCSRaw(ui->leScaleX,ui->leScaleY, m_entity->getScale()));
}

void LC_InsertPropertiesEditingWidget::onAngleEditingFinished() {
    m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
}

void LC_InsertPropertiesEditingWidget::onRowsEditingFinished() {
    m_entity->setRows(RS_Math::round(RS_Math::eval(ui->leRows->text())));
}

void LC_InsertPropertiesEditingWidget::onColsEditingFinished() {
    m_entity->setCols(RS_Math::round(RS_Math::eval(ui->leCols->text())));
}

void LC_InsertPropertiesEditingWidget::onSpacingEditingFinished() {
    m_entity->setSpacing(RS_Vector(RS_Math::eval(ui->leColSpacing->text()),
                                 RS_Math::eval(ui->leRowSpacing->text())));
}

void LC_InsertPropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickInsertionPoint, "insert", ui->leInsertionPointX, ui->leInsertionPointY);
    if (!ALLOW_PICK_SCALE) {
        pickDistanceSetup(ui->tbPIckScaleX,  "scaleX",ui->leScaleX);
        pickDistanceSetup(ui->tbPIckScaleY, "scaleY",ui->leScaleY);
    }
    pickAngleSetup(ui->tbPickAngle, "angle",ui->leAngle);
    pickDistanceSetup(ui->tbPickRowSpacing,  "rowSpacing",ui->leRowSpacing);
    pickDistanceSetup(ui->tbPickRowSpacing,  "colSpacing",ui->leColSpacing);
}

void LC_InsertPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Insert*>(entity);

    toUI(m_entity->getInsertionPoint(), ui->leInsertionPointX, ui->leInsertionPointY);
    toUIRaw(m_entity->getScale(), ui->leScaleX, ui->leScaleY);
    toUIAngleDeg(m_entity->getAngle(), ui->leAngle);

    QString s;
    s.setNum(m_entity->getRows());
    ui->leRows->setText(s);
    s.setNum(m_entity->getCols());
    ui->leCols->setText(s);
    s.setNum(m_entity->getSpacing().y);
    ui->leRowSpacing->setText(s);
    s.setNum(m_entity->getSpacing().x);
    ui->leColSpacing->setText(s);
}
