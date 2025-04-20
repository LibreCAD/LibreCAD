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

#ifndef LC_ENTITYPROPERTIESDLG_H
#define LC_ENTITYPROPERTIESDLG_H

#include "lc_dialog.h"

class QCheckBox;
class LC_GraphicViewport;
class RS_Vector;
class QLineEdit;


class LC_EntityPropertiesDlg:public LC_Dialog{
    Q_OBJECT
public:
    explicit LC_EntityPropertiesDlg(QWidget* parent, const QString& dlgName, LC_GraphicViewport* viewport);
    virtual void updateEntity() = 0;
protected:
    LC_GraphicViewport* m_viewport;

    double toUCSAngle(double angle) const;
    RS_Vector toUCSVector(const RS_Vector& vect) const;

    QPair<QString, QString> toUIStr(const RS_Vector &vect) const;
    void toUI(const RS_Vector &vect, QLineEdit* sx, QLineEdit *sy) const;
    RS_Vector toWCSVector(const RS_Vector& vect) const;
    RS_Vector toWCSVector(const QString &sx, const QString &sy, const RS_Vector& wcsDefaults) const;
    RS_Vector toWCS(QLineEdit* leX, const QLineEdit* leY, const RS_Vector& wcsDefaults) const;

    QString asString(double value) const;
    QString asStringAngleDeg(double value) const;
    QString asStringAngle(double value) const;
    bool toDoubleAngle(const QString &strValue, double &res, double notMeaningful, bool positiveOnly) const;
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly) const;
    double toDouble(const QString &strValue, double notMeaningful = 0.0, double defValue = 0.0) const;
    double toDoubleAngle(const QString &strValue, double notMeaningful = 0.0, double defValue = 0.0) const;

    double toWCSValue(const QString &val, double wcsDefault);
    double toWCSValue(QLineEdit *ed, double wcsDefault);
    void toUIValue(double val, QLineEdit *ed);
    void toUIAngleDeg(double wcsAngle, QLineEdit *ed);
    void toUIAngleDegRaw(double val, QLineEdit *ed);
    void toUIBool(bool val, QCheckBox *ed);

    double toWCSAngle(QLineEdit *ed, double wcsDefault);
    double toWCSAngle(const QString &val, double wcsDefault);
    double toRawAngleValue(QLineEdit *ed, double ucsDefault);
    void toUIRaw(const RS_Vector &vect, QLineEdit *leX, QLineEdit *leY) const;
    QPair<QString, QString> toUIStrRaw(const RS_Vector &vect) const;
    RS_Vector toWCSRaw(QLineEdit *leX, const QLineEdit *leY, const RS_Vector& defs) const;

    QString toUIAngleDeg(double wcsAngle) const;
};

#endif // LC_ENTITYPROPERTIESDLG_H
