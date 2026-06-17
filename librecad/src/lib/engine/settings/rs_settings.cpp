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
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

#include "rs_debug.h"
#include "rs_settings.h"

#include "rs_pen.h"

namespace {
    constexpr const char* G_KEY_MIGRATED_FROM = "_migratedFrom";
    constexpr const char* G_KEY_SCHEMA_MAJOR  = "_schemaMajor";
    constexpr const char* G_KEY_SCHEMA_MINOR  = "_schemaMinor";
    constexpr const char* G_LEGACY_APP_NAME   = "LibreCAD";

    // App names matching "LibreCAD-<integer>" are versioned production
    // stores; everything else (e.g. "LibreCAD-tests") opts out of
    // migration so unit tests don't inherit real user settings.
    bool isVersionedAppName(const QString& appKey) {
        static const QRegularExpression re(QStringLiteral("^LibreCAD-\\d+$"));
        return re.match(appKey).hasMatch();
    }

    // Cross-platform "does this QSettings store have any LibreCAD-owned
    // data?" check. allKeys() is NOT reliable here: on macOS, NSUserDefaults
    // falls through to the global domain so allKeys() returns dozens of
    // com/apple/* system keys even when our app-specific plist has never
    // been written.
    //
    // For file-backed formats (Linux .conf, macOS .plist, IniFormat) we
    // check the backing file's existence — Qt only creates the file on
    // sync() when there's data to write, so non-existence proves
    // emptiness.
    //
    // For the Windows registry path (Q_OS_WIN + NativeFormat), the registry
    // does not leak global keys, so childGroups()/childKeys() at the root
    // are reliable.
    bool isStoreEmpty(QSettings& s) {
#ifdef Q_OS_WIN
        if (s.format() == QSettings::NativeFormat) {
            return s.childGroups().isEmpty() && s.childKeys().isEmpty();
        }
#endif
        return !QFileInfo::exists(s.fileName());
    }
}

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

    // First-run migration: if this is a versioned production store and
    // it's empty, look for a prior-major sibling and copy its contents.
    // Test app names (e.g. "LibreCAD-tests") skip migration so test
    // runs don't inherit real user settings from prior majors.
    if (isVersionedAppName(appKey) && isStoreEmpty(*settings)) {
        migrateFromPriorMajor(companyKey, settings,
                              LC_SETTINGS_SCHEMA_MAJOR);
    }

    // Always stamp the current schema major so future tooling can
    // distinguish stores by major. The schema-minor field is the hook
    // for any within-major schema break a future patch needs to apply.
    settings->setValue(QLatin1String(G_KEY_SCHEMA_MAJOR),
                       LC_SETTINGS_SCHEMA_MAJOR);
    int schemaMinor = settings->value(
        QLatin1String(G_KEY_SCHEMA_MINOR), 0).toInt();
    // (no within-major migrations yet — when one lands, bump schemaMinor
    //  here as: if (schemaMinor < 1) { ...; schemaMinor = 1; })
    settings->setValue(QLatin1String(G_KEY_SCHEMA_MINOR), schemaMinor);

    INSTANCE = new RS_Settings(settings);
}

void RS_Settings::copyAll(QSettings* src, QSettings* dst) {
    // Copy keys at the current group level. Skip top-level keys
    // beginning with "_" — those are schema meta-state that must not
    // be inherited across major versions.
    const bool atRoot = src->group().isEmpty();
    for (const QString& key : src->childKeys()) {
        if (atRoot && key.startsWith(QLatin1Char('_'))) {
            continue;
        }
        dst->setValue(key, src->value(key));
    }
    // Recurse into child groups so arbitrarily-deep nesting is handled
    // (Qt's allKeys() returns flat slash-paths, but childGroups() is
    // one level at a time).
    for (const QString& group : src->childGroups()) {
        src->beginGroup(group);
        dst->beginGroup(group);
        copyAll(src, dst);
        dst->endGroup();
        src->endGroup();
    }
}

QString RS_Settings::migrateFromPriorMajor(const QString& companyKey,
                                            QSettings* dst,
                                            int currentMajor) {
    auto tryCopy = [&](const QString& priorApp) -> bool {
        QSettings prior(companyKey, priorApp);
        if (isStoreEmpty(prior)) {
            return false;
        }
        copyAll(&prior, dst);
        dst->setValue(QLatin1String(G_KEY_MIGRATED_FROM), priorApp);
        dst->setValue(QLatin1String(G_KEY_SCHEMA_MAJOR), currentMajor);
        // Reset within-major schema state — prior major's _schemaMinor
        // is meaningless under the new major's schema.
        dst->setValue(QLatin1String(G_KEY_SCHEMA_MINOR), 0);
        return true;
    };

    // Search versioned siblings, highest-numbered first.
    for (int n = currentMajor - 1; n >= 1; --n) {
        const QString priorApp = QStringLiteral("LibreCAD-%1").arg(n);
        if (tryCopy(priorApp)) {
            return priorApp;
        }
    }
    // Final fallback: the legacy un-versioned name — catches users
    // upgrading from any pre-versioning LibreCAD release.
    if (tryCopy(QLatin1String(G_LEGACY_APP_NAME))) {
        return QLatin1String(G_LEGACY_APP_NAME);
    }
    return QString();
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
        value = settings->value(fullName, QVariant(def)).toString();
        cache[fullName] = value;
    }
    return value.toString();
}

int RS_Settings::readColor(const QString &key, int def) {
    return readColorSingle(m_group, key, def);
}

QStringList RS_Settings::getAllKeys() const {
    return settings->allKeys();
}

QStringList RS_Settings::getChildKeys() const {
    QString currentGroup = settings->group();
    settings->beginGroup(m_group);
    auto result = settings->childKeys();
    settings->endGroup();
    settings->beginGroup(currentGroup);
    return result;
}

void RS_Settings::remove(const QString& key) const {
    QString fullName = getFullName(m_group, key);
    settings->remove(fullName);
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

QByteArray RS_Settings::readByteArray(const QString &key) {
    return readByteArraySingle(m_group, key);
}

QByteArray RS_Settings::readByteArraySingle(const QString& group, const QString &key) {
    QString fullName = getFullName(group, key);
    return settings->value(fullName, "").toByteArray();
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

void RS_Settings::writePen(QString name, RS_Pen const &pen){
    LC_SET("pen" + name + "Color", pen.getColor().name());
    LC_SET("pen" + name + "LineType", pen.getLineType());
    LC_SET("pen" + name + "Width", pen.getWidth());
}

RS_Pen RS_Settings::readPen(QString name, RS_Pen &defaultPen){
    RS_Color color = QColor(LC_GET_STR("pen" + name + "Color", defaultPen.getColor().name()));
    // FIXME - well, that's a bit ugly - if there is a mess in setting value, cast from int to short will be illegal. Need additional validation there?
    RS2::LineType lineType = static_cast<RS2::LineType> (LC_GET_INT("pen" + name + "LineType", defaultPen.getLineType()));
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth> (LC_GET_INT("pen" + name + "Width", defaultPen.getWidth()));

    RS_Pen result = RS_Pen(color, lineWidth, lineType);
    return result;
}
