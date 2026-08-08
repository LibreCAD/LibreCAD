/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

#include <array>
#include <cmath>

#include <QFontMetrics>
#include <QLineEdit>

#include "lc_convert.h"
#include "lc_secondmoment.h"
#include "rs_hatch.h"
#include "rs_settings.h"
#include "ui_lc_hatchpropertieseditingwidget.h"

namespace {
    //! Significant digits shown for every numeric field of this widget.
    constexpr int g_precision = 8;

    /**
     * Widest text QString::number(v, 'g', g_precision) can produce: sign, leading
     * digit, decimal point, g_precision-1 further digits and a three digit exponent.
     */
    const QString g_widestValue = QStringLiteral("-1.2345678e-308");

    //! Every field of the widget that shows a value.
    std::array<QLineEdit*, 13> valueFields(Ui::LC_HatchPropertiesEditingWidget* ui) {
        return {ui->lePattern, ui->leScale, ui->leAngle,
                ui->leArea, ui->leCentroidX, ui->leCentroidY,
                ui->leIxx, ui->leIyy, ui->leIxy,
                ui->leI1, ui->leI2, ui->lePrincipalAngle, ui->leDegenerate};
    }
}

LC_HatchPropertiesEditingWidget::LC_HatchPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_HatchPropertiesEditingWidget) {
    ui->setupUi(this);

    setupValueFields();

    connect(ui->leScale, &QLineEdit::editingFinished, this, &LC_HatchPropertiesEditingWidget::onScaleEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_HatchPropertiesEditingWidget::onAngleEditingFinished);
    connect(ui->cbSolid, &QCheckBox::toggled, this, &LC_HatchPropertiesEditingWidget::onSolidToggled);
}

/**
 * The moments span many orders of magnitude, so the value fields have to be wide
 * enough for the longest string g_precision digits can produce. Sizing from the
 * font rather than from a pixel constant keeps this correct at any DPI. The two
 * extra character widths stand in for the frame and the text margins.
 */
void LC_HatchPropertiesEditingWidget::setupValueFields() {
    const QFontMetrics fm = ui->leArea->fontMetrics();
    const int valueWidth = fm.horizontalAdvance(g_widestValue) + 2 * fm.horizontalAdvance(QLatin1Char('0'));

    for (QLineEdit* ed: valueFields(ui)) {
        ed->setMinimumWidth(valueWidth);
    }
}

/**
 * A QLineEdit keeps whatever scroll position it had, so a field that was once too
 * narrow shows the tail of the number. Rewind every field so the most significant
 * digits are the ones on screen.
 */
void LC_HatchPropertiesEditingWidget::showValueStarts() {
    for (QLineEdit* ed: valueFields(ui)) {
        ed->setCursorPosition(0);
    }
}

//! Like toUIValue(), but at this widget's precision instead of the shared default.
void LC_HatchPropertiesEditingWidget::setValue(double value, QLineEdit* ed) const {
    ed->setText(LC_Convert::asString(value, g_precision));
}

//! Like toUIAngleDeg(), but at this widget's precision instead of the shared default.
void LC_HatchPropertiesEditingWidget::setAngle(double wcsAngle, QLineEdit* ed) const {
    ed->setText(LC_Convert::asStringAngleDeg(toUCSAngle(wcsAngle), g_precision));
}

LC_HatchPropertiesEditingWidget::~LC_HatchPropertiesEditingWidget() {
    delete ui;
}

void LC_HatchPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Hatch*>(entity);

    LC_GROUP_GUARD("Draw");
    toUIBool(m_entity->isSolid(), ui->cbSolid);
    ui->lePattern->setText(m_entity->getPattern());
    setValue(m_entity->getScale(), ui->leScale);
    setAngle(m_entity->getAngle(), ui->leAngle);

    updateMomentFields();
    showValueStarts();
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
    setValue(area, ui->leArea);

    RS_Vector centroid = m_entity->getCentroid();
    if (centroid.valid) {
        const RS_Vector ucsCentroid = toUCSVector(centroid);
        setValue(ucsCentroid.x, ui->leCentroidX);
        setValue(ucsCentroid.y, ui->leCentroidY);
    } else {
        ui->leCentroidX->setText(tr("N/A"));
        ui->leCentroidY->setText(tr("N/A"));
    }

    LC_SecondMoment m = m_entity->getMomentOfInertia();

    // Raw central moments
    setValue(m.ixx, ui->leIxx);
    setValue(m.iyy, ui->leIyy);
    setValue(m.ixy, ui->leIxy);

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

    setValue(I1, ui->leI1);
    setValue(I2, ui->leI2);
    setAngle(theta, ui->lePrincipalAngle);

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
