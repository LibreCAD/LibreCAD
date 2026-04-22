/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_spline_explode_options_widget.h"

#include "lc_action_spline_modify_explode.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_spline_explode_options_widget.h"

LC_SplineExplodeOptionsWidget::LC_SplineExplodeOptionsWidget(): ui(new Ui::LC_SplineExplodeOptionsWidget),
      m_segmentsCountFromDrawing{0} {
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::toggled, this, &LC_SplineExplodeOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::toggled, this, &LC_SplineExplodeOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::toggled, this, &LC_SplineExplodeOptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->cbCustomSegmentsCount, &QCheckBox::toggled, this, &LC_SplineExplodeOptionsWidget::cbCustomSegmentCountClicked);
    connect(ui->cbPolyline, &QCheckBox::toggled, this, &LC_SplineExplodeOptionsWidget::cbPolylineClicked);
    connect(ui->sbSegmentsCount, &QSpinBox::valueChanged, this, &LC_SplineExplodeOptionsWidget::sbSegmentsCountValueChanged);
}

LC_SplineExplodeOptionsWidget::~LC_SplineExplodeOptionsWidget() {
    delete ui;
}

void LC_SplineExplodeOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionSplineExplode*>(a);
    m_segmentsCountFromDrawing = m_action->getSegmentsCountFromDrawing();

    const bool toPolyline = m_action->isToPolyline();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool keepOriginal = m_action->isKeepOriginals();
    const bool useCustomSegmentsCount = m_action->isUseCustomSegmentsCount();
    const int customSegmentsCount = m_action->getCustomSegmentsCount();

    LC_GuardedSignalsBlocker({
        ui->cbPolyline,
        ui->sbSegmentsCount,
        ui->cbCustomSegmentsCount,
        ui->cbKeepOriginals,
        ui->cbLayer,
        ui->cbCurrentAttr
    });

    ui->cbPolyline->setChecked(toPolyline);
    ui->sbSegmentsCount->setValue(customSegmentsCount);

    ui->cbCustomSegmentsCount->setChecked(useCustomSegmentsCount);
    ui->sbSegmentsCount->setEnabled(useCustomSegmentsCount);
    if (!useCustomSegmentsCount) {
        ui->sbSegmentsCount->setValue(m_segmentsCountFromDrawing);
    }

    ui->cbKeepOriginals->setChecked(keepOriginal);
    ui->cbLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
}

void LC_SplineExplodeOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::cbPolylineClicked(const bool val) {
    m_action->setUsePolyline(val);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::sbSegmentsCountValueChanged(const int value) {
    m_action->setSegmentsCountValue(value);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::cbCustomSegmentCountClicked(const bool val) {
    m_action->setUseCustomSegmentsCount(val);
    m_action->updateOptions();
}

void LC_SplineExplodeOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
