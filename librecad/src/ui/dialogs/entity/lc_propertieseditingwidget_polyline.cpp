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

#include "lc_propertieseditingwidget_polyline.h"

#include "rs_polyline.h"
#include "ui_lc_propertieseditingwidget_polyline.h"

LC_PropertiesEditingWidgetPolyline::LC_PropertiesEditingWidgetPolyline(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_PolylinePropertiesEditingWidget){
    ui->setupUi(this);
    connect(ui->cbClosed, &QCheckBox::toggled, this, &LC_PropertiesEditingWidgetPolyline::onClosedToggled);
}

LC_PropertiesEditingWidgetPolyline::~LC_PropertiesEditingWidgetPolyline(){
    delete ui;
}

void LC_PropertiesEditingWidgetPolyline::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Polyline*>(entity);
    toUIBool(m_entity->isClosed(), ui->cbClosed);
}

void LC_PropertiesEditingWidgetPolyline::onClosedToggled([[maybe_unused]]bool checked) const {
    m_entity->setClosed(ui->cbClosed->isChecked(),0);

    // fixme - sand move to upper level in call hierarchy
    m_entity->update();
    m_entity->calculateBorders();
}
