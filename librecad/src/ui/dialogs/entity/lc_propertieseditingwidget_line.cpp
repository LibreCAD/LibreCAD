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

#include "lc_propertieseditingwidget_line.h"

#include "rs_line.h"
#include "ui_lc_propertieseditingwidget_line.h"

LC_PropertiesEditingWidgetLine::LC_PropertiesEditingWidgetLine(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_LinePropertiesEditingWidget){

    ui->setupUi(this);

    connect(ui->leStartX, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetLine::onStartEditingFinished);
    connect(ui->leStartY, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetLine::onStartEditingFinished);
    connect(ui->leEndX, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetLine::onEndEditingFinished);
    connect(ui->leEndY, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetLine::onEndEditingFinished);
}

LC_PropertiesEditingWidgetLine::~LC_PropertiesEditingWidgetLine(){
    delete ui;
}

void LC_PropertiesEditingWidgetLine::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointStart, "startPoint", ui->leStartX, ui->leStartY);
    pickPointSetup(ui->wPickPointEnd, "endPoint", ui->leEndX, ui->leEndY);
}

void LC_PropertiesEditingWidgetLine::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Line*>(entity);
    toUI(m_entity->getStartpoint(), ui->leStartX, ui->leStartY);
    toUI(m_entity->getEndpoint(), ui->leEndX, ui->leEndY);
}

void LC_PropertiesEditingWidgetLine::onStartEditingFinished() const {
    m_entity->setStartpoint(toWCS(ui->leStartX, ui->leStartY, m_entity->getStartpoint()));
}

void LC_PropertiesEditingWidgetLine::onEndEditingFinished() const {
    m_entity->setEndpoint(toWCS(ui->leEndX, ui->leEndY, m_entity->getStartpoint()));
}
