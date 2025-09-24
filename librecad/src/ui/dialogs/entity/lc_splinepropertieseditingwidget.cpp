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

#include "lc_splinepropertieseditingwidget.h"

#include "rs_math.h"
#include "rs_spline.h"
#include "ui_lc_splinepropertieseditingwidget.h"

LC_SplinePropertiesEditingWidget::LC_SplinePropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_SplinePropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->cbClosed, &QCheckBox::toggled, this, &LC_SplinePropertiesEditingWidget::onClosedToggled);
    connect(ui->cbDegree, &QComboBox::currentIndexChanged, this, &LC_SplinePropertiesEditingWidget::onDegreeIndexChanged);
}

LC_SplinePropertiesEditingWidget::~LC_SplinePropertiesEditingWidget(){
    delete ui;
}

void LC_SplinePropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Spline*>(entity);
    QString s;
    s.setNum(m_entity->getDegree());
    ui->cbDegree->setCurrentIndex(ui->cbDegree->findText(s) );

    toUIBool(m_entity->isClosed(), ui->cbClosed);
}

void LC_SplinePropertiesEditingWidget::onClosedToggled([[maybe_unused]]bool checked) {
    m_entity->setClosed(ui->cbClosed->isChecked());
}

void LC_SplinePropertiesEditingWidget::onDegreeIndexChanged([[maybe_unused]]int index) {
    m_entity->setDegree(RS_Math::round(RS_Math::eval(ui->cbDegree->currentText())));
}
