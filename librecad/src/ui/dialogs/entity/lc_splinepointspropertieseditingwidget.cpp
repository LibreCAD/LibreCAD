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

#include <QStandardItemModel>
#include <lc_splinepoints.h>
#include "lc_splinepointspropertieseditingwidget.h"
#include "ui_lc_splinepointspropertieseditingwidget.h"

LC_SplinePointsPropertiesEditingWidget::LC_SplinePointsPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_SplinePointsPropertiesEditingWidget){
    ui->setupUi(this);

    connect(ui->cbClosed, &QCheckBox::toggled, this, &LC_SplinePointsPropertiesEditingWidget::onClosedToggled);
}

LC_SplinePointsPropertiesEditingWidget::~LC_SplinePointsPropertiesEditingWidget(){
    delete ui;
}

void LC_SplinePointsPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity =  static_cast<LC_SplinePoints*>(entity);
    ui->cbClosed->setChecked(m_entity->isClosed());

    //number of control points
    auto const& bData = m_entity->getData();
    auto const n = bData.splinePoints.size();
    if (n <= 2) {
        ui->rbControlPoints->setChecked(true);
        ui->rbSplinePoints->setEnabled(false);
    } else {
        ui->rbSplinePoints->setChecked(true);
    }
    updatePoints();
}

void LC_SplinePointsPropertiesEditingWidget::updateEntityData() {
    //update Spline Points
    auto model = static_cast<QStandardItemModel *>(ui->tvPoints->model());
    size_t const n = model->rowCount();
    auto &d = m_entity->getData();

    //update points
    bool const useSpline = ui->rbSplinePoints->isChecked();
    auto &vps = useSpline ? d.splinePoints : d.controlPoints;
    size_t const n0 = vps.size();
    //update points
    for (size_t i = 0; i < n; ++i) {
        auto &vp = vps.at(i < n0 ? i : n0 - 1);
        auto const &vpx = model->item(i, 0)->text();
        auto const &vpy = model->item(i, 1)->text();

        RS_Vector wcsPoint = toWCSVector(vpx, vpy, vp);

        vp.x = wcsPoint.x;
        vp.y = wcsPoint.y;
    }
    m_entity->update();
}

void LC_SplinePointsPropertiesEditingWidget::updatePoints() {
    bool const useSpline = ui->rbSplinePoints->isChecked();

    auto const &bData = m_entity->getData();
    auto const &pts = useSpline ? bData.splinePoints : bData.controlPoints;
    auto model = new QStandardItemModel(pts.size(), 2, this);
    model->setHorizontalHeaderLabels({"x", "y"});

    //set spline data
    for (size_t row = 0; row < pts.size(); ++row) {
        auto const &vp = pts.at(row);
        QPair<QString, QString> pair = toUIStr(vp);

        auto *x = new QStandardItem(pair.first);
        auto *y = new QStandardItem(pair.second);

        model->setItem(row, 0, x);
        model->setItem(row, 1, y);
    }
    ui->tvPoints->setModel(model);
}

void LC_SplinePointsPropertiesEditingWidget::onClosedToggled([[maybe_unused]]bool checked) {
    m_entity->setClosed(ui->cbClosed->isChecked());
}
