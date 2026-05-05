/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li github.com/dxli
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

#include "lc_hatchpropertieseditingwidget.h"

#include <cmath>

#include "lc_secondmoment.h"
#include "rs_hatch.h"
#include "rs_settings.h"
#include "ui_lc_hatchpropertieseditingwidget.h"

LC_HatchPropertiesEditingWidget::LC_HatchPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_HatchPropertiesEditingWidget) {
    ui->setupUi(this);

    connect(ui->leScale, &QLineEdit::editingFinished, this, &LC_HatchPropertiesEditingWidget::onScaleEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_HatchPropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->cbSolid, &QCheckBox::toggled, this, &LC_HatchPropertiesEditingWidget::onSolidToggled);
}

LC_HatchPropertiesEditingWidget::~LC_HatchPropertiesEditingWidget() {
    delete ui;
}

void LC_HatchPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Hatch*>(entity);

    LC_GROUP_GUARD("Draw");
    toUIBool(m_entity->isSolid(), ui->cbSolid);
    ui->lePattern->setText(m_entity->getPattern());
    toUIValue(m_entity->getScale(), ui->leScale);
    toUIAngleDeg(m_entity->getAngle(), ui->leAngle);

    updateMomentFields();
}

void LC_HatchPropertiesEditingWidget::saveSettings() {
    LC_GROUP_GUARD("Draw");
    LC_SET("HatchSolid", m_entity->isSolid());
    LC_SET("HatchPattern", m_entity->getPattern());
    LC_SET("HatchScale", ui->leScale->text());
    LC_SET("HatchAngle", ui->leAngle->text());
}

void LC_HatchPropertiesEditingWidget::updateMomentFields() {
    double area = m_entity->getTotalArea();
    toUIValue(area, ui->leArea);

    RS_Vector centroid = m_entity->getCentroid();
    if (centroid.valid) {
        toUI(centroid, ui->leCentroidX, ui->leCentroidY);
    } else {
        ui->leCentroidX->setText(tr("N/A"));
        ui->leCentroidY->setText(tr("N/A"));
    }

    LC_SecondMoment m = m_entity->getMomentOfInertia();

    // Raw central moments
    toUIValue(m.ixx, ui->leIxx);
    toUIValue(m.iyy, ui->leIyy);
    toUIValue(m.ixy, ui->leIxy);

    // Principal axes: eigenvalues of the inertia tensor
    //   I = | iyy  -ixy |
    //       | -ixy  ixx |
    // Eigenvalues:  I_avg ± sqrt( ((iyy - ixx)/2)^2 + ixy^2 )
    double avg = (m.ixx + m.iyy) / 2.0;
    double delta = (m.iyy - m.ixx) / 2.0;
    double R = std::sqrt(delta * delta + m.ixy * m.ixy);
    double I1 = avg + R;  // maximum principal moment
    double I2 = avg - R;  // minimum principal moment

    // Check for degeneracy: when R is very small, I1 ≈ I2
    // This indicates rotational symmetry (e.g., circle, square)
    // In such cases, any direction is a principal axis
    const double tolerance = 1e-10;
    bool isDegenerate = (R <= tolerance);
    
    // Principal axis angle (angle of I2 axis measured from x-axis)
    // Avoid calling atan2(0, 0) in degenerate cases
    double theta = 0.0;  // default to 0 for degenerate cases
    if (!isDegenerate) {
        // Non-degenerate case: calculate principal axis angle
        theta = 0.5 * std::atan2(-m.ixy, delta);  // radians
    }
    // else: degenerate case (I1 ≈ I2), keep theta = 0

    toUIValue(I1, ui->leI1);
    toUIValue(I2, ui->leI2);
    toUIAngleDeg(theta, ui->lePrincipalAngle);
    
    // Display degeneracy status
    ui->leDegenerate->setText(isDegenerate ? tr("Yes") : tr("No"));
    if (isDegenerate) {
        ui->leDegenerate->setStyleSheet("QLineEdit { color: gray; font-style: italic; }");
        ui->lePrincipalAngle->setStyleSheet("QLineEdit { color: gray; font-style: italic; }");
    } else {
        ui->leDegenerate->setStyleSheet("");
        ui->lePrincipalAngle->setStyleSheet("");
    }
}

void LC_HatchPropertiesEditingWidget::onScaleEditingFinished() {
    m_entity->setScale(toWCSValue(ui->leScale, m_entity->getScale()));
    saveSettings();
}

void LC_HatchPropertiesEditingWidget::onAngleEditingFinished() {
    m_entity->setAngle(toWCSAngle(ui->leAngle, m_entity->getAngle()));
    saveSettings();
}

void LC_HatchPropertiesEditingWidget::onSolidToggled([[maybe_unused]] bool checked) {
    m_entity->setSolid(ui->cbSolid->isChecked());
    saveSettings();
}
