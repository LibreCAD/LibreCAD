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

#ifndef LC_ACTIONOPTIONSBASE_H
#define LC_ACTIONOPTIONSBASE_H

#include <QString>
#include <rs_vector.h>

class LC_ActionOptions {
public:
    virtual ~LC_ActionOptions() = default;
    virtual void saveOptions() = 0;
    virtual void loadOptions() = 0;
};

class LC_ActionOptionsBase: public LC_ActionOptions{
public:
    LC_ActionOptionsBase(const QString& groupName, const QString& namePrefix) : m_optionsSettingsGroupName{groupName},
                                                                                m_settingsPrefix{namePrefix} {
    }
    void saveOptions() override;
    void loadOptions() override;
protected:
    // saving settings shortcut methods
    virtual void doSaveOptions() {}
    virtual void doLoadOptions() {}

    // conversion utilities
    QString fromDouble(double value);
    bool toDouble(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    bool toDoubleAngleDegrees(const QString &strValue, double &res, double notMeaningful, bool positiveOnly);
    void save(const QString& name, const QString& value);
    void save(const QString& name, int value);
    void save(const QString& name, bool value);
    void save(const QString& name, double value);
    void save(const QString& name, const RS_Vector& value);
    /**
   * Default name for settings group name
   * @return name of group
   */
    virtual QString getSettingsGroupName(){return m_optionsSettingsGroupName;}
    /**
     * Default name for prefix for settings. It assumes that all settings for the action starts with the same prefix.
     * @return  prefix to use.
     */
    virtual QString getSettingsOptionNamePrefix(){ return m_settingsPrefix;}

    // loading settings shortcut methods
    QString load(const QString& name, const QString& defaultValue);
    int loadInt(const QString& name, int defaultValue);
    bool loadBool(const QString& name, bool defaultValue);
    double loadDouble(const QString& name, double defaultValue);
    RS_Vector loadVector(const QString& name, const RS_Vector& defaultValue);

    QString m_optionsSettingsGroupName;
    QString m_settingsPrefix;
};

#endif
