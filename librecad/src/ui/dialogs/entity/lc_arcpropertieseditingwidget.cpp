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
#include <cmath>
#include <limits>

#include <QDoubleValidator>
#include <QSignalBlocker>

#include "lc_arcpropertieseditingwidget.h"

#include "rs_arc.h"
#include "rs_math.h"
#include "ui_lc_arcpropertieseditingwidget.h"

LC_ArcPropertiesEditingWidget::LC_ArcPropertiesEditingWidget(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_ArcPropertiesEditingWidget){
    ui->setupUi(this);

    // Set up validators for input fields
    auto coordValidator = new QDoubleValidator(this);
    ui->leCenterX->setValidator(coordValidator);
    ui->leCenterY->setValidator(coordValidator);
    ui->leAngle1->setValidator(coordValidator);
    ui->leAngle2->setValidator(coordValidator);

    auto positiveValidator = new QDoubleValidator(0.0, std::numeric_limits<double>::max(), 10, this);
    ui->leRadius->setValidator(positiveValidator);
    ui->leDiameter->setValidator(positiveValidator);
    ui->leArcLength->setValidator(positiveValidator);
    ui->leSweepAngle->setValidator(positiveValidator);
    ui->leBulge->setValidator(positiveValidator);

    connect(ui->leCenterX, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leCenterY, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onCenterEditingFinished);
    connect(ui->leRadius, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onRadiusEditingFinished);
    connect(ui->leDiameter, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onDiameterEditingFinished);
    connect(ui->leArcLength, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onArcLengthEditingFinished);
    connect(ui->leSweepAngle, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onSweepAngleEditingFinished);
    connect(ui->leBulge, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onBulgeEditingFinished);
    connect(ui->leAngle1, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onAngle1EditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_ArcPropertiesEditingWidget::onAngle2EditingFinished);
    connect(ui->cbReversed, &QCheckBox::toggled, this, &LC_ArcPropertiesEditingWidget::onReversedToggled);
    connect(ui->leRadius, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onRadiusTextChanged);
    connect(ui->leDiameter, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onDiameterTextChanged);
    connect(ui->leArcLength, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onArcLengthTextChanged);
    connect(ui->leSweepAngle, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onSweepAngleTextChanged);
    connect(ui->leBulge, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onBulgeTextChanged);
    connect(ui->leAngle1, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onAngle1TextChanged);
    connect(ui->leAngle2, &QLineEdit::textChanged, this, &LC_ArcPropertiesEditingWidget::onAngle2TextChanged);
}

LC_ArcPropertiesEditingWidget::~LC_ArcPropertiesEditingWidget(){
    delete ui;
}

void LC_ArcPropertiesEditingWidget::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Arc*>(entity);

    {
        QSignalBlocker r(ui->leRadius);
        QSignalBlocker d(ui->leDiameter);
        toUI(m_entity->getCenter(), ui->leCenterX, ui->leCenterY);
        toUIValue(m_entity->getRadius(), ui->leRadius);
        toUIValue(m_entity->getRadius() * 2, ui->leDiameter);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(m_entity->getAngle1())), ui->leAngle1);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(m_entity->getAngle2())), ui->leAngle2);
        toUIBool(m_entity->isReversed(), ui->cbReversed);
        updateUI();
    }
}

void LC_ArcPropertiesEditingWidget::onCenterEditingFinished() {
    m_entity->setCenter(toWCS(ui->leCenterX, ui->leCenterY, m_entity->getCenter()));
}

void LC_ArcPropertiesEditingWidget::onRadiusEditingFinished() {
    double newRadius = RS_Math::eval(ui->leRadius->text(), m_entity->getRadius());
    if (newRadius > 0.0) {  // Ensure positive radius
        m_entity->setRadius(newRadius);
        QSignalBlocker d(ui->leDiameter);
        toUIValue(newRadius * 2, ui->leDiameter);
        updateUI();
    } else {
        // Revert to current value if invalid
        toUIValue(m_entity->getRadius(), ui->leRadius);
    }
}

void LC_ArcPropertiesEditingWidget::onDiameterEditingFinished() {
    double newDiameter = RS_Math::eval(ui->leDiameter->text(), m_entity->getRadius() * 2);
    double newRadius = newDiameter / 2.0;
    if (newRadius > 0.0) {  // Ensure positive radius
        m_entity->setRadius(newRadius);
        QSignalBlocker r(ui->leRadius);
        toUIValue(newRadius, ui->leRadius);
        updateUI();
    } else {
        // Revert to current value if invalid
        toUIValue(m_entity->getRadius() * 2, ui->leDiameter);
    }
}

