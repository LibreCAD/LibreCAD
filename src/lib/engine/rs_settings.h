/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <iostream>
#include <qglobal.h>

#include <qsettings.h>

#include "rs_string.h"
#include "rs_dict.h"

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
    void init(const RS_String& companyKey, const RS_String& appKey);

    void beginGroup(const RS_String& group);
    void endGroup();

    bool writeEntry(const RS_String& key, int value);
    bool writeEntry(const RS_String& key, double value);
    bool writeEntry(const RS_String& key, const QVariant& value);
    bool writeEntry(const RS_String& key, const QString& value);
    RS_String readEntry(const RS_String& key,
                        const RS_String& def = RS_String::null,
                        bool* ok = 0);
    QByteArray readByteArrayEntry(const RS_String& key,
                        const RS_String& def = RS_String::null,
                        bool* ok = 0);
    int readNumEntry(const RS_String& key, int def=0, bool* ok=0);
	

public:
    ~RS_Settings();

private:
	QVariant readEntryCache(const RS_String& key);
	void addToCache(const RS_String& key, const QVariant& value);

protected:
    static RS_Settings* uniqueInstance;

	RS_Dict<QVariant> cache;
    RS_String companyKey;
    RS_String appKey;
    RS_String group;
    bool initialized;
};

#endif

