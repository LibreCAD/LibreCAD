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


#ifndef RS_SETTINGS_H
#define RS_SETTINGS_H

#include <QObject>
#include <QVariant>

class RS_Pen;
class QSettings;

// ---------------------------------------------------------------------------

#define RS_SETTINGS RS_Settings::instance()
#define LC_ALL_KEYS RS_Settings::instance()->getAllKeys
#define LC_REMOVE RS_Settings::instance()->remove
#define LC_CHILD_KEYS RS_Settings::instance()->getChildKeys
#define LC_SET RS_Settings::instance()->write
#define LC_SET_COLOR RS_Settings::instance()->writeColor
#define LC_SET_ONE RS_Settings::instance()->writeSingle
#define LC_GET_STR RS_Settings::instance()->readStr
#define LC_GET_ONE_STR RS_Settings::instance()->readStrSingle
#define LC_GET_INT RS_Settings::instance()->readInt
#define LC_GET_COLOR RS_Settings::instance()->readColor
#define LC_GET_ONE_INT RS_Settings::instance()->readIntSingle
#define LC_GET_BOOL RS_Settings::instance()->readBool
#define LC_GET_ONE_BOOL RS_Settings::instance()->readBoolSingle
#define LC_GET_BARRAY RS_Settings::instance()->readByteArray
#define LC_GROUP RS_Settings::instance()->beginGroup
#define LC_GROUP_GUARD auto _guard_settings = RS_Settings::instance()->beginGroupGuard
#define LC_GROUP_END RS_Settings::instance()->endGroup

/**
 * This class can store and reload settings from a 
 * configuration file or the windoze registry.
 * Please note that the Qt default implementation doesn't
 * work as one would expect. That's why this class overwrites
 * most of the default behaviour.
 * 
 */
class RS_Settings: public QObject {
    Q_OBJECT
public:
    // ---------------------------------------------------------------------------
    // Default Settings
    // ---------------------------------------------------------------------------
    static constexpr auto SNAP_INDICATOR          = "#FFC200";
    static constexpr auto SNAP_INDICATOR_LINES    = "#FFC200";
    static constexpr auto BACKGROUND              = "Black";
    static constexpr auto COLOR_GRID_POINTS       = "Gray";
    static constexpr auto COLOR_GRID_LINES        = "#aeaeff";
    static constexpr auto COLOR_META_GRID_POINTS  = "#404040";
    static constexpr auto COLOR_META_GRID_LINES   = "#55557f";
    static constexpr auto SELECT                  = "#A54747";
    static constexpr auto HIGHLIGHT         = "#739373";
    static constexpr auto START_HANDLE      = "Cyan";
    static constexpr auto HANDLE            = "Blue";
    static constexpr auto END_HANDLE        = "Blue";
    static constexpr auto RELATIVE_ZERO_COLOR = "Red";
    static constexpr auto X_AXIS_COLOR        = "Red";
    static constexpr auto Y_AXIS_COLOR        = "Green";
    static constexpr auto PREVIEW_REF_COLOR   = "Yellow";
    static constexpr auto PREVIEW_REF_HIGHLIGHT_COLOR = "Green";
    static constexpr auto OVERLAY_BOX_LINE_INVERTED = "#32ff32";
    static constexpr auto OVERLAY_BOX_FILL_INVERTED = "#09ff09";
    static constexpr auto OVERLAY_BOX_LINE = "#3232ff";
    static constexpr auto OVERLAY_BOX_FILL = "#0909ff";

    static constexpr auto OVERLAY_INFO_CURSOR_ABSOLUTE_POS = "Yellow";
    static constexpr auto OVERLAY_INFO_CURSOR_SNAP = "Cyan";
    static constexpr auto OVERLAY_INFO_CURSOR_RELATIVE_POS = "Orange";
    static constexpr auto OVERLAY_INFO_CURSOR_COMMAND_PROMPT = "Gray";

    static constexpr auto ANGLES_BASIS_DIRECTION = "#017CFF";
    static constexpr auto ANGLES_BASIS_ANGLE_RAY = "#00FFFF";
    static constexpr auto VISUAL_SNAP_VERTEXES = "#00ff00";
    static constexpr auto VISUAL_SNAP_ENTITIES = "#00ff00";
    static constexpr auto VISUAL_SNAP_PROJECTED_SNAP = "#00ffff";
    static constexpr auto VISUAL_SNAP_DOCUMENT_ENTITIES = "#00ff00";

    static constexpr auto RELATIVE_POSITION_BACKGROUND = BACKGROUND;
    static constexpr auto RELATIVE_POSITION_FONT = "Cyan";