void LC_ArcPropertiesEditingWidget::onArcLengthEditingFinished() {
    double newLength = RS_Math::eval(ui->leArcLength->text(), m_entity->getLength());
    double radius = m_entity->getRadius();
    double maxLength = 2 * M_PI * radius;
    if (newLength > 0.0 && newLength <= maxLength) {
        double newTheta = newLength / radius;
        double sign = m_entity->isReversed() ? -1.0 : 1.0;
        double newEndRad = m_entity->getAngle1() + sign * newTheta;
        m_entity->setAngle2(newEndRad);
        QSignalBlocker a(ui->leAngle2);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(newEndRad)), ui->leAngle2);
        updateUI();
    } else {
        toUIValue(m_entity->getLength(), ui->leArcLength);
    }
}

void LC_ArcPropertiesEditingWidget::onSweepAngleEditingFinished() {
    double sweep_deg = RS_Math::eval(ui->leSweepAngle->text(), RS_Math::rad2deg(m_entity->getAngleLength()));
    if (sweep_deg > 0.0) {
        double sweep_rad = RS_Math::deg2rad(sweep_deg);
        double sign = m_entity->isReversed() ? -1.0 : 1.0;
        double new_end_rad = m_entity->getAngle1() + sign * sweep_rad;
        m_entity->setAngle2(new_end_rad);
        QSignalBlocker a(ui->leAngle2);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(new_end_rad)), ui->leAngle2);
        updateUI();
    } else {
        toUIValue(RS_Math::rad2deg(m_entity->getAngleLength()), ui->leSweepAngle);
    }
}

void LC_ArcPropertiesEditingWidget::onBulgeEditingFinished() {
    double currentTheta = m_entity->getAngleLength();
    double currentBulge = m_entity->getRadius() * (1 - std::cos(currentTheta / 2.0));
    double newBulge = RS_Math::eval(ui->leBulge->text(), currentBulge);
    if (newBulge > 0.0) {
        double theta = currentTheta;
        if (theta > 0.0 && theta < 2 * M_PI) {
            double cos_term = std::cos(theta / 2.0);
            if (cos_term < 1.0) {
                double newRadius = newBulge / (1 - cos_term);
                m_entity->setRadius(newRadius);
                QSignalBlocker r(ui->leRadius);
                QSignalBlocker d(ui->leDiameter);
                toUIValue(newRadius, ui->leRadius);
                toUIValue(newRadius * 2, ui->leDiameter);
                updateUI();
            } else {
                toUIValue(currentBulge, ui->leBulge);
            }
        } else {
            toUIValue(currentBulge, ui->leBulge);
        }
    } else {
        toUIValue(currentBulge, ui->leBulge);
    }
}

void LC_ArcPropertiesEditingWidget::onAngle1EditingFinished() {
    double currentStartRad = RS_Math::correctAngle(m_entity->getAngle1());
    double currentStartDeg = RS_Math::rad2deg(currentStartRad);
    double deg = RS_Math::eval(ui->leAngle1->text(), currentStartDeg);
    double rad = RS_Math::deg2rad(deg);
    double correctedRad = RS_Math::correctAngle(rad);
    m_entity->setAngle1(correctedRad);
    QSignalBlocker a1(ui->leAngle1);
    toUIValue(RS_Math::rad2deg(correctedRad), ui->leAngle1);
    updateUI();
}

void LC_ArcPropertiesEditingWidget::onAngle2EditingFinished() {
    double currentEndRad = RS_Math::correctAngle(m_entity->getAngle2());
    double currentEndDeg = RS_Math::rad2deg(currentEndRad);
    double deg = RS_Math::eval(ui->leAngle2->text(), currentEndDeg);
    double rad = RS_Math::deg2rad(deg);
    double correctedRad = RS_Math::correctAngle(rad);
    m_entity->setAngle2(correctedRad);
    QSignalBlocker a2(ui->leAngle2);
    toUIValue(RS_Math::rad2deg(correctedRad), ui->leAngle2);
    updateUI();
}

void LC_ArcPropertiesEditingWidget::onReversedToggled([[maybe_unused]]bool checked) {
    if (m_entity->isReversed() != ui->cbReversed->isChecked()) {
        m_entity->revertDirection();
        QSignalBlocker a1(ui->leAngle1);
        QSignalBlocker a2(ui->leAngle2);
        double startRad = RS_Math::correctAngle(m_entity->getAngle1());
        toUIValue(RS_Math::rad2deg(startRad), ui->leAngle1);
        double endRad = RS_Math::correctAngle(m_entity->getAngle2());
        toUIValue(RS_Math::rad2deg(endRad), ui->leAngle2);
    }
}

void LC_ArcPropertiesEditingWidget::onRadiusTextChanged(const QString &text) {
    double value = RS_Math::eval(text, 0.0);
    if (value > 0.0) {
        QSignalBlocker d(ui->leDiameter);
        toUIValue(value * 2, ui->leDiameter);
        updateDerivedFields(value, m_entity->getAngleLength());
    }
}

