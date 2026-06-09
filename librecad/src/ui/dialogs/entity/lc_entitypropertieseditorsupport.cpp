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

#include "lc_entitypropertieseditorsupport.h"

#include <QCheckBox>
#include <QLineEdit>

#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "rs_math.h"

RS_Vector LC_EntityPropertiesEditorSupport::toUCSVector(const RS_Vector &vect) const{
    const RS_Vector result = m_viewport->toUCS(vect);
    return result;
}

LC_EntityPropertiesEditorSupport::LC_EntityPropertiesEditorSupport(QWidget* parent):QWidget(parent), m_viewport{nullptr} {
}

void LC_EntityPropertiesEditorSupport::setGraphicViewport(LC_GraphicViewport* viewport) {
    m_viewport= viewport;
}

double LC_EntityPropertiesEditorSupport::toUCSAngle(const double wcsAngle) const {
    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    const double ucsBasisAngle = m_viewport->toBasisUCSAngle(ucsAngle);
    return ucsBasisAngle;
}

double LC_EntityPropertiesEditorSupport::toWCSAngle(const QString &val, const double wcsDefault) const {
    const double ucsDefault = toUCSAngle(wcsDefault);
    const double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);

    const double ucsAngleDeg = toDoubleAngle(val, 0.0, ucsDefaultDeg);
    const double ucsAngleBasis = RS_Math::deg2rad(ucsAngleDeg);
    const double ucsAngleAbs = m_viewport->toAbsUCSAngle(ucsAngleBasis);
    const double wcsAngle =  m_viewport->toWorldAngle(ucsAngleAbs);
    return wcsAngle;
}

double LC_EntityPropertiesEditorSupport::toWCSValue(const QString& val, const double wcsDefault) const {
    // note - this implementation assumes that there is no scale transformations due to coordinate system!
    return toDouble(val, 0.0, wcsDefault);
}

RS_Vector LC_EntityPropertiesEditorSupport::toWCSVector(const RS_Vector &vect) const{
    const RS_Vector result = m_viewport->toWorld(vect);
    return result;
}

double LC_EntityPropertiesEditorSupport::toRawAngleValue(const QLineEdit* ed, const double ucsDefault) const {
    const double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);
    const double rawAngleDeg = toDoubleAngle(ed->text(), 0.0, ucsDefaultDeg);
    const double rawAngle = RS_Math::deg2rad(rawAngleDeg);
    return rawAngle;
}

QPair<QString, QString> LC_EntityPropertiesEditorSupport::toUIStr(const RS_Vector &vect) const {
    const RS_Vector uiPos = toUCSVector(vect);
    QString sx = asString(uiPos.x);
    QString sy = asString(uiPos.y);
    QPair<QString, QString> result(sx,sy);
    return result;
}

QPair<QString, QString> LC_EntityPropertiesEditorSupport::toUIStrRaw(const RS_Vector &vect) const {
    QString sx = asString(vect.x);
    QString sy = asString(vect.y);
    QPair<QString, QString> result(sx,sy);
    return result;
}

void LC_EntityPropertiesEditorSupport::toUI(const RS_Vector &vect, QLineEdit* leX, QLineEdit* leY) const {
    const auto [fst, snd] = toUIStr(vect);
    leX->setText(fst);
    leY->setText(snd);
}

void LC_EntityPropertiesEditorSupport::toUIRaw(const RS_Vector &vect, QLineEdit* leX, QLineEdit* leY) const {
    const auto [fst, snd] = toUIStrRaw(vect);
    leX->setText(fst);
    leY->setText(snd);
}

void LC_EntityPropertiesEditorSupport::toUIValue(const double val, QLineEdit* ed) const {
    const QString sValue = asString(val);
    ed->setText(sValue);
}

void LC_EntityPropertiesEditorSupport::toUIAngleDeg(const double wcsAngle, QLineEdit* ed) const {
    const QString sValue = toUIAngleDeg(wcsAngle);
    ed->setText(sValue);
}

