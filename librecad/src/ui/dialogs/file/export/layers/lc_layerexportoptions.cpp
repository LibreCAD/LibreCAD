/*
 * **************************************************************************
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
 * *********************************************************************
 */

#include "lc_layerexportoptions.h"

#include "lc_layersexporter.h"
#include "rs_settings.h"
#include "ui_lc_layerexportoptions.h"

LC_LayerExportOptionsWidget::LC_LayerExportOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_LayerExportOptionsWidget){

    ui->setupUi(this);
    connect(ui->cbCreateSeparateDrawingForLayer, &QCheckBox::toggled, this,
            &LC_LayerExportOptionsWidget::onCreateSeparateDrawingToggled);
    loadFromOptions();
}

LC_LayerExportOptionsWidget::~LC_LayerExportOptionsWidget(){
    delete ui;
}

void LC_LayerExportOptionsWidget::onCreateSeparateDrawingToggled([[maybe_unused]] bool enable) {
    ui->cbStoreEntitiesInOriginalLayer->setEnabled(ui->cbCreateSeparateDrawingForLayer->isChecked());
}

void LC_LayerExportOptionsWidget::fillLayerExportOptions(LC_LayersExportOptions* options) {
    options->m_exportNamedViews = ui->cbExportNamedViews->isChecked();
    options->m_exportUCSs = ui->cbExportUCSs->isChecked();
    options->m_createSeparateDocumentPerLayer  = ui->cbCreateSeparateDrawingForLayer->isChecked();
    options->m_putEntitiesToOriginalLayer = ui->cbStoreEntitiesInOriginalLayer->isChecked();
    saveToOptions();
}

void LC_LayerExportOptionsWidget::loadFromOptions() {
    LC_GROUP_GUARD("Export.Layers");
    {
        ui->cbExportUCSs->setChecked(LC_GET_BOOL("ExportUCS", true));
        ui->cbExportNamedViews->setChecked(LC_GET_BOOL("ExportViews", true));
        bool separateDocForLayer = LC_GET_BOOL("DocumentPerLayer", false);
        ui->cbCreateSeparateDrawingForLayer->setChecked(separateDocForLayer);
        ui->cbStoreEntitiesInOriginalLayer->setChecked(LC_GET_BOOL("EntitiesInOriginalLayer", false));
        ui->cbStoreEntitiesInOriginalLayer->setEnabled(separateDocForLayer);
    }
}

void LC_LayerExportOptionsWidget::saveToOptions() {
    LC_GROUP_GUARD("Export.Layers");
    {
        LC_SET("ExportUCS", ui->cbExportUCSs->isChecked());
        LC_SET("ExportViews", ui->cbExportNamedViews->isChecked());
        LC_SET("DocumentPerLayer", ui->cbCreateSeparateDrawingForLayer->isChecked());
        LC_SET("EntitiesInOriginalLayer", ui->cbStoreEntitiesInOriginalLayer->isChecked());
    }
}
