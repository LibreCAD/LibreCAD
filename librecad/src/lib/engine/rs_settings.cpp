/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

// RVT_PORT changed QSettings s(QSettings::Ini) to QSettings s("./qcad.ini", QSettings::IniFormat);
#include <QSettings>

#include "rs_debug.h"
#include "rs_settings.h"

RS_Settings::GroupGuard::GroupGuard(const QString &group):m_group{group} {}

RS_Settings::GroupGuard::~GroupGuard(){
    try {
        const auto instance = RS_Settings::instance();
        instance ->endGroup();
        if (!m_group.isEmpty()) {
            RS_Settings::instance()-> beginGroup(m_group);
        }
    } catch (...) {
        LC_LOG(RS_Debug::D_CRITICAL) << "RS_SETTINGS cleanup error";
    }
}


bool RS_Settings::save_is_allowed = true;


RS_Settings *RS_Settings::instance() {
    return INSTANCE;
}

/**
 * Initialisation.
 *
 * @param companyKey String that identifies the company. Must start
 *        with a "/". E.g. "/RibbonSoft"
 * @param appKey String that identifies the application. Must start
 *        with a "/". E.g. "/LibreCAD"
 */
void RS_Settings::init(const QString &companyKey,const QString &appKey) {
    auto* settings = new QSettings(companyKey, appKey);
    INSTANCE = new RS_Settings(settings);
}

RS_Settings::RS_Settings(QSettings *qsettings) {
    settings = qsettings;
    m_group = "";
}

RS_Settings::~RS_Settings() {
    delete settings;
    cache.clear();
}


void RS_Settings::beginGroup(QString group)  {
    QString actualGroup = group;
    m_group = std::move(actualGroup);
}

void RS_Settings::endGroup()  {
    m_group.clear();
}

std::unique_ptr<RS_Settings::GroupGuard> RS_Settings::beginGroupGuard(QString group) {
    auto guard = std::make_unique<RS_Settings::GroupGuard>(std::move(m_group));
    m_group = std::move(group);
    return guard;
}

bool RS_Settings::write(const QString &key, int value) {
    return writeSingle(m_group, key, value);
}
bool RS_Settings::writeColor(const QString &key, int value) {    ;
    return writeEntry(key, QVariant(value % 0x80000000));
}

bool RS_Settings::writeSingle(const QString& group, const QString &key, int value) {
    return writeEntrySingle(group, key, QVariant(value));
}

bool RS_Settings::write(const QString &key, const QString &value) {
    return writeSingle(m_group, key, value);
}

bool RS_Settings::writeSingle(const QString& group, const QString &key, const QString &value) {
    return writeEntrySingle(group, key, QVariant(value));
}

bool RS_Settings::write(const QString &key, double value) {
    return writeSingle(m_group, key, value);
}

bool RS_Settings::writeSingle(const QString & group, const QString &key, double value) {
    return writeEntrySingle(group, key, QVariant(value));
}
bool RS_Settings::write(const QString &key, bool value) {
    return writeSingle(m_group, key, value);
}

bool RS_Settings::writeSingle(const QString &group, const QString &key, bool value) {
    return writeSingle(group, key, value ? 1 : 0);
}

bool RS_Settings::readBool(const QString &key, bool defaultValue) {
    return readBoolSingle(m_group, key, defaultValue);
}

bool RS_Settings::readBoolSingle(const QString &group, const QString &key, bool defaultValue) {
    int def = defaultValue ? 1 : 0;
    return readIntSingle(group, key, def) == 1;
}

bool RS_Settings::writeEntry(const QString &key, const QVariant &value) {
    return writeEntrySingle(m_group, key, QVariant(value));
}

QString RS_Settings::getFullName(const QString &group, const QString &key) const {
    const QString &fullName = QString("/%1/%2").arg(group, key);
    return fullName;
}

void RS_Settings::emitOptionsChanged() {
    emit optionsChanged();
}

QString RS_Settings::readStr(const QString &key,const QString &def) {
    return readStrSingle(m_group, key, def);
}

bool RS_Settings::writeEntrySingle(const QString& group, const QString &key, const QVariant &value) {
    QString fullName = getFullName(group, key);

    // Skip writing operations if the key is found in the cache and
    // its value is the same as the new one (it was already written).

    QVariant ret = readEntryCache(fullName);
    if (ret.isValid() && ret == value) {
        return true;
    }

    // RVT_PORT not supported anymore s.insertSearchPath(QSettings::Windows, companyKey);

    settings->setValue(fullName, value);
    cache[fullName] = value;

    // basically, that's a shortcut that we put value from cache as old value (instead of actual reading of it).
    // however, in most cases, properties will be read before modification, so that's fine
    emit optionChanged(m_group, key, ret, value);

    return true;
}

QString RS_Settings::readStrSingle(const QString& group, const QString &key,const QString &def) {
    QString fullName = getFullName(group, key);
    QVariant value = readEntryCache(fullName);
    if (!value.isValid()) {
        value = settings->value(fullName, QVariant(def));
        cache[fullName] = value;
    }    
    return value.toString();
}

int RS_Settings::readColor(const QString &key, int def) {
    return readColorSingle(m_group, key, def);
}


int RS_Settings::readColorSingle(const QString& group, const QString &key, int def) {
    QString fullName = getFullName(group, key);
    QVariant value = readEntryCache(fullName);
    if (!value.isValid()) {
        value = settings->value(fullName, QVariant(def));
        cache[fullName] = value;
    }
    unsigned long long uValue = value.toULongLong();
    uValue = uValue % 0x80000000ull;
    int result = int(uValue);
    return result;
}

int RS_Settings::readInt(const QString &key, int def) {
    return readIntSingle(m_group, key, def);
}

int RS_Settings::readIntSingle(const QString& group, const QString &key, int def) {
    QString fullName = getFullName(group, key);
    QVariant value = readEntryCache(fullName);
    if (!value.isValid()) {
        value = settings->value(fullName, QVariant(def));
        cache[fullName] = value;
    }
    int result = value.toInt();
    return result;
}

QVariant RS_Settings::readEntryCache(const QString &key) {
    if (cache.count(key) == 0) {
        return QVariant();
    }
    return cache[key];
}

void RS_Settings::clear_all() {
    settings->clear();
    cache.clear();
    save_is_allowed = false;
}

void RS_Settings::clear_geometry() {
    settings->remove("/Geometry");
    cache.clear();
    save_is_allowed = false;
}
