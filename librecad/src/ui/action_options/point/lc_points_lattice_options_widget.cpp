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

#include "lc_points_lattice_options_widget.h"

#include "lc_action_draw_points_lattice.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_points_lattice_options_widget.h"

LC_PointsLatticeOptionsWidget::LC_PointsLatticeOptionsWidget(): ui(new Ui::LC_PointsLatticeOptionsWidget) {
    ui->setupUi(this);
    connect(ui->sbNumX, &QSpinBox::valueChanged, this, &LC_PointsLatticeOptionsWidget::onColumnsChanged);
    connect(ui->sbNumY, &QSpinBox::valueChanged, this, &LC_PointsLatticeOptionsWidget::onRowsChanged);
    connect(ui->cbAdjustLastPoint, &QCheckBox::toggled, this, &LC_PointsLatticeOptionsWidget::onAdjustLastPointToggled);
}

LC_PointsLatticeOptionsWidget::~LC_PointsLatticeOptionsWidget() {
    delete ui;
}

void LC_PointsLatticeOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawPointsLattice*>(a);

    const int columns = m_action->getColumnPointsCount();
    const int rows = m_action->getRowPointsCount();
    const bool adjustLastPoint = m_action->isAdjustLastPointToFirst();

    LC_GuardedSignalsBlocker({ui->sbNumX, ui->sbNumY, ui->cbAdjustLastPoint});

    ui->sbNumX->setValue(columns);
    ui->sbNumY->setValue(rows);
    ui->cbAdjustLastPoint->setChecked(adjustLastPoint);
}

void LC_PointsLatticeOptionsWidget::onColumnsChanged([[maybe_unused]] int value) {
    const int val = ui->sbNumX->value();
    m_action->setColumnPointsCount(val);
    m_action->updateOptions();
}

void LC_PointsLatticeOptionsWidget::onRowsChanged([[maybe_unused]] int value) {
    const int val = ui->sbNumY->value();
    m_action->setRowPointsCount(val);
    m_action->updateOptions();
}

void LC_PointsLatticeOptionsWidget::onAdjustLastPointToggled([[maybe_unused]] bool value) {
    const bool val = ui->cbAdjustLastPoint->isChecked();
    m_action->setAdjustLastPointToFirst(val);
    m_action->updateOptions();
}

void LC_PointsLatticeOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}
