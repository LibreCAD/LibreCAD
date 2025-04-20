
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

#include <QCoreApplication>
#include "rs_vector.h"
#include "rs.h"

class QString;
class LC_GraphicViewport;
class RS_Document;

class LC_QuickInfoBaseData{
    Q_DECLARE_TR_FUNCTIONS(LC_QuickInfoBaseData)
public:
    LC_QuickInfoBaseData();
    virtual ~LC_QuickInfoBaseData() = default;
    QString getFormattedVectorForIndex(int index) const;
    virtual RS_Vector getVectorForIndex(int index) const = 0;
    virtual bool updateForCoordinateViewMode(int mode) = 0;
    virtual void clear() = 0;
    virtual bool hasData() const = 0;
    void setDocumentAndView(RS_Document *document, LC_GraphicViewport* view);
    void updateFormats(); // fixme - sand - this method should be called as soon as settings will be updated..

    int getCoordinatesMode() const{return m_coordinatesMode;};
    void setCoordinatesMode(int value){m_coordinatesMode = value;};

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
    RS_Document* m_document = nullptr;
    LC_GraphicViewport* m_viewport = nullptr;
    int m_coordinatesMode = COORD_ABSOLUTE;

    RS2::Unit m_unit;
    RS2::LinearFormat m_linearFormat;
    int m_linearPrecision;
    RS2::AngleFormat m_angleFormat;
    int m_anglePrecision;

    double m_anglesBase = 0;
    bool m_anglesCounterClockWise = true;

    // fixme - sand - think about these formatting methods.. they are present there, and similar ones are in snapper...
    // fixme - what about moving them to RS_GraphicView which is shared anyway may be? And this will simplify updating cached formats...
    QString formatWCSVector(const RS_Vector &wcsPos) const;
    QString formatUCSVector(const RS_Vector &ucsPos) const;
    QString formatWCSDeltaVector(const RS_Vector &wcsDelta) const;
    QString formatWCSAngle(double wcsAngle) const;
    QString formatUCSAngle(double wcsAngle) const;
    QString formatLinear(double length) const;
    QString formatDouble(const double &x) const;
    QString formatInt(const int &x) const;
    QString createLink(QString &data, const QString &path, int index, const QString& title, const QString &value);
    void appendLinear(QString &result, const QString &label, double value) const;
    void appendDouble(QString &result, const QString &label, double value) const;
    void appendWCSAngle(QString &result, const QString &label, double value) const;
    void appendRawAngle(QString &result, const QString &label, double value) const;
    void appendArea(QString &result, const QString &label, double value) const;
    void appendWCSAbsolute(QString &result, const QString &label, const RS_Vector& value) const;
    void appendWCSAbsoluteDelta(QString &result, const QString &label, const RS_Vector& value) const;
    void appendRelativePolar(QString &result, const QString &label, const RS_Vector& value) const;
    void appendInt(QString &result, const QString &label, const int& value) const;
    void appendValue(QString &result, const QString &label, const QString& value);
    QString formatRawAngle(double angle) const;

    const RS_Vector& getRelativeZero() const;
};

#endif // LC_QUICKINFOBASEDATA_H
