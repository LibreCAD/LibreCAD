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

#include "../dialogs/entity/lc_dlgdimordinate.h"

#include "lc_dimordinate.h"
#include "rs_graphic.h"
#include "ui_lc_dlgdimordinate.h"

LC_DlgDimOrdinate::LC_DlgDimOrdinate(QWidget* parent, LC_GraphicViewport* viewport, LC_DimOrdinate* dim)
    : LC_EntityPropertiesDlg(parent, "DlgDimOrdinate", viewport)
      , ui(new Ui::LC_DlgDimOrdinate) {
    ui->setupUi(this);
    setEntity(dim);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_DlgDimOrdinate::accept);
}

LC_DlgDimOrdinate::~LC_DlgDimOrdinate() {
    delete ui;
}

void LC_DlgDimOrdinate::languageChange(){
    ui->retranslateUi(this);
}

void LC_DlgDimOrdinate::setEntity(LC_DimOrdinate* d) {
    m_entity = d;

    RS_Graphic* graphic = m_entity->getGraphic();
    if (graphic) {
        ui->cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = m_entity->getLayer(false);
    if (lay) {
        ui->cbLayer->setLayer(*lay);
    }

    ui->wPen->setPen(m_entity, lay, tr("Pen"));
    ui->wLabel->setLabel(m_entity->getLabel(false));
    bool ordinateX = m_entity->isForXDirection();
    ui->rbOrdinateX->setChecked(ordinateX);
    ui->rbOrdinateY->setChecked(!ordinateX);
}

void LC_DlgDimOrdinate::updateEntity() {
    m_entity->setLabel(ui->wLabel->getLabel());
    m_entity->setForXDirection(ui->rbOrdinateX->isChecked());

    m_entity->setPen(ui->wPen->getPen());
    m_entity->setLayer(ui->cbLayer->getLayer());

    m_entity->updateDim(true);
}
