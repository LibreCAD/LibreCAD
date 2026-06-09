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

#ifndef LC_ACTIONOPTIONSSUPPORT_H
#define LC_ACTIONOPTIONSSUPPORT_H

#include <QString>

#include "rs.h"

class RS_ActionInterface;

class LC_ActionOptionsSupport {
public:
    virtual ~LC_ActionOptionsSupport() = default;
    void setAction(RS_ActionInterface * a, bool update = false);
    virtual void hideOptions() = 0;
    /**
     * Extension point. Method allows action to request update of UI (say, by enabling or hiding some parts of the widget).
     * Mode value is action-specific and should be processed by related option widget.
     * @param mode
     * @param value
     * @param value
     */
    virtual void updateUI([[maybe_unused]]int mode, [[maybe_unused]]const QVariant* value){}
protected:

    /**
     * Setter for corresponding action
     * @param a action
     */
    virtual void doUpdateByAction(RS_ActionInterface* a) = 0;
    /**
     * Performs check that provided action type is accepted by options widget
     * @param actionType type of action
     * @return true if type is ok, false otherwise
     */
    // fixme - sand - review whether check for rtti is needed at all...
    virtual bool checkActionRttiValid(RS2::ActionType actionType);
    virtual void preSetupByAction(RS_ActionInterface* a) = 0;
    virtual void cleanup() = 0;

    // conversion utilities
    QString fromDouble(double value);
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    bool toDoubleAngleDegrees(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
};

#endif
