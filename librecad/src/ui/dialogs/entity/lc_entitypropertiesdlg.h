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

class LC_GraphicViewport;
class RS_Vector;

class LC_EntityPropertiesDlg:public LC_Dialog{
    Q_OBJECT
public:
    explicit LC_EntityPropertiesDlg(QWidget* parent, const QString& dlgName, LC_GraphicViewport* viewport);
    ~LC_EntityPropertiesDlg() override;
    virtual void updateEntity() = 0;
protected:
    LC_GraphicViewport* viewport;

    QString toUIAngle(double angle) const;
    double toEntityAngle(QString value) const;
    RS_Vector toUIVector(const RS_Vector& vect) const;
    RS_Vector toEntityVector(const RS_Vector& vect) const;

    QString asString(double value);
    QString asStringAngleDeg(double value);
    QString asStringAngle(double value);
    bool toDoubleAngle(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    double toDouble(const QString &strValue, double notMeaningful = 0.0, double defValue = 0.0);
};

#endif // LC_ENTITYPROPERTIESDLG_H