    // Used to have RAII style GroupGuard: endGroup is called automatically whenever a unique_ptr<GroupGuard>
    // goes out of scope
    class GroupGuard {
    public:
        explicit GroupGuard(const QString &group);
        ~GroupGuard();
    private:
        QString m_group;
    };

    ~RS_Settings() override;

/**
    * @return Instance to the unique settings object.
    */

    static RS_Settings *instance();

    /**
     * Schema major version. Bumped only when LibreCAD's settings layout
     * changes incompatibly with a prior major release. Decoupled from
     * LC_VERSION on purpose: LC_VERSION carries git suffixes and
     * alpha/beta/rc labels that should not gate a settings migration.
     *
     * The QSettings application name is "LibreCAD-<N>" where N is this
     * constant; each major gets its own backing store and migrations
     * across majors copy from the highest-numbered prior store.
     */
    static constexpr int LC_SETTINGS_SCHEMA_MAJOR = 2;

    /**
     * Initialize the system.
     *
     * If `appKey` matches "LibreCAD-<integer>" and the resulting store
     * is empty, scans for a prior-major sibling store under the same
     * `companyKey` (highest match wins) and finally falls back to the
     * legacy un-versioned name "LibreCAD". On hit, copies the prior
     * store into the current one and stamps `_migratedFrom`,
     * `_schemaMajor`, `_schemaMinor` sentinels at the top level.
     *
     * @param companyKey Company Key
     * @param appKey Application key
     */
    static void init(const QString& companyKey, const QString& appKey);

    /**
     * Recursively copy all groups and keys from `src` to `dst`. Schema
     * meta-state keys at the top level (any key starting with "_") are
     * skipped so the prior major's meta-state does not pollute the new
     * store.
     */
    static void copyAll(QSettings* src, QSettings* dst);

    /**
     * Probe for a prior-major sibling store under `companyKey`. Returns
     * the app name that was copied from (e.g., "LibreCAD-1") and stamps
     * `_migratedFrom` / `_schemaMajor` / `_schemaMinor` on `dst`. Returns
     * an empty string if no prior store was found.
     *
     * Search order: "LibreCAD-(currentMajor-1)", "LibreCAD-(currentMajor-2)",
     * ..., "LibreCAD-1", then the legacy un-versioned "LibreCAD".
     */
    static QString migrateFromPriorMajor(const QString& companyKey,
                                         QSettings* dst,
                                         int currentMajor);

    // RAII style group guard: endGroup() is called automatically at the end of lifetime of the returned object
    std::unique_ptr<GroupGuard> beginGroupGuard(QString group);
    void beginGroup(const QString& group);
    void endGroup();

    bool write(const QString& key, int value);
    bool write(const QString& key, const QVariant& variant);
    bool writeColor(const QString& key, int value);
    bool write(const QString& key, double value);
    bool writeEntry(const QString& key, const QVariant& value);
    bool write(const QString& key, const QString& value);
    bool write(const QString& key, bool value);
    bool writeSingle(const QString &group, const QString &key, int value);
    bool writeSingle(const QString &group, const QString &key, const QString &value);
    bool writeSingle(const QString &group, const QString &key, double value);
    bool writeSingle(const QString &group, const QString &key, bool value);

    int readInt(const QString& key, int def= 0);
    int readIntSingle(const QString &group, const QString &key, int def);
    int readColor(const QString& key, int def= 0);
    int readColorSingle(const QString &group, const QString &key, int def);
    QString readStrSingle(const QString &group, const QString &key, const QString &def);
    QString readStr(const QString& key,const QString& def = QString());
    bool readBool(const QString &key, bool defaultValue = false);
    bool readBoolSingle(const QString& group, const QString &key, bool defaultValue = false);
    QByteArray readByteArraySingle(const QString &group, const QString &key) const;
    QByteArray readByteArray(const QString &key) const;

    void clearAll();
    void clearGeometry();

    static bool saveIsAllowed;

    void emitOptionsChanged();

    static void writePen(const QString& name, const RS_Pen&pen);
    static RS_Pen readPen(const QString& name, const RS_Pen &defaultPen);

    QSettings* getSettings() const {
        return m_settings;
    }

    QStringList getAllKeys() const;
    QStringList getChildKeys() const;

    void remove(const QString &key) const;

signals:
    void optionChanged(const QString& groupName, const QString &propertyName, QVariant oldValue, QVariant newValue);
    void optionsChanged();

private:
    explicit RS_Settings(QSettings *qsettings);
    QVariant readEntryCache(const QString& key);

protected:
    std::map<QString, QVariant> m_cache;
    QString m_group;
    QSettings *m_settings = nullptr;
    static inline RS_Settings* INSTANCE;

    bool writeEntrySingle(const QString &group, const QString &key, const QVariant &value);
    QString getFullName(const QString &group, const QString &key) const;
};

#endif
