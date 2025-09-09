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

#include "lc_pointpropertieseditingwidget.h"

#include "rs_point.h"
#include "ui_lc_pointpropertieseditingwidget.h"

LC_PointPropertiesEditingWidget::LC_PointPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_PointPropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->lePosX, &QLineEdit::editingFinished, this, &LC_PointPropertiesEditingWidget::onPosXEditingFinished);
    connect(ui->lePosX, &QLineEdit::editingFinished, this, &LC_PointPropertiesEditingWidget::onPosYEditingFinished);
}

LC_PointPropertiesEditingWidget::~LC_PointPropertiesEditingWidget(){
    delete ui;
}

void LC_PointPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Point*>(entity);
    toUI(m_entity->getPos(), ui->lePosX, ui->lePosY);
}

void LC_PointPropertiesEditingWidget::onPosXEditingFinished() {
    m_entity->setPos(toWCS(ui->lePosX, ui->lePosY, m_entity->getPos()));
}

void LC_PointPropertiesEditingWidget::onPosYEditingFinished() {
    m_entity->setPos(toWCS(ui->lePosX, ui->lePosY, m_entity->getPos()));
}
