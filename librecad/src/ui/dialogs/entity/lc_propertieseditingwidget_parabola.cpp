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

#include "lc_propertieseditingwidget_parabola.h"

#include <QStandardItemModel>
#include <array>

#include "lc_parabola.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "ui_lc_propertieseditingwidget_parabola.h"

LC_PropertiesEditingWidgetParabola::LC_PropertiesEditingWidgetParabola(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_ParabolaPropertiesEditingWidget){
    ui->setupUi(this);
}

LC_PropertiesEditingWidgetParabola::~LC_PropertiesEditingWidgetParabola(){
    delete ui;
}

void LC_PropertiesEditingWidgetParabola::setEntity(RS_Entity* entity) {
    m_entity = static_cast<LC_Parabola*>(entity);
    updatePoints();
}

void LC_PropertiesEditingWidgetParabola::updatePoints(){
    const LC_ParabolaData& bData = m_entity->getData();
    const std::array<RS_Vector, 3>& pts = bData.m_controlPoints;
    const auto model = new QStandardItemModel(pts.size(), 2, this);
    model->setHorizontalHeaderLabels({"x", "y"});

    //set control data
    for (size_t row = 0; row < pts.size(); ++row) {
        const auto& vp = pts.at(row);

        auto [fst, snd] = toUIStr(vp);

        auto* x = new QStandardItem(fst);
        model->setItem(row, 0, x);
        auto* y = new QStandardItem(snd);
        model->setItem(row, 1, y);
    }
    model->setRowCount(pts.size());
    ui->tvPoints->setModel(model);
}

// fixme - sand - it's better to rely on updating individual entity property as result of control change. that will insure that
// fixme - only properties that were actually modified by the user will be changed (so no loss or precision for other properties)

void LC_PropertiesEditingWidgetParabola::updateEntityData() {
    //update Spline Points
    const auto model = static_cast<QStandardItemModel*>(ui->tvPoints->model());
    model->setRowCount(3);
    //update points
    std::array<RS_Vector, 3> vps;
    //update points
    for (size_t i = 0; i < 3; ++i) {
        auto& vp = vps.at(i);
        const auto& vpx = model->item(i, 0)->text();
        const auto& vpy = model->item(i, 1)->text();

        // fixme - this will affect precision of existing data. REWORK to save only actually changed ones
        const RS_Vector wcsPoint = toWCSVector(vpx, vpy, vp, VectorModificationState::BOTH);

        vp.x = wcsPoint.x;
        vp.y = wcsPoint.y;
    }
    // fixme - sand - review and report this via dialog, not via command line!
    if (std::abs(std::remainder(vps.front().angleTo(vps[1]) - vps.back().angleTo(vps[1]), M_PI)) < RS_TOLERANCE_ANGLE) {
        RS_DIALOGFACTORY->commandMessage(tr("Parabola control points cannot be collinear"));
        return;
    }
    auto& d = m_entity->getData();
    d.m_controlPoints = vps;
    m_entity->update();
}
