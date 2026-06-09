/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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


#include "lc_action_options_base.h"

#include <math.h>

#include "lc_convert.h"
#include "lc_linemath.h"
#include "rs_math.h"
#include "rs_settings.h"

/**
 * Generic method for saving settings. Simply delegates actual saving of options to doSaveSettings (that should be implemented in inherited widget),
 * by wrapping it by settings begin/end group calls.
 */
void LC_ActionOptionsBase::saveOptions(){
    LC_GROUP(getSettingsGroupName());
    doSaveOptions();
    LC_GROUP_END();
}

void LC_ActionOptionsBase::loadOptions() {
    LC_GROUP(getSettingsGroupName());
    doLoadOptions();
    LC_GROUP_END();
}

/**
 * Just utility method for conversion of provided string to double value.
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_ActionOptionsBase::toDouble(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
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
bool LC_ActionOptionsBase::toDoubleAngleDegrees(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
    const bool ok = LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
    return ok;
}

/**
 * Utility method for version of double value to string that will be displayed in editor
 * @param value double value
 * @return corresponding string
 */
QString LC_ActionOptionsBase::fromDouble(double value){
    // here we'll convert to decimal degrees always...regardless of original input format.
    if (LC_LineMath::isNotMeaningful(value)){
        value = 0.0;
    }
    return QString::number(value, 'g', 12);
}


/**
 * Utility method that loads setting value. Default implementation assumes that all settings for the action
 * has the same prefix, and this method creates full name of settings based on prefix returned by getSettingsOptionNamePrefix()
 * @param name short name of setting (without prefix)
 * @param defaultValue default value that should be used
 * @return setting value
 */
QString LC_ActionOptionsBase::load(const QString& name, const QString& defaultValue){
    const QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_STR(key, defaultValue);
}

/**
 * Utility method that loads int settings value
 * @param name
 * @param defaultValue
 * @return
 */
int LC_ActionOptionsBase::loadInt(const QString& name, const int defaultValue){
    const QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_INT(key, defaultValue);
}

bool LC_ActionOptionsBase::loadBool(const QString& name, const bool defaultValue){
    const QString key = getSettingsOptionNamePrefix() + name;
    return LC_GET_INT(key, defaultValue ? 1 : 0) == 1;
}

double LC_ActionOptionsBase::loadDouble(const QString& name, const double defaultValue){
    const QString key = getSettingsOptionNamePrefix() + name;
    const QString strValue = LC_GET_STR(key, "");
    if (strValue.isEmpty()) {
        return defaultValue;
    }
    double result = NAN;
    const bool ok = toDouble(strValue, result, 0.0, false);
    if (ok) {
        return result;
    }
    return defaultValue;
}

void LC_ActionOptionsBase::save(const QString& name, const QString& value){
    const QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value);
}

void LC_ActionOptionsBase::save(const QString& name, const int value){
    const QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value);
}

void LC_ActionOptionsBase::save(const QString& name, const bool value){
    const QString key = getSettingsOptionNamePrefix() + name;
    LC_SET(key, value ? 1 : 0);
}

void LC_ActionOptionsBase::save(const QString& name, const double value) {
    const QString key = getSettingsOptionNamePrefix() + name;
    const QString strValue = fromDouble(value);
    LC_SET(key,  strValue);
}

void LC_ActionOptionsBase::save(const QString& name, const RS_Vector& value) {
    const QString key = getSettingsOptionNamePrefix() + name;
    save(key + "_X", value.x);
    save(key + "_Y", value.y);
}

RS_Vector LC_ActionOptionsBase::loadVector(const QString& name, const RS_Vector& defaultValue) {
    const QString key = getSettingsOptionNamePrefix() + name;
    const double newX = loadDouble(key + "_X", defaultValue.x);
    const double newY = loadDouble(key + "_Y", defaultValue.x);
    const RS_Vector result(newX,newY);
    return result;
}
