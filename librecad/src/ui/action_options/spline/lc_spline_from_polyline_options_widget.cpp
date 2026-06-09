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

#include "lc_spline_from_polyline_options_widget.h"

#include "lc_action_spline_from_polyline.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_spline_from_polyline_options_widget.h"

LC_SplineFromPolylineOptionsWidget::LC_SplineFromPolylineOptionsWidget(): ui(new Ui::LC_SplineFromPolylineOptionsWidget) {
    ui->setupUi(this);

    connect(ui->cbKeepOriginals, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptionsWidget::cbKeepOriginalsClicked);
    connect(ui->cbCurrentAttr, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptionsWidget::cbUseCurrentAttributesClicked);
    connect(ui->cbLayer, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptionsWidget::cbUseCurrentLayerClicked);
    connect(ui->cbFitPoints, &QCheckBox::toggled, this, &LC_SplineFromPolylineOptionsWidget::cbUseFitPointsClicked);
    connect(ui->sbDegree, &QSpinBox::valueChanged, this, &LC_SplineFromPolylineOptionsWidget::sbDegreeValueChanged);
    connect(ui->sbMidPoints, &QSpinBox::valueChanged, this, &LC_SplineFromPolylineOptionsWidget::sbMidPointsValueChanged);
}

LC_SplineFromPolylineOptionsWidget::~LC_SplineFromPolylineOptionsWidget() {
    delete ui;
}

void LC_SplineFromPolylineOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionSplineFromPolyline*>(a);

    const bool useFitPoints = m_action->isUseFitPoints();
    const bool useCurrentAttributes = m_action->isUseCurrentAttributes();
    const bool useCurrentLayer = m_action->isUseCurrentLayer();
    const bool keepOriginal = m_action->isKeepOriginals();
    const int midPoints = m_action->getSegmentPoints();
    const int splineDegree = m_action->getSplineDegree();

    LC_GuardedSignalsBlocker({ui->sbDegree, ui->sbMidPoints, ui->cbFitPoints, ui->cbCurrentAttr, ui->cbKeepOriginals, ui->cbLayer});
    ui->cbKeepOriginals->setChecked(keepOriginal);
    ui->cbLayer->setChecked(useCurrentLayer);
    ui->cbCurrentAttr->setChecked(useCurrentAttributes);
    ui->cbFitPoints->setChecked(useFitPoints);
    ui->sbDegree->setValue(splineDegree);
    ui->cbFitPoints->setEnabled(splineDegree == 2);
    ui->sbMidPoints->setValue(midPoints);
}

void LC_SplineFromPolylineOptionsWidget::cbKeepOriginalsClicked(const bool val) {
    m_action->setKeepOriginals(val);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::cbUseCurrentAttributesClicked(const bool val) {
    m_action->setUseCurrentAttributes(val);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::cbUseCurrentLayerClicked(const bool val) {
    m_action->setUseCurrentLayer(val);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::cbUseFitPointsClicked(const bool val) {
    m_action->setUseFitPoints(val);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::sbDegreeValueChanged(const int value) {
    m_action->setSplineDegree(value);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::sbMidPointsValueChanged(const int value) {
    m_action->setSegmentPoints(value);
    m_action->updateOptions();
}

void LC_SplineFromPolylineOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
