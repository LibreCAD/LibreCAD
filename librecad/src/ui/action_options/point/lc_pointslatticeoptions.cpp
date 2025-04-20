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

#include "lc_pointslatticeoptions.h"
#include "lc_actiondrawpointslattice.h"
#include "ui_lc_pointslatticeoptions.h"

LC_PointsLatticeOptions::LC_PointsLatticeOptions()
    : LC_ActionOptionsWidgetBase(RS2::ActionDrawPointsLattice, "Draw", "PointsLattice")
    , ui(new Ui::LC_PointsLatticeOptions){
    ui->setupUi(this);
    connect(ui->sbNumX, &QSpinBox::valueChanged, this, &LC_PointsLatticeOptions::onColumnsChanged);
    connect(ui->sbNumX, &QSpinBox::valueChanged, this, &LC_PointsLatticeOptions::onRowsChanged);
    connect(ui->cbAdjustLastPoint, &QCheckBox::toggled, this, &LC_PointsLatticeOptions::onAdjustLastPointToggled);
}

LC_PointsLatticeOptions::~LC_PointsLatticeOptions(){
    delete ui;
}

void LC_PointsLatticeOptions::doSaveSettings() {
    save("Columns", ui->sbNumX->value());
    save("Rows", ui->sbNumY->value());
    save("AdjustLastPoint", ui->cbAdjustLastPoint->isChecked());
}

void LC_PointsLatticeOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<LC_ActionDrawPointsLattice *>(a);
    int rows;
    int columns;
    bool adjustLastPoint;
    if (update){
        columns = m_action->getColumnPointsCount();
        rows = m_action->getRowPointsCount();
        adjustLastPoint = m_action->isAdjustLastPointToFirst();
    }
    else{
        columns = loadInt("Columns", 1);
        rows = loadInt("Rows", 1);
        adjustLastPoint = loadBool("AdjustLastPoint", true);
    }
    setColumnsToActionAndView(columns);
    setRowsToActionAndView(rows);
    setAdjustLastPointToActionAndView(adjustLastPoint);
}

void LC_PointsLatticeOptions::onColumnsChanged([[maybe_unused]]int value) {
    setColumnsToActionAndView(ui->sbNumX->value());
}

void LC_PointsLatticeOptions::onRowsChanged([[maybe_unused]]int value) {
    setRowsToActionAndView(ui->sbNumY->value());
}

void LC_PointsLatticeOptions::onAdjustLastPointToggled([[maybe_unused]]bool value) {
    setAdjustLastPointToActionAndView(ui->cbAdjustLastPoint->isChecked());
}

void LC_PointsLatticeOptions::setAdjustLastPointToActionAndView(bool value) {
   ui->cbAdjustLastPoint->setChecked(value);
   m_action->setAdjustLastPointToFirst(value);
}

void LC_PointsLatticeOptions::setColumnsToActionAndView(int value) {
    ui->sbNumX->setValue(value);
    m_action->setColumnPointsCount(value);
}

void LC_PointsLatticeOptions::setRowsToActionAndView(int value) {
    ui->sbNumY->setValue(value);
    m_action->setRowPointsCount(value);
}

void LC_PointsLatticeOptions::languageChange(){
    ui->retranslateUi(this);
}
