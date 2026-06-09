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

#include "lc_propertieseditingwidget_ellipse.h"

#include "rs_debug.h"
#include "rs_ellipse.h"
#include "ui_lc_propertieseditingwidget_ellipse.h"

LC_PropertiesEditingWidgetEllipse::LC_PropertiesEditingWidgetEllipse(QWidget *parent)
    : LC_EntityPropertiesEditorWidget(parent)
    , ui(new Ui::LC_EllipsePropertiesEditingWidget){
    ui->setupUi(this);

    connect(ui->leCenterX, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onCenterEditingFinished);
    connect(ui->leCenterY, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onCenterEditingFinished);
    connect(ui->leMajor, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onMajorEditingFinished);
    connect(ui->leMinor, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onMinorEditingFinished);
    connect(ui->leRotation, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onRotationEditingFinished);
    connect(ui->leAngle1, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onAngle1EditingFinished);
    connect(ui->leAngle2, &QLineEdit::editingFinished, this, &LC_PropertiesEditingWidgetEllipse::onAngle2EditingFinished);
    connect(ui->cbReversed, &QCheckBox::toggled, this, &LC_PropertiesEditingWidgetEllipse::onReversedToggled);
}

LC_PropertiesEditingWidgetEllipse::~LC_PropertiesEditingWidgetEllipse(){
    delete ui;
}

void LC_PropertiesEditingWidgetEllipse::setEntity(RS_Entity* entity) {
    m_entity = static_cast<RS_Ellipse*>(entity);

    toUI(m_entity->getCenter(), ui->leCenterX, ui->leCenterY);

    const double majorAxisLen = m_entity->getMajorRadius();
    const double minorAxisLen = m_entity->getMinorRadius();

    toUIValue(majorAxisLen, ui->leMajor);
    toUIValue(minorAxisLen, ui->leMinor);

    const double wcsMajorAngle = m_entity->getMajorP().angle();

    toUIAngleDeg(wcsMajorAngle, ui->leRotation);

    // fixme - sand - for ellipse arc, internal angles are used (assuming that major angle = 0). Rework this for the consistency over the entire UI - use ucs angle like RS_ARC!!
    toUIAngleDegRaw(m_entity->getAngle1(), ui->leAngle1);
    toUIAngleDegRaw(m_entity->getAngle2(), ui->leAngle2);

    toUIBool(m_entity->isReversed(), ui->cbReversed);
}

void LC_PropertiesEditingWidgetEllipse::onCenterEditingFinished() const {
    m_entity->setCenter(toWCS(ui->leCenterX, ui->leCenterY, m_entity->getCenter()));
}

bool LC_PropertiesEditingWidgetEllipse::updateEllipseGeometry() const {
    const double major = toWCSValue(ui->leMajor, m_entity->getMajorRadius());
    if (major < RS_TOLERANCE) {
        LC_ERR << __func__<<"(): invalid ellipse major radius: "<< major<<", ellipse not modified";
        return true;
    }
    const double minor = toWCSValue(ui->leMinor, m_entity->getMinorRadius());
    const double rotation = toWCSAngle(ui->leRotation, m_entity->getMajorP().angle());

    m_entity->setMajorP(RS_Vector::polar(major, rotation));
    m_entity->setRatio(minor/major);
    return false;
}

void LC_PropertiesEditingWidgetEllipse::onMajorEditingFinished() const {
    updateEllipseGeometry();
}

void LC_PropertiesEditingWidgetEllipse::onMinorEditingFinished() const {
    updateEllipseGeometry();
}

void LC_PropertiesEditingWidgetEllipse::onRotationEditingFinished() const {
    updateEllipseGeometry();
}

void LC_PropertiesEditingWidgetEllipse::onAngle1EditingFinished() const {
    const double angle = toRawAngleValue(ui->leAngle1, m_entity->getAngle1());
    m_entity->setAngle1(angle);
}

void LC_PropertiesEditingWidgetEllipse::onAngle2EditingFinished() const {
    const double angle = toRawAngleValue(ui->leAngle2, m_entity->getAngle2());
    m_entity->setAngle1(angle);
}

void LC_PropertiesEditingWidgetEllipse::onReversedToggled([[maybe_unused]]bool checked) const {
    m_entity->setReversed(ui->cbReversed->isChecked());
}

void LC_PropertiesEditingWidgetEllipse::setupInteractiveInputWidgets() {
    pickPointSetup(ui->wPickPointCenter, "center", ui->leCenterX, ui->leCenterY);
    pickDistanceSetup(ui->tbPickMajor, "major", ui->leMajor);
    pickDistanceSetup(ui->tbPickMajor, "minor", ui->leMinor);
    pickAngleSetup(ui->tbPickRotation, "rotation", ui->leRotation);
    pickAngleSetup(ui->tbPickStartAngle, "angle1", ui->leAngle1);
    pickAngleSetup(ui->tbPickEndAngle, "angle2", ui->leAngle2);
}
