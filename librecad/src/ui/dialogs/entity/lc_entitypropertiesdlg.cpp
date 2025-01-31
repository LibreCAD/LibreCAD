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
#include "rs_vector.h"
#include "lc_graphicviewport.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_convert.h"

LC_EntityPropertiesDlg::LC_EntityPropertiesDlg(QWidget *parent, const QString& dlgName, LC_GraphicViewport *vp):
    LC_Dialog(parent, dlgName), viewport{vp}{}

LC_EntityPropertiesDlg::~LC_EntityPropertiesDlg() {}

QString LC_EntityPropertiesDlg::toUIAngle(double angle) const{
    return QString();
}

double LC_EntityPropertiesDlg::toEntityAngle(QString value) const{
    return 0;
}

RS_Vector LC_EntityPropertiesDlg::toUIVector(const RS_Vector &vect) const{
    return RS_Vector();
}

RS_Vector LC_EntityPropertiesDlg::toEntityVector(const RS_Vector &vect) const{
    return RS_Vector();
}

double LC_EntityPropertiesDlg::toDouble(const QString &strValue, double notMeaningful, double defValue) {
    double result;
    if (!toDouble(strValue, result, notMeaningful, false)){
        result = defValue;
    }
    return result;
}

bool LC_EntityPropertiesDlg::toDouble(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    return LC_Convert::toDouble(strValue, res, notMeaningful, positiveOnly);
}

bool LC_EntityPropertiesDlg::toDoubleAngle(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    return LC_Convert::toDoubleAngle(strValue, res, notMeaningful, positiveOnly);
}

QString LC_EntityPropertiesDlg::asString(double value){
    return LC_Convert::asString(value);
}

QString LC_EntityPropertiesDlg::asStringAngle(double value){
    return LC_Convert::asStringAngle(value);
}

QString LC_EntityPropertiesDlg::asStringAngleDeg(double value){
    return LC_Convert::asStringAngleDeg(value);
}
