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
#include "lc_quickinfobasedata.h"
#include "rs_graphic.h"
#include "rs_units.h"

LC_QuickInfoBaseData::LC_QuickInfoBaseData()= default;

/**
 * Performs formatting of given vector according to units specified by drawing preferences
 * @param vector vector
 * @return  formatted string
 */
QString LC_QuickInfoBaseData::formatVector(const RS_Vector &vector) const{

    RS_Graphic* graphic = document->getGraphic();
    QString x = RS_Units::formatLinear(vector.x,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());
    QString y = RS_Units::formatLinear(vector.y,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());

    QString result = x + ", " + y;
    return result;
}

/**
 * performs formatting of angle value according to settings of drawing preferences
 * @param angle
 * @return
 */
QString LC_QuickInfoBaseData::formatAngle(double angle){
    RS_Graphic* graphic = document->getGraphic();
    QString result =  RS_Units::formatAngle(angle,  graphic->getAngleFormat(), graphic->getAnglePrecision());
    return result;
}

/**
 * formats linear value according to settings of drawing preferences
 * @param length
 * @return
 */
QString LC_QuickInfoBaseData::formatLinear(double length){
    RS_Graphic* graphic = document->getGraphic();
    QString result = RS_Units::formatLinear(length,  graphic->getUnit(),  graphic->getLinearFormat(),graphic->getLinearPrecision());
    return result;
}

/**
 * creates HTML link tag and appends it to provided string
 * @param data string to append
 * @param path path value
 * @param index index that is appended as query of url
 * @param title title for link
 * @param value value for the link
 * @return original string to append
 */
QString LC_QuickInfoBaseData::createLink(QString & data, const QString &path, int index, const char* title, QString & value){
    QString idx;
    idx.setNum(index);
    data.append("<a href='").append("/").append(path).append("?").append(idx).append("' title='").append(tr(title)).append("'").append(">");
    data.append(value);
    data.append("</a>");
    return data;
}

/**
 * Creates formatted vector for given model index
 * @param index
 * @return
 */
QString LC_QuickInfoBaseData::getFormattedVectorForIndex(const int index) const{
    RS_Vector coord = getVectorForIndex(index);
    QString result = "";
    if (coord.valid){
        result = formatVector(coord);
    }
    return result;
}

/**
 * generic setup for document and view
 * @param doc
 * @param view
 */
void LC_QuickInfoBaseData::setDocumentAndView(RS_Document *doc, QG_GraphicView *view){
    clear();
    document = doc;
    graphicView = view;
}



