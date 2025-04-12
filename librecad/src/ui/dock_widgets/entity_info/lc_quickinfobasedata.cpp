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
 * @param wcsPos vector
 * @return  formatted string
 */
QString LC_QuickInfoBaseData::formatWCSVector(const RS_Vector &wcsPos) const{
    double ucsX, ucsY;
    m_viewport->toUCS(wcsPos, ucsX, ucsY);

    QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x + ", " + y;
    return result;
}

QString LC_QuickInfoBaseData::formatUCSVector(const RS_Vector &ucsPos) const{
    QString x = RS_Units::formatLinear(ucsPos.x, m_unit, m_linearFormat, m_linearPrecision);
    QString y = RS_Units::formatLinear(ucsPos.y, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x + ", " + y;
    return result;
}

QString LC_QuickInfoBaseData::formatWCSDeltaVector(const RS_Vector &wcsDelta) const{
    double ucsX, ucsY;
    m_viewport->toUCSDelta(wcsDelta, ucsX, ucsY);

    QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x + ", " + y;
    return result;
}

/**
 * performs formatting of angle value according to settings of drawing preferences
 * @param wcsAngle
 * @return
 */
QString LC_QuickInfoBaseData::formatWCSAngle(double wcsAngle) const {
    if (m_viewport->hasUCS()){
        wcsAngle = m_viewport->toUCSAngle(wcsAngle);
    }
    double ucsRelAngle = m_viewport->toUCSBasisAngle(wcsAngle, m_anglesBase, m_anglesCounterClockWise);
    return formatRawAngle(ucsRelAngle);
}

QString LC_QuickInfoBaseData::formatUCSAngle(double wcsAngle) const {
    double ucsRelAngle = m_viewport->toUCSBasisAngle(wcsAngle, m_anglesBase, m_anglesCounterClockWise);
    return formatRawAngle(ucsRelAngle);
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
QString LC_QuickInfoBaseData::formatLinear(double length) const {
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
QString LC_QuickInfoBaseData::createLink(QString & data, const QString &path, int index, const QString& title, const QString & value){
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
        result = formatWCSVector(coord);
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
    m_document = doc;
    m_viewport = view;
    updateFormats();
}

void LC_QuickInfoBaseData::updateFormats(){
    if (m_document != nullptr) {
        RS_Graphic *graphic = m_document->getGraphic();
        m_unit = graphic->getUnit();
        m_linearFormat = graphic->getLinearFormat();
        m_linearPrecision = graphic->getLinearPrecision();
        m_angleFormat = graphic->getAngleFormat();
        m_anglePrecision = graphic->getAnglePrecision();

        m_anglesBase = graphic->getAnglesBase();
        m_anglesCounterClockWise = graphic->areAnglesCounterClockWise();
    }
}

void LC_QuickInfoBaseData::appendLinear(QString &result, const QString &label, double value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatLinear(value));
}

void LC_QuickInfoBaseData::appendDouble(QString &result, const QString &label, double value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatDouble(value));
}

void LC_QuickInfoBaseData::appendArea(QString &result, const QString &label, double value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatLinear(value));
}

void LC_QuickInfoBaseData::appendWCSAngle(QString &result, const QString &label, double value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(formatWCSAngle(value));
}

void LC_QuickInfoBaseData::appendRawAngle(QString &result, const QString &label, double value) const {
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

void LC_QuickInfoBaseData::appendWCSAbsolute(QString &result, const QString &label, const RS_Vector &value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    double ucsX, ucsY;
    m_viewport->toUCS(value, ucsX, ucsY);
    result.append(formatLinear(ucsX)).append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendWCSAbsoluteDelta(QString &result, const QString &label, const RS_Vector &value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    double ucsX, ucsY;
    m_viewport->toUCSDelta(value, ucsX, ucsY);
    result.append(formatLinear(ucsX)).append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendRelativePolar(QString &result, const QString &label, const RS_Vector &value) const {
    result.append("\n");
    result.append(label);
    result.append(": @");
    result.append(formatLinear(value.x)).append(" < ").append(formatWCSAngle(value.y));
}

void LC_QuickInfoBaseData::appendInt(QString &result, const QString &label, const int &value) const {
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
    return m_viewport->getRelativeZero();
}
