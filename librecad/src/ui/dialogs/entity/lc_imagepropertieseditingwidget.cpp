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

#include "lc_imagepropertieseditingwidget.h"

#include <QFileInfo>

#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_image.h"
#include "rs_units.h"
#include "ui_lc_imagepropertieseditingwidget.h"

#define ALLOW_PICK_SCALE false

LC_ImagePropertiesEditingWidget::LC_ImagePropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_ImagePropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->leInsertX, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onInsertionPointEditingFinished);
    connect(ui->leInsertY, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onInsertionPointEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->leWidth, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onWidthChanged);
    connect(ui->leHeight, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onHeightChanged);
    connect(ui->leScale, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onScaleChanged);
    connect(ui->leDPI, &QLineEdit::editingFinished, this, &LC_ImagePropertiesEditingWidget::onDPIChanged);
    connect(ui->pbFile, &QPushButton::toggled, this,&LC_ImagePropertiesEditingWidget::onImageFileClick);
    connect(ui->lePath, &QLineEdit::textChanged, this, &LC_ImagePropertiesEditingWidget::onPathChanged);
    if (!ALLOW_PICK_SCALE) {
        ui->tbPickScale->setVisible(false);
    }
}

LC_ImagePropertiesEditingWidget::~LC_ImagePropertiesEditingWidget(){
    delete ui;
}

void LC_ImagePropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Image*>(entity);
}


void LC_ImagePropertiesEditingWidget::onWidthChanged() {
    double width = toWCSValue(ui->leWidth, m_entity->getWidth());
    m_scale = width / m_entity->getWidth();

    toUIValue(m_entity->getHeight()*m_scale, ui->leHeight);
    toUIValue(m_scale, ui->leScale);
}

void LC_ImagePropertiesEditingWidget::onHeightChanged() {
    double height = toWCSValue(ui->leHeight, m_entity->getHeight());
    m_scale = height / m_entity->getHeight();

    toUIValue(m_entity->getWidth()*m_scale, ui->leWidth);
    toUIValue(m_scale, ui->leScale);
}

void LC_ImagePropertiesEditingWidget::onScaleChanged() {
    m_scale = toWCSValue(ui->leScale, m_scale);
    toUIValue(m_entity->getWidth()*m_scale, ui->leWidth);
    toUIValue(m_entity->getHeight()*m_scale, ui->leHeight);
    toUIValue(RS_Units::scaleToDpi(m_scale, m_entity->getGraphicUnit()), ui->leDPI);
}

void LC_ImagePropertiesEditingWidget::onDPIChanged(){
    double oldDpi = RS_Units::scaleToDpi(m_scale, m_entity->getGraphicUnit()); // todo - what if scale was changed? Save dpi in dlg?
    double dpi = toWCSValue(ui->leDPI, oldDpi);
    m_scale = RS_Units::dpiToScale(dpi, m_entity->getGraphicUnit());
    toUIValue(m_scale, ui->leScale);
    toUIValue(m_entity->getWidth()*m_scale, ui->leWidth);
    toUIValue(m_entity->getHeight()*m_scale, ui->leHeight);
}

void LC_ImagePropertiesEditingWidget::onInsertionPointEditingFinished() {
    m_entity->setInsertionPoint(toWCS(ui->leInsertX, ui->leInsertY, m_entity->getInsertionPoint()));
}

void LC_ImagePropertiesEditingWidget::onAngleEditingFinished() {
    double orgScale = m_entity->getUVector().magnitude();
    m_scale /= orgScale;
    double orgAngle = m_entity->getUVector().angle();
    double angle = toWCSAngle(ui->leAngle, orgAngle);
    m_entity->scale(m_entity->getInsertionPoint(), RS_Vector(m_scale, m_scale));
    m_entity->rotate(m_entity->getInsertionPoint(), angle - orgAngle);
}

void LC_ImagePropertiesEditingWidget::onImageFileClick() {
    ui->lePath->setText(RS_DIALOGFACTORY->requestImageOpenDialog()); // fixme - is it bad dependency?
}

void LC_ImagePropertiesEditingWidget::onPathChanged(const QString&) {
    auto text = ui->lePath->text().trimmed();
    if (QFileInfo(text).isFile()) {
        m_entity->setFile(text);
    }
}

void LC_ImagePropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickInsertionPoint, "insert", ui->leInsertX, ui->leInsertY);
    pickDistanceSetup(ui->tbPickWidth, "width", ui->leWidth);
    pickDistanceSetup(ui->tbPickHeight, "height", ui->leHeight);
    pickAngleSetup(ui->tbPickAngle, "angle", ui->leAngle);
    if (!ALLOW_PICK_SCALE) {
        pickDistanceSetup(ui->tbPickScale, "scale", ui->leScale);
    }
}
