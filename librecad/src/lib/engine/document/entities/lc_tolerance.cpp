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
#include "lc_tolerance.h"

#include <QRegularExpression>
#include<iostream>

#include "rs_color.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_filterdxfrw.h"
#include "rs_line.h"
#include "rs_units.h"

LC_ToleranceData::~LC_ToleranceData() = default;

LC_Tolerance::LC_Tolerance(RS_EntityContainer* parent, const LC_ToleranceData& d)
    :RS_EntityContainer(parent), m_joinFirstField{false}, m_toleranceData(d), m_dimtxt{0.0}{
    RS_EntityContainer::calculateBorders();
}

RS_Entity* LC_Tolerance::clone() const {
    auto* d = new LC_Tolerance(*this);
    d->setOwner(isOwner());
    d->detach();
    return d;
}

void LC_Tolerance::update() {
    clear();
    if (isDeleted()) {
        return;
    }

    doUpdateDim();

    calculateBorders();
}

RS_VectorSolutions LC_Tolerance::getRefPoints() const {
    return {m_toleranceData.insertionPoint};
}

void LC_Tolerance::move(const RS_Vector& offset) {
    m_toleranceData.insertionPoint.move(offset);
    update();
}

void LC_Tolerance::rotate(const RS_Vector& center, const double angle) {
    m_toleranceData.insertionPoint.rotate(center, angle);
    update();
}

void LC_Tolerance::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_toleranceData.insertionPoint.rotate(center, angleVector);
    update();
}

void LC_Tolerance::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_toleranceData.insertionPoint.scale(center, factor);
    update();
}

void LC_Tolerance::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_toleranceData.insertionPoint.mirror(axisPoint1, axisPoint2);
    update();
}

void LC_Tolerance::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_toleranceData.insertionPoint)<1.0e-4) {
        m_toleranceData.insertionPoint += offset;
        update();
    }
}

std::ostream& operator<<(std::ostream& os, const LC_ToleranceData& td) {
    os << "("
        << td.insertionPoint << ','
        << td.directionVector << ','
        << td.dimStyleName.toLatin1().data() << ','
        << td.textCode.toLatin1().data() << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const LC_Tolerance& d) {
    os << " Tolerance: " << d.getData() << "\n";
    return os;
}

void LC_Tolerance::doUpdate() {

}

QList<QStringList> LC_Tolerance::getFields() const {
    // create list of string lists with field texts:
    QList<QStringList> ret;

    QStringList lines = m_toleranceData.textCode.split("^J");
    const qsizetype len = lines.length();
    for (qsizetype k = 0; k < len; k++) {
        const QString& line = lines[k];
        QStringList lineFields = line.split("%%v");
        ret.append(lineFields);
    }

    return ret;
}

double LC_Tolerance::getTextHeight() const {
    return getGraphicVariable("$DIMTXT", 2.5, 40);
}

// fixme - sand - temporary method, move to entity?
double LC_Tolerance::getGraphicVariable(const QString& key, const double defMM, const int code) const {
    double v = getGraphicVariableDouble(key, RS_MINDOUBLE);
    if (v <= RS_MINDOUBLE) {
        addGraphicVariable(key, RS_Units::convert(defMM, RS2::Millimeter, getGraphicUnit()), code);
        v = getGraphicVariableDouble(key, 1.0);
    }
    return v;
}

double LC_Tolerance::getDimtxt(const bool scale) const {
    double v = 2.5;

    // get value from override:
    if (m_dimtxt>0.0) {
        v = m_dimtxt;
    }
    else {
        v = getTextHeight();
    }

    if (scale) {
        v *= getDimscale();
    }

    return v;
}
void LC_Tolerance::setDimtxt(const double f) {
    m_dimtxt = f;
    update();
}

double LC_Tolerance::getGeneralScale() const {
    return getGraphicVariable("$DIMSCALE", 1.0, 40);
}

double LC_Tolerance::getDimscale() const {
    // get value from override:
    if (m_dimscale>0.0) {
        return m_dimscale;
    }

    const double v = getGeneralScale();
    return v;
}

QString LC_Tolerance::getTextStyle() const {
    return getGraphicVariableString("$DIMTXSTY", "standard");
}

RS_Color LC_Tolerance::getTextColor() const {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRT", 0));
}

RS_Pen LC_Tolerance::getPenForText() const {
    RS_Pen result(getTextColor(), RS2::WidthByBlock, RS2::SolidLine);
    return result;
}

RS_Color LC_Tolerance::getDimensionLineColor() const {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRD", 0));
}

RS_Pen LC_Tolerance::getPenForLines() const {
    // RS_Pen result(RS_Color(RS2::FlagByBlock), RS2::WidthByBlock, RS2::SolidLine);
    RS_Pen result(getDimensionLineColor(), RS2::WidthByBlock, RS2::SolidLine);
    return result;
}

