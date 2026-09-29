/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_SETTINGGUARD_H
#define LC_SETTINGGUARD_H

#include <utility>

#include <QSettings>
#include <QString>
#include <QVariant>

#include "rs_settings.h"

namespace lc::test {

/**
 * Sets one setting for the length of a scope and puts back what was there: the
 * old value, or no value when the setting did not exist. A test that depends
 * on a setting pins it with one of these instead of trusting what earlier tests
 * of the process left behind.
 */
class SettingGuard {
public:
    SettingGuard(RS_Settings* settings, QString group, QString key)
        : m_settings(settings), m_group(std::move(group)), m_key(std::move(key)),
          m_fullKey(QString("/%1/%2").arg(m_group, m_key)),
          m_existed(settings->getSettings()->contains(m_fullKey)),
          m_value(settings->getSettings()->value(m_fullKey)) {}

    ~SettingGuard() {
        auto groupGuard = m_settings->beginGroupGuard(m_group);
        if (m_existed) {
            m_settings->write(m_key, m_value);
        }
        else {
            m_settings->write(m_key, QVariant{});
            m_settings->remove(m_key);
        }
    }

    void set(const bool value) const { m_settings->writeSingle(m_group, m_key, value); }
    void set(const int value) const { m_settings->writeSingle(m_group, m_key, value); }
    void set(const double value) const { m_settings->writeSingle(m_group, m_key, value); }
    void set(const QString& value) const { m_settings->writeSingle(m_group, m_key, value); }
    // a string literal would pick the bool overload
    void set(const char*) const = delete;

    SettingGuard(const SettingGuard&) = delete;
    SettingGuard& operator=(const SettingGuard&) = delete;

private:
    RS_Settings* m_settings;
    QString m_group;
    QString m_key;
    QString m_fullKey;
    bool m_existed;
    QVariant m_value;
};

} // namespace lc::test

#endif
