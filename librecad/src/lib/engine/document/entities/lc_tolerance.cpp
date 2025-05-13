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
#include<iostream>
#include "lc_tolerance.h"

#include <QRegularExpression>

#include "rs_color.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_filterdxfrw.h"
#include "rs_line.h"
#include "rs_text.h"
#include "rs_units.h"

LC_ToleranceData::~LC_ToleranceData() = default;

LC_Tolerance::LC_Tolerance(RS_EntityContainer* parent, const LC_ToleranceData& d)
    :RS_EntityContainer(parent), m_toleranceData(d){
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
    if (isUndone()) {
        return;
    }

    doUpdateDim();

    calculateBorders();
}

RS_VectorSolutions LC_Tolerance::getRefPoints() const {
    return {m_toleranceData.m_insertionPoint};
}

void LC_Tolerance::move(const RS_Vector& offset) {
    m_toleranceData.m_insertionPoint.move(offset);
    update();
}

void LC_Tolerance::rotate(const RS_Vector& center, double angle) {
    m_toleranceData.m_insertionPoint.rotate(center, angle);
    update();
}

void LC_Tolerance::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_toleranceData.m_insertionPoint.rotate(center, angleVector);
    update();
}

void LC_Tolerance::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_toleranceData.m_insertionPoint.scale(center, factor);
    update();
}

void LC_Tolerance::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_toleranceData.m_insertionPoint.mirror(axisPoint1, axisPoint2);
    update();
}

void LC_Tolerance::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_toleranceData.m_insertionPoint)<1.0e-4) {
        m_toleranceData.m_insertionPoint += offset;
        update();
    }
}

std::ostream& operator<<(std::ostream& os, const LC_ToleranceData& td) {
    os << "("
        << td.m_insertionPoint << ','
        << td.m_directionVector << ','
        << td.m_dimStyleName.toLatin1().data() << ','
        << td.m_textCode.toLatin1().data() << ")";
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

    QStringList lines = m_toleranceData.m_textCode.split("^J");
    for (int k=0; k<lines.length(); k++) {
        QString line = lines[k];
        //qDebug() << "line:" << line;

        QStringList lineFields = line.split("%%v");
        ret.append(lineFields);
    }

    return ret;
}

double LC_Tolerance::getTextHeight() {
    return getGraphicVariable("$DIMTXT", 2.5, 40);
}

// fixme - sand - temporary method, move to entity?
double LC_Tolerance::getGraphicVariable(const QString& key, double defMM, int code) {
    double v = getGraphicVariableDouble(key, RS_MINDOUBLE);
    if (v <= RS_MINDOUBLE) {
        addGraphicVariable(key, RS_Units::convert(defMM, RS2::Millimeter, getGraphicUnit()), code);
        v = getGraphicVariableDouble(key, 1.0);
    }
    return v;
}

double LC_Tolerance::getDimtxt(bool scale) {
    double v = 2.5;

    // get value from override:
    if (dimtxt>0.0) {
        v = dimtxt;
    }
    else {
        v = getTextHeight();
    }

    if (scale) {
        v *= getDimscale();
    }

    return v;
}
void LC_Tolerance::setDimtxt(double f) {
    dimtxt = f;
    update();
}

double LC_Tolerance::getGeneralScale() {
    return getGraphicVariable("$DIMSCALE", 1.0, 40);
}

double LC_Tolerance::getDimscale()  {
    // get value from override:
    if (dimscale>0.0) {
        return dimscale;
    }

    double v = getGeneralScale();
    return v;
}

QString LC_Tolerance::getTextStyle() {
    return getGraphicVariableString("$DIMTXSTY", "standard");
}

RS_Color LC_Tolerance::getTextColor() {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRT", 0));
}

RS_Pen LC_Tolerance::getPenForText() {
    RS_Pen result(getTextColor(), RS2::WidthByBlock, RS2::SolidLine);
    return result;
}

RS_Color LC_Tolerance::getDimensionLineColor() {
    return RS_FilterDXFRW::numberToColor(getGraphicVariableInt("$DIMCLRD", 0));
}

RS_Pen LC_Tolerance::getPenForLines() {
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
    joinFirstField = false;

    // find out if we need to join the first fields of the first two lines:
    if (fields.length()>1 && fields[0].length()>0 && fields[1].length()>0) {
        QString field1 = fields[0][0];
        QString field2 = fields[1][0];
        QRegularExpression reg = QRegularExpression("\\\\F[gG][dD][tT];", QRegularExpression::CaseInsensitiveOption);
        field1.replace(reg, "\\Fgdt;");
        field2.replace(reg, "\\Fgdt;");
        if (!field1.isEmpty()) {
            joinFirstField = (field1==field2);
        }
    }

    double cursorY = 0;

    double angle = m_toleranceData.m_directionVector.angle();
    double textAngle = 0.0;

    RS_Pen textPen = getPenForText();

    for (int k=0; k<fields.length(); k++) {
        QStringList fieldsOfLine = fields[k];
        double cursorX = dimtxt/2.0;

        QList<double> row;
        row << 0.0;
        divisions.append(row);

        // render text strings with distance of dimtxt:
        for (int i=0; i<fieldsOfLine.length(); i++) {
            QString field = fieldsOfLine[i];
            LC_ERR << "field:" << field;
            if (field.isEmpty()) {
                continue;
            }

            RS_MTextData textData = RS_MTextData({cursorX, cursorY},
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
            if (k==0 && i==0 && joinFirstField) {
                text->move({0, -dimtxt});
            }

            cursorX += text->getUsedTextWidth();
            cursorX += dimtxt;
            divisions.last().push_back(cursorX - dimtxt/2);

            if (k==1 && i==0 && joinFirstField) {
                // skip first symbol of second line if fields are joined:
                continue;
            }
            text->rotate({0,0},angle);
            text->move(m_toleranceData.m_insertionPoint);

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
    QList<RS_Line> ret;

    double dimtxt = getDimtxt();
    double offsetY = 0.0;

    RS_Vector location = m_toleranceData.m_insertionPoint;
    double angle = m_toleranceData.m_directionVector.angle();

    RS_Pen linesPen = getPenForLines();

    for (int i = 0; i < divisions.length(); i++) {
        //qDebug() << "divisions:" << divisions[i];

        auto current = divisions[i];
        qsizetype currentLength = current.length();
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
                if (joinFirstField && i == 1 && currentLength > 1) {
                    startX = current[1];
                }
                RS_Line* line = addDimComponentLine({startX, dimtxt + offsetY}, {current.last(), dimtxt + offsetY}, linesPen);
                line->rotate(angle);
                line->move(location);
            }

            {
                // bottom line of current line:
                double startX = current.first();
                if (joinFirstField && i == 0 && currentLength > 1) {
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
    auto line = new RS_Line(this, {start, end});
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