void LC_Tolerance::doUpdateDim(){
    QList<QList<double>> divisionPoints;

    createTextLabels(divisionPoints);
    createFrameLines(divisionPoints);

    divisionPoints.clear();
}


void LC_Tolerance::createTextLabels(QList<QList<double>> &divisions) {
    double dimtxt = getDimtxt();
    //qDebug() << "text:" << text;

    QList<QStringList> fields = getFields();
    m_joinFirstField = false;

    qint64 qsizetype = fields.length();
    // find out if we need to join the first fields of the first two lines:
    if (qsizetype > 1 && !fields[0].empty() && !fields[1].empty()) {
        QString field1 = fields[0][0];
        QString field2 = fields[1][0];
        auto reg = QRegularExpression("\\\\F[gG][dD][tT];", QRegularExpression::CaseInsensitiveOption);
        field1.replace(reg, "\\Fgdt;");
        field2.replace(reg, "\\Fgdt;");
        if (!field1.isEmpty()) {
            m_joinFirstField = field1==field2;
        }
    }

    double cursorY = 0;

    double angle = m_toleranceData.directionVector.angle();
    double textAngle = 0.0;

    RS_Pen textPen = getPenForText();

    for (qint64 k=0; k<qsizetype; k++) {
        const QStringList& fieldsOfLine = fields[k];
        double cursorX = dimtxt/2.0;

        QList<double> row;
        row << 0.0;
        divisions.append(row);

        qint64 len = fieldsOfLine.length();
        // render text strings with distance of dimtxt:
        for (qint64 i = 0; i < len; i++) {
            const QString& field = fieldsOfLine[i];
            LC_ERR << "field:" << field;
            if (field.isEmpty()) {
                continue;
            }

            auto textData = RS_MTextData({cursorX, cursorY},
                        dimtxt, 30.0,
                        RS_MTextData::VAMiddle,
                        RS_MTextData::HALeft,
                        RS_MTextData::LeftToRight,
                        RS_MTextData::Exact,
                        1.0,
                        field,
                        getTextStyle(),
                        textAngle);

            auto text = new RS_MText(this, textData);
            addDimComponentEntity(text, textPen);

            // move first symbol of first line down if fields are joined:
            if (k==0 && i==0 && m_joinFirstField) {
                text->move({0, -dimtxt});
            }

            cursorX += text->getUsedTextWidth();
            cursorX += dimtxt;
            divisions.last().push_back(cursorX - (dimtxt / 2));

            if (k==1 && i==0 && m_joinFirstField) {
                // skip first symbol of second line if fields are joined:
                continue;
            }
            text->rotate({0,0},angle);
            text->move(m_toleranceData.insertionPoint);
        }

        if (!divisions.isEmpty() && divisions.last().length() ==1) {
            // remove single division line:
            divisions.removeLast();
        }
        else {
            cursorY -= dimtxt * 2;
        }
    }
}
void LC_Tolerance::createFrameLines(QList<QList<double>> &divisions)  {

    const double dimtxt = getDimtxt();
    double offsetY = 0.0;

    const RS_Vector location = m_toleranceData.insertionPoint;
    const double angle = m_toleranceData.directionVector.angle();

    const RS_Pen linesPen = getPenForLines();
    const qint64 divisionsLen = divisions.length();
    for (qint64 i = 0; i < divisionsLen; i++) {
        //qDebug() << "divisions:" << divisions[i];

        const auto& current = divisions[i];
        const qsizetype currentLength = current.length();
        // never show vertical lines for empty rows:
        if (currentLength > 1) {
            for (int k = 0; k < currentLength; k++) {
                double division = current.at(k);
                RS_Line* line = addDimComponentLine({division, -dimtxt + offsetY}, {division, dimtxt + offsetY}, linesPen);
                line->rotate(angle);
                line->move(location);
            }

            {
                // top line of current line:
                double startX = current.first();
                if (m_joinFirstField && i == 1 && currentLength > 1) {
                    startX = current[1];
                }
                RS_Line* line = addDimComponentLine({startX, dimtxt + offsetY}, {current.last(), dimtxt + offsetY}, linesPen);
                line->rotate(angle);
                line->move(location);
            }

            {
                // bottom line of current line:
                double startX = current.first();
                if (m_joinFirstField && i == 0 && currentLength > 1) {
                    startX = current[1];
                }
                RS_Line* line = addDimComponentLine({startX, -dimtxt + offsetY}, {current.last(), -dimtxt + offsetY}, linesPen);
                line->rotate(angle);
                line->move(location);
            }
        }

        if (!current.isEmpty()) {
            offsetY -= dimtxt * 2;
        }
    }
}

RS_Line* LC_Tolerance::addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen &pen) {
    const auto line = new RS_Line(this, {start, end});
    line->setPen(pen);
    line->setLayer(nullptr);
    addEntity(line);
    return line;
}

void LC_Tolerance::addDimComponentEntity(RS_Entity* en, const RS_Pen &pen) {
    en->setPen(pen);
    en->setLayer(nullptr);
    addEntity(en);
}
