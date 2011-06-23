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

#include <QHash>
#include <QSettings>

#define RS_SETTINGS RS_Settings::instance()

/**
 * This class can store and reload settings from a 
 * configuration file or the windoze registry.
 * Please note that the Qt default implementation doesn't
 * work as one would expect. That's why this class overwrites
 * most of the default behaviour.
 * 
 */
class RS_Settings {
public:
    RS_Settings();

public:
    /**
     * @return Instance to the unique settings object.
     */
    static RS_Settings* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_Settings();
        }
        return uniqueInstance;
    }

    /**
     * Initialize the system.
     *
     * @param companyKey Company Key
     * @param appKey Application key
     */
    void init(const QString& companyKey, const QString& appKey);

    void beginGroup(const QString& group);
    void endGroup();

    bool writeEntry(const QString& key, int value);
    bool writeEntry(const QString& key, double value);
    bool writeEntry(const QString& key, const QVariant& value);
    bool writeEntry(const QString& key, const QString& value);
    QString readEntry(const QString& key,
                        const QString& def = QString::null,
                        bool* ok = 0);
    QByteArray readByteArrayEntry(const QString& key,
                        const QString& def = QString::null,
                        bool* ok = 0);
    int readNumEntry(const QString& key, int def=0, bool* ok=0);
	

public:
    ~RS_Settings();

private:
        QVariant readEntryCache(const QString& key);
        void addToCache(const QString& key, const QVariant& value);

protected:
    static RS_Settings* uniqueInstance;

    QHash<QString, QVariant*> cache;
    QString companyKey;
    QString appKey;
    QString group;
    bool initialized;
};

#endif

