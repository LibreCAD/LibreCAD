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
#include "rs_math.h"
#include "lc_linemath.h"

// fixme - sand - review the entire codebase and insure uniform string/double and vice-versa conversion
QString LC_Convert::asString(double value, int precision){
    return QString::number(value, 'g', precision);
}

QString LC_Convert::asStringAngle(double value, int precision){
    return QString::number(value, 'g', precision);
}

QString LC_Convert::asStringAngleDeg(double value, int precision){
    double angleDeg = RS_Math::rad2deg(value);
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
bool LC_Convert::toDouble(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    bool ok = false;
    double x = RS_Math::eval(strValue, &ok);
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
bool LC_Convert::toDoubleAngleRad(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    double radValue;
    bool result = toDoubleAngle(strValue, radValue, notMeaningful, positiveOnly);
    if (result){
        res = RS_Math::deg2rad(radValue);
    }
    return result;
}

bool LC_Convert::toDoubleAngle(const QString& strValue, double &res, double notMeaningful, bool positiveOnly){
    bool ok = false;
    double x = RS_Math::eval(strValue, &ok);
    if(ok){
        res = LC_LineMath::getMeaningfulAngle(x, notMeaningful);
        if (positiveOnly){
            res = std::abs(res);
        }
    }
    return ok;
}
