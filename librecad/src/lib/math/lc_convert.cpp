/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#include "lc_convert.h"

#include <QRegularExpression>

#include "lc_linemath.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"

// fixme - sand - review the entire codebase and insure uniform string/double and vice-versa conversion
QString LC_Convert::asString(double value, const int precision){
    if (LC_LineMath::isNotMeaningful(value)){
        value = 0.0;
    }
    return QString::number(value, 'g', precision);
}

QString LC_Convert::asStringAngle(double value, const int precision){
    if (!LC_LineMath::isMeaningfulAngle(value)){
        value = 0.0;
    }
    return QString::number(value, 'g', precision);
}

QString LC_Convert::asStringAngleDeg(double value, const int precision){
    if (!LC_LineMath::isMeaningfulAngle(value)){
        value = 0.0;
    }
    const double angleDeg = RS_Math::rad2deg(value);
    return QString::number(angleDeg, 'g', precision);
}


/**
 * Just utility method for conversion of provided string to double value.
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_Convert::toDouble(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
    bool ok = false;
    const double x = RS_Math::eval(strValue, &ok);
    if(ok){
        res = LC_LineMath::getMeaningful(x, notMeaningful);
        if (positiveOnly){
            res = std::abs(res);
        }
    }
    return ok;
}

/**
 * Just utility method for conversion of provided string to double value, given that double is for angle
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_Convert::toDoubleAngleRad(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
    double radValue;
    const bool result = parseToToDoubleAngleDegrees(strValue, radValue, notMeaningful, positiveOnly);
    if (result){
        const double resRad = RS_Math::deg2rad(radValue);
        res = RS_Math::correctAngle(resRad);
    }
    return result;
}


bool LC_Convert::parseToToDoubleAngleDegrees(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
    bool ok = false;
    const bool doNotAllowNonDecimalAnglesInput = LC_GET_ONE_BOOL("CADPreferences", "InputAnglesAsDecimalsOnly", false);
    double angleDegrees;
    // cleanup degree size, if any
    if (doNotAllowNonDecimalAnglesInput){
        QString stringToEval = strValue;
        stringToEval = stringToEval.remove( QChar(0xB0));
        angleDegrees = RS_Math::eval(stringToEval, &ok);
    }
    else{
        QString stringToEval = strValue;
        stringToEval = stringToEval.remove( QChar(0xB0));
        stringToEval = RS_Units::replaceAllPotentialAnglesByDecimalDegrees(stringToEval, &ok);
        angleDegrees = RS_Math::eval(stringToEval, &ok);
    }
    if(ok){
        res = LC_LineMath::getMeaningfulAngle(angleDegrees, notMeaningful);
        if (positiveOnly){
            res = std::abs(res);
        }
    }
    return ok;
}

/**
   * @{description}       Update a length string to support fraction
   *                      (1 1/2") to (1+1/2")
   *                      (1"1/2) to (1+1/2")
  */
QString LC_Convert::updateForFraction(QString input) {
    // if the expression is already valid, bypass fraction processing
    bool okay = false;
    const double value = RS_Math::eval(input, &okay);
    if (okay) {
        return QString{}.setNum(value, 'g', 10);
    }

    // support fraction at the end: (1'1/2) => (1 1/2')
    static const QRegularExpression rx{R"((\D*)([\d]+)\s*(['"])([\d]+)/([\d]+)\s*$)"};

    const QRegularExpressionMatch match = rx.match(input);
    if (match.hasMatch()) {
        const qsizetype pos = match.capturedStart();
        input = input.left(pos) + match.captured(1) + match.captured(2) + " " + match.captured(4) + "/" +
                match.captured(5) + match.captured(3);
    }
    static std::array<std::tuple<QRegularExpression, int, int>, 3> regexps{
                {{QRegularExpression{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*([\D$]))"},3, 5},
                    {QRegularExpression{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*(['"]))"},3, 5},
                    {QRegularExpression{R"((\D*)\s*([\d]+)/([\d]+)\s*([\D$]))"},2, 4},}};

    LC_LOG << "input=" << input;
    static QRegularExpression expression(R"(\s+)");
    for (auto &[regx, index, tailI]: regexps) {
        input = evaluateFraction(input, regx, index, tailI).replace(expression, QString{});
    }
    LC_LOG << "eval: " << input;
    return input;
}

QString LC_Convert::evaluateFraction(QString input, const QRegularExpression& rx, const int index, const int tailI) {
    const QString copy = input;
    const QRegularExpressionMatch match = rx.match(copy);

    if (match.hasMatch()) {
        const qsizetype pos = match.capturedStart();
        LC_ERR << "Evaluate: " << copy;
        const QString formula = ((index != 2) ? match.captured(2) + "+" : QString{}) + match.captured(index) + "/" +
                          match.captured(index + 1);
        LC_ERR << "formula=" << formula;
        const QString value = QString{}.setNum(RS_Math::eval(formula), 'g', 10);
        LC_ERR << "formula=" << formula << ": value=" << value;

        const QString tail = QString{R"(\)"} + QString::number(tailI);

        return input.left(pos)
               + input.mid(pos, match.capturedLength()).replace(rx, R"( \1)" + value + tail)
               + evaluateFraction(input.right(input.size() - pos - match.capturedLength()), rx, index, tailI);
    }
    return input;
}
[[deprecated]] // fixme - garbase value if doNotAllow is true and ok is valse?
double LC_Convert::evalAngleValue(const QString& angleStr, bool& ok2) {
    double angleDegrees;
    ok2 = parseToToDoubleAngleDegrees(angleStr, angleDegrees, 0.0, false);
    return angleDegrees;
}

bool LC_Convert::toInt(const QString& strVal, int& res) {
    bool ok;
    res = strVal.toInt(&ok);
    return ok;
}
