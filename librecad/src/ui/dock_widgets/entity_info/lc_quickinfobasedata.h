
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
#include "rs_document.h"
#include "qg_graphicview.h"

class LC_QuickInfoBaseData
{
    Q_DECLARE_TR_FUNCTIONS(LC_QuickInfoBaseData)

public:
    LC_QuickInfoBaseData();
    QString getFormattedVectorForIndex(int index) const;
    virtual RS_Vector getVectorForIndex(int index) const = 0;
    virtual bool updateForCoordinateViewMode(int mode) = 0;
    virtual void clear() = 0;
    virtual bool hasData() const = 0;
    void setDocumentAndView(RS_Document *document, QG_GraphicView* view);

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
    RS_GraphicView* graphicView = nullptr;
    int coordinatesMode = COORD_ABSOLUTE;

    QString formatVector(const RS_Vector &vector) const;
    QString formatAngle(double angle);
    QString formatLinear(double length);
    QString createLink(QString &data, const QString &path, int index, const char *title, QString &value);
};

#endif // LC_QUICKINFOBASEDATA_H
