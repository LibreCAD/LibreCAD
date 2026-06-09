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

#include "lc_propertieseditingwidget_insert.h"

#include "rs_insert.h"
#include "rs_math.h"
#include "ui_lc_propertieseditingwidget_insert.h"

#define ALLOW_PICK_SCALE false

LC_PropertiesEditingWidgetInsert::LC_PropertiesEditingWidgetInsert(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_InsertPropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->leInsertionPointX, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onInsertionPointEditingFinished);
    connect(ui->leInsertionPointY, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onInsertionPointEditingFinished);
    connect(ui->leScaleX, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onScaleEditingFinishedChanged);
    connect(ui->leScaleY, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onScaleEditingFinishedChanged);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onAngleEditingFinished);
    connect(ui->leRows, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onRowsEditingFinished);
    connect(ui->leCols, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onRowsEditingFinished);
    connect(ui->leColSpacing, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onSpacingEditingFinished);
    connect(ui->leRowSpacing, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetInsert::onSpacingEditingFinished);
    if (!ALLOW_PICK_SCALE) {
        ui->tbPIckScaleX->setVisible(false);
        ui->tbPIckScaleY->setVisible(false);
    }
}

LC_PropertiesEditingWidgetInsert::~LC_PropertiesEditingWidgetInsert(){
    delete ui;
}

void LC_PropertiesEditingWidgetInsert::onInsertionPointEditingFinished() const {
    m_entity->setInsertionPoint(toWCS(ui->leInsertionPointX, ui->leInsertionPointY, m_entity->getInsertionPoint()));
}

void LC_PropertiesEditingWidgetInsert::onScaleEditingFinishedChanged() const {
    m_entity->setScale(toWCSRaw(ui->leScaleX,ui->leScaleY, m_entity->getScale()));
}

void LC_PropertiesEditingWidgetInsert::onAngleEditingFinished() const {
    m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
}

void LC_PropertiesEditingWidgetInsert::onRowsEditingFinished() const {
    m_entity->setRows(RS_Math::round(RS_Math::eval(ui->leRows->text())));
}

void LC_PropertiesEditingWidgetInsert::onColsEditingFinished() const {
    m_entity->setCols(RS_Math::round(RS_Math::eval(ui->leCols->text())));
}

void LC_PropertiesEditingWidgetInsert::onSpacingEditingFinished() const {
    m_entity->setSpacing(RS_Vector(RS_Math::eval(ui->leColSpacing->text()),
                                 RS_Math::eval(ui->leRowSpacing->text())));
}

void LC_PropertiesEditingWidgetInsert::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickInsertionPoint, "insert", ui->leInsertionPointX, ui->leInsertionPointY);
    if (!ALLOW_PICK_SCALE) {
        pickDistanceSetup(ui->tbPIckScaleX,  "scaleX",ui->leScaleX);
        pickDistanceSetup(ui->tbPIckScaleY, "scaleY",ui->leScaleY);
    }
    pickAngleSetup(ui->tbPickAngle, "angle",ui->leAngle);
    pickDistanceSetup(ui->tbPickRowSpacing,  "rowSpacing",ui->leRowSpacing);
    pickDistanceSetup(ui->tbPickRowSpacing,  "colSpacing",ui->leColSpacing);
}

void LC_PropertiesEditingWidgetInsert::setEntity(RS_Entity* entity) {
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
