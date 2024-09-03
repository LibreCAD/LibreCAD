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

#include <map>
#include <memory>

#include <QString>
#include <QObject>
#include <QVariant>

class QSettings;

// ---------------------------------------------------------------------------

#define RS_SETTINGS RS_Settings::instance()
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
    static constexpr char const* snap_indicator    = "#FFC200";
    static constexpr char const* background        = "Black";
    static constexpr char const* grid              = "Gray";
    static constexpr char const* meta_grid         = "#404040";
    static constexpr char const* select            = "#A54747";
    static constexpr char const* highlight         = "#739373";
    static constexpr char const* start_handle      = "Cyan";
    static constexpr char const* handle            = "Blue";
    static constexpr char const* end_handle        = "Blue";
    static constexpr char const* relativeZeroColor = "Red";
    static constexpr char const* previewRefColor = "Yellow";
    static constexpr char const* previewRefHighlightColor = "Green";

    // Used to have RAII style GroupGuard: endGroup is called automatically whenever a unique_ptr<GroupGuard>
    // goes out of scope
    class GroupGuard {
    public:
        explicit GroupGuard(const QString &group);
        ~GroupGuard();
    private:
        QString m_group;
    };

    virtual ~RS_Settings();

/**
    * @return Instance to the unique settings object.
    */

    static RS_Settings *instance();

    /**
     * Initialize the system.
     *
     * @param companyKey Company Key
     * @param appKey Application key
     */
    static void init(const QString& companyKey, const QString& appKey);

    // RAII style group guard: endGroup() is called automatically at the end of lifetime of the returned object
    std::unique_ptr<GroupGuard> beginGroupGuard(QString group);
    void beginGroup(QString group);
    void endGroup();

    bool write(const QString& key, int value);
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

    void clear_all();
    void clear_geometry();
    static bool save_is_allowed;

    void emitOptionsChanged();


signals:
    void optionChanged(const QString& groupName, const QString &propertyName, QVariant oldValue, QVariant newValue);
    void optionsChanged();

private:
    explicit RS_Settings(QSettings *qsettings);
    QVariant readEntryCache(const QString& key);

protected:
    std::map<QString, QVariant> cache;
    QString m_group;
    QSettings *settings;
    static inline RS_Settings* INSTANCE;

    bool writeEntrySingle(const QString &group, const QString &key, const QVariant &value);

    QString getFullName(const QString &group, const QString &key) const;
};

#endif