QString LC_EntityPropertiesEditorSupport::toUIAngleDeg(const double wcsAngle) const {
    const double ucsAngle = toUCSAngle(wcsAngle);
    QString sValue = asStringAngleDeg(ucsAngle);
    return sValue;
}

void LC_EntityPropertiesEditorSupport::toUIBool(const bool val, QCheckBox* ed){
    ed->setChecked(val);
}

void LC_EntityPropertiesEditorSupport::toUIAngleDegRaw(const double val, QLineEdit* ed) const {
    const QString sValue = asStringAngleDeg(val);
    ed->setText(sValue);
}

RS_Vector LC_EntityPropertiesEditorSupport::toWCSVector(const QString &sx, const QString &sy, const RS_Vector& wcsDefaults, const VectorModificationState state) const {
    RS_Vector ucsDefaults = toUCSVector(wcsDefaults);
    if (state == X) {
        const double uix = toDouble(sx, 0.0, ucsDefaults.x);
        ucsDefaults.x = uix;
    }
    else if (state == Y){
        const double uiy = toDouble(sy, 0.0, ucsDefaults.y);
        ucsDefaults.y = uiy;
    }
    else {
        const double uix = toDouble(sx, 0.0, ucsDefaults.x);
        const double uiy = toDouble(sy, 0.0, ucsDefaults.y);
        ucsDefaults.x = uix;
        ucsDefaults.y = uiy;
    }
    const RS_Vector entityPos = toWCSVector(ucsDefaults);
    return entityPos;
}

QLineEdit* LC_EntityPropertiesEditorSupport::getIfSender(QLineEdit* e, const QObject* sender) {
    return e == sender ? e: nullptr;
}

RS_Vector LC_EntityPropertiesEditorSupport::toWCS(const QLineEdit* leX, const QLineEdit* leY, const RS_Vector& wcsDefaults) const{
    VectorModificationState state = BOTH;
    const auto sndr = sender();
    if (leX == sndr) {
        state = X;
    }
    else if (leY == sndr) {
        state = Y;
    }
    return toWCSVector(leX->text(), leY->text(), wcsDefaults, state);
}

RS_Vector LC_EntityPropertiesEditorSupport::toWCSRaw(const QLineEdit* leX, const QLineEdit* leY, const RS_Vector& defs) const{
    return RS_Vector(toDouble(leX->text(), 0, defs.x), toDouble(leY->text(), 0, defs.y));
}

double LC_EntityPropertiesEditorSupport::toWCSValue(const QLineEdit* ed, const double wcsDefault) const {
    return toWCSValue(ed->text(), wcsDefault);
}

double LC_EntityPropertiesEditorSupport::toWCSAngle(const QLineEdit* ed, const double wcsDefault) const {
    return toWCSAngle(ed->text(), wcsDefault);
}

double LC_EntityPropertiesEditorSupport::toDouble(const QString &strValue, const double notMeaningful, const double defValue) const {
    double result = 0.;
    const bool meaningful =  toDouble(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

double LC_EntityPropertiesEditorSupport::toDoubleAngle(const QString &strValue, const double notMeaningful, const double defValue) const {
    double result = 0.;
    const bool meaningful =  toDoubleAngle(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

bool LC_EntityPropertiesEditorSupport::toDouble(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly) const{
    return LC_Convert::toDouble(strValue, res, notMeaningful, positiveOnly);
}

bool LC_EntityPropertiesEditorSupport::toDoubleAngle(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly) const{
    return LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
}

QString LC_EntityPropertiesEditorSupport::asString(const double value) const{
    return LC_Convert::asString(value);
}

QString LC_EntityPropertiesEditorSupport::asStringAngle(const double value) const{
    return LC_Convert::asStringAngle(value);
}

QString LC_EntityPropertiesEditorSupport::asStringAngleDeg(const double value) const{
    return LC_Convert::asStringAngleDeg(value);
}
