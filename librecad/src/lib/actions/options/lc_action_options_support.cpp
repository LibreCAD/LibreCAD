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

#include "lc_action_options_support.h"

#include "lc_convert.h"
#include "lc_linemath.h"
#include "rs_actioninterface.h"
#include "rs_math.h"


/**
 * Setter for the action. Checks that action rtti type is valid and delegates actual processing to doSetAction.
 * doSetAction() is wrapped in beginGroup/endGroup for settings, and the name of group is defined by
 * getSettingsGroupName() method.
 * @param a action
 * @param update true if option values should be updated by the action, false - if they should be read from settings
 */
void LC_ActionOptionsSupport::setAction(RS_ActionInterface *a, [[maybe_unused]]const bool update){
    if (a != nullptr){
        const RS2::ActionType actionType = a->rtti();
        if (checkActionRttiValid(actionType)){
            // that should be ok for the most of the actions as most probably they will rely on the same group
            preSetupByAction(a);
            doUpdateByAction(a);
        }
        else{
            // Q_ASSERT_X(false, typeid(*this).name(), "::setAction: wrong action type");
        }
    }
    else {
        cleanup();
    }
}


/**
 * Extension point for specific widgets to check that action type is valid
 * @param actionType rtti type of action
 * @return true if action is valid
 */
bool LC_ActionOptionsSupport::checkActionRttiValid([[maybe_unused]]RS2::ActionType actionType) {
    return true;
}


/**
 * Method is called from the action when options should be hidden.
 * Simply hides UI and saves settings.
 */
void LC_ActionOptionsSupport::hideOptions(){

}

/**
 * Just utility method for conversion of provided string to double value.
 * @param strValue string that represents value
 * @param res result of conversion
 * @param notMeaningful value that should be returned if given string denotes non-meaningful value (less than tolerance)
 * @param positiveOnly if true, positive value (via std::abs()) will be always returned, false - otherwise.
 * @return true if string was converted without errors
 */
bool LC_ActionOptionsSupport::toDouble(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
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
bool LC_ActionOptionsSupport::toDoubleAngleDegrees(const QString& strValue, double &res, const double notMeaningful, const bool positiveOnly){
    const bool ok = LC_Convert::parseToToDoubleAngleDegrees(strValue, res, notMeaningful, positiveOnly);
    return ok;
}

/**
 * Utility method for version of double value to string that will be displayed in editor
 * @param value double value
 * @return corresponding string
 */
QString LC_ActionOptionsSupport::fromDouble(double value){
    // here we'll convert to decimal degrees always...regardless of original input format.
    if (LC_LineMath::isNotMeaningful(value)){
        value = 0.0;
    }
    return QString::number(value, 'g', 6);
}
