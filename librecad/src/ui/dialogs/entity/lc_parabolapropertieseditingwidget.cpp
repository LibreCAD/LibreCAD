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

#include "lc_parabolapropertieseditingwidget.h"

#include <QStandardItemModel>

#include "lc_parabola.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "ui_lc_parabolapropertieseditingwidget.h"

LC_ParabolaPropertiesEditingWidget::LC_ParabolaPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_ParabolaPropertiesEditingWidget){
    ui->setupUi(this);
}

LC_ParabolaPropertiesEditingWidget::~LC_ParabolaPropertiesEditingWidget(){
    delete ui;
}

void LC_ParabolaPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<LC_Parabola*>(entity);
    updatePoints();
}

void LC_ParabolaPropertiesEditingWidget::updatePoints(){
    auto const& bData = m_entity->getData();
    auto const& pts = bData.controlPoints;
    auto model = new QStandardItemModel(pts.size(), 2, this);
    model->setHorizontalHeaderLabels({"x", "y"});

    //set control data
    for (size_t row = 0; row < pts.size(); ++row) {
        auto const& vp = pts.at(row);

        QString uiX, uiY;
        QPair<QString, QString> uiPoint = toUIStr(vp);

        auto* x = new QStandardItem(uiPoint.first);
        model->setItem(row, 0, x);
        auto* y = new QStandardItem(uiPoint.second);
        model->setItem(row, 1, y);
    }
    model->setRowCount(pts.size());
    ui->tvPoints->setModel(model);
}

void LC_ParabolaPropertiesEditingWidget::updateEntityData() {
    //update Spline Points
    auto model = static_cast<QStandardItemModel*>(ui->tvPoints->model());
    model->setRowCount(3);
    //update points
    std::array<RS_Vector, 3> vps;
    //update points
    for (size_t i = 0; i < 3; ++i) {
        auto& vp = vps.at(i);
        auto const& vpx = model->item(i, 0)->text();
        auto const& vpy = model->item(i, 1)->text();

        RS_Vector wcsPoint = toWCSVector(vpx, vpy, vp);

        vp.x = wcsPoint.x;
        vp.y = wcsPoint.y;
    }
    // fixme - sand - review and report this via dialog, not via command line!
    if (std::abs(std::remainder(vps.front().angleTo(vps[1]) - vps.back().angleTo(vps[1]), M_PI)) < RS_TOLERANCE_ANGLE) {
        RS_DIALOGFACTORY->commandMessage(tr("Parabola control points cannot be collinear"));
        return;
    }
    auto& d = m_entity->getData();
    d.controlPoints = vps;
    m_entity->update();
}
