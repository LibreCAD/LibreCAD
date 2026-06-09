/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_entitypropertiesdlg.h"

#include <QCheckBox>
#include <QLineEdit>

#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "rs_math.h"
#include "rs_vector.h"

LC_EntityPropertiesDlg::LC_EntityPropertiesDlg(QWidget *parent, const QString& dlgName, LC_GraphicViewport *viewport):
    LC_Dialog(parent, dlgName), m_viewport{viewport}{}

RS_Vector LC_EntityPropertiesDlg::toUCSVector(const RS_Vector &vect) const{
    const RS_Vector result = m_viewport->toUCS(vect);
    return result;
}

double LC_EntityPropertiesDlg::toUCSAngle(const double wcsAngle) const {
    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    const double ucsBasisAngle = m_viewport->toBasisUCSAngle(ucsAngle);
    return ucsBasisAngle;
}

double LC_EntityPropertiesDlg::toWCSAngle(const QString &val, const double wcsDefault) const {
    const double ucsDefault = toUCSAngle(wcsDefault);
    const double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);

    const double ucsAngleDeg = toDoubleAngle(val, 0.0, ucsDefaultDeg);
    const double ucsAngleBasis = RS_Math::deg2rad(ucsAngleDeg);
    const double ucsAngleAbs = m_viewport->toAbsUCSAngle(ucsAngleBasis);
    const double wcsAngle =  m_viewport->toWorldAngle(ucsAngleAbs);
    return wcsAngle;
}

double LC_EntityPropertiesDlg::toWCSValue(const QString& val, const double wcsDefault) const {
    // note - this implementation assumes that there is no scale transformations due to coordinate system!
    return toDouble(val, 0, wcsDefault);
}

RS_Vector LC_EntityPropertiesDlg::toWCSVector(const RS_Vector &vect) const{
    const RS_Vector result = m_viewport->toWorld(vect);
    return result;
}

double LC_EntityPropertiesDlg::toRawAngleValue(const QLineEdit* ed, const double ucsDefault) const {
    const double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);
    const double rawAngleDeg = toDoubleAngle(ed->text(), 0.0, ucsDefaultDeg);
    const double rawAngle = RS_Math::deg2rad(rawAngleDeg);
    return rawAngle;
}

QPair<QString, QString> LC_EntityPropertiesDlg::toUIStr(const RS_Vector &vect) const {
    const RS_Vector uiPos = toUCSVector(vect);
    QString sx = asString(uiPos.x);
    QString sy = asString(uiPos.y);
    QPair<QString, QString> result(sx,sy);
    return result;
}

QPair<QString, QString> LC_EntityPropertiesDlg::toUIStrRaw(const RS_Vector &vect) const {
    QString sx = asString(vect.x);
    QString sy = asString(vect.y);
    QPair<QString, QString> result(sx,sy);
    return result;
}

void LC_EntityPropertiesDlg::toUI(const RS_Vector &vect, QLineEdit* leX, QLineEdit* leY) const {
    const auto [fst, snd] = toUIStr(vect);
    leX->setText(fst);
    leY->setText(snd);
}

void LC_EntityPropertiesDlg::toUIRaw(const RS_Vector &vect, QLineEdit* leX, QLineEdit* leY) const {
    const auto [fst, snd] = toUIStrRaw(vect);
    leX->setText(fst);
    leY->setText(snd);
}

void LC_EntityPropertiesDlg::toUIValue(const double val, QLineEdit* ed) const {
    const QString sValue = asString(val);
    ed->setText(sValue);
}

void LC_EntityPropertiesDlg::toUIAngleDeg(const double wcsAngle, QLineEdit* ed) const {
    const QString sValue = toUIAngleDeg(wcsAngle);
    ed->setText(sValue);
}

QString LC_EntityPropertiesDlg::toUIAngleDeg(const double wcsAngle) const {
    const double ucsAngle = toUCSAngle(wcsAngle);
    QString sValue = asStringAngleDeg(ucsAngle);
    return sValue;
}

void LC_EntityPropertiesDlg::toUIBool(const bool val, QCheckBox* ed){
    ed->setChecked(val);
}

void LC_EntityPropertiesDlg::toUIAngleDegRaw(const double val, QLineEdit* ed) const {
    const QString sValue = asStringAngleDeg(val);
    ed->setText(sValue);
}

RS_Vector LC_EntityPropertiesDlg::toWCSVector(const QString &sx, const QString &sy, const RS_Vector& wcsDefaults) const {
    const RS_Vector ucsDefaults = toUCSVector(wcsDefaults);
    const double uix = toDouble(sx, 0.0, ucsDefaults.x);
    const double uiy = toDouble(sy, 0.0, ucsDefaults.y);

    const auto& uiPos = RS_Vector(uix,uiy);
    const RS_Vector entityPos = toWCSVector(uiPos);
    return entityPos;
}

RS_Vector LC_EntityPropertiesDlg::toWCS(const QLineEdit* leX, const QLineEdit* leY, const RS_Vector& wcsDefaults) const{
    return toWCSVector(leX->text(), leY->text(), wcsDefaults);
}

RS_Vector LC_EntityPropertiesDlg::toWCSRaw(const QLineEdit* leX, const QLineEdit* leY, const RS_Vector& defs) const{
    return RS_Vector(toDouble(leX->text(), 0, defs.x), toDouble(leY->text(), 0, defs.y));
}

double LC_EntityPropertiesDlg::toWCSValue(const QLineEdit* ed, const double wcsDefault) const {
    return toWCSValue(ed->text(), wcsDefault);
}

double LC_EntityPropertiesDlg::toWCSAngle(const QLineEdit* ed, const double wcsDefault) const {
    return toWCSAngle(ed->text(), wcsDefault);
}

double LC_EntityPropertiesDlg::toDouble(const QString &strValue, const double notMeaningful, const double defValue) const {
    double result = 0.;
    const bool meaningful =  toDouble(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

double LC_EntityPropertiesDlg::toDoubleAngle(const QString &strValue, const double notMeaningful, const double defValue) const {
    double result = 0.;
    const bool meaningful =  toDoubleAngle(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

bool LC_EntityPropertiesDlg::toDouble(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly) const{
    return LC_Convert::toDouble(strValue, res, notMeaningful, positiveOnly);
}

bool LC_EntityPropertiesDlg::toDoubleAngle(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly) const{
    return LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
}

QString LC_EntityPropertiesDlg::asString(const double value) const{
    return LC_Convert::asString(value);
}

QString LC_EntityPropertiesDlg::asStringAngle(const double value) const{
    return LC_Convert::asStringAngle(value);
}

QString LC_EntityPropertiesDlg::asStringAngleDeg(const double value) const{
    return LC_Convert::asStringAngleDeg(value);
}
