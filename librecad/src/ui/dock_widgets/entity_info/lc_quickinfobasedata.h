
/****************************************************************************
*
* Basic data holder for properties and coordinates. Includes just several
* utility methods.

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
**********************************************************************/
#ifndef LC_QUICKINFOBASEDATA_H
#define LC_QUICKINFOBASEDATA_H

#include <QString>
#include <QCoreApplication>
#include "rs_vector.h"
#include "rs.h"


class LC_GraphicViewport;
class RS_Document;

class LC_QuickInfoBaseData{
    Q_DECLARE_TR_FUNCTIONS(LC_QuickInfoBaseData)
public:
    LC_QuickInfoBaseData();
    QString getFormattedVectorForIndex(int index) const;
    virtual RS_Vector getVectorForIndex(int index) const = 0;
    virtual bool updateForCoordinateViewMode(int mode) = 0;
    virtual void clear() = 0;
    virtual bool hasData() const = 0;
    void setDocumentAndView(RS_Document *document, LC_GraphicViewport* view);
    void updateFormats(); // fixme - sand - this method should be called as soon as settings will be updated..

    int getCoordinatesMode() const{return coordinatesMode;};
    void setCoordinatesMode(int value){coordinatesMode = value;};

    /**
     * Defines the mode for displaying coordinates
     */
    enum CoordinateMode{
        COORD_ABSOLUTE, // absolute coordinate is shown
        COORD_RELATIVE, // coordinate relative to relative point is shown
        COORD_RELATIVE_TO_FIRST, // coordinate is relative to first coordinate in the list
        COORD_RELATIVE_TO_PREVIOUS // coordinate relative to previous coordinate in the list
    };

protected:
    RS_Document* document = nullptr;
    LC_GraphicViewport* viewport = nullptr;
    int coordinatesMode = COORD_ABSOLUTE;

    RS2::Unit m_unit;
    RS2::LinearFormat m_linearFormat;
    int m_linearPrecision;
    RS2::AngleFormat m_angleFormat;
    int m_anglePrecision;

    // fixme - sand - think about these formatting methods.. they are present there, and similar ones are in snapper...
    // fixme - what about moving them to RS_GraphicView which is shared anyway may be? And this will simplify updating cached formats...
    QString formatVector(const RS_Vector &vector) const;
    QString formatDeltaVector(const RS_Vector &vector) const;
    QString formatAngle(double angle);
    QString formatLinear(double length);
    QString formatDouble(const double &x) const;
    QString formatInt(const int &x) const;
    QString createLink(QString &data, const QString &path, int index, QString title, QString &value);
    void appendLinear(QString &result, const QString &label, double value);
    void appendDouble(QString &result, const QString &label, double value);
    void appendAngle(QString &result, const QString &label, double value);
    void appendRawAngle(QString &result, const QString &label, double value);
    void appendArea(QString &result, const QString &label, double value);
    void appendAbsolute(QString &result, const QString &label, const RS_Vector& value);
    void appendAbsoluteDelta(QString &result, const QString &label, const RS_Vector& value);
    void appendRelativePolar(QString &result, const QString &label, const RS_Vector& value);
    void appendInt(QString &result, const QString &label, const int& value);
    void appendValue(QString &result, const QString &label, const QString& value);
    QString formatRawAngle(double angle) const;

    const RS_Vector& getRelativeZero() const;
};

#endif // LC_QUICKINFOBASEDATA_H
