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
#include "rs_units.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "lc_graphicviewport.h"
#include "lc_quickinfobasedata.h"

LC_QuickInfoBaseData::LC_QuickInfoBaseData()= default;

/**
 * Performs formatting of given vector according to units specified by drawing preferences
 * @param vector vector
 * @return  formatted string
 */
QString LC_QuickInfoBaseData::formatVector(const RS_Vector &vector) const{
    double ucsX, ucsY;
    viewport->toUCS(vector, ucsX, ucsY);

    QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x + ", " + y;
    return result;
}

QString LC_QuickInfoBaseData::formatDeltaVector(const RS_Vector &vector) const{
    double ucsX, ucsY;
    viewport->toUCSDelta(vector, ucsX, ucsY);

    QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x + ", " + y;
    return result;
}

/**
 * performs formatting of angle value according to settings of drawing preferences
 * @param angle
 * @return
 */
QString LC_QuickInfoBaseData::formatAngle(double angle){
    if (viewport->hasUCS()){
        angle = viewport->toUCSAngle(angle);
    }
    return formatRawAngle(angle);
}

QString LC_QuickInfoBaseData::formatRawAngle(double angle) const {
    QString result =  RS_Units::formatAngle(angle, m_angleFormat, m_anglePrecision);
    return result;
}

/**
 * formats linear value according to settings of drawing preferences
 * @param length
 * @return
 */
QString LC_QuickInfoBaseData::formatLinear(double length){
    QString result = RS_Units::formatLinear(length,  m_unit,  m_linearFormat,m_linearPrecision);
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
QString LC_QuickInfoBaseData::createLink(QString & data, const QString &path, int index, QString title, QString & value){
    QString idx;
    idx.setNum(index);
    data.append("<a href='").append("/").append(path).append("?").append(idx).append("' title='").append(title).append("'").append(">");
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
void LC_QuickInfoBaseData::setDocumentAndView(RS_Document *doc, LC_GraphicViewport *view){
    clear();
    document = doc;
    viewport = view;
    updateFormats();
}

void LC_QuickInfoBaseData::updateFormats(){
    if (document != nullptr) {
        RS_Graphic *graphic = document->getGraphic();
        m_unit = graphic->getUnit();
        m_linearFormat = graphic->getLinearFormat();
        m_linearPrecision = graphic->getLinearPrecision();
        m_angleFormat = graphic->getAngleFormat();
        m_anglePrecision = graphic->getAnglePrecision();
    }
}

void LC_QuickInfoBaseData::appendLinear(QString &result, const QString &label, double value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatLinear(value));
}

void LC_QuickInfoBaseData::appendDouble(QString &result, const QString &label, double value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatDouble(value));
}

void LC_QuickInfoBaseData::appendArea(QString &result, const QString &label, double value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatLinear(value));
}

void LC_QuickInfoBaseData::appendAngle(QString &result, const QString &label, double value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatAngle(value));
}

void LC_QuickInfoBaseData::appendRawAngle(QString &result, const QString &label, double value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatRawAngle(value));
}

void LC_QuickInfoBaseData::appendValue(QString &result, const QString &label, const QString& value){
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(value);
}

void LC_QuickInfoBaseData::appendAbsolute(QString &result, const QString &label, const RS_Vector &value) {
    result.append("\n");
    result.append(label);
    result.append(": ");
    double ucsX, ucsY;
    viewport->toUCS(value, ucsX, ucsY);
    result.append(formatLinear(ucsX)).append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendAbsoluteDelta(QString &result, const QString &label, const RS_Vector &value) {
    result.append("\n");
    result.append(label);
    result.append(": ");
    double ucsX, ucsY;
    viewport->toUCSDelta(value, ucsX, ucsY);
    result.append(formatLinear(ucsX)).append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendRelativePolar(QString &result, const QString &label, const RS_Vector &value) {
    result.append("\n");
    result.append(label);
    result.append(": @");
    result.append(formatLinear(value.x)).append(" < ").append(formatAngle(value.y));
}

void LC_QuickInfoBaseData::appendInt(QString &result, const QString &label, const int &value) {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatInt(value));
}

/**
 * Formatting double value
 * @param x
 * @return
 */
QString LC_QuickInfoBaseData::formatDouble(const double &x) const{
    QString result =  RS_Units::formatDecimal(x, RS2::Unit::None, m_linearPrecision, false);
    return result;
}

/**
 * formatting int value
 * @param x
 * @return
 */
QString LC_QuickInfoBaseData::formatInt(const int &x) const{
    QString result;
    result.setNum(x);
    return result;
}

const RS_Vector &LC_QuickInfoBaseData::getRelativeZero() const {
    return viewport->getRelativeZero();
}