void LC_ArcPropertiesEditingWidget::onDiameterTextChanged(const QString &text) {
    double value = RS_Math::eval(text, 0.0);
    if (value > 0.0) {
        double newRadius = value / 2.0;
        QSignalBlocker r(ui->leRadius);
        toUIValue(newRadius, ui->leRadius);
        updateDerivedFields(newRadius, m_entity->getAngleLength());
    }
}

void LC_ArcPropertiesEditingWidget::onArcLengthTextChanged(const QString &text) {
    double value = RS_Math::eval(text, 0.0);
    if (value > 0.0) {
        double radius = m_entity->getRadius();
        double newTheta = value / radius;
        QSignalBlocker s(ui->leSweepAngle);
        toUIValue(RS_Math::rad2deg(newTheta), ui->leSweepAngle);
        double sign = m_entity->isReversed() ? -1.0 : 1.0;
        double newEndRad = m_entity->getAngle1() + sign * newTheta;
        QSignalBlocker a2(ui->leAngle2);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(newEndRad)), ui->leAngle2);
        updateDerivedFields(radius, newTheta);
    }
}

void LC_ArcPropertiesEditingWidget::onSweepAngleTextChanged(const QString &text) {
    double sweep_deg = RS_Math::eval(text, 0.0);
    if (sweep_deg > 0.0) {
        double sweep_rad = RS_Math::deg2rad(sweep_deg);
        double sign = m_entity->isReversed() ? -1.0 : 1.0;
        double new_end_rad = m_entity->getAngle1() + sign * sweep_rad;
        QSignalBlocker a2(ui->leAngle2);
        toUIValue(RS_Math::rad2deg(RS_Math::correctAngle(new_end_rad)), ui->leAngle2);
        double radius = m_entity->getRadius();
        updateDerivedFields(radius, sweep_rad);
    }
}

void LC_ArcPropertiesEditingWidget::onBulgeTextChanged(const QString &text) {
    double value = RS_Math::eval(text, 0.0);
    if (value > 0.0) {
        double theta = m_entity->getAngleLength();
        if (theta > 0.0 && theta < 2 * M_PI) {
            double cos_term = std::cos(theta / 2.0);
            if (cos_term < 1.0) {
                double newRadius = value / (1 - cos_term);
                QSignalBlocker r(ui->leRadius);
                QSignalBlocker d(ui->leDiameter);
                toUIValue(newRadius, ui->leRadius);
                toUIValue(newRadius * 2, ui->leDiameter);
                updateDerivedFields(newRadius, theta);
            }
        }
    }
}

void LC_ArcPropertiesEditingWidget::onAngle1TextChanged(const QString &text) {
    double degA1 = RS_Math::eval(text, 0.0);
    double radA1 = RS_Math::deg2rad(degA1);
    double currentRadA2 = m_entity->getAngle2();
    double newTheta = std::abs(currentRadA2 - radA1);
    double radius = m_entity->getRadius();
    updateDerivedFields(radius, newTheta);
}

void LC_ArcPropertiesEditingWidget::onAngle2TextChanged(const QString &text) {
    double degA2 = RS_Math::eval(text, 0.0);
    double radA2 = RS_Math::deg2rad(degA2);
    double currentRadA1 = m_entity->getAngle1();
    double newTheta = std::abs(radA2 - currentRadA1);
    double radius = m_entity->getRadius();
    updateDerivedFields(radius, newTheta);
}

void LC_ArcPropertiesEditingWidget::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickRadius,  "radius", ui->leRadius);
    pickDistanceSetup(ui->tbPickDiameter, "diameter", ui->leDiameter);
    pickDistanceSetup(ui->tbPickArcLength, "arcLength", ui->leArcLength);
    pickAngleSetup(ui->tbPickSweepAngle, "sweepAngle", ui->leSweepAngle);
    pickDistanceSetup(ui->tbPickBulge, "bulge", ui->leBulge);
    pickAngleSetup(ui->tbPickStartAngle,  "startAngle", ui->leAngle1);
    pickAngleSetup(ui->tbPickEndAngle, "endAngle",  ui->leAngle2);
}

void LC_ArcPropertiesEditingWidget::updateDerivedFields(double radius, double theta) {
    QSignalBlocker al(ui->leArcLength);
    toUIValue(radius * theta, ui->leArcLength);
    QSignalBlocker sa(ui->leSweepAngle);
    toUIValue(RS_Math::rad2deg(theta), ui->leSweepAngle);
    QSignalBlocker b(ui->leBulge);
    toUIValue(radius * (1 - std::cos(theta / 2.0)), ui->leBulge);
}

void LC_ArcPropertiesEditingWidget::updateUI() {
    updateDerivedFields(m_entity->getRadius(), m_entity->getAngleLength());
}
