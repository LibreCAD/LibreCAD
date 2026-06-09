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

#include "lc_graphicviewport.h"
#include "rs_graphic.h"

LC_QuickInfoBaseData::LC_QuickInfoBaseData()= default;

/**
 * Performs formatting of given vector according to units specified by drawing preferences
 * @param wcsPos vector
 * @return  formatted string
 */
QString LC_QuickInfoBaseData::formatWCSVector(const RS_Vector &wcsPos) const{
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatWCSVector(wcsPos);
}

QString LC_QuickInfoBaseData::formatUCSVector(const RS_Vector &ucsPos) const{
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatUCSVector(ucsPos);
}

QString LC_QuickInfoBaseData::formatWCSDeltaVector(const RS_Vector &wcsDelta) const{
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatWCSDeltaVector(wcsDelta);
}

/**
 * performs formatting of angle value according to settings of drawing preferences
 * @param wcsAngle
 * @return
 */
QString LC_QuickInfoBaseData::formatWCSAngle(const double wcsAngle) const {
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatWCSAngle(wcsAngle);
}

QString LC_QuickInfoBaseData::formatUCSAngle(const double wcsAngle) const {
     if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatUCSAngle(wcsAngle);
}

QString LC_QuickInfoBaseData::formatRawAngle(const double angle) const {
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatRawAngle(angle);
}

/**
 * Formatting double value
 * @param x
 * @return
 */
QString LC_QuickInfoBaseData::formatDouble(const double x) const{
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatDouble(x);
}

/**
 * formatting int value
 * @param x
 * @return
 */
QString LC_QuickInfoBaseData::formatInt(const int x) const{
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatInt(x);
}

/**
 * formats linear value according to settings of drawing preferences
 * @param length
 * @return
 */
QString LC_QuickInfoBaseData::formatLinear(const double length) const {
    if (m_formatter == nullptr) {
        return "";
    }
    return m_formatter->formatLinear(length);
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
QString LC_QuickInfoBaseData::createLink(QString & data, const QString &path, const int index, const QString& title, const QString & value){
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
    const RS_Vector coord = getVectorForIndex(index);
    QString result;
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
void LC_QuickInfoBaseData::setDocumentAndView(RS_Document *doc, LC_GraphicViewport *view) {
    clear();
    m_document = doc;
    m_viewport = view;
    if (view != nullptr) {
        m_formatter = view->getFormatter();
        updateFormats();
    }
    else{
        m_formatter = nullptr;
    }
}

void LC_QuickInfoBaseData::updateFormats(){
    if (m_document != nullptr) {
        const RS_Graphic *graphic = m_document->getGraphic();
        m_formatter->updateByGraphic(graphic); // fixme - fmt - most probably it's not necessary
    }
}

void LC_QuickInfoBaseData::appendLinear(QString &result, const QString &label, const double value) const {
    createLabelValueString(result, label,formatLinear(value));
}

void LC_QuickInfoBaseData::createLabelValueString(QString& result, const QString& label, const QString & value) const {
    result.append("\n");
    result.append(label);
    result.append(": ");
    result.append(value);
}

void LC_QuickInfoBaseData::appendDouble(QString &result, const QString &label, const double value) const {
    createLabelValueString(result, label, formatDouble(value));
}

void LC_QuickInfoBaseData::appendArea(QString &result, const QString &label, const double value) const {
    createLabelValueString(result, label,formatLinear(value));
}

void LC_QuickInfoBaseData::appendWCSAngle(QString &result, const QString &label, const double value) const {
    createLabelValueString(result, label,formatWCSAngle(value));
}

void LC_QuickInfoBaseData::appendRawAngle(QString &result, const QString &label, const double value) const {
    createLabelValueString(result, label,formatRawAngle(value));
}

void LC_QuickInfoBaseData::appendValue(QString &result, const QString &label, const QString& value) const {
    createLabelValueString(result, label, value);
}

void LC_QuickInfoBaseData::appendWCSAbsolute(QString &result, const QString &label, const RS_Vector &value) const {
    if (m_viewport == nullptr) {
        return;
    }
    double ucsX, ucsY;
    m_viewport->toUCS(value, ucsX, ucsY);
    createLabelValueString(result, label, formatLinear(ucsX));
    result.append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendWCSAbsoluteDelta(QString &result, const QString &label, const RS_Vector &value) const {
    if (m_viewport == nullptr) {
        return;
    }

    double ucsX, ucsY;
    m_viewport->toUCSDelta(value, ucsX, ucsY);
    createLabelValueString(result, label, formatLinear(ucsX));
    result.append(",").append(formatLinear(ucsY));
}

void LC_QuickInfoBaseData::appendRelativePolar(QString &result, const QString &label, const RS_Vector &value) const {
    createLabelValueString(result, label,"@");
    result.append(formatLinear(value.x)).append(" < ").append(formatWCSAngle(value.y));
}

void LC_QuickInfoBaseData::appendInt(QString &result, const QString &label, const int value) const {
    createLabelValueString(result, label,formatInt(value));
}

const RS_Vector &LC_QuickInfoBaseData::getRelativeZero() const {
    if (m_viewport == nullptr) {
        return m_absentRelZero;
    }
    return m_viewport->getRelativeZero();
}
