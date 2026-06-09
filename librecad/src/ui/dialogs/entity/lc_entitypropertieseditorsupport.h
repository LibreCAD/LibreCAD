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

#ifndef LC_ENTITYPROPERTIESEDITORSUPPORT_H
#define LC_ENTITYPROPERTIESEDITORSUPPORT_H

#include <QString>
#include <QWidget>

#include "rs_vector.h"

class QCheckBox;
class QLineEdit;
class LC_GraphicViewport;

class LC_EntityPropertiesEditorSupport: public QWidget{
    Q_OBJECT
public:
    explicit LC_EntityPropertiesEditorSupport(QWidget* parent);
    void setGraphicViewport(LC_GraphicViewport* viewport);
protected:
    enum VectorModificationState {
        X, Y, BOTH
    };

    LC_GraphicViewport* m_viewport;

    double toUCSAngle(double wcsAngle) const;
    RS_Vector toUCSVector(const RS_Vector& vect) const;

    QPair<QString, QString> toUIStr(const RS_Vector &vect) const;
    void toUI(const RS_Vector &vect, QLineEdit* leX, QLineEdit *leY) const;
    RS_Vector toWCSVector(const RS_Vector& vect) const;
    RS_Vector toWCSVector(const QString &sx, const QString &sy, const RS_Vector& wcsDefaults, VectorModificationState state) const;
    QLineEdit* getIfSender(QLineEdit* e, const QObject* sender);
    RS_Vector toWCS(const QLineEdit* leX, const QLineEdit* leY, const RS_Vector& wcsDefaults) const;

    QString asString(double value) const;
    QString asStringAngleDeg(double value) const;
    QString asStringAngle(double value) const;
    bool toDoubleAngle(const QString &strValue, double &res, double notMeaningful, bool positiveOnly) const;
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly) const;
    double toDouble(const QString &strValue, double notMeaningful = 0.0, double defValue = 0.0) const;
    double toDoubleAngle(const QString &strValue, double notMeaningful = 0.0, double defValue = 0.0) const;

    double toWCSValue(const QString &val, double wcsDefault) const;
    double toWCSValue(const QLineEdit *ed, double wcsDefault) const;
    void toUIValue(double val, QLineEdit *ed) const;
    void toUIAngleDeg(double wcsAngle, QLineEdit *ed) const;
    void toUIAngleDegRaw(double val, QLineEdit *ed) const;
    void toUIBool(bool val, QCheckBox *ed);

    double toWCSAngle(const QLineEdit *ed, double wcsDefault) const;
    double toWCSAngle(const QString &val, double wcsDefault) const;
    double toRawAngleValue(const QLineEdit *ed, double ucsDefault) const;
    void toUIRaw(const RS_Vector &vect, QLineEdit *leX, QLineEdit *leY) const;
    QPair<QString, QString> toUIStrRaw(const RS_Vector &vect) const;
    RS_Vector toWCSRaw(const QLineEdit *leX, const QLineEdit *leY, const RS_Vector& defs) const;

    QString toUIAngleDeg(double wcsAngle) const;
};

#endif
