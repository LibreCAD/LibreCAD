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

#include <QLineEdit>
#include <QCheckBox>

#include "lc_convert.h"
#include "lc_entitypropertiesdlg.h"
#include "lc_graphicviewport.h"
#include "rs_math.h"
#include "rs_vector.h"

LC_EntityPropertiesDlg::LC_EntityPropertiesDlg(QWidget *parent, const QString& dlgName, LC_GraphicViewport *vp):
    LC_Dialog(parent, dlgName), m_viewport{vp}{}

RS_Vector LC_EntityPropertiesDlg::toUCSVector(const RS_Vector &vect) const{
    RS_Vector result = m_viewport->toUCS(vect);
    return result;
}

double LC_EntityPropertiesDlg::toUCSAngle(double wcsAngle) const {
    double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    double ucsBasisAngle = m_viewport->toBasisUCSAngle(ucsAngle);
    return ucsBasisAngle;
}

double LC_EntityPropertiesDlg::toWCSAngle(const QString &val, double wcsDefault){
    double ucsDefault = toUCSAngle(wcsDefault);
    double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);

    double ucsAngleDeg = toDoubleAngle(val, 0.0, ucsDefaultDeg);
    double ucsAngleBasis = RS_Math::deg2rad(ucsAngleDeg);
    double ucsAngleAbs = m_viewport->toAbsUCSAngle(ucsAngleBasis);
    double wcsAngle =  m_viewport->toWorldAngle(ucsAngleAbs);
    return wcsAngle;
}

double LC_EntityPropertiesDlg::toWCSValue(const QString& val, double wcsDefault){
    // note - this implementation assumes that there is no scale transformations due to coordinate system!
    return toDouble(val, 0, wcsDefault);
}

RS_Vector LC_EntityPropertiesDlg::toWCSVector(const RS_Vector &vect) const{
    RS_Vector result = m_viewport->toWorld(vect);
    return result;
}

double LC_EntityPropertiesDlg::toRawAngleValue(QLineEdit* ed, double ucsDefault) {
    double ucsDefaultDeg = RS_Math::rad2deg(ucsDefault);
    double rawAngleDeg = toDoubleAngle(ed->text(), 0.0, ucsDefaultDeg);
    double rawAngle = RS_Math::deg2rad(rawAngleDeg);
    return rawAngle;
}

QPair<QString, QString> LC_EntityPropertiesDlg::toUIStr(const RS_Vector &vect) const {
    RS_Vector uiPos = toUCSVector(vect);
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
    QPair<QString, QString> pair = toUIStr(vect);
    leX->setText(pair.first);
    leY->setText(pair.second);
}

void LC_EntityPropertiesDlg::toUIRaw(const RS_Vector &vect, QLineEdit* leX, QLineEdit* leY) const {
    QPair<QString, QString> pair = toUIStrRaw(vect);
    leX->setText(pair.first);
    leY->setText(pair.second);
}

void LC_EntityPropertiesDlg::toUIValue(double val, QLineEdit* ed){
    QString sValue = asString(val);
    ed->setText(sValue);
}

void LC_EntityPropertiesDlg::toUIAngleDeg(double wcsAngle, QLineEdit* ed){
    QString sValue = toUIAngleDeg(wcsAngle);
    ed->setText(sValue);
}

QString LC_EntityPropertiesDlg::toUIAngleDeg(double wcsAngle) const {
    double ucsAngle = toUCSAngle(wcsAngle);
    QString sValue = asStringAngleDeg(ucsAngle);
    return sValue;
}

void LC_EntityPropertiesDlg::toUIBool(bool val, QCheckBox* ed){
    ed->setChecked(val);
}

void LC_EntityPropertiesDlg::toUIAngleDegRaw(double val, QLineEdit* ed){
    QString sValue = asStringAngleDeg(val);
    ed->setText(sValue);
}

RS_Vector LC_EntityPropertiesDlg::toWCSVector(const QString &sx, const QString &sy, const RS_Vector& defaults) const {
    RS_Vector ucsDefaults = toUCSVector(defaults);
    double uix = toDouble(sx, 0.0, ucsDefaults.x);
    double uiy = toDouble(sy, 0.0, ucsDefaults.y);

    const RS_Vector &uiPos = RS_Vector(uix,uiy);
    RS_Vector entityPos = toWCSVector(uiPos);
    return entityPos;
}

RS_Vector LC_EntityPropertiesDlg::toWCS(QLineEdit* leX, const QLineEdit* leY, const RS_Vector& defaults) const{
    return toWCSVector(leX->text(), leY->text(), defaults);
}

RS_Vector LC_EntityPropertiesDlg::toWCSRaw(QLineEdit* leX, const QLineEdit* leY, const RS_Vector& defs) const{
    return RS_Vector(toDouble(leX->text(), 0, defs.x), toDouble(leY->text(), 0, defs.y));
}

double LC_EntityPropertiesDlg::toWCSValue(QLineEdit* ed, double wcsDefault){
    return toWCSValue(ed->text(), wcsDefault);
}

double LC_EntityPropertiesDlg::toWCSAngle(QLineEdit* ed, double wcsDefault){
    return toWCSAngle(ed->text(), wcsDefault);
}

double LC_EntityPropertiesDlg::toDouble(const QString &strValue, double notMeaningful, double defValue) const {
    double result = 0.;
    const bool meaningful =  toDouble(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

double LC_EntityPropertiesDlg::toDoubleAngle(const QString &strValue, double notMeaningful, double defValue) const {
    double result = 0.;
    const bool meaningful =  toDoubleAngle(strValue, result, notMeaningful, false);
    return meaningful ? result : defValue;
}

bool LC_EntityPropertiesDlg::toDouble(const QString& strValue, double &res, double notMeaningful, bool positiveOnly) const{
    return LC_Convert::toDouble(strValue, res, notMeaningful, positiveOnly);
}

bool LC_EntityPropertiesDlg::toDoubleAngle(const QString& strValue, double &res, double notMeaningful, bool positiveOnly) const{
    return LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
}

QString LC_EntityPropertiesDlg::asString(double value) const{
    return LC_Convert::asString(value);
}

QString LC_EntityPropertiesDlg::asStringAngle(double value) const{
    return LC_Convert::asStringAngle(value);
}

QString LC_EntityPropertiesDlg::asStringAngleDeg(double value) const{
    return LC_Convert::asStringAngleDeg(value);
}
